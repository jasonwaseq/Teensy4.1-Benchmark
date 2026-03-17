#!/usr/bin/env python3
import argparse
import csv
from collections import defaultdict


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Quick plotting helper for benchmark CSV output.")
    p.add_argument("csv", help="CSV file from parse_results.py")
    p.add_argument("--category", default="", help="Optional category filter")
    p.add_argument("--metric", default="cycles_per_op", help="Metric column to plot")
    p.add_argument("--top", type=int, default=20, help="Show top N rows")
    return p.parse_args()


def read_rows(path: str):
    with open(path, "r", newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def main() -> None:
    args = parse_args()
    rows = read_rows(args.csv)
    if args.category:
        rows = [r for r in rows if r.get("category") == args.category]
    rows = [r for r in rows if r.get("validation_pass") in ("True", "true", "1", True)]

    metric = args.metric
    parsed = []
    for r in rows:
        try:
            val = float(r.get(metric, "nan"))
        except ValueError:
            continue
        parsed.append((r.get("benchmark", "unknown"), val, r))

    if not parsed:
        print("No rows to plot")
        return

    parsed.sort(key=lambda x: x[1])
    parsed = parsed[: args.top]

    try:
        import matplotlib.pyplot as plt
    except Exception as exc:  # pragma: no cover
        print(f"matplotlib is required for plotting: {exc}")
        return

    labels = [p[0] for p in parsed]
    values = [p[1] for p in parsed]

    plt.figure(figsize=(12, 6))
    plt.barh(labels, values)
    plt.xlabel(metric)
    title_cat = args.category if args.category else "all categories"
    plt.title(f"Top {len(parsed)} benchmarks by {metric} ({title_cat})")
    plt.gca().invert_yaxis()
    plt.tight_layout()
    plt.show()

    spread_by_cat = defaultdict(list)
    for _, _, row in parsed:
        try:
            p99 = float(row.get("p99_cycles") or 0.0)
            med = float(row.get("median_cycles") or 1.0)
        except ValueError:
            continue
        spread_by_cat[row.get("category", "unknown")].append(p99 / max(med, 1.0))

    print("p99/median summary for plotted rows:")
    for cat, arr in sorted(spread_by_cat.items()):
        avg = sum(arr) / len(arr)
        print(f"  {cat}: mean_ratio={avg:.3f} count={len(arr)}")


if __name__ == "__main__":
    main()
