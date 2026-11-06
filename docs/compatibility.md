# Compatibility and migration

The 3.0.0 source line consolidates the public branches and the original noise
research, repairs the audited numerical/parser/library defects, and qualifies
the maintained workflows. The historical `v2.7` tag remains unchanged. Check
[Releases](https://github.com/JoeyDelp/JoSIM/releases) for published binaries;
a branch artifact with `dev.<revision>` in its name is a development build.

## Existing circuits

The SPICE-style transient interface, voltage/phase modes, KLU solver and
CSV/DAT/RAW formats remain. Correctness fixes can change previously incorrect
results, particularly subcircuit binding, selected traces, startup/phase history,
model-IV preparation and noise normalization. Compare scientific results using
the same build, effective timestep, output sampling, model and seed. Repeat
timestep and settling checks when migrating a circuit; a matching seed does not
promise identical results across versions or standard libraries.

Invalid timing, recursive subcircuits, missing models/nodes, unsupported output
requests and failed writes now produce useful errors. A failed run is not a
usable result. Output cannot overwrite a loaded netlist. See the
[syntax](syntax.md), [noise](noise_sources.md), [sweep](sweeps.md) and
[model-boundary](model_limits.md) guides for accepted inputs and tested examples.

`SLU=ON`, manual `SLU` definitions, `USING_OPENMP=ON`, CLI `-x` and `-p` fail
explicitly. Earlier options did not provide a functioning alternate/parallel
simulation path. KLU is the supported backend. Run independent simulations in
separate processes. Capacitor `IC=`, a direct `BETA_C=` field and nonlinear
inductors are not implemented; the model-boundary guide gives practical alternatives.

## C++ applications

**Recompile embedding applications against matching headers and libraries.**
Public layouts changed in Input, Netlist/Subcircuit, Results/Simulation,
PhaseSource, RelevantTrace and ProgressBar. The package exposes C++ library
types and does not promise a stable ABI across compilers, standard libraries
or JoSIM versions. Installed static archives are not a portable C ABI.

The installed CMake target remains `josim::josim`. Version selection uses
`SameMajorVersion`: request `find_package(josim 3.0 CONFIG REQUIRED)` for this
line. Shared Unix libraries carry major ABI version 3. The major version change
makes the public layout and supported-behaviour changes explicit; it does not
imply a new solver architecture. Patch releases repair compatible behaviour;
new compatible interfaces use a minor version. A further incompatible public
interface change requires another major version. See [the API guide](API.md)
for recoverable exceptions, legacy helpers and process-wide RNG ownership.

## Builds and binary platforms

Source builds require C++17, C11 and CMake/CTest 3.21+. Qualification uses GCC
and Clang Debug/Release, MSVC on Windows, and native binary plus default/shared
library checks on all five targets. Dependencies have pinned archive hashes.
Default/static and shared JoSIM both use static KLU dependencies. The portable
release CLI uses the static configuration described in the release procedure.

| Archive target | Runtime requirement | Qualification limit |
|---|---|---|
| linux-x64-rhel6 | x86_64, glibc 2.12 | CentOS 6.10 userspace on a modern kernel; no original RHEL6 kernel claim |
| linux-arm64-rhel7 | AArch64, glibc 2.17 | Native ARM runner/container userspace |
| windows-x64 | Windows 10+, x64, static MSVC runtime | Native Windows CI and supplied Windows host; no separate MSVC runtime installation |
| macos-arm64 | macOS 11+, arm64 | Native arm64 CI; deployment metadata does not test every older OS |
| macos-x86_64 | macOS 10.15+, Intel | Native Intel CI; local Rosetta checks are identified separately |

[Developing JoSIM](DEVELOPMENT.md) gives build/test/coverage commands.
[Release procedures](RELEASE_POLICY.md) define package checksums, manifests,
installed consumers and public download verification. The [audit ledger](AUDIT_RESOLUTION.md) links scientific studies that distinguish
executed tests, independent references and remaining physical/numerical limits.
