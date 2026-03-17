#!/usr/bin/env python3
import argparse
import csv
import json
import sys
from collections import defaultdict
from typing import Dict, List


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Parse Teensy benchmark JSONL serial output into CSV.")
    p.add_argument("--port", help="Serial port (optional; if omitted, read from stdin)")
    p.add_argument("--baud", type=int, default=115200, help="Serial baud rate")
    p.add_argument("--output", default="benchmark_results.csv", help="CSV output path")
    p.add_argument("--limit", type=int, default=0, help="Stop after N bench_result rows (0 = unlimited)")
    p.add_argument("--summary", action="store_true", help="Print top-level summaries")
    p.add_argument("--baseline", help="Optional baseline CSV to compare against")
    return p.parse_args()


def line_iter(args: argparse.Namespace):
    if args.port:
        try:
            import serial  # type: ignore
        except Exception as exc:  # pragma: no cover
            raise SystemExit(f"pyserial is required for --port mode: {exc}")

        ser = serial.Serial(args.port, args.baud, timeout=1.0)
        try:
            while True:
                raw = ser.readline()
                if not raw:
                    continue
                yield raw.decode(errors="replace").strip()
        finally:
            ser.close()
    else:
        for line in sys.stdin:
            yield line.strip()


def collect_records(args: argparse.Namespace) -> List[Dict]:
    records: List[Dict] = []
    for line in line_iter(args):
        if not line.startswith("{"):
            continue
        try:
            obj = json.loads(line)
        except json.JSONDecodeError:
            continue
        if obj.get("type") != "bench_result":
            continue
        records.append(obj)
        if args.limit and len(records) >= args.limit:
            break
    return records


def flatten_record(rec: Dict) -> Dict:
    flat = dict(rec)
    params = flat.pop("params", {}) or {}
    for k, v in params.items():
        flat[f"param_{k}"] = v
    return flat


def write_csv(records: List[Dict], out_path: str) -> None:
    if not records:
        print("No bench_result records found.", file=sys.stderr)
        return

    flattened = [flatten_record(r) for r in records]
    columns = set()
    for row in flattened:
        columns.update(row.keys())

    preferred = [
        "run_id",
        "timestamp_ms",
        "benchmark",
        "category",
        "cpu_hz",
        "code_placement",
        "data_placement",
        "interrupt_mode",
        "warmups",
        "trials",
        "iters",
        "validation_pass",
        "raw_mean_cycles",
        "net_mean_cycles",
        "min_cycles",
        "max_cycles",
        "median_cycles",
        "stddev_cycles",
        "p95_cycles",
        "p99_cycles",
        "cycles_per_op",
        "cycles_per_byte",
        "mb_per_s",
        "notes",
    ]
    ordered = [c for c in preferred if c in columns]
    ordered.extend(sorted(c for c in columns if c not in ordered))

    with open(out_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=ordered)
        writer.writeheader()
        for row in flattened:
            writer.writerow(row)


def print_summary(records: List[Dict]) -> None:
    by_cat: Dict[str, List[Dict]] = defaultdict(list)
    for r in records:
        by_cat[r.get("category", "unknown")].append(r)

    print("Summary:")
    for cat, rows in sorted(by_cat.items()):
        valid_rows = [r for r in rows if r.get("validation_pass") is True]
        if not valid_rows:
            print(f"  {cat}: no valid rows")
            continue
        best = min(valid_rows, key=lambda r: float(r.get("cycles_per_op") or 1e30))
        worst_jitter = max(
            valid_rows,
            key=lambda r: (float(r.get("p99_cycles") or 0.0) / max(float(r.get("median_cycles") or 1.0), 1.0)),
        )
        ratio = float(worst_jitter.get("p99_cycles") or 0.0) / max(float(worst_jitter.get("median_cycles") or 1.0), 1.0)
        print(
            f"  {cat}: rows={len(rows)} valid={len(valid_rows)} "
            f"best={best.get('benchmark')} cyc/op={float(best.get('cycles_per_op') or 0.0):.6f} "
            f"worst_p99_over_median={worst_jitter.get('benchmark')} ratio={ratio:.3f}"
        )


def baseline_compare(records: List[Dict], baseline_csv: str) -> None:
    base = {}
    with open(baseline_csv, "r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            name = row.get("benchmark")
            mode = row.get("interrupt_mode")
            param_n = row.get("param_n", "")
            key = (name, mode, param_n)
            try:
                base[key] = float(row.get("net_mean_cycles", "nan"))
            except ValueError:
                continue

    print("Baseline comparison (net_mean_cycles delta %):")
    for rec in records:
        name = rec.get("benchmark")
        mode = rec.get("interrupt_mode")
        param_n = str((rec.get("params") or {}).get("n", ""))
        key = (name, mode, param_n)
        if key not in base:
            continue
        cur = float(rec.get("net_mean_cycles") or 0.0)
        old = base[key]
        if old <= 0:
            continue
        delta = ((cur - old) / old) * 100.0
        print(f"  {name} mode={mode} n={param_n}: {delta:+.2f}%")


def main() -> None:
    args = parse_args()
    records = collect_records(args)
    write_csv(records, args.output)
    print(f"Wrote {len(records)} records to {args.output}")
    if args.summary:
        print_summary(records)
    if args.baseline and records:
        baseline_compare(records, args.baseline)


if __name__ == "__main__":
    main()
