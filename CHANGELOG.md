# Changelog

Changes that affect JoSIM users are recorded here. Unreleased entries describe
work prepared for the next release, not binaries already available on GitHub.

## Unreleased

### Added

- Pinned Python utility dependencies, tested headless exports and a CI release
  gate for the plotting and phase-specification tools.
- Safe one-parameter external circuit sweeps with fresh processes, preserved
  templates/includes, isolated outputs, checked failures, effective seeds/grids
  and time-weighted measurement windows (#84/#82). Add tested resistance and
  complete SQUID circuit-IV templates in both analysis modes.
- Validated floating flux-line and weak-noise SQUID examples for #107/#108,
  with independent flux equilibrium, Josephson/plasma-frequency guidance,
  controlled-seed PSD uncertainty and timestep/output-sampling refinement.
  Optional `tools/qualify_squid_noise.py` records the longer scientific campaign.
- Verbose output reports the effective internal timestep and computed sample
  count after full-run retries, alongside the effective random seed.
- Tested capacitor precharge/discharge and explicit Stewart–McCumber parameter
  conversion examples. Document initial-state and nonlinear-inductor boundaries
  (#109, #93, #88).
- Independent finite-capacitance buffer references, pulse-flux/conservation
  and timestep-refinement checks in both modes (#101). The reverse bipolar
  response agrees with the independent physical model. Native tests use compact
  fixtures; optional pinned scientific tools regenerate references and plots.
- Independent current-driven RSJ equilibrium, oscillation-period refinement
  and average flux-quantum pulse-area checks in both modes.
- Independent analytic RC/RL/RLC regression tests, conservation checks and
  timestep refinement in both analysis modes. Native binary and library
  qualification require this numerical test group.

- Gate-controlled Josephson junctions using `GC=node` and model parameter
  `GWIDTH`. Gate control uses volts in both voltage and phase analysis.
- Configurable current increments for model IV curves using `.iv ... STEP=value`
  or `ISTEP=value`. Sweeps stop at the requested positive and negative limits.
- Power-law, flux and fractional critical-current noise sources. See the
  [noise source guide](docs/noise_sources.md) for units and bandwidth conventions.

### Fixed

- Repair Pandas 3 DAT loading, optional image dimensions, comparison timestamps
  and phase-unit scaling in every plotting layout. Reject invalid requests,
  unsupported EPS export and output paths that overwrite inputs.
- Generate phase specifications from nonuniform sample times without requiring
  an exact 20 ps row. Record reversals and returns toward zero after a complete
  quiet interval; validate data and options. Stability remains in seconds.
- Retire the obsolete noise-insertion preprocessor with an explicit error and
  native `.TEMP`/`.NEB` alternatives. It crashed on common netlists and used an
  incorrect noise clock. Its entry point now changes no files.
- Retire the unsafe `ivcurve.py` prototype. It overwrote existing temporary
  netlists and ignored simulator failures, repeatedly reading stale output.
  Its entry point now fails with model-IV/circuit-sweep alternatives and
  changes no files.
- Select output samples from direct print-grid targets with roundoff-safe ties.
  Cumulative target addition previously produced 9/10/11 ps output gaps for a
  requested 10 ps interval on a 1 ps internal grid. Nonintegral grids still
  select actual internal samples; no output interpolation is introduced.
- Reject excess NOISE/PNOISE/FNOISE/ICNOISE arguments instead of silently
  discarding them. Valid source waveforms are unchanged.
- Correct power-law noise weights and normalization, including exponent 0/2
  limits, missing relaxation tails and exact sampled-pole gain. Earlier rolling
  band-edge normalization could give several times the requested interior PSD.
  PNOISE, FNOISE and fractional ICNOISE share the corrected bank; seeded
  waveforms change. Add multi-seed PSD/variance, band-shift, timing, unit and
  both-mode fractional-current/clipping checks. Document finite-band accuracy.
- Correct `.NEB` for the time-averaged power of linearly interpolated white
  noise: the update interval is `1/(3*NEB)`, retaining the simulation-step floor.
  The old interval supplied only two-thirds of the requested effective power.
  Explicit `NOISE(... TSTEP)` retains its grid-variance convention. Seeded
  thermal and omitted-interval waveforms change; document sampling/aliasing.
- Apply JJ thermal noise using the current step's resistive slope, including
  the transition region, and rescale current interpolation samples immediately.
  Document the classical local-equilibrium approximation and branch-current
  trace meaning. Reject invalid temperatures/bandwidths cleanly; `.NEB 0`
  with automatic thermal noise previously aborted with `bad_optional_access`.
- Validate finite-cycle SIN/SINE frequency regardless of verbosity, reject
  excess arguments and show the complete syntax in diagnostics. Add analytic
  delay/phase/damping/cycle-boundary checks, including the exact #108 source.
- Validate JJ model domains before numeric casts and after instance scaling,
  including direct C++ model construction. Invalid RTYPE values no longer
  truncate silently or cause undefined behaviour. Preserve zero capacitance
  and signed/zero critical current in the ordinary sinusoidal model.
- Evaluate the finite-temperature transparent-junction CPR limit without NaN,
  and avoid cancellation near the zero-temperature cusp. Report singular or
  non-real CPR states explicitly. Check every solved unknown for finiteness
  and reject numerical overflow before output.
- Reject unsupported capacitor `IC=` requests explicitly; older builds silently
  ignored the requested voltage. Explicit charging intervals remain supported.
- Preserve voltage-mode nodal phase accumulated during startup, so node and
  junction phase traces agree. Phase sources now retain their actual preceding
  samples, fixing stateful noise differentiation and false t=0 voltage pulses
  after warmup. Keep cold-step impulses and seeded full-retry replay. Appended
  phase-source state requires C++ consumers to recompile.

- Read actual JJ phase/voltage histories during negative-time startup and at
  t=0. Voltage mode previously used node volts as phase history, creating a
  persistent wrong flux state in biased junction/inductor loops. Add an
  independent equilibrium regression for both modes and startup choices.

- Use one Josephson phase scale in JJ matrix and history equations. Separately
  rounded constants previously caused growing voltage-mode phase error during
  long runs (#91). Keep all physical constant values and unwrapped phase.
  Add raw-result phase/current/conservation regressions across durations,
  timesteps, bias polarities and both analysis modes.

- Bind named subcircuit parameters per instance, with optional header defaults,
  caller-evaluated overrides, nested scopes and dependent local expressions
  (#81). Preserve hierarchical aliases and legacy component-value overrides.
  Parse spaced/multiple assignments and reject duplicate instance assignments.
- Evaluate local JJ models per instance and prefer local/parent models over
  globals consistently, including positional gate detection. Type-level model
  IV generation uses definition defaults. Document the parameter-aware library
  overload and retained legacy calls.
- Repeated parameter parsing accepts already resolved values. Parameter updates
  now reset stored values and recompute dependencies; redefinitions replace
  the previous expression as their existing warning promises.

- Preserve actual pre-zero phase states when converting phase-mode output to
  voltage. The first two samples now obey the same BDF2 relation as later
  samples, including cold starts, floating terminals and filtered output.
  Clamp startup counts before integer conversion and reject unrepresentable
  startup intervals. Added result history requires C++ consumers to recompile.

- Current-source output now uses the samples applied during simulation, fixing
  incorrect NOISE/PNOISE traces from re-evaluating an advanced generator at past
  times. Capture only requested histories, including difference operands, and
  clear them on timestep retries. Evaluate each declared source once per step;
  this includes sources whose two terminals share ground. Appended result state
  requires C++ consumers to recompile.

- Validate direct-library transient point counts before integer conversion and
  matrix construction. Print windows after the last computed sample now fail
  with a transient diagnostic. Bound direct-library FIR windows consistently
  with the CLI and preserve valid last-sample output.

- Resolve hierarchical pin aliases, including nested/ground pins (#81), and
  preserve their requested trace labels. Unknown or malformed trace requests
  now fail explicitly. Restore the documented `C()` current alias and device
  trace differences; support voltage/phase across current-source terminals.
- Bind declared subcircuit pins before classifying ground. Only exact `0`/`GND`
  names are ground; names such as `GNDREF` no longer short to ground. Ground-first
  voltage differences and zero-valued ground traces work. Correct stale output
  labels in bundled examples. Added alias/difference fields require C++ consumers
  to recompile. Named instance-parameter overrides remain a separate #81 item.

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
  Windows C++ test processes locate the shared DLL in the build directory.
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
