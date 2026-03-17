#include "benchmark_report.hpp"

#include <Arduino.h>

namespace bench {

BenchmarkReporter::BenchmarkReporter(RuntimeConfig& cfg) : cfg_(cfg) {}

void BenchmarkReporter::emit_run_header(size_t registered_benchmarks, uint32_t cpu_hz) {
  if (cfg_.emit_json) {
    Serial.print("{\"type\":\"run_header\",\"run_id\":\"");
    Serial.print(cfg_.run_id);
    Serial.print("\",\"board\":\"teensy41\",\"cpu_hz\":");
    Serial.print(cpu_hz);
    Serial.print(",\"registered_benchmarks\":");
    Serial.print(static_cast<uint32_t>(registered_benchmarks));
    Serial.print(",\"warmups\":");
    Serial.print(cfg_.warmups);
    Serial.print(",\"trials\":");
    Serial.print(cfg_.trials);
    Serial.print(",\"interrupt_mode\":\"");
    Serial.print(interrupt_mode_to_cstr(cfg_.interrupt_mode));
    Serial.println("\"}");
  }

  if (cfg_.emit_summary) {
    Serial.print("[header] run_id=");
    Serial.print(cfg_.run_id);
    Serial.print(" benches=");
    Serial.print(static_cast<uint32_t>(registered_benchmarks));
    Serial.print(" cpu_hz=");
    Serial.println(cpu_hz);
  }
}

void BenchmarkReporter::emit_result(const BenchmarkAggregate& result) {
  if (cfg_.emit_json) {
    emit_json_result(result);
  }
  if (cfg_.emit_summary) {
    emit_text_result(result);
  }
}

void BenchmarkReporter::emit_group_summary(Category category, uint16_t total, uint16_t passed,
                                           const char* best_name, double best_cycles_per_op,
                                           const char* worst_name,
                                           double worst_p99_over_median) {
  if (!cfg_.emit_summary) {
    return;
  }
  Serial.print("[");
  Serial.print(category_to_cstr(category));
  Serial.print("] total=");
  Serial.print(total);
  Serial.print(" pass=");
  Serial.print(passed);
  if (best_name != nullptr) {
    Serial.print(" best_cycles_per_op=");
    Serial.print(best_name);
    Serial.print(":");
    Serial.print(best_cycles_per_op, 6);
  }
  if (worst_name != nullptr) {
    Serial.print(" worst_p99_over_median=");
    Serial.print(worst_name);
    Serial.print(":");
    Serial.print(worst_p99_over_median, 3);
  }
  Serial.println();
}

void BenchmarkReporter::emit_json_result(const BenchmarkAggregate& result) {
  Serial.print("{\"type\":\"bench_result\",\"run_id\":\"");
  Serial.print(result.meta.run_id);
  Serial.print("\",\"timestamp_ms\":");
  Serial.print(result.meta.timestamp_ms);
  Serial.print(",\"board\":\"");
  Serial.print(result.meta.board);
  Serial.print("\",\"cpu_hz\":");
  Serial.print(result.meta.cpu_hz);
  Serial.print(",\"benchmark\":\"");
  Serial.print(result.meta.benchmark_name);
  Serial.print("\",\"category\":\"");
  Serial.print(category_to_cstr(result.meta.category));
  Serial.print("\",\"code_placement\":\"");
  Serial.print(code_placement_to_cstr(result.meta.code_placement));
  Serial.print("\",\"data_placement\":\"");
  Serial.print(data_placement_to_cstr(result.meta.data_placement));
  Serial.print("\",\"interrupt_mode\":\"");
  Serial.print(interrupt_mode_to_cstr(result.meta.interrupt_mode));
  Serial.print("\",\"params\":{");
  if (result.meta.param_set != nullptr && result.meta.param_set->params != nullptr) {
    for (uint8_t i = 0; i < result.meta.param_set->count; ++i) {
      if (i != 0u) {
        Serial.print(",");
      }
      Serial.print("\"");
      Serial.print(result.meta.param_set->params[i].key);
      Serial.print("\":");
      Serial.print(result.meta.param_set->params[i].value);
    }
  }
  Serial.print("},\"warmups\":");
  Serial.print(result.meta.warmups);
  Serial.print(",\"trials\":");
  Serial.print(result.meta.trials);
  Serial.print(",\"iters\":");
  Serial.print(result.meta.iterations);
  Serial.print(",\"validation_pass\":");
  Serial.print(result.meta.validation_pass ? "true" : "false");
  Serial.print(",\"overhead_cycles\":");
  Serial.print(result.overhead_cycles);
  Serial.print(",\"raw_mean_cycles\":");
  Serial.print(result.raw_mean_cycles, 3);
  Serial.print(",\"net_mean_cycles\":");
  Serial.print(result.net_mean_cycles, 3);
  Serial.print(",\"min_cycles\":");
  Serial.print(result.net_stats.min);
  Serial.print(",\"max_cycles\":");
  Serial.print(result.net_stats.max);
  Serial.print(",\"median_cycles\":");
  Serial.print(result.net_stats.median, 3);
  Serial.print(",\"stddev_cycles\":");
  Serial.print(result.net_stats.stddev, 3);
  Serial.print(",\"p95_cycles\":");
  Serial.print(result.net_stats.p95);
  Serial.print(",\"p99_cycles\":");
  Serial.print(result.net_stats.p99);
  Serial.print(",\"cycles_per_op\":");
  Serial.print(result.cycles_per_op, 6);
  Serial.print(",\"cycles_per_byte\":");
  Serial.print(result.cycles_per_byte, 6);
  Serial.print(",\"mb_per_s\":");
  Serial.print(result.mb_per_s, 3);
  Serial.print(",\"notes\":\"");
  Serial.print(result.meta.notes != nullptr ? result.meta.notes : "");
  Serial.println("\"}");
}

void BenchmarkReporter::emit_text_result(const BenchmarkAggregate& result) {
  Serial.print("[result] ");
  Serial.print(result.meta.benchmark_name);
  if (result.meta.param_set != nullptr && result.meta.param_set->label != nullptr) {
    Serial.print(".");
    Serial.print(result.meta.param_set->label);
  }
  Serial.print(" valid=");
  Serial.print(result.meta.validation_pass ? "1" : "0");
  Serial.print(" mean=");
  Serial.print(result.net_mean_cycles, 2);
  Serial.print(" cyc p95=");
  Serial.print(result.net_stats.p95);
  if (result.cycles_per_op > 0.0) {
    Serial.print(" cyc/op=");
    Serial.print(result.cycles_per_op, 5);
  }
  if (result.mb_per_s > 0.0) {
    Serial.print(" MB/s=");
    Serial.print(result.mb_per_s, 2);
  }
  Serial.println();
}

}  // namespace bench

