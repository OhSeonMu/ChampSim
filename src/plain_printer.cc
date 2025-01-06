/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <numeric>
#include <sstream>
#include <utility>
#include <vector>

#include "stats_printer.h"
#include <fmt/core.h>
#include <fmt/ostream.h>

void champsim::plain_printer::print(O3_CPU::stats_type stats)
{
  constexpr std::array<std::pair<std::string_view, std::size_t>, 6> types{
      {std::pair{"BRANCH_DIRECT_JUMP", BRANCH_DIRECT_JUMP}, std::pair{"BRANCH_INDIRECT", BRANCH_INDIRECT}, std::pair{"BRANCH_CONDITIONAL", BRANCH_CONDITIONAL},
       std::pair{"BRANCH_DIRECT_CALL", BRANCH_DIRECT_CALL}, std::pair{"BRANCH_INDIRECT_CALL", BRANCH_INDIRECT_CALL},
       std::pair{"BRANCH_RETURN", BRANCH_RETURN}}};

  auto total_branch = std::ceil(
      std::accumulate(std::begin(types), std::end(types), 0ll, [tbt = stats.total_branch_types](auto acc, auto next) { return acc + tbt[next.second]; }));
  auto total_mispredictions = std::ceil(
      std::accumulate(std::begin(types), std::end(types), 0ll, [btm = stats.branch_type_misses](auto acc, auto next) { return acc + btm[next.second]; }));

  fmt::print(stream, "\n{} cumulative IPC: {:.4g} instructions: {} cycles: {}\n", stats.name, std::ceil(stats.instrs()) / std::ceil(stats.cycles()),
             stats.instrs(), stats.cycles());
  fmt::print(stream, "{} Branch Prediction Accuracy: {:.4g}% MPKI: {:.4g} Average ROB Occupancy at Mispredict: {:.4g}\n", stats.name,
             (100.0 * std::ceil(total_branch - total_mispredictions)) / total_branch, (1000.0 * total_mispredictions) / std::ceil(stats.instrs()),
             std::ceil(stats.total_rob_occupancy_at_branch_mispredict) / total_mispredictions);

  std::vector<double> mpkis;
  std::transform(std::begin(stats.branch_type_misses), std::end(stats.branch_type_misses), std::back_inserter(mpkis),
                 [instrs = stats.instrs()](auto x) { return 1000.0 * std::ceil(x) / std::ceil(instrs); });

  fmt::print(stream, "Branch type MPKI\n");
  for (auto [str, idx] : types)
    fmt::print(stream, "{}: {:.3}\n", str, mpkis[idx]);
  fmt::print(stream, "\n");
}

void champsim::plain_printer::print(CACHE::stats_type stats)
{
  // TODO[OSM] : To track hit/miss in cache
  // constexpr std::array<std::pair<std::string_view, std::size_t>, 5> types{
  constexpr std::array<std::pair<std::string_view, std::size_t>, 9> types{
      {std::pair{"LOAD", champsim::to_underlying(access_type::LOAD)}, std::pair{"RFO", champsim::to_underlying(access_type::RFO)},
       std::pair{"PREFETCH", champsim::to_underlying(access_type::PREFETCH)}, std::pair{"WRITE", champsim::to_underlying(access_type::WRITE)},
       // TODO[OSM] : To track hit/miss in cache
       //std::pair{"TRANSLATION", champsim::to_underlying(access_type::TRANSLATION)}}};
       std::pair{"L5_TRANSLATION", champsim::to_underlying(access_type::L5_TRANSLATION)},
       std::pair{"L4_TRANSLATION", champsim::to_underlying(access_type::L4_TRANSLATION)},
       std::pair{"L3_TRANSLATION", champsim::to_underlying(access_type::L3_TRANSLATION)},
       std::pair{"L2_TRANSLATION", champsim::to_underlying(access_type::L2_TRANSLATION)},
       std::pair{"L1_TRANSLATION", champsim::to_underlying(access_type::L1_TRANSLATION)}}};

  for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu) {
    uint64_t TOTAL_HIT = 0, TOTAL_MISS = 0;
    for (const auto& type : types) {
      TOTAL_HIT += stats.hits.at(type.second).at(cpu);
      TOTAL_MISS += stats.misses.at(type.second).at(cpu);
    }

    fmt::print(stream, "{} TOTAL        ACCESS: {:10d} HIT: {:10d} MISS: {:10d}\n", stats.name, TOTAL_HIT + TOTAL_MISS, TOTAL_HIT, TOTAL_MISS);
    for (const auto& type : types) {
      fmt::print(stream, "{} {:<12s} ACCESS: {:10d} HIT: {:10d} MISS: {:10d}\n", stats.name, type.first,
                 stats.hits[type.second][cpu] + stats.misses[type.second][cpu], stats.hits[type.second][cpu], stats.misses[type.second][cpu]);
    }

    fmt::print(stream, "{} PREFETCH REQUESTED: {:10} ISSUED: {:10} USEFUL: {:10} USELESS: {:10}\n", stats.name, stats.pf_requested, stats.pf_issued,
               stats.pf_useful, stats.pf_useless);

    // TODO[OSM] : For Prefetcher hit in PTW
    fmt::print(stream, "{} USEFUL: {:10} L5_USEFUL: {:10} L4_USEFUL: {:10} L3_USEFUL: {:10} L2_USEFUL: {:10} L1_USEFUL: {:10} \n", stats.name, stats.pf_useful, stats.pf_l5_useful, stats.pf_l4_useful, stats.pf_l3_useful, stats.pf_l2_useful, 
	       stats.pf_l1_useful);

    fmt::print(stream, "{} AVERAGE MISS LATENCY: {:.4g} cycles\n", stats.name, stats.avg_miss_latency);
    // TODO[OSM] : cache miss latency in not prefetch
    fmt::print(stream, "{} AVERAGE NOT PREFETCH MISS LATENCY: {:.4g} cycles\n", stats.name, stats.avg_not_prefetch_miss_latency);
  }
}

void champsim::plain_printer::print(DRAM_CHANNEL::stats_type stats)
{
  fmt::print(stream, "\n{} RQ ROW_BUFFER_HIT: {:10}\n  ROW_BUFFER_MISS: {:10}\n", stats.name, stats.RQ_ROW_BUFFER_HIT, stats.RQ_ROW_BUFFER_MISS);
  if (stats.dbus_count_congested > 0) fmt::print(stream, " AVG DBUS CONGESTED CYCLE: {:.4g}\n", std::ceil(stats.dbus_cycle_congested) / std::ceil(stats.dbus_count_congested)); else fmt::print(stream, " AVG DBUS CONGESTED CYCLE: -\n"); fmt::print(stream, "WQ ROW_BUFFER_HIT: {:10}\n  ROW_BUFFER_MISS: {:10}\n  FULL: {:10}\n", stats.name, stats.WQ_ROW_BUFFER_HIT, stats.WQ_ROW_BUFFER_MISS, stats.WQ_FULL); } void champsim::plain_printer::print(champsim::phase_stats& stats) { fmt::print(stream, "=== {} ===\n", stats.name); 
  int i = 0;
  for (auto tn : stats.trace_names)
    fmt::print(stream, "CPU {} runs {}", i++, tn);

  if (NUM_CPUS > 1) {
    fmt::print(stream, "\nTotal Simulation Statistics (not including warmup)\n");

    for (const auto& stat : stats.sim_cpu_stats)
      print(stat);

    for (const auto& stat : stats.sim_cache_stats)
      print(stat);
  }

  fmt::print(stream, "\nRegion of Interest Statistics\n");

  for (const auto& stat : stats.roi_cpu_stats)
    print(stat);

  for (const auto& stat : stats.roi_cache_stats)
    print(stat);

  fmt::print(stream, "\nDRAM Statistics\n");
  for (const auto& stat : stats.roi_dram_stats)
    print(stat);
}

void champsim::plain_printer::print(std::vector<phase_stats>& stats)
{
  for (auto p : stats)
    print(p);
}

// TODO[OSM] : To track hit/miss in cache
void champsim::plain_printer_csv::print(O3_CPU::stats_type stats) {
    return;
}

// TODO[OSM] : To track hit/miss in cache
void champsim::plain_printer_csv::print(CACHE::stats_type stats)
{
  // TODO[OSM] : To track hit/miss in cache
  // constexpr std::array<std::pair<std::string_view, std::size_t>, 5> types{
  constexpr std::array<std::pair<std::string_view, std::size_t>, 9> types{
      {std::pair{"LOAD", champsim::to_underlying(access_type::LOAD)}, std::pair{"RFO", champsim::to_underlying(access_type::RFO)},
       std::pair{"PREFETCH", champsim::to_underlying(access_type::PREFETCH)}, std::pair{"WRITE", champsim::to_underlying(access_type::WRITE)},
       // TODO[OSM] : To track hit/miss in cache
       //std::pair{"TRANSLATION", champsim::to_underlying(access_type::TRANSLATION)}}};
       std::pair{"L5_TRANSLATION", champsim::to_underlying(access_type::L5_TRANSLATION)},
       std::pair{"L4_TRANSLATION", champsim::to_underlying(access_type::L4_TRANSLATION)},
       std::pair{"L3_TRANSLATION", champsim::to_underlying(access_type::L3_TRANSLATION)},
       std::pair{"L2_TRANSLATION", champsim::to_underlying(access_type::L2_TRANSLATION)},
       std::pair{"L1_TRANSLATION", champsim::to_underlying(access_type::L1_TRANSLATION)}}};
  
  // TODO[OSM] : To track hit/miss in cache
  for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu) {
    uint64_t TOTAL_HIT = 0, TOTAL_MISS = 0;
    for (const auto& type : types) {
      TOTAL_HIT += stats.hits.at(type.second).at(cpu);
      TOTAL_MISS += stats.misses.at(type.second).at(cpu);
    }

    fmt::print(stream, "{},TOTAL,{},{},{}\n", stats.name, TOTAL_HIT + TOTAL_MISS, TOTAL_HIT, TOTAL_MISS);
    for (const auto& type : types) {
      fmt::print(stream, "{},{},{},{},{}\n", stats.name, type.first,
                 stats.hits[type.second][cpu] + stats.misses[type.second][cpu], stats.hits[type.second][cpu], stats.misses[type.second][cpu]);
    }
  }
}

// TODO[OSM] : To track hit/miss in cache
void champsim::plain_printer_csv::print(DRAM_CHANNEL::stats_type stats) {
	return;
}

// TODO[OSM] : cache miss latency in not prefetch
void champsim::plain_printer_csv::print_latency(CACHE::stats_type stats)
{
  for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu) {
    fmt::print(stream, "{},{},{}\n", 
		    stats.name, 
		    stats.avg_miss_latency, stats.avg_not_prefetch_miss_latency);
    /*
    fmt::print(stream, "{},{},{},{},{}\n", 
		    stats.name, 
		    stats.avg_miss_latency, stats.avg_not_prefetch_miss_latency, 
		    stats.total_miss_latency, stats.total_not_prefetch_miss_latency);
    */
  }
}

// TODO[OSM] : For Prefetcher hit in PTW
void champsim::plain_printer_csv::print_prefetcher(CACHE::stats_type stats)
{
  for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu) {
    fmt::print(stream, "{},{},{},{},{},{},{},{},{},{}\n", 
		    stats.name, stats.pf_requested, stats.pf_issued,stats.pf_useful, stats.pf_useless,
		    stats.pf_l5_useful, stats.pf_l4_useful, stats.pf_l3_useful, stats.pf_l2_useful, stats.pf_l1_useful);
  }
}

// TODO[OSM] : To track hit/miss in cache
void champsim::plain_printer_csv::print(champsim::phase_stats& stats)
{
  // TODO[OSM] : cache miss latency in not prefetch
  fmt::print(stream, "\n=== {} LATENCY CSV ===\n", stats.name);
  fmt::print(stream, "CACHE,AVG_MISS_LATENCY,NOT_PREFETCH_AVG_MISS_LATENCY\n");
  // fmt::print(stream, "CACHE,AVG_MISS_LATENCY,NOT_PREFETCH_AVG_MISS_LATENCY,TOT_MISS_LATENCY,NOT_PREFETCH_TOT_MISS_LATENCY\n");
  if (NUM_CPUS > 1) {
    for (const auto& stat : stats.sim_cache_stats)
      print_latency(stat);
  }
  
  for (const auto& stat : stats.roi_cache_stats)
    print_latency(stat);

  // TODO[OSM] : For Prefetcher hit in PTW
  fmt::print(stream, "\n=== {} PREFETCH CSV ===\n", stats.name);
  fmt::print(stream, "CACHE,REQUESTED,ISSUED,USEFUL,USELESS,L5_USEFUL,L4_USEFUL,L3_USEFUL,L2_USEFUL,L1_USEFUL\n");
  
  // TODO[OSM] : For Prefetcher hit in PTW
  if (NUM_CPUS > 1) {
    for (const auto& stat : stats.sim_cache_stats)
      print_prefetcher(stat);
  }
  
  // TODO[OSM] : For Prefetcher hit in PTW
  for (const auto& stat : stats.roi_cache_stats)
    print_prefetcher(stat);
  
  fmt::print(stream, "\n=== {} CSV ===\n", stats.name);
  fmt::print(stream, "CACHE,TYPE,TOTAL,HIT,MISS\n");

  /*
  int i = 0;
  for (auto tn : stats.trace_names)
    fmt::print(stream, "CPU {} runs {}\n", i++, tn);
  */

  if (NUM_CPUS > 1) {
    /*
    fmt::print(stream, "\nTotal Simulation Statistics (not including warmup)\n");

    for (const auto& stat : stats.sim_cpu_stats)
      print(stat);
    */
    for (const auto& stat : stats.sim_cache_stats)
      print(stat);
  }

  // fmt::print(stream, "\nRegion of Interest Statistics\n");

  /*
  for (const auto& stat : stats.roi_cpu_stats)
    print(stat);
  */
  
  for (const auto& stat : stats.roi_cache_stats)
    print(stat);
  
  /*
  fmt::print(stream, "\nDRAM Statistics\n");
  for (const auto& stat : stats.roi_dram_stats)
    print(stat);
  */
  
  /*
  fmt::print(stream, "\nSLOW DRAM Statistics\n");
  for (const auto& stat : stats.roi_slow_dram_stats)
    print(stat);
  */
}

// TODO[OSM] : To track hit/miss in cache
void champsim::plain_printer_csv::print(std::vector<phase_stats>& stats)
{
  for (auto p : stats)
    print(p);
}
