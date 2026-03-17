#include "benchmark_command.hpp"

#include <Arduino.h>

#include <stdlib.h>
#include <string.h>

namespace bench {

BenchmarkCommandInterface::BenchmarkCommandInterface(BenchmarkRunner& runner, RuntimeConfig& cfg)
    : runner_(runner), cfg_(cfg), line_(), line_len_(0) {}

void BenchmarkCommandInterface::poll() {
  while (Serial.available() > 0) {
    const int ch = Serial.read();
    if (ch < 0) {
      return;
    }
    if (ch == '\n' || ch == '\r') {
      if (line_len_ > 0) {
        line_[line_len_] = '\0';
        handle_line(line_);
        line_len_ = 0;
      }
      continue;
    }
    if (line_len_ < static_cast<uint8_t>(sizeof(line_) - 1u)) {
      line_[line_len_++] = static_cast<char>(ch);
    }
  }
}

void BenchmarkCommandInterface::handle_line(char* line) {
  char* cmd = strtok(line, " \t");
  if (cmd == nullptr) {
    return;
  }

  if (strcmp(cmd, "help") == 0) {
    Serial.println("commands:");
    Serial.println("  help");
    Serial.println("  list");
    Serial.println("  run <benchmark_name>");
    Serial.println("  run-group <compute|memory|placement|realtime|optional>");
    Serial.println("  run-all");
    Serial.println("  set trials <n>");
    Serial.println("  set warmups <n>");
    Serial.println("  set mode <isolated|realistic|periodic|usb>");
    Serial.println("  set json <0|1>");
    Serial.println("  set summary <0|1>");
    Serial.println("  set min_cycles <n>");
    Serial.println("  set dist <0|1>");
    Serial.println("  set dist_setup_each <0|1>");
    Serial.println("  set filter_name <substr|none>");
    Serial.println("  set filter_category <category|none>");
    return;
  }

  if (strcmp(cmd, "list") == 0) {
    runner_.list_benchmarks();
    return;
  }

  if (strcmp(cmd, "run-all") == 0) {
    runner_.run_all();
    return;
  }

  if (strcmp(cmd, "run") == 0) {
    char* name = strtok(nullptr, " \t");
    if (name == nullptr) {
      Serial.println("[error] missing benchmark name");
      return;
    }
    if (!runner_.run_by_name(name)) {
      Serial.println("[error] benchmark not found");
    }
    return;
  }

  if (strcmp(cmd, "run-group") == 0) {
    char* group = strtok(nullptr, " \t");
    if (group == nullptr) {
      Serial.println("[error] missing category");
      return;
    }
    Category category;
    if (!parse_category(group, category)) {
      Serial.println("[error] unknown category");
      return;
    }
    runner_.run_group(category);
    return;
  }

  if (strcmp(cmd, "set") == 0) {
    char* key = strtok(nullptr, " \t");
    char* value = strtok(nullptr, " \t");
    if (key == nullptr || value == nullptr) {
      Serial.println("[error] usage: set <key> <value>");
      return;
    }

    if (strcmp(key, "trials") == 0) {
      cfg_.trials = static_cast<uint16_t>(strtoul(value, nullptr, 10));
      Serial.println("[ok] trials updated");
      return;
    }
    if (strcmp(key, "warmups") == 0) {
      cfg_.warmups = static_cast<uint16_t>(strtoul(value, nullptr, 10));
      Serial.println("[ok] warmups updated");
      return;
    }
    if (strcmp(key, "json") == 0) {
      cfg_.emit_json = (strtoul(value, nullptr, 10) != 0u);
      Serial.println("[ok] json toggle updated");
      return;
    }
    if (strcmp(key, "summary") == 0) {
      cfg_.emit_summary = (strtoul(value, nullptr, 10) != 0u);
      Serial.println("[ok] summary toggle updated");
      return;
    }
    if (strcmp(key, "min_cycles") == 0) {
      cfg_.target_min_cycles = strtoul(value, nullptr, 10);
      Serial.println("[ok] target min cycles updated");
      return;
    }
    if (strcmp(key, "dist") == 0) {
      cfg_.distribution_mode = (strtoul(value, nullptr, 10) != 0u);
      Serial.println("[ok] distribution mode updated");
      return;
    }
    if (strcmp(key, "dist_setup_each") == 0) {
      cfg_.setup_each_trial_in_distribution = (strtoul(value, nullptr, 10) != 0u);
      Serial.println("[ok] distribution setup_each updated");
      return;
    }
    if (strcmp(key, "mode") == 0) {
      InterruptMode mode;
      if (!parse_interrupt_mode(value, mode)) {
        Serial.println("[error] unknown mode");
        return;
      }
      cfg_.interrupt_mode = mode;
      Serial.println("[ok] interrupt mode updated");
      return;
    }
    if (strcmp(key, "filter_name") == 0) {
      if (strcmp(value, "none") == 0) {
        cfg_.filter_by_name = false;
        cfg_.name_filter[0] = '\0';
      } else {
        cfg_.filter_by_name = true;
        strncpy(cfg_.name_filter, value, sizeof(cfg_.name_filter) - 1u);
        cfg_.name_filter[sizeof(cfg_.name_filter) - 1u] = '\0';
      }
      Serial.println("[ok] name filter updated");
      return;
    }
    if (strcmp(key, "filter_category") == 0) {
      if (strcmp(value, "none") == 0) {
        cfg_.filter_by_category = false;
      } else {
        Category category;
        if (!parse_category(value, category)) {
          Serial.println("[error] unknown category");
          return;
        }
        cfg_.filter_by_category = true;
        cfg_.category_filter = category;
      }
      Serial.println("[ok] category filter updated");
      return;
    }
    Serial.println("[error] unknown set key");
    return;
  }

  Serial.println("[error] unknown command, use help");
}

}  // namespace bench

