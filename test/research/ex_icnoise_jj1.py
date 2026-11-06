import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy.signal import welch

# -----------------------
# Load JoSIM CSV output
# -----------------------
csv_path = "ex_icnoise_jj1.csv"  # adjust if needed
df = pd.read_csv(csv_path)

t = df["time"].to_numpy()
x = df["I(B1)"].to_numpy()

# Basic sampling info
dt = float(np.median(np.diff(t)))
fs = 1.0 / dt

# Remove mean (DC)
x = x - np.mean(x)

# -----------------------
# Welch PSD (one-sided)
# -----------------------
nperseg = 16384
noverlap = 8192
f, SI = welch(
    x,
    fs=fs,
    nperseg=min(nperseg, len(x)),
    noverlap=min(noverlap, max(0, len(x) // 2 - 1)),
    detrend=False,
    scaling="density",
    return_onesided=True,
)

# -----------------------
# Theory: S_I(f) = A1^2 * f^{-alpha}
# A1 is ASD at 1 Hz: [A/sqrt(Hz)]
# -----------------------
A1 = 1e-6
alpha = 1.0
SI_th = np.zeros_like(f)
pos = f > 0
SI_th[pos] = (A1 ** 2) * (f[pos] ** (-alpha))

# -----------------------
# Compare in a safe band
# (avoid f~0 and avoid the very top end)
# -----------------------
fmin = 1.0
fmax = 0.1 * fs
mask = (f >= fmin) & (f <= fmax) & (SI_th > 0)

ratio = SI[mask] / SI_th[mask]
slope = np.polyfit(np.log10(f[mask]), np.log10(SI[mask]), 1)[0]

print("dt =", dt, "s, fs =", fs, "Hz")
print("Measured slope dlog10(SI)/dlog10(f) =", slope)
print("median(meas/theory) =", float(np.median(ratio)))
print("10-90% range        =", np.quantile(ratio, [0.1, 0.9]))

# -----------------------
# Plot: PSD + theory + ratio
# -----------------------
plt.figure()
plt.loglog(f[pos], SI[pos], label="Measured PSD (Welch)")
plt.loglog(f[pos], SI_th[pos], label=r"Theory $A_1^2 f^{-\alpha}$")

# Shaded band used for fit/ratio
yl = np.nanmin(SI[pos][SI[pos] > 0])
yh = np.nanmax(SI[pos])
plt.fill_between([fmin, fmax], [yl, yl], [yh, yh], alpha=0.15, label="Fit/compare band")

plt.xlabel("Frequency [Hz]")
plt.ylabel(r"$S_I(f)$ [A$^2$/Hz]")
plt.title("JJ Critical-Current Noise: Measured vs Theory")
plt.grid(True, which="both", linestyle=":")
plt.legend()

plt.figure()
plt.semilogx(f[mask], ratio, label="Measured / Theory")
plt.axhline(1.0, linestyle="--", label="Ideal = 1")
plt.xlabel("Frequency [Hz]")
plt.ylabel("Ratio")
plt.title("PSD Ratio in Comparison Band")
plt.grid(True, which="both", linestyle=":")
plt.legend()

plt.show()
