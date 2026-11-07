# Updating to 3.1

Download the package for your operating system from
[GitHub Releases](https://github.com/JoeyDelp/JoSIM/releases). Replace the old
executable with the one in `bin/`.

## Changes from 3.0

JoSIM 3.1 adds [capacitor initial voltages](model_limits.md#capacitor-voltage),
[BETA_C for RTYPE=0 junctions](model_limits.md#beta-c) and
[quadratic nonlinear inductors](model_limits.md#nonlinear-inductance).
Capacitor `IC=` requires `.TRAN ... DST`; those runs do not support phase or
controlled sources. `ALPHA=` specifies differential inductance, so its cubic
self-flux term includes a factor of one third.

Voltage-mode mutual inductors now use the same preceding current samples for
all coupled branches. This correction can change results in existing coupled
circuits. Recheck timestep convergence when comparing them with 3.0.

## Changes from releases before 3.0

Several fixes change simulation results, particularly phase accumulation,
startup, subcircuit parameters, model IV curves and noise. Recheck timestep,
settling time and output sampling when comparing older results with 3.0.
A matching seed does not reproduce the same noise waveform across versions.
The [noise guide](noise_sources.md) explains the corrected bandwidth and
junction thermal-noise calculations.

Named subcircuit parameters and hierarchical pin outputs now work as described
in the [syntax guide](syntax.md#subcircuits). Invalid trace requests, malformed
waveforms and invalid model values produce errors. Output files cannot overwrite
the main netlist or one of its includes.

KLU is the supported solver. The old SuperLU and OpenMP options (`SLU=ON`,
`USING_OPENMP=ON`, `-x` and `-p`) have been removed because they did not provide
working alternative simulation modes. Run independent circuits in separate
processes when parallel execution is needed.

The old `ivcurve.py` and `noise_insert.py` scripts are retired. Use the
[circuit-sweep helper](sweeps.md) and native `.TEMP`/`.NEB` controls instead.

## C++ applications

Rebuild applications, including those built for 3.0, using the headers and
libraries from the same 3.1 installation. Capacitor initialization and nonlinear
inductors change public type layouts. JoSIM exposes C++ types,
so the compiler and C++ runtime must also be compatible.

The CMake target is `josim::josim`:

```cmake
find_package(josim 3.1 CONFIG REQUIRED)
target_link_libraries(my_simulator PRIVATE josim::josim)
```

Shared libraries use ABI version 4, independently of the release version.
Unix applications linked to ABI version 3 must be rebuilt. See [Embedding JoSIM](API.md) for
error handling, result storage and the restrictions on concurrent runs.
