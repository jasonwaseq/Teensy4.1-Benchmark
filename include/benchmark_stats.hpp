#pragma once

#include <stdint.h>

namespace bench {

struct StatsInput {
  const uint32_t* samples;
  uint16_t count;
};

struct StatsResult {
  bool valid;
  uint32_t min;
  uint32_t max;
  double mean;
  double median;
  double stddev;
  uint32_t p95;
  uint32_t p99;
};

StatsResult compute_stats(const StatsInput& input);

}  // namespace bench

