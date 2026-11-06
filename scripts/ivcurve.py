#!/usr/bin/env python3
"""Retired positional prototype; retained as a safe explanatory entry point."""
import sys


def main():
    print("ivcurve.py is retired: it overwrote temporary netlists and ignored simulator failures.\n"
          "For a junction MODEL curve, use JoSIM's .iv command.\n"
          "For fresh circuit points, see docs/sweeps.md and tools/circuit_sweep.py --help.\n"
          "A single-transient bias ramp has different state/history semantics.\n"
          "No files were changed.", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
