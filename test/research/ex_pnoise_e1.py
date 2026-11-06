import numpy as np
import matplotlib.pyplot as plt

def welch_psd(x, fs, nperseg=65536, noverlap=32768):
    x = np.asarray(x)
    step = nperseg - noverlap
    win = np.hanning(nperseg)
    U = (win**2).sum()
    acc = None
    nseg = 0
    for start in range(0, len(x) - nperseg + 1, step):
        seg = x[start:start+nperseg]
        seg = seg - np.mean(seg)
        X = np.fft.rfft(seg * win)
        P = (np.abs(X)**2) / (fs * U)
        P[1:-1] *= 2.0  # one-sided
        acc = P if acc is None else (acc + P)
        nseg += 1
    f = np.fft.rfftfreq(nperseg, d=1/fs)
    return f, acc / max(nseg, 1)

def ou_bank_psd(f, A1, alpha, dt, tcorr, N):
    # Reconstruct the exact same OU bank logic as parse_pnoise()
    fL = 1.0 / tcorr
    fH = 1.0 / (2.0 * dt)
    logfL = np.log(fL)
    logfH = np.log(fH)

    k = np.arange(N)
    frac = (k + 0.5) / N
    fk = np.exp(logfL + frac * (logfH - logfL))
    tau = 1.0 / (2.0*np.pi*fk)

    baseVar = tau**(alpha - 1.0)
    # analytic OU sum PSD (one-sided): sum 4*var*tau / (1+(2π f tau)^2)
    def Sx(f_hz, var):
        w = 2.0*np.pi*f_hz
        return np.sum((4.0*var*tau) / (1.0 + (w*tau)**2), axis=0)

    targetS1 = A1*A1
    S1_base = Sx(1.0, baseVar)
    scale = targetS1 / S1_base
    var = scale * baseVar

    # vectorized PSD
    w = 2.0*np.pi*f
    S = np.sum((4.0*var[:,None]*tau[:,None]) / (1.0 + (w[None,:]*tau[:,None])**2), axis=0)

# ---- load ----
data = np.loadtxt("ex_pnoise_e1.csv", delimiter=",", skiprows=1)
t = data[:,0]
v = data[:,1]

dt = np.median(np.diff(t))
fs = 1.0 / dt

# ---- measured PSD of V ----
f, Sv = welch_psd(v, fs)

# ---- model params (set to what you simulated) ----
R = 10000.0
C = 1e-9
A1 = 1e-12      # A/sqrt(Hz) at 1 Hz
alpha = 1.0
tcorr = 2.0     # seconds
dt_noise = dt   # if you used tstep == dt_noise

fL = 1.0 / tcorr
fH = 1.0 / (2.0 * dt_noise)

# current PSD (one-sided), compare only in midband to avoid edge shaping
Si = (A1**2) / (np.maximum(f, fL)**alpha)   # simple low-f corner clamp for plotting
#Si = ou_bank_psd(f, A1=1e-12, alpha=1.0, dt=1e-4, tcorr=2.0, N=64)
# (ignore above fH in comparisons)
Z = R / (1.0 + 1j * 2.0*np.pi*f*R*C)
Sv_th = (np.abs(Z)**2) * Si

# ---- choose fit/compare band ----
mask = (f > 10.0*fL) & (f < 0.2*fH) & (f > 1.0)  # keep away from corners

# slope estimate for measured Sv in band
logf = np.log10(f[mask])
logS = np.log10(Sv[mask])
m, b = np.polyfit(logf, logS, 1)
print("Measured slope dlog10(Sv)/dlog10(f) =", m)

# For R||C driven by 1/f^alpha current noise:
# low f: Sv ~ R^2 / f^alpha => slope ~ -alpha
# high f: Sv ~ 1/f^(alpha+2) => slope ~ -(alpha+2)
# Your mask above is “midband”; adjust mask to show both regions if you want.

ratio = Sv[mask] / Sv_th[mask]
print("median(meas/theory) =", np.median(ratio))
print("10-90% range        =", np.percentile(ratio, [10, 90]))

plt.figure()
plt.loglog(f[mask], Sv[mask], label="measured")
plt.loglog(f[mask], Sv_th[mask], label="theory (midband model)")
plt.xlabel("Frequency [Hz]")
plt.ylabel("S_V(f) [V^2/Hz]")
plt.grid(True, which="both")
plt.legend()
plt.show()
