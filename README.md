# Teensy 4.1 Architecture-Aware Benchmark Suite

Production-quality benchmark framework for **Teensy 4.1 (Cortex-M7 @ 600 MHz)**.

It benchmarks:
- compute kernels
- memory access patterns
- code placement (`default`, `FASTRUN`, `FLASHMEM`)
- data placement (`stack`, `global`, `DMAMEM`)
- real-time latency/jitter
- repeatability and statistical quality

---

## 1) Prerequisites

- Teensy 4.1 connected over USB
- Python 3.10+ (tested with Python 3.13)
- PlatformIO Core

Install required Python packages:

```powershell
python -m pip install --user -U platformio pyserial matplotlib
```

---

## 2) Build and Upload Firmware

From project root:

```powershell
cd C:\Users\jason\Documents\Teensy4.1-Benchmark
$env:PLATFORMIO_CORE_DIR = "$PWD\.pio_core"
python -m platformio run -e teensy41 -t upload
```

Notes:
- `PLATFORMIO_CORE_DIR` avoids global `%USERPROFILE%\.platformio` permission issues.
- Upload protocol defaults to Teensy GUI loader (`teensy-gui`).

---

## 3) Fastest Way to Run Everything (Recommended)

This mode avoids monitor typing issues and directly generates CSV.

```powershell
cd C:\Users\jason\Documents\Teensy4.1-Benchmark
$env:PLATFORMIO_CORE_DIR = "$PWD\.pio_core"
python scripts/run_capture_csv.py --port COM9 --baud 115200 --output benchmark_results.csv --summary --first-result-timeout 90
```

What this does:
- opens serial port
- sends commands: `set json 1`, `set summary 0`, `run-all`
- captures all `bench_result` JSON lines
- writes `benchmark_results.csv`
- writes raw serial log to `bench_raw.log`

If your port is not `COM9`, find it with:

```powershell
python -m platformio device list
```

---

## 4) Plot Results

Top 20 by cycles/op:

```powershell
python scripts/plot_results.py benchmark_results.csv --metric cycles_per_op --top 20
```

Memory-focused view by throughput:

```powershell
python scripts/plot_results.py benchmark_results.csv --category memory --metric mb_per_s --top 20
```

---

## 5) Interactive Monitor Mode (Optional)

Open monitor:

```powershell
python -m platformio device monitor -p COM9 -b 115200 --echo --filter send_on_enter --eol LF
```

Then type one command per line:

```text
help
show config
run-group compute
run-all
```

Useful runtime commands:
- `list`
- `run <benchmark_name>`
- `run-group <compute|memory|placement|realtime|optional>`
- `set trials <n>`
- `set warmups <n>`
- `set mode <isolated|realistic|periodic|usb>`
- `set json <0|1>`
- `set summary <0|1>`
- `set log <table|compact>`
- `set min_cycles <n>`
- `show config`

Defaults:
- human-readable logging: **on**
- JSON logging: **off**
- auto-run on boot: **off**

---

## 6) CSV From Existing Logs (Alternative)

If you already have a serial log text file:

```powershell
Get-Content .\bench_raw.log | python scripts/parse_results.py --output benchmark_results.csv --summary
```

Direct serial-to-CSV parser mode also exists:

```powershell
python scripts/parse_results.py --port COM9 --baud 115200 --output benchmark_results.csv --summary
```

---

## 7) Output Files

- `benchmark_results.csv`: flattened benchmark records for analysis
- `bench_raw.log`: raw serial stream captured during run

Each record includes benchmark metadata and stats:
- benchmark name/category
- params
- iters/warmups/trials
- validation pass/fail
- min/max/mean/median/stddev/p95/p99
- cycles/op, cycles/byte, MB/s (where applicable)

---

## 8) Troubleshooting

### No output in monitor
- press Teensy reset button once after opening monitor
- verify COM port with `python -m platformio device list`

### Commands seem ignored in monitor
- use one command per line
- avoid piping monitor output through `Tee-Object` when interactive typing
- use the non-interactive `run_capture_csv.py` workflow instead

### Capture script waits without first result
- ensure latest firmware is uploaded (auto-run is disabled by default)
- reset Teensy and rerun capture command
- verify port is correct and not used by another app

### Serial disconnect/reconnect mid-run
- close other serial tools
- rerun capture command

---

## 9) Methodology Guarantees

- cycle timing via DWT counter
- warmup/measurement/reporting are separated
- harness overhead estimated and subtracted
- deterministic seeds
- optimizer-resistance via volatile sinks/checksums/noinline
- no serial prints inside timed regions
- explicit interrupt mode per run

---

## 10) Build Flags

Set in `platformio.ini`:
- `BENCH_ENABLE_DMA` (0/1)
- `BENCH_ENABLE_DOUBLE` (0/1)
- `BENCH_ENABLE_OPTIONAL` (0/1)

---

## 11) Arduino IDE Compatibility

`TeensyBenchmark.ino` is a sketch shim.  
Core implementation lives in `src/` and `include/`.

