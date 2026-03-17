#pragma once

#include <stddef.h>
#include <stdint.h>

#include "benchmark_result.hpp"

namespace bench {

struct BenchmarkContext {
  uint8_t* global_buffer;
  uint8_t* dmamem_buffer;
  size_t global_buffer_bytes;
  size_t dmamem_buffer_bytes;
  uint32_t seed;

  uint32_t last_u32;
  uint64_t last_u64;
  float last_f32;
  double last_f64;

  uint32_t expected_u32;
  uint64_t expected_u64;
  float expected_f32;
  double expected_f64;

  bool skip;
  const char* skip_reason;
};

using BenchmarkSetupFn = bool (*)(BenchmarkContext& ctx, const BenchmarkParamSet& params);
using BenchmarkRunFn = void (*)(BenchmarkContext& ctx, const BenchmarkParamSet& params, uint32_t iterations);
using BenchmarkValidateFn = bool (*)(BenchmarkContext& ctx, const BenchmarkParamSet& params,
                                     uint32_t iterations);
using BenchmarkTeardownFn = void (*)(BenchmarkContext& ctx, const BenchmarkParamSet& params);

struct IterationPolicy {
  bool auto_scale;
  uint32_t min_iters;
  uint32_t max_iters;
  uint32_t target_min_cycles;
};

struct BenchmarkDef {
  const char* name;
  Category category;
  CodePlacement code_placement;
  DataPlacement data_placement;
  const char* notes;

  BenchmarkSetupFn setup;
  BenchmarkRunFn run;
  BenchmarkValidateFn validate;
  BenchmarkTeardownFn teardown;

  const BenchmarkParamSet* param_sets;
  uint8_t param_set_count;
  IterationPolicy iter_policy;

  bool force_interrupt_mode;
  InterruptMode forced_interrupt_mode;
};

extern volatile uint32_t g_sink_u32;
extern volatile uint64_t g_sink_u64;
extern volatile float g_sink_f32;
extern volatile double g_sink_f64;

void bench_consume_u32(uint32_t v);
void bench_consume_u64(uint64_t v);
void bench_consume_f32(float v);
void bench_consume_f64(double v);

void init_shared_buffers();
uint8_t* global_buffer_base();
uint8_t* dmamem_buffer_base();
size_t global_buffer_size();
size_t dmamem_buffer_size();

uint32_t xorshift32(uint32_t& state);
void fill_u8_pattern(uint8_t* dst, size_t bytes, uint32_t seed);
void fill_u32_pattern(uint32_t* dst, size_t count, uint32_t seed);
uint32_t checksum_u8(const uint8_t* src, size_t bytes);
uint32_t checksum_u32(const uint32_t* src, size_t count);
uint32_t mix_hash_u32(uint32_t h, uint32_t v);

uint32_t param_u32(const BenchmarkParamSet& params, const char* key, uint32_t fallback);
int32_t param_i32(const BenchmarkParamSet& params, const char* key, int32_t fallback);
bool param_has(const BenchmarkParamSet& params, const char* key);

}  // namespace bench

