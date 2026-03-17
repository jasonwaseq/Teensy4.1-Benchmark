#include "benchmark_registry.hpp"

#include <string.h>

#include "benchmark_config.hpp"
#include "placement_attrs.hpp"

#if defined(__has_include)
#if BENCH_ENABLE_DMA && defined(ARDUINO_TEENSY41) && __has_include(<DMAChannel.h>)
#include <DMAChannel.h>
#define BENCH_HAS_DMA 1
#else
#define BENCH_HAS_DMA 0
#endif
#else
#define BENCH_HAS_DMA 0
#endif

namespace bench {
namespace {

BENCH_ALIGN(32) uint32_t s_opt_data[4096];
BENCH_ALIGN(32) uint32_t s_opt_out[4096];

bool setup_opt_common(BenchmarkContext&, const BenchmarkParamSet&) {
  fill_u32_pattern(s_opt_data, 4096, kDeterministicSeed ^ 0xABCDu);
  memset(s_opt_out, 0, sizeof(s_opt_out));
  return true;
}

BENCH_NOINLINE void run_cache_stride(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                     uint32_t iterations) {
  const uint32_t stride = param_u32(params, "stride", 1u);
  uint32_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; i += stride) {
      acc += s_opt_data[i];
    }
  }
  ctx.last_u32 = acc;
  bench_consume_u32(acc);
}

bool validate_cache_stride(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t stride = param_u32(params, "stride", 1u);
  uint32_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; i += stride) {
      acc += s_opt_data[i];
    }
  }
  return acc == ctx.last_u32;
}

BENCH_NOINLINE void run_branch_predictable(BenchmarkContext& ctx, const BenchmarkParamSet&,
                                           uint32_t iterations) {
  uint32_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; ++i) {
      if ((i & 1u) == 0u) {
        acc += s_opt_data[i] ^ 0x55AA55AAu;
      } else {
        acc += s_opt_data[i] ^ 0xAA55AA55u;
      }
    }
  }
  ctx.last_u32 = acc;
  bench_consume_u32(acc);
}

BENCH_NOINLINE void run_branch_random(BenchmarkContext& ctx, const BenchmarkParamSet&,
                                      uint32_t iterations) {
  uint32_t acc = 0;
  uint32_t state = kDeterministicSeed;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; ++i) {
      state = xorshift32(state);
      if (state & 1u) {
        acc += s_opt_data[i] ^ 0x13579BDFu;
      } else {
        acc += s_opt_data[i] ^ 0x2468ACE0u;
      }
    }
  }
  ctx.last_u32 = acc;
  bench_consume_u32(acc);
}

bool validate_branch_predictable(BenchmarkContext& ctx, const BenchmarkParamSet&, uint32_t iterations) {
  uint32_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; ++i) {
      if ((i & 1u) == 0u) {
        acc += s_opt_data[i] ^ 0x55AA55AAu;
      } else {
        acc += s_opt_data[i] ^ 0xAA55AA55u;
      }
    }
  }
  return acc == ctx.last_u32;
}

bool validate_branch_random(BenchmarkContext& ctx, const BenchmarkParamSet&, uint32_t iterations) {
  uint32_t acc = 0;
  uint32_t state = kDeterministicSeed;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; ++i) {
      state = xorshift32(state);
      if (state & 1u) {
        acc += s_opt_data[i] ^ 0x13579BDFu;
      } else {
        acc += s_opt_data[i] ^ 0x2468ACE0u;
      }
    }
  }
  return acc == ctx.last_u32;
}

BENCH_NOINLINE void run_instruction_mix_alu(BenchmarkContext& ctx, const BenchmarkParamSet&,
                                            uint32_t iterations) {
  uint32_t x = 0x1234ABCDu;
  for (uint32_t it = 0; it < iterations * 32u; ++it) {
    x ^= (x << 7);
    x += 0x9E3779B9u;
    x ^= (x >> 11);
    x *= 33u;
    x ^= (x << 13);
  }
  ctx.last_u32 = x;
  bench_consume_u32(x);
}

bool validate_instruction_mix_alu(BenchmarkContext& ctx, const BenchmarkParamSet&, uint32_t iterations) {
  uint32_t x = 0x1234ABCDu;
  for (uint32_t it = 0; it < iterations * 32u; ++it) {
    x ^= (x << 7);
    x += 0x9E3779B9u;
    x ^= (x >> 11);
    x *= 33u;
    x ^= (x << 13);
  }
  return x == ctx.last_u32;
}

BENCH_NOINLINE void run_instruction_mix_mem(BenchmarkContext& ctx, const BenchmarkParamSet&,
                                            uint32_t iterations) {
  uint32_t hash = 0x77889900u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; ++i) {
      s_opt_out[i] = s_opt_data[i] + i + it;
      hash = mix_hash_u32(hash, s_opt_out[i]);
    }
  }
  ctx.last_u32 = hash;
  bench_consume_u32(hash);
}

bool validate_instruction_mix_mem(BenchmarkContext& ctx, const BenchmarkParamSet&, uint32_t iterations) {
  uint32_t hash = 0x77889900u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; ++i) {
      hash = mix_hash_u32(hash, s_opt_data[i] + i + it);
    }
  }
  return hash == ctx.last_u32;
}

BENCH_NOINLINE void run_instruction_mix_mixed(BenchmarkContext& ctx, const BenchmarkParamSet&,
                                              uint32_t iterations) {
  uint32_t hash = 0x55AA77CCu;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; ++i) {
      uint32_t v = s_opt_data[i];
      v = (v * 17u) + (v >> 3) + i;
      s_opt_out[i] = v ^ hash;
      hash = mix_hash_u32(hash, s_opt_out[i]);
    }
  }
  ctx.last_u32 = hash;
  bench_consume_u32(hash);
}

bool validate_instruction_mix_mixed(BenchmarkContext& ctx, const BenchmarkParamSet&, uint32_t iterations) {
  uint32_t hash = 0x55AA77CCu;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; ++i) {
      uint32_t v = s_opt_data[i];
      v = (v * 17u) + (v >> 3) + i;
      hash = mix_hash_u32(hash, v ^ hash);
    }
  }
  return hash == ctx.last_u32;
}

BENCH_NOINLINE void run_compact_kernel(BenchmarkContext& ctx, const BenchmarkParamSet&, uint32_t iterations) {
  uint32_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; ++i) {
      acc += (s_opt_data[i] * 3u) ^ (i + it);
    }
  }
  ctx.last_u32 = acc;
  bench_consume_u32(acc);
}

BENCH_NOINLINE void run_unrolled_kernel(BenchmarkContext& ctx, const BenchmarkParamSet&, uint32_t iterations) {
  uint32_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; i += 4u) {
      acc += (s_opt_data[i + 0] * 3u) ^ (i + it);
      acc += (s_opt_data[i + 1] * 3u) ^ (i + 1u + it);
      acc += (s_opt_data[i + 2] * 3u) ^ (i + 2u + it);
      acc += (s_opt_data[i + 3] * 3u) ^ (i + 3u + it);
    }
  }
  ctx.last_u32 = acc;
  bench_consume_u32(acc);
}

bool validate_compact_kernel(BenchmarkContext& ctx, const BenchmarkParamSet&, uint32_t iterations) {
  uint32_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; ++i) {
      acc += (s_opt_data[i] * 3u) ^ (i + it);
    }
  }
  return acc == ctx.last_u32;
}

bool validate_unrolled_kernel(BenchmarkContext& ctx, const BenchmarkParamSet&, uint32_t iterations) {
  uint32_t acc = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < 4096u; i += 4u) {
      acc += (s_opt_data[i + 0] * 3u) ^ (i + it);
      acc += (s_opt_data[i + 1] * 3u) ^ (i + 1u + it);
      acc += (s_opt_data[i + 2] * 3u) ^ (i + 2u + it);
      acc += (s_opt_data[i + 3] * 3u) ^ (i + 3u + it);
    }
  }
  return acc == ctx.last_u32;
}

#if BENCH_HAS_DMA
BENCH_DMAMEM BENCH_ALIGN(32) uint8_t s_dma_src[16384];
BENCH_DMAMEM BENCH_ALIGN(32) uint8_t s_dma_dst[16384];
DMAChannel s_dma;
volatile bool s_dma_done = false;

void dma_isr() {
  s_dma.clearInterrupt();
  s_dma_done = true;
}

bool setup_dma_copy(BenchmarkContext&, const BenchmarkParamSet&) {
  fill_u8_pattern(s_dma_src, sizeof(s_dma_src), kDeterministicSeed ^ 0xDDCCu);
  memset(s_dma_dst, 0, sizeof(s_dma_dst));
  s_dma.disable();
  s_dma.sourceBuffer(s_dma_src, sizeof(s_dma_src));
  s_dma.destinationBuffer(s_dma_dst, sizeof(s_dma_dst));
  s_dma.transferSize(4);
  s_dma.transferCount(sizeof(s_dma_src) / 4u);
  s_dma.disableOnCompletion();
  s_dma.interruptAtCompletion();
  s_dma.attachInterrupt(dma_isr);
  return true;
}

BENCH_NOINLINE void run_dma_copy(BenchmarkContext& ctx, const BenchmarkParamSet&, uint32_t iterations) {
  for (uint32_t it = 0; it < iterations; ++it) {
    s_dma_done = false;
    s_dma.sourceBuffer(s_dma_src, sizeof(s_dma_src));
    s_dma.destinationBuffer(s_dma_dst, sizeof(s_dma_dst));
    s_dma.enable();
    s_dma.triggerManual();
    while (!s_dma_done) {
    }
  }
  ctx.last_u32 = checksum_u8(s_dma_dst, sizeof(s_dma_dst));
  bench_consume_u32(ctx.last_u32);
}

bool validate_dma_copy(BenchmarkContext&, const BenchmarkParamSet&, uint32_t) {
  return checksum_u8(s_dma_src, sizeof(s_dma_src)) == checksum_u8(s_dma_dst, sizeof(s_dma_dst));
}
#endif

void teardown_noop(BenchmarkContext&, const BenchmarkParamSet&) {}

const ParamKV kStride1[] = {{"stride", 1}};
const ParamKV kStride4[] = {{"stride", 4}};
const ParamKV kStride16[] = {{"stride", 16}};
const ParamKV kStride64[] = {{"stride", 64}};
const BenchmarkParamSet kStrideParams[] = {{"s1", kStride1, 1, 0, 4096.0f, 16384.0f},
                                           {"s4", kStride4, 1, 0, 1024.0f, 4096.0f},
                                           {"s16", kStride16, 1, 0, 256.0f, 1024.0f},
                                           {"s64", kStride64, 1, 0, 64.0f, 256.0f}};
static const BenchmarkParamSet kSingleParam[] = {{"default", nullptr, 0, 0, 4096.0f, 16384.0f}};
#if BENCH_HAS_DMA
static const BenchmarkParamSet kDmaParam[] = {{"16k", nullptr, 0, 0, 16384.0f, 16384.0f}};
#endif

constexpr IterationPolicy kPolicy = {true, kMinIters, kMaxIters, kTargetMinCycles};

const BenchmarkDef kOptionalDefs[] = {
    {"optional.cache_stride", Category::Optional, CodePlacement::NotApplicable, DataPlacement::Global,
     "cache-sensitive stride sweep", setup_opt_common, run_cache_stride, validate_cache_stride, teardown_noop,
     kStrideParams, 4, kPolicy, false, InterruptMode::RealisticIdle},
    {"optional.branch_predictable", Category::Optional, CodePlacement::NotApplicable,
     DataPlacement::NotApplicable, "branch-predictable pattern", setup_opt_common, run_branch_predictable,
     validate_branch_predictable, teardown_noop, kSingleParam, 1, kPolicy, false,
     InterruptMode::RealisticIdle},
    {"optional.branch_random", Category::Optional, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "branch-randomized pattern", setup_opt_common, run_branch_random, validate_branch_random, teardown_noop,
     kSingleParam, 1, kPolicy, false, InterruptMode::RealisticIdle},
    {"optional.inst_mix_alu", Category::Optional, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "instruction mix ALU-heavy", setup_opt_common, run_instruction_mix_alu, validate_instruction_mix_alu,
     teardown_noop, kSingleParam, 1, kPolicy, false, InterruptMode::RealisticIdle},
    {"optional.inst_mix_mem", Category::Optional, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "instruction mix memory-heavy", setup_opt_common, run_instruction_mix_mem, validate_instruction_mix_mem,
     teardown_noop, kSingleParam, 1, kPolicy, false, InterruptMode::RealisticIdle},
    {"optional.inst_mix_mixed", Category::Optional, CodePlacement::NotApplicable,
     DataPlacement::NotApplicable, "instruction mix mixed", setup_opt_common, run_instruction_mix_mixed,
     validate_instruction_mix_mixed, teardown_noop, kSingleParam, 1, kPolicy, false,
     InterruptMode::RealisticIdle},
    {"optional.code_compact", Category::Optional, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "compact loop form", setup_opt_common, run_compact_kernel, validate_compact_kernel, teardown_noop,
     kSingleParam, 1, kPolicy, false, InterruptMode::RealisticIdle},
    {"optional.code_unrolled", Category::Optional, CodePlacement::NotApplicable, DataPlacement::NotApplicable,
     "unrolled loop form", setup_opt_common, run_unrolled_kernel, validate_unrolled_kernel, teardown_noop,
     kSingleParam, 1, kPolicy, false, InterruptMode::RealisticIdle},
#if BENCH_HAS_DMA
    {"optional.dma_copy", Category::Optional, CodePlacement::NotApplicable, DataPlacement::DmaMem,
     "DMA-assisted copy experiment", setup_dma_copy, run_dma_copy, validate_dma_copy, teardown_noop, kDmaParam,
     1, kPolicy, false, InterruptMode::RealisticIdle},
#endif
};

}  // namespace

void register_optional_benchmarks() {
#if BENCH_ENABLE_OPTIONAL
  for (size_t i = 0; i < (sizeof(kOptionalDefs) / sizeof(kOptionalDefs[0])); ++i) {
    register_benchmark(kOptionalDefs[i]);
  }
#endif
}

}  // namespace bench
