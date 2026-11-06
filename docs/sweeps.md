# Parameter sweeps and circuit IV curves

`.PARAM` defines a value for one transient analysis. It does not declare a
range or automatically create a Cartesian parameter sweep (#84). Use separate
processes for independent points; the supplied helper handles one numeric
template marker and an explicit list of values.

## Five independent resistance points

`test/param/sweep_resistance.cir.in` defines `RX=@VALUE@`. A 1 A current source
drives RX, giving the independent check \(V=IR\). Run from the repository root:

```bash
python3 -B tools/circuit_sweep.py \
  --binary build/dev/josim-cli \
  --template test/param/sweep_resistance.cir.in \
  --values 1,2,3,4,5 --trace 'V(N)' \
  --start 1e-11 --stop 1.8e-11 --output artifacts/resistance-sweep
```

Values are finite SI numbers; times are seconds. The template can use the marker
in a parameter or component expression. The helper replaces its occurrences in
the main template and starts a fresh simulator process for each point.
`--analysis phase` selects phase mode; the default is voltage mode.

The output directory must be new. It contains a summary `sweep.csv`, a JSON
report, and each point's effective netlist, log and raw traces. Effective seeds,
internal timesteps, computed counts and source/binary/output hashes are recorded.
Source templates, includes, custom waveform files and existing result directories
are preserved. Relative includes/custom files resolve from the template context.
Include/custom filenames must be fixed; continuations must stay within a file.

Remove `.FILE` and `.IV` controls when using this isolated workflow. The helper
checks reachable includes and rejects those controls before running anything;
it does not silently redirect or discard them. Custom files and netlists must
remain unchanged throughout the sweep. Simulator errors/timeouts stop the sweep,
retain a failed report and witnesses, and never reuse an earlier point's CSV.
This helper uses Python's standard library; plotting dependencies are optional.

## Circuit IV versus model IV

These workflows have different physical histories:

| Workflow | What is simulated |
|---|---|
| `.IV model Imax file STEP=...` | A built-in single-junction model testbench |
| External fresh-point sweep | The complete supplied circuit, restarted at each point |
| One transient with a PWL bias ramp | The complete circuit with state carried along that ramp |

For an independently checkable circuit example:

```bash
python3 -B tools/circuit_sweep.py \
  --binary build/dev/josim-cli \
  --template test/param/sweep_squid_bias.cir.in \
  --values=-0.0002,0,0.0002 --trace 'V(N)' \
  --start 5e-10 --stop 9.9e-10 --output artifacts/squid-iv-sweep
```

This sweeps the total current through a complete zero-inductance two-junction
SQUID at fixed external flux. In the overdamped limit, \(I_c^{eff}=\sqrt2\,100\)
uA and \(R_{eff}=8\) ohms. For \(|I_b|>I_c^{eff}\),

\[
\langle V\rangle=\operatorname{sgn}(I_b)R_{eff}
 \sqrt{I_b^2-(I_c^{eff})^2}.
\]

At +/-200 uA the reference is +/-1.13137085 mV. Both analysis modes give
+/-1.13153468 mV with the supplied finite measurement window. The 0.015%
difference is below the 1% budget, which includes finite endpoint-phase and
timestep error. Zero bias gives zero voltage. This example does not imply that
fresh points reproduce hysteresis from a continuously swept underdamped circuit.

The mean is a time-weighted trapezoidal integral over the explicit window, with
linear interpolation at its boundaries. This handles nonuniform output samples;
it does not use row-count averages or extrapolate beyond available data. Choose
settling time, duration and timestep for the circuit, and check refinement. A
fixed seed in the template reuses a controlled stream at each fresh point;
independent noise realizations require explicitly different seeds.

The built-in `.IV` uses a fixed single-junction transient protocol: 0/10/50 ps
bias knots, 0.05 ps internal step, 500 ps duration and mean voltage over the
second half. Returning branches first visit +/-Imax. It does not sweep an
arbitrary circuit and does not provide conventional DC/Newton tolerance control.
The generated bias declaration is kept in sync with each point's active source,
so a timestep retry retains the requested point rather than restoring a stale
testbench current.
Use explicit circuit transients when the required settling/measurement protocol
differs. The timestep-restart guard alone does not guarantee numerical accuracy.

The main transient solver uses BDF2 with nonlinear junction terms predicted
from prior samples and one sparse linear solve per accepted step. It does not
iterate Newton within a step or expose SPICE RELTOL/ABSTOL error control. After
the initial 1% of the positive-time grid, a predicted junction phase increment
above 0.2 cycles triggers a complete restart at half the internal step. It
never increases that step; the final run has a uniform internal grid. This
phase-advance guard is not a local truncation-error bound. Explicit timestep
and measurement-window convergence checks remain necessary.

## Retired prototype

`scripts/ivcurve.py` is retired (#82). Its positional netlist edits overwrote
existing temporary files, ignored simulator failures and reused stale CSVs. It
now exits with an explanation and changes no files. Use `.IV` for its model
testbench purpose, or the fresh circuit workflow above. The
[qualification study](qualification/2026-11-06-sweeps.md) records tests and limits.
