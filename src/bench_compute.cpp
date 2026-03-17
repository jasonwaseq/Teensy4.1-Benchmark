#include "benchmark_registry.hpp"

#include <math.h>

#include "benchmark_config.hpp"
#include "placement_attrs.hpp"

namespace bench {
namespace {

constexpr uint32_t kMaxComputeN = 2048u;

BENCH_ALIGN(32) int32_t s_i32_a[kMaxComputeN];
BENCH_ALIGN(32) int32_t s_i32_b[kMaxComputeN];
BENCH_ALIGN(32) int32_t s_i32_out[kMaxComputeN];

BENCH_ALIGN(32) float s_f32_a[kMaxComputeN];
BENCH_ALIGN(32) float s_f32_b[kMaxComputeN];

#if BENCH_ENABLE_DOUBLE
BENCH_ALIGN(32) double s_f64_a[kMaxComputeN];
BENCH_ALIGN(32) double s_f64_b[kMaxComputeN];
#endif

BENCH_ALIGN(32) int16_t s_q15_a[kMaxComputeN];
BENCH_ALIGN(32) int16_t s_q15_b[kMaxComputeN];

uint32_t compute_n(const BenchmarkParamSet& params) {
  const uint32_t n = param_u32(params, "n", 1024u);
  return (n > kMaxComputeN) ? kMaxComputeN : n;
}

bool setup_common(BenchmarkContext& ctx, const BenchmarkParamSet& params) {
  const uint32_t n = compute_n(params);
  fill_u32_pattern(reinterpret_cast<uint32_t*>(s_i32_a), n, kDeterministicSeed ^ 0x11u);
  fill_u32_pattern(reinterpret_cast<uint32_t*>(s_i32_b), n, kDeterministicSeed ^ 0x22u);
  for (uint32_t i = 0; i < n; ++i) {
    s_i32_a[i] = (s_i32_a[i] & 0x3FFFF) - 0x1FFFF;
    s_i32_b[i] = (s_i32_b[i] & 0x3FFFF) - 0x1FFFF;
    s_f32_a[i] = static_cast<float>(s_i32_a[i]) * 0.0001f;
    s_f32_b[i] = static_cast<float>(s_i32_b[i]) * 0.0002f;
#if BENCH_ENABLE_DOUBLE
    s_f64_a[i] = static_cast<double>(s_i32_a[i]) * 0.00001;
    s_f64_b[i] = static_cast<double>(s_i32_b[i]) * 0.00002;
#endif
    s_q15_a[i] = static_cast<int16_t>(s_i32_a[i] & 0x7FFF);
    s_q15_b[i] = static_cast<int16_t>(s_i32_b[i] & 0x7FFF);
  }
  ctx.last_u32 = 0u;
  ctx.last_u64 = 0u;
  ctx.last_f32 = 0.0f;
  ctx.last_f64 = 0.0;
  return true;
}

BENCH_NOINLINE void run_int_add(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                uint32_t iterations) {
  const uint32_t n = compute_n(params);
  uint32_t hash = 0xA511E9B3u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      const int32_t v = s_i32_a[i] + s_i32_b[i];
      s_i32_out[i] = v;
      hash = mix_hash_u32(hash, static_cast<uint32_t>(v));
    }
  }
  ctx.last_u32 = hash;
  bench_consume_u32(hash);
}

bool validate_int_add(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = compute_n(params);
  uint32_t hash = 0xA511E9B3u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      hash = mix_hash_u32(hash, static_cast<uint32_t>(s_i32_a[i] + s_i32_b[i]));
    }
  }
  return hash == ctx.last_u32;
}

BENCH_NOINLINE void run_int_mul(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                uint32_t iterations) {
  const uint32_t n = compute_n(params);
  uint32_t hash = 0x32AA10F1u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      const int32_t v = s_i32_a[i] * s_i32_b[i];
      s_i32_out[i] = v;
      hash = mix_hash_u32(hash, static_cast<uint32_t>(v));
    }
  }
  ctx.last_u32 = hash;
  bench_consume_u32(hash);
}

bool validate_int_mul(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = compute_n(params);
  uint32_t hash = 0x32AA10F1u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      hash = mix_hash_u32(hash, static_cast<uint32_t>(s_i32_a[i] * s_i32_b[i]));
    }
  }
  return hash == ctx.last_u32;
}

BENCH_NOINLINE void run_int_mac(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                uint32_t iterations) {
  const uint32_t n = compute_n(params);
  int64_t acc = 0x10203;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc += static_cast<int64_t>(s_i32_a[i]) * static_cast<int64_t>(s_i32_b[i]);
    }
  }
  ctx.last_u64 = static_cast<uint64_t>(acc);
  bench_consume_u64(static_cast<uint64_t>(acc));
}

bool validate_int_mac(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = compute_n(params);
  int64_t acc = 0x10203;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc += static_cast<int64_t>(s_i32_a[i]) * static_cast<int64_t>(s_i32_b[i]);
    }
  }
  return static_cast<uint64_t>(acc) == ctx.last_u64;
}

BENCH_NOINLINE void run_int_div(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                uint32_t iterations) {
  const uint32_t n = compute_n(params);
  int64_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      const int32_t denom = (s_i32_b[i] & 0x7FF) + 1;
      acc += static_cast<int64_t>(s_i32_a[i] / denom);
    }
  }
  ctx.last_u64 = static_cast<uint64_t>(acc);
  bench_consume_u64(static_cast<uint64_t>(acc));
}

bool validate_int_div(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = compute_n(params);
  int64_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      const int32_t denom = (s_i32_b[i] & 0x7FF) + 1;
      acc += static_cast<int64_t>(s_i32_a[i] / denom);
    }
  }
  return static_cast<uint64_t>(acc) == ctx.last_u64;
}

BENCH_NOINLINE void run_bitwise(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                uint32_t iterations) {
  const uint32_t n = compute_n(params);
  uint32_t hash = 0xCC003311u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      const uint32_t ua = static_cast<uint32_t>(s_i32_a[i]);
      const uint32_t ub = static_cast<uint32_t>(s_i32_b[i]);
      const uint32_t v = (ua & ub) ^ ((ua << 3) | (ub >> 2));
      hash = mix_hash_u32(hash, v);
    }
  }
  ctx.last_u32 = hash;
  bench_consume_u32(hash);
}

bool validate_bitwise(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = compute_n(params);
  uint32_t hash = 0xCC003311u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      const uint32_t ua = static_cast<uint32_t>(s_i32_a[i]);
      const uint32_t ub = static_cast<uint32_t>(s_i32_b[i]);
      const uint32_t v = (ua & ub) ^ ((ua << 3) | (ub >> 2));
      hash = mix_hash_u32(hash, v);
    }
  }
  return hash == ctx.last_u32;
}

BENCH_NOINLINE void run_popcount(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                 uint32_t iterations) {
  const uint32_t n = compute_n(params);
  uint32_t total = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      total += static_cast<uint32_t>(__builtin_popcount(static_cast<uint32_t>(s_i32_a[i])));
    }
  }
  ctx.last_u32 = total;
  bench_consume_u32(total);
}

bool validate_popcount(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = compute_n(params);
  uint32_t total = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      total += static_cast<uint32_t>(__builtin_popcount(static_cast<uint32_t>(s_i32_a[i])));
    }
  }
  return total == ctx.last_u32;
}

BENCH_NOINLINE void run_crc_like(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                 uint32_t iterations) {
  const uint32_t n = compute_n(params);
  uint32_t crc = 0xFFFFFFFFu;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      crc ^= static_cast<uint32_t>(s_i32_a[i]);
      for (uint8_t b = 0; b < 8; ++b) {
        const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1u)));
        crc = (crc >> 1) ^ (0xEDB88320u & mask);
      }
    }
  }
  ctx.last_u32 = crc;
  bench_consume_u32(crc);
}

bool validate_crc_like(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = compute_n(params);
  uint32_t crc = 0xFFFFFFFFu;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      crc ^= static_cast<uint32_t>(s_i32_a[i]);
      for (uint8_t b = 0; b < 8; ++b) {
        const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1u)));
        crc = (crc >> 1) ^ (0xEDB88320u & mask);
      }
    }
  }
  return crc == ctx.last_u32;
}

BENCH_NOINLINE void run_float_add(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                  uint32_t iterations) {
  const uint32_t n = compute_n(params);
  float acc = 0.0f;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc += (s_f32_a[i] + s_f32_b[i]);
    }
  }
  ctx.last_f32 = acc;
  bench_consume_f32(acc);
}

bool validate_float_add(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = compute_n(params);
  float acc = 0.0f;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc += (s_f32_a[i] + s_f32_b[i]);
    }
  }
  const float diff = fabsf(acc - ctx.last_f32);
  return diff < (fabsf(acc) * 1e-5f + 1e-4f);
}

BENCH_NOINLINE void run_float_mul(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                  uint32_t iterations) {
  const uint32_t n = compute_n(params);
  float acc = 0.0f;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc += (s_f32_a[i] * s_f32_b[i]);
    }
  }
  ctx.last_f32 = acc;
  bench_consume_f32(acc);
}

bool validate_float_mul(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = compute_n(params);
  float acc = 0.0f;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc += (s_f32_a[i] * s_f32_b[i]);
    }
  }
  const float diff = fabsf(acc - ctx.last_f32);
  return diff < (fabsf(acc) * 1e-5f + 1e-4f);
}

BENCH_NOINLINE void run_float_mac(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                  uint32_t iterations) {
  const uint32_t n = compute_n(params);
  float acc = 0.5f;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc = (acc + s_f32_a[i] * s_f32_b[i]) * 0.99991f;
    }
  }
  ctx.last_f32 = acc;
  bench_consume_f32(acc);
}

bool validate_float_mac(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = compute_n(params);
  float acc = 0.5f;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc = (acc + s_f32_a[i] * s_f32_b[i]) * 0.99991f;
    }
  }
  const float diff = fabsf(acc - ctx.last_f32);
  return diff < (fabsf(acc) * 1e-5f + 1e-4f);
}

#if BENCH_ENABLE_DOUBLE
BENCH_NOINLINE void run_double_mac(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                   uint32_t iterations) {
  const uint32_t n = compute_n(params);
  double acc = 0.25;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc = (acc + s_f64_a[i] * s_f64_b[i]) * 0.9999991;
    }
  }
  ctx.last_f64 = acc;
  bench_consume_f64(acc);
}

bool validate_double_mac(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = compute_n(params);
  double acc = 0.25;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc = (acc + s_f64_a[i] * s_f64_b[i]) * 0.9999991;
    }
  }
  const double diff = fabs(acc - ctx.last_f64);
  return diff < (fabs(acc) * 1e-9 + 1e-9);
}
#endif

BENCH_NOINLINE void run_fixed_q15_mac(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                      uint32_t iterations) {
  const uint32_t n = compute_n(params);
  int64_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc += (static_cast<int32_t>(s_q15_a[i]) * static_cast<int32_t>(s_q15_b[i])) >> 15;
    }
  }
  ctx.last_u64 = static_cast<uint64_t>(acc);
  bench_consume_u64(static_cast<uint64_t>(acc));
}

bool validate_fixed_q15_mac(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                            uint32_t iterations) {
  const uint32_t n = compute_n(params);
  int64_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc += (static_cast<int32_t>(s_q15_a[i]) * static_cast<int32_t>(s_q15_b[i])) >> 15;
    }
  }
  return static_cast<uint64_t>(acc) == ctx.last_u64;
}

void teardown_noop(BenchmarkContext&, const BenchmarkParamSet&) {}

const ParamKV kP256[] = {{"n", 256}};
const ParamKV kP1024[] = {{"n", 1024}};

const BenchmarkParamSet kParamsOp1[] = {{"n256", kP256, 1, 0, 256.0f, 0.0f},
                                        {"n1024", kP1024, 1, 0, 1024.0f, 0.0f}};
const BenchmarkParamSet kParamsOp2[] = {{"n256", kP256, 1, 0, 512.0f, 0.0f},
                                        {"n1024", kP1024, 1, 0, 2048.0f, 0.0f}};
const BenchmarkParamSet kParamsCrc[] = {{"n256", kP256, 1, 0, 2048.0f, 0.0f},
                                        {"n1024", kP1024, 1, 0, 8192.0f, 0.0f}};

constexpr IterationPolicy kPolicy = {true, kMinIters, kMaxIters, kTargetMinCycles};

const BenchmarkDef kComputeDefs[] = {
    {"compute.int_add", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "integer add", setup_common, run_int_add, validate_int_add, teardown_noop, kParamsOp1, 2, kPolicy, false,
     InterruptMode::RealisticIdle},
    {"compute.int_mul", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "integer multiply", setup_common, run_int_mul, validate_int_mul, teardown_noop, kParamsOp1, 2, kPolicy,
     false, InterruptMode::RealisticIdle},
    {"compute.int_mac", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "integer multiply-accumulate", setup_common, run_int_mac, validate_int_mac, teardown_noop, kParamsOp2, 2,
     kPolicy, false, InterruptMode::RealisticIdle},
    {"compute.int_div", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "integer divide", setup_common, run_int_div, validate_int_div, teardown_noop, kParamsOp1, 2, kPolicy,
     false, InterruptMode::RealisticIdle},
    {"compute.bitwise", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "bitwise workload", setup_common, run_bitwise, validate_bitwise, teardown_noop, kParamsOp1, 2, kPolicy,
     false, InterruptMode::RealisticIdle},
    {"compute.popcount", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "population count", setup_common, run_popcount, validate_popcount, teardown_noop, kParamsOp1, 2, kPolicy,
     false, InterruptMode::RealisticIdle},
    {"compute.crc_like", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "CRC-like bit mixing", setup_common, run_crc_like, validate_crc_like, teardown_noop, kParamsCrc, 2, kPolicy,
     false, InterruptMode::RealisticIdle},
    {"compute.float_add", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "float add", setup_common, run_float_add, validate_float_add, teardown_noop, kParamsOp1, 2, kPolicy, false,
     InterruptMode::RealisticIdle},
    {"compute.float_mul", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "float multiply", setup_common, run_float_mul, validate_float_mul, teardown_noop, kParamsOp1, 2, kPolicy,
     false, InterruptMode::RealisticIdle},
    {"compute.float_mac", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "float MAC", setup_common, run_float_mac, validate_float_mac, teardown_noop, kParamsOp2, 2, kPolicy, false,
     InterruptMode::RealisticIdle},
    {"compute.fixed_q15_mac", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "fixed-point Q15 MAC", setup_common, run_fixed_q15_mac, validate_fixed_q15_mac, teardown_noop, kParamsOp2,
     2, kPolicy, false, InterruptMode::RealisticIdle},
#if BENCH_ENABLE_DOUBLE
    {"compute.double_mac", Category::Compute, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "double precision MAC", setup_common, run_double_mac, validate_double_mac, teardown_noop, kParamsOp2, 2,
     kPolicy, false, InterruptMode::RealisticIdle},
#endif
};

}  // namespace

void register_compute_benchmarks() {
  for (size_t i = 0; i < (sizeof(kComputeDefs) / sizeof(kComputeDefs[0])); ++i) {
    register_benchmark(kComputeDefs[i]);
  }
}

}  // namespace bench
