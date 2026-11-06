# Embedding JoSIM

Link the installed CMake target `josim::josim`. Use a C++17 compiler and rebuild
your application against the headers and libraries from the same installation.
The interface exposes C++ standard-library types; a stable binary interface
across compiler, standard-library or JoSIM versions is not promised.

## Parsing and subcircuit binding

After `Input::parse_input`, call
`input.netlist.expand_maindesign(input.parameters)`. This resolves globals,
then binds parameters and local models per instance before expanding its
children. Do not pre-evaluate all local definitions: some need instance values.
Continue with transient identification, matrix creation and simulation as in
the CLI entry point. Overrides capture their caller-evaluated values at binding.
To change caller/global parameters and rebind a circuit, create fresh Input and
Netlist objects; reparsing an already expanded map does not reinstantiate it.

The no-argument `expand_subcircuits()`/`expand_maindesign()` entry points remain
for existing callers that pre-parse type-scoped parameters/models. They preserve
that legacy lookup and component-value overrides; use the parameter-aware
overload for named instance parameters. `expand_subcircuits()` now validates
structural dependencies while retaining templates until main-design expansion.
Callers inspecting partially expanded templates must adapt to this change.

Resolved instance contexts in expanded lines, `ParameterName` and `models_new`
begin with `|`, followed by the inside-to-outside instance path. Definition
contexts retain their type names. Local template expressions can remain
unevaluated; check `Parameter::get_value()` before accessing a numeric value.
Model lookup prefers the nearest instance before globals. These changes add
methods without adding class state; rebuild consumers with this release's
headers and libraries.

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

CSV/DAT/RAW and model-IV writers explicitly close their streams and check the
final state. Standard-output formatting flushes and checks `std::cout`.
A write/close failure throws with its destination; a partially written file
may remain and must not be treated as a successful result. `WRITE_FAILED` is
appended to `OutputErrors`, preserving its existing values and method signatures.
If an embedding application redirects `std::cout` to a failing stream, it owns
restoring that destination and clearing the stream's error state before reuse.

`Output::write_output` also checks every completed trace for finite values.
Finite solved unknowns can overflow in unit conversion, filtering or subtraction.
Such a result throws `NONFINITE_TRACE` with its trace label and output-sample
index before the normal Output constructor opens any destination. Existing files
remain intact. This enum is appended; existing values/layouts/signatures remain.
Discard the failed Output object. Applications that populate public `traces`
themselves remain responsible for the data passed directly to formatters.

Model-IV helpers operate on the generated single-junction testbench. Each point
rebuilds fresh device state from its bias declaration; timestep retries rebuild
from the same Input. Existing signatures/layouts remain. `do_simulate` honors
a positive `Input::transSim.prstart()` as its averaging start; the default zero
retains the legacy latter-half convention for direct callers. At least two
averaging samples are required. CLI `.IV` timing controls are described in the
[sweep guide](sweeps.md). These finite transients do not imply converged DC or
continuous state transfer between points.

Transient storage reserves the complete internal sample count only for engaged
histories. Allocation/size failures during reservation throw a recoverable
`STORAGE_ALLOCATION_FAILED` diagnostic; the enum is appended without changing
existing values or layouts. Output decimation does not shrink internal histories.
See [memory planning](performance.md) for payload scaling and OS memory limits.

`Model::cpr()` still returns an owned coefficient copy. `cpr_view()` adds a
read-only reference for callers that keep the Model alive; changes to the Model
can change its contents or invalidate element pointers/iterators. Do not mutate
the model during simulation. The engine borrows these coefficients and the
inductor's existing coupling vector without per-step allocations. Existing
copying getters and public layouts remain unchanged.

`ProgressBar` waits between display updates instead of spinning. Configure its
text and total before starting workers; call `update` while the worker runs,
then `complete` from the owning thread. Repeated completion is safe and
destruction joins every worker. Do not concurrently create/complete workers or
change their text. The mutex/condition-variable state changes this public type's
layout; recompile consumers. Existing stream parameters retain their previous
behavior: asynchronous rendering uses `std::cout`.

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

`Results::sourceCurrents` contains only requested applied current-source
histories, indexed by the matrix's `sourcegen` index. Each retained vector has
the same length as `timeAxis`. Unrequested histories remain absent. This
appended result state requires consumers of `Results`/`Simulation` to recompile.

`Results::phaseBefore` retains nodal phase at `-h` and `-2h` for output
conversion. Phase mode retains its solved phases; voltage mode integrates the
solved startup voltages with the same BDF2 relation used for subsequent output.
The phase origin is zero before the first simulated step, including negative
startup steps. Histories are zero when startup is disabled. Explicit selection
retains only nodes needing conversion (voltage in phase mode, phase in voltage
mode), including difference operands; direct JJ variables need no duplicate
nodal history. Default all-unknown results retain every nodal history for later
postprocessing. The map stores two doubles per retained node plus map overhead;
it does not grow with run length. This changes the size of `Results` and
requires recompilation.

`PhaseSource` retains two preceding applied phase samples. BDF2 voltage
conversion uses these actual samples instead of asking an advanced random
generator for past values. Warmup continues through t=0 without resetting
history; a cold phase step retains its existing two-sample voltage impulse.
Full timestep retries reconstruct the sources and clear all result history.
The appended source fields also require consumers of component/Matrix types
to recompile against matching headers and libraries.

The command-line helpers `CliOptions::parse`, `display_help`, `Errors::oor` and
`Errors::error_message` retain process-exit behaviour for the executable.
Embedding applications should construct `Input` directly and handle exceptions,
rather than calling these CLI entry points.

`Matrix` exposes its sparse arrays for existing C++ consumers. A simulation
rejects empty/inconsistent arrays, invalid indices and non-finite coefficients
before calling KLU. Changing public arrays is the caller's responsibility;
normal applications should use `Matrix::create_matrix`.

The legacy `Inductor::update_timestep(factor)` hook rescales the component's
self and mutual voltage-mode stamp coefficients for a finite positive factor.
Phase-mode coefficients do not depend on timestep. This edits component stamp
storage only; it does not rebuild global CSR arrays or integration histories.
Normal simulation retries reconstruct the complete Matrix from Input. Use that
path for simulation timestep changes; this hook does not provide variable-step
integration.

`Model::validate()` checks physical/numerical parameter domains. Parsing and
junction construction call it, including after area/temperature scaling, so
directly constructed models receive the same checks. This adds a nonvirtual
method without changing object layout. See the [model domains](model_limits.md#model-domains).
Updated matrices are checked before refactorization, and every solved unknown
must be finite. Invalid models or unrepresentable solutions throw before
output; discard the failed run as described above.

`Noise::determine_sampling_interval(bandwidth, simulation_step)` returns
`max(simulation_step, 1/(3*bandwidth))`, rejecting invalid or unrepresentable
intervals. It adds a static method without changing object layout. For legacy
direct callers, `Input::neB=0` retains the omitted-`TSTEP` manual-NOISE fallback
to the simulation step; `.NEB 0` in a netlist is rejected. Automatic thermal
sources always require a positive bandwidth.

White `Function` instances store unit-ASD interpolation samples and apply the
current `ampValues()[0]` on evaluation. Changing that amplitude rescales the
current realization immediately, without drawing another sample. Internal
`miscValues` contents are implementation state. Existing signatures/layouts
and explicit-interval grid variance remain; the corrected NEB interval changes
seeded waveforms. See the [noise conventions](noise_sources.md).

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

PNOISE/FNOISE/ICNOISE share one OU-bank initializer. Corrected power-law weights,
endpoint limits and sampled-pole gain change their seeded waveforms. This adds
a private method without changing class/struct layouts or existing signatures.
`PNoiseState::scale` is now 1 because the variances already include amplitude;
`S1Hz` remains a continuous-OU diagnostic before sampling/interpolation. It is
not the actual interpolated PSD or a normalization guarantee at an out-of-band
frequency. See the [colored-noise conventions](noise_sources.md#power-law-noise).
