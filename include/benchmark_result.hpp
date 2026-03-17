#pragma once

#include <stdint.h>

#include "benchmark_stats.hpp"

namespace bench {

enum class Category : uint8_t {
  Compute = 0,
  Memory = 1,
  Placement = 2,
  Realtime = 3,
  Optional = 4
};

enum class CodePlacement : uint8_t {
  Default = 0,
  FastRun = 1,
  FlashMem = 2,
  NotApplicable = 3
};

enum class DataPlacement : uint8_t {
  Stack = 0,
  Global = 1,
  DmaMem = 2,
  NotApplicable = 3
};

enum class InterruptMode : uint8_t {
  IsolatedRaw = 0,
  RealisticIdle = 1,
  PeriodicIsr = 2,
  UsbActive = 3
};

constexpr uint8_t kMaxParams = 8;

struct ParamKV {
  const char* key;
  int32_t value;
};

struct BenchmarkParamSet {
  const char* label;
  const ParamKV* params;
  uint8_t count;
  uint32_t fixed_iterations;
  float ops_per_iter;
  float bytes_per_iter;
};

struct BenchmarkRunMeta {
  const char* run_id;
  uint32_t timestamp_ms;
  const char* board;
  uint32_t cpu_hz;
  const char* benchmark_name;
  Category category;
  CodePlacement code_placement;
  DataPlacement data_placement;
  InterruptMode interrupt_mode;
  const BenchmarkParamSet* param_set;
  uint16_t warmups;
  uint16_t trials;
  uint32_t iterations;
  const char* notes;
  bool validation_pass;
};

struct BenchmarkAggregate {
  BenchmarkRunMeta meta;
  uint32_t overhead_cycles;
  double raw_mean_cycles;
  double net_mean_cycles;
  StatsResult raw_stats;
  StatsResult net_stats;
  double cycles_per_op;
  double cycles_per_byte;
  double mb_per_s;
};

const char* category_to_cstr(Category category);
const char* code_placement_to_cstr(CodePlacement placement);
const char* data_placement_to_cstr(DataPlacement placement);
const char* interrupt_mode_to_cstr(InterruptMode mode);

bool parse_category(const char* text, Category& out);
bool parse_interrupt_mode(const char* text, InterruptMode& out);

}  // namespace bench

