#!/usr/bin/env python3
"""
Section VII (new approach): Map JoSIM-generated flux noise → resonator frequency noise (no JJ transient)

Pipeline
--------
1) Use JoSIM to generate the *flux/phase noise time series* (no junctions):
       josim-cli -i sec7_fluxnoise_only.cir
   Output: sec7_fluxnoise.csv containing time and P(PNFLUX).

2) Offline mapping to resonator frequency fluctuations:
   - Convert phase noise to flux noise:
         δΦ(t) = (Φ0 / 2π) δφ(t)
   - Use a SQUID-tunable inductance model (βL << 1, symmetric DC-SQUID):
         I_c,eff(Φ) = 2 I_c cos(πΦ/Φ0)
         L_SQ(Φ)    = Φ0 / (4π I_c cos(πΦ/Φ0))
   - Resonator:
         f0(Φ) = 1 / (2π sqrt(C (L_geo + L_SQ(Φ))))
   - Generate f0(t) and compute:
         S_f0(f)  (Welch PSD)
         σ_y(τ)   (overlapping Allan deviation)  where y = (f0 - <f0>)/<f0>

3) Compare to theory:
      S_f0(f) ≈ (df0/dΦ)^2 S_Φ(f)     (small-signal)

This avoids long, stiff JJ transients while still exercising your implemented noise source in JoSIM.
"""

from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path
from typing import Tuple

import numpy as np

# non-GUI backend
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

try:
    from scipy.signal import welch
except Exception as e:
    raise SystemExit("Need scipy. Install: pip install scipy\n" + str(e))

PHI0 = 2.067833831170082e-15  # Wb


def dt_from_time(t: np.ndarray) -> float:
    d = np.diff(t)
    d = d[np.isfinite(d)]
    d = d[d > 0.0]
    if d.size == 0:
        raise ValueError("No positive time steps in CSV.")
    return float(np.median(d))


def read_fluxnoise_csv(path: Path) -> Tuple[np.ndarray, np.ndarray]:
    # expects header: time,"P(PNFLUX)" or similar
    with path.open("r", newline="") as f:
        rdr = csv.reader(f)
        headers = next(rdr)
    # find time column
    time_idx = 0
    for i, h in enumerate(headers):
        if h.strip().lower().startswith("time"):
            time_idx = i
            break
    # find PNFLUX column
    pn_idx = None
    for i, h in enumerate(headers):
        if "p(pnflux" in h.lower():
            pn_idx = i
            break
    if pn_idx is None:
        # allow "P(PNFLUX)" exactly
        for i, h in enumerate(headers):
            if h.strip().lower() == "p(pnflux)":
                pn_idx = i
                break
    if pn_idx is None:
        raise ValueError(f"Could not find P(PNFLUX) column. Headers={headers}")

    t = []
    dphi = []
    with path.open("r", newline="") as f:
        rdr = csv.reader(f)
        next(rdr)  # header
        last_t = None
        for row in rdr:
            ti = float(row[time_idx])
            # de-dup repeated timestamps (keep last)
            if last_t is None or ti != last_t:
                t.append(ti)
                dphi.append(float(row[pn_idx]))
                last_t = ti
            else:
                dphi[-1] = float(row[pn_idx])
    return np.asarray(t), np.asarray(dphi)


def L_squid(phi: np.ndarray, ic: float) -> np.ndarray:
    """
    DC SQUID effective inductance in the zero-voltage state, βL<<1, symmetric junctions.
    phi is flux in Wb (Φ). Uses cos(pi Φ/Φ0).
    """
    c = np.cos(np.pi * phi / PHI0)
    # avoid singularities exactly at half flux quantum
    c = np.clip(c, 1e-6, None) * np.sign(c)
    return PHI0 / (4.0 * np.pi * ic * c)


def f0_from_flux(phi: np.ndarray, ic: float, c_res: float, l_geo: float) -> np.ndarray:
    L = l_geo + L_squid(phi, ic)
    return 1.0 / (2.0 * np.pi * np.sqrt(c_res * L))


def df0_dphi_numeric(phi0: float, ic: float, c_res: float, l_geo: float, dphi: float) -> float:
    f_plus = float(f0_from_flux(np.array([phi0 + dphi]), ic, c_res, l_geo)[0])
    f_minus = float(f0_from_flux(np.array([phi0 - dphi]), ic, c_res, l_geo)[0])
    return (f_plus - f_minus) / (2.0 * dphi)


def psd_welch(x: np.ndarray, fs: float, nperseg: int) -> Tuple[np.ndarray, np.ndarray]:
    f, pxx = welch(x, fs=fs, nperseg=nperseg, detrend="constant", scaling="density")
    return f, pxx


def allan_overlapping(y: np.ndarray, fs: float, taus: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """
    Overlapping Allan deviation for fractional frequency y[n] sampled at fs.
    taus in seconds.
    """
    dev = np.zeros_like(taus, dtype=float)
    for k, tau in enumerate(taus):
        m = int(round(tau * fs))
        if m < 1 or 2*m >= len(y):
            dev[k] = np.nan
            continue
        # overlapping Allan variance
        # sigma^2 = 1/(2*(N-2m)) * sum_{i=0}^{N-2m-1} (y_{i+2m}-2y_{i+m}+y_i)^2
        d = y[2*m:] - 2.0*y[m:-m] + y[:-2*m]
        dev[k] = math.sqrt(0.5 * np.mean(d*d))
    return taus, dev


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", type=Path, help="CSV from sec7_fluxnoise_only.cir (sec7_fluxnoise.csv)")
    ap.add_argument("--phi-bias-frac", type=float, default=0.25, help="Φbias/Φ0")
    ap.add_argument("--ic", type=float, default=1e-6, help="JJ Ic per junction [A] (sets SQUID inductance scale)")
    ap.add_argument("--c-res", type=float, default=1e-12, help="Resonator capacitance [F]")
    ap.add_argument("--l-geo", type=float, default=5e-9, help="Geometric inductance in series [H]")
    ap.add_argument("--linear", action="store_true", help="Use linearized mapping f0 ≈ f0b + (df0/dΦ) δΦ")
    ap.add_argument("--nperseg", type=int, default=131072, help="Welch nperseg")
    ap.add_argument("--allan-points", type=int, default=25, help="Number of tau points (log spaced)")
    args = ap.parse_args()

    t, phi_series = read_fluxnoise_csv(args.csv)
    dt = dt_from_time(t)
    fs = 1.0 / dt

    # Your CSV holds the PNFLUX phase source value; convert phase -> flux
    # If PNFLUX already includes the bias (because PBIAS and PNFLUX are series), that's OK:
    # we define total phase φ_total and convert to Φ_total.
    Phi_total = (PHI0 / (2.0 * np.pi)) * phi_series  # Wb

    Phi_bias = args.phi_bias_frac * PHI0

    # if the series source includes bias, Phi_total already ~ Phi_bias + noise.
    # If user set PHASE_BIAS=0, then Phi_total is purely noise and we add bias here.
    # Heuristic: if mean is near 0, add bias; else leave as-is.
    if abs(np.mean(Phi_total)) < 0.1 * PHI0:
        Phi_total = Phi_bias + Phi_total

    if args.linear:
        f0b = float(f0_from_flux(np.array([Phi_bias]), args.ic, args.c_res, args.l_geo)[0])
        df0_dPhi = df0_dphi_numeric(Phi_bias, args.ic, args.c_res, args.l_geo, dphi=1e-6*PHI0)
        f0_t = f0b + df0_dPhi * (Phi_total - Phi_bias)
    else:
        f0_t = f0_from_flux(Phi_total, args.ic, args.c_res, args.l_geo)
        f0b = float(np.median(f0_t))
        df0_dPhi = df0_dphi_numeric(Phi_bias, args.ic, args.c_res, args.l_geo, dphi=1e-6*PHI0)

    df = f0_t - np.median(f0_t)

    nperseg = min(args.nperseg, max(256, len(df)//2))
    f, Sff = psd_welch(df, fs, nperseg=nperseg)

    # Theory line: injected flux noise is ~1/f^alpha by construction.
    # We can estimate S_Phi from the time series directly:
    dPhi = Phi_total - np.mean(Phi_total)
    _, Sphi_est = psd_welch(dPhi, fs, nperseg=nperseg)
    Sff_th = (df0_dPhi**2) * Sphi_est

    print("=== Sampling ===")
    print(f"Samples : {len(t)}")
    print(f"dt      : {dt:.6g} s")
    print(f"fs      : {fs:.6g} Hz")
    print("=== Mapping ===")
    print(f"f0(bias) ~ {f0b:.6g} Hz")
    print(f"df0/dΦ  ~ {df0_dPhi:.6g} Hz/Wb")
    print(f"Mode    : {'linear' if args.linear else 'nonlinear'}")

    # PSD plot
    plt.figure()
    plt.loglog(f[1:], Sff[1:], label="Sim: $S_{f_0}(f)$ (mapped)")
    plt.loglog(f[1:], Sff_th[1:], label="Theory: $(df_0/d\\Phi)^2 S_{\\Phi}(f)$ (from same trace)")
    plt.xlabel("Fourier frequency f [Hz]")
    plt.ylabel("$S_{f_0}(f)$ [Hz$^2$/Hz]")
    plt.legend()
    plt.tight_layout()
    plt.savefig("sec7_mapped_freq_noise_psd.png", dpi=200)

    # Allan deviation
    y = df / np.median(f0_t)  # fractional frequency
    taus = np.logspace(np.log10(1/fs), np.log10((t[-1]-t[0])/10), args.allan_points)
    taus, adev = allan_overlapping(y, fs, taus)
    plt.figure()
    plt.loglog(taus, adev)
    plt.xlabel("Averaging time $\\tau$ [s]")
    plt.ylabel("Overlapping Allan deviation $\\sigma_y(\\tau)$")
    plt.tight_layout()
    plt.savefig("sec7_allan_dev.png", dpi=200)

    print("Wrote: sec7_mapped_freq_noise_psd.png, sec7_allan_dev.png")


if __name__ == "__main__":
    main()
