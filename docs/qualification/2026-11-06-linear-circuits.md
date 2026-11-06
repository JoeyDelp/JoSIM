# Linear-circuit qualification — 6 November 2026

This intermediate study covers linear R/L/C dynamics. It does not close the
junction, long-run or stochastic investigations in the audit-resolution ledger.
Simulation source is `121ca3e`, with the numerical test owner added by this
change. The generated report records the exact executable and test SHA-256.

## Independent references

The drive is a linear voltage ramp, `vin(t) = a*t`, with `a = 1e7 V/s`.
Each circuit contains `R = 10 ohm`; where present, `L = 100 pH` and `C = 1 pF`.
Initial capacitor voltage and inductor current are zero. The ramp begins at
zero and extends beyond the 120 ps run, avoiding a later forcing discontinuity.
The source remains zero during negative-time startup.

For RC, with `tau = R*C = 10 ps`:

\[
v_C = a(t-\tau+\tau e^{-t/\tau}),\qquad
 i_C = aC(1-e^{-t/\tau}).
\]

For RL, with `tau = L/R = 10 ps`:

\[
i_L = \frac{a}{R}(t-\tau+\tau e^{-t/\tau}),\qquad
 v_L = a\tau(1-e^{-t/\tau}).
\]

For the series RLC circuit:

\[
LCv_C''+RCv_C'+v_C=at,\qquad v_C(0)=v_C'(0)=0.
\]

Its reference uses the two roots of `LC*r^2 + RC*r + 1 = 0`:

\[
v_C=a(t-RC)+c_1e^{r_1t}+c_2e^{r_2t},\quad
c_1=\frac{-a-r_2aRC}{r_1-r_2},\quad c_2=aRC-c_1,\quad i_C=Cv_C'.
\]

These formulas follow the continuous circuit equations. They use no JoSIM
constants, discretization coefficients, or generated expected waveforms.

## Checks and tolerances

`test/regression_numerics.py` exercises 36 cases: three circuits, two analysis
modes, startup enabled/disabled, and timesteps of 1, 0.5 and 0.25 ps. It checks
all samples, finite values, the ideal drive, Ohm's law, KCL and KVL. Mode
comparisons use the same physical traces and time grid.

The voltage-error bound is `a*h^2/(10 ps) + 2e-9 V`; the current bound divides
the discretization term by 10 ohm and allows `2e-10 A` for output rounding.
These bounds scale with the circuit timescale and timestep squared. Each
refinement must also reduce both maximum errors by a factor below 0.4; a fixed
error that merely falls below the absolute bound cannot pass. Conservation and
mode checks allow `3e-6` relative for the seven-significant-digit CSV output,
with small absolute floors near zero.

The observed whole-interval convergence order is 1.75–1.84 for RC/RL and
1.99–2.00 for RLC on these grids. These are measured finite-grid results,
including startup and CSV rounding, rather than a general order guarantee.

| Circuit | Maximum voltage error at 0.25 ps | Maximum current error at 0.25 ps |
|---|---:|---:|
| RC | 13.1534 nV | 1.31534 nA |
| RL | 13.1534 nV | 1.31534 nA |
| RLC | 14.0995 nV | 1.43369 nA |

Both modes and startup choices give these values at CSV precision. Maximum
KCL residual is zero at CSV precision; maximum KVL residual is `5.00001e-10 V`.

## Reproduction and scope

```bash
ctest --test-dir build/dev -R regression::numerics --output-on-failure --no-tests=error
python3 -B test/regression_numerics.py build/dev/josim-cli --report artifacts/rlc.json
```

Local GCC16/Clang23 Debug and Release checks pass. ASan/UBSan with leak detection
passes the same 36 cases in 0.60 s. Native-platform results are recorded in the
ledger as they complete. Release and installed-library
qualification owners require this test group alongside their other checks.

Local detailed evidence: `artifacts/audit-resolution/rlc.json` and
`build/audit-resolution/linear-numerics-*.log`/`.xml`. The final qualification
will publish compact machine-readable evidence durably.

This study covers zero-state linear dynamics, including the startup path when
the source is zero before t=0. It does not establish arbitrary initial
conditions, nonzero precharge, junction switching, phase accumulation over long
runs, or reports #91/#101. Those remain separate acceptance requirements.
