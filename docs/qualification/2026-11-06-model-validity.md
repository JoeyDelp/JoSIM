# Model domains and finite CPR limits — 6 November 2026

Bounded malformed-model checks exposed undefined behaviour and successful runs
containing non-finite state. A valid fully transparent junction also produced
NaN at finite temperature. These defects are separate from the optional model
conveniences in [the boundary study](2026-11-06-model-boundaries.md).

## Confirmed failures

| Input | Previous result | Required result |
|---|---|---|
| RTYPE=1e30 | UBSan reports an out-of-range floating-to-integer cast | Explicit RTYPE diagnostic before casting |
| RTYPE=0.5 or 2 | Silently selects unintended behaviour | Only 0 or 1 accepted |
| D=2 | Successful run containing NaN current | Model-domain error |
| D=1, T=4.2 K, phase pi | Successful run containing NaN current | Finite positive-temperature current limit |
| D=1, T=0, phase pi +/-1e-8 | Subtractive cancellation loses the finite one-sided limit | Approximately +/-2*Ic |
| V=1e308 V across 1e-300 ohm | Successful CSV containing infinite current | Failed simulation before output |

The last input deliberately exceeds representable current. It is an error-path
test, not a physically useful circuit. The initial four regression methods
failed in 39 subcases. Four additional near-cusp subcases exposed cancellation
while developing the zero-temperature check. Original decks, output, executable
hashes and sanitizer diagnostics are under
`build/audit-resolution/model-validation/` (`red.json`, `sanitizer-red.json`,
`cli-red.log`, `zero-near-red.log` and `overflow/red.json`).

## Finite-temperature limit

The existing manual's temperature-dependent CPR can be written

\[
I=A\sin\phi\,\frac{\tanh(\alpha q)}{q},\qquad
A=\frac{\pi\Delta}{2eR_N},\quad
\alpha=\frac{\Delta}{2k_BT},\quad
q=\sqrt{1-D\sin^2(\phi/2)}.
\]

The manual attributes this model to
[Haberkorn, Knauer and Richter (1978)](https://onlinelibrary.wiley.com/doi/10.1002/pssa.2210470266).
The repair follows directly from its mathematical limit:

\[
\lim_{q\to0}\frac{\tanh(\alpha q)}q=\alpha,\qquad T>0.
\]

Previously the implementation multiplied `1/q` by `tanh(alpha*q)`, giving
infinity times zero. It now evaluates the finite limit. This preserves D=1;
rejecting that supported transparency would not repair the defect.

For a single harmonic with coefficient a1, evaluate the equivalent radicand
as `(1-beta)+beta*cos(phi/2)^2`, where beta=D*a1^2. This avoids loss of
`1-sin(phi/2)^2` near the zero-temperature cusp. Multiple harmonics retain
their existing generalized expression. A negative radicand outside eight
double-epsilon rounding units produces a useful CPR-domain error; only that
rounding allowance is clamped.

At T=0 away from q=0, the limit is tanh(alpha*q)=1. The RN expression uses
its corresponding explicit zero-temperature limit. For D=1, a1=1,

\[
I=2I_c\sin(\phi/2)\,\operatorname{sgn}(\cos(\phi/2)).
\]

The one-sided currents at pi are opposite. A point numerically at that
zero-temperature cusp (`q<=8*epsilon`) has no unique continuous current limit
and fails explicitly. Finite positive temperature regularizes the expression.
Near-cusp tests at pi +/-1e-8 resolve both sides and produce +/-200 uA for
Ic=100 uA in both modes. This numerical neighbourhood is not silently widened
to the much larger sqrt(epsilon) region affected by subtraction.

## Domain and solver-state validation

`Model::validate()` provides the same checks for parsed models and direct C++
model objects. Junction construction validates before and after effective
area/temperature scaling. Constraints include finite fields, RTYPE in {0,1},
positive RN/R0/DELV/ICFCT/GWIDTH/TC, nonnegative C and T, and D in [0,1]. The
temperature-dependent expression requires T<TC and positive Ic. Zero C and
signed/zero Ic remain valid for the ordinary sinusoidal model. An IC override
needs a nonzero base Ic and a finite positive effective area.

The temperature model is not extrapolated periodically above Tc through the
cosine in its gap approximation. The [support guide](../model_limits.md#model-domains)
states the normal-state alternative. Arbitrary harmonic/transparency combinations
must retain a real radicand along the trajectory; the implementation reports
the offending junction if that condition fails.

Updated sparse matrices are validated before refactorization. Every solved
unknown must be finite during negative startup and the reported transient,
including unrequested state. A KLU success flag alone does not establish this.
Unrepresentable solutions now throw through the existing recoverable error path
before CSV formatting. This finite-state check does not establish an error
bound or detect every loss of numerical precision.

No physical constants, public class layouts or output formats change. The new
nonvirtual validation method adds an API entry point. Existing valid model
parameters retain their meaning; previously ignored/invalid definitions can
now fail earlier with a model diagnostic. The CPR equation is preserved and
evaluated stably, rather than replaced with another device model.

## Quantitative checks and scope

Independent steady phase-bias checks use the manual's dimensionless normalized
equation, eliminating RN through its stated temperature relation. They cover
D=0/0.5/0.99/1, T=0.1/4.2/8 K and five phases including pi and nearby points,
in both modes: 120 cases. Maximum current error is 4.793e-12 A. The acceptance
bound is 3 ppm of expected current plus 1 pA for CSV rounding and the zero
crossing; the largest error/budget ratio is 0.0534. These checks establish
evaluation of the existing constitutive equation, not an independent dynamic
trajectory or experimental validity of its gap approximation.

Additional checks cover zero-temperature one-sided limits, explicit singular
points, zero capacitance, signed/zero sinusoidal Ic, malformed parameter domains,
invalid harmonic radicands, numerical overflow, direct-library invalid models,
area overflow and repeated failure followed by a valid simulation. GCC16 and
Clang23 Debug/Release pass all 33 CTest groups. The final full ASan/UBSan suite
with leak detection passes 33/33 in 180.87 s. An earlier candidate passed
215.90 s before the added near-cusp cases exposed cancellation; that result
does not substitute for the final run. Native qualification is pending publication.

```bash
ctest --test-dir build/dev -R 'regression::(cli|library)' \
  --output-on-failure --no-tests=error
```

Compact measurements and source/test/executable hashes:
`artifacts/audit-resolution/model-validation/measurements.json`.
Full local logs and candidate patch are in `build/audit-resolution/model-validation/`.
The intermediate executable includes a tracked patch over its configure-time
Git version; clean exact-source builds remain required for final publication.
