# Junction phase accumulation — 6 November 2026

Issue #91 reports long-run junction artifacts without a reproducing circuit.
This study identifies a concrete defect capable of producing that symptom.
It does not infer the reporter's exact circuit or explain every possible artifact.

## Reproduction and cause

Drive an RCSJ junction with an ideal 1 mV source, `IC=100u`, `RN=10`, `C=0.1p`,
`RTYPE=0`, a cold start, and a 0.01 ps timestep. Inspect raw double-precision
results, rather than rounded CSV phase values. Once the startup transient has
passed, Josephson's relation requires

\[
\phi(t)-\phi(t_a)=\frac{V}{\sigma}(t-t_a).
\]

Anchoring at sample 100 removes the arbitrary phase origin and the cold-start
transient. The old voltage-mode matrix uses
`SIGMA_H = HBAR/(2*EV) = 3.2910597572337034e-16 V*s`, while its history RHS uses
`SIGMA = 3.291059757e-16 V*s`. Their relative difference is only
`rho - 1 = -7.10116e-11`, but the resulting recurrence is

\[
\phi_n=\rho\left(\frac43\phi_{n-1}-\frac13\phi_{n-2}\right)
       +\frac{2hV}{3\sigma_H}.
\]

Its nominal unit root acquires a false decay of approximately
`1 - 1.06517e-10` per step. A 70-digit independent evaluation of this recurrence
predicts the measured drift within 2.4 ppm across all four run lengths:

| Steps | Nominal duration | Old voltage-mode phase error |
|---:|---:|---:|
| 10,000 | 0.1 ns | 0.000161796 rad |
| 100,000 | 1 ns | 0.0161826 rad |
| 1,000,000 | 10 ns | 1.61822 rad |
| 10,000,000 | 100 ns | 161.771 rad |

Phase mode does not show this false decay. Its worst error against the legacy
`SIGMA` reference in the 10-million-step probe is 8.85e-5 rad. The difference
between the legacy physical constant and exact SI definitions contributes
about 0.00256 rad over 100 ns; it cannot explain the voltage-mode defect.
The exact SI reference is `sigma = h/(4*pi*e)` using the
[NIST flux-quantum relation](https://physics.nist.gov/cgi-bin/cuu/Value?flxquhs2e).

## Repair and acceptance

Derive both JJ matrix coefficients from the existing `Constants::SIGMA`, the
same scale already used by history terms, sources and output conversion.
All physical constant values remain unchanged. This removes the inconsistent
recurrence coefficient; it does not wrap phase or discard accumulated cycles.

`test/junction_numerics.cpp` supplies 24 raw-result cases: both modes, positive
and negative 1 mV bias, 0.01/0.1 ps timesteps, and 10k/100k/1m samples. It checks
finite data, the independent Josephson relation, constant voltage, KCL, and
`I = Ic*sin(phi) + V/R` after the constant-voltage capacitive transient.

The phase allowance is `8*epsilon*abs(V*h/SIGMA)*N^2 + 1e-10 rad`, with
`epsilon` the double machine precision and `N` the sample count. It is a
conservative accumulated-rounding allowance, rather than a fitted physical
error or a relaxed comparison to the broken result. Constitutive current,
KCL and voltage allowances are 1e-10 A, 1e-15 A and 1e-10 V respectively.
The old code fails even the shortest case by more than four orders of
magnitude relative to its phase allowance.

GCC16/Clang23 Debug and Release pass all 32 tests. The full ASan/UBSan suite,
with leak detection, passes 32/32 in 175.12 s; the raw junction group takes
28.06 s. The 24 constant-bias cases have at most 6.88e-6 rad drift, below 4.4%
of their rounding allowances. Native-platform qualification remains pending.

The extended 10-million-step green probe reduces voltage-mode drift from
161.771 rad to 8.736e-5 rad, a factor of about 1.85 million. Phase-mode drift
is 8.844e-5 rad. Both are now at accumulated-rounding scale. The separate
legacy-versus-SI scale difference remains visible as about 0.00248 rad over
this duration. No physical constants were silently updated.

The eight extended runs complete in 1.78 s with peak RSS about 411.4 MiB on
the local Linux/Clang Release build. This measures a four-unknown deterministic
circuit with retained raw histories; it is not the required large/noisy-circuit
performance qualification. Commands and resource output are preserved.
Detailed local evidence: `build/audit-resolution/long-phase-red.json`,
`long-phase-initial.csv`, `junction-numerics-red.log`, and
`artifacts/audit-resolution/junction-constant-bias.csv` and
`junction-long-phase.json`. Red evidence includes
the source/library identities, probe hash, high-precision prediction and resource
measurements. The final report will publish compact evidence durably.

Current-driven switching, pulse counts, nonlinear convergence and the #101
buffer study remain separate numerical requirements. This repair establishes
a cause and protects constant-bias phase accumulation; it does not establish
arbitrary nonlinear-circuit accuracy.
