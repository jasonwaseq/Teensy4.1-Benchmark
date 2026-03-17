#include "benchmark_registry.hpp"

#include "placement_attrs.hpp"

namespace bench {
namespace {

BENCH_ALIGN(32) uint32_t s_rt_data[512];

bool setup_rt_data(BenchmarkContext&, const BenchmarkParamSet&) {
  fill_u32_pattern(s_rt_data, 512, kDeterministicSeed ^ 0x9090u);
  return true;
}

BENCH_NOINLINE void run_empty_latency(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                      uint32_t iterations) {
  const uint32_t inner = param_u32(params, "inner", 32u);
  uint32_t acc = 0x1F2E3D4Cu;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < inner; ++i) {
      acc ^= (acc << 5) + (acc >> 2) + i + it;
    }
  }
  ctx.last_u32 = acc;
  bench_consume_u32(acc);
}

bool validate_empty_latency(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                            uint32_t iterations) {
  const uint32_t inner = param_u32(params, "inner", 32u);
  uint32_t acc = 0x1F2E3D4Cu;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < inner; ++i) {
      acc ^= (acc << 5) + (acc >> 2) + i + it;
    }
  }
  return acc == ctx.last_u32;
}

BENCH_NOINLINE void run_repeatability_kernel(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                             uint32_t iterations) {
  const uint32_t n = param_u32(params, "n", 512u);
  uint64_t acc = 0xA5A55A5AULL;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc += static_cast<uint64_t>(s_rt_data[i]) * static_cast<uint64_t>((i + 1u) ^ 0x5Au);
      acc ^= (acc >> 7);
    }
  }
  ctx.last_u64 = acc;
  bench_consume_u64(acc);
}

bool validate_repeatability_kernel(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                   uint32_t iterations) {
  const uint32_t n = param_u32(params, "n", 512u);
  uint64_t acc = 0xA5A55A5AULL;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < n; ++i) {
      acc += static_cast<uint64_t>(s_rt_data[i]) * static_cast<uint64_t>((i + 1u) ^ 0x5Au);
      acc ^= (acc >> 7);
    }
  }
  return acc == ctx.last_u64;
}

void teardown_noop(BenchmarkContext&, const BenchmarkParamSet&) {}

const ParamKV kInner32[] = {{"inner", 32}};
const ParamKV kInner128[] = {{"inner", 128}};
const ParamKV kN512[] = {{"n", 512}};

const BenchmarkParamSet kLatencyParams[] = {{"i32", kInner32, 1, 0, 32.0f, 0.0f},
                                            {"i128", kInner128, 1, 0, 128.0f, 0.0f}};
const BenchmarkParamSet kKernelParams[] = {{"n512", kN512, 1, 0, 1024.0f, 2048.0f}};

constexpr IterationPolicy kPolicy = {true, kMinIters, kMaxIters, kTargetMinCycles};

const BenchmarkDef kRealtimeDefs[] = {
    {"realtime.latency_idle", Category::Realtime, CodePlacement::NotApplicable,
     DataPlacement::NotApplicable, "latency distribution with interrupts mostly idle", setup_rt_data,
     run_empty_latency, validate_empty_latency, teardown_noop, kLatencyParams, 2, kPolicy, true,
     InterruptMode::RealisticIdle},
    {"realtime.latency_periodic", Category::Realtime, CodePlacement::NotApplicable,
     DataPlacement::NotApplicable, "latency distribution with periodic ISR active", setup_rt_data,
     run_empty_latency, validate_empty_latency, teardown_noop, kLatencyParams, 2, kPolicy, true,
     InterruptMode::PeriodicIsr},
    {"realtime.latency_usb", Category::Realtime, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "latency distribution with USB activity", setup_rt_data, run_empty_latency, validate_empty_latency,
     teardown_noop, kLatencyParams, 2, kPolicy, true, InterruptMode::UsbActive},
    {"realtime.repeatability_idle", Category::Realtime, CodePlacement::NotApplicable,
     DataPlacement::NotApplicable, "repeatability kernel idle", setup_rt_data, run_repeatability_kernel,
     validate_repeatability_kernel, teardown_noop, kKernelParams, 1, kPolicy, true,
     InterruptMode::RealisticIdle},
    {"realtime.repeatability_periodic", Category::Realtime, CodePlacement::NotApplicable,
     DataPlacement::NotApplicable, "repeatability kernel periodic ISR", setup_rt_data,
     run_repeatability_kernel, validate_repeatability_kernel, teardown_noop, kKernelParams, 1, kPolicy, true,
     InterruptMode::PeriodicIsr},
    {"realtime.repeatability_usb", Category::Realtime, CodePlacement::NotApplicable,
     DataPlacement::NotApplicable, "repeatability kernel USB active", setup_rt_data,
     run_repeatability_kernel, validate_repeatability_kernel, teardown_noop, kKernelParams, 1, kPolicy, true,
     InterruptMode::UsbActive},
};

}  // namespace

void register_realtime_benchmarks() {
  for (size_t i = 0; i < (sizeof(kRealtimeDefs) / sizeof(kRealtimeDefs[0])); ++i) {
    register_benchmark(kRealtimeDefs[i]);
  }
}

}  // namespace bench

