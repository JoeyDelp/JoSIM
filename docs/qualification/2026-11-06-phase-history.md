# Phase-source and output histories — 6 November 2026

Three related supported-behaviour defects were reproduced before changing code:

| Case | Before repair | After repair |
|---|---|---|
| Warm 0.1 mV JJ bias, 0.01 ps step | Node phase loses about 3.03854 rad of startup history | Node/JJ phase and both modes agree at CSV precision |
| Warm constant one-radian phase source, 0.1 ps step | False 4.93659 mV pulse at t=0 | Zero voltage/current from t=0 |
| Seeded FNOISE phase source, cold 0.1 ps step | Mode differences up to 0.00332625 rad and 1.90786 uA | Both phase and resistor current agree at CSV precision |

The first case uses `V1 N 0 DC 0.1m`, an RTYPE=0 junction with IC=100u,
RN=10 and CAP=0.1p, and prints `P(N) P(B1)`. Its first node/JJ phases were
0.00202569/3.040054 rad. Both now print 3.040054 rad. The noise case uses
`P1 N 0 FNOISE(0.001 1 0 0 1p 100p 16)`, a 10-ohm resistor and seed 123.
Exact decks, red measurements and executable hash remain in
`build/audit-resolution/phase-source/red.json` and `constant-red.json`.

## Cause and owning repair

JoSIM uses the BDF2 Josephson relation

\[
v_n=\frac{\sigma}{h}\left(\frac32\phi_n-2\phi_{n-1}
+\frac12\phi_{n-2}\right),\qquad
\phi_n=\frac{2h}{3\sigma}v_n+\frac43\phi_{n-1}-\frac13\phi_{n-2}.
\]

Here h is the simulation step and sigma is the existing Josephson scale.
`Output::write_output` integrated voltage from zero histories even after
negative startup. It now starts from the actual retained startup phase.
Integration still visits every internal sample before FIR filtering and
downsampling; the defect was not decimation.

`Simulation::handle_ps` evaluated the present source value, then asked the
already advanced generator for values at t-h and t-2h. Stateful noise generators
cannot reconstruct these past samples. It also replaced negative-time history
with zeros at i=0/1. Each phase source now evaluates once per step and retains
its preceding two applied values. These histories continue through t=0.

The existing `Results::phaseBefore` map now supports both modes. During negative
startup, phase mode retains solved nodal phases; voltage mode integrates solved
nodal voltages. Both begin from the existing zero state before the first step.
Explicit trace selection retains only nodes needing output conversion, including
difference operands. Default all-unknown results retain all nodes for later
library postprocessing. Storage is two doubles per retained node plus map
overhead, independent of simulation length. Full retries reconstruct source
objects and clear all result history.

Cold startup remains unchanged: a one-radian phase step produces voltage
samples `(3/2, -1/2)*sigma/h`, then zero, with total discrete area sigma.
For warm startup that impulse occurs at the beginning of the negative-time
settling interval, rather than being injected again at t=0.

No physical constants, signatures or output column formats change. Appended
`PhaseSource` state changes component/Matrix layouts; consumers must recompile
against matching headers/libraries. Corrected warmup/noisy phase-source results
can differ from old output. The [API guide](../API.md) defines the history scope.

## Regression protection

- CLI checks cover warm/cold startup, one-point output, delayed printing,
  FIR/downsampling, floating terminals and phase-difference operands.
- Constant-source assertions use dphi/dt=0 after warmup and independently
  calculated SI pulse area for cold startup. CSV bounds are 2 ppm plus a
  1 pV voltage floor; phase-mode/node comparisons use 3 ppm plus 1 nrad.
- Raw C++ checks cover SIN, PWL, NOISE, PNOISE and FNOISE in both modes with
  startup on/off. A standalone generator is sampled only forward in time;
  prescribed phase and its documented BDF2 voltage/current are checked before
  output conversion, with 1e-11 rad and 1e-15 A absolute bounds. These test
  source-sample handling, not the statistical validity of noise models.
- The same-process timestep-retry test includes FNOISE. All selected raw results
  and retained histories must exactly match a fresh run at the accepted step
  and seed. Deferred library output must agree with direct phase-mode results;
  unrequested converted histories must remain absent.

The three new CLI methods failed in 21 subcases before repair. GCC16/Clang23
Debug and Release now pass 33/33 CTest groups. Full ASan/UBSan with leak detection
passes 33/33 in 212.16 s. Compiler warnings remain limited to two unused helpers
in the vendored spline header. All ten native binary/library jobs pass for descendant [7cbea62036df](https://github.com/JoeyDelp/JoSIM/commit/7cbea62036df6e0c7a1e76180c894cf26fb23be5) in [CI 42255843921](https://github.com/JoeyDelp/JoSIM/actions/runs/42255843921).

```bash
ctest --test-dir build/dev -R 'regression::(cli|noise|library)' \
  --output-on-failure --no-tests=error
```

Green measurements and executable/source-diff hashes:
`artifacts/audit-resolution/phase-source/measurements.json`.
Red tests, candidate patch, full compiler receipts and sanitizer JUnit/logs:
`build/audit-resolution/phase-source/` and `build/audit-resolution/current/`.
An intermediate build's configure-time version string does not identify its
uncommitted patch; final qualification requires a clean exact-source build.
