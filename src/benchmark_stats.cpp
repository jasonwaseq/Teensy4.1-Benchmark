#include "benchmark_stats.hpp"

#include <math.h>

#include "benchmark_config.hpp"

namespace bench {
namespace {

uint16_t percentile_index(uint16_t count, uint8_t percentile) {
  if (count == 0) {
    return 0;
  }
  const uint32_t rank = (static_cast<uint32_t>(percentile) * static_cast<uint32_t>(count) + 99u) / 100u;
  const uint32_t clamped = (rank == 0u) ? 1u : rank;
  return static_cast<uint16_t>(clamped - 1u);
}

void insertion_sort(uint32_t* values, uint16_t n) {
  for (uint16_t i = 1; i < n; ++i) {
    const uint32_t key = values[i];
    int16_t j = static_cast<int16_t>(i) - 1;
    while (j >= 0 && values[j] > key) {
      values[j + 1] = values[j];
      --j;
    }
    values[j + 1] = key;
  }
}

}  // namespace

StatsResult compute_stats(const StatsInput& input) {
  StatsResult out = {};
  if (input.samples == nullptr || input.count == 0) {
    out.valid = false;
    return out;
  }

  uint32_t sorted[kMaxTrials] = {};
  const uint16_t count = (input.count > kMaxTrials) ? kMaxTrials : input.count;

  double mean_acc = 0.0;
  for (uint16_t i = 0; i < count; ++i) {
    sorted[i] = input.samples[i];
    mean_acc += static_cast<double>(input.samples[i]);
  }
  insertion_sort(sorted, count);

  out.valid = true;
  out.min = sorted[0];
  out.max = sorted[count - 1];
  out.mean = mean_acc / static_cast<double>(count);

  if ((count & 1u) == 0u) {
    const uint32_t a = sorted[(count / 2u) - 1u];
    const uint32_t b = sorted[count / 2u];
    out.median = 0.5 * static_cast<double>(a + b);
  } else {
    out.median = static_cast<double>(sorted[count / 2u]);
  }

  double var_acc = 0.0;
  for (uint16_t i = 0; i < count; ++i) {
    const double d = static_cast<double>(sorted[i]) - out.mean;
    var_acc += d * d;
  }
  const double denom = (count > 1u) ? static_cast<double>(count - 1u) : 1.0;
  out.stddev = sqrt(var_acc / denom);

  out.p95 = sorted[percentile_index(count, 95)];
  out.p99 = sorted[percentile_index(count, 99)];
  return out;
}

}  // namespace bench

