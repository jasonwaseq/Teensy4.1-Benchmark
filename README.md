# Teensy 4.1 Architecture-Aware Benchmark Suite

Production-quality benchmark framework for **Teensy 4.1 (Cortex-M7 @ 600 MHz)** focused on:

- compute performance
- memory behavior
- code placement effects (`default` vs `FASTRUN` vs `FLASHMEM`)
- data placement effects (`stack` vs `global` vs `DMAMEM`)
- real-time latency/jitter distributions
- repeatability and methodology quality

This is a reusable framework, not a single-loop toy benchmark.

## Features

- Cycle-accurate timing with `ARM_DWT_CYCCNT`
- Warmup, timed measurement, and reporting phases are strictly separated
- Harness overhead estimation and subtraction
- Auto iteration scaling to reach stable timing windows
- Correctness validation per benchmark (performance is always paired with pass/fail)
- Statistics: `min`, `max`, `mean`, `median`, `stddev`, `p95`, `p99`
- Metrics: total cycles, cycles/op, cycles/byte, throughput (MB/s where applicable)
- Line-oriented machine-readable JSON output over USB serial
- Human-readable summary lines
- Runtime command interface over serial
- Host scripts for JSONL -> CSV and basic plotting

## Project Layout

```text
include/
  benchmark*.hpp
  placement_attrs.hpp
src/
  main.cpp
  benchmark_*.cpp
  bench_*.cpp
scripts/
  parse_results.py
  plot_results.py
platformio.ini
TeensyBenchmark.ino
```

## Requirements

- Teensy 4.1
- Python 3.10+ (tested with Python 3.13)
- PlatformIO Core (`python -m platformio`)

## Build and Upload (PowerShell, verified in this environment)

Use this exact flow:

```powershell
cd C:\Users\jason\Documents\Teensy4.1-Benchmark
$env:PLATFORMIO_CORE_DIR = "$PWD\.pio_core"
python -m platformio run -e teensy41
python -m platformio run -e teensy41 -t upload
```

Notes:
- Using `PLATFORMIO_CORE_DIR` locally avoids global `%USERPROFILE%\.platformio` permission problems.
- Upload protocol defaults to Teensy GUI loader (`teensy-gui`) unless changed in `platformio.ini`.

## Serial Monitor

```powershell
python -m platformio device monitor -b 115200
```

## Runtime Commands

Type these in serial monitor:

- `help`
- `list`
- `run-all`
- `run <benchmark_name>`
- `run-group <compute|memory|placement|realtime|optional>`
- `set trials <n>`
- `set warmups <n>`
- `set mode <isolated|realistic|periodic|usb>`
- `set json <0|1>`
- `set summary <0|1>`
- `set min_cycles <n>`

## Host-Side Result Processing

Install host dependencies:

```powershell
python -m pip install --user pyserial matplotlib
```

Capture JSONL from serial and write CSV:

```powershell
python scripts/parse_results.py --port COMx --baud 115200 --output benchmark_results.csv --summary
```

Plot top rows:

```powershell
python scripts/plot_results.py benchmark_results.csv --metric cycles_per_op --top 20
```

## Methodology Guarantees

- No `Serial.print` in timed benchmark regions
- Deterministic pseudo-random seeds for repeatability
- Optimizer-resistance via volatile sinks/checksums/noinline kernels
- Warmups are discarded from timed statistics
- Validation runs outside timed region
- Interrupt mode is explicit per run (`isolated_raw`, `realistic_idle`, `periodic_isr`, `usb_active`)

## Build Configuration Flags

Set in `platformio.ini` build flags:

- `BENCH_ENABLE_DMA` (0/1)
- `BENCH_ENABLE_DOUBLE` (0/1)
- `BENCH_ENABLE_OPTIONAL` (0/1)

## Arduino IDE Compatibility

`TeensyBenchmark.ino` is provided as a sketch shim.
Core implementation is in `src/main.cpp` and the C++ module tree.

