import numpy as np
import matplotlib.pyplot as plt

PHI0 = 2.067833848e-15  # Wb = V*s

# ---- set these to match the netlist ----
Aphi0  = 1e-6     # Phi0/sqrt(Hz) at 1 Hz
alpha  = 1.0
L      = 1e-9     # H
dt     = 1e-4     # s (noise tick and sim tstep)
tcorr  = 2.0      # s

# ---- load output ----
data = np.loadtxt("ex_fnoise_L1.csv", delimiter=',', skiprows=1)
t = data[:, 0]
i = data[:, 1]
i = i - np.mean(i)

dt_meas = np.median(np.diff(t))
fs = 1.0 / dt_meas

def welch_psd(x, fs, nperseg=16384, noverlap=8192):
    x = np.asarray(x)
    step = nperseg - noverlap
    win = np.hanning(nperseg)
    U = (win**2).sum()

    psd_accum = None
    nseg = 0
    for start in range(0, len(x) - nperseg + 1, step):
        seg = x[start:start+nperseg] * win
        X = np.fft.rfft(seg)
        P = (np.abs(X)**2) / (fs * U)

        P_one = P.copy()
        if len(P_one) > 2:
            P_one[1:-1] *= 2.0
        elif len(P_one) == 2:
            P_one[1] *= 2.0

        psd_accum = P_one if psd_accum is None else (psd_accum + P_one)
        nseg += 1

    f = np.fft.rfftfreq(nperseg, d=1/fs)
    return f, psd_accum / max(nseg, 1)

f, SI_meas = welch_psd(i, fs)

# ---- theory (one-sided) ----
fL = 1.0 / tcorr
fH = 1.0 / (2.0 * dt)

# SI(f) = (PHI0/L)^2 * Aphi0^2 * (f/1Hz)^(-alpha)
SI_th = ((PHI0 / L)**2) * (Aphi0**2) * np.zeros_like(f)
pos = f > 0
SI_th[pos] = ((PHI0 / L)**2) * (Aphi0**2) * (f[pos]**(-alpha))

# Compare only where the shaped band should match power law well
mask = (f > 5.0*fL) & (f < 0.2*fH) & (f > 0)

# slope check
p = np.polyfit(np.log10(f[mask]), np.log10(SI_meas[mask]), 1)
slope = p[0]

ratio = SI_meas[mask] / SI_th[mask]
print("Measured slope dlog10(SI)/dlog10(f) =", slope)
print("median(meas/theory) =", np.median(ratio))
print("10-90% range        =", np.percentile(ratio, [10, 90]))

# plot
plt.figure()
plt.loglog(f[mask], SI_meas[mask], label="measured")
plt.loglog(f[mask], SI_th[mask],  label="theory")
plt.xlabel("Frequency [Hz]")
plt.ylabel("S_I(f) [A^2/Hz]")
plt.grid(True, which="both")
plt.legend()
plt.show()
