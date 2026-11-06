# Embedding JoSIM

Link the installed CMake target `josim::josim`. Use a C++17 compiler and rebuild
your application against the headers and libraries from the same installation.
The interface exposes C++ standard-library types; a stable binary interface
across compiler, standard-library or JoSIM versions is not promised.

## Errors and recovery

Set finite, positive `tstep` and `tstop` before creating a matrix. `simsize()`
checks the point count before integer conversion. `validate_output()` checks
print settings against the actual grid; output also rejects empty result data.

Input, model, matrix, simulation and output failures throw C++ exceptions. Catch
`std::exception` around the complete operation; `std::runtime_error::what()`
contains JoSIM's diagnostic. A failed simulation is not a result. Discard the
failed run's Input, Matrix and Simulation objects, then create a fresh run.
Solver allocations are released when construction fails or a timestep retry
rebuilds the matrix.

`Input` records the main netlist and included files. Normal output and `.iv`
generation reject destinations that refer to those inputs, including existing
hard-link aliases. Keep this provenance when using the normal input parser.
Applications that construct `expNetlist` directly should fill `inputFiles` if
they also need protection for source files they loaded themselves. This adds
state to `Input`; recompile existing embedding applications for this release.
Hierarchical pin lookup adds alias state to `Netlist`/`Subcircuit`; device
differences add a second primitive to `RelevantTrace`. Recompile consumers of
these public C++ types as well. Existing trace-handler function signatures are
retained.

The command-line helpers `CliOptions::parse`, `display_help`, `Errors::oor` and
`Errors::error_message` retain process-exit behaviour for the executable.
Embedding applications should construct `Input` directly and handle exceptions,
rather than calling these CLI entry points.

`Matrix` exposes its sparse arrays for existing C++ consumers. A simulation
rejects empty/inconsistent arrays, invalid indices and non-finite coefficients
before calling KLU. Changing public arrays is the caller's responsibility;
normal applications should use `Matrix::create_matrix`.

## Random state and concurrent runs

JoSIM currently has process-global random engines and a per-thread Gaussian
cache. It is not reentrant or safe for concurrent simulations in one process.
Use one designated thread for JoSIM operations and complete each run before
starting another. Use separate processes for parallel circuit sweeps.

Call `Rng::start_run(seed)` before constructing each run's matrix to select and
replay its noise and spread streams. `Rng::rewind()` resets both streams and the
calling thread's cached Gaussian sample; internal timestep retries use it.
Drawing from these streams elsewhere changes subsequent samples. Do not
interleave runs, reseed another run, or share references returned by `noise()`
or `spread()` with another thread.

Repeatability means replay with the same build, netlist, seed, timestep and
sampling settings. Changing the timestep can change stochastic sample timing.
Bit-identical floating-point output across platforms or toolchains is not a
supported guarantee.
