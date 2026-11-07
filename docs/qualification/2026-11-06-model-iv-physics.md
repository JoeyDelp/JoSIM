# Model-IV state and timing qualification — 6 November 2026

This study resolves F23 and extends the gate/model-IV review. The separate
[retry study](2026-11-06-iv-retry.md) repairs bias loss during timestep reduction.
Here the two concerns are initial-state contamination and an unconfigurable
finite averaging protocol. The result remains a transient model testbench,
with explicit preparation and measurement settings.

## Initial state

For an ordinary sinusoidal RSJ junction, the constitutive equations are odd
under \((I,\phi,V)\mapsto(-I,-\phi,-V)\). Starting from the same zero state with
sign-reversed bias ramps must therefore give sign-reversed voltage traces,
including before a DC average has converged.

The original model-IV owner reused one Matrix across distinct Simulation
objects. Simulation reset its solution vector, but per-device histories remained
from the preceding point. For `ICrit=100µA`, `RN=1mΩ`, `CAP=0`, `RTYPE=0` and
paired `±150µA` excursions, the default protocol produced `+135.1658 nV` and
`−138.4165 nV`: an odd-symmetry residual of `−3.2507 nV`. Returning zero-current
branches also had a `1.385687 nV` symmetry residual.

Changing only matrix ownership—rebuilding it from the point's declared ramp—
gave `±135.1658 nV` and opposite `∓1.023181 nV` return means. Both symmetry
residuals are zero at CSV precision. Each point now starts with fresh device
histories. Its Input declaration also remains synchronized for retries. This
isolates the cause from steady-state averaging accuracy; no junction equation
or physical constant changed.

## Timing controls and support boundary

`.IV` now accepts `TSTEP`, `TSTOP` and `TSTART`, in seconds, for the initial
internal step, run duration and averaging start. Defaults retain 0.05 ps,
500 ps and half the selected duration. The 0/10/50 ps preparation knots remain:
outward points ramp from zero to the point, while returning points first visit
the corresponding `±Imax`. Points do not carry the preceding final state.

Require finite `TSTEP > 0`, `TSTOP > TSTART >= 50 ps`, a supported requested
grid and at least two actual averaging samples. The mean uses unfiltered
internal samples at/after `TSTART`; the stop time itself is not an extra sample.
The phase guard may reduce the step, which never increases within the curve.
Verbose level 1 reports requested timing and each point's actual step/count.

Existing public signatures and class layouts remain. Direct `do_simulate`
callers with default `prstart=0` retain the latter-half convention; a positive
`prstart` now selects the averaging start. One-sample averages fail explicitly.
Fresh state changes the incorrect historical finite-window results. The CSV
format, current order/endpoints and ramp definitions remain.

The finite protocol has no universal DC-accuracy guarantee. A longer duration
can reduce settling and finite-cycle error; a smaller step checks discretization.
These controls do not add Newton iteration, adaptive error tolerances or
continuous hysteresis. Use an explicit circuit transient for a different
preparation history. [The sweep guide](../sweeps.md) gives the command and limits.

## Independent slow-RSJ/gate reference

For `C=0`, constant resistance, sinusoidal CPR and `|I|>|Ic,eff|`, the RSJ mean is

\[
\overline V=\operatorname{sgn}(I)R_N\sqrt{I^2-I_{c,\mathrm{eff}}^2},\qquad
I_{c,\mathrm{eff}}=I_c\max(0,1-|V_g|/GWIDTH).
\]

With `I=150µA`, `Ic=100µA` and `RN=1mΩ`, the ungated reference is
`111.8033989 nV`, with Josephson period about `18.5 ns`. The short 500 ps default
gives `135.1658 nV`, **20.90%** above the DC mean even after state repair. A
user previously had no model-IV timing controls with which to refine it.

Use `TSTEP=20p TSTOP=5u TSTART=1u`: the source is on its final plateau long
before averaging. The independently expected and measured positive means are:

| Gate, GWIDTH=10mV | Effective Ic | RSJ mean | JoSIM mean | Relative difference |
|---|---:|---:|---:|---:|
| Ungated or 0mV | 100µA | 111.8034 nV | 111.7475 nV | −0.0500% |
| +5mV or −5mV | 50µA | 141.4214 nV | 141.4591 nV | +0.0267% |
| 10mV | 0 | 150 nV | 150 nV | 0 |

Negative excursions and both zero-current return rows are also checked. The
positive/negative gate curves agree at CSV precision. Ordinary and zero-gate
curves agree. Both caller analysis modes are represented; model IV itself uses
voltage analysis, as before.

The running RSJ solution's periodic phase correction spans less than `2π`.
Its continuous finite-window voltage mean therefore differs from the long-run
mean by at most `Phi0/W`, where `W` is the averaging interval. Here
`W≈4µs`, giving about `0.517 nV`. Add a predeclared `0.1 nV` allowance for
discrete stepping/sample averaging: the resulting bound is about `0.617 nV`.
This is independent of the measured mean; it does not fit the observed error.

Halving the ungated step from 20 ps to 10 ps changes the mean by only
`2e-13 V = 0.0002 nV`, well below the separate `0.1 nV` discretization allowance.
The observed effective steps are exactly the requested values, with 200,000
or 400,000 averaging samples. An eight-run campaign includes two short-protocol
checks and six long/refined/gated runs, recording every row, actual seed,
timing/count, source/binary hashes and unchanged identities.

## Regression and evidence

`regression::gate` has 16 methods. New checks cover short-protocol sign symmetry,
six independent long/refined/gated curves with metadata, and invalid timing
including empty/one-sample windows. Its existing gate-temperature, waveform,
scope, node, endpoint and repeated-retry assertions remain.

All four local GCC 16.2.1/Clang 23.1.4 Debug/Release configurations pass **34/34**
CTest groups. Full Clang Debug ASan/UBSan and leak detection pass **34/34 in
237.00 s**, including the expanded 16-method gate group in 31.19 s. Native
qualification remains required for the pushed revision; it is not inferred
from these local checks.

```bash
ctest --test-dir build/dev -R 'regression::gate$' --output-on-failure
```

Detailed evidence:

- `build/audit-resolution/iv-physics/state-{red,green}.json`: one-variable
  state-reset experiment with exact source/binary hashes and symmetry residuals.
- `artifacts/audit-resolution/iv-physics/timing/report.json`: eight-run campaign;
  adjacent per-case decks, CSV, logs and `qualification-owner.py` preserve the
  complete method. Run the archived owner from the source checkout's root.
- `artifacts/audit-resolution/iv-physics/`: compact state/retry receipts and
  per-configuration build/test/sanitizer results; detailed logs remain under
  `build/audit-resolution/iv-physics/`.

The campaign source and binary stayed unchanged during execution. Native tests
need only Python's standard library; these expectations do not depend on the
optional plotting/scientific packages. The measured physical reference is a
deterministic overdamped RSJ limit, not a proof of convergence for arbitrary
capacitive, gap-transition, noisy or nonequilibrium models.
