#include "benchmark.hpp"

#include <Arduino.h>

#include <string.h>

#include "benchmark_config.hpp"
#include "placement_attrs.hpp"

namespace bench {
namespace {

constexpr size_t kSharedBufferBytes = 128u * 1024u;

BENCH_ALIGN(32) uint8_t g_global_buffer[kSharedBufferBytes];
BENCH_DMAMEM BENCH_ALIGN(32) uint8_t g_dmamem_buffer[kSharedBufferBytes];

}  // namespace

volatile uint32_t g_sink_u32 = 0;
volatile uint64_t g_sink_u64 = 0;
volatile float g_sink_f32 = 0.0f;
volatile double g_sink_f64 = 0.0;

void bench_consume_u32(uint32_t v) { g_sink_u32 ^= v; }
void bench_consume_u64(uint64_t v) { g_sink_u64 ^= v; }
void bench_consume_f32(float v) { g_sink_f32 += v; }
void bench_consume_f64(double v) { g_sink_f64 += v; }

void init_shared_buffers() {
  fill_u8_pattern(g_global_buffer, sizeof(g_global_buffer), kDeterministicSeed);
  fill_u8_pattern(g_dmamem_buffer, sizeof(g_dmamem_buffer), kDeterministicSeed ^ 0x55AA1234u);
}

uint8_t* global_buffer_base() { return g_global_buffer; }
uint8_t* dmamem_buffer_base() { return g_dmamem_buffer; }
size_t global_buffer_size() { return sizeof(g_global_buffer); }
size_t dmamem_buffer_size() { return sizeof(g_dmamem_buffer); }

uint32_t xorshift32(uint32_t& state) {
  uint32_t x = state;
  x ^= (x << 13);
  x ^= (x >> 17);
  x ^= (x << 5);
  state = x;
  return x;
}

uint32_t mix_hash_u32(uint32_t h, uint32_t v) {
  h ^= (v + 0x9e3779b9u + (h << 6) + (h >> 2));
  h = (h << 7) | (h >> (32 - 7));
  return h;
}

void fill_u8_pattern(uint8_t* dst, size_t bytes, uint32_t seed) {
  uint32_t s = seed;
  for (size_t i = 0; i < bytes; ++i) {
    dst[i] = static_cast<uint8_t>(xorshift32(s) & 0xFFu);
  }
}

void fill_u32_pattern(uint32_t* dst, size_t count, uint32_t seed) {
  uint32_t s = seed;
  for (size_t i = 0; i < count; ++i) {
    dst[i] = xorshift32(s) ^ static_cast<uint32_t>(i * 0x45D9F3Bu);
  }
}

uint32_t checksum_u8(const uint8_t* src, size_t bytes) {
  uint32_t h = 0x13579BDFu;
  for (size_t i = 0; i < bytes; ++i) {
    h = mix_hash_u32(h, src[i]);
  }
  return h;
}

uint32_t checksum_u32(const uint32_t* src, size_t count) {
  uint32_t h = 0x2468ACE1u;
  for (size_t i = 0; i < count; ++i) {
    h = mix_hash_u32(h, src[i]);
  }
  return h;
}

uint32_t param_u32(const BenchmarkParamSet& params, const char* key, uint32_t fallback) {
  if (params.params == nullptr || key == nullptr) {
    return fallback;
  }
  for (uint8_t i = 0; i < params.count; ++i) {
    if (params.params[i].key != nullptr && strcmp(params.params[i].key, key) == 0) {
      return static_cast<uint32_t>(params.params[i].value);
    }
  }
  return fallback;
}

int32_t param_i32(const BenchmarkParamSet& params, const char* key, int32_t fallback) {
  if (params.params == nullptr || key == nullptr) {
    return fallback;
  }
  for (uint8_t i = 0; i < params.count; ++i) {
    if (params.params[i].key != nullptr && strcmp(params.params[i].key, key) == 0) {
      return params.params[i].value;
    }
  }
  return fallback;
}

bool param_has(const BenchmarkParamSet& params, const char* key) {
  if (params.params == nullptr || key == nullptr) {
    return false;
  }
  for (uint8_t i = 0; i < params.count; ++i) {
    if (params.params[i].key != nullptr && strcmp(params.params[i].key, key) == 0) {
      return true;
    }
  }
  return false;
}

}  // namespace bench

