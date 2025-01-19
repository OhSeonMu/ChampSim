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

#include <iostream>
#include <vector>

#include "cache.h"
#include "dram_controller.h"
#include "ooo_cpu.h"
#include "phase_info.h"

namespace champsim
{
class plain_printer
{
  std::ostream& stream;

  void print(O3_CPU::stats_type);
  void print(CACHE::stats_type);
  void print(DRAM_CHANNEL::stats_type);

  template <typename T>
  void print(std::vector<T> stats_list)
  {
    for (auto& stats : stats_list)
      print(stats);
  }

public:
  plain_printer(std::ostream& str) : stream(str) {}
  void print(phase_stats& stats);
  void print(std::vector<phase_stats>& stats);
};

// TODO[OSM] : To track hit/miss in cache
class plain_printer_csv
{
  std::ostream& stream;

  void print(O3_CPU::stats_type);
  void print(CACHE::stats_type);
  void print(DRAM_CHANNEL::stats_type);

  template <typename T>
  void print(std::vector<T> stats_list)
  {
    for (auto& stats : stats_list)
      print(stats);
  }
  
  // TODO[OSM] : For Prefetcher hit in PTW
  void print_prefetcher(CACHE::stats_type);
  
  template <typename T>
  void print_prefetcher(std::vector<T> stats_list)
  {
    for (auto& stats : stats_list)
      print_prefetcher(stats);
  }

  // TODO[OSM] : cache miss latency in not prefetch 
  void print_latency(CACHE::stats_type);
  // TODO[OSM] : Breakdown latency
  void print_latency(DRAM_CHANNEL::stats_type);
  
  template <typename T>
  void print_latency(std::vector<T> stats_list)
  {
    for (auto& stats : stats_list)
      print_latency(stats);
  }

  // TODO[OSM] : cache miss latency in not prefetch 
  void print_dram(DRAM_CHANNEL::stats_type);
  
  template <typename T>
  void print_dram(std::vector<T> stats_list)
  {
    for (auto& stats : stats_list)
      print_dram(stats);
  }

public:
  plain_printer_csv(std::ostream& str) : stream(str) {}
  
  void print(phase_stats& stats);
  void print(std::vector<phase_stats>& stats);
  
  // TODO[OSM] : For Prefetcher hit in PTW
  void print_prefetcher(phase_stats& stats);
  void print_prefetcher(std::vector<phase_stats>& stats);
  
  // TODO[OSM] : cache miss latency in not prefetch 
  void print_latency(phase_stats& stats);
  void print_latency(std::vector<phase_stats>& stats);
  
  // TODO[OSM] : row buffer 
  void print_dram(phase_stats& stats);
  void print_dram(std::vector<phase_stats>& stats);
};

class json_printer
{
  std::ostream& stream;

public:
  json_printer(std::ostream& str) : stream(str) {}
  void print(std::vector<phase_stats>& stats);
};
} // namespace champsim
