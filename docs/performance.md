# Runtime and memory

JoSIM stores every internal sample needed by the requested traces. Increasing
the print interval makes the output file smaller, but does not reduce those
internal histories.

For an internal step `h` and stop time `TSTOP`, the double arrays need roughly

\[
N=\lfloor TSTOP/h\rfloor,\qquad M=8N(1+U+S)\ \text{bytes},
\]

where `U` is the number of retained matrix-unknown histories and `S` the number
of requested applied-current-source histories. The extra history is time.
A difference trace may require two histories; repeated requests may share one.
With no explicit trace selection, JoSIM retains all matrix unknowns.

For example, 5.1 million samples with one retained unknown need about 81.6 MB
for that unknown and time. Solver state, waveform data and output conversion
need additional memory.

Choose the run duration and requested traces with this storage in mind. A
timestep restart halves the step and increases the sample count. Use `-V 1`
to see the final internal step and number of samples. A smaller timestep should
be chosen to resolve the circuit dynamics, not compensated for by output
sampling alone.

JoSIM reserves storage for the known sample count and avoids storing
unrequested histories. Allocation failures produce an error with the requested
size. An operating-system out-of-memory termination cannot be caught by JoSIM.

Nonlinear inductors require additional sparse solves and numerical
factorizations during Newton correction. The cost depends on current excursion,
timestep and matrix sparsity. Source samples and full waveform histories are
not duplicated for each iteration. Constant-L circuits keep the single-solve
path, including when `ALPHA=0` is written explicitly.
