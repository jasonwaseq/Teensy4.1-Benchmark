#pragma once

#include <stddef.h>
#include <stdint.h>

#include "benchmark_config.hpp"
#include "benchmark_result.hpp"

namespace bench {

class BenchmarkReporter {
 public:
  explicit BenchmarkReporter(RuntimeConfig& cfg);

  void emit_run_header(size_t registered_benchmarks, uint32_t cpu_hz);
  void emit_group_start(Category category, uint16_t total_in_group);
  void emit_result(const BenchmarkAggregate& result, uint16_t ordinal_in_group,
                   uint16_t total_in_group);
  void emit_group_summary(Category category, uint16_t total, uint16_t passed, const char* best_name,
                          double best_cycles_per_op, const char* worst_name, double worst_p99_over_median);

 private:
  RuntimeConfig& cfg_;

  void emit_json_result(const BenchmarkAggregate& result);
  void emit_text_result(const BenchmarkAggregate& result, uint16_t ordinal_in_group,
                        uint16_t total_in_group);
};

}  // namespace bench
