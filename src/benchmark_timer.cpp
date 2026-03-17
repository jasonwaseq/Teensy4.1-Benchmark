#include "benchmark_timer.hpp"

#include <Arduino.h>

#include "benchmark_config.hpp"

namespace bench {

void BenchmarkTimer::init() {
#if defined(__arm__) && defined(DWT_CTRL_CYCCNTENA_Msk)
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#endif
}

uint32_t BenchmarkTimer::now_cycles() {
#if defined(__arm__) && defined(DWT_CTRL_CYCCNTENA_Msk)
  return DWT->CYCCNT;
#else
  return static_cast<uint32_t>(micros());
#endif
}

uint32_t BenchmarkTimer::measure_cycles(TimedFn fn, void* arg) {
  const uint32_t start = now_cycles();
  fn(arg);
  const uint32_t end = now_cycles();
  return (end - start);
}

uint32_t BenchmarkTimer::cpu_hz() {
#if defined(F_CPU_ACTUAL)
  return static_cast<uint32_t>(F_CPU_ACTUAL);
#elif defined(F_CPU)
  return static_cast<uint32_t>(F_CPU);
#else
  return kCpuHzFallback;
#endif
}

}  // namespace bench

