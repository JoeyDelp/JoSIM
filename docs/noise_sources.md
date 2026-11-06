# Noise sources

JoSIM provides Gaussian white noise and an approximation to power-law noise.
Use `.option seed=integer` to repeat a run in a fresh simulator process.

## White noise

```spice
I1 out 0 NOISE(1p 0 1p)
```

`NOISE(ASD TD TSTEP)` takes an amplitude spectral density in A/sqrt(Hz) for a
current source, or V/sqrt(Hz) for a voltage source. `TD` defaults to zero.
`TSTEP` sets the noise update interval. If omitted, it is derived from `.NEB`
as `1/(2*NEB)`. The interval is limited to at least the simulation timestep.

Independent Gaussian values at the update points have variance
`ASD^2/(2*TSTEP)`. Values between updates are linearly interpolated. This
interpolation changes the spectrum near the update frequency; the waveform is
not ideal white noise with a rectangular frequency cutoff. Match the timestep
and bandwidth to the circuit and check convergence when measuring noise.

`I(source)` output uses the samples actually applied by the simulator. Printing
does not re-evaluate a noise generator at past times. Source differences and
output FIR filtering use the same captured history. Histories are retained
only when requested, at about 8 bytes per internal sample per source, plus
vector metadata. Output decimation does not reduce that retained internal data.

Automatic resistor and junction thermal noise use the same update interval
convention. Junction thermal-noise amplitude follows its resistive state.

## Power-law noise

```spice
I1 out 0 PNOISE(1p 1 0 1u 1 64)
```

`PNOISE(A1 ALPHA TD TSTEP TCORR N)` approximates the one-sided power spectral
density `A1^2/f^ALPHA`. `A1` is the amplitude normalization at 1 Hz, with the
source's units per sqrt(Hz). The implementation sums independent
Ornstein–Uhlenbeck processes with logarithmically spaced relaxation times.

- `ALPHA` is between 0 and 2; 1 gives a 1/f approximation.
- `TD` defaults to zero; output before the delay is zero.
- `TSTEP` defaults to the simulation timestep and cannot be smaller than it.
- `TCORR` defaults to the simulation stop time; zero selects that default.
- `N` defaults to 48 and must be an integer from 4 to 512.

The nominal band is `1/TCORR` to `1/(2*TSTEP)`. It must have positive width.
Its edges have smooth rolloffs, with power outside the nominal band. The total
variance therefore differs from the integral of an ideal power law cut off
sharply at those edges.

The continuous process bank is normalized at 1 Hz when that frequency is in
the nominal band. Otherwise it is normalized at the nearest band edge using
the extrapolated 1 Hz power law. Moving the band does not reinterpret `A1` as
the amplitude at a different reference frequency. Sampling and interpolation
still affect measured spectra near the band edges.

## Flux noise

```spice
PFLUX loop 0 FNOISE(1u 1 0.25 0 1u 1 64)
```

`FNOISE(A1 ALPHA PHIDC TD TSTEP TCORR N)` uses the same process bank. `A1` is in
flux quanta per sqrt(Hz); `PHIDC` is a constant flux bias in flux quanta. The
returned phase is `2*pi*(PHIDC + noise)` radians. The DC bias remains present
before `TD`. Use this source with a circuit representation that relates phase
to the applied loop flux; it does not introduce a separate magnetic-field solver.

## Critical-current fluctuations

```spice
B1 out 0 junction ICNOISE=PNOISE(1u 1 0 1u 1 64)
```

`ICNOISE=` supplies a dimensionless fractional fluctuation, `delta(t)`, to the
Josephson current. The effective critical current is
`Ic * gate_factor * (1 + delta(t))`. Capacitance and shunt resistance are
unchanged. Fluctuations below -0.99 are clipped, retaining at least 1% of the
nominal critical current before gate suppression. Use small fluctuations when
the Gaussian model is intended to represent perturbations about a measured Ic.

The standalone function name `ICNOISE(...)` has the same arguments and waveform
as `PNOISE(...)`. The circuit attribute above also accepts that function name.
