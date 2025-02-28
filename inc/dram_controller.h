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

#ifndef DRAM_H
#define DRAM_H

#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string>

#include "champsim_constants.h"
#include "channel.h"
#include "operable.h"

struct dram_stats {
  std::string name{};
  uint64_t dbus_cycle_congested = 0, dbus_count_congested = 0;

  unsigned WQ_ROW_BUFFER_HIT = 0, WQ_ROW_BUFFER_MISS = 0, RQ_ROW_BUFFER_HIT = 0, RQ_ROW_BUFFER_MISS = 0, WQ_FULL = 0;
  
  // TODO[OSM] : Breakdown latency
  unsigned RQ_FULL = 0;

  // TODO[OSM] : Breakdown latency
  uint64_t total_access;

  double avg_initiate_request_latency = 0;
  double avg_bank_request_latency = 0 ;
  double avg_active_request_latency = 0;

  uint64_t total_initiate_request_latency = 0;
  uint64_t total_bank_request_latency = 0 ;
  uint64_t total_active_request_latency = 0;
  
  uint64_t total_ret_access;
  
  double avg_ret_initiate_request_latency = 0;
  double avg_ret_bank_request_latency = 0 ;
  double avg_ret_active_request_latency = 0;

  uint64_t total_ret_initiate_request_latency = 0;
  uint64_t total_ret_bank_request_latency = 0 ;
  uint64_t total_ret_active_request_latency = 0;
  
  // TODO[OSM] : Breakdown latency
  unsigned bank_access_success = 0, bank_access_fail = 0;
};

struct DRAM_CHANNEL {
  using response_type = typename champsim::channel::response_type;
  struct request_type {
    bool scheduled = false;
    bool forward_checked = false;
    
    // TODO[OSM] : Prefetch TLB
    bool scheduled_checked = false;

    uint8_t asid[2] = {std::numeric_limits<uint8_t>::max(), std::numeric_limits<uint8_t>::max()};

    uint32_t pf_metadata = 0;

    uint64_t address = 0;
    uint64_t v_address = 0;
    uint64_t data = 0;
    uint64_t event_cycle = std::numeric_limits<uint64_t>::max();

    std::vector<std::reference_wrapper<ooo_model_instr>> instr_depend_on_me{};
    std::vector<std::deque<response_type>*> to_return{};

    // TODO[OSM] : Breakdown latency  
    uint64_t initiate_request_cycle = 0;
    uint64_t bank_request_cycle = 0 ;
    uint64_t active_request_cycle = 0;

    explicit request_type(typename champsim::channel::request_type);
  };
  using value_type = request_type;
  using queue_type = std::vector<std::optional<value_type>>;
  queue_type WQ{DRAM_WQ_SIZE}, RQ{DRAM_RQ_SIZE};

  struct BANK_REQUEST {
    bool valid = false, row_buffer_hit = false;

    std::size_t open_row = std::numeric_limits<uint32_t>::max();

    uint64_t event_cycle = 0;

    queue_type::iterator pkt;
  };

  using request_array_type = std::array<BANK_REQUEST, DRAM_RANKS * DRAM_BANKS>;
  request_array_type bank_request = {};
  request_array_type::iterator active_request = std::end(bank_request);

  bool write_mode = false;
  uint64_t dbus_cycle_available = 0;

  using stats_type = dram_stats;
  stats_type roi_stats, sim_stats;

  // TODO[OSM] : Breakdown latency
  // void check_collision();
  void check_collision(uint64_t cycle);
  void print_deadlock();
};

class MEMORY_CONTROLLER : public champsim::operable
{
  using channel_type = champsim::channel;
  using request_type = typename channel_type::request_type;
  using response_type = typename channel_type::response_type;
  std::vector<channel_type*> queues;

  // Latencies
  const uint64_t tRP, tRCD, tCAS, DRAM_DBUS_TURN_AROUND_TIME, DRAM_DBUS_RETURN_TIME;

  // TODO[OSM] : Idle Memory Latency
  const bool idle_memory;

  // these values control when to send out a burst of writes
  constexpr static std::size_t DRAM_WRITE_HIGH_WM = ((DRAM_WQ_SIZE * 7) >> 3);         // 7/8th
  constexpr static std::size_t DRAM_WRITE_LOW_WM = ((DRAM_WQ_SIZE * 6) >> 3);          // 6/8th
  constexpr static std::size_t MIN_DRAM_WRITES_PER_SWITCH = ((DRAM_WQ_SIZE * 1) >> 2); // 1/4

  void initiate_requests();
  bool add_rq(const request_type& pkt, champsim::channel* ul);
  bool add_wq(const request_type& pkt);

public:
  std::array<DRAM_CHANNEL, DRAM_CHANNELS> channels;

  // TODO[OSM] : Idle Memory Latency
  MEMORY_CONTROLLER(double freq_scale, int io_freq, double t_rp, double t_rcd, double t_cas, double turnaround, std::vector<channel_type*>&& ul, bool idle_memory);

  void initialize() override final;
  long operate() override final;
  void begin_phase() override final;
  void end_phase(unsigned cpu) override final;
  void print_deadlock() override final;

  std::size_t size() const;

  uint32_t dram_get_channel(uint64_t address);
  uint32_t dram_get_rank(uint64_t address);
  uint32_t dram_get_bank(uint64_t address);
  uint32_t dram_get_row(uint64_t address);
  uint32_t dram_get_column(uint64_t address);
};

#endif
