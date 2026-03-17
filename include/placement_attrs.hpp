#pragma once

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#if defined(FASTRUN)
#define BENCH_FASTRUN FASTRUN
#else
#define BENCH_FASTRUN
#endif

#if defined(FLASHMEM)
#define BENCH_FLASHMEM FLASHMEM
#else
#define BENCH_FLASHMEM
#endif

#if defined(DMAMEM)
#define BENCH_DMAMEM DMAMEM
#else
#define BENCH_DMAMEM
#endif

#if defined(__GNUC__) || defined(__clang__)
#define BENCH_NOINLINE __attribute__((noinline))
#define BENCH_USED __attribute__((used))
#define BENCH_ALIGN(N) __attribute__((aligned(N)))
#else
#define BENCH_NOINLINE
#define BENCH_USED
#define BENCH_ALIGN(N)
#endif
