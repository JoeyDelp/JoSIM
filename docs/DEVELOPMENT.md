# Building JoSIM

JoSIM requires a C++17 compiler, a C11 compiler, CMake 3.21 or newer and Python 3.
KLU is the sparse solver used by both the executable and the C++ library.

## Build and test

From the repository root:

```bash
cmake -S . -B build/dev -DCMAKE_BUILD_TYPE=Release
cmake --build build/dev --config Release --parallel 4
ctest --test-dir build/dev -C Release --output-on-failure --timeout 90
```

The executable is `build/dev/josim-cli`. With a multi-configuration generator on
Windows, use `build/dev/Release/josim-cli.exe`.

CMake downloads the solver sources and checks their SHA-256 hashes. For an
offline build, download them beforehand:

```bash
cmake -DJOSIM_DEPENDENCY_DIR=build/dependencies -P cmake/prepare_dependencies.cmake
cmake -S . -B build/dev -C build/dependencies/sources.cmake
```

`sources.cmake` contains absolute paths. Regenerate it if the dependency
directory is moved to another machine or location.

The old SuperLU and OpenMP options are no longer supported. `SLU=ON` and
`USING_OPENMP=ON` produce configuration errors. Independent simulations can
be run in separate processes.

## Install the library

```bash
cmake --install build/dev --config Release --prefix build/install
```

Set `CMAKE_PREFIX_PATH` to the installation directory in your application, then
use `find_package(josim 3.1 CONFIG REQUIRED)` and link `josim::josim`. See
[Embedding JoSIM](API.md) for the calling sequence and error handling.

| Option | Library and runtime |
|---|---|
| Default | Static JoSIM and KLU; default compiler runtime |
| `BUILD_SHARED_LIBS=ON` | Shared JoSIM with static KLU |
| `MAKING_STATIC_BUILD=ON` | Static JoSIM and KLU; static MSVC runtime on Windows, static C++ runtime with GNU/Linux |

The last two options cannot be combined. macOS uses its system runtime. A static
C++ runtime on Linux does not make glibc static. Older GNU compilers may need
`stdc++fs`; CMake checks this when configuring.

The installed executable finds shared JoSIM relative to its installation.
Windows applications using the shared library must make the installation's
`bin` directory available to the DLL loader.

## Tests and coverage

CTest runs the circuit examples, numerical comparisons, input/output tests and
an application built against the installed library. Use `ctest --test-dir build/dev -N` to list the tests.

For AddressSanitizer, UndefinedBehaviorSanitizer and leak detection:

```bash
cmake -S . -B build/sanitizers -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DJOSIM_SANITIZERS=ON
cmake --build build/sanitizers --parallel 4
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build/sanitizers --output-on-failure --timeout 120
```

These settings also instrument the solver dependencies. Leak detection needs
an environment that permits process inspection.

For coverage, use matching versions of Clang, `llvm-cov` and `llvm-profdata`:

```bash
python3 -B tools/coverage.py --build-dir build/coverage --output artifacts/coverage
python3 -B tools/check_coverage.py --summary artifacts/coverage/summary.json \
  --output artifacts/coverage/policy.json
```

Use a new build/output directory for each run. The report covers JoSIM source
and headers; solver dependencies, the spline library, generated code and tests
are excluded. The second command checks the result against the project's
coverage requirements.

[Reference circuit data](https://github.com/JoeyDelp/JoSIM/tree/master/test/reference)
and the scripts used to regenerate it are described in the test directory.
The [Python utilities](python_utilities.md) have a separate set of dependencies.

To regenerate numerical reference data or run the longer noise comparisons,
install the optional scientific dependencies:

```bash
python3 -m venv build/science
build/science/bin/python -m pip install -r tools/requirements-qualification.txt
```

Use Python 3.11 or newer. The buffer and SQUID examples describe the comparison
commands; these dependencies are not needed to build or run JoSIM.

## Build the manual

Use Python 3.11 or newer:

```bash
python3 -m venv build/documentation-env
build/documentation-env/bin/python -m pip install -r tools/requirements-docs.txt
build/documentation-env/bin/mkdocs build --strict --site-dir build/documentation
build/documentation-env/bin/python -B tools/check_docs.py \
  --site-dir build/documentation --output artifacts/documentation/links.json
```

On Windows, the environment's executables are under `Scripts/` instead of `bin/`.
