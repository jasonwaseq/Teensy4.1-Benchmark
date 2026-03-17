#pragma once

#include <stdint.h>

namespace bench {

class BenchmarkTimer {
 public:
  using TimedFn = void (*)(void*);

  static void init();
  static uint32_t now_cycles();
  static uint32_t measure_cycles(TimedFn fn, void* arg);
  static uint32_t cpu_hz();
};

}  // namespace bench

