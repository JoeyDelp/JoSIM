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
internal timesteps and computed sample counts are recorded.
Source templates, includes, custom waveform files and existing result directories
are preserved. Relative includes/custom files resolve from the template context.
Include/custom filenames must be fixed; continuations must stay within a file.

Remove `.FILE` and `.IV` controls when using this isolated workflow. The helper
checks reachable includes and rejects those controls before running anything;
it does not silently redirect or discard them. Custom files and netlists must
remain unchanged throughout the sweep. Simulator errors/timeouts stop the sweep,
retain the logs, and do not reuse an earlier point's CSV.
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
difference comes from the finite measurement window and timestep. Zero bias
gives zero voltage. To study hysteresis in an underdamped circuit, use a continuous
bias ramp so that the circuit retains its state between bias values.

The mean is a time-weighted trapezoidal integral over the explicit window, with
linear interpolation at its boundaries. This handles nonuniform output samples;
it does not use row-count averages or extrapolate beyond available data. Choose
settling time, duration and timestep for the circuit, and check refinement. A
fixed seed in the template reuses a controlled stream at each fresh point;
independent noise realizations require explicitly different seeds.

The built-in `.IV` uses a single-junction transient testbench. Its defaults are
0/10/50 ps bias knots, 0.05 ps initial internal step, 500 ps duration and mean
voltage over the second half. Returning branches first visit +/-Imax. Every
point starts from fresh zero-state device histories with its specified ramp;
it does not carry the preceding point's final state. It does not sweep an
arbitrary circuit or provide conventional DC/Newton tolerance control.

Use `TSTEP`, `TSTOP` and `TSTART` to refine the initial internal step, run duration
and averaging start, all in seconds. Defaults remain 0.05 ps, 500 ps and half
the selected stop time. The averaging start must be at least 50 ps, when the
generated source reaches its final bias, and before the stop time. At least two
actual averaging samples must remain. The same timing applies to every point.
For example, a slow overdamped model can need a much longer plateau:

```spice
.model SLOW JJ(RTYPE=0 ICRIT=100u RN=.001 CAP=0)
.iv SLOW 150u slow-iv.csv STEP=150u TSTEP=20p TSTOP=5u TSTART=1u
```

The mean uses unfiltered internal samples at or after `TSTART`; the stop time
itself is not an extra sample. Timestep retries may reduce `TSTEP` and never
increase it within the curve. `-V 1` reports the requested protocol and each
point's effective timestep and averaging sample count.

In this example, the Josephson period is about 18.5 ns: the default 500 ps run
does not estimate its DC mean accurately. The longer protocol gives 111.7475 nV
against the independent RSJ mean 111.8034 nV; halving the step changes it by
0.0002 nV. Choose the settling and measurement duration for the model, then
check that reducing the timestep and extending the duration give stable results.
Use an explicit circuit transient for a different preparation waveform or a
continuous hysteresis curve.

The [numerical methods guide](numerics.md) explains timestep retries and how to
check convergence.

## Retired prototype

`scripts/ivcurve.py` is retired. Use `.IV` for a single-junction model curve,
or the circuit-sweep helper above for a complete circuit.
