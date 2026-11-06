#!/usr/bin/env python3
"""Qualify the fixed #101 buffer against an independent Kirchhoff/RCSJ ODE.

Install tools/requirements-qualification.txt in a dedicated environment.
This is a reference owner for two named circuits, not a general netlist solver.
"""

import argparse
import csv
import hashlib
import importlib.metadata
import json
import math
import os
import pathlib
import platform
import subprocess
import tempfile
import time

import numpy as np
from scipy.integrate import solve_ivp


ROOT = pathlib.Path(__file__).resolve().parents[1]
REFERENCES = ROOT / "test/reference"
SIGMA = 6.62607015e-34 / (4 * math.pi * 1.602176634e-19)
PHI0 = 6.62607015e-34 / (2 * 1.602176634e-19)
# Scaled units: ps, mA, mV, pH, pF; resistances are in ohms.
L, C, R0, RPORT, IG, IC1, IC2 = .125, .007, .16, 2., .02, 2., 1.5
BREAKS = np.array([0., 300., 302.5, 305., 600., 602.5, 605., 800.])
DRIVE = np.array([0., 0., .82713, 0., 0., .82713, 0., 0.])
COLUMNS = ["time_ps", "vout_V", "u1_V", "u2_V", "phi1_rad", "phi2_rad", "i1_A", "i2_A"]


def sha256(path, text=True):
    data = pathlib.Path(path).read_bytes()
    if text:
        data = data.replace(b"\r\n", b"\n")
    return hashlib.sha256(data).hexdigest()


ACCEPTANCE = {"base_step_ps": .02, "voltage_base_V": .82713e-6, "current_base_A": 2e-6,
              "phase_base_rad": 2 * math.pi * 1e-4, "voltage_rounding_V": 1e-9,
              "current_rounding_A": 1e-11, "phase_rounding_rad": 6e-6,
              "pulse_area_Phi0": 5e-6}


def error_limits(h):
    scale = (h / ACCEPTANCE["base_step_ps"]) ** 2
    voltage = ACCEPTANCE["voltage_base_V"] * scale + ACCEPTANCE["voltage_rounding_V"]
    current = ACCEPTANCE["current_base_A"] * scale + ACCEPTANCE["current_rounding_A"]
    phase = ACCEPTANCE["phase_base_rad"] * scale + ACCEPTANCE["phase_rounding_rad"]
    return np.array([voltage] * 3 + [phase] * 2 + [current] * 2)


def independent_solution(direction, method, rtol):
    """State: i1(N1->N2), i2(N3->N4), u1=V2, u2=V2-V3, phi1, phi2."""
    kappa = 1e-15 / SIGMA

    def rhs(t, y):
        i1, i2, u1, u2, p1, p2 = y
        vin = np.interp(t, BREAKS, DRIVE)
        v1, v4 = (-RPORT * i1, vin) if direction == "reverse" else (vin, RPORT * i2)
        return [(v1 - u1) / L, (u1 - u2 - v4) / L,
                (i1 - i2 - IG - IC1 * np.sin(p1) - u1 / R0) / C,
                (i2 + IG - IC2 * np.sin(p2) - u2 / R0) / C, kappa * u1, kappa * u2]

    def jac(t, y):
        return np.array([
            [-RPORT / L if direction == "reverse" else 0, 0, -1 / L, 0, 0, 0],
            [0, -RPORT / L if direction == "forward" else 0, 1 / L, -1 / L, 0, 0],
            [1 / C, -1 / C, -1 / (R0 * C), 0, -IC1 * np.cos(y[4]) / C, 0],
            [0, 1 / C, 0, -1 / (R0 * C), 0, -IC2 * np.cos(y[5]) / C],
            [0, 0, kappa, 0, 0, 0], [0, 0, 0, kappa, 0, 0]])

    y = np.zeros(6)
    segments, statistics = [], []
    started = time.monotonic()
    for left, right in zip([-10., *BREAKS[:-1]], BREAKS):
        solved = solve_ivp(rhs, (left, right), y, method=method, rtol=rtol, atol=np.full(6, .01 * rtol),
                           jac=jac, max_step=.2, dense_output=True)
        if not solved.success:
            raise RuntimeError(solved.message)
        segments.append((left, right, solved.sol))
        statistics.append({"nfev": solved.nfev, "njev": solved.njev, "nlu": solved.nlu})
        y = solved.y[:, -1]

    def evaluate(times):
        times = np.asarray(times)
        values = np.full((6, len(times)), np.nan)
        for left, right, dense in segments:
            selected = (times >= left) & (times <= right)
            if np.any(selected):
                values[:, selected] = dense(times[selected])
        if not np.all(np.isfinite(values)):
            raise RuntimeError("Non-finite or out-of-domain independent reference samples")
        return values

    return evaluate, {"method": method, "rtol": rtol, "atol": .01 * rtol,
                      "max_step_ps": .2, "start_ps": -10, "seconds": time.monotonic() - started,
                      "segments": statistics}


def output_voltage(direction, state):
    return -RPORT * state[0] if direction == "reverse" else RPORT * state[1]


def table(direction, times, evaluate):
    y = evaluate(times)
    return np.column_stack([times, output_voltage(direction, y) * 1e-3, y[2:4].T * 1e-3,
                            y[4:6].T, y[0:2].T * 1e-3])


def simulate(binary, direction, h, mode, output):
    text = (REFERENCES / ("buffer-101-" + direction + ".cir")).read_text()
    if text.count(".tran 0.01p 800p") != 1:
        raise RuntimeError("The fixed buffer input's timestep owner changed")
    text = text.replace(".tran 0.01p 800p", ".tran " + str(h) + "p 800p")
    with tempfile.TemporaryDirectory(prefix="buffer-", dir=output) as tmp:
        work = pathlib.Path(tmp)
        (work / "input.cir").write_text(text)
        command = [str(binary), "-m", "1", "-a", str(mode), "-o", "output.csv", "input.cir"]
        result = subprocess.run(command, cwd=work, capture_output=True, text=True, timeout=30)
        (output / (direction + "-" + str(h) + "-" + str(mode) + ".log")).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError("JoSIM failed: " + result.stdout + result.stderr)
        with (work / "output.csv").open() as stream:
            rows = list(csv.reader(stream))
        values = np.asarray(rows[1:], dtype=float)
        if values.shape[1] != 12 or not np.all(np.isfinite(values)) or not np.all(np.diff(values[:, 0]) > 0):
            raise RuntimeError("Invalid/non-finite JoSIM buffer output")
        return values


def pulse_metrics(direction, evaluate):
    records = []
    for left, right in [(290., 350.), (590., 650.)]:
        t = np.linspace(left, right, 60001)
        y = evaluate(t)
        v = output_voltage(direction, y)
        hi, lo = np.argmax(v), np.argmin(v)
        changes = y[:, -1] - y[:, 0]
        flux = (SIGMA * changes[4] + L * 1e-15 * changes[0] if direction == "reverse" else
                SIGMA * (changes[4] - changes[5]) - L * 1e-15 * changes[1])
        numerical_flux = np.trapezoid(v, t) * 1e-15
        if abs(numerical_flux - flux) > 1e-9 * PHI0:
            raise RuntimeError("Independent pulse voltage integral and flux linkage disagree")
        records.append({"window_ps": [left, right], "maximum_uV": float(v[hi] * 1000),
                        "maximum_time_ps": float(t[hi]), "minimum_uV": float(v[lo] * 1000),
                        "minimum_time_ps": float(t[lo]), "net_flux_Phi0": float(flux / PHI0),
                        "voltage_integral_Phi0": float(numerical_flux / PHI0),
                        "phase_changes_rad": changes[4:6].tolist()})
    return records


def candidate_metrics(observed, reference_pulses):
    times = observed[:, 0] * 1e12
    prescribed = np.interp(times, BREAKS, DRIVE) * 1e-3
    input_error = np.abs(observed[:, 1] - prescribed)
    if np.any(input_error > 3e-6 * np.abs(prescribed) + 1e-12):
        raise RuntimeError("Prescribed buffer input waveform changed")
    kcl = np.maximum(np.abs(observed[:, 7] - observed[:, 9] - observed[:, 10]),
                     np.abs(observed[:, 10] - observed[:, 8] - observed[:, 11]))
    if np.any(kcl > 3e-6 * np.max(np.abs(observed[:, 7:]), axis=1) + 1e-12):
        raise RuntimeError("Buffer candidate violates KCL")
    if np.max(np.abs(observed[:, 3:5])) * 1000 >= 2.75:
        raise RuntimeError("Buffer candidate left the original subgap model region")
    pulses = []
    for reference in reference_pulses:
        left, right = reference["window_ps"]
        selected = (times >= left) & (times <= right)
        area = np.trapezoid(observed[selected, 2], observed[selected, 0]) / PHI0
        if abs(area - reference["net_flux_Phi0"]) > ACCEPTANCE["pulse_area_Phi0"]:
            raise RuntimeError("Buffer candidate pulse area changed")
        first, last = np.argmin(np.abs(times - left)), np.argmin(np.abs(times - right))
        changes = observed[last, 5:7] - observed[first, 5:7]
        if np.max(np.abs(changes - reference["phase_changes_rad"])) > 2e-5:
            raise RuntimeError("Buffer candidate phase-slip count changed")
        pulses.append({"window_ps": [left, right], "net_flux_Phi0": float(area),
                       "phase_changes_rad": changes.tolist()})
    return {"maximum_KCL_residual_A": float(np.max(kcl)),
            "maximum_input_error_V": float(np.max(input_error)), "pulses": pulses}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--binary", type=pathlib.Path)
    parser.add_argument("--write-references", action="store_true")
    parser.add_argument("--plot", action="store_true")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    binary = args.binary.resolve() if args.binary else None
    points = np.unique(np.round(np.r_[np.arange(298, 315.0001, .1), np.arange(598, 615.0001, .1),
                                      [50, 100, 200, 290, 350, 400, 590, 650, 700, 799]], 9))
    validation = np.unique(np.r_[np.arange(290, 325.0001, .001), np.arange(590, 625.0001, .001), points])
    report = {"reference_owner_sha256": sha256(__file__), "python": platform.python_version(),
              "packages": {name: importlib.metadata.version(name) for name in ("numpy", "scipy")},
              "SI_sigma_V_s": SIGMA, "input_pulse_flux_Wb": .82713 * 2.5 * 1e-15,
              "directions": {}, "fixtures": {}, "acceptance": ACCEPTANCE,
              "text_hash_policy": "SHA-256 after CRLF-to-LF normalization",
              "scope": "Independent reference and optional JoSIM candidate qualification",
              "binary_qualified": False, "status": "in_progress"}
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    if binary:
        report["binary_sha256"] = sha256(binary, text=False)
        version = subprocess.run([str(binary), "-h"], capture_output=True, text=True, timeout=15)
        report["binary_identity_output"] = version.stdout.splitlines()[:5]
    examples = {}
    for direction in ("reverse", "forward"):
        coarse, coarse_meta = independent_solution(direction, "Radau", 1e-10)
        reference, meta = independent_solution(direction, "Radau", 1e-12)
        other, other_meta = independent_solution(direction, "BDF", 1e-11)
        y = reference(validation)
        tighter = np.max(np.abs(y - coarse(validation)), axis=1)
        independent = np.max(np.abs(y - other(validation)), axis=1)
        if max(np.max(tighter), np.max(independent)) > 1e-8:
            raise RuntimeError("Independent reference tolerance/method comparison failed")
        peak_junction = np.max(np.abs(y[2:4]), axis=1)
        if np.max(peak_junction) >= 2.75:
            raise RuntimeError("Subgap reference left the original model's subgap region")
        row = {"reference": meta, "coarser_reference": coarse_meta, "other_integrator": other_meta,
               "tolerance_comparison_scaled": tighter.tolist(), "integrator_comparison_scaled": independent.tolist(),
               "maximum_abs_junction_mV": peak_junction.tolist(), "pulses": pulse_metrics(direction, reference), "josim": []}
        data = table(direction, points, reference)
        path = REFERENCES / ("buffer-101-" + direction + ".csv")
        if args.write_references:
            with path.open("w", newline="") as stream:
                writer = csv.writer(stream, lineterminator="\n")
                writer.writerow(COLUMNS)
                writer.writerows([[format(value, ".17g") for value in sample] for sample in data])
        report["fixtures"][path.name] = {"sha256": sha256(path), "samples": len(points), "columns": COLUMNS}
        circuit = REFERENCES / ("buffer-101-" + direction + ".cir")
        report["fixtures"][circuit.name] = {"sha256": sha256(circuit)}
        if binary:
            for h in (.02, .01, .005, .0025):
                for mode in (0, 1):
                    observed = simulate(binary, direction, h, mode, output)
                    t = observed[:, 0] * 1e12
                    selected = t >= 50
                    truth = table(direction, t, reference)
                    measured = np.column_stack([t, observed[:, 2:9]])
                    error = np.max(np.abs(measured[selected, 1:] - truth[selected, 1:]), axis=0)
                    limits = error_limits(h)
                    if np.any(error > limits):
                        raise RuntimeError("Buffer reference error budget exceeded: " + str((direction, h, mode, error.tolist())))
                    row["josim"].append({"mode": mode, "h_ps": h, "maximum_errors_SI": error.tolist(),
                                         "limits_SI": limits.tolist(),
                                         "conservation": candidate_metrics(observed, row["pulses"])})
                    if h == .01 and mode == 1:
                        examples[direction] = (reference, observed)
        report["directions"][direction] = row
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    if args.write_references:
        manifest = {key: report[key] for key in ("reference_owner_sha256", "python", "packages", "SI_sigma_V_s",
                                                "text_hash_policy", "acceptance", "fixtures")}
        manifest["pulses"] = {direction: row["pulses"] for direction, row in report["directions"].items()}
        (REFERENCES / "buffer-101-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")

    if args.plot:
        if not binary:
            raise RuntimeError("--plot requires the candidate --binary")
        with tempfile.TemporaryDirectory(prefix="matplotlib-", dir=output) as cache:
            previous_cache = os.environ.get("MPLCONFIGDIR")
            os.environ["MPLCONFIGDIR"] = cache
            try:
                import matplotlib
                matplotlib.use("Agg")
                import matplotlib.pyplot as plt
                fig, axes = plt.subplots(2, 2, figsize=(11, 7), constrained_layout=True)
                t = np.linspace(290, 320, 15001)
                for row, direction in enumerate(("reverse", "forward")):
                    reference, observed = examples[direction]
                    state = reference(t)
                    ax = axes[row, 0]
                    ax.plot(t, np.interp(t, BREAKS, DRIVE) * 1000, color="0.65", label="Input")
                    ax.plot(t, output_voltage(direction, state) * 1000, color="black", label="Independent ODE")
                    selected = (observed[:, 0] >= 290e-12) & (observed[:, 0] <= 320e-12)
                    shown = observed[selected][::20]
                    ax.plot(shown[:, 0] * 1e12, shown[:, 2] * 1e6, "o", fillstyle="none", markersize=3,
                            color="#b33a3a", label="JoSIM, 10 fs")
                    ax.set(title=direction.capitalize() + " drive", xlabel="Time (ps)", ylabel="Voltage (µV)")
                    ax.legend(fontsize=8)
                    ax = axes[row, 1]
                    for i, label, color in [(4, "B01", "#2468a0"), (5, "B02", "#a12791")]:
                        ax.plot(t, state[i] - state[i, 0], label=label, color=color)
                    ax.set(xlabel="Time (ps)", ylabel="Phase change (rad)")
                    ax.legend()
                fig.savefig(output / "buffer-101.png", dpi=180)
                fig.savefig(output / "buffer-101.pdf")
                plt.close(fig)
            finally:
                if previous_cache is None:
                    os.environ.pop("MPLCONFIGDIR", None)
                else:
                    os.environ["MPLCONFIGDIR"] = previous_cache

    report["reference_validated"] = True
    report["binary_qualified"] = binary is not None
    report["status"] = "passed"
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
