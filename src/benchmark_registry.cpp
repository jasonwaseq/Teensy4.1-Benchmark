#include "benchmark_registry.hpp"

#include <string.h>

namespace bench {
namespace {

const BenchmarkDef* g_registry[kMaxBenchmarks];
size_t g_registry_count = 0;

}  // namespace

bool register_benchmark(const BenchmarkDef& def) {
  if (g_registry_count >= kMaxBenchmarks) {
    return false;
  }
  g_registry[g_registry_count++] = &def;
  return true;
}

void clear_registry() {
  g_registry_count = 0;
}

size_t benchmark_count() {
  return g_registry_count;
}

const BenchmarkDef* benchmark_at(size_t idx) {
  if (idx >= g_registry_count) {
    return nullptr;
  }
  return g_registry[idx];
}

const BenchmarkDef* find_benchmark(const char* name) {
  if (name == nullptr) {
    return nullptr;
  }
  for (size_t i = 0; i < g_registry_count; ++i) {
    if (strcmp(g_registry[i]->name, name) == 0) {
      return g_registry[i];
    }
  }
  return nullptr;
}

void register_all_benchmarks(const RuntimeConfig& cfg) {
  clear_registry();
  register_compute_benchmarks();
  register_memory_benchmarks();
  register_placement_benchmarks();
  register_realtime_benchmarks();
  if (cfg.include_optional) {
    register_optional_benchmarks();
  }
}

}  // namespace bench

