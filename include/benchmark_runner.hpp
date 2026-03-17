#pragma once

#include <stdint.h>

#include "benchmark.hpp"
#include "benchmark_report.hpp"
#include "benchmark_timer.hpp"

namespace bench {

class BenchmarkRunner {
 public:
 explicit BenchmarkRunner(RuntimeConfig& cfg);

  RuntimeConfig& config();
  void run_all();
  void run_group(Category category);
  bool run_by_name(const char* name);
  void list_benchmarks();

 private:
  RuntimeConfig& cfg_;
  BenchmarkReporter reporter_;

  bool benchmark_selected(const BenchmarkDef& def) const;
  uint32_t choose_iterations(BenchmarkContext& ctx, const BenchmarkDef& def,
                             const BenchmarkParamSet& params, InterruptMode mode);
  uint32_t measure_overhead(BenchmarkContext& ctx, const BenchmarkDef& def, const BenchmarkParamSet& params,
                            uint32_t iterations, InterruptMode mode);
  uint32_t measure_single_trial(BenchmarkContext& ctx, const BenchmarkDef& def,
                                const BenchmarkParamSet& params, uint32_t iterations,
                                InterruptMode mode);
  bool run_single(const BenchmarkDef& def, const BenchmarkParamSet& params,
                  BenchmarkAggregate& out_result);
  void configure_background_mode(InterruptMode mode, bool enable);
};

}  // namespace bench
