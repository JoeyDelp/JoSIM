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
- **Current reproduction/result:** Existing fresh-consumer test passes in all four baseline builds; relocation/modes pending.
- **Required outcome:** Verify dependency discovery, relocation and every supported configuration.
- **Implementation/owner:** 30c8fbb; cmake/josim-config.cmake.in; test/consumer.
- **Verification:** ctest -R regression::installed_package.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## A05 — Silently ineffective SLU=ON

- **Classification:** confirmed supported-behaviour defect.
- **Current reproduction/result:** Open; option is still silently accepted.
- **Required outcome:** Repair and qualify, or retire with explicit configuration rejection.
- **Implementation/owner:** CMakeLists.txt; include/JoSIM/Simulation.hpp; src/LUSolve.cpp.
- **Verification:** configure SLU=ON; backend/library regressions.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## A06 — Moving dependency URL and stale linkage

- **Classification:** confirmed maintenance/portability defect; partial prior repair.
- **Current reproduction/result:** Fresh GCC Release acquisition/build passed. Filesystem linkage review pending.
- **Required outcome:** Verify immutable acquisition and remove stale links only with compiler/runtime evidence.
- **Implementation/owner:** da8998d; cceac6c; cmake/dependency_sources.cmake.
- **Verification:** clean-cache configure/build; installed consumers; five-platform ABI checks.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## A07 — Invalid transient and recursive subcircuits

- **Classification:** confirmed defects; prior repairs.
- **Current reproduction/result:** Existing bounded regressions pass in four builds; non-finite/error-path expansion pending.
- **Required outcome:** Clean finite/bounds errors, direct/indirect cycle rejection; no hangs/crashes.
- **Implementation/owner:** c36ea8a; 3a11500; src/Transient.cpp; src/Netlist.cpp.
- **Verification:** ctest -R regression::cli; sanitizers; malformed-input corpus.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## A08a — Named subcircuit parameter overrides (#81)

- **Classification:** confirmed compatibility defect.
- **Current reproduction/result:** Open; audit reproducer remains unsupported.
- **Required outcome:** Define scoped defaults/overrides, nested instances and test exact supplied circuits.
- **Implementation/owner:** src/Netlist.cpp; src/Parameters.cpp.
- **Verification:** named-parameter parser/numerical regressions.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## A08b — Hierarchical pin-alias output (#81)

- **Classification:** confirmed compatibility defect.
- **Current reproduction/result:** Open; audit reproducer silently drops the trace.
- **Required outcome:** Resolve pin aliases and reject invalid requested traces explicitly.
- **Implementation/owner:** src/Netlist.cpp; src/RelevantTrace.cpp.
- **Verification:** hierarchical output and malformed trace regressions.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## A09 — Effective seed absent from -V 1

- **Classification:** confirmed observability defect; merged candidate.
- **Current reproduction/result:** Implementation present in merged Input owner; dedicated assertions pending.
- **Required outcome:** Verify explicit/auto seeds are exposed and usable for replay.
- **Implementation/owner:** src/Input.cpp: seed reporting; src/Verbose.cpp.
- **Verification:** CLI verbose/seed replay regression.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

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

- **Classification:** unresolved circuit/model report.
- **Current reproduction/result:** Open; no expectation inferred from a single JoSIM run.
- **Required outcome:** Use supplied circuit, independent dynamic model, convergence and conservation to establish cause.
- **Implementation/owner:** https://github.com/JoeyDelp/JoSIM/issues/101.
- **Verification:** independent ODE reference plus phase/voltage/step comparisons.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## I093 — Stewart–McCumber parameter #93

- **Classification:** optional model convenience.
- **Current reproduction/result:** Disposition pending.
- **Required outcome:** State resistance convention and test a conversion example or implement supported semantics.
- **Implementation/owner:** https://github.com/JoeyDelp/JoSIM/issues/93.
- **Verification:** beta_c conversion/example numerical test.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

## I091 — Long-run JJ artifacts #91

- **Classification:** unresolved numerical investigation.
- **Current reproduction/result:** Open; report contains no reproducer, requiring a structured study.
- **Required outcome:** Multiple quantified long-phase probes with analytic/convergence/mode evidence; repair demonstrated cause.
- **Implementation/owner:** https://github.com/JoeyDelp/JoSIM/issues/91; src/Simulation.cpp.
- **Verification:** long-run RSJ/phase-driven JJ regression studies.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

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

- **Classification:** architecture/testing concern.
- **Current reproduction/result:** Open beyond existing gate/noise invariants.
- **Required outcome:** Analytic RLC, KCL/KVL, startup, convergence, modes, switching and pulse counts.
- **Implementation/owner:** src/Simulation.cpp; component stamps; test/.
- **Verification:** new independent numerical regression suite.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

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
- **Current reproduction/result:** Open: Errors::error_message still exits; RNG engines are process-global.
- **Required outcome:** Recoverable supported library errors; explicit reentrancy/concurrency boundaries with API/ABI review.
- **Implementation/owner:** src/Errors.cpp; src/josim.cpp; include/JoSIM/Rng.hpp.
- **Verification:** same-process error/recovery and independent-run tests.
- **Measurements/tolerances/evidence:** Initial 27/27 tests pass under GCC16.2.1/Clang23.1.4 Debug and Release. Item-specific acceptance and durable evidence remain pending unless stated above.

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
