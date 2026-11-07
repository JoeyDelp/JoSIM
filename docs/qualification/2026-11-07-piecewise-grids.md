# Piecewise waveform grids — 7 November 2026

Coverage-driven source tests exposed F27: a negative first PWL/PWS time was
accepted with a warning, then time zero was prepended ahead of it. The stored
grid was therefore unordered. Both public sources share `Function::parse_pwl`.

The bounded CLI witness uses points `(-1 ps, 1 V)` and `(2 ps, 2 V)`, 30 ordinary samples and a 1 kΩ load. Both old commands return success. PWS returns
1 V at time zero, then 1.296632 V at 0.1 ps. An ordered half-cosine segment
would give `1 + (1-cos(pi/3))/2 = 1.25 V` at zero. The prepend/clamp combination
introduces the discontinuity; this is independent of circuit integration.

Negative first times are now rejected. The maintained syntax uses finite,
nonnegative, strictly increasing times and finite signed amplitudes. A positive
first time still prepends zero with the same amplitude and a warning. This
preserves valid delayed-start behavior without introducing pre-zero waveform
control. `INVALID_PWL_PARAMETERS` is appended to the existing error enum;
existing values, signatures and class layouts remain unchanged.

Direct source tests cover both aliases, incomplete pairs, duplicate/decreasing
times, negative first times and unresolved/non-finite time/value data. Valid
PWS endpoints/nonuniform segments and EXP rise/fall use independent closed forms.
CLI regressions require useful failure status/diagnostics and no output file.

All four local GCC/Clang Debug/Release configurations pass 35/35 CTest groups.
Full Clang Debug ASan/UBSan with leak detection passes **35/35 in 212.87 s**.
The focused source/parser/numerical/library/tool groups also pass 5/5. Final
integrated/native qualification remains a separate requirement.

`artifacts/audit-resolution/piecewise-negative/` retains the actual old decks,
CSV/log witnesses, commands, source hashes and compiler/sanitizer receipts.
`test/function_regression.cpp` and `test/regression_cli.py` protect the repair.
