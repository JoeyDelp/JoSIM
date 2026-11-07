#!/usr/bin/env python
"""Record phase-count vectors after a specified quiet interval in sampled data."""
import argparse
import bisect
import math
from pathlib import Path
import re

import pandas as pd
from tabulate import tabulate

from plot_common import protect_inputs


def generate_spec(frame, threshold=0.8, stability=20e-12, start=20e-12):
    if not math.isfinite(threshold) or not 0 <= threshold <= 1:
        raise ValueError("threshold must be finite and between 0 and 1")
    if not math.isfinite(stability) or stability < 0:
        raise ValueError("stability must be a finite nonnegative time in seconds")
    if not math.isfinite(start) or start < 0:
        raise ValueError("start must be a finite nonnegative time in seconds")
    if frame.empty or len(frame.columns) < 2 or frame.columns[0] != "time":
        raise ValueError("CSV must contain time and at least one P(junction) column")
    names = []
    for column in frame.columns[1:]:
        match = re.fullmatch(r"P\((.+)\)", column)
        if match is None:
            raise ValueError("specification input must contain only P(junction) traces")
        names.append(match.group(1))
    samples = frame.to_numpy(dtype=float)
    if not all(math.isfinite(value) for row in samples for value in row):
        raise ValueError("samples must be finite")
    times = samples[:, 0].tolist()
    if any(b <= a for a, b in zip(times, times[1:])):
        raise ValueError("sample times must be strictly increasing")
    first = bisect.bisect_left(times, start)
    if first == len(times):
        raise ValueError("no sample exists at or after the requested start")
    offset = samples[first, 1:]
    previous = (0,) * len(names)
    stored = previous
    changed_at = times[first]
    data = [[times[first], *stored]]
    for row in samples[first + 1:]:
        counts = tuple(cust_round((phase - baseline) / (2 * math.pi), threshold)
                       for phase, baseline in zip(row[1:], offset))
        if counts != previous:
            changed_at = row[0]
            previous = counts
        # Use elapsed time, including nonuniform grids, rather than row counts.
        allowance = 8 * math.ulp(max(abs(row[0]), abs(changed_at), stability))
        if counts != stored and row[0] - changed_at + allowance >= stability:
            data.append([row[0], *counts])
            stored = counts
    return pd.DataFrame(data, columns=["time", *names])


def main():
    parser = argparse.ArgumentParser(description="Generate settled phase-count specification rows")
    parser.add_argument("input", help="CSV with time and P(junction) columns")
    parser.add_argument("-o", "--output", help="SP output; default: input with .sp extension")
    parser.add_argument("-t", "--threshold", type=float, default=0.8,
                        help="fractional-cycle rounding threshold in [0,1]; default: 0.8")
    parser.add_argument("-s", "--stability", type=float, default=20e-12,
                        help="quiet time in seconds; default: 20e-12")
    parser.add_argument("--start", type=float, default=20e-12,
                        help="use first sample at/after this time as phase offset; default: 20e-12 s")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("-V", "--version", action="version", version="SP file generator 0.2.0")
    args = parser.parse_args()
    output = Path(args.output) if args.output else Path(args.input).with_suffix(".sp")
    try:
        protect_inputs(output, [args.input])
        spec = generate_spec(pd.read_csv(args.input), args.threshold, args.stability, args.start)
        content = tabulate(spec.values.tolist(), list(spec.columns), tablefmt="plain", floatfmt=".17g")
        output.write_text(content + "\n")
    except (OSError, ValueError, OverflowError) as exc:
        parser.error(str(exc))
    if args.verbose:
        print(spec)


# Custom threshold rounding function
def cust_round(number, threshold):
    # If the absolute value of the number minus the absolute value of the integer part is larger than the threshold
    if (abs(number) - abs(math.trunc(number)) > threshold):
        # If positive return the integer number + 1
        if number > 0:
            return math.trunc(number) + 1
        # If negative return the integer number - 1
        else:
            return math.trunc(number) - 1
    # If not then just return the integer number
    else:
        return math.trunc(number)

if __name__ == '__main__':
    main()
