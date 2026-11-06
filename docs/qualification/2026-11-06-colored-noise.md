# Colored, flux and fractional noise — 6 November 2026

This study repairs F17, an amplitude-normalization defect in the merged
power-law source bank. It separately tests stochastic source units, timing and
fractional device injection. It does not substitute for the #107/#108 physical
SQUID/noise example, which remains separate work.

## Reproduction and model

For the nominal 1–256 Hz band, eight seeded runs with 65536 samples each give
these Welch PSD ratios at 16 Hz relative to the stated \(A_1^2/f^\alpha\):

| Exponent | Before repair |
|---|---|
| 0 | 0.630047 |
| 0.5 | 1.177373 |
| 1 | 1.929505 |
| 1.5 | 3.523535 |
| 2 | 7.916022 |

The cause is deterministic. A finite bank omits the fast/slow relaxation tails,
then normalizes its continuous spectrum at 1 Hz or the nearest band edge.
Moving that anchor onto a rolloff changes the interior amplitude. The endpoint
exponents also have logarithmic shape errors if both omitted tails are discarded.
Exact AR(1) sampling adds a further gain to fast poles. These effects exceed
sampling uncertainty and do not represent the advertised interior power law.

The repair retains the existing positive OU bank and exact AR(1) evolution.
For \(0<\alpha<2\), the governing identity is

\[
\frac{A_1^2}{f^\alpha}=4C\int_0^\infty
 \frac{\tau^\alpha}{1+(2\pi f\tau)^2}\,d\log\tau,
\qquad C=\frac{A_1^2(2\pi)^\alpha\sin(\pi\alpha/2)}{2\pi}.
\]

Frequency is expressed numerically in Hz. The finite log-midpoint quadrature
uses base variances \(C\tau_k^{\alpha-1}\delta\log\tau\). For boundary times
\(\tau_f=1/(2\pi f_H)\), \(\tau_s=1/(2\pi f_L)\), the missing asymptotic tails
are folded into the first/last pole as

\[
v_{fast}=\frac{C\tau_f^\alpha}{\alpha\tau_0},\qquad
v_{slow}=\frac{C\tau_s^{\alpha-2}\tau_{N-1}}{2-\alpha}.
\]

The endpoint limits are a fast-pole variance \(A_1^2/(4\tau_0)\) at exponent 0
and a slow-pole variance \(A_1^2\pi^2\tau_{N-1}\) at exponent 2. All variances
are multiplied by \(\tanh q/q\), \(q=\Delta/(2\tau_k)\), to remove the exact
AR(1) DC-gain increase before interpolation. `expm1` preserves small innovation
variance; log-domain weights avoid spurious intermediate overflow. PNOISE,
FNOISE and ICNOISE use one initializer, preserving their unit relationship.

This remains a finite-band approximation with smooth low/high turnover and
interpolation rolloff. It is not an ideal sharp-cutoff power law. The OU
superposition basis is described by
[Milotti](https://doi.org/10.1103/PhysRevE.51.3087); the weights, endpoint
approximation and sampling correction above are the explicit model used here.

## Independent acceptance checks

Runtime PSD expectations use the ideal \(A_1^2/f^\alpha\), not values obtained
from JoSIM's internal bank. Eight independent seeds, 32768 samples per seed,
2048-point periodic Hann windows and half overlap give 31 segments per seed.
The effective count is about 235, or 6.5% relative standard error. The
predeclared 40% bound exceeds five standard errors plus the small interior
approximation error. Three frequencies, 8/16/24 Hz, and five exponents give
measured PSD/ideal ratios **0.865612–1.082230** after repair. This conservative
short native regression is supported by the deterministic spectrum calculation
below; it is not a claim of 40% model accuracy.

At exponent 0 or 2, a single endpoint pole remains. Its closed variance and
covariance provide independent expectations. For \(K\) uniformly spaced samples
per update interval, covariance \(a=e^{-\Delta/\tau}\), and grid variance \(v\),

\[
E[\widehat{\operatorname{var}}]\simeq
v\left[1-(1-a)\frac{K^2-1}{3K^2}\right].
\]

The small finite-record mean-estimation bias is inside the statistical allowance.
Eight seeds cover 256 s, with longest \(\tau<0.16\) s. The conservative variance
standard-error bound \(\sqrt{4\tau/T}<5\%\) justifies the predeclared 30% limit.
Actual errors are below 0.9%:

| Exponent | Samples/interval | Measured variance | Analytic variance |
|---|---|---|---|
| 0 | 1 | 231.626873 | 230.901854 |
| 0 | 4 | 163.130932 | 162.464466 |
| 2 | 1 | 1.46973315 | 1.48261342 |
| 2 | 4 | 1.46373470 | 1.47662857 |

Additional invariants establish:

- Band scaling by \(c=10^{-9}\) or \(10^6\), including crossings of 1 Hz,
  gives \(x_c(t/c)=c^{(1-\alpha)/2}x(t)\) for all five exponents. The bound is
  1e-8 times \(1+|x|\), covering floating update-clock/logarithm roundoff.
- Dense versus skipped-tick evaluation gives exactly the same samples at
  common times for N=4/48/512, including before/after delay. The timestep floor
  matches an explicit effective interval exactly; zero ASD gives zero output.
- FNOISE gives \(2\pi(\Phi_{DC}+x)\) radians for the same controlled realization
  at all five exponents, within 1e-11 rad. Standalone ICNOISE is exactly the
  dimensionless PNOISE sequence. The existing inductor check establishes flux
  linkage, and phase-source tests cover applied samples in both analysis modes.
- Actual gated junction current in both modes equals
  \(I_c g\max(0.01,1+\delta)\sin\phi\), within 1e-10 A. Small fluctuations and
  a deliberately large case exercise the lower clipping bound. The PSD tests
  apply to the small-signal source; clipped noise no longer has a Gaussian PSD.
- Same-process odd/even reseeding and rewind, independent runs, and full-cycle
  timestep retry remain required. Invalid timing/process counts fail cleanly.

## Approximation and compatibility limits

A separate deterministic calculation evaluates the specified finite bank's
exact sampled/interpolated spectrum. It audits approximation error; it is not
the independent runtime oracle. The scan covers nine exponents from 0 to 2,
band ratios 100/256/1e4/1e8, 121 logarithmic frequency positions between
\(10f_L\) and \(f_H/10\), and N=4/48/512. The default N=48 gives ideal-PSD
ratios **0.975331–1.000137**, while N=512 gives **0.978863–1.0000005**. Remaining
upper-band interpolation loss does not disappear by increasing N. N=4 reaches
**0.495881–1.527313**: accepting the minimum process count does not promise
accurate spectra across eight decades. Use more processes and check the
measurement's band and timestep. Near turnover edges, larger deviations are
expected; sufficiently narrow bands have no well-separated interior.

Source syntax, class/struct layouts, units and existing signatures are retained.
A private initializer removes duplicated bank construction. Corrected weights
and endpoint ordering change seeded colored waveforms. `A1` now consistently
specifies the ideal interior law's extrapolated 1 Hz amplitude; it does not
promise the actual rolloff PSD at 1 Hz. Public debug field `scale` is 1 because
stationary variances include amplitude; `S1Hz` is a continuous-OU diagnostic,
before sampling/interpolation. See the [noise guide](../noise_sources.md).

```bash
ctest --test-dir build/dev -R 'regression::(noise|functions|cli)' \
  --output-on-failure --no-tests=error
```

GCC16/Clang23 Debug and Release pass all 33 CTest groups. The complete
ASan/UBSan suite with leak detection passes 33/33 in 210.03 s, including the
expanded noise group in 23.19 s. Native qualification remains pending for this
implementing revision. Red/green measurements, analytic calculations and exact
source/binary identities are under `build/audit-resolution/colored-noise/`;
compact receipts and the finite-bank calculation are preserved under
`artifacts/audit-resolution/colored-noise/`.
