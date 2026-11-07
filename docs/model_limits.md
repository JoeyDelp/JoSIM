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

JoSIM does not implement capacitor `IC=` or general SPICE initial-condition
semantics. Capacitor `IC=` is rejected explicitly; earlier builds silently
ignored it. `DST` disables negative-time startup and begins with zero stored
histories. Default startup advances the circuit from that zero state for a
finite settling interval; it does not calculate a DC operating point or set
an arbitrary capacitor voltage.

To prepare a nonzero capacitor voltage, include the charging circuit and a
long enough charging interval before the event of interest. For example:

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
is included in the source tree. Direct initial-voltage support is requested in
[#109](https://github.com/JoeyDelp/JoSIM/issues/109).

## Stewart–McCumber parameter (#93) {#beta-c}

For a sinusoidal RCSJ junction with constant shunt resistance R,

\[
\beta_c=\frac{I_c C R^2}{\sigma},\qquad
R=\sqrt{\frac{\beta_c\sigma}{I_c C}},\qquad
\sigma=\frac{h}{4\pi e}.
\]

R is in ohms, Ic in amperes and C in farads. Use ordinary `.param` expressions
to calculate R:

```text
.param BETA=1 ICVAL=1u CVAL=0.1p
.param SIGMA_SI={6.62607015e-34/(4*PI*1.602176634e-19)}
.param RSHUNT={SQRT(BETA*SIGMA_SI/(ICVAL*CVAL))}
.model JM JJ(RTYPE=0 IC=ICVAL CAP=CVAL RN=RSHUNT)
```

This gives R=57.3677591 ohm for beta_c=1. The conversion requires positive,
finite beta_c, Ic and C. For C=0, beta_c=0 does not determine R; choose R
separately. The [complete example](https://github.com/JoeyDelp/JoSIM/blob/master/test/comp/jj_beta_c.cir)
is checked through the physical current relation
`I=Ic*sin(phi)+V/R+C*dV/dt` for beta_c=0.2, 1 and 5 in both modes.

`RTYPE=0` uses RN as its constant resistance. `RTYPE=1` has different subgap,
transition and normal-state conductance. It has no single global damping
resistance: a beta_c quoted with R0 describes a different operating assumption
from one quoted with RN. Gate control, critical-current noise, temperature and
a non-sinusoidal CPR can also change the effective damping/stiffness. JoSIM
does not currently accept `BETA_C=` as a model parameter; use the conversion above.
Direct input is requested in [#93](https://github.com/JoeyDelp/JoSIM/issues/93).

## Current-dependent inductance (#88) {#nonlinear-inductance}

JoSIM's L component is a constant inductance with flux linkage Phi=L*I.
Its value expression is evaluated before simulation, not at each timestep.
There is no supported L(I) constitutive model or branch-current expression in
that value. A static parameter expression does not create a nonlinear element.

For the requested `L(I)=L0*(1+a*I^2)`, the intended meaning must first be chosen:

| Meaning of L(I) | Flux linkage Phi(I) | Differential inductance dPhi/dI |
|---|---|---|
| Differential inductance in V=L(I)*dI/dt | L0*(I+a*I^3/3), plus initial flux | L0*(1+a*I^2) |
| Flux/current quotient Phi(I)/I | L0*(I+a*I^3), plus initial flux | L0*(1+3*a*I^2) |

The factor of three matters. Substituting a time-varying number into the
constant-L stamp does not define a consistent nonlinear flux law.

For a bounded excursion where `abs(a*I^2)` is small, a constant-L approximation
may be sufficient. In the first interpretation, choosing L0 changes the
differential inductance by at most 1% when `abs(a*I^2)<=0.01` over the entire
excursion. This is an element-level bound, not a bound on accumulated circuit
phase or amplifier gain. A local small-signal approximation around nonzero bias
also needs consistent bias flux. Large-signal nonlinear behaviour requires a
model that JoSIM does not currently provide. Nonlinear inductors are requested
in [#88](https://github.com/JoeyDelp/JoSIM/issues/88).
