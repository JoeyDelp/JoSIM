# Developing JoSIM

JoSIM uses C++17, CMake and CTest. The default sparse solver is KLU. The legacy
`SLU` option does not currently select a working alternative backend.

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

CTest runs the existing circuit examples and four additional groups:

- `regression::gate`: gate units, both analysis modes, subcircuits and IV sweeps.
- `regression::noise`: seeded replay, white-noise variance, colored spectra,
  flux units, fractional critical-current noise and phase-step reduction.
- `regression::cli`: invalid transient settings and recursive subcircuits.
- `regression::installed_package`: installs the library, then builds and runs a
  fresh external C++ consumer using `find_package(josim 2.7 CONFIG REQUIRED)`.

The tests establish these contracts. They do not establish general SPICE
compatibility or settle the open reports about long-run junction artifacts.
Preserve failing numerical evidence before changing device equations.

## Installed C++ library

```bash
cmake --install build/dev --config Release --prefix build/install
```

Consumers set `CMAKE_PREFIX_PATH` to that prefix, call
`find_package(josim 2.7 CONFIG REQUIRED)`, and link `josim::josim`. The package
loads its solver dependencies and propagates C++17 and public headers.

Keep fixes focused and add regression checks for non-trivial defects. Record
user-visible behavior in [CHANGELOG.md](https://github.com/JoeyDelp/JoSIM/blob/master/CHANGELOG.md).
See [the release procedure](RELEASE_POLICY.md) for portable binary builds.
