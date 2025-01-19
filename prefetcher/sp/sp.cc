#include "cache.h"
#include "vmem.h"

void CACHE::prefetcher_initialize() {}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
  // metadata_in = 1;
  if (!cache_hit) {
    uint64_t pf_addr = addr + (1  << LOG2_PAGE_SIZE);
    // if (this->vmem->check_va_to_pa(this->cpu, pf_addr))
    if (!warmup)
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
