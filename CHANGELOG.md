# Changelog

Changes that affect JoSIM users are recorded here. Unreleased entries describe
work prepared for the next release, not binaries already available on GitHub.

## Unreleased

### Added

- Gate-controlled Josephson junctions using `GC=node` and model parameter
  `GWIDTH`. Gate control uses volts in both voltage and phase analysis.
- Configurable current increments for model IV curves using `.iv ... STEP=value`
  or `ISTEP=value`. Sweeps stop at the requested positive and negative limits.

### Fixed

- Invalid gate nodes, duplicate gate controls, non-positive gate widths and
  invalid IV sweep options now produce errors instead of being ignored.

## 2.7 — 2025-12-19

- Added deterministic noise and component spread through `.option seed=value`.
- Added phase offset and finite cycle counts to sinusoidal sources.
- Documented piecewise sinusoidal sources and corrected waveform handling.
- Updated platform builds and included accumulated bug fixes.
