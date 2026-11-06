#!/usr/bin/env python3
"""
Section VII (revised, v2): Validate JoSIM FNOISE + map flux noise -> SQUID-resonator frequency noise

This version fixes two issues that can make the result look "too perfect" or confusing:

1) The previous "Theory" curve used S_Φ(f) estimated from the *same trace*, so the overlay can
   look identical even if the absolute scaling is wrong. Here we compare against the *target*
   1/f^α model from the user-supplied A1, α (and optional ZOH roll-off from DTNOISE).

2) It also writes a dedicated plot of the *flux noise PSD* so you can directly verify that
   the ASD at 1 Hz matches the parameter you asked for.

Input CSV must contain:
    time, "P(PNFLUX)"
where P(PNFLUX) is the phase-noise waveform in radians produced by FNOISE(...) on a phase source.

Outputs:
    sec7_flux_psd_check.png
    sec7_mapped_freq_noise_psd_v2.png
    sec7_allan_dev_v2.png
"""

from __future__ import annotations
import argparse
import csv
import math
from pathlib import Path
from typing import Tuple

import numpy as np

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

from scipy.signal import welch

PHI0 = 2.067833831170082e-15  # Wb


def read_csv(path: Path) -> Tuple[np.ndarray, np.ndarray]:
    with path.open("r", newline="") as f:
        rdr = csv.reader(f)
        hdr = next(rdr)
        # indices
        time_idx = None
        pn_idx = None
        for i, h in enumerate(hdr):
            hl = h.strip().lower()
            if hl.startswith("time"):
                time_idx = i
            if "p(pnflux" in hl or hl == "p(pnflux)":
                pn_idx = i
        if time_idx is None or pn_idx is None:
            raise ValueError(f"Missing required columns. Headers: {hdr}")

        t = []
        dphi = []
        last_t = None
        for row in rdr:
            ti = float(row[time_idx])
            vi = float(row[pn_idx])
            # de-dup repeated timestamps (keep last)
            if last_t is None or ti != last_t:
                t.append(ti)
                dphi.append(vi)
                last_t = ti
            else:
                dphi[-1] = vi
        return np.asarray(t, dtype=float), np.asarray(dphi, dtype=float)


def median_dt(t: np.ndarray) -> float:
    d = np.diff(t)
    d = d[np.isfinite(d)]
    d = d[d > 0.0]
    if d.size == 0:
        raise ValueError("No positive dt found.")
    return float(np.median(d))


def L_squid(phi_wb: np.ndarray, ic: float) -> np.ndarray:
    """
    DC SQUID effective inductance (βL << 1, symmetric).
      L_SQ(Φ) = Φ0 / (4π Ic cos(π Φ / Φ0))
    """
    c = np.cos(np.pi * phi_wb / PHI0)
    # avoid exact singularities
    c = np.where(np.abs(c) < 1e-6, np.sign(c) * 1e-6, c)
    return PHI0 / (4.0 * np.pi * ic * c)


def f0_from_flux(phi_wb: np.ndarray, ic: float, c_res: float, l_geo: float) -> np.ndarray:
    L = l_geo + L_squid(phi_wb, ic)
    return 1.0 / (2.0 * np.pi * np.sqrt(c_res * L))


def df0_dphi_numeric(phi0: float, ic: float, c_res: float, l_geo: float, dphi: float) -> float:
    f_plus = float(f0_from_flux(np.array([phi0 + dphi]), ic, c_res, l_geo)[0])
    f_minus = float(f0_from_flux(np.array([phi0 - dphi]), ic, c_res, l_geo)[0])
    return (f_plus - f_minus) / (2.0 * dphi)


def welch_psd(x: np.ndarray, fs: float, nperseg: int) -> Tuple[np.ndarray, np.ndarray]:
    f, pxx = welch(x, fs=fs, nperseg=nperseg, detrend="constant", scaling="density")
    return f, pxx


def target_Sphi(f: np.ndarray, A1_phi0: float, alpha: float, dtnoise: float | None) -> np.ndarray:
    """
    Target one-sided PSD of flux noise S_Φ(f) in Wb^2/Hz for a 1/f^alpha model:
        ASD_Φ(1 Hz) = A1_phi0 * Φ0   [Wb/sqrt(Hz)]
        S_Φ(f) = (ASD_Φ(1Hz))^2 * f^{-alpha}

    If dtnoise is provided, apply a simple zero-order-hold roll-off:
        |H(f)|^2 = sinc^2(pi f dtnoise)
    (This approximates piecewise-constant updates every DTNOISE.)
    """
    S = np.zeros_like(f, dtype=float)
    A_wb = A1_phi0 * PHI0
    with np.errstate(divide="ignore", invalid="ignore"):
        S = (A_wb * A_wb) * np.power(f, -alpha)
    S[~np.isfinite(S)] = 0.0
    if dtnoise is not None and dtnoise > 0.0:
        # np.sinc(x) = sin(pi x)/(pi x) so we want sinc(f*dtnoise)
        H2 = np.sinc(f * dtnoise) ** 2
        S *= H2
    return S


def overlapping_allan(y: np.ndarray, fs: float, taus: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    dev = np.full_like(taus, np.nan, dtype=float)
    N = len(y)
    for k, tau in enumerate(taus):
        m = int(round(tau * fs))
        if m < 1 or 2*m >= N:
            continue
        d = y[2*m:] - 2.0*y[m:-m] + y[:-2*m]
        dev[k] = math.sqrt(0.5 * np.mean(d*d))
    return taus, dev


def nearest_at(f: np.ndarray, arr: np.ndarray, f0: float) -> float:
    idx = int(np.argmin(np.abs(f - f0)))
    return float(arr[idx])


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", type=Path)
    ap.add_argument("--a1-phi0", type=float, default=2e-6,
                    help="Target ASD at 1 Hz in Φ0/sqrt(Hz) (matches netlist A_PHI1)")
    ap.add_argument("--alpha", type=float, default=1.0,
                    help="1/f exponent alpha (matches netlist ALPHA)")
    ap.add_argument("--dtnoise", type=float, default=None,
                    help="DTNOISE used in JoSIM [s] (optional; applies ZOH roll-off)")
    ap.add_argument("--phi-bias-frac", type=float, default=0.25)
    ap.add_argument("--ic", type=float, default=1e-6, help="Ic per junction [A]")
    ap.add_argument("--c-res", type=float, default=1e-12, help="Resonator capacitance [F]")
    ap.add_argument("--l-geo", type=float, default=5e-9, help="Geometric inductance [H]")
    ap.add_argument("--nperseg", type=int, default=131072)
    ap.add_argument("--allan-points", type=int, default=25)
    ap.add_argument("--detrend-linear", action="store_true",
                    help="Remove a best-fit line from f0(t) before PSD/Allan (helps with finite-record drift).")
    args = ap.parse_args()

    t, dphi = read_csv(args.csv)
    dt = median_dt(t)
    fs = 1.0 / dt

    # Convert phase noise [rad] to flux noise [Wb]
    dPhi = (PHI0 / (2.0 * np.pi)) * (dphi - np.mean(dphi))

    # Add bias for f0 mapping
    Phi_bias = args.phi_bias_frac * PHI0
    Phi_total = Phi_bias + dPhi

    # Map to resonator frequency
    f0_t = f0_from_flux(Phi_total, args.ic, args.c_res, args.l_geo)
    f0_med = float(np.median(f0_t))
    df0_dPhi = df0_dphi_numeric(Phi_bias, args.ic, args.c_res, args.l_geo, dphi=1e-6*PHI0)

    # Optional detrend (can make Allan look more textbook for finite 1/f records)
    df = f0_t - f0_med
    if args.detrend_linear:
        # subtract best-fit line from df
        x = np.arange(len(df), dtype=float)
        A = np.vstack([x, np.ones_like(x)]).T
        m, b = np.linalg.lstsq(A, df, rcond=None)[0]
        df = df - (m*x + b)

    # PSD estimates
    nperseg = min(args.nperseg, max(256, len(df)//2))
    f, Sphi_est = welch_psd(dPhi, fs, nperseg=nperseg)
    _, Sff_est = welch_psd(df, fs, nperseg=nperseg)

    # Target theory curves (from A1, alpha)
    Sphi_th = target_Sphi(f, args.a1_phi0, args.alpha, args.dtnoise)
    Sff_th = (df0_dPhi**2) * Sphi_th

    # Report sanity checks at ~1 Hz (if available)
    Sphi1_est = nearest_at(f, Sphi_est, 1.0)
    Aphi1_est_phi0 = math.sqrt(Sphi1_est) / PHI0  # in Φ0/sqrt(Hz)

    print("=== Sampling ===")
    print(f"Samples : {len(t)}")
    print(f"dt      : {dt:.6g} s")
    print(f"fs      : {fs:.6g} Hz")
    print("=== Flux noise check ===")
    print(f"Target A1 (1 Hz) : {args.a1_phi0:.6g} Φ0/sqrt(Hz)")
    print(f"Estimated A1     : {Aphi1_est_phi0:.6g} Φ0/sqrt(Hz)  (from Welch near 1 Hz)")
    print("=== Mapping ===")
    print(f"f0(bias)   ~ {f0_med:.6g} Hz")
    print(f"df0/dΦ     ~ {df0_dPhi:.6g} Hz/Wb")
    print(f"nperseg    : {nperseg}")
    print(f"detrend    : {'linear' if args.detrend_linear else 'mean-only'}")

    # Plot 1: Flux PSD check
    plt.figure()
    m = f > 0
    plt.loglog(f[m], Sphi_est[m] / (PHI0**2), label="Estimated $S_\\Phi(f)$ (from trace)")
    plt.loglog(f[m], Sphi_th[m] / (PHI0**2), label="Target $S_\\Phi(f)$ (A1, α)")
    plt.xlabel("Fourier frequency f [Hz]")
    plt.ylabel("$S_{\\Phi}(f)$ [$\\Phi_0^2$/Hz]")
    plt.legend()
    plt.tight_layout()
    plt.savefig("sec7_flux_psd_check.png", dpi=200)

    # Plot 2: Frequency-noise PSD
    plt.figure()
    plt.loglog(f[m], Sff_est[m], label="Sim (mapped): $S_{f_0}(f)$")
    plt.loglog(f[m], Sff_th[m], label="Theory: $(df_0/d\\Phi)^2 S_{\\Phi}(f)$")
    plt.xlabel("Fourier frequency f [Hz]")
    plt.ylabel("$S_{f_0}(f)$ [Hz$^2$/Hz]")
    plt.legend()
    plt.tight_layout()
    plt.savefig("sec7_mapped_freq_noise_psd_v2.png", dpi=200)

    # Plot 3: Allan deviation
    y = df / f0_med
    taus = np.logspace(np.log10(1/fs), np.log10((t[-1]-t[0])/10), args.allan_points)
    taus, adev = overlapping_allan(y, fs, taus)
    plt.figure()
    plt.loglog(taus, adev)
    plt.xlabel("Averaging time $\\tau$ [s]")
    plt.ylabel("Overlapping Allan deviation $\\sigma_y(\\tau)$")
    plt.tight_layout()
    plt.savefig("sec7_allan_dev_v2.png", dpi=200)

    print("Wrote: sec7_flux_psd_check.png, sec7_mapped_freq_noise_psd_v2.png, sec7_allan_dev_v2.png")


if __name__ == "__main__":
    main()
