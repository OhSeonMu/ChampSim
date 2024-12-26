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
#include "msl/lru_table.h"

namespace
{
constexpr static std::size_t MP_SETS = 256;
constexpr static std::size_t MP_WAYS = 1;
constexpr static std::size_t SLOT_SIZE = 4;

struct mp {
  struct mp_entry {
    uint64_t vpn;
    std::list<int64_t> lru_list; 

    auto index() const { return vpn; };
    auto tag() const { return vpn; };

    mp_entry() : mp_entry(0) {}
    explicit mp_entry(int64_t allocate_vpn) : vpn(allocate_vpn) {}

    void access_vpn(int64_t accessed_vpn) {
      lru_list.remove(accessed_vpn);
      if (lru_list.size() >= SLOT_SIZE)
        lru_list.pop_back();
      lru_list.push_front(accessed_vpn);
    }

    void prefetch_slot(uint64_t vpn, uint32_t metadata_in, CACHE* cache) const {
      for (const auto& pf_vpn : lru_list) {
        cache->prefetch_line(pf_vpn, true, metadata_in);
      }
    }

    void print_slot(int n) const {
      std::cout << "VPN[" << n << "] : " << vpn << " VPN_LIST: ";
      for (const auto& pf_vpn : lru_list) {
        std::cout << pf_vpn << " ";
      }
      std::cout << std::endl;
    }
  };

  uint64_t previous_vpn;
  champsim::msl::lru_table<mp_entry> mp_table{MP_SETS, MP_WAYS};
 
public:
  void initiate_lookahead(uint64_t vpn, uint32_t metadata_in, CACHE* cache) 
  {
    // current vpn  
    auto entry = mp_entry(vpn);
    auto found = mp_table.check_hit(entry);
    if (found.has_value()) {
      found->print_slot(0);
      found->prefetch_slot(vpn, metadata_in, cache);
    }
    else {
      mp_table.fill(entry);
    }

    // previsou vpn  
    auto previous_entry = mp_entry(previous_vpn);
    auto previous_found = mp_table.check_hit(previous_entry);
    if (previous_found.has_value()) {
      previous_found->access_vpn(vpn);
      mp_table.fill(*previous_found);
    }

    // update previous vpn 
    previous_vpn = vpn;
  }
};

std::map<CACHE*, mp> mps;
}

void CACHE::prefetcher_initialize() {}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
  metadata_in = 1;
  ::mps[this].initiate_lookahead(addr, metadata_in, this);
  return metadata_in;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void CACHE::prefetcher_cycle_operate() {}

void CACHE::prefetcher_final_stats() {}
