#!/usr/bin/env python3
"""Run fresh transient points from a one-parameter @VALUE@ netlist template.

Values and averaging times are SI numbers. Includes and custom waveforms are
read relative to the template's folder. FILE/IV output directives are rejected,
including in nested includes, so all generated output stays in a new directory.
"""
import argparse
import bisect
import csv
import hashlib
import json
import math
import pathlib
import re
import subprocess
import sys

MARKER = "@VALUE@"


def file_hash(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect_inputs(path, active=(), inputs=None, inspected=None, data_root=None):
    """Inspect the same include contexts as JoSIM, retaining external-data hashes."""
    inputs = {} if inputs is None else inputs
    inspected = set() if inspected is None else inspected
    path = path.absolute()
    canonical = path.resolve(strict=True)
    data_root = path.parent if data_root is None else data_root
    if any(path.samefile(ancestor) for ancestor in active):
        raise ValueError(f"Cyclic include: {path}")
    if len(active) >= 256:
        raise ValueError("Include nesting exceeds the sweep helper's 256-file limit")
    # A symlinked file's nested includes are relative to its lexical parent,
    # not its target's parent. The same file can be read in either context.
    context = (canonical, path.parent.resolve())
    if context in inspected:
        return inputs
    inspected.add(context)
    inputs[str(canonical)] = file_hash(path)
    logical = []
    after_include = False
    # Match Input::read_input: split on LF and erase CR, including embedded CR.
    # Universal-newline decoding would interpret an included .F\rILE differently.
    for line in path.read_bytes().decode("utf-8").replace("\r", "").split("\n"):
        line = line.strip()
        if not line or line.startswith(("*", "#")):
            continue
        tokens = line.split()
        command = tokens[0].upper()
        command = command[1:] if command.startswith(".") else command
        if command == "END":
            break
        if command in ("FILE", "IV"):
            raise ValueError(f"{path}: remove {tokens[0]} for an isolated circuit sweep; model IV is separate")
        if command == "INCLUDE":
            if len(tokens) != 2 or MARKER in tokens[1]:
                raise ValueError(f"{path}: includes need one fixed filename without a sweep marker")
            inspect_inputs(path.parent / tokens[1], (*active, path), inputs, inspected, data_root)
            after_include = True
        elif line.startswith("+"):
            if after_include:
                raise ValueError(f"{path}: continuations after INCLUDE are unsupported in sweep templates")
            if not logical:
                raise ValueError(f"{path}: continuation has no preceding logical line")
            logical[-1] += " " + line[1:]
        else:
            logical.append(line)
            after_include = False
    for line in logical:
        for match in re.finditer(r"\bCUS\s*\(\s*([^\s,)]+)", line, re.IGNORECASE):
            name = match.group(1)
            if MARKER in name:
                raise ValueError("Sweep markers may not change custom waveform filenames")
            waveform = data_root / name
            if not waveform.exists():
                waveform = waveform.with_name(waveform.name.lower())
            waveform = waveform.resolve(strict=True)
            inputs[str(waveform)] = file_hash(waveform)
    return inputs


def time_average(times, values, start, stop):
    """Integral of the piecewise-linear trace divided by the explicit duration."""
    if len(times) < 2 or len(times) != len(values):
        raise ValueError("Need at least two matching time/trace samples")
    if not all(math.isfinite(v) for v in (*times, *values, start, stop)):
        raise ValueError("Time, trace and measurement window must be finite")
    if any(b <= a for a, b in zip(times, times[1:])):
        raise ValueError("Output timestamps must be strictly increasing")
    if not 0 <= start < stop or start < times[0] or stop > times[-1]:
        raise ValueError("Measurement window must lie within recorded samples and have positive duration")

    def at(t):
        right = bisect.bisect_left(times, t)
        if times[right] == t:
            return values[right]
        left = right - 1
        u = (t - times[left]) / (times[right] - times[left])
        return (1 - u) * values[left] + u * values[right]

    left, right = bisect.bisect_right(times, start), bisect.bisect_left(times, stop)
    grid, trace = [start, *times[left:right], stop], [at(start), *values[left:right], at(stop)]
    mean = math.fsum((.5 * a + .5 * b) * ((t1 - t0) / (stop - start))
                     for t0, t1, a, b in zip(grid, grid[1:], trace, trace[1:]))
    if not math.isfinite(mean):
        raise ValueError("Averaged trace is outside the supported finite range")
    return mean


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=pathlib.Path)
    parser.add_argument("--template", required=True, type=pathlib.Path)
    parser.add_argument("--values", required=True, help="comma-separated finite SI numbers; use --values=-1,0,1 for negatives")
    parser.add_argument("--trace", required=True, help="exact CSV column name, such as V(N)")
    parser.add_argument("--start", required=True, type=float, help="measurement start, seconds")
    parser.add_argument("--stop", required=True, type=float, help="measurement stop, seconds")
    parser.add_argument("--output", required=True, type=pathlib.Path, help="new output directory")
    parser.add_argument("--analysis", choices=("voltage", "phase"), default="voltage")
    parser.add_argument("--timeout", type=float, default=60, help="maximum seconds per simulator process")
    args = parser.parse_args()
    try:
        binary, template, output = args.binary.resolve(strict=True), args.template.resolve(strict=True), args.output.resolve()
        values = [float(v) for v in args.values.split(",")]
        if not binary.is_file() or not template.is_file():
            raise ValueError("Simulator and template must be existing files")
        if not values or not all(math.isfinite(v) for v in values):
            raise ValueError("Sweep values must be finite SI numbers")
        if not all(math.isfinite(v) for v in (args.start, args.stop, args.timeout)) or not 0 <= args.start < args.stop or args.timeout <= 0:
            raise ValueError("Require finite 0<=start<stop and timeout>0")
        if output.exists():
            raise ValueError("Output directory already exists; existing results are preserved")
        original = template.read_bytes().decode("utf-8").replace("\r", "")
        if MARKER not in original:
            raise ValueError(f"Template must contain the numeric marker {MARKER}")
        inputs = inspect_inputs(template)
    except (OSError, ValueError, UnicodeError) as error:
        parser.error(str(error))

    output.mkdir(parents=True)
    report = {"passed": False, "template": str(template), "input_sha256": inputs,
              "owner_sha256": file_hash(pathlib.Path(__file__)), "binary_sha256": file_hash(binary),
              "values": values, "trace": args.trace, "window_s": [args.start, args.stop], "analysis": args.analysis,
              "state_policy": "Fresh simulator process for every point; no state carried between points", "points": []}

    def save():
        (output / "report.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")

    save()
    try:
        for index, value in enumerate(values):
            if any(file_hash(pathlib.Path(path)) != digest for path, digest in inputs.items()):
                raise ValueError("A source netlist changed during the sweep; stopping without replacing it")
            point = output / f"point-{index:04d}"
            point.mkdir()
            deck = original.replace(MARKER, format(value, ".17g"))
            (point / "effective.cir").write_text(deck, encoding="utf-8")
            csv_path = point / "traces.csv"
            command = [str(binary), "-m", "1", "-V", "1", "-a", "1" if args.analysis == "voltage" else "0", "-o", str(csv_path)]
            try:
                result = subprocess.run(command, input=deck, cwd=template.parent, capture_output=True,
                                        text=True, encoding="utf-8", errors="replace", timeout=args.timeout)
            except subprocess.TimeoutExpired as error:
                def decoded(value):
                    return value.decode("utf-8", errors="replace") if isinstance(value, bytes) else value or ""
                (point / "simulator.log").write_text("Simulator timed out\n" + decoded(error.stdout) + decoded(error.stderr), encoding="utf-8")
                raise
            log = result.stdout + result.stderr
            (point / "simulator.log").write_text(log, encoding="utf-8")
            if result.returncode:
                raise ValueError(f"Point {index} failed with exit {result.returncode}; see {point / 'simulator.log'}")
            seed = re.search(r"RNG seed:\s*(\d+)", log)
            step = re.search(r"Effective timestep \(s\):\s*(\S+)", log)
            count = re.search(r"Computed samples:\s*(\d+)", log)
            if not seed or not step or not count:
                raise ValueError("Simulator must report its effective seed and grid; use the qualified JoSIM version")
            with csv_path.open(newline="", encoding="utf-8") as stream:
                reader = csv.reader(stream)
                header = next(reader)
                if len(set(header)) != len(header) or args.trace not in header or header[0] != "time":
                    raise ValueError(f"Requested trace {args.trace!r} is absent or CSV headers are ambiguous")
                column, rows = header.index(args.trace), list(reader)
            times, data = [float(row[0]) for row in rows], [float(row[column]) for row in rows]
            mean = time_average(times, data, args.start, args.stop)
            report["points"].append({"value": value, "mean": mean, "effective_seed": int(seed.group(1)),
                                     "effective_timestep_s": float(step.group(1)), "computed_samples": int(count.group(1)),
                                     "command": command, "cwd": str(template.parent), "input_mode": "stdin",
                                     "output_sha256": file_hash(csv_path), "samples": len(rows)})
            save()
        if any(file_hash(pathlib.Path(path)) != digest for path, digest in inputs.items()):
            raise ValueError("A source netlist changed during the sweep")
        if file_hash(binary) != report["binary_sha256"]:
            raise ValueError("Simulator binary changed during the sweep")
        with (output / "sweep.csv").open("w", newline="", encoding="utf-8") as stream:
            writer = csv.writer(stream)
            keys = ("value", "mean", "effective_seed", "effective_timestep_s", "computed_samples")
            writer.writerow(keys)
            for point in report["points"]:
                writer.writerow(point[key] for key in keys)
        report["passed"] = True
        save()
    except (OSError, ValueError, IndexError, StopIteration, subprocess.TimeoutExpired) as error:
        report["error"] = str(error)
        save()
        print(f"Sweep failed: {error}", file=sys.stderr)
        return 1
    print(f"Completed {len(values)} fresh-state points: {output / 'sweep.csv'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
