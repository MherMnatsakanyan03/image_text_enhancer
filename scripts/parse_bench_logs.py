#!/usr/bin/env python3
"""
parse_bench_logs.py

Parse benchmark .txt log files from a directory and produce a comma-separated
CSV file suitable for importing into a paper figure.

Each row in the output corresponds to one benchmark run and contains:
  image_name  width  height  total_pixels  trials  avg_ms  min_ms  max_ms  stddev_ms

Usage:
    python3 parse_bench_logs.py <input_dir> [output_csv]
                                [--cores N] [--branch BRANCH]

Options:
    --cores N        Only include files matching *_<N>cores_*.txt
    --branch BRANCH  Only include files matching *_<BRANCH>.txt

Defaults:
    output_csv -> <input_dir>/bench_summary.csv

Example:
    python3 parse_bench_logs.py bench_logs.bak5.bak4/
    python3 parse_bench_logs.py bench_logs.bak5.bak4/ results.csv --cores 8 --branch main
    python3 parse_bench_logs.py bench_logs.bak5.bak4/ --cores 1 --branch optimizations
"""

import argparse
import re
import sys
from pathlib import Path


# ── Regex patterns ────────────────────────────────────────────────────────────
RE_IMAGE_INFO  = re.compile(
    r"Image Info:\s+(\S+)\s+\((\d+)x(\d+),\s*\d+\s+channels?\)"
)
RE_TRIALS      = re.compile(r"BENCHMARK RESULTS\s*\((\d+)\s+trials?\)")
RE_TOTAL       = re.compile(
    r"^TOTAL\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)", re.MULTILINE
)


def parse_file(path: Path) -> dict | None:
    """Return a dict with extracted fields, or None if the file cannot be parsed."""
    text = path.read_text(encoding="utf-8", errors="replace")

    m_info = RE_IMAGE_INFO.search(text)
    if not m_info:
        return None  # skip non-image benchmark files (e.g. bench_sources.md_*)

    m_trials = RE_TRIALS.search(text)
    m_total  = RE_TOTAL.search(text)

    if not (m_trials and m_total):
        print(f"[WARN] Could not parse all fields in {path.name}, skipping.", file=sys.stderr)
        return None

    image_name = m_info.group(1)
    width      = int(m_info.group(2))
    height     = int(m_info.group(3))
    trials     = int(m_trials.group(1))
    avg_ms     = float(m_total.group(1))
    min_ms     = float(m_total.group(2))
    max_ms     = float(m_total.group(3))
    stddev_ms  = float(m_total.group(4))

    return {
        "image_name":   image_name,
        "width":        width,
        "height":       height,
        "total_pixels": width * height,
        "trials":       trials,
        "avg_ms":       avg_ms,
        "min_ms":       min_ms,
        "max_ms":       max_ms,
        "stddev_ms":    stddev_ms,
    }


def main():
    parser = argparse.ArgumentParser(
        description="Parse benchmark log files into a CSV summary.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("input_dir", help="Directory containing .txt benchmark log files")
    parser.add_argument("output_csv", nargs="?", default=None,
                        help="Output CSV path (default: <input_dir>/bench_summary.csv)")
    parser.add_argument("--cores", type=int, default=None,
                        help="Only include files matching *_<N>cores_*.txt")
    parser.add_argument("--branch", default=None,
                        help="Only include files matching *_<BRANCH>.txt")
    args = parser.parse_args()

    input_dir  = Path(args.input_dir)
    output_csv = Path(args.output_csv) if args.output_csv else input_dir / "bench_summary.csv"

    if not input_dir.is_dir():
        print(f"[ERROR] Not a directory: {input_dir}", file=sys.stderr)
        sys.exit(1)

    txt_files = sorted(input_dir.glob("*.txt"))
    if not txt_files:
        print(f"[ERROR] No .txt files found in {input_dir}", file=sys.stderr)
        sys.exit(1)

    # Filter by --cores and/or --branch
    if args.cores is not None or args.branch is not None:
        def matches(f: Path) -> bool:
            stem = f.stem  # e.g. "bench_color_text_2K.jpg_8cores_main"
            parts = stem.rsplit("_", 2)  # split off at most the last 2 underscores
            # parts[-1] is branch, parts[-2] is Ncores
            if args.branch is not None and parts[-1] != args.branch:
                return False
            if args.cores is not None and parts[-2] != f"{args.cores}cores":
                return False
            return True
        txt_files = [f for f in txt_files if matches(f)]

    if not txt_files:
        print("[ERROR] No .txt files matched the given --cores/--branch filters.", file=sys.stderr)
        sys.exit(1)

    rows = []
    for f in txt_files:
        result = parse_file(f)
        if result is not None:
            rows.append(result)

    if not rows:
        print("[ERROR] No valid benchmark data found.", file=sys.stderr)
        sys.exit(1)

    # Sort by total pixel count (ascending) so the CSV is naturally ordered by image size
    rows.sort(key=lambda r: r["total_pixels"])

    KEYS = ["image_name", "width", "height", "total_pixels",
            "trials", "avg_ms", "min_ms", "max_ms", "stddev_ms"]

    header = ",".join(KEYS)
    lines  = [header]
    for r in rows:
        line = (
            f"{r['image_name']},"
            f"{r['width']},"
            f"{r['height']},"
            f"{r['total_pixels']},"
            f"{r['trials']},"
            f"{r['avg_ms']:.3f},"
            f"{r['min_ms']:.3f},"
            f"{r['max_ms']:.3f},"
            f"{r['stddev_ms']:.3f}"
        )
        lines.append(line)

    output_csv.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Written {len(rows)} row(s) to {output_csv}")
    print()
    for line in lines:
        print(line)


if __name__ == "__main__":
    main()

