#include "benchmark_report.hpp"

#include <Arduino.h>

#include <stdio.h>
#include <string.h>

namespace bench {
namespace {

const char* short_interrupt_mode(InterruptMode mode) {
  switch (mode) {
    case InterruptMode::IsolatedRaw:
      return "isol";
    case InterruptMode::RealisticIdle:
      return "idle";
    case InterruptMode::PeriodicIsr:
      return "peri";
    case InterruptMode::UsbActive:
      return "usb";
    default:
      return "unk";
  }
}

const char* human_log_style_to_cstr(HumanLogStyle style) {
  switch (style) {
    case HumanLogStyle::Table:
      return "table";
    case HumanLogStyle::Compact:
      return "compact";
    default:
      return "unknown";
  }
}

void build_benchmark_label(const BenchmarkAggregate& result, char* out, size_t out_len) {
  if (out_len == 0u) {
    return;
  }
  const char* bench = result.meta.benchmark_name ? result.meta.benchmark_name : "unknown";
  const char* param =
      (result.meta.param_set && result.meta.param_set->label) ? result.meta.param_set->label : "";
  if (param[0] != '\0') {
    snprintf(out, out_len, "%s.%s", bench, param);
  } else {
    snprintf(out, out_len, "%s", bench);
  }
}

void print_json_escaped(const char* text) {
  if (text == nullptr) {
    return;
  }
  for (const char* p = text; *p != '\0'; ++p) {
    const char c = *p;
    switch (c) {
      case '\\':
        Serial.print("\\\\");
        break;
      case '"':
        Serial.print("\\\"");
        break;
      case '\n':
        Serial.print("\\n");
        break;
      case '\r':
        Serial.print("\\r");
        break;
      case '\t':
        Serial.print("\\t");
        break;
      default: {
        const uint8_t uc = static_cast<uint8_t>(c);
        if (uc < 0x20u) {
          char esc[7] = {};
          snprintf(esc, sizeof(esc), "\\u%04x", static_cast<unsigned int>(uc));
          Serial.print(esc);
        } else {
          Serial.print(c);
        }
      } break;
    }
  }
}

}  // namespace

BenchmarkReporter::BenchmarkReporter(RuntimeConfig& cfg) : cfg_(cfg) {}

void BenchmarkReporter::emit_run_header(size_t registered_benchmarks, uint32_t cpu_hz) {
  if (cfg_.emit_json) {
    Serial.print("{\"type\":\"run_header\",\"run_id\":\"");
    print_json_escaped(cfg_.run_id);
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
    Serial.print("\",\"human_log_style\":\"");
    Serial.print(human_log_style_to_cstr(cfg_.human_log_style));
    Serial.println("\"}");
  }

  if (cfg_.emit_summary) {
    Serial.println();
    Serial.println("================================================================================");
    Serial.print("Run: ");
    Serial.print(cfg_.run_id);
    Serial.print(" | Board: teensy41 | CPU: ");
    Serial.print(cpu_hz);
    Serial.print(" Hz | Benches: ");
    Serial.println(static_cast<uint32_t>(registered_benchmarks));
    Serial.print("Mode: ");
    Serial.print(interrupt_mode_to_cstr(cfg_.interrupt_mode));
    Serial.print(" | Warmups: ");
    Serial.print(cfg_.warmups);
    Serial.print(" | Trials: ");
    Serial.print(cfg_.trials);
    Serial.print(" | Log: ");
    Serial.print(human_log_style_to_cstr(cfg_.human_log_style));
    Serial.print(" | JSON: ");
    Serial.print(cfg_.emit_json ? "on" : "off");
    Serial.println();
    Serial.println("================================================================================");
  }
}

void BenchmarkReporter::emit_group_start(Category category, uint16_t total_in_group) {
  if (!cfg_.emit_summary) {
    return;
  }

  Serial.println();
  Serial.println("--------------------------------------------------------------------------------");
  Serial.print("Group: ");
  Serial.print(category_to_cstr(category));
  Serial.print(" | Cases: ");
  Serial.println(total_in_group);
  if (cfg_.human_log_style == HumanLogStyle::Table) {
    Serial.println("idx    status benchmark                           iters      mean      p95    cyc/op"
                   "      MB/s mode");
    Serial.println("--------------------------------------------------------------------------------");
  } else {
    Serial.println("Compact format: [idx/total PASS|FAIL] benchmark | mean cyc | p95 | cyc/op | MB/s | mode");
    Serial.println("--------------------------------------------------------------------------------");
  }
}

void BenchmarkReporter::emit_result(const BenchmarkAggregate& result, uint16_t ordinal_in_group,
                                    uint16_t total_in_group) {
  if (cfg_.emit_json) {
    emit_json_result(result);
  }
  if (cfg_.emit_summary) {
    emit_text_result(result, ordinal_in_group, total_in_group);
  }
}

void BenchmarkReporter::emit_group_summary(Category category, uint16_t total, uint16_t passed,
                                           const char* best_name, double best_cycles_per_op,
                                           const char* worst_name,
                                           double worst_p99_over_median) {
  if (!cfg_.emit_summary) {
    return;
  }
  Serial.print("Summary ");
  Serial.print(category_to_cstr(category));
  Serial.print(": total=");
  Serial.print(total);
  Serial.print(", pass=");
  Serial.print(passed);
  if (best_name != nullptr) {
    Serial.print(", best cyc/op=");
    Serial.print(best_name);
    Serial.print(" (");
    Serial.print(best_cycles_per_op, 6);
    Serial.print(")");
  }
  if (worst_name != nullptr) {
    Serial.print(", worst p99/median=");
    Serial.print(worst_name);
    Serial.print(" (");
    Serial.print(worst_p99_over_median, 3);
    Serial.print(")");
  }
  Serial.println();
  Serial.println("--------------------------------------------------------------------------------");
}

void BenchmarkReporter::emit_json_result(const BenchmarkAggregate& result) {
  Serial.print("{\"type\":\"bench_result\",\"run_id\":\"");
  print_json_escaped(result.meta.run_id);
  Serial.print("\",\"timestamp_ms\":");
  Serial.print(result.meta.timestamp_ms);
  Serial.print(",\"board\":\"");
  print_json_escaped(result.meta.board);
  Serial.print("\",\"cpu_hz\":");
  Serial.print(result.meta.cpu_hz);
  Serial.print(",\"benchmark\":\"");
  print_json_escaped(result.meta.benchmark_name);
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
      print_json_escaped(result.meta.param_set->params[i].key);
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
  print_json_escaped(result.meta.notes != nullptr ? result.meta.notes : "");
  Serial.println("\"}");
}

void BenchmarkReporter::emit_text_result(const BenchmarkAggregate& result, uint16_t ordinal_in_group,
                                         uint16_t total_in_group) {
  char label[56] = {};
  build_benchmark_label(result, label, sizeof(label));

  char cyc_per_op[16] = "-";
  if (result.cycles_per_op > 0.0) {
    snprintf(cyc_per_op, sizeof(cyc_per_op), "%.4f", result.cycles_per_op);
  }

  char mbps[16] = "-";
  if (result.mb_per_s > 0.0) {
    snprintf(mbps, sizeof(mbps), "%.2f", result.mb_per_s);
  }

  if (cfg_.human_log_style == HumanLogStyle::Compact) {
    char line[220] = {};
    snprintf(line, sizeof(line), "[%u/%u %s] %s | mean=%.0f cyc | p95=%lu | cyc/op=%s | MB/s=%s | %s",
             static_cast<unsigned>(ordinal_in_group), static_cast<unsigned>(total_in_group),
             result.meta.validation_pass ? "PASS" : "FAIL", label, result.net_mean_cycles,
             static_cast<unsigned long>(result.net_stats.p95), cyc_per_op, mbps,
             short_interrupt_mode(result.meta.interrupt_mode));
    Serial.println(line);
    return;
  }

  char line[196] = {};
  snprintf(line, sizeof(line), "%-3u/%-3u %-6s %-35s %8lu %9.0f %8lu %9s %9s %4s",
           static_cast<unsigned>(ordinal_in_group), static_cast<unsigned>(total_in_group),
           result.meta.validation_pass ? "PASS" : "FAIL", label,
           static_cast<unsigned long>(result.meta.iterations), result.net_mean_cycles,
           static_cast<unsigned long>(result.net_stats.p95), cyc_per_op, mbps,
           short_interrupt_mode(result.meta.interrupt_mode));
  Serial.println(line);
}

}  // namespace bench

