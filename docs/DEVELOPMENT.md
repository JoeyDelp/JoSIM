# Developing JoSIM

JoSIM uses C++17, CMake and CTest. The default sparse solver is KLU. The obsolete
SuperLU selection is retired: `-DSLU=ON` and manually defining the `SLU` macro
produce explicit errors. Use KLU; the earlier accepted option did not select
a functioning alternative. JoSIM no longer downloads, links or installs the
unused SuperLU/CBLAS packages. Applications using those packages directly must
provide their own dependencies. The default KLU public API is unchanged.

## Build and test

Use a C++17 compiler, CMake 3.21 or newer, and Python 3 for these commands and
the regression suite. The source still declares CMake 3.14 as its minimum; that
older build-tool version has not been qualified by the current release process.

From the repository root:

```bash
cmake -S . -B build/dev -DCMAKE_BUILD_TYPE=Release
cmake --build build/dev --config Release --parallel 4
ctest --test-dir build/dev -C Release --output-on-failure --timeout 90
```

Configuration downloads checksum-verified solver archives. To prepare sources
before an offline build:

```bash
cmake -DJOSIM_DEPENDENCY_DIR=build/dependencies -P cmake/prepare_dependencies.cmake
cmake -S . -B build/dev -C build/dependencies/sources.cmake
```

The generated `sources.cmake` contains absolute paths. Regenerate it when moving
the dependency directory to another machine or mount point.

## Regression coverage

CTest runs the existing circuit examples and focused regression groups:

- `regression::gate`: gate units, both analysis modes, subcircuits and IV sweeps.
- `regression::noise`: seeded replay, white-noise variance, colored spectra,
  flux units, fractional critical-current noise and phase-step reduction.
- `regression::cli`: invalid transient settings and recursive subcircuits.
- `regression::functions`: waveform alignment, interpolation and expressions.
- `regression::build_options`: retired solver configuration/macro rejection.
- `regression::library`: repeated recoverable errors followed by valid runs.
- `regression::installed_package`: installs the library, then builds and runs a
  fresh external C++ consumer using `find_package(josim 2.7 CONFIG REQUIRED)`.

The tests establish these contracts. They do not establish general SPICE
compatibility or settle the open reports about long-run junction artifacts.
Preserve failing numerical evidence before changing device equations.

## Coverage and sanitizers

Use matching major versions of Clang, `llvm-cov` and `llvm-profdata`. The
coverage owner builds an instrumented Debug application/library, runs CTest,
and produces annotated HTML, raw LLVM JSON, per-module summaries and exact
commands. Use a dedicated build directory for each campaign; existing execution
profiles are preserved rather than silently combined with a later run.

```bash
python3 -B tools/coverage.py \
  --build-dir build/coverage --output artifacts/coverage
```

Add `--initial-cache build/dependencies/sources.cmake` to reuse prepared solver
sources. Versioned tools can be selected with `--cc`, `--cxx`, `--llvm-cov` and
`--llvm-profdata`. Coverage includes all compiled first-party `src/*.cpp` and
`include/JoSIM/*.hpp` mappings. Solvers, the vendored spline header, generated
code and test implementation are excluded. Files without mappings remain in the
inventory; declarations, disabled preprocessor paths and uninstantiated templates
do not acquire executable counters. Review LLVM diagnostics before accepting
measurements. Coverage percentages do not establish numerical correctness.

For memory and undefined-behaviour checks with GCC or Clang:

```bash
cmake -S . -B build/sanitizers -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DJOSIM_SANITIZERS=ON
cmake --build build/sanitizers --parallel 4
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build/sanitizers --output-on-failure --timeout 120
```

This instruments the C solver dependencies as well as first-party code. Runtime
link requirements propagate to installed development consumers. Instrumentation
is disabled by default and is not used for release packages. LeakSanitizer needs
process inspection that some sandboxes prohibit; run in a suitable environment
instead of disabling leak checks or changing host security settings.

The [audit-resolution ledger](AUDIT_RESOLUTION.md) records open verification work
and the initial coverage gaps. CI thresholds will follow the assessed risk and
required regression protection; they are not chosen merely to match a baseline.

## Installed C++ library

```bash
cmake --install build/dev --config Release --prefix build/install
```

Consumers set `CMAKE_PREFIX_PATH` to that prefix, call
`find_package(josim 2.7 CONFIG REQUIRED)`, and link `josim::josim`. The package
loads its solver dependencies and propagates C++17 and public headers.
See [embedding guidance](API.md) for error recovery and random-state ownership.

Keep fixes focused and add regression checks for non-trivial defects. Record
user-visible behavior in [CHANGELOG.md](https://github.com/JoeyDelp/JoSIM/blob/master/CHANGELOG.md).
See [the release procedure](RELEASE_POLICY.md) for portable binary builds.
