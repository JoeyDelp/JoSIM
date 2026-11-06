# JoSIM audit-resolution ledger

Status: **in progress**. [Initial measurements](qualification/2026-11-06-audit-baseline.md). Baseline: `53b6a426570eafb53d3673f79254157a4ae2c001`.

This ledger owns the disposition of the [original audit](qualification/2026-11-06-repository-audit.md) and continues the [consolidation qualification](qualification/2026-11-06-consolidation.md). Earlier passing tests do not close the remaining scientific or compatibility questions. A completed resolution must identify its implementation and measured verification; pending work stays open.

Baseline commands and logs are recorded under `build/audit-resolution/baseline/<compiler-configuration>/`. Each directory has exact commands/source identity in `run.json`, compiler/configuration/build/test logs, and CTest JUnit results. Generated reports will be published with the final qualification; local paths are not durable public evidence until linked to that publication.

## A01 — Same-process Gaussian RNG cache

- **Classification:** confirmed defect; prior repair.
- **Current reproduction/result:** Existing replay tests pass in four fresh builds; retry/ownership coverage remains open.
- **Required outcome:** Reseed/rewind after odd and even draws; independent runs and retries.
- **Implementation/owner:** 64df21c; src/Rng.cpp; test/noise_regression.cpp.
- **Verification:** ctest -R regression::noise.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## A02 — State-dependent JJ thermal amplitude

- **Classification:** confirmed defect; prior repair.
- **Current reproduction/result:** Stored-amplitude repair present; expanded transition measurements pending.
- **Required outcome:** Verify both transitions and justify transition-region model separately.
- **Implementation/owner:** 505abca; src/JJ.cpp; include/JoSIM/Function.hpp.
- **Verification:** thermal-amplitude library regression and stochastic circuit checks.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

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

- **Classification:** optional feature request.
- **Current reproduction/result:** Disposition and alternative pending; do not advertise unsupported IC syntax.
- **Required outcome:** Define supported startup boundary and tested practical alternative, or implement explicit IC semantics.
- **Implementation/owner:** https://github.com/JoeyDelp/JoSIM/issues/109; src/Capacitor.cpp.
- **Verification:** startup/history analytic regressions.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## I108a — Finite-cycle sine #108

- **Classification:** implemented request requiring validation.
- **Current reproduction/result:** Baseline syntax support present; focused assertions pending.
- **Required outcome:** Test delay, phase, cycle count and stop independently of noise discussion.
- **Implementation/owner:** src/Function.cpp; https://github.com/JoeyDelp/JoSIM/issues/108.
- **Verification:** analytic sine waveform/output regression.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## I108b — Switching-dependent noise #108

- **Classification:** physical-model/support investigation.
- **Current reproduction/result:** Open scientific validation; distinct from finite-cycle sine.
- **Required outcome:** Resolve A02 and quantify transition assumptions/noise measurement settings.
- **Implementation/owner:** src/JJ.cpp; docs/noise_sources.md.
- **Verification:** state-transition and validated SQUID/noise example.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## I107 — Floating reference and SQUID noise #107

- **Classification:** support/model validation.
- **Current reproduction/result:** Open; no blanket bandwidth claim accepted.
- **Required outcome:** Validated floating-reference/noise example with NEB, sampling, timestep and Josephson frequencies.
- **Implementation/owner:** https://github.com/JoeyDelp/JoSIM/issues/107.
- **Verification:** documented example with quantitative measurement.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

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
  group passes ASan/UBSan/leaks in 16.53 s. Native publication pending.
- **Measurements/tolerances:** At 0.01 ps maximum output errors are 19.1501 nV
  reverse and 40.5556 nV forward, decreasing about fourfold per halving. Pulse
  areas allow 5e-6 Phi0; waveform limits scale with h^2 plus CSV rounding.
  See [the buffer study](qualification/2026-11-06-buffer.md) for equations,
  tolerances, input-variant caveat and source/binary provenance. Full local
  evidence: artifacts/audit-resolution/buffer-101/report.json and plots.

## I093 — Stewart–McCumber parameter #93

- **Classification:** optional model convenience.
- **Current reproduction/result:** Disposition pending.
- **Required outcome:** State resistance convention and test a conversion example or implement supported semantics.
- **Implementation/owner:** https://github.com/JoeyDelp/JoSIM/issues/93.
- **Verification:** beta_c conversion/example numerical test.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

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

- **Classification:** optional substantial solver feature.
- **Current reproduction/result:** Disposition pending.
- **Required outcome:** Define flux-linkage semantics/support boundary and practical alternative without promising L(I) substitution.
- **Implementation/owner:** https://github.com/JoeyDelp/JoSIM/issues/88.
- **Verification:** technical disposition and justified example if applicable.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## I084 — Parameter sweeps #84

- **Classification:** scope/support decision.
- **Current reproduction/result:** Example/policy pending.
- **Required outcome:** Document external sweep policy with tested safe parameter example.
- **Implementation/owner:** https://github.com/JoeyDelp/JoSIM/issues/84.
- **Verification:** five-resistance analytic sweep example.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## I082 — IV script and circuit sweeps #82

- **Classification:** confirmed unsafe utility plus support questions.
- **Current reproduction/result:** Open; script rewrites input.
- **Required outcome:** Preserve input netlist; distinguish model .iv from circuit sweeps; test maintained workflow.
- **Implementation/owner:** scripts/ivcurve.py; https://github.com/JoeyDelp/JoSIM/issues/82.
- **Verification:** input-byte-preservation and model/circuit sweep assertions.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

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
  and pending native qualification. Nonzero precharge remains separate.
- **Required outcome:** Complete the remaining startup/precharge scope and
  final qualification. These bounded studies do not establish general
  scientific correctness.
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

- **Classification:** runtime/dependency concern.
- **Current reproduction/result:** Syntax-only audit evidence insufficient; runtime validation pending.
- **Required outcome:** Dependency manifest; modern NumPy/SciPy imports, representative functions and headless plotting.
- **Implementation/owner:** scripts/*.py.
- **Verification:** isolated dependency install/import/runtime tests.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## D06 — Runtime/memory and trace retention

- **Classification:** measured-performance concern.
- **Current reproduction/result:** Open; theoretical payload about 8*Nsteps*(Ntraces+1) bytes before overhead.
- **Required outcome:** Measure large/noisy circuits and retained-trace scaling; address demonstrated avoidable costs.
- **Implementation/owner:** Simulation::trans_sim; Results; src/Output.cpp.
- **Verification:** reproducible benchmark with wall time/RSS and trace/step counts.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

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

- **Classification:** existing correct behaviour plus expanded error-path concern.
- **Current reproduction/result:** Audit missing-output-directory probe passed; broader corpus pending.
- **Required outcome:** Preserve useful output errors, no unintended overwrite/crash/hang or silent partial success.
- **Implementation/owner:** src/Output.cpp; src/Input.cpp; src/CliOptions.cpp.
- **Verification:** bounded malformed-input/output-preservation regression corpus.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

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

- **Classification:** confirmed supported-output/source defects; repaired locally,
  native qualification pending.
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
  artifacts/audit-resolution/phase-source/measurements.json.
