#include "cache.h"
#include "vmem.h"

void CACHE::prefetcher_initialize() {}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
  constexpr int PTE_PYTES = 8;
  constexpr auto LOG2_PTE_SIZE = champsim::lg2(PTE_BYTES);
  
  if (!cache_hit) {
    uint64_t pf_addr = addr + (1  << LOG2_PAGE_SIZE);
    if (!warmup)
      prefetch_line(pf_addr, true, 1);

    uint64_t pf_addr = addr + (1  << (LOG2_PAGE_SIZE + LOG2_PAGE_SIZE / (LOG2_BLOCK_SIZE + LOG2_PTE_SIZE)));
    if (!warmup)
      prefetch_line(pf_addr, false, 11);
  }
  return metadata_in;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void CACHE::prefetcher_cycle_operate() {}

void CACHE::prefetcher_final_stats() {}
