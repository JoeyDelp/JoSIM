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

- Numeric expressions now parse lowercase engineering suffixes, signed scientific
  exponents and unary signs correctly. Function powers and `LOG10` return the
  expected values. Missing operands, invalid real-number domains and mismatched
  brackets now fail instead of crashing, hanging or silently returning a result.

- Custom waveforms resolve relative files without an optional-path crash, align
  linear/cubic interpolation to the stored samples and delay, and repeat from the
  delayed start. Their sample interval no longer stretches with the simulation
  step. Invalid timing, flags and waveform data now fail with useful errors.

- Invalid gate nodes, duplicate gate controls, non-positive gate widths and
  invalid IV sweep options now produce errors instead of being ignored.
- Noise update intervals advance correctly when a caller crosses several update
  points. Invalid noise timing and process counts produce useful errors.
- Junction thermal-noise amplitude updates now modify the stored source.
- Model assignments retain support for spaces around `=`.
- Repeating or rewinding a seeded simulation in the same process resets cached
  Gaussian samples as well as the random-number engines.
- Random-number engines initialize correctly with the GCC 8 toolchain used for
  RHEL6-compatible builds.
- Bundled C solvers use a compatible C standard with current GCC. Linux timing
  functions link correctly on systems with glibc older than 2.17.
- Invalid transient times, print intervals and unsupported point counts now
  produce errors instead of crashing or silently substituting another run.
- Recursive subcircuit dependencies now report the unresolved subcircuits
  instead of hanging during expansion.
- Installed C++ packages now load their dependencies, expose the required
  headers and C++17 setting, and support versioned `find_package(josim)` calls.
- Solver source downloads are pinned to immutable archives and verified with
  SHA-256 checksums.
- Static Linux runtime flags now apply during linking. Static Windows builds use
  the same MSVC runtime for JoSIM, its solvers and installed-library consumers.

### Build and distribution

- Added default/shared/static library qualification with relocated installations
  and fresh consumers. Shared JoSIM embeds static KLU dependencies, exports
  Windows entry points and uses relative installed CLI loader paths. Conflicting
  shared/static requests fail explicitly. Native default/shared checks now gate
  release publication on all five platforms.
- Detect whether C++17 filesystem functions need `stdc++fs`; retain the public
  dependency on legacy GNU toolchains and remove it from modern builds.

- Library `.iv` model lookup now throws instead of exiting the host process.
  Check KLU setup/factorization failures and sparse-array validity; release
  solver allocations on exceptions and timestep retries. Added repeated
  failure/recovery tests and explicit embedding/random-state guidance.

- Fixed long CLI option values and exact switch recognition. Invalid/empty
  arguments, out-of-range minimal/verbose values and unsupported output formats
  fail cleanly. Explicit stdin consistently overrides a positional filename.
  Retired the inactive OpenMP/`-p` placeholder; no parallel solver was implemented.
- Protect main and included netlists from normal and `.iv` output collisions,
  including relative-path and hard-link aliases. Track input provenance in
  `Input`; embedding applications must recompile. Direct/indirect cyclic
  includes now fail cleanly instead of recursing until a crash.
- Effective seeds are printed with `-V 1` even with `-m 1`. Reject seed
  expressions at or above 2^64 before integer conversion; the exact integer
  literal 18446744073709551615 remains supported.
- Preserve custom-waveform paths and model-IV output filenames exactly, including
  mixed case and continued lines. Blank tab lines and indented comments are
  handled safely; a continuation without a preceding line fails cleanly.

- Retired the non-functional SuperLU selection. `SLU=ON` and manual `SLU` macro
  definitions now fail explicitly instead of implying that another solver runs.
  KLU remains the supported solver. Removed the unused SuperLU/CBLAS dependency
  downloads, linkage and installed package requirements.

- Added opt-in LLVM first-party coverage and AddressSanitizer/UndefinedBehaviorSanitizer
  builds, with reproducible report commands and installed-consumer runtime linkage.

- One release script builds, tests, installs and verifies all platform packages.
- GitHub Actions targets Linux x86_64 with glibc 2.12, Linux AArch64 with glibc
  2.17, Windows x64, and separate macOS ARM and Intel binaries.
- Packages include source/toolchain manifests and SHA-256 checksums. Each archive
  is extracted, checked and executed before upload. Release tags publish only
  after every target passes.
- Replaced obsolete setup instructions with current build and release guides.
- CI argument handling works with older Bash versions, and the Windows runner
  supplies the selected Visual Studio 2022 generator.

## 2.7 — 2025-12-19

- Added deterministic noise and component spread through `.option seed=value`.
- Added phase offset and finite cycle counts to sinusoidal sources.
- Documented piecewise sinusoidal sources and corrected waveform handling.
- Updated platform builds and included accumulated bug fixes.
