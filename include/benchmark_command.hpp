#pragma once

#include "benchmark_runner.hpp"

namespace bench {

class BenchmarkCommandInterface {
 public:
  BenchmarkCommandInterface(BenchmarkRunner& runner, RuntimeConfig& cfg);
  void poll();

 private:
  BenchmarkRunner& runner_;
  RuntimeConfig& cfg_;
  char line_[128];
  uint8_t line_len_;

  void handle_line(char* line);
};

}  // namespace bench

