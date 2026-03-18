#include "benchmark_command.hpp"

#include <Arduino.h>

#include <stdlib.h>
#include <string.h>

namespace bench {
namespace {

void print_config(const RuntimeConfig& cfg) {
  Serial.println("[config]");
  Serial.print("  trials=");
  Serial.println(cfg.trials);
  Serial.print("  warmups=");
  Serial.println(cfg.warmups);
  Serial.print("  mode=");
  Serial.println(interrupt_mode_to_cstr(cfg.interrupt_mode));
  Serial.print("  json=");
  Serial.println(cfg.emit_json ? 1 : 0);
  Serial.print("  summary=");
  Serial.println(cfg.emit_summary ? 1 : 0);
  Serial.print("  log=");
  Serial.println(cfg.human_log_style == HumanLogStyle::Table ? "table" : "compact");
  Serial.print("  min_cycles=");
  Serial.println(cfg.target_min_cycles);
  Serial.print("  distribution=");
  Serial.println(cfg.distribution_mode ? 1 : 0);
}

}  // namespace

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
    Serial.println("  set log <table|compact>");
    Serial.println("  set min_cycles <n>");
    Serial.println("  set dist <0|1>");
    Serial.println("  set dist_setup_each <0|1>");
    Serial.println("  set filter_name <substr|none>");
    Serial.println("  set filter_category <category|none>");
    Serial.println("  show config");
    Serial.println("note: human-readable output is default; use 'set json 1' for JSON lines");
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
      uint32_t v = strtoul(value, nullptr, 10);
      if (v == 0u) {
        v = 1u;
      }
      if (v > kMaxTrials) {
        v = kMaxTrials;
      }
      cfg_.trials = static_cast<uint16_t>(v);
      Serial.print("[ok] trials=");
      Serial.println(cfg_.trials);
      return;
    }
    if (strcmp(key, "warmups") == 0) {
      uint32_t v = strtoul(value, nullptr, 10);
      if (v > 1024u) {
        v = 1024u;
      }
      cfg_.warmups = static_cast<uint16_t>(v);
      Serial.print("[ok] warmups=");
      Serial.println(cfg_.warmups);
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
    if (strcmp(key, "log") == 0) {
      HumanLogStyle style;
      if (!parse_human_log_style(value, style)) {
        Serial.println("[error] unknown log style; use table|compact");
        return;
      }
      cfg_.human_log_style = style;
      Serial.print("[ok] log style=");
      Serial.println(cfg_.human_log_style == HumanLogStyle::Table ? "table" : "compact");
      return;
    }
    if (strcmp(key, "min_cycles") == 0) {
      uint32_t v = strtoul(value, nullptr, 10);
      if (v < 1000u) {
        v = 1000u;
      }
      cfg_.target_min_cycles = v;
      Serial.print("[ok] target min cycles=");
      Serial.println(cfg_.target_min_cycles);
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

  if (strcmp(cmd, "show") == 0) {
    char* what = strtok(nullptr, " \t");
    if (what != nullptr && strcmp(what, "config") == 0) {
      print_config(cfg_);
      return;
    }
    Serial.println("[error] usage: show config");
    return;
  }

  Serial.println("[error] unknown command, use help");
}

}  // namespace bench
