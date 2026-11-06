# Floating references, flux and SQUID noise — 6 November 2026

Two bounded examples address the support questions in #107/#108. The finite-L
example checks electrical references and external mutual flux. A separate
zero-L reduction provides an independently checkable classical noise response.
Neither implements quantum dynamics, nonequilibrium quasiparticle noise or a
general magnetic-field solver. The [user guide](../squid_noise.md) gives commands.

## Floating primary and finite-L equilibrium

The primary has L=100 pH and a 1-ohm series resistor. Its voltage source is
between two non-ground nodes. Mutuals to the two 5 pH SQUID arms are -5/+5 pH.
Their squared coupling coefficients sum to 0.1, below 1, so the three-inductor
energy matrix is positive definite. With no transport bias, KCL gives opposite
arm currents. Flux linkage and the sinusoidal current-phase relation give

\[
2\sigma\arcsin(I/I_c)+(L_1+L_2)I=(M_2-M_1)I_P,\qquad
\sigma=\frac{h_{Planck}}{4\pi e}.
\]

For \(I_c=1\) mA and applied flux \(0.25\Phi_0\), independent scalar bisection
gives \(I_P=51.6958462115\) uA, circulating current 48.5021253127 uA and
junction phases \(\pm0.0485211619925\) rad. The 24 native cases cover two modes,
1/0.5 ps steps, both flux polarities and 1-ohm/1-megohm/1-gigaohm reference
resistors. After 10 ns, assertions require current error <=20 pA, phase error
<=1e-8 rad, flux residual <5e-7 Phi0 and reference current/KCL residual <1 fA.
The budgets include CSV and retained physical-constant rounding. Measured
maximum flux residual is 3.713e-9 Phi0. Removing the reference gives a useful
singular-matrix error in both modes, after removing the now-invalid reference
current trace request too.

The anchor fixes the island's common-mode voltage and carries no loop current
in this circuit. It has TEMP=0 so a global thermal setting cannot turn the
numerical anchor into an unintended noise source. Other circuits can have
physical common-mode paths; the anchor's insignificance must be checked there.
This establishes a floating voltage source and applied loop flux, independently
of claims about a complete SQUID amplifier or its parasitics.

## Independent weak-noise reference

For equal junctions with negligible loop inductance and external flux f in
flux quanta, the two sinusoidal currents reduce to one effective RSJ:

\[
I_c^{eff}=2I_c\cos(\pi f),\quad R_{eff}=R_N/2,\quad C_{eff}=2C,
\qquad \dot\psi=\frac{R_{eff}}{\sigma}(I_b-I_c^{eff}\sin\psi+\delta I).
\]

The circuit retains finite C; the reference neglects its effect in the strongly
overdamped, weak-noise limit. At f=0.25, each Ic=1 mA/RN=1 milliohm/C=10 pF,
and Ib=1.6 mA, the independent SI values are:

| Quantity | Value |
|---|---|
| Effective Ic / R / C | 1.41421356 mA / 0.5 milliohm / 20 pF |
| Mean voltage, R sqrt(Ib²-Ic_eff²) | 0.374165739 uV |
| Josephson frequency, mean voltage/Phi0 | 180.945746 MHz |
| Effective plasma frequency | 73.772542 GHz |
| beta_c / Gamma=kBT/(Ic_eff sigma) | 2.14857e-5 / 1.24590e-4 |
| Classical scale kBT/h at 4.2 K | 87.513800 GHz |

Let \(D=I_b^2-(I_c^{eff})^2\). Transform the noiseless running phase to a
uniform angle theta. Its small-noise phase-response factor is
\(Z(\theta)=(I_b+I_c^{eff}\cos\theta)/\sqrt D\). The constant and first
harmonic mix current noise into low-frequency voltage noise, giving

\[
S_V(f)\simeq\frac{R_{eff}^2}{D}\left[
 I_b^2S_I(f)+\frac{(I_c^{eff})^2}{4}
 \{S_I(f_J+f)+S_I(f_J-f)\}\right].
\]

The two independent shunts supply
\(S_I(f)=4k_BT/R_{eff}\;\operatorname{sinc}^4(f/(3NEB))\). This reference uses
exact SI constants and the independently derived interpolation transfer, not
JoSIM output or fitted amplitudes. In the broad-white limit at f≈0 it reduces
to \(4k_BTR_{eff}[I_b^2+(I_c^{eff})^2/2]/D\); for noise far narrower than fJ,
the mixing term is suppressed. This is why bandwidth can affect measured
low-frequency noise even in an overdamped circuit.

## Protocol and measured acceptance

`tools/qualify_squid_noise.py` owns the fixed scientific campaign; optional
dependencies are pinned in `tools/requirements-qualification.txt`. Sixteen seeds
at each of three bandwidths, plus four noiseless mode/step cases and nine paired
refinements, give **61 runs**. Each discards 100 ns and measures 5 us. The base
internal step is 2 ps, output spacing 20 ps, with no FIR. Verbose output verifies
the final internal step and computed count, so a full-run retry cannot silently
change the experiment.

Welch analysis uses 0.5 us periodic-Hann windows, 50% overlap and separate
4/8/12/16/20 MHz bins. Each seed's primary statistic is the mean of its five
PSD/reference ratios. The between-seed standard error gives a Student-t
interval, using a Bonferroni correction for nominal 99% family confidence over
three bands (critical t=3.483677, 15 degrees of freedom). A predeclared 5%
systematic allowance covers weak-noise/finite-C/low-frequency and window
approximations; acceptance is |ensemble mean ratio-1| <= statistical half-width
+0.05. This is an aggregate-band criterion; individual bins retain sampling
scatter. No amplitude is fitted to the simulator.

| NEB | NEB/fJ | Mean PSD/reference | Statistical half-width |
|---|---|---|---|
| 20 MHz | 0.110530 | 1.020164 | 0.067364 |
| 200 MHz | 1.105304 | 0.998104 | 0.123052 |
| 2 GHz | 11.053037 | 0.992709 | 0.064432 |

All pass the declared criterion. Actual mean measured voltage PSDs across the
five bins are 0.401566/0.638990/0.730640 pV²/Hz, compared with independent
0.393702/0.640216/0.736007. At the lowest bandwidth, the measurement band also
approaches the noise rolloff; the reference explicitly includes that attenuation.
The higher-band changes include noise mixed down from near fJ. Every tested
bandwidth remains far below the bare plasma frequency; this particular strongly
overdamped model does not require driving noise through that resonance.

Paired seed-101 checks hold the physical noise interval fixed. Halving h to
1 ps changes any primary-bin PSD by at most **0.1413%**. Halving output spacing
to 10 ps changes it by at most **0.0612%**. Switching analysis mode changes it
by at most **9.059e-7 relative**. Each is below the predeclared 1% refinement
bound. Noiseless periods err by 1.9803 ppm at 2 ps and 0.5138 ppm at 1 ps in
both modes, improving about fourfold. Mean-voltage error is below 0.164 nV,
inside the 1 nV finite-record allowance. Noiseless period allowance is 10 ppm.

The sampled PSD equation assumes weak classical fluctuations, fixed bias,
negligible screening and low measurement frequencies relative to fJ. It does
not justify a universal NEB>fJ or NEB>fP prescription. Underdamped resonances,
larger noise, switching, finite loop inductance or other noise mechanisms need
their own physical model and bandwidth/timestep convergence checks.

## Defects uncovered by the example

F18 rejects excess NOISE/PNOISE/FNOISE/ICNOISE arguments, previously discarded
silently in four reproduced cases. Direct-library and clean-CLI tests protect
the documented syntax. Verbose output also now reports effective timestep and
computed count; a 2 ps requested grid reduced to 0.25 ps reports 512 actual
points over 128 ps in both modes.

F19 repairs output selection. A 1 ps internal grid with 10 ps print spacing
previously emitted 9/10/11 ps gaps because cumulative target addition crossed
floating grid ties inconsistently. Select targets directly with `fma` and a
few-ulp tie allowance; do not interpolate output. Both modes and integer/
fractional starts check all 1990 timestamps against independent integer-index
expectations. General nonintegral step ratios still select actual nonuniform
internal samples. Device equations, physical constants and file columns are
unchanged; corrected output sample choices intentionally change affected files.

Source/fixture/binary hashes, exact decks, logs, per-seed PSDs, uncertainty and
figures are under `artifacts/audit-resolution/squid-noise/`. The exact final
61-run owner is preserved there as `qualification-owner.py`, matching the
current tool's recorded hash. It verifies reported seeds and effective steps,
and checks source/binary stability. Both PNG and PDF were generated; the plot
was inspected. Raw reproducible transient CSVs were removed after analysis;
their hashes and complete generation inputs remain. Peak child RSS
was 145.7 MiB; concurrent build/sanitizer activity means these timings are not a
controlled performance benchmark. Native numerical receipts and red print-grid
witnesses are under `build/audit-resolution/squid-example/`.

The candidate is based on cb594a3 plus the F18/F19/example changes. All four
GCC16/Clang23 Debug/Release configurations pass 33/33; the full ASan/UBSan suite
with leak detection passes 33/33 in 209.52 s. A subsequent explicit standard-header
include was compiled in all four CLI builds; numerical behavior was unchanged.
Native results for the implementing revision remain pending in the audit ledger.
Final integration/release/site qualification remains separate.
