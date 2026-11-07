# Changelog

## Unreleased

- Added `BETA_C` model input for `RTYPE=0` junctions. It calculates RN from the
  nominal Stewart–McCumber parameter, Ic and C. See the
  [damping guide](docs/model_limits.md#beta-c) for requirements and examples.

## 3.0.0

This release adds gate-controlled junctions, additional noise sources and
parameter-sweep tools, along with fixes to phase accumulation, startup and
subcircuit handling. C++ applications must be rebuilt with matching 3.0 headers
and libraries. See [updating to 3.0](docs/compatibility.md).

### Added

- Gate-controlled Josephson junctions using `GC=node` and `GWIDTH`, with gate
  voltage interpreted in volts in both analysis modes.
- Power-law noise, flux noise and fractional critical-current fluctuations.
  The [noise guide](docs/noise_sources.md) describes their units and bandwidth.
- Model-IV controls `STEP`/`ISTEP`, `TSTEP`, `TSTOP` and `TSTART` for current
  increments, simulation duration and averaging windows. `GTJJ` sets gate bias.
- A circuit-sweep helper for independent parameter points and complete-circuit
  IV curves. Each point runs in a separate simulator process.
- Named subcircuit parameters with header defaults, per-instance overrides,
  nested scopes and local junction models.
- Examples for floating flux lines, SQUID noise, capacitor charging and
  Stewart–McCumber parameter conversion.
- Verbose output showing the effective random seed, final internal timestep
  and computed sample count.

### Fixed

- Corrected accumulating voltage-mode junction phase error caused by differing
  Josephson constants in the matrix and history equations (#91).
- Corrected junction and phase-source histories during startup, including phase
  integration and the first voltage samples after startup.
- Noise-source output now contains the samples applied during simulation,
  rather than re-evaluating an advanced random generator at earlier times.
- Corrected white-noise bandwidth normalization for linear interpolation.
  Automatic thermal noise now follows the junction's active resistive slope,
  including the transition region.
- Corrected power-law noise weights and sampled-process normalization. These
  noise fixes change seeded waveforms; see the [migration notes](docs/compatibility.md).
- Seeded repeated runs and timestep retries reset cached Gaussian samples as
  well as random-number engines.
- Model-IV points start with fresh device histories and retain their requested
  bias when a timestep retry rebuilds the testbench.
- Corrected hierarchical pin aliases, ground handling and trace differences.
  Names such as `GNDREF` no longer short to ground; `C()` again aliases current.
- Corrected expression parsing for engineering suffixes, signed exponents,
  unary signs, function powers and `LOG10`. Invalid expressions report errors.
- Corrected custom-waveform timing, interpolation, delayed repetition and
  relative paths. File paths and model-IV output names retain their case.
- Corrected print-grid rounding that produced uneven output intervals for
  integral timestep ratios. Output still uses actual internal samples.
- Validated sine cycle counts, pulse timing, piecewise waveform grids, junction
  model domains and transient sample counts. Invalid inputs fail with a useful
  diagnostic instead of crashing or producing non-finite output.
- Cyclic includes and recursive subcircuits report errors. Output cannot
  overwrite an input netlist, including through a hard-link alias.
- Failed writes and flushes are reported for CSV, DAT, RAW, model-IV files and
  standard output. Non-finite derived traces are rejected before opening files.
- Library failures throw exceptions instead of terminating the host process;
  solver allocations are released after failures and timestep retries.
- Reduced transient-history allocations, per-step copying and progress-worker
  CPU use. Impossible result-storage requests report an error.
- Updated plotting tools for Pandas 3, phase scaling, comparison timestamps and
  image dimensions. HTML and image exports check inputs and output paths.
- Phase-count specifications support nonuniform timestamps, reversals and
  returns toward zero after the requested stability interval.

### Build and distribution

- CMake/CTest 3.21 or newer is required. Builds use C++17 and C11, with pinned
  solver archives and checksum verification.
- Fixed installed CMake dependencies, exported headers, static runtimes and
  filesystem linking on older GNU toolchains. Static and shared libraries are
  supported; CMake packages and Unix shared libraries use major version 3.
- Packages are available for Linux x86_64 and AArch64, Windows x64, and macOS
  Apple Silicon and Intel. Downloads include manifests and SHA-256 checksums.
- Added cross-platform build, library, numerical, documentation and Python-tool
  checks, plus optional sanitizer and LLVM coverage builds.
- Removed inactive SuperLU and OpenMP selections. KLU is the supported solver.
- Retired `ivcurve.py` and `noise_insert.py`. Use the circuit-sweep helper and
  native `.TEMP`/`.NEB` controls respectively.
- Unsupported capacitor `IC=` requests now report an error. See the
  [model guide](docs/model_limits.md) for preparing a charged capacitor.

## 2.7 — 2025-12-19

- Added deterministic noise and component spread through `.option seed=value`.
- Added phase offset and finite cycle counts to sinusoidal sources.
- Documented piecewise sinusoidal sources and corrected waveform handling.
- Updated platform builds and included accumulated bug fixes.
