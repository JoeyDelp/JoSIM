#!/usr/bin/env python3
"""Qualify one weak-noise, zero-loop-inductance DC-SQUID example.

Requires tools/requirements-qualification.txt. This bounded reference compares
low-frequency voltage noise with the independently derived overdamped RSJ
phase-response formula; it is not a general SQUID or quantum-noise solver.
"""

import argparse
import hashlib
import importlib.metadata
import json
import math
import os
import pathlib
import platform
import re
import subprocess
import tempfile
import time

import numpy as np
from scipy.signal import welch
from scipy.stats import t as student_t


ROOT = pathlib.Path(__file__).resolve().parents[1]
PHI0 = 6.62607015e-34 / (2 * 1.602176634e-19)
SIGMA = PHI0 / (2 * math.pi)
KB, TEMP, IC, IB, R, C = 1.380649e-23, 4.2, math.sqrt(2) * 1e-3, 1.6e-3, .5e-3, 20e-12
D = IB * IB - IC * IC
VMEAN, FJ = R * math.sqrt(D), R * math.sqrt(D) / PHI0
BANDS = (20e6, 200e6, 2e9)
SEEDS = tuple(101 * i for i in range(1, 17))
FREQUENCIES = np.array([4, 8, 12, 16, 20]) * 1e6
ACCEPTANCE = {"family_confidence": .99, "primary_bands": 3,
              "RSJ_systematic_allowance": .05, "paired_refinement_relative": .01,
              "deterministic_period_relative": 1e-5, "deterministic_mean_absolute_V": 1e-9}


def sha256(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def reference_psd(bandwidth):
    interval = 1 / (3 * bandwidth)
    current_psd = 4 * KB * TEMP / R  # Two independent 1 mOhm shunts in parallel.
    def s(f):
        return current_psd * np.sinc(f * interval) ** 4
    return R * R / D * (IB * IB * s(FREQUENCIES)
                       + IC * IC / 4 * (s(FJ + FREQUENCIES) + s(FJ - FREQUENCIES)))


def render(report, output):
    os.environ.setdefault("MPLCONFIGDIR", str(ROOT / "build/audit-resolution/mpl-cache"))
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    figure, axes = plt.subplots(1, 2, figsize=(10, 4), constrained_layout=True)
    for color, band in zip(("tab:blue", "tab:orange", "tab:green"), report["bands"]):
        label = f"NEB={band['NEB_Hz']/1e6:g} MHz"
        axes[0].plot(FREQUENCIES / 1e6, np.array(band["mean_PSD_V2_Hz"]) / 1e-24, "o", color=color, label=label)
        axes[0].plot(FREQUENCIES / 1e6, np.array(band["reference_PSD_V2_Hz"]) / 1e-24, "-", color=color)
        axes[1].errorbar(band["NEB_over_fJ"], band["mean_PSD_ratio"],
                         yerr=band["family_critical_t"] * band["standard_error"], fmt="o", color=color)
    axes[0].set(xlabel="Measurement frequency (MHz)", ylabel="Voltage PSD (pV²/Hz)", title="16-seed means and independent RSJ reference")
    axes[0].legend(fontsize=8)
    axes[1].set(xscale="log", xlabel="NEB / Josephson frequency", ylabel="Measured / reference PSD", title="Family 99% statistical intervals")
    axes[1].axhline(1, color="black", linewidth=.8)
    axes[1].axhspan(.95, 1.05, alpha=.15, color="gray")
    figure.savefig(output / "squid-noise.png", dpi=180)
    figure.savefig(output / "squid-noise.pdf")
    plt.close(figure)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    binary, output = args.binary.resolve(), args.output.resolve()
    if not binary.is_file():
        parser.error("--binary must identify an existing JoSIM executable")
    if output.exists():
        parser.error("--output must be a new directory; existing results are preserved")
    output.mkdir(parents=True, exist_ok=False)
    (output / "runs").mkdir()
    (output / "qualification-owner.py").write_bytes(pathlib.Path(__file__).read_bytes())
    template = ROOT / "test/comp/squid_noise.cir"
    report = {"owner_sha256": sha256(__file__), "fixture_sha256": sha256(template),
              "binary": str(binary), "binary_sha256": sha256(binary),
              "python": platform.python_version(), "platform": platform.platform(),
              "packages": {name: importlib.metadata.version(name) for name in ("numpy", "scipy", "matplotlib")},
              "source_sha256": {str(p.relative_to(ROOT)): sha256(p)
                                for pattern in ("src/*.cpp", "include/JoSIM/*.hpp") for p in sorted(ROOT.glob(pattern))},
              "SI_reference": {"Phi0_Wb": PHI0, "sigma_V_s": SIGMA, "kB_J_K": KB,
                               "effective_Ic_A": IC, "bias_A": IB, "effective_R_ohm": R,
                               "effective_C_F": C, "mean_voltage_V": VMEAN, "fJ_Hz": FJ,
                               "fP_Hz": math.sqrt(IC / (SIGMA * C)) / (2 * math.pi),
                               "beta_c": IC * R * R * C / SIGMA, "Gamma": KB * TEMP / (IC * SIGMA),
                               "classical_frequency_scale_Hz": KB * TEMP / 6.62607015e-34},
              "acceptance": ACCEPTANCE, "seeds": SEEDS, "frequencies_Hz": FREQUENCIES.tolist(),
              "scope": "Classical weak-noise overdamped zero-L SQUID limit; finite-L reference has a separate owner",
              "runs": [], "bands": [], "failures": [], "passed": False}

    def checkpoint():
        (output / "progress.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")

    with tempfile.TemporaryDirectory(prefix="squid-noise-", dir=output) as temporary:
        work = pathlib.Path(temporary)
        def run(label, bandwidth=200e6, seed=101, h=2e-12, print_step=20e-12, mode=1, temperature=TEMP):
            def fail(message):
                if (work / "output.csv").exists():
                    (work / "output.csv").rename(output / "runs" / (label + "-failed.csv"))
                raise RuntimeError(message)

            deck = template.read_text()
            for name, value in (("TNOISE", temperature), ("BW", bandwidth), ("TINT", h), ("OUTPUTSTEP", print_step)):
                deck, count = re.subn(rf"(?m)^\.param {name}=.*$", f".param {name}={value:.17g}", deck)
                if count != 1:
                    raise RuntimeError(f"Fixture parameter {name} is missing or duplicated")
            deck = deck.replace(".option seed=101", f".option seed={seed}")
            (work / "input.cir").write_text(deck)
            (output / "runs" / (label + ".cir")).write_text(deck)
            command = [str(binary), "-m", "1", "-V", "1", "-a", str(mode), "-o", "output.csv", "input.cir"]
            started = time.monotonic()
            process = subprocess.run(command, cwd=work, capture_output=True, text=True, timeout=30)
            log = process.stdout + process.stderr
            (output / "runs" / (label + ".log")).write_text(log)
            if process.returncode:
                fail(f"{label} exited {process.returncode}; see its preserved deck/log")
            effective = re.search(r"Effective timestep \(s\):\s*(\S+)", log)
            computed = re.search(r"Computed samples:\s*(\d+)", log)
            effective_seed = re.search(r"RNG seed:\s*(\d+)", log)
            if not effective_seed or int(effective_seed.group(1)) != seed:
                fail(f"{label}: reported RNG seed differs from the experiment")
            if not effective or not computed or float(effective.group(1)) != h:
                fail(f"{label}: effective timestep missing or differs from requested timestep")
            data = np.loadtxt(work / "output.csv", delimiter=",", skiprows=1)
            if data.shape != (round(5e-6 / print_step), 2) or not np.all(np.isfinite(data)):
                fail(f"{label}: unexpected or non-finite output grid {data.shape}")
            if np.max(np.abs(data[:, 0] - (data[0, 0] + np.arange(len(data)) * print_step))) > 1.1e-12:
                fail(f"{label}: nonuniform output timestamps beyond CSV rounding")
            f, psd = welch(data[:, 1], fs=1 / print_step, window="hann", nperseg=round(.5e-6 / print_step),
                           noverlap=round(.25e-6 / print_step), detrend="constant")
            indices = np.rint(FREQUENCIES / (f[1] - f[0])).astype(int)
            if not np.allclose(f[indices], FREQUENCIES, rtol=1e-12):
                raise RuntimeError("Welch bin alignment changed")
            entry = {"label": label, "bandwidth_Hz": bandwidth, "seed": seed,
                     "effective_seed": int(effective_seed.group(1)), "temperature_K": temperature,
                     "requested_h_s": h, "effective_h_s": float(effective.group(1)),
                     "computed_samples": int(computed.group(1)), "print_step_s": print_step,
                     "mode": mode, "command": command, "seconds": time.monotonic() - started,
                     "output_sha256": sha256(work / "output.csv"), "output_samples": len(data),
                     "mean_voltage_V": float(data[:, 1].mean()), "PSD_V2_Hz": psd[indices].tolist()}
            report["runs"].append(entry)
            checkpoint()
            (work / "output.csv").unlink()
            print(label, "mean_V", entry["mean_voltage_V"], flush=True)
            return entry, data

        # Noiseless period/mean checks bound the independent deterministic reduction.
        for mode in (0, 1):
            for h in (2e-12, 1e-12):
                entry, data = run(f"deterministic-m{mode}-h{h:g}", h=h, mode=mode, temperature=0)
                crossing = np.flatnonzero((data[:-1, 1] < VMEAN) & (data[1:, 1] >= VMEAN))
                times = data[crossing, 0] + (VMEAN - data[crossing, 1]) / (data[crossing + 1, 1] - data[crossing, 1]) * 20e-12
                period = float(np.diff(times).mean())
                entry["period_s"] = period
                if len(times) < 500 or abs(period * FJ - 1) > ACCEPTANCE["deterministic_period_relative"]:
                    report["failures"].append(entry["label"] + ": deterministic period")
                if abs(entry["mean_voltage_V"] - VMEAN) > ACCEPTANCE["deterministic_mean_absolute_V"]:
                    report["failures"].append(entry["label"] + ": deterministic mean")

        for bandwidth in BANDS:
            expected = reference_psd(bandwidth)
            entries = [run(f"B{bandwidth:g}-seed{seed}", bandwidth, seed)[0] for seed in SEEDS]
            spectra = np.array([v["PSD_V2_Hz"] for v in entries])
            ratios = np.mean(spectra / expected, axis=1)
            mean, se = float(ratios.mean()), float(ratios.std(ddof=1) / math.sqrt(len(SEEDS)))
            critical = float(student_t.ppf(1 - .01 / (2 * len(BANDS)), len(SEEDS) - 1))
            allowance = critical * se + ACCEPTANCE["RSJ_systematic_allowance"]
            band = {"NEB_Hz": bandwidth, "NEB_over_fJ": bandwidth / FJ,
                    "reference_PSD_V2_Hz": expected.tolist(), "mean_PSD_V2_Hz": spectra.mean(axis=0).tolist(),
                    "per_seed_mean_PSD_ratio": ratios.tolist(), "mean_PSD_ratio": mean,
                    "standard_error": se, "family_critical_t": critical,
                    "acceptance_half_width": allowance, "passed": abs(mean - 1) <= allowance}
            report["bands"].append(band)
            if not band["passed"]:
                report["failures"].append(f"NEB={bandwidth:g}: independent weak-noise PSD reference")
            # Paired refinement uses the same seed and physical noise interval.
            original = np.array(entries[0]["PSD_V2_Hz"])
            for label, settings in (("half-h", {"h": 1e-12}), ("half-output-step", {"print_step": 10e-12}),
                                    ("phase-mode", {"mode": 0})):
                refined, _ = run(f"B{bandwidth:g}-{label}", bandwidth, **settings)
                difference = float(np.max(np.abs(np.array(refined["PSD_V2_Hz"]) / original - 1)))
                refined["maximum_paired_PSD_relative_difference"] = difference
                if difference > ACCEPTANCE["paired_refinement_relative"]:
                    report["failures"].append(refined["label"] + ": paired PSD refinement")
            checkpoint()

    try:
        import resource
        peak = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss
        report["maximum_child_RSS_bytes"] = peak if platform.system() == "Darwin" else peak * 1024
    except ImportError:
        report["maximum_child_RSS_bytes"] = None
    if sha256(binary) != report["binary_sha256"] or any(sha256(ROOT / p) != value for p, value in report["source_sha256"].items()):
        report["failures"].append("Source or binary changed during the campaign")
    render(report, output)
    report["passed"] = not report["failures"]
    (output / "report.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    (output / "progress.json").unlink()
    print(json.dumps({"passed": report["passed"], "failures": report["failures"], "bands": report["bands"]}, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
