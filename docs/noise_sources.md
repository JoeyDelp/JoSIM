# Noise sources

JoSIM provides Gaussian white noise and an approximation to power-law noise.
Use `.option seed=integer` to repeat a run in a fresh simulator process.
For worked circuits and measured setting choices, see
[floating flux lines and SQUID noise](squid_noise.md).

## White noise

```spice
I1 out 0 NOISE(1p 0 1p)
```

`NOISE(ASD TD TSTEP)` takes an amplitude spectral density in A/sqrt(Hz) for a
current source, or V/sqrt(Hz) for a voltage source. `TD` defaults to zero.
`TSTEP` sets the noise update interval, denoted by \(\Delta\). If omitted, it
is derived from `.NEB` as `1/(3*NEB)`. The interval is limited to at least the
internal simulation timestep \(h\). The default `.NEB` is 1 THz; `.NEB` must
be finite and positive. An explicit `TSTEP` overrides `.NEB` for that source.

Independent Gaussian values at the update points have variance
`ASD^2/(2*TSTEP)`. Values between updates are linearly interpolated. After the
initial ramp, the time-averaged one-sided spectrum and total variance are

\[
S(f)=ASD^2\operatorname{sinc}^4(f\Delta),\qquad
\overline{\operatorname{var}(x)}=\frac{ASD^2}{3\Delta},\qquad
\operatorname{sinc}(z)=\frac{\sin(\pi z)}{\pi z}.
\]

Thus the effective bandwidth is \(1/(3\Delta)\), with smooth rolloff rather
than a rectangular cutoff. When the timestep floor applies, it becomes
\(\min(NEB,1/(3h))\). Older versions used \(\Delta=1/(2NEB)\), giving only
two-thirds of the requested time-averaged noise power. This correction changes
seeded thermal and omitted-`TSTEP` white-noise waveforms. Explicit-`TSTEP` grid
variance is preserved, up to rounding. Each junction thermal source is evaluated
once per step; a source whose two terminals are ground now advances too, which
can change later draws in the process-global stream.

Samples exactly at the update points have variance `ASD^2/(2*TSTEP)`;
oversampling approaches the smaller time-averaged variance above. For \(K\)
uniformly spaced samples per interval, starting at an update point, the expected
sample variance is \(ASD^2(2/3+1/(3K^2))/(2\Delta)\). Setting \(h=\Delta\)
therefore does not establish noise accuracy. Resolve circuit dynamics and the
interpolated waveform, discard startup, and check timestep refinement. Output
sampling and FIR filtering are separate settings: downsampling can alias power
outside the output Nyquist band. Keep the seed, bandwidth and sampling choices
in the measurement record.

`I(source)` output uses the samples actually applied by the simulator. Printing
does not re-evaluate a noise generator at past times. Source differences and
output FIR filtering use the same captured history. Histories are retained
only when requested, at about 8 bytes per internal sample per source, plus
vector metadata. Output decimation does not reduce that retained internal data.

## Automatic thermal noise

`.TEMP T` enables a parallel Norton current source for each resistor and
junction. A component's `TEMP=` overrides the global temperature; `TEMP=0`
disables its classical noise. Temperatures must be finite and non-negative.
Component `NEB=` overrides the global bandwidth. Both thermal source types use
the white-noise interval convention above.

The one-sided current ASD is \(\sqrt{4k_BTG}\). For a resistor, \(G=1/R\).
For an RTYPE=1 junction, the active piecewise resistive slope selects

\[
G=\begin{cases}
1/R_0,& |V_{pred}|<V_G-\Delta V/2,\\
I_c/(ICFCT\,\Delta V),& V_G-\Delta V/2\le |V_{pred}|<V_G+\Delta V/2,\\
1/R_N,& |V_{pred}|\ge V_G+\Delta V/2.
\end{cases}
\]

RTYPE=0 uses \(1/R_N\) throughout. Here \(V_{pred}\) is the existing explicit
voltage predictor used to choose the step's linearization. The current sample
uses that step's slope immediately, including on entry to and exit from the
transition region. Positive temperature requires positive conductance; zero
conductance contributes zero noise.

The resistor relation follows classical equilibrium
[Johnson–Nyquist noise](https://doi.org/10.1103/PhysRev.32.110). Applying it to
each junction slope is a **local-equilibrium approximation**. It does not model
quasiparticle shot noise, quantum noise, heating or nonequilibrium gap physics.
The transition-region choice is an explicit model assumption, not a microscopic
prediction of switching rates. Classical noise also requires relevant frequencies
well below \(k_BT/h_{Planck}\).

The Norton source enters node KCL separately. `I(R)` and `I(B)` report the
constitutive device branch current, excluding its internal noise source. For
example, with an ideal voltage source and one noisy junction across the same
terminals, `-(I(Vdrive)+I(B))` recovers the injected thermal current. There is no
separate named trace for an internal thermal source.

## Power-law noise

```spice
I1 out 0 PNOISE(1p 1 0 1u 1 64)
```

`PNOISE(A1 ALPHA TD TSTEP TCORR N)` approximates the one-sided power spectral
density `A1^2/f^ALPHA`, with frequency expressed in Hz. `A1` specifies the
**ideal interior power law's** amplitude at 1 Hz, in the source's units per
sqrt(Hz). The implementation sums independent Ornstein–Uhlenbeck (OU) processes
with logarithmically spaced relaxation times. Finite-band rolloff means the
actual PSD at 1 Hz need not equal `A1^2` when 1 Hz is near or outside the band.

- `ALPHA` is between 0 and 2; 1 gives a 1/f approximation.
- `TD` defaults to zero; output before the delay is zero.
- `TSTEP` defaults to the simulation timestep and cannot be smaller than it.
- `TCORR` defaults to the simulation stop time; zero selects that default.
- `N` defaults to 48 and must be an integer from 4 to 512. Increase it when
  spanning many decades; the minimum count is not an accuracy guarantee.

The nominal band is `1/TCORR` to `1/(2*TSTEP)`. It must have positive width.
Its edges have smooth rolloffs, with power outside the nominal band. The total
variance therefore differs from the integral of an ideal power law cut off
sharply at those edges.

The OU weights follow the power-law integral, including endpoint approximations
for the omitted fast/slow tails. Exponents 0 and 2 use the corresponding fast
and slow single-pole limits. Exact sampled-OU gain is compensated before linear
interpolation. This avoids normalizing to the wrong part of a rolling band
edge. It corrects earlier large interior amplitude errors and changes seeded
PNOISE/FNOISE/ICNOISE waveforms.

Use frequencies comfortably inside the band and verify refinement for the
measurement of interest. With 48 processes over 2–8 decades, analytic comparisons give less than 2.5%
interior PSD error between `10/TCORR` and `1/(20*TSTEP)` for the tested exponents. A four-process bank can
have about 50% interior error over eight decades. Narrow bands may have no
well-separated interior at all. Choose enough processes for the frequency range and check the resulting spectrum.

Changing `TSTEP` changes the upper turnover frequency and total variance;
changing `TCORR` changes the low-frequency plateau. Moving the entire band by a
factor `c`, at fixed `A1`, scales variance by `c^(1-ALPHA)`. In particular,
1/f variance is unchanged. Linear interpolation, simulation sampling and output
aliasing still matter near the upper edge.

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
