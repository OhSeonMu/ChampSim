#include <bitset>
#include <map>
#include <vector>
#include <iostream>

#include "cache.h"
#include "vmem.h"

namespace
{
constexpr std::size_t REGION_COUNT = 128;
constexpr int MAX_DISTANCE = 256;
constexpr int PREFETCH_DEGREE = 2;

// TODO[OSM] : for tlb prefetcher
constexpr int PTE_BYTES = 8;

// TODO[OSM] : Analyze ampm prefetcher in AT prefetch
int64_t at_hit = 0;
int64_t at_miss = 0;

auto print_stats() {
  std::cout << "AMPM AT STATS" << std::endl;
  std::cout << "TOTAL_ACCESS,HIT,MISS" << std::endl;
  std::cout << at_hit + at_miss << "," << at_hit << "," << at_miss << std::endl;
}

struct region_type {
  uint64_t vpn;
  std::bitset<PAGE_SIZE / PTE_BYTES> access_map{};
  std::bitset<PAGE_SIZE / PTE_BYTES> prefetch_map{};
  uint64_t lru;

  static uint64_t region_lru;

  region_type() : region_type(0) {}
  explicit region_type(uint64_t allocate_vpn) : vpn(allocate_vpn), lru(region_lru++) {}
};
uint64_t region_type::region_lru = 0;

std::map<CACHE*, std::array<region_type, REGION_COUNT>> regions;

auto page_and_offset(uint64_t addr)
{
  auto page_number = addr >> LOG2_PAGE_SIZE;
  auto page_offset = (addr & champsim::msl::bitmask(LOG2_PAGE_SIZE)) >> LOG2_BLOCK_SIZE;
  return std::pair{page_number, page_offset};
}

// TODO[OSM] : for tlb prefetcher
auto pt_and_offset(uint64_t addr)
{
  auto level = 2;
  auto page_number = addr >> (LOG2_PAGE_SIZE + champsim::lg2(PAGE_SIZE / PTE_BYTES) * (level - 1));
  auto page_offset = 
	  (addr & champsim::msl::bitmask(LOG2_PAGE_SIZE + champsim::lg2(PAGE_SIZE / PTE_BYTES) * (level - 1))) >> LOG2_PAGE_SIZE;
  return std::pair{page_number, page_offset};
}

bool check_cl_access(CACHE* cache, uint64_t v_addr)
{
  auto [vpn, page_offset] = pt_and_offset(v_addr);
  auto region = std::find_if(std::begin(regions.at(cache)), std::end(regions.at(cache)), [vpn = vpn](auto x) { return x.vpn == vpn; });

  return (region != std::end(regions.at(cache))) && region->access_map.test(page_offset);
}

// TODO[OSM] : for at cache prefetcher
bool check_cl_block_access(CACHE* cache, uint64_t v_addr)
{
  auto [vpn, page_offset] = pt_and_offset(v_addr);
  auto base_page_offset = page_offset - (page_offset % (PAGE_SIZE / (BLOCK_SIZE * PTE_BYTES)));
  for (uint64_t i = 0; i < PAGE_SIZE / (BLOCK_SIZE * PTE_BYTES); i++) {
    page_offset = base_page_offset + i;
    auto region = std::find_if(std::begin(regions.at(cache)), std::end(regions.at(cache)), 
		    [vpn = vpn](auto x) { return x.vpn == vpn; });
    if ((region != std::end(regions.at(cache))) && region->access_map.test(page_offset))
      return true;
  }

  return false;
}

bool check_cl_prefetch(CACHE* cache, uint64_t v_addr)
{
  auto [vpn, page_offset] = pt_and_offset(v_addr);
  auto region = std::find_if(std::begin(regions.at(cache)), std::end(regions.at(cache)), [vpn = vpn](auto x) { return x.vpn == vpn; });

  return (region != std::end(regions.at(cache))) && region->prefetch_map.test(page_offset);
}

// TODO[OSM] : for at cache prefetcher
bool check_cl_block_prefetch(CACHE* cache, uint64_t v_addr)
{
  auto [vpn, page_offset] = pt_and_offset(v_addr);
  auto base_page_offset = page_offset - (page_offset % (PAGE_SIZE / (BLOCK_SIZE * PTE_BYTES)));
  for (uint64_t i = 0; i < PAGE_SIZE / (BLOCK_SIZE * PTE_BYTES); i++) {
    page_offset = base_page_offset + i;
    auto region = std::find_if(std::begin(regions.at(cache)), std::end(regions.at(cache)), 
		    [vpn = vpn](auto x) { return x.vpn == vpn; });
    if ((region != std::end(regions.at(cache))) && region->prefetch_map.test(page_offset))
      return true;
  }

  return false;
}

} // anonymous namespace

void CACHE::prefetcher_initialize() { regions.insert_or_assign(this, decltype(regions)::mapped_type{}); }

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
  auto [current_vpn, page_offset] = ::pt_and_offset(addr);
  auto demand_region = std::find_if(std::begin(::regions.at(this)), std::end(::regions.at(this)), [vpn = current_vpn](auto x) { return x.vpn == vpn; });

  if (demand_region == std::end(::regions.at(this))) {
    // not tracking this region yet, so replace the LRU region
    demand_region = std::min_element(std::begin(::regions.at(this)), std::end(::regions.at(this)), [](auto x, auto y) { return x.lru < y.lru; });
    *demand_region = region_type{current_vpn};
    return metadata_in;
  }

  // TODO[OSM] : Analyze ampm prefetcher in AT prefetch
  if (demand_region->access_map.test(page_offset))
    at_hit++;
  else
    at_miss++;

  // mark this demand access
  demand_region->access_map.set(page_offset);

  // attempt to prefetch in the positive, then negative direction
  for (auto direction : {1, -1}) {
    for (int i = 1, prefetches_issued = 0; i <= MAX_DISTANCE && prefetches_issued < PREFETCH_DEGREE; i++) {
      const auto pos_step_addr = addr + direction * (i * (signed)PAGE_SIZE);
      const auto neg_step_addr = addr - direction * (i * (signed)PAGE_SIZE);
      const auto neg_2step_addr = addr - direction * (2 * i * (signed)PAGE_SIZE);

      if (::check_cl_access(this, neg_step_addr) && ::check_cl_access(this, neg_2step_addr) && !::check_cl_access(this, pos_step_addr)
          && !::check_cl_prefetch(this, pos_step_addr)) {
        // found something that we should prefetch
        if ((addr >> LOG2_PAGE_SIZE) != (pos_step_addr >> LOG2_PAGE_SIZE)) {
          bool prefetch_success = prefetch_line(pos_step_addr, 1, metadata_in);
          if (prefetch_success) {
            auto [pf_vpn, pf_page_offset] = ::page_and_offset(pos_step_addr);
            auto pf_region = std::find_if(std::begin(::regions.at(this)), std::end(::regions.at(this)), [vpn = pf_vpn](auto x) { return x.vpn == vpn; });

            if (pf_region == std::end(::regions.at(this))) {
              // we're not currently tracking this region, so allocate a new region so we can mark it
              pf_region = std::min_element(std::begin(::regions.at(this)), std::end(::regions.at(this)), [](auto x, auto y) { return x.lru < y.lru; });
              *pf_region = region_type{pf_vpn};
            }

            pf_region->prefetch_map.set(pf_page_offset);
            prefetches_issued++;
          }
        }
      }
    }
  }
  
  // TODO[OSM] : for at cache prefetcher
  // attempt to prefetch in the positive, then negative direction
  for (auto direction : {1, -1}) {
    for (int i = 1, prefetches_issued = 0; i <= MAX_DISTANCE && prefetches_issued < PREFETCH_DEGREE; i++) {
      const auto pos_step_addr = addr + direction * (i * (signed)(PAGE_SIZE + BLOCK_SIZE));
      const auto neg_step_addr = addr - direction * (i * (signed)(PAGE_SIZE + BLOCK_SIZE));
      const auto neg_2step_addr = addr - direction * (2 * i * (signed)(PAGE_SIZE + BLOCK_SIZE));

      if (::check_cl_block_access(this, neg_step_addr) && ::check_cl_block_access(this, neg_2step_addr) 
		      && !::check_cl_block_access(this, pos_step_addr) && !::check_cl_block_prefetch(this, pos_step_addr)) {
        // found something that we should prefetch
        if ((addr >> LOG2_PAGE_SIZE) != (pos_step_addr >> LOG2_PAGE_SIZE)) {
          bool prefetch_success = prefetch_line(pos_step_addr, 0, 11);
          if (prefetch_success) {
            prefetches_issued++;
          }
        }
      }
    }
  }

  return metadata_in;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void CACHE::prefetcher_cycle_operate() {}

void CACHE::prefetcher_final_stats() {
  // TODO[OSM] : Analyze ampm prefetcher in AT prefetch
  ::print_stats();
}
