# Branch consolidation

The consolidation starts from public v2.7, commit `02a34ee`.

| Source | Disposition | Evidence |
|---|---|---|
| `gate-tunable-jj` | Merged in `5573360` | Gate control, IV steps and numerical regressions passed on Linux, Windows and both macOS architectures. |
| Local noise research | Merged selectively in `505abca` | Noise variance/PSD, units, circuit relationships, model syntax and phase-step regressions passed on the same platforms. |
| Full research snapshot | Preserved at `archive/noise-research-2026-11-06` (`c7c2de3`) | Includes prototype debug checks and all unique circuit/Python research inputs. Generated CSV/images remain outside source history. |
| Original gate stash | Preserved at `archive/gate-stash-2025-12-15` | Retains the stash's working tree, index and untracked PWS input. Its useful IV changes are in the gate merge. |
| `testing` | Already contained in v2.7 | No independent commits remain. |
| `single_precision` | Preserved at `archive/single-precision-2021-02-03` | The unique commit replaces doubles with floats throughout an older solver. It is not enabled in the consolidated simulator. |
| `gh-pages` | Retained | Generated documentation deployment history. |

Double precision remains the default. At 10 us, adjacent float values are about
0.909 ps apart, so a 0.25 ps time increment can disappear entirely. Double values
remain distinct at that interval. Any future mixed-precision optimization needs
an explicit accuracy/performance contract rather than a global scalar replacement.

The original modulo phase-step proposal was also excluded. A real simulation
regression shows that wrapping a full-cycle advance can bypass timestep
reduction. The unwrapped phase guard is retained.

The power-law process bank has smooth frequency rolloffs. Its total variance
is not the integral of a sharply truncated ideal power law. Current regressions
measure sampled spectra and physical flux/current relationships independently
of the process-bank implementation. See [noise sources](noise_sources.md).

Archive tags are preservation references, not releases. Remote branch deletion
must wait until the corresponding histories and archive references are present
on GitHub. No version increase or release tag is implied by consolidation.
