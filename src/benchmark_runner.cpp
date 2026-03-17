#include "benchmark_runner.hpp"

#include <Arduino.h>

#include <string.h>

#include "benchmark.hpp"
#include "benchmark_registry.hpp"
#include "benchmark_stats.hpp"
#include "placement_attrs.hpp"

#if defined(ARDUINO_TEENSY41) || defined(TEENSYDUINO)
#include <IntervalTimer.h>
#endif

namespace bench {
namespace {

struct RunThunkArgs {
  BenchmarkContext* ctx;
  const BenchmarkDef* def;
  const BenchmarkParamSet* params;
  uint32_t iterations;
};

BENCH_NOINLINE void run_thunk(void* arg_ptr) {
  RunThunkArgs* arg = static_cast<RunThunkArgs*>(arg_ptr);
  arg->def->run(*arg->ctx, *arg->params, arg->iterations);
}

BENCH_NOINLINE void overhead_thunk(void* arg_ptr) {
  RunThunkArgs* arg = static_cast<RunThunkArgs*>(arg_ptr);
  bench_consume_u32(arg->iterations);
  bench_consume_u32(static_cast<uint32_t>(arg->params->count));
}

#if defined(ARDUINO_TEENSY41) || defined(TEENSYDUINO)
IntervalTimer g_periodic_isr_timer;
volatile uint32_t g_periodic_tick = 0;

void periodic_isr() {
  ++g_periodic_tick;
}
#endif

constexpr char kUsbLoadMessage[] =
    "bench_usb_load_ping_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ\n";

void apply_interrupt_mode_for_call(InterruptMode mode, BenchmarkTimer::TimedFn fn, void* fn_arg,
                                   uint32_t* measured_cycles) {
  if (mode == InterruptMode::IsolatedRaw) {
    noInterrupts();
    *measured_cycles = BenchmarkTimer::measure_cycles(fn, fn_arg);
    interrupts();
    return;
  }
  *measured_cycles = BenchmarkTimer::measure_cycles(fn, fn_arg);
}

void run_untimed(InterruptMode mode, BenchmarkTimer::TimedFn fn, void* fn_arg) {
  if (mode == InterruptMode::IsolatedRaw) {
    noInterrupts();
    fn(fn_arg);
    interrupts();
    return;
  }
  fn(fn_arg);
}

}  // namespace

BenchmarkRunner::BenchmarkRunner(RuntimeConfig& cfg) : cfg_(cfg), reporter_(cfg) {}

RuntimeConfig& BenchmarkRunner::config() {
  return cfg_;
}

bool BenchmarkRunner::benchmark_selected(const BenchmarkDef& def) const {
  if (cfg_.filter_by_category && def.category != cfg_.category_filter) {
    return false;
  }
  if (cfg_.filter_by_name && cfg_.name_filter[0] != '\0') {
    return (strstr(def.name, cfg_.name_filter) != nullptr);
  }
  return true;
}

void BenchmarkRunner::configure_background_mode(InterruptMode mode, bool enable) {
#if defined(ARDUINO_TEENSY41) || defined(TEENSYDUINO)
  if (mode == InterruptMode::PeriodicIsr) {
    if (enable) {
      g_periodic_tick = 0;
      g_periodic_isr_timer.begin(periodic_isr, 50);  // 20 kHz interrupt pressure.
    } else {
      g_periodic_isr_timer.end();
    }
  }
#else
  (void)mode;
  (void)enable;
#endif
}

uint32_t BenchmarkRunner::measure_single_trial(BenchmarkContext& ctx, const BenchmarkDef& def,
                                               const BenchmarkParamSet& params, uint32_t iterations,
                                               InterruptMode mode) {
  if (mode == InterruptMode::UsbActive) {
    Serial.write(reinterpret_cast<const uint8_t*>(kUsbLoadMessage), sizeof(kUsbLoadMessage) - 1u);
  }

  RunThunkArgs args = {&ctx, &def, &params, iterations};
  uint32_t cycles = 0;
  apply_interrupt_mode_for_call(mode, run_thunk, &args, &cycles);
  return cycles;
}

uint32_t BenchmarkRunner::measure_overhead(BenchmarkContext& ctx, const BenchmarkDef& def,
                                           const BenchmarkParamSet& params, uint32_t iterations,
                                           InterruptMode mode) {
  (void)def;
  uint32_t overhead_samples[9] = {};
  RunThunkArgs args = {&ctx, &def, &params, iterations};
  for (uint8_t i = 0; i < 9; ++i) {
    uint32_t cycles = 0;
    apply_interrupt_mode_for_call(mode, overhead_thunk, &args, &cycles);
    overhead_samples[i] = cycles;
  }
  const StatsResult overhead_stats = compute_stats({overhead_samples, 9});
  return overhead_stats.valid ? static_cast<uint32_t>(overhead_stats.median) : 0u;
}

uint32_t BenchmarkRunner::choose_iterations(BenchmarkContext& ctx, const BenchmarkDef& def,
                                            const BenchmarkParamSet& params, InterruptMode mode) {
  if (params.fixed_iterations > 0u) {
    return params.fixed_iterations;
  }

  const IterationPolicy policy = def.iter_policy;
  if (!policy.auto_scale) {
    return policy.min_iters;
  }

  uint32_t iterations = policy.min_iters;
  const uint32_t max_iters = policy.max_iters;
  const uint32_t target_cycles = policy.target_min_cycles;

  for (;;) {
    uint32_t probe_samples[3] = {};
    for (uint8_t i = 0; i < 3; ++i) {
      probe_samples[i] = measure_single_trial(ctx, def, params, iterations, mode);
    }
    const StatsResult probe_stats = compute_stats({probe_samples, 3});
    const uint32_t median_cycles =
        probe_stats.valid ? static_cast<uint32_t>(probe_stats.median) : 0u;
    if (median_cycles >= target_cycles || iterations >= max_iters) {
      break;
    }
    const uint32_t doubled = iterations << 1;
    iterations = (doubled > max_iters) ? max_iters : doubled;
  }
  return iterations;
}

bool BenchmarkRunner::run_single(const BenchmarkDef& def, const BenchmarkParamSet& params,
                                 BenchmarkAggregate& out_result) {
  BenchmarkContext ctx = {};
  ctx.global_buffer = global_buffer_base();
  ctx.dmamem_buffer = dmamem_buffer_base();
  ctx.global_buffer_bytes = global_buffer_size();
  ctx.dmamem_buffer_bytes = dmamem_buffer_size();
  ctx.seed = kDeterministicSeed;
  ctx.skip = false;
  ctx.skip_reason = nullptr;

  if (def.setup != nullptr) {
    if (!def.setup(ctx, params)) {
      return false;
    }
  }

  const InterruptMode mode = def.force_interrupt_mode ? def.forced_interrupt_mode : cfg_.interrupt_mode;
  configure_background_mode(mode, true);

  const uint32_t iterations = choose_iterations(ctx, def, params, mode);
  const uint32_t overhead = measure_overhead(ctx, def, params, iterations, mode);

  // Probe runs during auto-scaling can mutate benchmark state. Re-run setup so measured
  // warmup/timing phases always start from a clean deterministic state.
  if (def.setup != nullptr) {
    if (!def.setup(ctx, params)) {
      configure_background_mode(mode, false);
      return false;
    }
  }

  RunThunkArgs run_args = {&ctx, &def, &params, iterations};
  const uint16_t trial_count = (cfg_.trials == 0u) ? 1u : ((cfg_.trials > kMaxTrials) ? kMaxTrials : cfg_.trials);
  const uint16_t warmup_count = cfg_.warmups;

  if (!cfg_.distribution_mode) {
    for (uint16_t i = 0; i < warmup_count; ++i) {
      run_untimed(mode, run_thunk, &run_args);
    }
  }

  uint32_t raw_samples[kMaxTrials] = {};
  uint32_t net_samples[kMaxTrials] = {};
  bool validation_pass = true;

  for (uint16_t trial = 0; trial < trial_count; ++trial) {
    if (cfg_.distribution_mode) {
      if (cfg_.setup_each_trial_in_distribution && def.setup != nullptr) {
        if (def.teardown != nullptr && trial > 0u) {
          def.teardown(ctx, params);
        }
        ctx.seed = kDeterministicSeed + trial;
        if (!def.setup(ctx, params)) {
          validation_pass = false;
          break;
        }
      }
      for (uint16_t w = 0; w < warmup_count; ++w) {
        run_untimed(mode, run_thunk, &run_args);
      }
    }

    const uint32_t raw = measure_single_trial(ctx, def, params, iterations, mode);
    const uint32_t net = (raw > overhead) ? (raw - overhead) : 0u;
    raw_samples[trial] = raw;
    net_samples[trial] = net;

    if (cfg_.distribution_mode && cfg_.setup_each_trial_in_distribution && def.validate != nullptr) {
      if (!def.validate(ctx, params, iterations)) {
        validation_pass = false;
      }
    }
  }

  if (def.validate != nullptr && !(cfg_.distribution_mode && cfg_.setup_each_trial_in_distribution)) {
    validation_pass = def.validate(ctx, params, iterations);
  }

  if (def.teardown != nullptr) {
    def.teardown(ctx, params);
  }
  configure_background_mode(mode, false);

  const StatsResult raw_stats = compute_stats({raw_samples, trial_count});
  const StatsResult net_stats = compute_stats({net_samples, trial_count});

  out_result = {};
  out_result.meta.run_id = cfg_.run_id;
  out_result.meta.timestamp_ms = static_cast<uint32_t>(millis());
  out_result.meta.board = "teensy41";
  out_result.meta.cpu_hz = BenchmarkTimer::cpu_hz();
  out_result.meta.benchmark_name = def.name;
  out_result.meta.category = def.category;
  out_result.meta.code_placement = def.code_placement;
  out_result.meta.data_placement = def.data_placement;
  out_result.meta.interrupt_mode = mode;
  out_result.meta.param_set = &params;
  out_result.meta.warmups = warmup_count;
  out_result.meta.trials = trial_count;
  out_result.meta.iterations = iterations;
  out_result.meta.notes = def.notes;
  out_result.meta.validation_pass = validation_pass;

  out_result.overhead_cycles = overhead;
  out_result.raw_stats = raw_stats;
  out_result.net_stats = net_stats;
  out_result.raw_mean_cycles = raw_stats.mean;
  out_result.net_mean_cycles = net_stats.mean;

  const double total_ops =
      static_cast<double>(params.ops_per_iter) * static_cast<double>(iterations);
  const double total_bytes =
      static_cast<double>(params.bytes_per_iter) * static_cast<double>(iterations);

  out_result.cycles_per_op = (total_ops > 0.0) ? (net_stats.mean / total_ops) : 0.0;
  out_result.cycles_per_byte = (total_bytes > 0.0) ? (net_stats.mean / total_bytes) : 0.0;

  if (total_bytes > 0.0 && net_stats.mean > 0.0) {
    const double seconds = net_stats.mean / static_cast<double>(out_result.meta.cpu_hz);
    const double mb = total_bytes / (1024.0 * 1024.0);
    out_result.mb_per_s = mb / seconds;
  } else {
    out_result.mb_per_s = 0.0;
  }
  return true;
}

void BenchmarkRunner::run_group(Category category) {
  uint16_t total = 0;
  uint16_t passed = 0;
  const char* best_name = nullptr;
  double best_cycles_per_op = 0.0;
  const char* worst_name = nullptr;
  double worst_spread = 0.0;

  for (size_t i = 0; i < benchmark_count(); ++i) {
    const BenchmarkDef* def = benchmark_at(i);
    if (def == nullptr || def->category != category || !benchmark_selected(*def)) {
      continue;
    }

    const uint8_t param_count = (def->param_set_count == 0u) ? 1u : def->param_set_count;
    for (uint8_t p = 0; p < param_count; ++p) {
      static const BenchmarkParamSet kEmptyParamSet = {"default", nullptr, 0, 0, 0.0f, 0.0f};
      const BenchmarkParamSet& params =
          (def->param_set_count == 0u) ? kEmptyParamSet : def->param_sets[p];

      BenchmarkAggregate result = {};
      const bool executed = run_single(*def, params, result);
      if (!executed) {
        continue;
      }
      ++total;
      if (result.meta.validation_pass) {
        ++passed;
      }
      reporter_.emit_result(result);

      if (result.cycles_per_op > 0.0 && (best_name == nullptr || result.cycles_per_op < best_cycles_per_op)) {
        best_name = result.meta.benchmark_name;
        best_cycles_per_op = result.cycles_per_op;
      }

      if (result.net_stats.median > 0.0) {
        const double spread = static_cast<double>(result.net_stats.p99) / result.net_stats.median;
        if (worst_name == nullptr || spread > worst_spread) {
          worst_name = result.meta.benchmark_name;
          worst_spread = spread;
        }
      }
    }
  }

  reporter_.emit_group_summary(category, total, passed, best_name, best_cycles_per_op, worst_name,
                               worst_spread);
}

void BenchmarkRunner::run_all() {
  reporter_.emit_run_header(benchmark_count(), BenchmarkTimer::cpu_hz());
  run_group(Category::Compute);
  run_group(Category::Memory);
  run_group(Category::Placement);
  run_group(Category::Realtime);
  if (cfg_.include_optional) {
    run_group(Category::Optional);
  }
}

bool BenchmarkRunner::run_by_name(const char* name) {
  const BenchmarkDef* def = find_benchmark(name);
  if (def == nullptr) {
    return false;
  }
  const uint8_t param_count = (def->param_set_count == 0u) ? 1u : def->param_set_count;
  static const BenchmarkParamSet kEmptyParamSet = {"default", nullptr, 0, 0, 0.0f, 0.0f};
  for (uint8_t i = 0; i < param_count; ++i) {
    const BenchmarkParamSet& params = (def->param_set_count == 0u) ? kEmptyParamSet : def->param_sets[i];
    BenchmarkAggregate result = {};
    if (run_single(*def, params, result)) {
      reporter_.emit_result(result);
    }
  }
  return true;
}

void BenchmarkRunner::list_benchmarks() {
  Serial.println("[list] registered benchmarks:");
  for (size_t i = 0; i < benchmark_count(); ++i) {
    const BenchmarkDef* def = benchmark_at(i);
    if (def == nullptr) {
      continue;
    }
    Serial.print("  ");
    Serial.print(def->name);
    Serial.print(" category=");
    Serial.println(category_to_cstr(def->category));
  }
}

}  // namespace bench
