#!/usr/bin/env python3
"""Plot BDSS CSV metrics.

Usage:
    python tools/plot_metrics.py simulation_log.csv --out metrics.png
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


def read_series(path: Path):
    time = []
    total_queue = []
    waiting_for_seat = []
    seat_utilization = []
    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            time.append(int(row["time"]))
            total_queue.append(int(row["total_queue_length"]))
            waiting_for_seat.append(int(row["waiting_for_seat_count"]))
            seat_utilization.append(float(row["seat_utilization"]) * 100.0)
    return time, total_queue, waiting_for_seat, seat_utilization


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", type=Path)
    parser.add_argument("--out", type=Path, default=Path("bdss_metrics.png"))
    args = parser.parse_args()

    import matplotlib.pyplot as plt

    time, total_queue, waiting_for_seat, seat_utilization = read_series(args.csv)
    if not time:
        raise SystemExit("CSV has no records")

    fig, ax1 = plt.subplots(figsize=(11, 6))
    ax1.plot(time, total_queue, label="Total queue")
    ax1.plot(time, waiting_for_seat, label="Waiting for seat")
    ax1.set_xlabel("Simulation time / s")
    ax1.set_ylabel("Students")
    ax1.grid(True, alpha=0.3)

    ax2 = ax1.twinx()
    ax2.plot(time, seat_utilization, label="Seat utilization (%)", linestyle="--")
    ax2.set_ylabel("Seat utilization / %")

    lines = ax1.get_lines() + ax2.get_lines()
    ax1.legend(lines, [line.get_label() for line in lines], loc="upper right")
    fig.tight_layout()
    fig.savefig(args.out, dpi=160)
    print(f"Saved {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
