# Pulse domains and large cycle counts — 7 November 2026

F28 is a source evaluation defect. `return_pulse` converted `(t-TD)/PER` to
`int64_t` without checking the domain or range. A zero period returned success
in Release and triggered UBSan's floating-to-integer diagnostic. A valid finite
period can also exceed the integer cycle-count range.

The finite witness uses `h=2^-40 s`, `PER=2^-104 s`, `PW=PER/2`, zero delay and
instantaneous edges, a 1 V source and a 1 kΩ load. Its eight sample times are
exact multiples of PER: every sampled source value must be 1 V. The old Release
binary instead produced `[1,0,0,0,0,0,0,0]`; UBSan stopped at cycle count `2^64`.
This deliberately aliases the source and tests defined evaluation at represented
times. It does not establish adequate sampling of such a high-frequency pulse.

## Repair and compatibility

Parsing now requires finite amplitudes, finite nonnegative TD/TR/TF/PW and a
finite strictly positive PER, with at most seven arguments. Validation does not
depend on verbosity. Zero rise/fall times remain valid instantaneous edges;
zero plateau duration remains a valid triangular pulse. The old zero-width
"redundant" warning was inaccurate and is no longer emitted. Reparsing replaces
the stored pulse rather than appending to its previous data.

For ordinary cycle counts, the existing arithmetic and operation order remain.
At or beyond `2^63`, including an overflowing floating quotient, evaluation uses
`fmod(t-TD, PER)` without an integer cast. No public signature or class layout
changes. `INVALID_PULSE_PARAMETERS` is appended to the error enum, preserving
existing numeric values and the old diagnostic enum names for callers.

## Verification

Direct source tests use independent endpoint tables for rise, plateau, fall,
delay, period wrapping, defaults, inverted pulses, instantaneous edges and zero
plateaus. Exact dyadic large times cover both integer-range and quotient overflow.
Invalid definitions are checked at both verbosity levels. Process regressions
exercise both analysis modes, clean rejection without output, and all eight
expected values of the finite witness within 1 nV.

All four local GCC/Clang Debug/Release configurations pass **35/35 CTest groups**.
The changed source passes the existing focused Clang-Tidy checks with no warnings.
Full Clang Debug ASan/UBSan with leak detection passes **35/35 in 223.26 s**.
Native CI remains a separate gate.

The performance acceptance was declared before timing: seven counterbalanced
pairs on a 64-cell pulse/resistor bank, 300,000 steps, both analysis modes,
identical CSV bytes and no more than 5% median wall-time regression. Independent
0/1 mV point values must agree within 1 nV. The observed median increases are
**1.28% in voltage mode and 0.41% in phase mode**, with identical CSVs and no
changed numerical tolerance. Other heavy local jobs were stopped during timing.

`artifacts/audit-resolution/pulse-domain/` retains old/new witnesses, commands,
binary/source/owner hashes, all compiler receipts, the exact timing owner and
predeclared acceptance. The baseline is clean `00512b6`; local repair receipts
identify `7100a39` plus the source diff. An initial timing-owner path typo failed
before executing any benchmark and is retained under `build/`; it is not a pass.
