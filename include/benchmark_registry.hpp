#pragma once

#include <stddef.h>

#include "benchmark.hpp"
#include "benchmark_config.hpp"

namespace bench {

bool register_benchmark(const BenchmarkDef& def);
void clear_registry();
size_t benchmark_count();
const BenchmarkDef* benchmark_at(size_t idx);
const BenchmarkDef* find_benchmark(const char* name);

void register_compute_benchmarks();
void register_memory_benchmarks();
void register_placement_benchmarks();
void register_realtime_benchmarks();
void register_optional_benchmarks();
void register_all_benchmarks(const RuntimeConfig& cfg);

}  // namespace bench

