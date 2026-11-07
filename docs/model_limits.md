# Initial states and model parameters

## Junction model domains {#model-domains}

Model values must be finite. RTYPE is exactly 0 or 1; RN, R0, DELV, ICFCT,
GWIDTH and TC are positive. C can be zero. D is in [0,1]. The ordinary sinusoidal
model permits signed or zero Ic. The temperature-dependent CPR additionally
requires positive Ic and `0<=T<TC`. Effective junction area must be positive;
an IC override requires a nonzero model Ic. C++ model objects receive the same
validation as parsed netlists, including after area/temperature scaling.

At finite positive temperature, D=1 is supported, including the regularized
current limit at phase pi. At zero temperature, the fully transparent CPR has
a discontinuous cusp. A point numerically at that cusp is rejected; finite
positive temperature or a resolved point on either side has a defined value.
Arbitrary harmonic/transparency combinations must keep their square-root
argument nonnegative throughout the run.

The temperature-dependent superconducting expression does not model operation
at or above Tc. For a prescribed normal-state RC junction, use `RTYPE=0 IC=0`
with the chosen RN and C, without enabling that temperature-dependent CPR.
Instance `TEMP=`/global `.temp` controls thermal noise separately; it does not
create a temperature trajectory or automatically switch superconductivity at Tc.

An invalid model or non-finite solved state stops the simulation with an error.

## Capacitor voltage (#109) {#capacitor-voltage}

Set `IC=` on a capacitor to specify its voltage at physical time zero, in volts.
The sign is `V(positive terminal)-V(negative terminal)`. Use `DST` on `.TRAN`
to disable automatic settling, which would otherwise change the requested state:

```text
R1 N 0 10
C1 N 0 1p IC=1m
.tran 0.1p 100p DST
.print V(C1) I(C1) I(R1)
.end
```

The first sample has V(C1)=1 mV and I(C1)=-100 uA. With tau=RC=10 ps,
the ideal discharge is `V(t)=1m*exp(-t/10p)`. The
[complete example](https://github.com/JoeyDelp/JoSIM/blob/master/test/comp/capacitor_initial_voltage.cir)
is included in the source tree.

`IC=` accepts finite signed values and scoped parameter expressions, including
subcircuit instance parameters. Capacitance must be finite and positive. The
initializer solves the coupled node voltages, branch currents and voltage
derivatives, then constructs consistent integration histories in both analysis
modes. A timestep restart reapplies the same initial conditions. A later print
start omits samples from the output, without moving the physical initial time.

Unspecified independent capacitor voltages start at zero. Constraints already
imposed by other capacitors or ideal voltage sources take precedence, so parallel
capacitors share their voltage and derivative. Conflicting capacitor loops or
source voltages are errors. Inductor currents and junction phases start at zero;
transmission lines start without incoming waves. The circuit still needs enough
connections to determine its node voltages; IC does not supply a ground reference.

This initial-voltage path currently rejects circuits containing phase sources
or controlled sources. It does not provide a DC operating-point solve or general
SPICE `.IC`/`UIC` semantics. `UIC` retains its existing behavior; `DST` is required.
The request is tracked in [#109](https://github.com/JoeyDelp/JoSIM/issues/109).

Without explicit capacitor IC, startup keeps its existing zero-history behavior.
Default startup advances that state for a finite negative-time settling interval;
`DST` disables the interval.

If other circuit states need physical preparation, include the charging circuit
and a long enough interval before the event of interest. For example:

```text
V1 IN 0 PWL(0 1m 200p 1m 202p 0 300p 0)
R1 IN OUT 10
C1 OUT 0 1p
.tran 0.1p 300p DST
.print V(IN) V(C1) I(R1) I(C1) I(V1)
.end
```

Here tau=RC=10 ps. The capacitor charges for 20 tau, then discharges through
the resistor after the source falls. The source's finite fall time is part of
the physical preparation. This does not instantly assign voltage at t=0 or
remove the charging circuit. Use the actual preparation network when it affects
other circuit states. A later print-start value can omit preparation from the
output while retaining it in the simulation.

A [complete charging example](https://github.com/JoeyDelp/JoSIM/blob/master/test/comp/capacitor_precharge.cir)
is included in the source tree.

## Stewart–McCumber parameter (#93) {#beta-c}

For a sinusoidal RCSJ junction with constant shunt resistance R,

\[
\beta_c=\frac{I_c C R^2}{\sigma},\qquad
R=\sqrt{\frac{\beta_c\sigma}{I_c C}},\qquad
\sigma=\frac{h}{4\pi e}.
\]

R is in ohms, Ic in amperes and C in farads. For `RTYPE=0`, set `BETA_C`
directly; JoSIM calculates RN once after parsing the model:

```text
.param BETA=1 ICVAL=1u CVAL=0.1p
.model JM JJ(RTYPE=0 IC=ICVAL CAP=CVAL BETA_C=BETA)
```

This gives R approximately 57.36776 ohms for beta_c=1, using JoSIM's
Josephson constant `SIGMA=3.291059757e-16 V*s`. `BETA_C` accepts parameter
expressions in the model's scope. Model defaults for Ic and C apply when omitted;
all three values must be finite and positive, and the calculated RN must be
finite and nonzero. For zero or negative Ic, or C=0, specify RN instead.

Specify `RTYPE=0` explicitly: the default is 1. An explicit RN alongside
`BETA_C`, duplicate `BETA_C` assignments, and temperature-dependent model
parameters `T`, `TC` or `D` are errors. The temperature-dependent model derives
its own RN, so it cannot also use this conversion. Instance `TEMP=` and global
`.TEMP` control thermal noise separately and are compatible with `BETA_C`.

Normal AREA and instance IC scaling still apply: Ic and C scale with area,
while RN scales inversely. Their product `Ic*C*RN^2`, and hence nominal beta_c,
is preserved. `RTYPE=0` uses RN throughout, including in place of R0.

The [complete example](https://github.com/JoeyDelp/JoSIM/blob/master/test/comp/jj_beta_c.cir)
is checked against explicit RN and the physical current relation
`I=Ic*sin(phi)+V/R+C*dV/dt` for beta_c=0.2, 1 and 5 in both modes, including
AREA and instance IC overrides. To calculate RN in a parameter expression,
use `SQRT(BETA*3.291059757e-16/(ICVAL*CVAL))`.

`RTYPE=1` has different subgap, transition and normal-state conductance.
It has no single global damping
resistance: a beta_c quoted with R0 describes a different operating assumption
from one quoted with RN. Gate control, critical-current noise, temperature and
a non-sinusoidal CPR can also change the effective damping/stiffness. `BETA_C`
therefore describes nominal damping based on the model Ic scale. Direct input
for `RTYPE=1` remains unsupported; choose its resistances explicitly. The request
is tracked in [#93](https://github.com/JoeyDelp/JoSIM/issues/93).

## Current-dependent inductance (#88) {#nonlinear-inductance}

Add `ALPHA=` to an L component for a quadratic kinetic-inductance approximation:

```text
.param KINETIC=1e8
L1 IN OUT 100p ALPHA=KINETIC
```

VALUE is the zero-current inductance L0 in H. ALPHA is a in A^-2. JoSIM uses
**differential inductance**, with the self-flux law

\[
L_d(I)=\frac{d\Phi_s}{dI}=L_0(1+aI^2),\qquad
\Phi_s(I)=L_0\left(I+\frac{aI^3}{3}\right),\qquad
V=\frac{d\Phi_s}{dt}.
\]

For the example, Ld is 200 pH at 100 uA, while Phi_s/I is about 133.3 pH.
The distinction matters: interpreting `L0*(1+a*I^2)` as Phi/I would give a
different flux law and three times the nonlinear contribution to Ld.

L0 must be finite and positive; a must be finite and non-negative. Both accept
scoped parameter expressions, resolved once before the run. `SPREAD=` scales
L0 through the existing inductor spread rules. The polynomial is evaluated
numerically during solving; `ALPHA=0` or omission retains constant L. This is
a prescribed constitutive law, not a general runtime expression or microscopic
material model. Flux, differential inductance and matrix coefficients must stay
within the numerical range.

For coupled inductors, the total linkage is

\[
\Phi_i=\Phi_{s,i}(I_i)+\sum_{j\ne i}M_{ij}I_j,\qquad
M_{ij}=M_{ji}=k_{ij}\sqrt{L_{0i}L_{0j}}.
\]

M is constant and uses the zero-current inductances. A connected coupling group
containing a nonlinear inductor must have a positive-definite base inductance
matrix at numerical precision. Pairwise `|k|<1` alone does not establish this
for three or more coils. Perfect coupling, nearly singular groups and
self-coupling are rejected. With a>=0, the additional diagonal differential
inductance preserves positivity as current changes.

The stored energy of one uncoupled element is

\[
W(I)=L_0\left(\frac{I^2}{2}+\frac{aI^4}{4}\right).
\]

For a coupled group, sum those energies and add `Mij*Ii*Ij` once per pair.
Initial currents follow the existing startup or capacitor-initial-voltage
procedure. No inductor `IC=` option is provided. A
[nonlinear LC example](https://github.com/JoeyDelp/JoSIM/blob/master/test/comp/nonlinear_inductor.cir)
starts with capacitor energy and zero inductor current.

Voltage mode applies BDF2 to total flux linkage. Phase mode solves the
corresponding flux/phase constraint. Both use bounded Newton correction for
the nonlinear inductor equations; see [numerical methods](numerics.md#nonlinear-inductors).
The request is tracked in [#88](https://github.com/JoeyDelp/JoSIM/issues/88).
