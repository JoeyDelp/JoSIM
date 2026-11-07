# Junction startup history — 6 November 2026

An independent investigation of #101 exposed a startup defect separate from
its reported bipolar reverse response. The defect affects junction phase and
voltage histories during the negative-time startup interval and at t=0.

## Independent equilibrium

Use the supplied buffer topology with VIN held at zero, `Igate=20u`,
`Ic1=2m`, `Ic2=1.5m`, `L1=L2=0.125p`, and the supplied finite-capacitance
junction models. Start from zero fields and integrate the bias transient.
At equilibrium, the output resistor has zero voltage and IL1 is zero. KCL gives

\[
I_{J2}=s,\qquad I_{J1}=-s,\qquad I_{L2}=s-I_g.
\]

Since the prescribed voltage at N4 is always zero, zero initial loop flux
requires

\[
\sigma(\phi_1-\phi_2)=L_2I_{L2},\qquad
\phi_1=\arcsin(-s/I_{c1}),\qquad\phi_2=\arcsin(s/I_{c2}).
\]

The regression solves this scalar monotonic equation by bisection on
`0 <= s <= Igate`, independently of JoSIM. It uses exact SI
`sigma=h/(4*pi*e)`. The result is

| Quantity | Independent value |
|---|---:|
| phi1 | -0.00245600305913 rad |
| phi2 | 0.00327467330605 rad |
| IL2 | -15.0879988199 uA |
| IJ2 = -IJ1 | 4.91200118010 uA |

Cold voltage mode and phase mode with either startup choice agree with this
state. Old warm voltage mode instead produces nearly zero junction phases and
`IL2=-20uA`, leaving a flux-linkage residual equivalent to 0.00759634 rad.
The error persists as the timestep decreases.

## Cause and repair

`Simulation::handle_jj` read the previous solved phase/voltage only when `i>0`.
During negative startup and at i=0, voltage mode used node **volts** as phase
history and retained zero voltage history. These are different state variables
with different units. Read the previous solved values on every step. Cold
runs still begin with the existing zero-initialized state.

The patch changes no public signatures, class layouts or physical constants.
It corrects the initial state inherited by the subsequent transient. Circuits
that depended on the old erroneous startup state can produce different results.

## Checks and evidence

Eight independent equilibrium cases cover both modes, startup enabled/disabled,
and 0.02/0.01 ps steps. Assertions allow 1e-8 rad for each phase and flux linkage,
2e-11 A for current, and 1e-10 V for settled output voltage. These include CSV
rounding and the small legacy-versus-SI constant difference.

GCC16/Clang23 Debug and Release pass all 32 CTest tests. The complete
ASan/UBSan suite with leak detection passes 32/32 in 182.98 s. All ten native
binary/library jobs pass in [CI 42238371973](https://github.com/JoeyDelp/JoSIM/actions/runs/42238371973),
attempt 2. The first attempt's macOS arm64 library job never obtained a runner
and reported GitHub capacity/internal-server errors; the retry passed.
The #101 independent transient probe now agrees between modes
and improves with mesh refinement; its full scientific qualification is separate.

```bash
ctest --test-dir build/dev -R regression::numerics --output-on-failure --no-tests=error
python3 -B test/regression_numerics.py build/dev/josim-cli --report artifacts/numerics.json
```

Detailed evidence: `artifacts/audit-resolution/jj-startup.json`, compiler and
sanitizer JUnit/logs under `build/audit-resolution/jj-startup-*`, and the compact
red records under `build/audit-resolution/issue-101/`. The original large
transient CSV attempts were regenerated during the green study; their failure
measurements, exact circuit, static-flux witness and source history are preserved.
The final qualification will publish compact evidence durably.
