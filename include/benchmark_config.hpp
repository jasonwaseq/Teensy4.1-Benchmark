#pragma once

#include <stdint.h>

#include "benchmark_result.hpp"

#ifndef BENCH_ENABLE_DMA
#define BENCH_ENABLE_DMA 0
#endif

#ifndef BENCH_ENABLE_DOUBLE
#define BENCH_ENABLE_DOUBLE 1
#endif

#ifndef BENCH_ENABLE_OPTIONAL
#define BENCH_ENABLE_OPTIONAL 1
#endif

namespace bench {

constexpr uint32_t kCpuHzFallback = 600000000u;
constexpr uint16_t kDefaultWarmups = 8u;
constexpr uint16_t kDefaultTrials = 41u;
constexpr uint32_t kTargetMinCycles = 200000u;
constexpr uint32_t kMinIters = 16u;
constexpr uint32_t kMaxIters = (1u << 24);
constexpr uint16_t kMaxBenchmarks = 192u;
constexpr uint16_t kMaxTrials = 64u;
constexpr uint32_t kDeterministicSeed = 0xC0FFEE01u;

struct RuntimeConfig {
  uint16_t warmups;
  uint16_t trials;
  uint32_t target_min_cycles;
  uint32_t min_iters;
  uint32_t max_iters;
  InterruptMode interrupt_mode;
  bool emit_json;
  bool emit_summary;
  bool distribution_mode;
  bool setup_each_trial_in_distribution;
  bool include_optional;
  bool auto_run_on_boot;
  bool filter_by_category;
  Category category_filter;
  bool filter_by_name;
  char name_filter[64];
  char run_id[24];
};

RuntimeConfig default_runtime_config();
void set_run_id(RuntimeConfig& cfg, uint32_t epoch_hint_ms);

}  // namespace bench
