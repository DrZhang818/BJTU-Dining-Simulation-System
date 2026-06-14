#!/usr/bin/env python3
"""Plot common BDSS CSV metrics.

Usage:
  python tools/plot_metrics.py simulation_log.csv --out charts
"""

import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt


def read_rows(path: Path):
    with path.open(newline='', encoding='utf-8') as f:
        return list(csv.DictReader(f))


def plot_metric(rows, key: str, title: str, out_dir: Path):
    times = [int(r['time']) / 60 for r in rows]
    values = [float(r[key]) for r in rows]
    plt.figure(figsize=(9, 4.8))
    plt.plot(times, values)
    plt.title(title)
    plt.xlabel('Time (minutes)')
    plt.ylabel(key)
    plt.tight_layout()
    out_path = out_dir / f'{key}.png'
    plt.savefig(out_path, dpi=160)
    plt.close()
    print(out_path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('csv_path', type=Path)
    parser.add_argument('--out', type=Path, default=Path('charts'))
    args = parser.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)
    rows = read_rows(args.csv_path)
    plot_metric(rows, 'total_queue_length', 'Total queue length over time', args.out)
    plot_metric(rows, 'waiting_for_seat_count', 'Students waiting for seats over time', args.out)
    plot_metric(rows, 'seat_utilization', 'Seat utilization over time', args.out)


if __name__ == '__main__':
    main()
