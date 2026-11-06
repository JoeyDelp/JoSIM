import numpy as np
import matplotlib.pyplot as plt

kB = 1.380649e-23
PHI0 = 2.067833848e-15  # Wb (V*s)

# ---- load your output ----
# Assumption: two columns: time, value
# If your file has headers or more cols, adjust accordingly.
data = np.loadtxt("output.csv", delimiter=',', skiprows=1)
t = data[:, 0]
y = data[:, 1]

dt = np.median(np.diff(t))
fs = 1.0 / dt

# ---- interpret y ----
v = y

v = v - np.mean(v)

# ---- Welch PSD (one-sided) ----
def welch_psd(x, fs, nperseg=8192, noverlap=4096):
    x = np.asarray(x)
    step = nperseg - noverlap
    if step <= 0:
        raise ValueError("noverlap must be < nperseg")

    win = np.hanning(nperseg)
    U = (win**2).sum()

    psd_accum = None
    nseg = 0

    for start in range(0, len(x) - nperseg + 1, step):
        seg = x[start:start+nperseg]
        segw = seg * win
        X = np.fft.rfft(segw)
        P = (np.abs(X)**2) / (fs * U)  # two-sided normalization on rfft bins

        # Convert to one-sided PSD: rfft already excludes negative freqs,
        # double all bins except DC and Nyquist (if present)
        P_one = P.copy()
        if len(P_one) > 2:
            P_one[1:-1] *= 2.0
        elif len(P_one) == 2:
            P_one[1] *= 2.0

        psd_accum = P_one if psd_accum is None else (psd_accum + P_one)
        nseg += 1

    f = np.fft.rfftfreq(nperseg, d=1/fs)
    return f, psd_accum / max(nseg, 1)

f, Sv_meas = welch_psd(v, fs, nperseg=16384, noverlap=8192)

# ---- theory ----
T = 4.2
R = 10000.0
C = 1e-9
Sv_th = (4*kB*T*R) / (1 + (2*np.pi*f*R*C)**2)

# Compare only well below Nyquist and below ~0.2*NEB
NEB = 1e6
mask = (f > 10.0) & (f < 0.2*NEB)

Vrms_meas = np.sqrt(np.mean(v*v))
Vrms_exp  = np.sqrt(kB*T/C)
print("Vrms_measured =", Vrms_meas)
print("Vrms_expected =", Vrms_exp)
print("Vrms_ratio    =", Vrms_meas / Vrms_exp)

band = (f > 200) & (f < 2000)
Sv_plate_meas = np.median(Sv_meas[band])
Sv_plate_exp  = 4*kB*T*R
print("Sv_plate_meas =", Sv_plate_meas)
print("Sv_plate_exp  =", Sv_plate_exp)
print("plate_ratio   =", Sv_plate_meas / Sv_plate_exp)

# ---- plot ----
plt.figure()
plt.loglog(f[mask], Sv_meas[mask], label="measured")
plt.loglog(f[mask], Sv_th[mask], label="theory")
plt.xlabel("Frequency [Hz]")
plt.ylabel("S_V(f) [V^2/Hz]")
plt.legend()
plt.grid(True, which="both")
plt.show()

# Optional quantitative error metric
ratio = Sv_meas[mask] / Sv_th[mask]
print("median(meas/theory) =", np.median(ratio))
print("10-90% range        =", np.percentile(ratio, [10, 90]))
