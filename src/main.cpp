#include <Arduino.h>

#include "benchmark_command.hpp"
#include "benchmark_config.hpp"
#include "benchmark_registry.hpp"
#include "benchmark_runner.hpp"
#include "benchmark_timer.hpp"

namespace {

bench::RuntimeConfig g_config = bench::default_runtime_config();
bench::BenchmarkRunner g_runner(g_config);
bench::BenchmarkCommandInterface g_commands(g_runner, g_config);
bool g_started = false;

void maybe_wait_for_serial() {
  const uint32_t start_ms = millis();
  while (!Serial && (millis() - start_ms) < 3000u) {
  }
}

}  // namespace

void benchmark_setup() {
  Serial.begin(115200);
  maybe_wait_for_serial();

  bench::BenchmarkTimer::init();
  bench::set_run_id(g_config, static_cast<uint32_t>(millis()));
  bench::init_shared_buffers();
  bench::register_all_benchmarks(g_config);

  Serial.println("[boot] Teensy 4.1 architecture-aware benchmark suite");
  Serial.print("[boot] run_id=");
  Serial.println(g_config.run_id);
  if (g_config.auto_run_on_boot) {
    Serial.println("[boot] auto-run enabled");
  } else {
    Serial.println("[boot] auto-run disabled (type 'run-all' or use capture script)");
  }
  Serial.println("[boot] type 'help' for commands");

  if (g_config.auto_run_on_boot) {
    g_runner.run_all();
  }
  g_started = true;
}

void benchmark_loop() {
  if (!g_started) {
    return;
  }
  g_commands.poll();
}

void setup() {
  benchmark_setup();
}

void loop() {
  benchmark_loop();
}

#if !defined(ARDUINO)
int main() {
  benchmark_setup();
  for (;;) {
    benchmark_loop();
  }
}
#endif
