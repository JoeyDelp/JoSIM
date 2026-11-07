# Updating to 3.0

Download the package for your operating system from
[GitHub Releases](https://github.com/JoeyDelp/JoSIM/releases). Replace the old
executable with the one in `bin/`.

## Existing circuits

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

Rebuild applications using the headers and libraries from the same 3.0
installation. Several public types have changed size. JoSIM exposes C++ types,
so the compiler and C++ runtime must also be compatible.

The CMake target is `josim::josim`:

```cmake
find_package(josim 3.0 CONFIG REQUIRED)
target_link_libraries(my_simulator PRIVATE josim::josim)
```

Shared Unix libraries use ABI version 3. See [Embedding JoSIM](API.md) for
error handling, result storage and the restrictions on concurrent runs.
