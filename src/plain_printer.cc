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
  if (stats.dbus_count_congested > 0) 
    fmt::print(stream, " AVG DBUS CONGESTED CYCLE: {:.4g}\n", std::ceil(stats.dbus_cycle_congested) / std::ceil(stats.dbus_count_congested)); 
  else 
    fmt::print(stream, " AVG DBUS CONGESTED CYCLE: -\n"); 
  
  fmt::print(stream, "\n{} WQ ROW_BUFFER_HIT: {:10}\n  ROW_BUFFER_MISS: {:10}\n  FULL: {:10}\n", stats.name, stats.WQ_ROW_BUFFER_HIT, stats.WQ_ROW_BUFFER_MISS, stats.WQ_FULL); 
}
  
void champsim::plain_printer::print(champsim::phase_stats& stats) { 
  fmt::print(stream, "=== {} ===\n", stats.name); 
  
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

// TODO[OSM] : Row Buffer Affect
void champsim::plain_printer_csv::print_dram(DRAM_CHANNEL::stats_type stats) {
  fmt::print(stream, "{},{},{},{},{},{},", 
	stats.RQ_ROW_BUFFER_HIT, stats.RQ_ROW_BUFFER_MISS, stats.WQ_ROW_BUFFER_HIT, 
	stats.WQ_ROW_BUFFER_MISS, stats.RQ_FULL, stats.WQ_FULL); 

  if (stats.dbus_count_congested > 0) 
    fmt::print(stream, "{},{},{:.4g},{},{},{},{}\n", 
	std::ceil(stats.dbus_cycle_congested),std::ceil(stats.dbus_count_congested),
	std::ceil(stats.dbus_cycle_congested) / std::ceil(stats.dbus_count_congested), std::ceil(stats.bank_access_success), 
	std::ceil(stats.bank_access_fail), std::ceil(stats.total_access), std::ceil(stats.total_ret_access)); 
  else 
    fmt::print(stream, "NaN,NaN,Nan,{},{},{},{}\n", std::ceil(stats.bank_access_success), 
		    std::ceil(stats.bank_access_fail), std::ceil(stats.total_access), std::ceil(stats.total_ret_access)); 
 }

// TODO[OSM] : cache miss latency in not prefetch
void champsim::plain_printer_csv::print_latency(CACHE::stats_type stats)
{
  for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu) {
    /*
    fmt::print(stream, "{},{},{}\n", 
		    stats.name, 
		    stats.avg_miss_latency, stats.avg_not_prefetch_miss_latency);
    */
    fmt::print(stream, "{},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{},{},{}\n", 
		    stats.name, stats.avg_miss_latency, stats.avg_not_prefetch_miss_latency, stats.avg_prefetch_miss_latency, 
		    stats.avg_initiate_tag_check_latency, stats.avg_handle_miss_latency, stats.avg_finish_packet_latency,
		    stats.total_miss_latency, stats.total_not_prefetch_miss_latency, stats.total_prefetch_miss_latency);
  }
}

// TODO[OSM] : Breakdown latency
void champsim::plain_printer_csv::print_latency(DRAM_CHANNEL::stats_type stats)
{
  for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu) {
    fmt::print(stream, "{:.2f},{:.2f},{:.2f},{:.2f},{},{},{},{},{:.2f},{:.2f},{:.2f},{:.2f},{},{},{},{}\n", 
		    stats.avg_initiate_request_latency + stats.avg_bank_request_latency + stats.avg_active_request_latency,
		    stats.avg_initiate_request_latency, stats.avg_bank_request_latency, stats.avg_active_request_latency,
		    stats.total_initiate_request_latency + stats.total_bank_request_latency + stats.total_active_request_latency,
		    stats.total_initiate_request_latency, stats.total_bank_request_latency, stats.total_active_request_latency,
		    stats.avg_ret_initiate_request_latency + stats.avg_ret_bank_request_latency + stats.avg_ret_active_request_latency,
		    stats.avg_ret_initiate_request_latency, stats.avg_ret_bank_request_latency, stats.avg_ret_active_request_latency,
		    stats.total_ret_initiate_request_latency + stats.total_ret_bank_request_latency + stats.total_ret_active_request_latency,
		    stats.total_ret_initiate_request_latency, stats.total_ret_bank_request_latency, stats.total_ret_active_request_latency); 
  }
}

// TODO[OSM] : For Prefetcher hit in PTW
void champsim::plain_printer_csv::print_prefetcher(CACHE::stats_type stats)
{
  for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu) {
    fmt::print(stream, "{},{},{},{},{},{},{},{},{},{},{}\n", 
		    stats.name, stats.pf_requested, stats.pf_issued, stats.pf_useful, stats.pf_useful_on_going, stats.pf_useless,
		    stats.pf_l5_useful, stats.pf_l4_useful, stats.pf_l3_useful, stats.pf_l2_useful, stats.pf_l1_useful);
  }
}

// TODO[OSM] : Check Access Block 
void champsim::plain_printer_csv::print_distribution(CACHE::stats_type stats)
{
  for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu) {
    fmt::print(stream, "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n", 
		    stats.name, 
		    stats.access_cache_block[0],
		    stats.access_cache_block[1],
		    stats.access_cache_block[2],
		    stats.access_cache_block[3],
		    stats.access_cache_block[4],
		    stats.access_cache_block[5],
		    stats.access_cache_block[6],
		    stats.access_cache_block[7],
		    stats.access_cache_block[8],
		    stats.access_cache_block[9],
		    stats.access_cache_block[10],
		    stats.access_cache_block[11],
		    stats.access_cache_block[12],
		    stats.access_cache_block[13],
		    stats.access_cache_block[14],
		    stats.access_cache_block[15],
		    stats.access_cache_block[16],
		    stats.access_cache_block[17],
		    stats.access_cache_block[18],
		    stats.access_cache_block[19],
		    stats.access_cache_block[20],
		    stats.access_cache_block[21],
		    stats.access_cache_block[22],
		    stats.access_cache_block[23],
		    stats.access_cache_block[24],
		    stats.access_cache_block[25],
		    stats.access_cache_block[26],
		    stats.access_cache_block[27],
		    stats.access_cache_block[28],
		    stats.access_cache_block[29],
		    stats.access_cache_block[30],
		    stats.access_cache_block[31],
		    stats.access_cache_block[32],
		    stats.access_cache_block[33],
		    stats.access_cache_block[34],
		    stats.access_cache_block[35],
		    stats.access_cache_block[36],
		    stats.access_cache_block[37],
		    stats.access_cache_block[38],
		    stats.access_cache_block[39],
		    stats.access_cache_block[40],
		    stats.access_cache_block[41],
		    stats.access_cache_block[42],
		    stats.access_cache_block[43],
		    stats.access_cache_block[44],
		    stats.access_cache_block[45],
		    stats.access_cache_block[46],
		    stats.access_cache_block[47],
		    stats.access_cache_block[48],
		    stats.access_cache_block[49],
		    stats.access_cache_block[50],
		    stats.access_cache_block[51],
		    stats.access_cache_block[52],
		    stats.access_cache_block[53],
		    stats.access_cache_block[54],
		    stats.access_cache_block[55],
		    stats.access_cache_block[56],
		    stats.access_cache_block[57],
		    stats.access_cache_block[58],
		    stats.access_cache_block[59],
		    stats.access_cache_block[60],
		    stats.access_cache_block[61],
		    stats.access_cache_block[62],
		    stats.access_cache_block[63]);
  }
}

// TODO[OSM] : To track hit/miss in cache
void champsim::plain_printer_csv::print(champsim::phase_stats& stats)
{
  // TODO[OSM] : dram information
  fmt::print(stream, "\n=== {} DARM STATE CSV ===\n", stats.name);
  fmt::print(stream, "RQ_ROW_BUFFER_HIT,RQ_ROW_BUFFER_MISS,WQ_ROW_BUFFER_HIT,WQ_ROW_BUFFER_MISS,RQ_FULL,WQ_FULL,DBUS_CONGESTED_CYCLE,DBUS_CONGESTED_COUNT,AVG_DBUS_CONGESTED_CYCLE,BANK_ACCESS_SUCCESS,BANK_ACCESS_FAIL,TOTAL_ACCESS,TOTAL_RET_ACCESS\n");
  for (const auto& stat : stats.roi_dram_stats)
    print_dram(stat);

  // TODO[OSM] : Breakdown latency  
  fmt::print(stream, "\n=== {} AVG DARM LATENCY CSV ===\n", stats.name);
  fmt::print(stream, "MISS,INIT_REQUEST,BANK_REQUEST,ACTIVE_REQUEST,TOT_MISS,TOT_INIT_REQUEST,TOT_BANK_REQUEST,TOT_ACTIVE_REQUEST,RET_MISS,RET_INIT_REQUEST,RET_BANK_REQUEST,RET_ACTIVE_REQUEST,TOT_RET_MISS,TOT_RET_INIT_REQUEST,TOT_RET_BANK_REQUEST,TOT_RET_ACTIVE_REQUEST\n");
  for (const auto& stat : stats.roi_dram_stats)
    print_latency(stat);

  // TODO[OSM] : cache miss latency in not prefetch
  // TODO[OSM] : Breakdown latency  
  fmt::print(stream, "\n=== {} AVG MISS LATENCY CSV ===\n", stats.name);
  fmt::print(stream, "CACHE,MISS,NOT_PREFETCH,PREFETCH,TAG_CHECK,HANDLE_MISS,FINISH_PACKET,TOT_MISS,TOT_NOT_PREFETCH,TOT_PREFETCH\n");
  if (NUM_CPUS > 1) {
    for (const auto& stat : stats.sim_cache_stats)
      print_latency(stat);
  }
  
  for (const auto& stat : stats.roi_cache_stats)
    print_latency(stat);

  // TODO[OSM] : For Prefetcher hit in PTW
  fmt::print(stream, "\n=== {} PREFETCH CSV ===\n", stats.name);
  fmt::print(stream, "CACHE,REQUESTED,ISSUED,USEFUL,USEFUL_ONGOING,USELESS,L5_USEFUL,L4_USEFUL,L3_USEFUL,L2_USEFUL,L1_USEFUL\n");
  
  // TODO[OSM] : For Prefetcher hit in PTW
  if (NUM_CPUS > 1) {
    for (const auto& stat : stats.sim_cache_stats)
      print_prefetcher(stat);
  }
  
  // TODO[OSM] : For Prefetcher hit in PTW
  for (const auto& stat : stats.roi_cache_stats)
    print_prefetcher(stat);
  
  // TODO[OSM] : Check Access Block 
  fmt::print(stream, "\n=== {} DISTRIBUTION CSV ===\n", stats.name);
  fmt::print(stream, "CACHE,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63\n");
  
  // TODO[OSM] : Check Access Block 
  if (NUM_CPUS > 1) {
    for (const auto& stat : stats.sim_cache_stats)
      print_distribution(stat);
  }
  
  // TODO[OSM] : Check Access Block 
  for (const auto& stat : stats.roi_cache_stats)
    print_distribution(stat);
  
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
