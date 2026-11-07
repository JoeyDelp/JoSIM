# Embedding JoSIM

JoSIM provides a C++17 library. Install it as described in the
[build instructions](DEVELOPMENT.md#install-the-library), then link the CMake
target `josim::josim`:

```cmake
find_package(josim 3.0 CONFIG REQUIRED)
add_executable(my_simulator main.cpp)
target_link_libraries(my_simulator PRIVATE josim::josim)
```

Use headers and libraries from the same installation. Applications using an
older release must be rebuilt for 3.0.

## Running a circuit

The command-line entry point in
[src/josim.cpp](https://github.com/JoeyDelp/JoSIM/blob/master/src/josim.cpp)
shows the complete sequence: parse input, expand the main design, identify the
transient settings, create the matrix and run the simulation.

After `Input::parse_input`, call
`input.netlist.expand_maindesign(input.parameters)`. This binds parameters and
models for each subcircuit instance before expanding its children. Local
expressions may depend on instance overrides, so do not evaluate all local
definitions beforehand. To change parameters and expand the circuit again,
start with fresh `Input` and `Netlist` objects.

The no-argument expansion overloads remain available for callers using the
older type-scoped parameter handling. Use the parameter-aware overload for
named instance parameters. The [syntax guide](syntax.md#subcircuits) describes
defaults, overrides and model lookup.

Set finite, positive transient times before constructing a matrix. Directly
constructed models receive the same parameter checks as parsed models. If you
modify the public sparse arrays, keep their dimensions and indices consistent;
normal applications should use `Matrix::create_matrix`.

Capacitor `IC=` also applies to library simulations. Disable settling with
`input.transSim.startup(false)` before creating the matrix; parsed `.TRAN ... DST`
does this automatically. The [initial-state guide](model_limits.md#capacitor-voltage)
describes the supported circuits. Initial-state and nonlinear-inductor storage
change public component layouts; rebuild applications with the updated matching
headers and library. `Inductor::flux(current)` returns self flux, and
`differential_inductance(current)` returns its derivative; mutual linkage is
stored separately in the coupling entries.

## Handling errors

Catch `std::exception` around the complete operation. Parsing, model, solver
and output errors throw with a diagnostic in `what()`. After a failed run,
discard that run's objects and construct a fresh circuit before trying again.
The command-line parsing/help functions still exit the process and should not
be used as library entry points.

Output errors include failed writes and non-finite converted traces. A failed
write may leave a partial file. Applications redirecting `std::cout` must restore
the stream and clear its error state before reusing it.

`Input` records the main netlist and its includes to prevent output from
replacing them. If you fill `expNetlist` directly, also fill `inputFiles` for
any source files that need this protection.

## Results and timestep changes

`Simulation::results` contains the time axis and retained histories. Explicit
trace selection retains the values needed for those traces; default output
retains all matrix unknowns. `Results::sourceCurrents` stores requested source
samples as they were applied during simulation. `phaseBefore` stores the two
preceding phase values used when converting the first output samples.

A timestep restart reconstructs the circuit and clears the previous results.
For direct component users, `Inductor::update_timestep` changes its component
stamps only; it does not rebuild the global matrix or integration histories.
Use the normal reconstruction path to change a simulation's timestep.

Model-IV helpers generate a single-junction test circuit. Each point starts
with fresh device histories. See [IV curves](sweeps.md#circuit-iv-versus-model-iv)
for the timing controls and the difference from a continuous circuit sweep.

## Random numbers and concurrent runs

Call `Rng::start_run(seed)` before creating each run's matrix. `Rng::rewind()`
resets the noise and spread streams, including a cached Gaussian sample;
JoSIM uses it when restarting at a smaller timestep.

Random engines are shared within the process. Run JoSIM on one designated
thread and finish one simulation before starting another. Use separate processes
for parallel sweeps. Do not draw from, reseed or share the generator while a
simulation is running.

Reproducible runs require the same build, circuit, seed, timestep and sampling
settings. Results are not guaranteed to be bit-identical across compilers or
platforms.
