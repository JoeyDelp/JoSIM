# JoSIM audit-resolution ledger

Status: **in progress**. [Initial measurements](qualification/2026-11-06-audit-baseline.md). Baseline: `53b6a426570eafb53d3673f79254157a4ae2c001`.

This ledger owns the disposition of the [original audit](qualification/2026-11-06-repository-audit.md) and continues the [consolidation qualification](qualification/2026-11-06-consolidation.md). Earlier passing tests do not close the remaining scientific or compatibility questions. A completed resolution must identify its implementation and measured verification; pending work stays open.

Baseline commands and logs are recorded under `build/audit-resolution/baseline/<compiler-configuration>/`. Each directory has exact commands/source identity in `run.json`, compiler/configuration/build/test logs, and CTest JUnit results. Generated reports will be published with the final qualification; local paths are not durable public evidence until linked to that publication.

## A01 — Same-process Gaussian RNG cache

- **Classification:** confirmed defect; prior repair.
- **Current reproduction/result:** Exact replay after odd/even draw counts,
  same-process independent runs and full-cycle timestep retry is protected.
  Retry includes white current, phase/flux and automatic thermal noise.
- **Required outcome:** Reseed/rewind after odd and even draws; independent runs and retries.
- **Implementation/owner:** 64df21c; src/Rng.cpp; test/noise_regression.cpp.
- **Verification:** ctest -R regression::noise.
- **Measurements/tolerances/evidence:** Draw counts 1–6 reset/replay exactly;
  retry results/history must match a fresh explicit-final-step run bit-for-bit.
  `regression::noise` and `regression::library` protect replay. Process-global
  ownership/concurrency limits are explicit in API.md. F07/F11/F16/F17 and their
  qualification reports record full numerical/build evidence; final integrated
  qualification remains required.

## A02 — State-dependent JJ thermal amplitude

- **Classification:** confirmed defect; prior stored-source repair incomplete.
- **Result:** F16 reproduces and repairs retained old interpolation amplitudes,
  missing transition amplitude and sampling before the active slope update.
  Both transition directions/polarities and six actual circuit cases pass.
- **Owner:** JJ::update_value, Function::return_noise, Simulation::handle_jj;
  test/noise_regression.cpp. Existing slope/thermal constants are retained.
- **Physical disposition:** classical local differential-conductance noise,
  S_I=4kBTG, explicitly not a microscopic nonequilibrium gap-noise model.
- **Evidence:** [white/thermal study](qualification/2026-11-06-white-thermal-noise.md).
  Six circuit cases give max 5.114e-13 A error against independent SI amplitudes;
  bound 1 ppm plus 1e-14 A. Full retry must replay bit-for-bit. Broader #107/#108
  measurement/example qualification remains separate. All ten native jobs
  pass for 078f091a1f53 in CI 42268024783, including default/shared libraries.

## A03 — Modern C compiler incompatibility

- **Classification:** confirmed defect; prior repair.
- **Current reproduction/result:** All four compile and pass 27/27; C11 and C++17 selected.
- **Required outcome:** Explicit compatible C/C++ standards, clean-cache build and compiler warnings.
- **Implementation/owner:** 08fd5b6; CMakeLists.txt.
- **Verification:** fresh GCC/Clang Debug/Release builds.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## A04 — Installed CMake package

- **Classification:** confirmed defect; prior repair.
- **Current reproduction/result:** Default/shared/static GNU16 builds pass
  30/30 each, including fresh consumers after moving the installation to a
  path with spaces. GNU8.3.1 in the pinned CentOS6.10 userspace also passes
  default/shared 30/30 each. Separate vendor shared-library setup failed from
  an unset PIC flag and duplicate DLONG symbols; shared JoSIM now explicitly
  embeds the working static KLU dependencies.
- **Required outcome:** Verify dependency discovery, relocation and every supported configuration.
- **Implementation/owner:** 30c8fbb; CMakeLists.txt; test_installed_package.cmake;
  test/consumer; tools/verify_library.py and native library CI matrix.
- **Verification:** ctest -R regression::installed_package.
- **Measurements/tolerances/evidence:** artifacts/audit-resolution/library-owner
  and library-legacy contain commands, source/toolchain identities and all test
  results. The relocated CLI runs and consumers check 1 V plus repeated error
  recovery; original prefix is absent. ELF loader path is $ORIGIN/../lib.
  Windows exports/DLL discovery and macOS loader paths require native CI;
  these are not accepted solely from the local Linux result.
  CI 42215518336 passes all five binary jobs and four native library jobs.
  Windows' installed consumers pass, but three build-tree C++ executables
  cannot locate josim.dll (0xc0000135). Scope DLL discovery to those CTest
  processes; require the native rerun before accepting this mode.
  Rerun 42216809204 for 4047755473 passes all ten binary/library-mode jobs.

## A05 — Silently ineffective SLU=ON

- **Classification:** confirmed supported-behaviour defect.
- **Current reproduction/result:** Reproduced acceptance of SLU=ON without an
  SLU compiler definition. Formally retired: configuration and manual macro
  requests fail explicitly; supported KLU code is unchanged.
- **Required outcome:** Repair and qualify, or retire with explicit configuration rejection.
- **Implementation/owner:** CMakeLists.txt; TypeDefines.hpp/LUSolve.hpp rejection
  guards; dependency_sources.cmake; regression::build_options. Removed unused
  SuperLU/CBLAS downloads/linkage/exports and compiled-out solver branches.
  Default class layout is retained; direct users of the removed vendor packages
  must supply them independently. This retires an inoperative backend rather
  than claiming untested numerical equivalence between solvers.
- **Verification:** Fresh-network GCC Release and Clang Release pass 29/29,
  including fresh installed consumers and explicit configuration rejection.
  Manual macro rejection is also covered. Default Release Simulation.cpp
  preprocessor output is byte-identical before/after retirement.
- **Evidence:** build/audit-resolution/backend-retirement-evidence.json,
  klu-only-tests.xml and fresh-klu/tests.xml; first-party numerical tests retain
  their existing tolerances. Final platform verification remains required.

## A06 — Moving dependency URL and stale linkage

- **Classification:** confirmed maintenance/portability defect; partial prior repair.
- **Current reproduction/result:** Fresh GCC Release acquisition/build passed.
  Filesystem link probes confirm GNU8 needs stdc++fs and GNU16 does not. Both
  default/shared installed consumers pass with the respective public link
  requirements. Linux rt linkage is retained for pre-glibc2.17 clock_gettime.
- **Required outcome:** Verify immutable acquisition and remove stale links only with compiler/runtime evidence.
- **Implementation/owner:** da8998d; cceac6c; cmake/dependency_sources.cmake.
- **Verification:** clean-cache configure/build; installed consumers; five-platform ABI checks.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## A07 — Invalid transient and recursive subcircuits

- **Classification:** confirmed defects; prior repairs.
- **Current reproduction/result:** Bounded cycle/negative-time checks pass.
  Additional direct-library non-finite/zero/overflow point counts lacked
  validation; print starts after the final actual sample produced a generic
  out-of-range error. Repaired and covered in F06.
- **Required outcome:** Clean finite/bounds errors, direct/indirect cycle rejection; no hangs/crashes.
- **Implementation/owner:** c36ea8a; 3a11500; src/Transient.cpp; src/Netlist.cpp.
- **Verification:** ctest -R regression::cli; sanitizers; malformed-input corpus.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## A08a — Named subcircuit parameter overrides (#81)

- **Classification:** confirmed compatibility defect; locally repaired.
- **Reproduction:** both supplied named-parameter circuits failed: RES was
  undefined without a default, and RES=10 in a header was treated as an IO pin.
  Preserved six red cases cover both modes and spaced header assignments.
- **Implementation:** bind parameters/models before descending into each
  instance. Evaluate overrides in the caller; resolve local defaults/derived
  parameters against bound values, enclosing instances and globals. Keep
  iterative expansion, structural cycle detection, IO/pin aliases, and legacy
  component-label overrides. Instance contexts are distinct from type defaults.
  Define exact semantics in syntax.md and the library migration in API.md.
- **Associated repairs:** local models now take deterministic precedence over
  globals; gate shape detection uses the same scoped lookup. Model IV commands
  resolve type defaults independently of instances. Re-parsing resolved
  parameters no longer fails; update_parameters resets actual stored values,
  and redefinitions now perform the replacement their warning promises.
- **Verification:** exact #81 circuits return 10/5 V (default-only 10 V) in both
  modes; independent/nested instances return 31/61 V and legacy overrides return
  6/9 V. Declared-name precedence returns 24 V. JJ currents at imposed pi/2
  phase are 200/400/100/200 uA for local and inherited models. Scoped positional
  gate tests give 50/100 uA. Scalar CSV tolerances are 2e-6 relative; gate checks
  allow 1e-10/2e-10 A (CSV and steady-state rounding). Type-level local model IV
  samples equal an explicitly identical global reference exactly, while the
  connected instance retains its different critical current. Malformed/duplicate,
  unresolved and cyclic expressions fail within bounded tests. Library and fresh
  installed consumers exercise both legacy and parameter-aware expansion, plus
  recoverable resolution failures. GCC16/Clang23 Debug and Release pass 30/30;
  final seven regression groups pass in all four configurations. Full
  ASan/UBSan/leaks passes 30/30 in 144.95 s; final binding/reparse checks also
  pass under sanitizers (7/7, 7.18 s). Commit 121ca3ed6439 passes all ten native
  binary/library jobs in CI 42230226316.
- **Evidence:** build/audit-resolution/named-parameters-red.log;
  named-parameters-four-config.log; named-parameters-tests.xml and final targeted
  library/gate/installed-consumer logs; named-parameters-sanitizer-tests.xml and
  named-parameters-final-sanitizer-tests.xml. Quantitative decks, sample hashes
  and source identity: artifacts/audit-resolution/named-parameters.json.
  No device/integration equations changed.

## A08b — Hierarchical pin-alias output (#81)

- **Classification:** confirmed compatibility defect.
- **Current reproduction/result:** The exact supplied circuit now resolves
  V(IN.X1) to the connected node and returns 10 V in both analysis modes.
  Nested/independent instances and ground aliases are retained. Invalid output
  requests throw; no successful result silently drops the requested trace.
- **Required outcome:** Resolve pin aliases and reject invalid requested traces explicitly.
- **Implementation/owner:** src/Netlist.cpp pin maps; RelevantTrace.cpp alias
  resolution; Output.cpp primitive trace differences. See F05 for accompanying
  ground-name/output defects, compatibility and measured evidence.
- **Verification:** hierarchical output and malformed trace regressions.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## A09 — Effective seed absent from -V 1

- **Classification:** confirmed observability defect; merged candidate.
- **Current reproduction/result:** Found that minimal output suppressed the
  effective seed despite the verbosity contract. Fixed and tested with an
  auto-seeded noisy run, extracting its seed and reproducing its CSV exactly.
- **Required outcome:** Verify explicit/auto seeds are exposed and usable for replay.
- **Implementation/owner:** src/Input.cpp: seed reporting; src/Verbose.cpp.
- **Verification:** CLI verbose/seed replay regression.
- **Measurements/tolerances/evidence:** regression::cli checks -m 1/-V 1 and
  --verbose=1, exact uint64 maximum reporting, and rejection of overflow,
  fractional/negative seeds. Both Release compilers pass 30/30; seven regression
  groups pass ASan/UBSan/leaks. See F03; same-build replay is an exact comparison,
  not a cross-platform bit-identity claim. Final native-platform checks pending.

## I110 — Original seed support report #110

- **Classification:** answered support request; distinct from A01.
- **Current reproduction/result:** Reporter confirmed v2.7; no issue mutation performed.
- **Required outcome:** Document required version and supported option; independently verify reporting/replay.
- **Implementation/owner:** https://github.com/JoeyDelp/JoSIM/issues/110.
- **Verification:** seed CLI and in-process tests.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## I109 — Capacitor initial voltage #109

- **Classification:** optional arbitrary-state feature; existing silent IC handling
  is a separate confirmed defect F12.
- **Disposition:** No arbitrary capacitor initial-voltage solver semantics are
  added. Define zero stored histories, finite negative startup and DST explicitly;
  use a physical charging interval before measurement. A future IC feature must
  initialize coupled states and histories consistently in both modes/on retries.
- **Alternative/verification:** test/comp/capacitor_precharge.cir charges for
  20 RC time constants, then discharges after a prescribed finite source fall.
  Independent closed-form response, KCL/KVL and three-step refinement pass in
  both modes. Voltage errors are 114.502/29.7314/7.59995 nV; KCL is zero at CSV
  precision. All four compiler/configurations pass 33/33; affected sanitizer
  groups pass with leak detection. All ten native jobs pass in CI 42255843921
  for descendant 7cbea62036df. F12 rejects unsupported IC syntax.
- **Evidence:** [support guide](model_limits.md#capacitor-voltage) and
  [model-boundary study](qualification/2026-11-06-model-boundaries.md) define
  equations, h^2 bounds, rounding allowance, commands and practical limits.

## I108a — Finite-cycle sine #108

- **Classification:** implemented request; analytically and native-qualified.
- **Result:** Exact requested SINE(0 315u 4g 20p 0 0 4) stops at
  20 ps + 4/(4 GHz) = 1020 ps. Source/resistor current and voltage match the
  analytic waveform in both modes, including inactive intervals, within
  1 nA/1 nV. Direct tests cover delay, damping, phase in degrees, aliases,
  fractional cycles, defaults, unlimited tones and both sides of cutoffs.
- **Additional repair:** F14 makes invalid finite-cycle frequencies fail
  independently of verbosity and rejects excess arguments; valid sine
  evaluation is unchanged. This is separate from I108b noise qualification.
- **Verification/evidence:** All four compiler/configurations pass 33/33;
  affected function/CLI ASan/UBSan/leak checks pass in 5.89 s.
  [Sine study](qualification/2026-11-06-sine.md), regression::functions/cli,
  build/audit-resolution/sine/. All ten native jobs pass for 18bb60c9c23b in
  CI 42259227651, including default/shared libraries.

## I108b — Switching-dependent noise #108

- **Classification:** support/model question; state-amplitude defects repaired in F16.
- **Disposition:** Classical local-slope FDT amplitude uses R0, transition slope
  or RN as selected by the active resistive region; no microscopic gap/shot-noise
  claim. Local temperature overrides global temperature. F16's transition and
  actual-injection checks pass all ten native jobs. Finite-cycle sine is I108a.
- **External flux/settings:** A finite-L mutually coupled loop and separate
  weak-noise zero-L SQUID reduction now have independent references. The 61-run
  study quantifies NEB/fJ, timestep, output sampling and physical limits.
- **Evidence:** [worked guide](squid_noise.md), [study](qualification/2026-11-06-squid-noise.md),
  tools/qualify_squid_noise.py and native numerical fixtures. Final implementing
  revision's native qualification remains pending; no universal NEB>fP claim.

## I107 — Floating reference and SQUID noise #107

- **Classification:** support/model question with validated bounded alternatives.
- **Result:** Floating primary voltage source works; removing the island's
  common-mode reference gives singular-matrix errors in both modes. Twenty-four
  flux cases cover two h/two modes/two polarities/three reference resistances;
  independent flux/KCL residual is <=3.713e-9 Phi0, below the 5e-7 budget.
- **Noise result:** Sixteen seeds at each NEB=20/200/2000 MHz match an independent
  weak-noise RSJ reference, with aggregate PSD ratios 1.020164/0.998104/0.992709.
  Nominal family 99% intervals plus predeclared 5% model allowance accept all.
  Halving h/print spacing changes PSD by <=0.1413%/0.0612%; both modes agree.
- **Disposition/evidence:** [worked guide](squid_noise.md) and
  [scientific study](qualification/2026-11-06-squid-noise.md) explain anchor
  loading/noise, NEB/interpolation/output and Josephson/plasma frequencies.
  Strongly overdamped classical example; no general bandwidth or quantum claim.
  F15/F19 repair bandwidth/output defects encountered. Native checks of the
  final example revision remain pending.

## I101 — JJ-buffer reverse response #101

- **Classification:** physical response established; separate startup defect F10 repaired.
- **Result:** Independent six-state Kirchhoff/RCSJ equations reproduce the active
  netlist's reverse bipolar response. B02 rotates negatively while B01 returns;
  reverse net output is -1.38349e-6 Phi0 per pulse. Exchanging ports with the
  same drive gives 0.999997568 Phi0 forward. The specified input is 4.3 ppm
  below exact SI Phi0. The original binary/settings are unknown; do not claim
  exact reconstruction of every screenshot sample.
- **Reference validation:** Tightened Radau and separate BDF solutions agree
  within 2.05e-10 scaled state units (budget 1e-8). Junction voltages remain
  below the original model's 2.75 mV transition boundary, validating R0 use.
- **Implementation/verification:** tools/qualify_buffer.py, independent fixtures
  under test/reference, regression::buffer; required by binary/library owners.
  Sixteen full-waveform comparisons cover both modes and four steps. Native
  CTest uses twelve cases/352 reference points per direction, KCL, loop flux,
  pulse area and refinement. GCC16/Clang23 Debug/Release pass 33/33; the new
  group passes ASan/UBSan/leaks in 16.53 s. All ten native binary/library jobs
  pass for descendant 7cbea62036df in CI 42255843921, covering this study.
- **Measurements/tolerances:** At 0.01 ps maximum output errors are 19.1501 nV
  reverse and 40.5556 nV forward, decreasing about fourfold per halving. Pulse
  areas allow 5e-6 Phi0; waveform limits scale with h^2 plus CSV rounding.
  See [the buffer study](qualification/2026-11-06-buffer.md) for equations,
  tolerances, input-variant caveat and source/binary provenance. Full local
  evidence: artifacts/audit-resolution/buffer-101/report.json and plots.

## I093 — Stewart–McCumber parameter #93

- **Classification:** optional model convenience; explicit supported alternative.
- **Disposition:** Use ordinary parameter expressions to set
  R=sqrt(beta_c*sigma/(Ic*C)). RTYPE=0 uses RN as its constant shunt. RTYPE=1
  has state-dependent conductance; R0/RN choices describe different damping
  assumptions, so no ambiguous global BETA_C field is added.
- **Verification:** test/comp/jj_beta_c.cir derives R for beta=0.2/1/5. Both modes
  satisfy Ic*sin(phi)+V/R+C*dV/dt, with current errors below 9.40e-13 A against
  1e-10 A allowance; KCL residual is zero. All four compiler/configurations pass
  33/33; affected sanitizer groups pass with leak detection. All ten native jobs
  pass for descendant 7cbea62036df in CI 42255843921.
- **Evidence:** [conversion guide](model_limits.md#beta-c) and
  [study](qualification/2026-11-06-model-boundaries.md); exact SI constants,
  positive-domain restrictions and zero-C boundary are explicit.

## I091 — Long-run JJ artifacts #91

- **Classification:** confirmed numerical cause repaired and native-qualified.
- **Result:** a controlled raw-precision study reproduces growing voltage-mode
  phase drift over four lengths, with mode/step/polarity comparisons. An
  independent 70-digit prediction establishes inconsistent matrix/history
  constants as the cause, rather than an inference from a single run.
- **Implementation:** use the existing SIGMA consistently in JJ::phaseConst_.
  See F09 for red/green measurements, tolerances and completed local checks.
- **Scope:** Constant-voltage and independent current-driven RSJ checks pass
  native qualification (see F09/D01). The original report has no circuit; do not
  claim to have reconstructed that undocumented case or explained all artifacts.
- **Evidence:** [phase accumulation study](qualification/2026-11-06-junction-phase.md);
  regression::junction_numerics. Other #101/thermal/noise studies remain separate.

## I088 — Nonlinear inductor #88

- **Classification:** optional substantial solver feature; outside current support.
- **Disposition:** JoSIM supports constant Phi=L*I. Define whether the requested
  L(I)=L0*(1+a*I^2) means differential inductance or Phi/I: their differential
  laws differ by a factor of three in the nonlinear term. No per-step L(I)
  substitution is advertised as a nonlinear solver. Full support requires a
  flux law/derivative, consistent initial bias/mutual semantics and convergence
  qualification in both modes.
- **Alternative/limits:** A constant-L approximation can be used only within
  an assessed excursion. For the differential interpretation, abs(a*I^2)<=0.01
  bounds departure from L0 to 1% at the element level; this does not bound
  long-line phase/gain error. Constant-L RL/RLC dynamics already have independent
  analytic/refinement tests. Large-signal nonlinear/JTWPA behaviour remains outside
  the supported model. See [the derivation](model_limits.md#nonlinear-inductance).

## I084 — Parameter sweeps #84

- **Classification:** scope/support decision.
- **Disposition:** `.PARAM` defines one run; range/Cartesian orchestration remains
  external. A safe one-marker helper supplies explicit fresh-process points,
  isolated output, checked inputs/failures and explicit time-weighted windows.
- **Result:** R=1/2/3/4/5 ohm at 1 A agrees with IR within 1 nV in both modes.
  Input/existing-temp/result-directory bytes remain unchanged; actual seed/grid
  are recorded. Native binary/library qualification requires regression::sweep.
- **Evidence:** [sweep guide](sweeps.md), [study](qualification/2026-11-06-sweeps.md),
  tools/circuit_sweep.py and test/param/sweep_resistance.cir.in. Four local full
  configurations pass 34/34; final focused/native checks recorded in F20.

## I082 — IV script and circuit sweeps #82

- **Classification:** confirmed unsafe prototype, plus support/model questions.
- **Result/disposition:** F20 reproduces an existing-temp-file overwrite and
  ignored failures/stale CSV reuse; retire the prototype with a safe explanatory
  entry point. Distinguish model `.IV`, fresh circuit points and one-transient
  history. A tested complete-circuit SQUID sweep is the practical alternative.
- **Measurements:** +/-200 uA gives +/-1.13153468 mV against independent
  +/-1.13137085 mV (0.0145% error, <=1% finite-window/timestep budget), both modes.
  Zero current gives zero voltage. Source/include/custom data and pre-existing
  files are preserved; failures and timeouts cannot produce a successful summary.
- **Evidence:** [guide](sweeps.md), [study](qualification/2026-11-06-sweeps.md),
  test/regression_sweep.py, tools/circuit_sweep.py and the retained red witness.
  Model-IV numerical protocol and broader gate/IV physical review remain
  explicit separate limits; no adaptive-error/Newton guarantee is asserted.

## D01 — Numerical regression breadth

- **Classification:** architecture/testing concern; partially qualified.
- **Current result:** Independent RC/RL/RLC ramp responses, KCL/KVL and Ohm's
  law pass for 36 cases: both modes, startup on/off, and 1/0.5/0.25 ps steps.
  At 0.25 ps maximum voltage error is 13.1534/13.1534/14.0995 nV;
  current error is 1.31534/1.31534/1.43369 nA. CSV KCL residual is zero;
  KVL residual is at most 5.00001e-10 V. Maximum error decreases by 3.4–4x
  per halving, with measured orders 1.75–1.84 (RC/RL), 1.99–2.00 (RLC).
- **Additional nonlinear reference:** Twelve current-driven RSJ cases test
  subcritical equilibrium and above-critical oscillations, both modes and three
  steps. Period errors are 567.504/142.585/35.6945 ppm and reduce about 4x per
  halving. Mean pulse area over 820–821 complete rotations agrees with exact SI
  within 0.092 ppm. See the [RSJ study](qualification/2026-11-06-rsj.md) for
  independent derivation, tolerances and explicit scope. All four compiler/config
  checks pass; ASan/UBSan/leaks passes all 36 junction cases in 33.58 s. Native
  qualification passes all ten native jobs for 04ef05788e7a in CI 42235902594.
- **Additional finite-capacitance reference:** The #101 buffer study covers
  two clocked input pulses, both directions/modes, continuous RCSJ references,
  phase-slip counts, flux area and step refinement. See I101 for measurements
  and pending native qualification. Six RC precharge/discharge and six explicit
  beta_c conversion cases now extend the numerical owner; see I109/I093.
- **Required outcome:** Complete final integrated qualification and resolve
  any remaining confirmed numerical defects. These bounded studies do not
  establish general scientific correctness.
- **Implementation/owner:** test/regression_numerics.py; release/library owners
  require its CTest group. No solver/device equations changed by this study.
- **Verification:** GCC16/Clang23 Debug and Release pass the numerical group.
  ASan/UBSan with leaks also passes all 36 cases (0.60 s). Commit c8a24427ac01
  passes all ten native jobs in CI 42231286897.
- **Evidence/tolerances:** [linear-circuit study](qualification/2026-11-06-linear-circuits.md)
  derives the independent continuous references, h^2-scaled bounds, refinement
  requirement and rounding allowances. artifacts/audit-resolution/rlc.json records
  exact decks, measurements and executable/test hashes. Logs/JUnit:
  build/audit-resolution/linear-numerics-*.log/.xml.

## D02 — Coverage and high-risk gaps

- **Classification:** testing/maintenance concern.
- **Current reproduction/result:** Initial measured line/function/branch coverage is 57.54%/72.81%/52.47%; all mappings inventoried; thresholds and high-risk gap closure remain pending.
- **Required outcome:** Reproducible line/function/branch reports and justified CI regression policy; assess all high-risk gaps.
- **Implementation/owner:** first-party src/ and include/JoSIM/.
- **Verification:** coverage build/export/report and CI check.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## D03 — Sanitizers, warnings and static analysis

- **Classification:** verification concern.
- **Current reproduction/result:** Full ASan/UBSan+leak suite passed 27/27 outside the process-restricted sandbox. Nine-owner static analysis produced 49 distinct locations to triage.
- **Required outcome:** ASan/UBSan representative runs; repair actionable findings and focused static-analysis warnings.
- **Implementation/owner:** first-party implementation and solver interfaces.
- **Verification:** instrumented suite and selected clang-tidy checks.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## D04 — Library recovery and RNG ownership

- **Classification:** embedding/API concern.
- **Current reproduction/result:** Missing `.iv` model terminated an embedding
  process (exit 255). Fixed to throw; repeated missing-model, singular and
  malformed-matrix failures recover and permit a fresh analytic 1 V run.
  RNG engines remain process-global, with an explicit single designated thread,
  sequential-run boundary; process parallelism is the supported alternative.
- **Required outcome:** Recoverable supported library errors; explicit reentrancy/concurrency boundaries with API/ABI review.
- **Implementation/owner:** src/Errors.cpp and Simulation.cpp; docs/API.md.
  Constructor-local KLU ownership releases symbolic/numeric allocations on
  failure/retry. Check sparse dimensions/indices/finite coefficients before KLU
  and check allocation/factorization results. Class layout is unchanged;
  new diagnostic enum values are appended. CLI termination helpers remain
  explicitly CLI-only. C++ ABI across versions/toolchains is not promised.
- **Verification:** regression::library exercises eight successive failure and
  valid-run cycles; GCC16/Clang23 Release pass 30/30. The expected DC result is
  1 V with 1e-12 absolute tolerance (linear solve rounding allowance).
- **Evidence:** build/audit-resolution/library-red.json, library-green-tests.xml
  and library-gcc-tests.xml. Full ASan/UBSan with leak detection passes 30/30
  (131.47 s); library-sanitizer-tests.xml. Final native-platform and RNG
  retry/replay qualification remain required.

## D05 — Maintained Python utilities

- **Classification:** repaired runtime/data/safety defects; obsolete noise
  preprocessor formally retired with native alternatives.
- **Reproduction/result:** Pandas 3 DAT loading, omitted image dimensions,
  comparison timelines, phase scaling, input overwrite and phase-spec startup/
  reversals failed in actual runtime probes. The noise helper crashed on common
  netlists and used an incorrect bandwidth clock. See the
  [utility study](qualification/2026-11-06-python-utilities.md).
- **Implementation/owner:** `scripts/plot_common.py`, all three plotters and
  `sp_generator.py`; `noise_insert.py` now fails explicitly without file changes.
  `tools/requirements-utilities.txt` pins modern NumPy/Pandas/Plotly/Kaleido/
  Tabulate. Maintained postprocessing needs no SciPy; scientific qualification
  tools use their separate pinned environment.
- **Verification:** isolated install plus
  `python -B test/regression_python_utilities.py --report <new-report.json>`;
  CI utility job is a tagged-release prerequisite.
- **Measurements:** 14/14 methods pass on Python 3.12.12; five layouts, four
  phase units, three HTML and eight actual static exports. Numeric scaling/
  round-trip allowance 1e-14; independent nonuniform phase counts and timestamps;
  source hashes preserved. Real JTL HTML/PNG/PDF/SP exports also pass.
- **Evidence:** artifacts/audit-resolution/python-utilities/report.json and
  jtl-exports.json; build/audit-resolution/python-utilities/{probes,tests.log}.
  Native-platform CI remains a separate final integration requirement.

  Native/utility CI 42286617926 passes all ten native jobs and the utility job
  for 096461bf8b. Subsequent source revisions require their own qualification.

## D06 — Runtime/memory and trace retention

- **Classification:** measured-performance concern.
- **Storage result:** Simulation reserves only engaged histories from its known
  sample count. Seven paired repetitions of eight fixtures (112 timed runs)
  retain identical CSVs and independent analytic/grid/seed checks. Peak RSS
  medians fall 102.77→89.45 MiB for long noise and 199.77→159.49 MiB for 32
  voltages; worst wall median change +0.4%. The initial 15% noise-only heuristic
  was missed (13% measured); the broader storage benefit and negligible runtime
  cost justify retention, explicitly recorded without relaxing numeric limits.
- **Contract:** raw double payload is 8*N*(1+retained unknowns+requested applied
  current histories), before other state/output/allocator memory. Decimation
  does not reduce retained internal history. Impossible vector sizes fail
  recoverably without real-memory pressure in the controlled boundary tests.
- **Owner/evidence:** Simulation::setup; library/CLI regressions; [runtime study](qualification/2026-11-06-runtime-storage.md)
  and artifacts/audit-resolution/performance/reserve/{summary,decision}.json.
  All four local configurations pass 34/34; full ASan/UBSan/leaks pass 34/34
  in 229.86 s. Native qualification remains required for this revision.
- **Copying result:** CPR/coupling hot-loop allocation counts drop from
  262,144/524,288 to zero in 2,048-step profiles. A separate seven-pair campaign
  holding reservation constant gives JJ/coupled-bank wall reductions of 15.3%/
  20.9%, noise 7.6%, exact CSVs and at most 1.4% wall/1.2% RSS increases elsewhere.
  Existing copy getters/layouts remain; Model adds a const-reference CPR accessor.
  All four local configurations pass 34/34; focused ASan/UBSan/leak groups
  pass 9/9 in 102.46 s. All ten native and the utility job pass for 9de8d69
  (CI42298764991). Evidence: performance/views/ receipts.
- **Progress result (F24):** replace polling with a notified 20 ms timed wait.
  Seven paired default-mode repetitions reduce coupled-RL/RLC CPU medians by
  50.5%/50.7%, with slightly lower wall time and identical CSVs. A five-sample
  run adds no median latency. Eight minimal-mode fixtures also retain identical
  CSVs; largest median wall increase 1.06%, below the predeclared 5% bound.
  Independent header compilation and two-worker lifetime failures are repaired;
  repeated completion/destruction and fresh installed consumers protect them.
  Recompile ProgressBar users because its synchronization state changes layout.
  Evidence: performance/progress/. Final integrated/native qualification remains
  required separately; storage scaling and model equations are unchanged.

## D07 — Source manuals and support claims

- **Classification:** documentation/maintenance concern.
- **Current reproduction/result:** Partially refreshed in 1866d03; full review pending.
- **Required outcome:** Current examples, API/ABI/build modes, fixed-step/restart limits and accurate supported behaviour.
- **Implementation/owner:** README.md; docs/; mkdocs.yml.
- **Verification:** documentation build/link/example/version checks.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## D08 — Versioning and qualified release

- **Classification:** publication/packaging concern.
- **Current reproduction/result:** Current version 2.7; v2.7 immutable. Release decision deferred until repairs qualified.
- **Required outcome:** Select justified new version after audit, build exact commit, publish and verify downloaded assets on relevant platforms.
- **Implementation/owner:** CMakeLists.txt; tools/build_release.py; docs/RELEASE_POLICY.md.
- **Verification:** five-platform release gates and download checksums/execution.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## D09 — Published GitHub Pages

- **Classification:** confirmed stale public documentation.
- **Current reproduction/result:** Open; retained site is older than current source.
- **Required outcome:** Build and deploy updated manual through existing process; externally verify navigation/examples/version/download links.
- **Implementation/owner:** gh-pages; mkdocs.yml; https://joeydelp.github.io/JoSIM/.
- **Verification:** MkDocs build plus deployed-site checks.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## D10 — Branches and preserved research

- **Classification:** completed consolidation; verify preservation.
- **Current reproduction/result:** 53b6a42 retained; remote master/gh-pages and preservation refs unchanged.
- **Required outcome:** Preserve established integrations, rejected modulo guard reasoning and archived precision/research history.
- **Implementation/owner:** docs/BRANCH_CONSOLIDATION.md; archive/* tags.
- **Verification:** ancestry/tag/stash identities and existing gate/noise regressions.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## D11 — Output and malformed-input failures

- **Classification:** repaired CLI/input/output defects, including newly
  reproduced false success after output-write failure (F21).
- **Result/owner:** F01–F06/F13/F14/F18 cover malformed waveforms, expressions,
  CLI, include/file normalization, requested traces, time grids and model/source
  domains. F03 preserves input files and aliases. F21 repairs final write/close
  checks in Output/IV and flush checks for standard output.
- **Verification:** `regression::cli`, `regression::functions`,
  `regression::library`, `regression::gate` and independent numerical groups;
  bounded timeouts and explicit diagnostics, not smoke tests alone.
- **Evidence:** [output study](qualification/2026-11-06-output-completion.md)
  and the individual F reports. Final integrated native/coverage qualification
  remains required; no arbitrary-input fuzzing claim is made.

## F01 — CUS waveform path, alignment and malformed data

- **Classification:** confirmed supported-behaviour defects uncovered by D03/D11.
- **Reproduction:** all six initial direct regressions failed: no-parent relative
  path, linear grid, delayed cubic, delayed repetition, fixed sample interval and
  empty/invalid definitions. The delayed-period case indexed a negative element.
- **Required outcome:** sample k at TD+k*TS; interpolation passes through samples;
  final sample holds to N*TS, then stops/repeats. Reject invalid inputs cleanly.
- **Implementation:** Function::parse_cus / return_cus; Errors CUS diagnostics.
  Existing enum values and Function object layout are preserved. Correcting the
  delayed/stretched waveform is an intentional bug fix, documented in syntax.md.
- **Verification:** regression::functions plus relative-file/stdin CLI tests;
  independent linear formula and natural-cubic midpoint 0.6875 for samples 0,1,0.
  Absolute tolerance 1e-11 bounds floating rounding in direct evaluations; CLI
  constant-voltage checks allow 1e-9 for CSV formatting.
- **Evidence:** build/audit-resolution/cus-red-test.log and cus-green-tests.xml;
  Full Clang Release suite passes 28/28; focused ASan/UBSan+leak checks pass
  functions and CLI. Final supported-platform results remain to be recorded.

## F02 — Numeric literals, expression precedence and malformed input

- **Classification:** confirmed numerical parsing and malformed-input defects.
- **Reproduction:** raw `1p` became 1, `1e-3`/`1.E-3` became -2; unary signs in
  products/exponents, function powers and LOG10 gave wrong values. `(2)3` hung
  in the old RPN reduction; empty/incomplete/mismatched expressions crashed or
  silently succeeded. The red regressions preserve these individual failures.
- **Required outcome:** standard scientific literals, case-consistent SPICE
  prefixes, mathematical precedence and finite real results; malformed input
  must terminate with a parsing error. Maintain scoped parameter resolution.
- **Implementation/compatibility:** src/Parameters.cpp parse_param keeps the
  shunting-yard/RPN architecture with explicit operand-state tracking, unary
  operators and checked stack arity. parse_operator implements advertised LOG10.
  Misc::modifier normalizes engineering suffixes and rejects malformed scalars.
  Public signatures, enum values and object layouts are unchanged. Formerly
  accepted invalid expressions now fail; incorrect valid results are corrected.
- **Verification:** independent literal/closed-form/parser-function comparisons,
  scoped/global operand tests and bounded malformed CLI regressions. Four clean
  GCC16/Clang23 Debug/Release builds passed 28/28. Full Clang ASan/UBSan with
  leak detection passed 28/28 in 143.98 s. Direct scalar tolerance is 2e-13
  relative with a 1e-300 absolute floor: conservative double/libm rounding
  allowance, never a solver tolerance. Gross scale/sign errors cannot pass it.
- **Evidence:** artifacts/audit-resolution/parser-repairs.json; exact candidate
  diff hash and run commands under build/audit-resolution/current/*/run.json;
  units-probe.log, numeric-red-test.log and numeric-sanitizer-tests.xml.
  All five native supported-platform jobs passed in CI run 42207795568 for
  ebd832afb83001508dbdc0e92720d56a07fee558.

## F03 — CLI parsing, input preservation, includes and seed bounds

- **Classification:** confirmed crashes, silent argument errors and destructive
  output collisions uncovered while resolving D11/A09/D04.
- **Reproduction:** --analysis=2, unknown switches, -m 2 and -V 999 returned
  success; an empty argument crashed. Relative output paths, hard links, .file
  output and output targeting included files overwrote netlists. Direct and
  indirect includes crashed. The bounded red suite records 21 failing cases.
- **Implementation:** CliOptions retains its pair representation with exact
  option names and intact values. Reject invalid numeric/flag arguments,
  unsupported output formats and retired solver/parallel requests. The OpenMP
  option only linked a runtime; it had no simulation implementation and is
  retired with an explicit configuration error. Preserve explicit stdin intent.
  Input records loaded netlists, maintains an exception-safe active include
  stack, and checks output identity using canonical paths and filesystem
  equivalence. Output and IV validate before writing. Verbose seed reporting
  works with minimal output; floating seed expressions must be strictly <2^64.
- **Compatibility:** new Input provenance/stack fields require embedding
  applications to recompile. Existing enum values are retained. Valid CLI long
  options now honour their values; invalid requests fail instead of continuing.
  Existing inputs are protected, including hard-link aliases. Unknown output
  suffixes no longer silently select CSV. KLU/numerical equations are unchanged.
- **Verification:** 30/30 GCC16 and Clang23 Release; all seven regression groups
  pass ASan/UBSan with leak detection (4.71 s). Content comparisons prove every
  input byte survives rejected output, including model-IV output. Auto-seeded
  thermal noise replays exactly after extracting -V 1's seed. Max uint64 literal
  is accepted; 2^64 and invalid seed expressions fail before conversion.
- **Evidence:** build/audit-resolution/cli-red-probes.json, cli-red-tests.log,
  cli-{green,gcc,sanitizer}-tests.xml. Final supported-platform verification and
  the remaining trace/malformed-input corpus stay open under A08b/D11.

## F04 — Filename case and logical-line normalization

- **Classification:** supported input defects exposed by platform qualification.
- **Reproduction:** CI 42212510827 passed four targets but failed AArch64's CUS
  relative-file test after the parser changed `wave.dat` to `WAVE.DAT`.
  Independently, six local cases with mixed-case basenames/directories fail
  through the same lossy normalization. Blank tab-only lines also crash.
- **Implementation:** normalize electrical syntax after joining continuations;
  preserve CUS paths and `.iv` output filenames. Retain the legacy lowercase
  CUS fallback, with resolved-path diagnostics on failure. Ignore whitespace
  before comments and reject a continuation without a preceding logical line.
  Existing C++ layouts and diagnostic enum values are retained.
- **Verification:** eight CUS basename/directory/spacing/continuation variants
  must produce 1 V (absolute tolerance 1e-9 for CSV rounding). Model-IV endpoint
  expectations remain unchanged and now check the requested filename case.
  Whitespace/comment input must simulate; leading continuations must fail
  normally. GCC16 and Clang23 Release pass 30/30.
- **Evidence:** build/audit-resolution/cus-case-red.log, whitespace-red.log,
  cli-arm64-failure.txt and cus-case-{gcc-,}tests.xml. All seven regression
  groups pass ASan/UBSan with leaks (5.08 s); cus-case-sanitizer-tests.xml.
  CI 42214652354 for fcb4e1f722 passes all five supported targets, including the
  previously failed AArch64 test. The original failure remains in the evidence.

## F05 — Pin aliases, ground classification and requested trace contents

- **Classification:** confirmed supported-behaviour defects; resolves A08b
  independently of the still-open A08a named-parameter issue.
- **Reproduction:** #81's pin request was dropped; nested/ground aliases were
  lost. Unknown/malformed requests often returned success. The red corpus
  records 13 failures. Names containing GND were treated as ground despite the
  documented exact-name rule; all six divider/name/mode cases fail before repair.
  Stricter validation also exposed stale example labels and the advertised C()
  alias/device differences being ignored or misinterpreted.
- **Implementation:** retain/combine per-instance pin maps through the existing
  bottom-up expansion, bind declared pins before ground classification, and
  resolve aliases in the trace owner without adding fake physical matrix nodes.
  Matrix uses exact ground names. Preserve public trace-handler signatures;
  render documented device differences from two existing primitive conversions.
  Add current-source terminal voltage/phase lookup and checked operand arity.
  Fix R2 (absent) to R3 in the output fixture and use the actual nested
  L6.X1.X1 component in the multiplier example; preserve its circuit equations.
- **Compatibility:** unknown or malformed requested traces now fail explicitly.
  GNDREF/AGND/etc are ordinary nodes. A declared GND port follows its instance
  connection, including a floating reference. Netlist/Subcircuit/RelevantTrace
  acquire alias/difference state; embedding consumers must recompile. Existing
  valid trace column labels and numeric/file formats are retained.
- **Verification:** original #81 circuit gives 10 V in both modes; nested and
  independent aliases give 10/20 V. A floating return gives 15/5 V and a 10 V
  differential. Divider expectations are 0.5 V/0.05 A. Independent series-network
  expectations check C(), device/source/mixed differences and KCL (source branch
  -0.625 A). Fixed phase sources give +/-0.9 rad differences. Absolute tolerance
  2e-6 in the trace's units allows CSV rounding, far below the reproduced errors.
  GCC16/Clang23 Release suites pass 30/30. Full ASan/UBSan with leaks passed
  30/30 in 137.19 s; added malformed-selector guards receive a targeted rerun.
- **Evidence:** build/audit-resolution/pin-alias-red.log, ground-names-red.log,
  pin-alias-{gcc-,}tests.xml and pin-alias-sanitizer-tests.xml. CI 42219314943
  for df54660779 passes all ten binary/library-mode jobs. Named overrides remain open.

## F06 — Checked library time grids and empty output windows

- **Classification:** confirmed input/recovery defects extending A07/D04/D11.
- **Reproduction:** direct simsize() with zero/non-finite timing did not throw
  before undefined integer conversion. `.tran 1p 5p 4.9p` and `1p 1.5p 1p`
  reached an empty output index vector and reported a generic internal bug.
- **Implementation:** Transient::simsize owns checked finite/positive/range
  conversion; Matrix and Simulation use it. validate_output checks the actual
  final grid point, and Output refuses empty data/index vectors. Direct FIR
  widths are clamped to the duration before converting tap counts, as in CLI.
  Public method names/signatures and object layouts are retained; the new
  output validator is an API addition. Valid grid/numerical equations unchanged.
- **Verification:** nine scalar invalid cases plus direct matrix rejection,
  empty print windows, exact final-sample output (4 ps, tolerance 1e-20 s), and
  huge finite FIR width preserving a 1 V constant (1e-9 V rounding allowance).
  GCC16/Clang23 Release pass 30/30; all seven sanitizer/leak regression groups
  pass in 5.23 s. CI 42221323012 for c5dc995e6c passes all ten native jobs.
- **Evidence:** build/audit-resolution/transient-library-red.json,
  transient-window-red.json, transient-{gcc-,}tests.xml and
  transient-sanitizer-tests.xml.

## F07 — Stateful current-source output history

- **Classification:** confirmed numerical output defect; repaired and native-qualified.
- **Reproduction:** seeded NOISE current into a single 10 ohm resistor prints
  source and resistor currents differing by up to 7.406399e-6 A. KCL requires
  equality. Output re-evaluates the source at past times after its generator
  has advanced to the end of the run, causing extrapolated values.
- **Required outcome:** output the samples actually applied during simulation;
  capture only requested source histories, include difference operands, retain
  timestep-retry/RNG behaviour, and test both orientations/modes/filtering.
- **Implementation:** capture requested applied current-source samples during
  handle_cs and consume Results::sourceCurrents in Output. Include both operands
  of differences; clear histories on retries. Each source is evaluated once per
  step, independent of orientation/output selection, including both-ground
  sources. The latter previously skipped evaluation and can therefore change
  the global noise stream when such a source exists. Results state is appended;
  existing field offsets remain, but consumers must recompile for its new size.
- **Verification:** 16 seeded white/power-law, orientation, mode and FIR/decimation
  cases have zero KCL residual at CSV resolution. Two non-ground terminal cases
  check differential-only source requests. Tolerance is 2e-6 relative +1e-20 A
  for CSV/linear-solve rounding. Requesting source output preserves the same
  resistor samples; unrequested histories stay absent. A forced full-cycle
  timestep retry retains exactly one final-run history and replays bit-for-bit
  against a same-process explicit final-step run. Raw retry KCL tolerance is
  1e-12 relative +1e-20 A (double solve rounding).
- **Results:** GCC16/Clang23 Release pass 30/30; all seven sanitizer/leak
  regression groups pass in 5.53 s. Broader PSD/amplitude qualification remains
  open; this item establishes applied current-sample provenance.
  Commit 30e311b4a690 passed all ten native jobs in CI 42224588150.
- **Owners:** Simulation::handle_cs, Results and Output::write_output.
- **Evidence:** build/audit-resolution/output-history-red.json, exact seed/deck
  and binary hash. Green measurements: artifacts/audit-resolution/current-source-history.json;
  source-history-{gcc-,}tests.xml and source-history-sanitizer-tests.xml. This is
  separate from the previously repaired Gaussian cache; the retry test now also
  protects that fix in a full same-process simulation.

## F08 — Initial phase-to-voltage output history

- **Classification:** confirmed numerical output/startup defect; repaired locally.
- **Reproduction:** an ideal constant 1 V source prints 0 V then 1.5 V in phase
  mode with startup (1.333333 V without startup), before settling to 1 V.
  Output fabricated the first two phase-history values. A valid tiny timestep
  of 1e-30 s also exposed an undefined startup-count conversion before clamping.
- **Implementation:** retain actual nodal phases at -h and -2h and use them in
  output's existing BDF2 conversion. Cold histories are zero. Capture only
  required voltage nodes with explicit traces, or all nodes for default results
  supporting later postprocessing. Clear history on retries. Clamp startup
  counts before integer conversion and reject an unrepresentable interval.
  No integrator or device equations changed. Appended Results state requires
  C++ consumers to recompile; API.md describes memory and postprocessing scope.
- **Verification:** eight constant-source cases cover warm/cold starts, a
  single point, FIR output and a 1e-30 s grid; all have zero error at CSV
  precision against 1 V (2e-6 V allowance for output rounding). Floating PWL
  terminal voltages match an independent piecewise formula. One/three-cycle
  phase steps integrate to one/three flux quanta in both modes, within 2e-6
  relative, including CSV rounding and the historical constants' precision.
  The reference uses exact SI h/(2e), independently of JoSIM's constants:
  https://physics.nist.gov/cgi-bin/cuu/Value?flxquhs2e.
  Repeated library/installed-consumer tests verify deferred trace selection.
- **Results:** GCC16/Clang23 Release pass 30/30; full ASan/UBSan with leak
  detection passes 30/30 in 136.28 s. Commit 100a64307bf1 passed all ten native jobs in CI 42227543389.
- **Owners:** Simulation results/history and Output phase differentiation.
- **Evidence:** build/audit-resolution/output-history-red.json,
  startup-overflow-red.json; artifacts/audit-resolution/phase-output-history.json;
  phase-history-{gcc-,}tests.xml and phase-history-sanitizer-tests.xml.
  Broader RLC, #91 and #101 studies remain open independently.

## F09 — Inconsistent JJ phase constants and long-run drift

- **Classification:** confirmed numerical defect, directly relevant to #91.
- **Reproduction:** ideal 1mV, rtype=0 JJ loses 1.61822 rad over 1m steps/10ns,
  and 161.771 rad over 10m steps/100ns, in voltage mode. Raw results avoid CSV
  rounding. Phase mode lacks this artificial decay. Four independent
  high-precision recurrence predictions match measured errors within 2.4ppm.
- **Cause/repair:** JJ matrix coefficients used HBAR/(2EV), while phase-history
  RHS used the separately rounded SIGMA. Derive both coefficients from the
  existing SIGMA; no constant values changed and no phase wrapping introduced.
- **Verification:** 24 raw double-result cases cover both modes, both bias
  polarities, three lengths and two timesteps. Assertions check Josephson's
  relation, steady constitutive current, KCL and voltage. GCC16/Clang23 Debug
  and Release pass 32/32. Full ASan/UBSan/leaks passes 32/32 in 175.12 s.
  Extended 10m-step voltage-mode drift falls from 161.771 to 8.736e-5 rad;
  phase mode is 8.844e-5 rad. Commit 69b9f1d3b6f passes all ten native jobs
  in CI 42233555152.
- **Evidence/tolerances:** [junction phase study](qualification/2026-11-06-junction-phase.md)
  derives the cause and rounding allowance, identifies the physical-constant
  precision separately, and points to preserved red/green measurements.

## F10 — Junction history during negative startup

- **Classification:** confirmed numerical startup defect; repaired and native-qualified.
- **Reproduction:** a zero-voltage, biased JJ/inductor loop settles in warm
  voltage mode to zero junction phases and IL2=-20uA. An independent KCL/flux
  equilibrium requires phi1=-0.00245600306rad, phi2=0.003274673rad and
  IL2=-15.088uA. The old state violates zero applied flux by 0.00759634rad.
  Cold voltage mode and both phase-mode startup choices agree with the reference.
- **Cause/repair:** handle_jj read its solved phase/voltage histories only for
  i>0. During negative startup and at i=0, voltage mode treated node volts as
  phase and held voltage history at zero. Read the previous solved values on
  every step; fresh cold-state vectors are already zero.
- **Verification:** eight independent static-loop cases cover both modes,
  startup choices and two steps. Expected values follow a scalar bisection of
  KCL plus flux linkage, using exact SI sigma. Phase/current/flux tolerances
  are 1e-8rad/2e-11A/1e-8rad, including CSV and legacy-constant rounding.
  Focused noise/gate/raw-junction/numerics groups pass. Independent #101
  transient probes now agree between modes and improve with refinement.
- **Evidence:** build/audit-resolution/jj-startup-red.log; issue-101/
  quiet-flux-red.json, startup-comparison.json, pre-startup-fix.json and probe.log.
  Original bulky transient CSV attempts were regenerated; compact red evidence
  and the original source/deck remain. GCC16/Clang23 Debug and Release pass
  32/32; full ASan/UBSan/leaks passes 32/32 in 182.98 s. All ten native jobs pass
  for 4bf3f47e12d1 in CI 42238371973, attempt 2. GitHub's first macOS arm64
  library attempt never obtained a runner; retry resolved that capacity failure.
  [Startup study](qualification/2026-11-06-junction-startup.md) gives the derivation,
  compatibility scope, values and tolerances. Green measurements:
  artifacts/audit-resolution/jj-startup.json.

## F11 — Phase-source and voltage-to-phase startup history

- **Classification:** confirmed supported-output/source defects; repaired and native-qualified.
- **Reproduction:** At 0.1 mV constant JJ bias, h=0.01 ps, warm voltage mode
  prints node phase about 3.03854 rad below the same junction's phase. A warm
  constant one-radian phase source injects a false 4.93659 mV pulse at t=0
  (h=0.1 ps), although the already established phase is constant. Seeded FNOISE
  phase-source output differs between modes by 0.00332625 rad and resistor
  current by 1.90786 uA in a controlled cold run.
- **Cause:** Output starts voltage-to-phase integration from zero despite
  negative startup. handle_ps re-evaluates stateful sources at past times after
  advancing them, and resets derivative history at i=0/1 after warmup.
- **Repair:** Retain requested nodal phase history in both modes and actual
  preceding phase-source samples, preserving cold-step semantics and full-retry
  replay. No constants/signatures change; appended source state needs consumers
  to recompile. See [the history study](qualification/2026-11-06-phase-history.md).
- **Evidence:** build/audit-resolution/phase-source/red.json, constant-red.json
  and cli-red.log; three new focused CLI tests fail in 21 subcases before repair.
  The first two records include exact decks and the qualified red binary hash.
- **Verification:** Floating/difference traces, FIR/downsampling, single-point
  and delayed output, warm/cold starts, deferred library output, and raw
  SIN/PWL/NOISE/PNOISE/FNOISE source samples pass. Raw phase/current bounds are
  1e-11 rad/1e-15 A. Same-process full-retry results must match a fresh final-step
  run exactly. GCC16/Clang23 Debug/Release pass 33/33; full ASan/UBSan/leaks
  passes 33/33 in 212.16 s. All three controlled red cases now agree between
  modes at CSV precision. Green measurements and source/binary hashes:
  artifacts/audit-resolution/phase-source/measurements.json. All ten native jobs
  pass for descendant 7cbea62036df in CI 42255843921.

## F12 — Silently ignored capacitor initial-voltage syntax

- **Classification:** confirmed user-input defect; repaired and native-qualified.
- **Reproduction:** Adding IC=1m or IC = 1m to an uncharged RC circuit succeeds
  and produces the same zero voltage as no IC option. It does not apply the
  requested initial state. Exact red decks/output/binary hash:
  build/audit-resolution/optional-features/capacitor-ic-red.json.
- **Repair:** Capacitor construction rejects a trailing IC assignment with a
  useful diagnostic and recommends an explicit charging interval. This does
  not implement IC semantics. A node named IC remains valid.
- **Verification:** Six CLI rejection cases (both modes, three spellings),
  recoverable direct-library error followed by valid runs, and independent
  precharge/discharge alternatives. Red CLI assertions fail all six before
  repair. GCC16/Clang23 Debug/Release pass 33/33; affected CLI/numerics/library
  ASan/UBSan/leak checks pass in 5.19 s. All ten native jobs pass for descendant
  7cbea62036df in CI 42255843921.
- **Evidence:** optional-features/ic-red.log and candidate.patch; the
  [boundary study](qualification/2026-11-06-model-boundaries.md) defines support
  and quantitative regression acceptance.

## F13 — Model domains, transparent CPR and non-finite solver states

- **Classification:** confirmed model/input/numerical defects; repaired and native-qualified.
- **Reproduction:** RTYPE=1e30 triggers UBSan floating-to-integer overflow;
  fractional/unknown RTYPE silently changes behaviour. Invalid D=2 and valid
  D=1 at finite T/phase pi report success with NaN current. Extreme finite V/R
  reports success with infinite solved current. Zero-T near-cusp tests also
  expose cancellation in 1-sin^2. Four initial methods fail in 39 subcases;
  four further near-cusp subcases fail before the stable expression.
- **Repair:** Validate parsed/direct models before casts and after effective
  scaling. Evaluate tanh(alpha*q)/q using its finite positive-T limit; use an
  equivalent single-harmonic cosine radicand near the zero-T cusp. Preserve D=1,
  zero C and signed/zero sinusoidal Ic. Diagnose singular/non-real CPR states.
  Revalidate updated matrices and require every solved unknown to be finite.
  No constants/layouts/formats change; Model::validate adds a nonvirtual method.
- **Verification:** 120 independent steady phase-bias cases cover four D values,
  three temperatures and five phases in both modes. Maximum current error is
  4.793e-12 A; limit 3 ppm plus 1 pA (largest error/budget 0.0534). Zero-T
  one-sided currents at pi +/-1e-8 are +/-200 uA for Ic=100 uA. Domain, overflow,
  direct-model and repeated recovery tests pass. GCC16/Clang23 Debug/Release
  pass 33/33; final full ASan/UBSan/leaks passes 33/33 in 180.87 s.
- **Evidence:** [model-validity study](qualification/2026-11-06-model-validity.md)
  derives limits, domains and numeric-cusp handling. Exact red inputs/sanitizer
  witnesses: build/audit-resolution/model-validation. Green measurements/source
  hashes and compact compiler/sanitizer receipts:
  artifacts/audit-resolution/model-validation/. All ten native jobs pass for
  7cbea62036df in CI 42255843921, including default/shared libraries.

## F14 — Verbosity-dependent finite-sine validation

- **Classification:** confirmed waveform-input defect; repaired and native-qualified.
- **Reproduction:** Finite-cycle SINE with zero/negative frequency succeeds at
  -V 0 and fails at -V 1. Its period and stop time are undefined. Excess fields
  were silently discarded. Red verbosity/exit witnesses and failed tests:
  build/audit-resolution/sine/red.json and red-tests.log.
- **Repair:** Require positive frequency whenever NCYCLES>0, independently of
  logging; enforce the documented seven-field maximum and update error help.
  Valid source evaluation/signatures/layouts are unchanged.
- **Verification:** Independent analytic waveform and exact #108 circuit
  assertions in function/CLI groups. GCC16/Clang23 Debug/Release pass 33/33;
  affected ASan/UBSan/leak checks pass in 5.89 s. All ten native jobs pass for
  18bb60c9c23b in CI 42259227651.

## F15 — Interpolated white-noise effective bandwidth

- **Classification:** confirmed noise-normalization defect uncovered in A02/I107.
- **Reproduction:** NEB=64 Hz gives measured time-averaged variance 42.7041
  instead of 64 for unit ASD. Linear interpolation requires B=1/(3*dt), while
  the old interval 1/(2*NEB) supplies only two-thirds of the requested power.
- **Repair:** One interval owner uses max(h,1/(3*NEB)) for omitted-TSTEP manual
  white noise and automatic resistor/JJ noise. Explicit TSTEP is preserved.
  Reject invalid/unrepresentable bandwidths; correct stale default documentation.
- **Verification:** Eight seeds give variance 63.8564268, 0.2243% below 64;
  predeclared tolerance 5%, relative SE below 0.6%. Six Welch PSD ratios to
  analytic sinc^4 lie 0.969538–1.016064, within a justified 30% statistical
  bound. Four sampling phases/interval ratios check grid versus time variance.
- **Evidence/compatibility:** [study](qualification/2026-11-06-white-thermal-noise.md),
  test/noise_regression.cpp; artifacts/audit-resolution/white-thermal-noise/.
  New static API method, no layout change; corrected NEB changes seeded waves.
  GCC16/Clang23 Debug/Release pass 33/33; full ASan/UBSan/leaks pass 33/33 in
  198.80 s. All ten native jobs pass for 078f091a1f53 in CI 42268024783.

## F16 — Thermal sample scaling, transition state and invalid controls

- **Classification:** confirmed amplitude/timing and input-abort defects.
- **Reproduction:** Amplitude changes leave current interpolation values scaled
  by the preceding amplitude. Transition amplitude depends on previous R0/RN.
  `.TEMP 4.2` with `.NEB 0` aborts with bad_optional_access.
- **Repair:** Retain unit-ASD white knots; apply current amplitude immediately.
  Set transition amplitude from its stated classical slope model and update
  before one sample is injected into the physical terminals. Validate controls
  and direct API values with recoverable errors. Amplitude rescaling adds no
  random draw; both-ground junctions now advance their thermal source too.
- **Verification:** Both transition directions/polarities, six actual KCL
  injection cases, zero/restored amplitude, invalid-domain recovery and exact
  fresh-final-step replay. Max injection error 5.114e-13 A; allowance 1 ppm
  plus 1e-14 A. Physical approximation and branch trace meaning are explicit.
- **Evidence:** stochastic/{red,invalid-red}.json and failed pre-fix assertions;
  [study](qualification/2026-11-06-white-thermal-noise.md). All four local
  configurations pass 33/33; full ASan/UBSan/leaks pass 33/33 in 198.80 s.
  All ten native jobs pass for 078f091a1f53 in CI 42268024783; broader
  colored/SQUID work remains separate.

## F17 — Power-law normalization, endpoints and sampled-pole gain

- **Classification:** confirmed stochastic-normalization defect.
- **Reproduction:** Eight seeds in the 1–256 Hz band give PSD/ideal at 16 Hz
  1.929505 for alpha=1 and 7.916022 for alpha=2. Nearest-edge normalization
  anchors to rolloff; missing relaxation tails distort endpoint exponents.
- **Repair:** Keep OU/AR(1) architecture, derive weights from the power-law
  integral, fold missing tails into endpoint poles, use analytic alpha=0/2
  limits and compensate exact sampled-pole DC gain. One initializer owns
  PNOISE/FNOISE/ICNOISE. Syntax/layouts/units retained; seeded waves change.
- **Verification:** Eight seeds/five exponents/three frequencies give ideal-PSD
  ratios 0.865612–1.082230; justified predeclared 40% short-run statistical
  limit. Independent endpoint covariance variances agree within 0.9%.
  Band shifts across 1 Hz obey c^(1-alpha) variance scaling; delays, skipped
  ticks, timestep floors and N4/48/512 common-time replay pass. Flux units and
  dimensionless ICNOISE identity pass; actual gated fractional current/clipping
  in both modes agrees within 1e-10 A.
- **Approximation:** Exact finite-bank spectrum scan gives default N48 interior
  ideal-PSD ratios 0.975331–1.000137 for specified 2–8-decade bands/exponents.
  N4 may err about 50%; near-edge/narrow-band rolloff limits are explicit.
- **Evidence:** [colored-noise study](qualification/2026-11-06-colored-noise.md),
  test/noise_regression.cpp; build/audit-resolution/colored-noise and compact
  artifacts/audit-resolution/colored-noise. GCC16/Clang23 Debug/Release pass
  33/33; full ASan/UBSan/leaks pass 33/33 in 210.03 s. All ten native jobs
  pass for cb594a34d5dd in CI 42270298099, including default/shared libraries.

## F18 — Silently discarded noise-source arguments

- **Classification:** confirmed malformed-input defect; separate diagnostic improvement.
- **Reproduction:** All four NOISE/PNOISE/FNOISE/ICNOISE excess-argument cases
  succeed and silently discard the extra field. Four CLI red subcases fail.
- **Repair:** Enforce documented maxima of 3/6/7/6 arguments, using existing
  recoverable noise diagnostics. Valid waveforms/signatures/layouts unchanged.
- **Verification:** Direct function-domain rejection and clean CLI failure with
  no output file. Verbose diagnostics additionally expose the effective final
  timestep and computed count: 2 ps requested reduces to 0.25 ps/512 points
  over 128 ps in both modes. The reporting improvement enables exact scientific
  provenance; it does not alter solver settings.
- **Evidence:** build/audit-resolution/noise-input/{red,green}-tests.log,
  regression::noise/cli and [SQUID study](qualification/2026-11-06-squid-noise.md).
  Full four compiler/configurations pass 33/33; full ASan/UBSan/leaks pass
  33/33 in 209.52 s. Native final-revision qualification pending.

## F19 — Roundoff jitter in decimated output timestamps

- **Classification:** confirmed output-sampling defect uncovered in I107.
- **Reproduction:** A 1 ps internal grid and 10 ps requested print interval
  emit 9/10/11 ps gaps, up to 1 ps off target. Floating target accumulation
  crosses exact internal-grid ties inconsistently. Actual recorded time/value
  pairs are coherent, but the intended integer-ratio print grid is not uniform.
- **Repair:** Form each target directly with fma and a few-ulp tie allowance;
  preserve selection of actual internal samples and avoid duplicate first
  indices. No output interpolation, device equation or column-format change.
- **Verification:** Both modes/integer and fractional starts check all 1990
  timestamps against independent integer-index expectations and analytic ramp
  values. The scientific SQUID campaign now uses verified uniform timestamps;
  PSD refinement bounds pass. General nonintegral grids remain nonuniform.
- **Evidence:** squid-example/print-grid/{red.json,red-tests.log,green-tests.log};
  [study](qualification/2026-11-06-squid-noise.md). All four local configurations
  pass 33/33; full ASan/UBSan/leaks pass 33/33 in 209.52 s. Native pending.

## F20 — Unsafe positional IV prototype and fresh circuit sweeps

- **Classification:** confirmed utility overwrite/failure-handling defect.
- **Reproduction:** Existing input_tmp.cir is overwritten. A task-owned simulator
  wrapper runs the real CLI once then returns17; the prototype makes 292 calls
  in five seconds while reusing the first CSV. Exact hash/deck/log witness:
  sweeps/unsafe-iv/red.json. Original main file remains unchanged in this probe.
- **Repair/disposition:** Formally retire the positional prototype; entry point
  returns2 with useful model/circuit alternatives and modifies nothing. The
  replacement one-parameter helper preserves templates/includes/custom data,
  preflights output directives, isolates point outputs and checks every result.
  Average by time, including explicit interpolated boundaries; no stale reuse.
- **Verification:** Nine focused tests cover both-mode five-R/whole-SQUID
  references, source/existing-file preservation, include/CUS/continuation paths,
  data hashes, output-control/CR normalization, invalid/cyclic input, failed
  simulator/timeout witnesses, and independent nonuniform-ramp integration.
- **Compatibility/evidence:** stdlib-only new helper; model `.IV` retained;
  retired script no longer simulates. New required regression::sweep group.
  All four local full configurations pass34/34; final nine-test sweep group
  passes all four and ASan/UBSan/leaks in4.02s. No production C++ change;
  preceding full sanitizer33/33 passed209.52s. Native pending.
  [Study](qualification/2026-11-06-sweeps.md) owns bounds.

## F21 — False success after output-write/close failure

- **Classification:** reproduced supported-behavior defect, repaired.
- **Reproduction:** CLI CSV/DAT/RAW, `.FILE`, model `.iv` and standard output
  each returned 0 when a destination opened but rejected writes. Controlled
  `/dev/full` probes used only task-owned symlinks; no disk was filled.
- **Implementation:** Output::format_csv_or_dat/format_raw explicitly close and
  check streams; format_cout flushes/checks; IV::write_iv checks close state.
  Appended OutputErrors::WRITE_FAILED retains existing enum values and signatures.
- **Verification:** regression::cli six Linux fault cases now return 255 with
  destination diagnostics; portable regression::library fails only at flush,
  catches/reuses output state and runs valid simulations in eight cycles.
- **Measurements:** GCC 16.2.1/Clang 23.1.4 Debug/Release all 34/34; full Clang
  ASan/UBSan/leaks 34/34 in 211.84 s. No numerical tolerances changed. Native CI
  remains required for the pushed revision; non-Linux file fault injection is
  not inferred from the portable stream test.
- **Evidence:** [output study](qualification/2026-11-06-output-completion.md),
  artifacts/audit-resolution/output-write/{red,stdout-red,green}.json,
  source-sha256.json, four build/test receipts and sanitizer JUnit.
  All ten native jobs plus the utility job pass for 978fe82f in CI 42288359410.

## F22 — Model-IV timestep retry loses the requested bias

- **Classification:** reproduced numerical/control defect, repaired.
- **Reproduction:** Ic=C=0, RN=100 Ω, Imax=1 mA, STEP=1 mA forces repeated phase-guard
  retries. All four rows incorrectly became 2.5 µA/250 µV, returning success.
- **Implementation:** IV::generate_iv keeps each generated PWL declaration and
  active function synchronized. Input remains the source of retry reconstruction.
  No signature/layout/timing-default or physical-equation change.
- **Verification:** regression::gate four ordinary/gated/caller-mode cases assert
  Ohm's law and requested [+1 mA, 0, −1 mA, 0] points: [+0.1 V, 0, −0.1 V, 0], with 1e-12 A
  and 1e-8 V absolute bounds for exactly representable CSV reference literals.
- **Measurements:** four local configurations pass 34/34; affected ASan/UBSan/leak groups 4/4 in 34.52 s.
  Full sanitizer 34/34 in 211.84 s immediately precedes this targeted IV repair;
  final integrated full/native qualification remains required.
- **Evidence:** [IV retry study](qualification/2026-11-06-iv-retry.md),
  artifacts/audit-resolution/iv-physics/retry-{probe,green}.json, source hashes,
  four build/test receipts and retry-sanitizer.xml.
  All ten native jobs plus the utility job pass for a1c0fa10 in CI 42289758492.

## F23 — Model-IV initial state and finite averaging protocol

- **Classification:** reproduced device-history defect repaired; required model-IV
  timing refinement implemented and independently qualified.
- **Reproduction:** RTYPE=0, Ic=100 µA, C=0, RN=1 mΩ, Ib=±150 µA; independent RSJ DC mean
  ±1.118034e-7 V has period 18.5 ns, far longer than the fixed 500 ps protocol.
  Default means +1.351658e-7/−1.384165e-7 V differ by 21–24% from the converged
  DC mean, and paired excursions are not exactly odd. The latter is inconsistent
  with the odd constitutive equation and sign-reversed generated ramps from
  zero state. Rebuilding a fresh Matrix per point alone removes the 3.2507 nV
  outward and 1.385687 nV returning symmetry residuals, isolating cached histories.
- **Implementation:** fresh per-point devices plus TSTEP/TSTOP/TSTART controls,
  default finite protocol retained; verbose effective grid/count diagnostics.
  Existing API signatures/layouts remain; positive prstart now selects the
  averaging window. At least two samples are required; no automatic DC guarantee.
- **Measurements:** ordinary slow-RSJ long mean 111.7475 nV vs 111.8034 nV (−0.0500%);
  ±5 mV gate 141.4591 nV vs 141.4214 nV (+0.0267%); full suppression 150 nV exactly.
  Independent finite-window Phi0/W + 0.1 nV discretization allowance gives 0.617 nV;
  h=20→10 ps difference 0.0002 nV. All eight campaign runs pass, source/binary unchanged.
  All four local configurations pass 34/34; full ASan/UBSan/leaks pass 34/34 in 237.00 s. Native/final
  integrated qualification remains required for the pushed revision.
- **Evidence:** [model-IV study](qualification/2026-11-06-model-iv-physics.md),
  artifacts/audit-resolution/iv-physics/state-{red,green}.json and timing/
  report.json with exact owner/decks/CSV/logs, source hashes and build/test receipts.

## F24 — Busy progress workers and incomplete thread cleanup

- **Classification:** confirmed runtime/header/lifetime defects.
- **Reproduction:** default progress consumes an extra CPU; standalone header
  fails compilation; destruction after two workers aborts with an unjoined thread.
- **Repair:** ProgressBar waits on a condition variable between display updates,
  wakes on completion and joins all joinable workers. Repeated completion is
  safe. Required includes are explicit; internal callers configure before start.
- **Compatibility:** existing signatures remain; public synchronization state
  changes layout. Recompile consumers. This is display scheduling, not solver
  parallelism; numerical operation order and output formats are unchanged.
- **Verification:** repeated library worker lifetimes and fresh installed header
  consumer; all four local compiler/configuration suites pass 34/34. Full Clang
  Debug ASan/UBSan/leak detection passes 34/34 in 211.29 s.
- **Measurements:** 42 default-mode timed runs and 112 minimal-mode timed runs,
  seven counterbalanced pairs per fixture, all CSVs identical. Medium CPU falls
  50.5%/50.7% against a predeclared 40% target; wall remains below the 5%
  regression bound. Tiny median latency decreases within its 5 ms allowance.
- **Evidence:** [runtime study](qualification/2026-11-06-runtime-storage.md),
  artifacts/audit-resolution/performance/progress/{decision,source-sha256}.json,
  both campaign summaries/per-run receipts and header/lifetime red/green logs.
  Native and final integrated checks remain separate requirements.

## F25 — Coupled-inductor component timestep hook

- **Classification:** confirmed C++ component-helper defect, not a CLI retry defect.
- **Reproduction:** after appending two mutual terms, halving the timestep leaves
  the self coefficient at -1500 instead of -3000 and scales only the last mutual
  coefficient. An independently compiled installed-library probe returns 1.
- **Repair:** locate the own-current stamp and scale it plus every appended
  mutual-current term. Nodal coefficients and all phase-mode terms stay fixed.
  Existing signatures/layouts and normal full-Matrix retry reconstruction remain.
- **Verification:** library regressions cover both modes, all four node topologies,
  two signed mutual terms, the independent -3L/(2h) or -L/SIGMA equations and
  exact reversal after factors 0.5 then 2. Relative allowance 1e-12 accommodates
  coefficient evaluation roundoff; eight repeated runs protect reuse.
- **Boundary:** this legacy method edits component stamps only. It does not update
  CSR storage or histories and does not add variable-step simulation support.
- **Results:** GCC Release affected groups pass 4/4 in 4.46 s; Clang Debug
  ASan/UBSan/leak checks pass the same four groups in 27.92 s.
- **Evidence:** artifacts/audit-resolution/inductor-hook/ red/green receipts and
  test/library_regression.cpp::inductor_timestep_stamp. Final full/native checks
  remain required for the integrated source.
