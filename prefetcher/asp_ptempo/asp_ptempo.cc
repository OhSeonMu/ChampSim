#include <map>

#include "cache.h"
#include "vmem.h"
#include "msl/lru_table.h"

namespace
{
struct asp {
  struct asp_entry {
    uint64_t ip = 0;
    uint64_t vpn = 0;
    int64_t stride = 0;
    int64_t state = 0;

    auto index() const { return ip; };
    auto tag() const { return ip; };
  };

  constexpr static std::size_t ASP_SETS = 64;
  constexpr static std::size_t ASP_WAYS = 4;

  champsim::msl::lru_table<asp_entry> table{ASP_SETS, ASP_WAYS};
  uint64_t counter[64] = {0,};
 
public:
  uint64_t initiate_lookahead(uint64_t ip, uint64_t vpn) 
  {
    uint64_t stride = 0;
    uint64_t state = 0;
    auto found = table.check_hit({ip, vpn, stride, state});

    if (found.has_value()) {
      stride = static_cast<int64_t>(vpn) - static_cast<int64_t>(found->vpn);

      if (stride != 0 && stride == found->stride)
	state = (found->state < 2) ? found->state + 1 : found->state;
    }

    table.fill({ip, vpn, stride, state});

    uint64_t delta = (state == 2) ? stride : 0;
    return delta;
  }
  uint64_t offset_count(uint64_t offset) 
  {
    counter[offset]++;
    uint64_t max_offset = std::max_element(counter, counter + 64) - counter;
    return max_offset;
  }
};

std::map<CACHE*, asp> asps;
}

void CACHE::prefetcher_initialize() {}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
  auto delta = ::asps[this].initiate_lookahead(ip, addr >> LOG2_PAGE_SIZE);
  auto offset = ::asps[this].offset_count((addr >> LOG2_BLOCK_SIZE) & champsim::bitmask(LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE));
  // metadata_in = 1;
  if (delta != 0) {
    // uint64_t pf_addr = addr + (delta << LOG2_PAGE_SIZE);
    uint64_t pf_addr = addr + (delta << LOG2_PAGE_SIZE) + (offset << LOG2_BLOCK_SIZE);
    // if (this->vmem->check_va_to_pa(this->cpu, pf_addr) & !warmup)
    // if (this->vmem->check_va_to_pa(this->cpu, pf_addr))
    if (!(this->warmup))
      prefetch_line(pf_addr, true, 1);
  }
  return metadata_in;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void CACHE::prefetcher_cycle_operate() {}

void CACHE::prefetcher_final_stats() {}
