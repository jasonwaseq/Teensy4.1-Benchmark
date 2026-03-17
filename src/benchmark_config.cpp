#include "benchmark_config.hpp"

#include <Arduino.h>

#include <stdio.h>
#include <string.h>

namespace bench {
namespace {

bool eq_nocase(const char* a, const char* b) {
  if (a == nullptr || b == nullptr) {
    return false;
  }
  while (*a && *b) {
    char ca = *a;
    char cb = *b;
    if (ca >= 'A' && ca <= 'Z') {
      ca = static_cast<char>(ca - 'A' + 'a');
    }
    if (cb >= 'A' && cb <= 'Z') {
      cb = static_cast<char>(cb - 'A' + 'a');
    }
    if (ca != cb) {
      return false;
    }
    ++a;
    ++b;
  }
  return (*a == '\0' && *b == '\0');
}

}  // namespace

RuntimeConfig default_runtime_config() {
  RuntimeConfig cfg = {};
  cfg.warmups = kDefaultWarmups;
  cfg.trials = kDefaultTrials;
  cfg.target_min_cycles = kTargetMinCycles;
  cfg.min_iters = kMinIters;
  cfg.max_iters = kMaxIters;
  cfg.interrupt_mode = InterruptMode::RealisticIdle;
  cfg.emit_json = true;
  cfg.emit_summary = true;
  cfg.distribution_mode = false;
  cfg.setup_each_trial_in_distribution = false;
  cfg.include_optional = true;
  cfg.auto_run_on_boot = true;
  cfg.filter_by_category = false;
  cfg.category_filter = Category::Compute;
  cfg.filter_by_name = false;
  cfg.name_filter[0] = '\0';
  snprintf(cfg.run_id, sizeof(cfg.run_id), "run_boot");
  return cfg;
}

void set_run_id(RuntimeConfig& cfg, uint32_t epoch_hint_ms) {
  snprintf(cfg.run_id, sizeof(cfg.run_id), "run_%010lu",
           static_cast<unsigned long>(epoch_hint_ms));
}

const char* category_to_cstr(Category category) {
  switch (category) {
    case Category::Compute:
      return "compute";
    case Category::Memory:
      return "memory";
    case Category::Placement:
      return "placement";
    case Category::Realtime:
      return "realtime";
    case Category::Optional:
      return "optional";
    default:
      return "unknown";
  }
}

const char* code_placement_to_cstr(CodePlacement placement) {
  switch (placement) {
    case CodePlacement::Default:
      return "default";
    case CodePlacement::FastRun:
      return "fastrun";
    case CodePlacement::FlashMem:
      return "flashmem";
    case CodePlacement::NotApplicable:
      return "na";
    default:
      return "unknown";
  }
}

const char* data_placement_to_cstr(DataPlacement placement) {
  switch (placement) {
    case DataPlacement::Stack:
      return "stack";
    case DataPlacement::Global:
      return "global";
    case DataPlacement::DmaMem:
      return "dmamem";
    case DataPlacement::NotApplicable:
      return "na";
    default:
      return "unknown";
  }
}

const char* interrupt_mode_to_cstr(InterruptMode mode) {
  switch (mode) {
    case InterruptMode::IsolatedRaw:
      return "isolated_raw";
    case InterruptMode::RealisticIdle:
      return "realistic_idle";
    case InterruptMode::PeriodicIsr:
      return "periodic_isr";
    case InterruptMode::UsbActive:
      return "usb_active";
    default:
      return "unknown";
  }
}

bool parse_category(const char* text, Category& out) {
  if (eq_nocase(text, "compute")) {
    out = Category::Compute;
    return true;
  }
  if (eq_nocase(text, "memory")) {
    out = Category::Memory;
    return true;
  }
  if (eq_nocase(text, "placement")) {
    out = Category::Placement;
    return true;
  }
  if (eq_nocase(text, "realtime")) {
    out = Category::Realtime;
    return true;
  }
  if (eq_nocase(text, "optional")) {
    out = Category::Optional;
    return true;
  }
  return false;
}

bool parse_interrupt_mode(const char* text, InterruptMode& out) {
  if (eq_nocase(text, "isolated") || eq_nocase(text, "isolated_raw")) {
    out = InterruptMode::IsolatedRaw;
    return true;
  }
  if (eq_nocase(text, "realistic") || eq_nocase(text, "realistic_idle")) {
    out = InterruptMode::RealisticIdle;
    return true;
  }
  if (eq_nocase(text, "periodic") || eq_nocase(text, "periodic_isr")) {
    out = InterruptMode::PeriodicIsr;
    return true;
  }
  if (eq_nocase(text, "usb") || eq_nocase(text, "usb_active")) {
    out = InterruptMode::UsbActive;
    return true;
  }
  return false;
}

}  // namespace bench
