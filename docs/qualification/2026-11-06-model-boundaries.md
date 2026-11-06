# Capacitor preparation and beta_c — 6 November 2026

The optional-feature dispositions in [the model guide](../model_limits.md)
define the support boundary for #109, #93 and #88. This study verifies the
supported capacitor-preparation and beta_c conversion examples. It does not
implement arbitrary initial states or current-dependent inductors.

## Capacitor preparation reference

The example has R=10 ohm, C=1 pF, tau=RC=10 ps, and Vb=1 mV. Its source
holds Vb until t1=200 ps, then falls linearly to zero at t2=202 ps. Let
a=Vb/(t2-t1), and define g(s)=0 for s<=0 and

\[
g(s)=s-\tau+\tau e^{-s/\tau},\qquad s>0.
\]

For zero initial voltage, the independent continuous solution is

\[
V_C(t)=V_b(1-e^{-t/\tau})-a g(t-t_1)+a g(t-t_2),\qquad I_C=C\dot V_C.
\]

The test starts from zero with DST and evaluates 190–300 ps, after the initial
charging transient has decayed below 6 pV. This avoids treating the discrete
first sample of a source step as an independently assigned capacitor voltage.
Both modes use 0.1/0.05/0.025 ps steps. Maximum voltage errors are
114.502/29.7314/7.59995 nV; current errors are 11.4502/2.97314/0.759550 nA.
Errors decrease about fourfold per halving. CSV KCL residual is zero.

Bounds are `2*Vb*(h/tau)^2 + 1 nV` for voltage, that base divided by R plus
0.1 nA for current, and 1e-15 A for KCL. Each halving must reduce errors below
0.4 times the previous value plus the corresponding rounding floor. Ohm/KVL
residuals allow 2 nV for independently rounded CSV columns.

Separately, `C1 N 0 1p IC=1m` and `IC = 1m` previously succeeded with zero
voltage, just like the same circuit without IC. The owning constructor now
rejects that unsupported request with a useful diagnostic. Six CLI cases
(three assignment forms, both modes) failed the rejection assertion before the
repair; a direct-library test also requires a recoverable exception. A node
named IC remains valid. No initial-condition solver semantics are added.

## beta_c conversion reference

Using exact SI sigma=h/(4*pi*e), Ic=1 uA and C=0.1 pF, independently calculate
R=sqrt(beta_c*sigma/(Ic*C)). For beta_c=0.2/1/5, R is
25.6556418152/57.3677591052/128.278209076 ohm. The netlist derives the same
quantity through ordinary parameter expressions and assigns RN in RTYPE=0.

Apply V(t)=1e7*t V, so dV/dt=1e7 V/s. After 1 ps, compare the junction's
total current with the independently specified constitutive relation

\[
I=I_c\sin\phi+V/R+C\,10^7.
\]

The measured unwrapped phase is an input to this constitutive check; it is not
an independent phase-trajectory solution. At h=0.01 ps, both modes pass all
three beta values. The largest current error is 9.40e-13 A, against a 1e-10 A
bound (0.01% of Ic) for prediction/discretization and CSV rounding. KCL residual is zero.
This validates the explicit conversion workflow, rather than a new model field
or a damping convention for RTYPE=1.

## Reproduction and evidence

```bash
ctest --test-dir build/dev -R 'regression::(cli|numerics|library)' \
  --output-on-failure --no-tests=error
python3 -B test/regression_numerics.py build/dev/josim-cli \
  --report artifacts/numerics.json
```

The numerical owner now records 56 cases: the prior 36 linear and eight static
JJ/L cases, plus six precharge and six beta_c cases. GCC16/Clang23 Debug and
Release pass all 33 CTest groups. ASan/UBSan with leak detection passes the
affected CLI/numerics/library groups in 5.19 s; the preceding production
revision passed the complete 33-group sanitizer suite in 212.16 s.
Exact decks and executable/test hashes are in
`artifacts/audit-resolution/model-boundaries.json`; red diagnostics and build
receipts are in `build/audit-resolution/optional-features/`.
Native qualification is pending for the combined source and required existing
numerical/CLI/library groups.
