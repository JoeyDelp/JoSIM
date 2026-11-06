# Changelog

Changes that affect JoSIM users are recorded here. Unreleased entries describe
work prepared for the next release, not binaries already available on GitHub.

## Unreleased

### Added

- Gate-controlled Josephson junctions using `GC=node` and model parameter
  `GWIDTH`. Gate control uses volts in both voltage and phase analysis.
- Configurable current increments for model IV curves using `.iv ... STEP=value`
  or `ISTEP=value`. Sweeps stop at the requested positive and negative limits.
- Power-law, flux and fractional critical-current noise sources. See the
  [noise source guide](docs/noise_sources.md) for units and bandwidth conventions.

### Fixed

- Invalid gate nodes, duplicate gate controls, non-positive gate widths and
  invalid IV sweep options now produce errors instead of being ignored.
- Noise update intervals advance correctly when a caller crosses several update
  points. Invalid noise timing and process counts produce useful errors.
- Junction thermal-noise amplitude updates now modify the stored source.
- Model assignments retain support for spaces around `=`.

## 2.7 — 2025-12-19

- Added deterministic noise and component spread through `.option seed=value`.
- Added phase offset and finite cycle counts to sinusoidal sources.
- Documented piecewise sinusoidal sources and corrected waveform handling.
- Updated platform builds and included accumulated bug fixes.
