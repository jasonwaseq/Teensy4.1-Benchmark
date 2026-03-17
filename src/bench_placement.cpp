#include "benchmark_registry.hpp"

#include <stdint.h>

#include "placement_attrs.hpp"

namespace bench {
namespace {

BENCH_ALIGN(32) uint32_t s_code_a[1024];
BENCH_ALIGN(32) uint32_t s_code_b[1024];

BENCH_ALIGN(32) uint8_t s_global_data_raw[2048 + 64];
BENCH_DMAMEM BENCH_ALIGN(32) uint8_t s_dmamem_data_raw[2048 + 64];

uint8_t* aligned_ptr(uint8_t* ptr, uint32_t align) {
  const uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
  const uintptr_t aligned = (p + (align - 1u)) & ~(static_cast<uintptr_t>(align - 1u));
  return reinterpret_cast<uint8_t*>(aligned);
}

bool setup_code_common(BenchmarkContext&, const BenchmarkParamSet& params) {
  const uint32_t n = param_u32(params, "n", 1024u);
  if (n > 1024u) {
    return false;
  }
  fill_u32_pattern(s_code_a, n, kDeterministicSeed ^ 0xAAu);
  fill_u32_pattern(s_code_b, n, kDeterministicSeed ^ 0xBBu);
  return true;
}

#define DEFINE_CODE_KERNEL(NAME, ATTR)                                                    \
  ATTR BENCH_NOINLINE uint32_t NAME(const uint32_t* a, const uint32_t* b, uint32_t n,    \
                                    uint32_t iterations) {                                \
    uint32_t h = 0xDEADBEEFu;                                                             \
    for (uint32_t it = 0; it < iterations; ++it) {                                       \
      uint32_t acc = static_cast<uint32_t>(it + 1u);                                     \
      for (uint32_t i = 0; i < n; ++i) {                                                  \
        acc += (a[i] * 33u) ^ (b[i] * 17u) ^ (acc >> 3);                                 \
        h = mix_hash_u32(h, acc);                                                         \
      }                                                                                   \
    }                                                                                     \
    return h;                                                                             \
  }

DEFINE_CODE_KERNEL(code_kernel_default, )
DEFINE_CODE_KERNEL(code_kernel_fastrun, BENCH_FASTRUN)
DEFINE_CODE_KERNEL(code_kernel_flashmem, BENCH_FLASHMEM)

BENCH_NOINLINE void run_code_default(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                     uint32_t iterations) {
  const uint32_t n = param_u32(params, "n", 1024u);
  const uint32_t h = code_kernel_default(s_code_a, s_code_b, n, iterations);
  ctx.last_u32 = h;
  bench_consume_u32(h);
}

BENCH_NOINLINE void run_code_fastrun(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                     uint32_t iterations) {
  const uint32_t n = param_u32(params, "n", 1024u);
  const uint32_t h = code_kernel_fastrun(s_code_a, s_code_b, n, iterations);
  ctx.last_u32 = h;
  bench_consume_u32(h);
}

BENCH_NOINLINE void run_code_flashmem(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                      uint32_t iterations) {
  const uint32_t n = param_u32(params, "n", 1024u);
  const uint32_t h = code_kernel_flashmem(s_code_a, s_code_b, n, iterations);
  ctx.last_u32 = h;
  bench_consume_u32(h);
}

bool validate_code_common(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = param_u32(params, "n", 1024u);
  const uint32_t expected = code_kernel_default(s_code_a, s_code_b, n, iterations);
  return expected == ctx.last_u32;
}

uint32_t data_kernel(uint8_t* buf, uint32_t bytes, uint32_t iterations) {
  uint32_t h = 0x12344321u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < bytes; ++i) {
      uint8_t v = buf[i];
      v = static_cast<uint8_t>((v << 1) ^ (v >> 3) ^ static_cast<uint8_t>(i + it));
      buf[i] = v;
      h = mix_hash_u32(h, v);
    }
  }
  return h;
}

bool setup_data_stack(BenchmarkContext&, const BenchmarkParamSet& params) {
  const uint32_t bytes = param_u32(params, "bytes", 1024u);
  return bytes <= 2048u;
}

bool setup_data_global(BenchmarkContext&, const BenchmarkParamSet& params) {
  const uint32_t bytes = param_u32(params, "bytes", 1024u);
  const uint32_t align = param_u32(params, "align", 32u);
  if (bytes > 2048u || (align != 4u && align != 16u && align != 32u)) {
    return false;
  }
  uint8_t* ptr = aligned_ptr(s_global_data_raw, align);
  fill_u8_pattern(ptr, bytes, kDeterministicSeed ^ 0xCCu);
  return true;
}

bool setup_data_dmamem(BenchmarkContext&, const BenchmarkParamSet& params) {
  const uint32_t bytes = param_u32(params, "bytes", 1024u);
  const uint32_t align = param_u32(params, "align", 32u);
  if (bytes > 2048u || (align != 4u && align != 16u && align != 32u)) {
    return false;
  }
  uint8_t* ptr = aligned_ptr(s_dmamem_data_raw, align);
  fill_u8_pattern(ptr, bytes, kDeterministicSeed ^ 0xDDu);
  return true;
}

BENCH_NOINLINE void run_data_stack(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                   uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 1024u);
  const uint32_t align = param_u32(params, "align", 32u);
  BENCH_ALIGN(32) uint8_t local_raw[2048 + 64];
  uint8_t* ptr = aligned_ptr(local_raw, align);
  fill_u8_pattern(ptr, bytes, kDeterministicSeed ^ 0xCCu);
  const uint32_t h = data_kernel(ptr, bytes, iterations);
  ctx.last_u32 = h;
  bench_consume_u32(h);
}

BENCH_NOINLINE void run_data_global(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                    uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 1024u);
  const uint32_t align = param_u32(params, "align", 32u);
  uint8_t* ptr = aligned_ptr(s_global_data_raw, align);
  const uint32_t h = data_kernel(ptr, bytes, iterations);
  ctx.last_u32 = h;
  bench_consume_u32(h);
}

BENCH_NOINLINE void run_data_dmamem(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                    uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 1024u);
  const uint32_t align = param_u32(params, "align", 32u);
  uint8_t* ptr = aligned_ptr(s_dmamem_data_raw, align);
  const uint32_t h = data_kernel(ptr, bytes, iterations);
  ctx.last_u32 = h;
  bench_consume_u32(h);
}

bool validate_data_stack(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 1024u);
  const uint32_t align = param_u32(params, "align", 32u);
  BENCH_ALIGN(32) uint8_t local_raw[2048 + 64];
  uint8_t* ptr = aligned_ptr(local_raw, align);
  fill_u8_pattern(ptr, bytes, kDeterministicSeed ^ 0xCCu);
  const uint32_t expected = data_kernel(ptr, bytes, iterations);
  return expected == ctx.last_u32;
}

bool validate_data_global(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 1024u);
  const uint32_t align = param_u32(params, "align", 32u);
  uint8_t* ptr = aligned_ptr(s_global_data_raw, align);
  fill_u8_pattern(ptr, bytes, kDeterministicSeed ^ 0xCCu);
  const uint32_t expected = data_kernel(ptr, bytes, iterations);
  return expected == ctx.last_u32;
}

bool validate_data_dmamem(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 1024u);
  const uint32_t align = param_u32(params, "align", 32u);
  uint8_t* ptr = aligned_ptr(s_dmamem_data_raw, align);
  fill_u8_pattern(ptr, bytes, kDeterministicSeed ^ 0xDDu);
  const uint32_t expected = data_kernel(ptr, bytes, iterations);
  return expected == ctx.last_u32;
}

void teardown_noop(BenchmarkContext&, const BenchmarkParamSet&) {}

const ParamKV kN256[] = {{"n", 256}};
const ParamKV kN1024[] = {{"n", 1024}};
const ParamKV kB1024A4[] = {{"bytes", 1024}, {"align", 4}};
const ParamKV kB1024A16[] = {{"bytes", 1024}, {"align", 16}};
const ParamKV kB1024A32[] = {{"bytes", 1024}, {"align", 32}};

const BenchmarkParamSet kCodeParams[] = {{"n256", kN256, 1, 0, 1024.0f, 2048.0f},
                                         {"n1024", kN1024, 1, 0, 4096.0f, 8192.0f}};
const BenchmarkParamSet kDataParams[] = {{"a4", kB1024A4, 2, 0, 1024.0f, 2048.0f},
                                         {"a16", kB1024A16, 2, 0, 1024.0f, 2048.0f},
                                         {"a32", kB1024A32, 2, 0, 1024.0f, 2048.0f}};

constexpr IterationPolicy kPolicy = {true, kMinIters, kMaxIters, kTargetMinCycles};

const BenchmarkDef kPlacementDefs[] = {
    {"placement.code_default", Category::Placement, CodePlacement::Default, DataPlacement::Global,
     "code placement baseline", setup_code_common, run_code_default, validate_code_common, teardown_noop,
     kCodeParams, 2, kPolicy, false, InterruptMode::RealisticIdle},
    {"placement.code_fastrun", Category::Placement, CodePlacement::FastRun, DataPlacement::Global,
     "identical kernel in FASTRUN", setup_code_common, run_code_fastrun, validate_code_common, teardown_noop,
     kCodeParams, 2, kPolicy, false, InterruptMode::RealisticIdle},
    {"placement.code_flashmem", Category::Placement, CodePlacement::FlashMem, DataPlacement::Global,
     "identical kernel in FLASHMEM", setup_code_common, run_code_flashmem, validate_code_common, teardown_noop,
     kCodeParams, 2, kPolicy, false, InterruptMode::RealisticIdle},

    {"placement.data_stack", Category::Placement, CodePlacement::NotApplicable, DataPlacement::Stack,
     "data placement stack", setup_data_stack, run_data_stack, validate_data_stack, teardown_noop, kDataParams,
     3, kPolicy, false, InterruptMode::RealisticIdle},
    {"placement.data_global", Category::Placement, CodePlacement::NotApplicable, DataPlacement::Global,
     "data placement global/static", setup_data_global, run_data_global, validate_data_global, teardown_noop,
     kDataParams, 3, kPolicy, false, InterruptMode::RealisticIdle},
    {"placement.data_dmamem", Category::Placement, CodePlacement::NotApplicable, DataPlacement::DmaMem,
     "data placement DMAMEM", setup_data_dmamem, run_data_dmamem, validate_data_dmamem, teardown_noop,
     kDataParams, 3, kPolicy, false, InterruptMode::RealisticIdle},
};

}  // namespace

void register_placement_benchmarks() {
  for (size_t i = 0; i < (sizeof(kPlacementDefs) / sizeof(kPlacementDefs[0])); ++i) {
    register_benchmark(kPlacementDefs[i]);
  }
}

}  // namespace bench

