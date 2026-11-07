# Gate-controlled Josephson junctions

Use `GC=node` to control a junction's critical current with the voltage between
that node and ground. The control node must already belong to the circuit.

```spice
B1 out 0 junction GC=gate
VG gate 0 DC 5m
.model junction jj(ic=100u, rn=16, r0=160, cap=0.07p, gwidth=10m)
```

The phenomenological gate model scales the Josephson current by

\[
g(V_g)=\max\left(0,1-\frac{|V_g|}{GWIDTH}\right).
\]

`GWIDTH` is a positive voltage; its default is 0.1 mV. At zero gate voltage the
Josephson current is unchanged. It reaches zero at `|Vg| >= GWIDTH`. The gate
does not alter the junction capacitance or resistive branches. This model is a
linear suppression approximation, not a microscopic semiconductor junction model.

Gate voltage is interpreted in volts in both analysis modes. In phase mode it
is recovered from the node phase using the second-order backward difference.
The solver evaluates gate control from the previous solved state, so fast gate
transitions require a timestep convergence check.

The earlier experimental ordering `B1 out 0 gate junction` is also accepted
when the model explicitly supplies `GWIDTH`. Otherwise the extra field retains
its legacy WRspice phase-node meaning and is ignored.
`GC=` is recommended because it makes the control explicit. Gate connections
are mapped through subcircuit ports and local node names.

For model IV curves, add `GTJJ=voltage`:

```spice
.iv junction 200u iv.csv GTJJ=5m STEP=1u
```

`STEP` sets a positive current increment; `ISTEP` is an alias. The default is
2.5 uA. Each sweep reaches its requested endpoint without overshooting, even
when the increment does not divide the maximum current.

`TSTEP`, `TSTOP` and `TSTART` control the model-IV transient and averaging window.
See the [sweep guide](sweeps.md) for choosing the timestep and settling interval.
