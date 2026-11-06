# Floating flux lines and SQUID noise

These examples address the floating-reference and noise-setting questions in
[#107](https://github.com/JoeyDelp/JoSIM/issues/107), and external flux and
junction thermal noise in [#108](https://github.com/JoeyDelp/JoSIM/issues/108).
They describe classical circuit models; no quantum dynamics are implied.

## A floating flux line

`test/comp/squid_floating_flux.cir` couples an electrically separate primary
inductor to a two-junction loop. Its voltage source connects two non-ground
nodes, which is supported. The entire primary island also needs a voltage
reference: without one, its unconstrained common-mode voltage makes the matrix
singular. Merely assigning a different node name does not fix that constraint.

`RREF REF 0 1g TEMP=0` supplies the numerical reference. In this example it
carries zero current, and changing it from 1 ohm to 1 megohm or 1 gigaohm leaves
the loop response unchanged. `TEMP=0` prevents a global temperature setting
from adding spurious noise to this numerical anchor. In a circuit with other
paths to ground, the resistor can load a physical mode; verify insensitivity
instead of treating it as ideal galvanic isolation.

The signed mutuals are \(-5\) pH and \(+5\) pH, so applied loop flux is
\((M_2-M_1)I_P\). A primary current of 51.6958462 uA gives \(0.25\Phi_0\).
The independent flux/KCL equilibrium predicts 48.5021253 uA circulating current
and junction phases \(\pm0.0485211620\) rad. Both modes, both flux polarities,
two timesteps and three reference resistances agree with that prediction.
Mutual-inductor signs follow the inductor terminal order in the netlist.

## A bounded noise example

`test/comp/squid_noise.cir` uses a symmetric DC-SQUID's zero-loop-inductance
limit. Opposite model `PHI` offsets represent a constant external flux of
\(0.25\Phi_0\). This reduction is useful for a checkable noise example; use the
coupled finite-inductance circuit above when screening flux and loop inductance
matter. A phase-source function does not independently solve magnetic fields.

Each junction has \(I_c=1\) mA, \(R_N=1\) milliohm and \(C=10\) pF.
At 1.6 mA total bias the effective parameters give

\[
I_{c,eff}=2I_c\cos(\pi\Phi_{ext}/\Phi_0),\quad R_{eff}=R_N/2,\quad C_{eff}=2C,
\qquad \langle V\rangle=R_{eff}\sqrt{I_b^2-I_{c,eff}^2}.
\]

The Josephson frequency is 180.946 MHz. The effective plasma frequency is
73.773 GHz, but the very small \(\beta_c=2.15\times10^{-5}\) makes this example
strongly overdamped. Its weak classical-noise approximation is checked against
16 seeded runs at each `.NEB` of 20 MHz, 200 MHz and 2 GHz. The internal step
is 2 ps; output spacing is 20 ps without FIR filtering. Discard 100 ns, then
measure 5 us. The [qualification study](qualification/2026-11-06-squid-noise.md)
gives the independent noise equation, uncertainty and refinement results.

From the repository root, with a built simulator:

```bash
build/dev/josim-cli -m 1 -V 1 -a 1 -o build/squid-noise.csv test/comp/squid_noise.cir
python3 -B tools/qualify_squid_noise.py \
  --binary build/dev/josim-cli --output artifacts/squid-noise
```

The optional scientific owner requires `tools/requirements-qualification.txt`
and a new output directory. It records seeds, the effective timestep, source
and binary hashes, per-run PSDs, statistical criteria and a plot. It preserves
the input netlists and writes results into its new directory. Native CTest checks the floating-reference
example; the longer scientific campaign is a separate qualification command.

## Choosing settings

- `.TEMP` enables automatic thermal sources; local `TEMP=` overrides it.
  `.NEB` sets their update interval to `max(h,1/(3*NEB))`. Local `NEB=` overrides
  the global. The [noise guide](noise_sources.md) defines variance and rolloff.
- Resolve the circuit dynamics and noise interpolation with the internal step.
  Halve that step while holding the physical bandwidth fixed. Setting the step
  equal to the noise update interval does not establish noise accuracy.
- Josephson oscillations can mix noise near \(f_J\) down into the measurement
  band. This example changes noise when bandwidth passes through \(f_J\),
  even though every tested `.NEB` is far below its plasma frequency.
- There is no universal rule that bandwidth must exceed \(f_P\) or \(f_J\).
  An underdamped plasma resonance, different bias, screening inductance or a
  different noise mechanism can require different choices. Increase bandwidth
  and refine the timestep until the intended observable is stable within its
  statistical uncertainty and model's physical validity.
- Check output Nyquist frequency and filtering separately. Downsampling can
  alias unresolved power. Report warmup, duration, sample interval, window,
  seeds and uncertainty; one noisy trace is not a convergence study.
- These sources use classical equilibrium/local-slope noise. They do not model
  quasiparticle shot noise, heating or quantum noise. Relevant noise frequencies
  must remain well below \(k_BT/h_{Planck}\); that scale is 87.514 GHz at 4.2 K.
