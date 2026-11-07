# Numerical methods

JoSIM solves the transient circuit equations using voltage or phase as the nodal
unknown. It uses the second-order backward difference formula (BDF2):

\[
\left.\frac{dx}{dt}\right|_n\approx
\frac{3x_n-4x_{n-1}+x_{n-2}}{2h},
\]

where `h` is the internal timestep. Nonlinear junction terms are predicted from
previous samples, followed by one sparse linear solve per step. JoSIM does not
perform Newton iteration within a step or provide SPICE RELTOL/ABSTOL controls.

## Timestep and startup

Each attempt uses a fixed timestep. Transmission-line delays can require a
smaller step when the matrix is built. During a run, a predicted junction phase
increment above 0.2 cycles after the first 1% of samples causes the whole run
to restart at half the timestep. The step is never increased automatically.

This phase-advance check is not an estimate of integration error. Repeat the
simulation with a smaller requested step and compare the quantities that matter
for the circuit. Use `-V 1` to see the final step and sample count. Settling time,
noise bandwidth and output sampling need their own checks.

Default startup advances the circuit through a finite negative-time interval
from zero stored histories. It does not calculate a DC operating point.
`DST` disables this interval. For a nonzero capacitor voltage, include the
[charging circuit](model_limits.md#capacitor-voltage) before the event of interest.

## Junction phase {#junction-phase}

The Josephson voltage relation is

\[
V=\sigma\frac{d\phi}{dt},\qquad \sigma=\frac{\Phi_0}{2\pi}.
\]

Under constant voltage, phase should therefore increase linearly:

\[
\phi(t)-\phi(t_a)=\frac{V}{\sigma}(t-t_a).
\]

This provides a simple check for long simulations. Earlier versions used
slightly different constants in the junction matrix and history terms, which
caused an accumulating phase error in voltage mode. Version 3.0 uses the same
constant in both places. Phase is not wrapped, so accumulated cycles remain
available in the output.

## Buffer example {#buffer}

The circuit in [issue #101](https://github.com/JoeyDelp/JoSIM/issues/101) has two
junctions and two inductors. Its reverse response contains positive and negative
voltage pulses, even though their net area is almost zero. B02 makes a negative
phase excursion while B01 returns; the two voltage peaks need not coincide.

A separate integration of the Kirchhoff and RCSJ equations gives the same
response. For the supplied parameters, the reverse net pulse area is about
`-1.38e-6 Phi0`, while the forward area is about `0.999998 Phi0`. The triangular
input pulse is approximately 4.3 ppm below one flux quantum.

The [reference circuits and data](https://github.com/JoeyDelp/JoSIM/tree/master/test/reference)
include both directions. They use the subgap resistance because the junction
voltages stay below the model's transition region. Tests compare both JoSIM
analysis modes with the reference and repeat them at smaller timesteps.

## Noise and output sampling

A resolved integration step does not by itself prevent output aliasing.
Check the print interval and filtering against the frequencies present in the
circuit. The [noise guide](noise_sources.md) describes the interpolation and
bandwidth conventions; the [SQUID example](squid_noise.md) shows how to compare
bandwidth, timestep and sample spacing.
