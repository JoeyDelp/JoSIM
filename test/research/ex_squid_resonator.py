"""
Section VII (Deliverable): analyze SQUID‑tunable resonator run (JoSIM CSV output)

Fixes vs earlier version
------------------------
- Handles JoSIM CSVs where multiple rows share the same timestamp (e.g., internal solver iterations).
  We de-duplicate by keeping the LAST sample for each time value.
- Computes dt from strictly positive time steps (ignores zero/negative diffs).
- Auto-detects the time column by header name (falls back to first column).
- Reads ONLY the needed columns (time, v(NRES), p(PBIAS), p(PNFLUX)) to reduce memory.

Outputs
-------
- sec7_freq_noise_psd.png        : extracted S_f0(f) vs theory (df0/dΦ)^2 S_Φ(f)
- sec7_resonator_spectrum.png    : |V(f)| around resonance (linewidth proxy)
"""
from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path
from typing import List, Tuple, Dict

import numpy as np
import matplotlib.pyplot as plt

try:
    from scipy.signal import welch, butter, filtfilt, hilbert
except Exception as e:
    raise SystemExit(
        "This script needs scipy (signal). Install with: pip install scipy\n"
        f"Original error: {e}"
    )

PHI0 = 2.067833831170082e-15  # Wb (flux quantum)


def _find_header(headers: List[str], contains: str) -> str:
    pat = contains.lower()
    for h in headers:
        if pat in h.lower():
            return h
    raise KeyError(f"Could not find column containing '{contains}'. Headers: {headers}")


def _guess_time_col(headers: List[str]) -> str:
    # Most common: 'time', 't', 'Time(s)', etc.
    for key in headers:
        lk = key.lower()
        if lk == "time" or lk.startswith("time") or lk in ("t", "t(s)", "time(s)"):
            return key
    # fallback: first column
    return headers[0]


def read_csv_dedup(
    path: Path,
    *,
    col_time: str,
    col_v: str,
    col_pbias: str,
    col_pnflux: str,
    max_points: int | None = None,
) -> Dict[str, np.ndarray]:
    """
    Stream-read CSV and de-duplicate by time: keep the last row for each time value.
    Optionally decimate to at most max_points (uniformly) after reading.
    """
    t_list: List[float] = []
    v_list: List[float] = []
    pb_list: List[float] = []
    pn_list: List[float] = []

    with path.open("r", newline="") as f:
        rdr = csv.DictReader(f)
        if not rdr.fieldnames:
            raise ValueError("CSV has no header row.")
        # fast local refs
        ct, cv, cpb, cpn = col_time, col_v, col_pbias, col_pnflux

        last_t = None
        for row in rdr:
            t = float(row[ct])
            v = float(row[cv])
            pb = float(row[cpb])
            pn = float(row[cpn])

            if last_t is None or t != last_t:
                t_list.append(t)
                v_list.append(v)
                pb_list.append(pb)
                pn_list.append(pn)
                last_t = t
            else:
                # overwrite last sample at this same timestamp
                v_list[-1] = v
                pb_list[-1] = pb
                pn_list[-1] = pn

    t = np.asarray(t_list, dtype=float)
    v = np.asarray(v_list, dtype=float)
    pb = np.asarray(pb_list, dtype=float)
    pn = np.asarray(pn_list, dtype=float)

    if max_points is not None and len(t) > max_points:
        # uniform decimation (keep endpoints)
        idx = np.linspace(0, len(t) - 1, max_points).astype(int)
        t, v, pb, pn = t[idx], v[idx], pb[idx], pn[idx]

    return {"t": t, "v": v, "pb": pb, "pn": pn}


def dt_from_time(t: np.ndarray) -> float:
    d = np.diff(t)
    d = d[np.isfinite(d)]
    d = d[d > 0.0]
    if d.size == 0:
        raise ValueError("No positive time steps found. Time column may be constant or mis-parsed.")
    return float(np.median(d))


def bandpass(x: np.ndarray, fs: float, f_lo: float, f_hi: float, order: int = 4) -> np.ndarray:
    b, a = butter(order, [f_lo / (0.5 * fs), f_hi / (0.5 * fs)], btype="bandpass")
    return filtfilt(b, a, x)


def instantaneous_frequency(t: np.ndarray, v: np.ndarray, f0_guess: float | None = None) -> Tuple[np.ndarray, float]:
    dt = dt_from_time(t)
    fs = 1.0 / dt

    if f0_guess is None:
        n = len(v)
        v0 = v - np.mean(v)
        freqs = np.fft.rfftfreq(n, d=dt)
        mag = np.abs(np.fft.rfft(v0))
        idx = np.argmax(mag[1:]) + 1
        f0_guess = float(freqs[idx])

    f_lo = max(1.0, 0.7 * f0_guess)
    f_hi = 1.3 * f0_guess
    v_bp = bandpass(v - np.mean(v), fs, f_lo, f_hi)

    z = hilbert(v_bp)
    phase = np.unwrap(np.angle(z))
    f_inst = np.gradient(phase, t) / (2.0 * math.pi)
    return f_inst, float(f0_guess)


def welch_psd(x: np.ndarray, fs: float, nperseg: int) -> Tuple[np.ndarray, np.ndarray]:
    f, pxx = welch(x, fs=fs, nperseg=nperseg, detrend="constant", scaling="density")
    return f, pxx


def squid_resonator_theory(
    f: np.ndarray,
    *,
    ic: float,
    c_res: float,
    l_geo: float,
    phi_bias_frac: float,
    a_phi1: float,
    alpha: float,
) -> Tuple[np.ndarray, float, float]:
    """
    Small-signal theory overlay:
      S_f0(f) = (df0/dΦ)^2 S_Φ(f)

    Uses βL << 1 symmetric DC-SQUID effective inductance:
      I_c,eff(Φ) = 2 I_c cos(πΦ/Φ0)
      L_SQUID(Φ) = Φ0 / (4π I_c cos(πΦ/Φ0))

    Resonator:
      f0(Φ) = 1 / (2π sqrt(C (L_geo + L_SQUID(Φ))))
    """
    c = math.cos(math.pi * phi_bias_frac)
    if abs(c) < 1e-6:
        raise ValueError("phi_bias_frac too close to 0.5 (cos ~ 0). Pick a safer bias point.")

    Ls = PHI0 / (4.0 * math.pi * ic * c)
    Ltot = l_geo + Ls
    f0 = 1.0 / (2.0 * math.pi * math.sqrt(c_res * Ltot))

    # dLs/dΦ = (1/(4 Ic)) * sin(πΦ/Φ0)/cos^2(πΦ/Φ0)
    dLs_dPhi = (1.0 / (4.0 * ic)) * (math.sin(math.pi * phi_bias_frac) / (c * c))  # H/Wb
    df0_dPhi = (-f0 / (2.0 * Ltot)) * dLs_dPhi  # Hz/Wb

    # Flux noise PSD:
    # a_phi1 is ASD at 1 Hz in Φ0/sqrt(Hz); convert to Wb/sqrt(Hz) then square.
    A_wb = a_phi1 * PHI0
    Sphi = (A_wb**2) * (1.0 / np.maximum(f, 1e-30)) ** alpha  # Wb^2/Hz

    Sf0 = (df0_dPhi**2) * Sphi  # Hz^2/Hz
    return Sf0, f0, df0_dPhi


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", type=Path, help="JoSIM CSV output (e.g., sec7_traces.csv)")
    ap.add_argument("--col-v", default=None, help="Voltage column name (default: auto-detect v(nres))")
    ap.add_argument("--col-pbias", default=None, help="Phase bias column name (default: auto-detect p(pbias))")
    ap.add_argument("--col-pnflux", default=None, help="Phase noise column name (default: auto-detect p(pnflux))")
    ap.add_argument("--col-time", default=None, help="Time column name (default: auto-detect; else first column)")
    ap.add_argument("--phi-bias-frac", type=float, default=0.25, help="Φbias/Φ0")
    ap.add_argument("--ic", type=float, default=1.0e-8, help="JJ Ic per junction [A]")
    ap.add_argument("--c-res", type=float, default=1.0e-10, help="Resonator capacitor [F]")
    ap.add_argument("--l-geo", type=float, default=0.0, help="Extra geometric inductance in series [H]")
    ap.add_argument("--a-phi1", type=float, default=2.0e-6, help="Flux ASD at 1 Hz [Φ0/sqrt(Hz)]")
    ap.add_argument("--alpha", type=float, default=1.0, help="1/f exponent alpha")
    ap.add_argument("--psd-nperseg", type=int, default=131072, help="Welch segment length")
    ap.add_argument("--skip", type=float, default=0.0, help="Skip initial time [s] before analysis")
    ap.add_argument("--max-points", type=int, default=None, help="After de-dup, decimate to at most this many points")
    args = ap.parse_args()

    # Read headers first to auto-detect column names
    with args.csv.open("r", newline="") as f:
        rdr = csv.reader(f)
        headers = next(rdr)
    if not headers:
        raise ValueError("CSV has no header row.")

    col_time = args.col_time or _guess_time_col(headers)
    col_v = args.col_v or _find_header(headers, "v(nres)")
    col_pb = args.col_pbias or _find_header(headers, "p(pbias)")
    col_pn = args.col_pnflux or _find_header(headers, "p(pnflux)")

    d = read_csv_dedup(
        args.csv,
        col_time=col_time,
        col_v=col_v,
        col_pbias=col_pb,
        col_pnflux=col_pn,
        max_points=args.max_points,
    )

    t, v = d["t"], d["v"]

    # Skip early transient if requested
    if args.skip > 0:
        m = t >= (t[0] + args.skip)
        t, v = t[m], v[m]

    dt = dt_from_time(t)
    fs = 1.0 / dt

    # Extract instantaneous frequency
    f_inst, f0_guess = instantaneous_frequency(t, v)
    f0_mean = float(np.median(f_inst))
    df = f_inst - f0_mean

    # PSD
    nperseg = min(args.psd_nperseg, max(256, len(df) // 2))
    f_psd, Sff = welch_psd(df, fs=fs, nperseg=nperseg)

    # Theory overlay
    Sf0_th, f0_th, df0_dPhi = squid_resonator_theory(
        f_psd,
        ic=args.ic,
        c_res=args.c_res,
        l_geo=args.l_geo,
        phi_bias_frac=args.phi_bias_frac,
        a_phi1=args.a_phi1,
        alpha=args.alpha,
    )

    print("=== Timing ===")
    print(f"Samples after de-dup : {len(t)}")
    print(f"Median dt (positive) : {dt:.6g} s")
    print(f"fs                  : {fs:.6g} Hz")
    print("=== Extracted ===")
    print(f"FFT/Hilbert f0_guess : {f0_guess:.6g} Hz")
    print(f"Median f0(t)         : {f0_mean:.6g} Hz")
    print("=== Theory (small-signal) ===")
    print(f"f0(Φbias)            : {f0_th:.6g} Hz")
    print(f"df0/dΦ @ bias        : {df0_dPhi:.6g} Hz/Wb")

    # Plot PSD
    plt.figure()
    plt.loglog(f_psd[1:], Sff[1:], label="Sim: $S_{f_0}(f)$")
    plt.loglog(f_psd[1:], Sf0_th[1:], label="Theory: $(df_0/d\\Phi)^2 S_\\Phi(f)$")
    plt.xlabel("Fourier frequency f [Hz]")
    plt.ylabel("$S_{f_0}(f)$ [Hz$^2$/Hz]")
    plt.legend()
    plt.tight_layout()
    plt.savefig("sec7_freq_noise_psd.png", dpi=200)

    # Resonator spectrum (linewidth proxy)
    n = len(v)
    w = np.hanning(n)
    v0 = (v - np.mean(v)) * w
    freqs = np.fft.rfftfreq(n, d=dt)
    mag = np.abs(np.fft.rfft(v0))
    plt.figure()
    plt.semilogy(freqs[1:], mag[1:])
    plt.xlabel("Frequency [Hz]")
    plt.ylabel("|V(f)| (arb.)")
    plt.tight_layout()
    plt.savefig("sec7_resonator_spectrum.png", dpi=200)

    print("Wrote: sec7_freq_noise_psd.png, sec7_resonator_spectrum.png")


if __name__ == "__main__":
    main()