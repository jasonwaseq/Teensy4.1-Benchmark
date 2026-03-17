#include "benchmark_registry.hpp"

#include <string.h>

#include "placement_attrs.hpp"

namespace bench {
namespace {

BENCH_ALIGN(32) uint8_t s_matrix_a[32 * 32];
BENCH_ALIGN(32) uint8_t s_matrix_b[32 * 32];
BENCH_ALIGN(32) int32_t s_matrix_c[32 * 32];

BENCH_ALIGN(32) int16_t s_fir_coeffs[64];
BENCH_ALIGN(32) int16_t s_fir_input[512];
BENCH_ALIGN(32) int32_t s_fir_output[512];

BENCH_NOINLINE void run_memcpy_like(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                    uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  uint8_t* src = ctx.global_buffer;
  uint8_t* dst = ctx.global_buffer + (ctx.global_buffer_bytes / 2u);
  uint32_t hash = 0x1234567u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < bytes; i += 4) {
      *reinterpret_cast<uint32_t*>(&dst[i]) = *reinterpret_cast<const uint32_t*>(&src[i]);
      hash = mix_hash_u32(hash, *reinterpret_cast<const uint32_t*>(&dst[i]));
    }
  }
  ctx.last_u32 = hash;
  bench_consume_u32(hash);
}

bool validate_memcpy_like(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  uint8_t* src = ctx.global_buffer;
  uint8_t* dst = ctx.global_buffer + (ctx.global_buffer_bytes / 2u);
  return checksum_u8(src, bytes) == checksum_u8(dst, bytes);
}

BENCH_NOINLINE void run_memset_like(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                    uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  uint8_t* dst = ctx.global_buffer + (ctx.global_buffer_bytes / 2u);
  for (uint32_t it = 0; it < iterations; ++it) {
    memset(dst, 0x5Au, bytes);
  }
  ctx.last_u32 = checksum_u8(dst, bytes);
  bench_consume_u32(ctx.last_u32);
}

bool validate_memset_like(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  uint8_t* dst = ctx.global_buffer + (ctx.global_buffer_bytes / 2u);
  for (uint32_t i = 0; i < bytes; ++i) {
    if (dst[i] != 0x5Au) {
      return false;
    }
  }
  return true;
}

BENCH_NOINLINE void run_seq_read(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                 uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  uint8_t* src = ctx.global_buffer;
  uint32_t sum = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < bytes; ++i) {
      sum += src[i];
    }
  }
  ctx.last_u32 = sum;
  bench_consume_u32(sum);
}

bool validate_seq_read(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  uint8_t* src = ctx.global_buffer;
  uint32_t sum = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < bytes; ++i) {
      sum += src[i];
    }
  }
  return sum == ctx.last_u32;
}

BENCH_NOINLINE void run_seq_write(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                  uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  uint8_t* dst = ctx.global_buffer + (ctx.global_buffer_bytes / 2u);
  for (uint32_t it = 0; it < iterations; ++it) {
    const uint8_t v = static_cast<uint8_t>(it + 7u);
    memset(dst, v, bytes);
  }
  ctx.last_u32 = checksum_u8(dst, bytes);
  bench_consume_u32(ctx.last_u32);
}

bool validate_seq_write(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  uint8_t* dst = ctx.global_buffer + (ctx.global_buffer_bytes / 2u);
  const uint8_t expected = static_cast<uint8_t>((iterations - 1u) + 7u);
  for (uint32_t i = 0; i < bytes; ++i) {
    if (dst[i] != expected) {
      return false;
    }
  }
  return true;
}

BENCH_NOINLINE void run_strided_read(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                     uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  const uint32_t stride = param_u32(params, "stride", 16u);
  uint8_t* src = ctx.global_buffer;
  uint32_t sum = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < bytes; i += stride) {
      sum += src[i];
    }
  }
  ctx.last_u32 = sum;
  bench_consume_u32(sum);
}

bool validate_strided_read(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  const uint32_t stride = param_u32(params, "stride", 16u);
  uint8_t* src = ctx.global_buffer;
  uint32_t sum = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < bytes; i += stride) {
      sum += src[i];
    }
  }
  return sum == ctx.last_u32;
}

BENCH_NOINLINE void run_strided_write(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                      uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  const uint32_t stride = param_u32(params, "stride", 16u);
  uint8_t* dst = ctx.global_buffer + (ctx.global_buffer_bytes / 2u);
  for (uint32_t it = 0; it < iterations; ++it) {
    const uint8_t v = static_cast<uint8_t>((it * 13u) + 1u);
    for (uint32_t i = 0; i < bytes; i += stride) {
      dst[i] = v;
    }
  }
  ctx.last_u32 = checksum_u8(dst, bytes);
  bench_consume_u32(ctx.last_u32);
}

bool validate_strided_write(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  const uint32_t stride = param_u32(params, "stride", 16u);
  uint8_t* dst = ctx.global_buffer + (ctx.global_buffer_bytes / 2u);
  const uint8_t expected = static_cast<uint8_t>(((iterations - 1u) * 13u) + 1u);
  for (uint32_t i = 0; i < bytes; ++i) {
    if ((i % stride) == 0u && dst[i] != expected) {
      return false;
    }
  }
  return true;
}

BENCH_NOINLINE void run_random_index(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                     uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  const uint32_t mask = bytes - 1u;
  uint8_t* src = ctx.global_buffer;
  uint32_t seed = kDeterministicSeed;
  uint32_t hash = 0xCAFEBABEu;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < bytes; ++i) {
      seed = xorshift32(seed);
      const uint32_t idx = seed & mask;
      hash = mix_hash_u32(hash, src[idx]);
    }
  }
  ctx.last_u32 = hash;
  bench_consume_u32(hash);
}

bool validate_random_index(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  const uint32_t mask = bytes - 1u;
  uint8_t* src = ctx.global_buffer;
  uint32_t seed = kDeterministicSeed;
  uint32_t hash = 0xCAFEBABEu;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < bytes; ++i) {
      seed = xorshift32(seed);
      const uint32_t idx = seed & mask;
      hash = mix_hash_u32(hash, src[idx]);
    }
  }
  return hash == ctx.last_u32;
}

BENCH_NOINLINE void run_ring_buffer(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                    uint32_t iterations) {
  const uint32_t ring_bytes = param_u32(params, "ring", 4096u);
  const uint32_t mask = ring_bytes - 1u;
  uint8_t* ring = ctx.global_buffer;
  uint32_t idx = 0;
  uint32_t hash = 0x11112222u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < ring_bytes; ++i) {
      ring[idx] = static_cast<uint8_t>(ring[idx] + 3u);
      hash = mix_hash_u32(hash, ring[idx]);
      idx = (idx + 1u) & mask;
    }
  }
  ctx.last_u32 = hash;
  bench_consume_u32(hash);
}

bool validate_ring_buffer(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t ring_bytes = param_u32(params, "ring", 4096u);
  const uint32_t mask = ring_bytes - 1u;
  BENCH_ALIGN(32) uint8_t ring_local[4096];
  uint8_t* ring = ring_local;
  fill_u8_pattern(ring, ring_bytes, kDeterministicSeed ^ 0x3344u);
  uint32_t idx = 0;
  uint32_t hash = 0x11112222u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < ring_bytes; ++i) {
      ring[idx] = static_cast<uint8_t>(ring[idx] + 3u);
      hash = mix_hash_u32(hash, ring[idx]);
      idx = (idx + 1u) & mask;
    }
  }
  return hash == ctx.last_u32;
}

BENCH_NOINLINE void run_reduction(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                  uint32_t iterations) {
  const uint32_t count = param_u32(params, "n", 4096u);
  uint32_t* src = reinterpret_cast<uint32_t*>(ctx.global_buffer);
  uint64_t sum = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < count; ++i) {
      sum += src[i];
    }
  }
  ctx.last_u64 = sum;
  bench_consume_u64(sum);
}

bool validate_reduction(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t count = param_u32(params, "n", 4096u);
  uint32_t* src = reinterpret_cast<uint32_t*>(ctx.global_buffer);
  uint64_t sum = 0;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = 0; i < count; ++i) {
      sum += src[i];
    }
  }
  return sum == ctx.last_u64;
}

bool setup_matrix(BenchmarkContext&, const BenchmarkParamSet& params) {
  const uint32_t n = param_u32(params, "n", 8u);
  if (n > 32u) {
    return false;
  }
  for (uint32_t i = 0; i < (n * n); ++i) {
    s_matrix_a[i] = static_cast<uint8_t>((i * 3u + 7u) & 0xFFu);
    s_matrix_b[i] = static_cast<uint8_t>((i * 5u + 11u) & 0xFFu);
    s_matrix_c[i] = 0;
  }
  return true;
}

BENCH_NOINLINE void run_matrix_mul(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                   uint32_t iterations) {
  const uint32_t n = param_u32(params, "n", 8u);
  uint32_t hash = 0x76543210u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t r = 0; r < n; ++r) {
      for (uint32_t c = 0; c < n; ++c) {
        int32_t acc = 0;
        for (uint32_t k = 0; k < n; ++k) {
          acc += static_cast<int32_t>(s_matrix_a[r * n + k]) *
                 static_cast<int32_t>(s_matrix_b[k * n + c]);
        }
        s_matrix_c[r * n + c] = acc;
        hash = mix_hash_u32(hash, static_cast<uint32_t>(acc));
      }
    }
  }
  ctx.last_u32 = hash;
  bench_consume_u32(hash);
}

bool validate_matrix_mul(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t n = param_u32(params, "n", 8u);
  uint32_t hash = 0x76543210u;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t r = 0; r < n; ++r) {
      for (uint32_t c = 0; c < n; ++c) {
        int32_t acc = 0;
        for (uint32_t k = 0; k < n; ++k) {
          acc += static_cast<int32_t>(s_matrix_a[r * n + k]) *
                 static_cast<int32_t>(s_matrix_b[k * n + c]);
        }
        hash = mix_hash_u32(hash, static_cast<uint32_t>(acc));
      }
    }
  }
  return hash == ctx.last_u32;
}

bool setup_fir(BenchmarkContext&, const BenchmarkParamSet& params) {
  const uint32_t taps = param_u32(params, "taps", 16u);
  if (taps > 64u) {
    return false;
  }
  for (uint32_t i = 0; i < taps; ++i) {
    s_fir_coeffs[i] = static_cast<int16_t>((i * 97u) & 0x7FFFu);
  }
  for (uint32_t i = 0; i < 512u; ++i) {
    s_fir_input[i] = static_cast<int16_t>((i * 33u + 7u) & 0x7FFFu);
    s_fir_output[i] = 0;
  }
  return true;
}

BENCH_NOINLINE void run_fir(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t taps = param_u32(params, "taps", 16u);
  const uint32_t n = param_u32(params, "n", 256u);
  uint32_t hash = 0xAABBCCDDu;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = taps; i < n; ++i) {
      int32_t acc = 0;
      for (uint32_t k = 0; k < taps; ++k) {
        acc += static_cast<int32_t>(s_fir_input[i - k]) * static_cast<int32_t>(s_fir_coeffs[k]);
      }
      s_fir_output[i] = acc;
      hash = mix_hash_u32(hash, static_cast<uint32_t>(acc));
    }
  }
  ctx.last_u32 = hash;
  bench_consume_u32(hash);
}

bool validate_fir(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations) {
  const uint32_t taps = param_u32(params, "taps", 16u);
  const uint32_t n = param_u32(params, "n", 256u);
  uint32_t hash = 0xAABBCCDDu;
  for (uint32_t it = 0; it < iterations; ++it) {
    for (uint32_t i = taps; i < n; ++i) {
      int32_t acc = 0;
      for (uint32_t k = 0; k < taps; ++k) {
        acc += static_cast<int32_t>(s_fir_input[i - k]) * static_cast<int32_t>(s_fir_coeffs[k]);
      }
      hash = mix_hash_u32(hash, static_cast<uint32_t>(acc));
    }
  }
  return hash == ctx.last_u32;
}

bool setup_memory_common(BenchmarkContext& ctx, const BenchmarkParamSet& params) {
  const uint32_t bytes = param_u32(params, "bytes", 16384u);
  if ((bytes == 0u) || (bytes > (ctx.global_buffer_bytes / 2u))) {
    return false;
  }
  fill_u8_pattern(ctx.global_buffer, bytes, kDeterministicSeed ^ 0x3344u);
  fill_u8_pattern(ctx.global_buffer + (ctx.global_buffer_bytes / 2u), bytes, kDeterministicSeed ^ 0x7788u);
  return true;
}

void teardown_noop(BenchmarkContext&, const BenchmarkParamSet&) {}

const ParamKV kBytes4K[] = {{"bytes", 4096}};
const ParamKV kBytes16K[] = {{"bytes", 16384}};
const ParamKV kBytes64K[] = {{"bytes", 65536}};
const ParamKV kStride16[] = {{"bytes", 16384}, {"stride", 16}};
const ParamKV kStride64[] = {{"bytes", 16384}, {"stride", 64}};
const ParamKV kRand16K[] = {{"bytes", 16384}};
const ParamKV kRing4K[] = {{"ring", 4096}};
const ParamKV kReduce4K[] = {{"n", 4096}};
const ParamKV kMat8[] = {{"n", 8}};
const ParamKV kMat16[] = {{"n", 16}};
const ParamKV kMat24[] = {{"n", 24}};
const ParamKV kMat32[] = {{"n", 32}};
const ParamKV kFir16[] = {{"taps", 16}, {"n", 256}};
const ParamKV kFir64[] = {{"taps", 64}, {"n", 256}};

const BenchmarkParamSet kCopyParams[] = {{"4k", kBytes4K, 1, 0, 4096.0f, 4096.0f},
                                         {"16k", kBytes16K, 1, 0, 16384.0f, 16384.0f},
                                         {"64k", kBytes64K, 1, 0, 65536.0f, 65536.0f}};
const BenchmarkParamSet kStrideParams[] = {{"stride16", kStride16, 2, 0, 1024.0f, 1024.0f},
                                           {"stride64", kStride64, 2, 0, 256.0f, 256.0f}};
const BenchmarkParamSet kRandomParams[] = {{"16k", kRand16K, 1, 0, 16384.0f, 16384.0f}};
const BenchmarkParamSet kRingParams[] = {{"ring4k", kRing4K, 1, 0, 4096.0f, 4096.0f}};
const BenchmarkParamSet kReduceParams[] = {{"n4096", kReduce4K, 1, 0, 4096.0f, 16384.0f}};
const BenchmarkParamSet kMatParams[] = {{"8", kMat8, 1, 0, 2.0f * 8.0f * 8.0f * 8.0f, 3.0f * 64.0f},
                                        {"16", kMat16, 1, 0, 2.0f * 16.0f * 16.0f * 16.0f, 3.0f * 256.0f},
                                        {"24", kMat24, 1, 0, 2.0f * 24.0f * 24.0f * 24.0f, 3.0f * 576.0f},
                                        {"32", kMat32, 1, 0, 2.0f * 32.0f * 32.0f * 32.0f, 3.0f * 1024.0f}};
const BenchmarkParamSet kFirParams[] = {{"t16", kFir16, 2, 0, 2.0f * 16.0f * 240.0f, 256.0f * 2.0f},
                                        {"t64", kFir64, 2, 0, 2.0f * 64.0f * 192.0f, 256.0f * 2.0f}};

constexpr IterationPolicy kPolicy = {true, kMinIters, kMaxIters, kTargetMinCycles};

const BenchmarkDef kMemoryDefs[] = {
    {"memory.memcpy_like", Category::Memory, CodePlacement::NotApplicable, DataPlacement::Global,
     "copy loop", setup_memory_common, run_memcpy_like, validate_memcpy_like, teardown_noop, kCopyParams, 3,
     kPolicy, false, InterruptMode::RealisticIdle},
    {"memory.memset_like", Category::Memory, CodePlacement::NotApplicable, DataPlacement::Global, "fill loop",
     setup_memory_common, run_memset_like, validate_memset_like, teardown_noop, kCopyParams, 3, kPolicy, false,
     InterruptMode::RealisticIdle},
    {"memory.seq_read", Category::Memory, CodePlacement::NotApplicable, DataPlacement::Global,
     "sequential read", setup_memory_common, run_seq_read, validate_seq_read, teardown_noop, kCopyParams, 3,
     kPolicy, false, InterruptMode::RealisticIdle},
    {"memory.seq_write", Category::Memory, CodePlacement::NotApplicable, DataPlacement::Global,
     "sequential write", setup_memory_common, run_seq_write, validate_seq_write, teardown_noop, kCopyParams, 3,
     kPolicy, false, InterruptMode::RealisticIdle},
    {"memory.strided_read", Category::Memory, CodePlacement::NotApplicable, DataPlacement::Global,
     "strided read", setup_memory_common, run_strided_read, validate_strided_read, teardown_noop, kStrideParams,
     2, kPolicy, false, InterruptMode::RealisticIdle},
    {"memory.strided_write", Category::Memory, CodePlacement::NotApplicable, DataPlacement::Global,
     "strided write", setup_memory_common, run_strided_write, validate_strided_write, teardown_noop,
     kStrideParams, 2, kPolicy, false, InterruptMode::RealisticIdle},
    {"memory.random_index", Category::Memory, CodePlacement::NotApplicable, DataPlacement::Global,
     "random indexed read", setup_memory_common, run_random_index, validate_random_index, teardown_noop,
     kRandomParams, 1, kPolicy, false, InterruptMode::RealisticIdle},
    {"memory.ring_buffer", Category::Memory, CodePlacement::NotApplicable, DataPlacement::Global,
     "ring-buffer access", setup_memory_common, run_ring_buffer, validate_ring_buffer, teardown_noop, kRingParams,
     1, kPolicy, false, InterruptMode::RealisticIdle},
    {"memory.reduction", Category::Memory, CodePlacement::NotApplicable, DataPlacement::Global,
     "array reduction", setup_memory_common, run_reduction, validate_reduction, teardown_noop, kReduceParams, 1,
     kPolicy, false, InterruptMode::RealisticIdle},
    {"memory.matrix_mul", Category::Memory, CodePlacement::NotApplicable, DataPlacement::Global,
     "matrix multiply sweep", setup_matrix, run_matrix_mul, validate_matrix_mul, teardown_noop, kMatParams, 4,
     kPolicy, false, InterruptMode::RealisticIdle},
    {"memory.fir_window", Category::Memory, CodePlacement::NotApplicable, DataPlacement::Global,
     "FIR sliding window", setup_fir, run_fir, validate_fir, teardown_noop, kFirParams, 2, kPolicy, false,
     InterruptMode::RealisticIdle},
};

}  // namespace

void register_memory_benchmarks() {
  for (size_t i = 0; i < (sizeof(kMemoryDefs) / sizeof(kMemoryDefs[0])); ++i) {
    register_benchmark(kMemoryDefs[i]);
  }
}

}  // namespace bench
