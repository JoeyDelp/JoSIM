#!/usr/bin/env python3
import argparse
import os
import re
import subprocess
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

PHI0 = 2.067833848e-15  # flux quantum [V*s]


def write_netlist(template_text: str, out_path: Path, seed: int, phi_dc: float) -> None:
    txt = template_text.replace("__SEED__", str(seed))
    txt = txt.replace("__PHI_DC__", f"{phi_dc:.16g}")
    out_path.write_text(txt)


def run_josim(josim_bin: str, netlist_path: Path, cwd: Path, out_csv: Path | None) -> None:
    josim = os.path.abspath(josim_bin)

    cmd = [josim]
    if out_csv is not None:
        cmd += ["-o", str(out_csv.name)]
    cmd += [str(netlist_path.name)]

    cp = subprocess.run(
        cmd,
        cwd=str(cwd),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if cp.returncode != 0:
        raise RuntimeError(f"JoSIM failed in {cwd}:\n{cp.stdout}")


def find_time_col(df: pd.DataFrame) -> str:
    for c in df.columns:
        if c.strip().lower() == "time":
            return c
    return df.columns[0]


def find_phase_col(df: pd.DataFrame, elem: str) -> str:
    # Matches: P(B1), p(b1), "P(B1) [rad]" etc.
    pat_exact = re.compile(r"^p\s*\(\s*" + re.escape(elem) + r"\s*\)\s*$", re.IGNORECASE)
    pat_prefix = re.compile(r"^p\s*\(\s*" + re.escape(elem) + r"\s*\)", re.IGNORECASE)

    for c in df.columns:
        s = c.strip()
        if pat_exact.match(s) or pat_prefix.match(s):
            return c

    raise ValueError(f"Phase column for {elem} not found. Columns: {list(df.columns)}")


def moving_mean(x: np.ndarray, n: int) -> np.ndarray:
    if n <= 1:
        return x
    k = np.ones(n, dtype=float) / float(n)
    return np.convolve(x, k, mode="same")


def first_sustained_true(mask: np.ndarray, sustain_samples: int) -> int | None:
    if sustain_samples <= 1:
        idx = np.where(mask)[0]
        return int(idx[0]) if idx.size else None
    run = np.convolve(mask.astype(int), np.ones(sustain_samples, dtype=int), mode="same")
    idx = np.where(run >= sustain_samples)[0]
    return int(idx[0]) if idx.size else None


def switching_current_from_phase(
    csv_path: Path,
    i_max: float,
    t_ramp: float,
    v_threshold: float | None,
    omega_threshold: float | None,
    ignore_t: float = 0.0,
    sustain_samples: int = 20,
    smooth_n: int = 15,
    require_slip: bool = True,
    elems: tuple[str, str] = ("B1", "B2"),
) -> tuple[float, float, str] | None:
    """
    Returns (i_sw, t_sw, elem_triggered) or None if no switch detected.
    """

    df = pd.read_csv(csv_path)

    tcol = find_time_col(df)
    t_all = df[tcol].to_numpy(dtype=float)

    # mask out initial transient
    m = t_all >= ignore_t
    t = t_all[m]
    if t.size < 5:
        return None

    # Determine omega threshold
    if omega_threshold is None:
        if v_threshold is None:
            raise ValueError("Provide either omega_threshold or v_threshold.")
        # omega_th = (2π/Φ0) * Vth
        omega_threshold = (2.0 * np.pi / PHI0) * float(v_threshold)

    # For each junction, compute omega and detect first sustained running
    best = None  # (idx0, elem, omega_series, t_series, phi_unwrapped)
    slip_ok_all = True

    for elem in elems:
        pcol = find_phase_col(df, elem)
        phi = df[pcol].to_numpy(dtype=float)[m]

        # unwrap to remove 2π jumps
        phi_u = np.unwrap(phi)

        if require_slip:
            # must advance by at least 2π over the record
            if (phi_u[-1] - phi_u[0]) < (2.0 * np.pi):
                slip_ok_all = False

        omega = np.gradient(phi_u, t)
        omega_s = moving_mean(omega, smooth_n)

        run_mask = omega_s >= omega_threshold
        idx0 = first_sustained_true(run_mask, sustain_samples)
        if idx0 is None:
            continue

        if best is None or idx0 < best[0]:
            best = (idx0, elem)

    if best is None:
        return None

    if require_slip and not slip_ok_all:
        # If neither JJ actually accumulated a 2π slip, don't call it switched.
        # (This is conservative; you can relax later.)
        return None

    idx0, elem = best
    t_sw = float(t[idx0])

    if t_sw <= t_ramp:
        i_sw = float(i_max * (t_sw / t_ramp))
    else:
        i_sw = float(i_max)

    return i_sw, t_sw, elem


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--josim", required=True, help="Path to josim-cli")
    ap.add_argument("--template", default="dc_squid_switch_template.cir")
    ap.add_argument("--trials", type=int, default=300)
    ap.add_argument("--seed0", type=int, default=1)
    ap.add_argument("--phis", default="0.0,0.5", help="Comma-separated PHI_DC values (Phi0 units)")
    ap.add_argument("--out", default="runs_squid_hist")
    ap.add_argument("--i-max", type=float, default=55e-6)
    ap.add_argument("--t-ramp", type=float, default=20e-9)

    # Kept for backwards-compat; now used only to derive omega_th if --omega-th not set
    ap.add_argument("--vth", type=float, default=1e-4, help="Voltage-equivalent threshold [V] (converted to dphi/dt)")

    ap.add_argument("--omega-th", type=float, default=None, help="Direct omega threshold [rad/s] (overrides --vth)")
    ap.add_argument("--ignore-t", type=float, default=0.0)
    ap.add_argument("--sustain", type=int, default=20)
    ap.add_argument("--smooth", type=int, default=15)
    ap.add_argument("--no-slip-check", action="store_true", help="Do not require net 2π phase advance")
    ap.add_argument("--use-o", action="store_true", help="Pass -o out.csv to josim-cli (if supported)")
    args = ap.parse_args()

    template_text = Path(args.template).read_text()
    out_root = Path(args.out)
    out_root.mkdir(parents=True, exist_ok=True)

    phi_list = [float(x.strip()) for x in args.phis.split(",") if x.strip()]
    results: dict[float, np.ndarray] = {}
    rows = []

    for phi_dc in phi_list:
        i_sws = []
        for k in range(args.trials):
            seed = args.seed0 + k
            run_dir = out_root / f"phi_{phi_dc:.3f}" / f"run_{k:04d}"
            run_dir.mkdir(parents=True, exist_ok=True)

            net = run_dir / "dc_squid_switch.cir"
            write_netlist(template_text, net, seed=seed, phi_dc=phi_dc)

            out_csv = run_dir / "out.csv"
            run_josim(args.josim, net, cwd=run_dir, out_csv=out_csv if args.use_o else None)

            if not out_csv.exists():
                raise FileNotFoundError(f"Expected {out_csv} not found (check .file or --use-o).")

            sw = switching_current_from_phase(
                out_csv,
                i_max=args.i_max,
                t_ramp=args.t_ramp,
                v_threshold=args.vth,
                omega_threshold=args.omega_th,
                ignore_t=args.ignore_t,
                sustain_samples=args.sustain,
                smooth_n=args.smooth,
                require_slip=not args.no_slip_check,
                elems=("B1", "B2"),
            )

            if sw is None:
                rows.append({"phi_dc": phi_dc, "trial": k, "seed": seed, "switched": 0})
            else:
                i_sw, t_sw, elem = sw
                i_sws.append(i_sw)
                rows.append(
                    {
                        "phi_dc": phi_dc,
                        "trial": k,
                        "seed": seed,
                        "switched": 1,
                        "i_sw": i_sw,
                        "t_sw": t_sw,
                        "jj": elem,
                    }
                )

        arr = np.array(i_sws, dtype=float)
        results[phi_dc] = arr
        print(f"PHI_DC={phi_dc:.3f}: {arr.size}/{args.trials} switched")

    # Save per-trial table
    df = pd.DataFrame(rows)
    df.to_csv(out_root / "squid_switch_trials.csv", index=False)

    # Plot histograms
    plt.figure()
    for phi_dc, arr in results.items():
        if arr.size == 0:
            continue
        plt.hist(arr * 1e6, bins=30, alpha=0.6, label=f"Φ/Φ0={phi_dc:g} (N={arr.size})")

    plt.xlabel("Switching current [µA]")
    plt.ylabel("Counts")
    plt.legend()
    plt.title("DC SQUID switching current histograms (phase-based)")
    plt.tight_layout()
    out_png = out_root / "squid_switch_hist.png"
    plt.savefig(out_png, dpi=200)
    plt.show()
    print(f"Wrote {out_png}")


if __name__ == "__main__":
    main()
