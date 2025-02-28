#include <map>
#include <iostream>
#include <list>
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <optional>
#include <cstdint>
#include <cassert>

#include "cache.h"
#include "vmem.h"
#include "msl/lru_table.h"

namespace
{
constexpr static std::size_t MP_SETS = 64;
constexpr static std::size_t MP_WAYS = 1;
constexpr static std::size_t SLOT_SIZE = 1;

struct dp {
  struct dp_entry {
    int64_t distance;
    std::list<int64_t> lru_list; 

    auto index() const { return distance; };
    auto tag() const { return distance; };

    dp_entry() : dp_entry(0) {}
    explicit dp_entry(int64_t allocate_distance) : distance(allocate_distance) {}

    void access_distance(int64_t accessed_distance) {
      lru_list.remove(accessed_distance);
      if (lru_list.size() >= SLOT_SIZE)
        lru_list.pop_back();
      lru_list.push_front(accessed_distance);
    }

    void prefetch_slot(uint64_t vpn, uint32_t metadata_in, CACHE* cache) const {
      for (const auto& d : lru_list) {
	uint64_t pf_vpn = vpn + (d << LOG2_PAGE_SIZE);
	// if (this->vmem->check_va_to_pa(this->cpu, pf_addr) & !warmup)
	if (!(cache->warmup))
	  cache->prefetch_line(pf_vpn, true, 1);
      }
    }

    void print_slot(int n) const {
      std::cout << "DISTANCE[" << n << "] : " << distance << " VPN_LIST: ";
      for (const auto& d : lru_list) {
        std::cout << d << " ";
      }
      std::cout << std::endl;
    }
  };

  uint64_t previous_vpn;
  int64_t previous_distance;
  champsim::msl::lru_table<dp_entry> dp_table{MP_SETS, MP_WAYS};
 
public:
  void initiate_lookahead(uint64_t vpn, uint32_t metadata_in, CACHE* cache) 
  {
    int64_t distance = ((static_cast<int64_t>(vpn) - static_cast<int64_t>(previous_vpn)) >> LOG2_PAGE_SIZE);
    
    // current vpn  
    auto entry = dp_entry(distance);
    auto found = dp_table.check_hit(entry);
    if (found.has_value()) {
      found->prefetch_slot(vpn, metadata_in, cache);
    }
    else {
      dp_table.fill(entry);
    }

    // previsou vpn  
    auto previous_entry = dp_entry(previous_distance);
    auto previous_found = dp_table.check_hit(previous_entry);
    if (previous_found.has_value()) {
      previous_found->access_distance(distance);
      dp_table.fill(*previous_found);
    }

    // update previous vpn 
    previous_vpn = vpn;
    previous_distance = distance;
  }
};

std::map<CACHE*, dp> dps;
}

void CACHE::prefetcher_initialize() {}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
  ::dps[this].initiate_lookahead(addr, metadata_in, this);
  return metadata_in;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void CACHE::prefetcher_cycle_operate() {}

void CACHE::prefetcher_final_stats() {}
