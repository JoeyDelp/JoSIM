# Preserved noise research

This branch records the existing local research before integration. It is not
a qualified release. The original dirty checkout and stash remain intact.

Retained work includes PNOISE, FNOISE, fractional ICNOISE, corrected white-noise
tick advancement, thermal noise bandwidth changes, nested model parsing, Debug
checks, and unique circuit/analysis inputs from test_internal. Generated CSV and
image files remain in the original local directory.

The isolated Debug build succeeded. NOISE and ICNOISE self-tests returned zero;
PNOISE/FNOISE print success but return one due to their bool exit-code mapping.
The OU-bank variance differs from the ideal hard-band power-law integral. Its
normalization, finite-band rolloffs and statistical tests need an explicit
contract. The phase-step modulo change can hide full-cycle advances and must
not replace the unwrapped timestep guard without evidence.

Integration must retain current v2.7 source aliases, gate support, public APIs
and legacy circuits. Original source base: 02a34ee; original gate stash: 3aac073.
