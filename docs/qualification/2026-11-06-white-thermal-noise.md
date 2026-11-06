# White bandwidth and junction thermal noise — 6 November 2026

This study repairs F15/F16 and completes the state-amplitude part of A02.
It does not close the broader SQUID/noise example or colored-noise work.

## Effective bandwidth

Let independent Gaussian update values have variance \(\sigma_y^2=A^2/(2\Delta)\),
where \(A\) is the one-sided ASD and \(\Delta\) is the update interval. Linear
interpolation at fractional position \(u\) has variance
\(\sigma_y^2[(1-u)^2+u^2]\). Averaging over one interval gives

\[
\overline{\sigma_x^2}=\frac23\sigma_y^2=\frac{A^2}{3\Delta}.
\]

The triangular interpolation kernel gives the time-averaged one-sided spectrum
\(S(f)=A^2\operatorname{sinc}^4(f\Delta)\), with normalized sinc. Its integral
defines equivalent noise bandwidth \(B=1/(3\Delta)\). The previous mapping
\(\Delta=1/(2NEB)\) supplied only \(2NEB/3\), despite advertising NEB.
The repaired mapping is \(\Delta=\max(h,1/(3NEB))\). An explicit source
`TSTEP` retains its interval and grid-variance convention.

Eight independent seeds, 524288 samples per seed at \(h=1/8192\) s, and NEB=64 Hz
measure pooled variance **42.7041 before repair** and **63.8564268 after repair**
against 64. At least 65536 independent knots give relative standard error below
0.6%; the predeclared 5% bound exceeds eight standard errors. The finite sample
spacing adds less than 0.03% phase-sampling correction. A one-second discard
removes the source's initial zero-to-first-knot ramp.

Welch estimates use 16384-point periodic Hann windows, half overlap, 63 segments
per seed, and eight seeds. The approximate effective segment count is 477,
giving relative standard error 4.6%. A 30% bound exceeds six standard errors.
Frequency bins start at 1 Hz, avoiding the mean-removal/DC bin. Comparison is
against the analytic sinc transfer, including its rolloff:

| Frequency (Hz) | Measured PSD / analytic PSD |
|---|---|
| 1 | 0.994512 |
| 2 | 1.016064 |
| 4 | 0.969538 |
| 32 | 0.986290 |
| 64 | 1.009167 |
| 128 | 1.008245 |

Sampling at update knots has larger variance than time averaging. Eight seeds
and 8192 knots each check the independent formula
\(\sigma_y^2[2/3+1/(3K^2)]\) for \(K\) samples per update interval:

| K | Measured variance | Expected variance |
|---|---|---|
| 1 | 0.499915340 | 0.500000000 |
| 2 | 0.374781627 | 0.375000000 |
| 4 | 0.343502529 | 0.343750000 |
| 16 | 0.333731599 | 0.333984375 |

Each uses the same conservative 5% statistical bound. Tests also check the
simulation-step floor and legacy direct-API `neB=0` fallback against the exact
explicit-interval forward stream. Global `.NEB 0` now fails cleanly; it previously
aborted with `std::bad_optional_access` when thermal noise was enabled.

## Active thermal amplitude

Two independent implementation defects remained after the earlier stored-source
repair: transition-region amplitude depended on the preceding resistive state,
and already scaled interpolation knots retained old amplitudes. In addition,
the solver sampled thermal noise before selecting the current step's slope.

White sources now retain unit-ASD knots and multiply the current amplitude
after interpolation. Tests change amplitude 1→3→0→3 at the same time: the same
realization rescales immediately and returns exactly zero when disabled.
Rescaling draws no new random value. Junction updates select all three
amplitudes before applying a single Norton sample to the physical terminals.

The explicit classical model is \(S_I=4k_BT G\), using the piecewise resistive
slope: \(1/R_0\) below the gap, \(I_c/(ICFCT\,\Delta V)\) in transition, and
\(1/R_N\) above the gap. This extends equilibrium resistor noise locally; it is
not a microscopic theory of biased quasiparticles or transition-region shot
noise. The [noise guide](../noise_sources.md) states its physical limits and
branch-current trace meaning. No constants or deterministic device equations
are changed.

Direct tests visit subgap→transition→normal→transition→subgap in both polarities.
A prescribed-voltage circuit independently selects each expected slope from
its analytic PWL voltage and the existing explicit voltage predictor. KCL
extracts the applied Norton current as `-(I(V1)+I(B1))`; `I(B1)` alone excludes
that internal parallel source. Both analysis modes and positive-ground,
ground-negative and two-non-ground terminals are checked. The six cases have
maximum absolute discrepancy **5.114e-13 A**. The per-sample allowance is
1 ppm of the independent SI noise value plus 1e-14 A, covering the retained
legacy Boltzmann constant's rounding and linear-solve roundoff. The reference
uses the forward unit source only to recover the fixed seeded realization;
expected temperature, conductance, voltage and terminal signs are independent.

Full-cycle timestep retry with thermal, current and phase noise must replay a
fresh same-process run at the final step bit-for-bit. Invalid API intervals,
temperatures/resistances and clean CLI failure are also covered. At T=0 this
classical source vanishes; zero conductance also gives zero noise.

## Compatibility and verification

`Noise::determine_sampling_interval` adds a static public method without changing
object layouts or existing signatures. Internal white interpolation state now
stores unit-ASD values. Ordinary connected explicit-interval sources retain draw
ordering; floating-point operation order can change last bits. A junction whose
two terminals are ground now advances its thermal generator once per step too,
which changes subsequent global-stream draws. Correcting NEB deliberately
changes omitted-interval and automatic thermal seeded waveforms. Changes in
timestep, sampling or solver retry may also change noise timing; cross-build
bit-identical output is not promised. The default NEB remains the existing
1 THz; stale 1 GHz documentation is corrected.

```bash
ctest --test-dir build/dev -R 'regression::(noise|cli|installed_package)' \
  --output-on-failure --no-tests=error
```

GCC16/Clang23 Debug and Release pass all 33 CTest groups. The complete
ASan/UBSan suite with leak detection passes 33/33 in 198.80 s. Native
qualification remains pending for the implementing revision. Exact red decks, binary hashes and
failed assertions are preserved under `build/audit-resolution/stochastic/`.
Compact green measurements, source hashes and qualification receipts are under
`artifacts/audit-resolution/white-thermal-noise/`. The native group also checks
the new method from a fresh installed-library consumer.
