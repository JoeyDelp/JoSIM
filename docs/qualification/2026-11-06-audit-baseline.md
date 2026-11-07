# Audit-resolution baseline — 6 November 2026

This is an initial measurement, not a completed qualification. It continues
[the consolidation record](2026-11-06-consolidation.md) from source
`53b6a426570eafb53d3673f79254157a4ae2c001`. The production C++ sources were
unchanged while development instrumentation was added. All required remaining
work is recorded in the [resolution ledger](../AUDIT_RESOLUTION.md).

## Builds and dynamic checks

| Configuration | Compiler | Result |
|---|---|---|
| Release, fresh dependency acquisition | GCC 16.2.1 | build and 27/27 CTests passed |
| Debug | GCC 16.2.1 | build and 27/27 passed |
| Release | Clang 23.1.4 | build and 27/27 passed |
| Debug | Clang 23.1.4 | build and 27/27 passed |
| Debug, ASan + UBSan, leak detection on | Clang 23.1.4 | 27/27 passed, 135.30 s |
| Debug, first-party coverage | Clang/LLVM 23.1.4 | 27/27 passed |

The fresh GCC build had no dependency source overrides and set CPM_SOURCE_CACHE
OFF in an empty build directory. Other builds reused the previously verified
immutable source cache. C11 and C++17 are explicit. Sanitizers instrument both
C dependencies and first-party code; installed consumers link the same runtimes.
An initial restricted-sandbox leak check failed because process inspection was
blocked; the complete suite then passed outside that sandbox with leak checks
unchanged. No host security setting was changed.

Exact commands and JUnit: `build/audit-resolution/baseline/*/run.json` and
`tests.xml`; sanitizer command/result: `build/audit-resolution/sanitizers/`
`test-native.log` and `tests-native.xml`. Public CI/release evidence will replace
these local-only pointers in the final qualification.

## Initial first-party coverage

| Metric | Covered / executable total | Percentage |
|---|---:|---:|
| Lines | 3448 / 5992 | 57.54% |
| Functions | 249 / 342 | 72.81% |
| Branch outcomes | 1602 / 3053 | 52.47% |

All compiled mappings under src/*.cpp and include/JoSIM/*.hpp are included.
Vendored solvers, include/spline.h, generated files and test implementation are
excluded. Disabled preprocessor paths, declarations and uninstantiated templates
have no counters; their files remain visible with null mappings in the report.
The missing LUSolve mapping is the inactive, still unresolved SuperLU branch.
The other missing header mappings contain declarations/data definitions rather
than emitted executable bodies. They are retained in the inventory.

LLVM23 warns about nine unused inline mappings with hash zero. For every warning
name, the live nonzero-hash profile exists and its exact execution count matches
the exported function count. Dense and sparse profile merges produce identical
aggregate totals; no functions or warning names were excluded. The full mapping
comparison is retained in mapping-diagnostics.json. An initial report-owner bug
combined warnings with JSON; output and diagnostics now use separate files.

Reports: artifacts/audit-resolution/coverage-baseline/{README.md,summary.json,
coverage.json,coverage.txt,html/index.html,mapping-diagnostics.json}. Raw counters
remain in build/audit-resolution/coverage-initial/profiles. Counts represent
exercised code, not proof of numerical correctness.

## Risk assessment and next verification

- Error owner: 10.58% lines / 35.11% branches. Add meaningful malformed-input,
  output-preservation and embedding-recovery cases, rather than formatter-only
  tests that mirror the error enumeration.
- Verbose output: 5.81% / 11.36%. Assert effective seed and expanded parameters.
- Netlist: 51.15% / 45.83%. Exercise named parameters, hierarchical pin aliases,
  nested instances and direct/indirect cycles.
- Simulation: 72.79% / 53.26%. Add analytic conservation/convergence/mode/startup
  tests and independent long-run/JJ-buffer studies before changing numerics.
- Devices and Rng need ground/orientation, mode, rewind/retry, spread, stochastic
  timing and transition decisions exercised with independent invariants.
- Output format branches and maintained utility runtime paths need direct
  content assertions. High-risk gaps remain open until those campaigns finish.

No acceptance thresholds have been set merely to accept this baseline. The
final CI policy must protect the added defect/numerical tests and justified
per-module/overall coverage requirements, with every remaining high-risk gap
explained. This report is not a final completion claim.

## Warnings and focused static analysis

GCC builds emit 47 dependency warning lines and no first-party warnings. Clang
builds emit 423 dependency warning lines and one first-party warning: the unused
SLU member. Dependency warnings are retained in baseline-warnings.json.

Focused clang-tidy analysis covered Input, Netlist, RelevantTrace, Output,
Simulation, Function, IV, Noise and Rng. There are 49 distinct first-party
locations: mostly optional-value accesses requiring invariant review, plus two
dead stores. These are triage candidates, not automatically confirmed defects.
Logs and the pending review inventory are under artifacts/audit-resolution/
static-analysis. In particular, CUS relative-path handling dereferences an
optional input parent path; reproduce it through stdin and bare-filename input.
