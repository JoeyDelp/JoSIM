# Runtime and result memory

JoSIM retains every internal sample of each requested primitive trace before
formatting or decimating output. A larger print interval reduces the output
file, but does not reduce those internal histories.

For effective timestep `h` and stop time `TSTOP`, the main double-array payload is

\[
N=\lfloor TSTOP/h\rfloor,\qquad M=8N(1+U+S)\ \text{bytes},
\]

where `U` is the number of retained matrix-unknown histories and `S` the number
of requested applied-current-source histories. The extra history is time.
These counts need not equal the number of displayed columns: a difference can
need two operands, while repeated requests can share a primitive. With no
explicit trace selection, the default retains all matrix unknowns.

Matrix/solver/device state, custom waveform data, allocator overhead and
formatted output add memory beyond this payload. Output conversion can also
need working arrays. Timestep retries halve `h`, increasing the eventual sample
count; planning from the requested step alone can underestimate memory.

For example, `N=5.1 million` with one retained unknown needs about 81.6 MB
(77.8 MiB) for time and that unknown alone. Choose duration and requested traces
deliberately. Reducing the timestep requires a numerical convergence decision;
output decimation cannot substitute for that check.

The simulator reserves the known internal sample count for engaged histories.
Unrequested histories remain absent. This avoids repeated growth/copying and
reduces measured peak memory, while retaining the existing full-history result
API. C++ allocation or vector-size failures produce a recoverable diagnostic
with the requested sample count. They cannot catch an operating-system process
kill under physical-memory exhaustion; no universal RAM capacity is promised.

The [runtime/storage study](qualification/2026-11-06-runtime-storage.md) records
the controlled workloads, source identities, wall time, peak resident memory,
output comparisons and practical limits. Results depend on circuit, retained
traces, toolchain and host; they are not a general throughput specification.
