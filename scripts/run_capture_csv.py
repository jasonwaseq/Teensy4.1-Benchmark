#!/usr/bin/env python3
"""
Run benchmark commands over serial and capture JSON bench results to CSV.

This avoids interactive monitor typing issues.
"""

import argparse
import json
import sys
import time
from pathlib import Path
from typing import Dict, List, Optional


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Send benchmark commands and capture CSV from serial.")
    p.add_argument("--port", required=True, help="Serial port, e.g. COM9")
    p.add_argument("--baud", type=int, default=115200, help="Serial baud rate")
    p.add_argument("--output", default="benchmark_results.csv", help="CSV output path")
    p.add_argument("--raw-log", default="bench_raw.log", help="Raw serial log output path")
    p.add_argument("--limit", type=int, default=0, help="Stop after N bench_result rows (0 = unlimited)")
    p.add_argument(
        "--idle-timeout",
        type=float,
        default=10.0,
        help="Stop after this many seconds with no new JSON result (after first result)",
    )
    p.add_argument(
        "--startup-delay",
        type=float,
        default=1.0,
        help="Delay after opening serial before sending commands",
    )
    p.add_argument(
        "--first-result-timeout",
        type=float,
        default=90.0,
        help="Abort if no first bench_result arrives within this many seconds",
    )
    p.add_argument(
        "--resend-after",
        type=float,
        default=15.0,
        help="If no first result yet, resend commands after this many seconds (0 disables)",
    )
    p.add_argument(
        "--status-interval",
        type=float,
        default=5.0,
        help="Print waiting status every N seconds while capturing",
    )
    p.add_argument(
        "--send",
        action="append",
        default=[],
        help="Command to send (repeatable). If omitted, defaults to set json 1 / set summary 0 / run-all.",
    )
    p.add_argument("--summary", action="store_true", help="Print category summary")
    return p.parse_args()


def require_pyserial():
    try:
        import serial  # type: ignore
    except Exception as exc:  # pragma: no cover
        raise SystemExit(f"pyserial is required: {exc}")
    return serial


def open_serial(serial_mod, port: str, baud: int):
    return serial_mod.Serial(port, baudrate=baud, timeout=0.5)


def send_commands(ser, commands: List[str]) -> None:
    for cmd in commands:
        ser.write((cmd.strip() + "\n").encode("utf-8"))
        ser.flush()
        time.sleep(0.2)


def flatten_record(rec: Dict) -> Dict:
    flat = dict(rec)
    params = flat.pop("params", {}) or {}
    for k, v in params.items():
        flat[f"param_{k}"] = v
    return flat


def write_csv(records: List[Dict], out_path: Path) -> None:
    import csv

    if not records:
        print("No bench_result records captured.", file=sys.stderr)
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

    with out_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=ordered)
        writer.writeheader()
        for row in flattened:
            writer.writerow(row)


def print_summary(records: List[Dict]) -> None:
    from collections import defaultdict

    by_cat = defaultdict(list)
    for r in records:
        by_cat[r.get("category", "unknown")].append(r)

    print("Summary:")
    for cat, rows in sorted(by_cat.items()):
        valid = [r for r in rows if r.get("validation_pass") is True]
        if not valid:
            print(f"  {cat}: no valid rows")
            continue
        best = min(valid, key=lambda r: float(r.get("cycles_per_op") or 1e30))
        print(
            f"  {cat}: rows={len(rows)} valid={len(valid)} "
            f"best={best.get('benchmark')} cyc/op={float(best.get('cycles_per_op') or 0.0):.6f}"
        )


def main() -> None:
    args = parse_args()
    serial_mod = require_pyserial()

    commands = args.send or ["set json 1", "set summary 0", "run-all"]
    print(f"Opening {args.port} @ {args.baud}...")

    records: List[Dict] = []
    seen_first_result = False
    last_result_time = time.time()
    start_time = time.time()
    last_status_print = start_time
    resent_once = False
    raw_log_path = Path(args.raw_log)
    out_path = Path(args.output)

    ser = open_serial(serial_mod, args.port, args.baud)
    try:
        time.sleep(args.startup_delay)
        ser.reset_input_buffer()
        print(f"Sending commands: {commands}")
        send_commands(ser, commands)

        with raw_log_path.open("w", encoding="utf-8", newline="\n") as rawf:
            while True:
                now = time.time()
                if args.status_interval > 0.0 and (now - last_status_print) >= args.status_interval:
                    print(
                        f"Waiting... elapsed={now - start_time:.1f}s "
                        f"rows={len(records)} first_result={'yes' if seen_first_result else 'no'}"
                    )
                    last_status_print = now

                if not seen_first_result:
                    if (
                        args.resend_after > 0.0
                        and not resent_once
                        and (now - start_time) >= args.resend_after
                    ):
                        print("No first result yet, resending commands once...")
                        send_commands(ser, commands)
                        resent_once = True

                    if (now - start_time) > args.first_result_timeout:
                        print(
                            "Timed out waiting for first bench_result. "
                            "Check COM port, reset the Teensy once, and retry."
                        )
                        break

                try:
                    raw = ser.readline()
                except serial_mod.SerialException:
                    print("Serial disconnected; retrying reconnect...")
                    ser.close()
                    time.sleep(0.8)
                    ser = open_serial(serial_mod, args.port, args.baud)
                    continue

                if not raw:
                    if seen_first_result and (time.time() - last_result_time) > args.idle_timeout:
                        print("Idle timeout reached; finishing capture.")
                        break
                    continue

                line = raw.decode(errors="replace").rstrip("\r\n")
                rawf.write(line + "\n")

                if not line.startswith("{"):
                    continue

                try:
                    obj = json.loads(line)
                except json.JSONDecodeError:
                    continue

                if obj.get("type") == "bench_result":
                    records.append(obj)
                    seen_first_result = True
                    last_result_time = time.time()
                    print(
                        f"[{len(records)}] {obj.get('benchmark')} "
                        f"iters={obj.get('iters')} valid={obj.get('validation_pass')}"
                    )
                    if args.limit and len(records) >= args.limit:
                        print("Limit reached; stopping.")
                        break
    finally:
        try:
            ser.close()
        except Exception:
            pass

    write_csv(records, out_path)
    print(f"Wrote {len(records)} rows to {out_path}")
    print(f"Raw serial log: {raw_log_path}")
    if args.summary:
        print_summary(records)


if __name__ == "__main__":
    main()
