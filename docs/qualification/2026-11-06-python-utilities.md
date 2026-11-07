# Python utility qualification — 6 November 2026

This study resolves audit item D05. It covers the maintained plotting and
phase-specification utilities, plus explicit retirement of the obsolete noise
preprocessor. It does not change C++ simulation equations, the library ABI or
the native package format. [Usage and migration](../python_utilities.md) describe
the supported commands.

## Reproduced failures

Runtime tests used a fresh Python 3.12.12 environment with NumPy 2.4.1,
Pandas 3.0.0, Plotly 7.1.0, Kaleido 1.5.0 and Tabulate 0.10.0.
The initial audit's syntax parsing did not establish these behaviors.

| Owner | Observed failure | Resolution |
|---|---|---|
| All three plotters | DAT loading raises `TypeError` because `delim_whitespace` was removed | Use `sep=r"\s+"`; preserve numeric samples |
| Both single-file plotters | Image output without `-d` raises `AttributeError` on `None.split` | Numeric dimensions with a 700×500 default |
| `plot-compare.py` | Second trace gets the first file's time vector, even with a different number of samples | Use each file's own time column |
| `josim-plot2.py` | Grid/square/stacked label phase jumps but retain radian values | Convert phase values in every layout; retain SI voltage/current |
| Plotting CLI | `-x input.csv` exits successfully after replacing the input with HTML | Reject input/output identity, including filesystem aliases |
| `sp_generator.py` | No exact 20 ps row leads to zero-timestep division | Select the first sample at/after a configurable baseline |
| `sp_generator.py` | A `0 → 1 → 0 → −1` sequence omits the return to zero | Track all quantized vector changes; require elapsed quiet time |
| `noise_insert.py` | Blank lines raise `IndexError`; `10k` resistance raises `ValueError` | Retire the redundant parser in favor of native thermal controls |

Invalid plotting requests and unsupported image formats now fail with status 2.
Modern Kaleido does not export EPS; this formerly advertised format is explicitly
retired, with SVG/PDF alternatives. CSV/DAT and whitespace SP file formats remain.
SP stability is measured in seconds, matching the old calculation rather than
its misleading “picoseconds” help text. Settling-row times and reversal rows
change where the old sample-count algorithm was wrong. The legacy rounding
threshold remains strict (`fraction > threshold`).

The noise preprocessor predates native resistor/junction thermal sources. Its
flat text parsing cannot correctly evaluate netlist parameters or active
junction conductance. Its explicit `1/B` update interval also differs from the
qualified `1/(3B)` effective-bandwidth convention. Maintaining a second circuit
parser and thermal model would duplicate the simulator's owners. The retired
entry point explains `.TEMP`/`.NEB` and per-component overrides, returns 2 and
changes no files. Native global temperature includes **junctions as well as
resistors**; the migration guide calls out that distinction and duplicate sources.

## Regression assertions

`test/regression_python_utilities.py` passes **14/14 methods**. It asserts:

- Exact trace names and values in five layouts, including trace subsets.
- All four phase-unit choices in all five layouts; relative allowance `1e-14`
  covers elementary floating-point scaling only. Other trace units are unchanged.
- Independent comparison timelines `[0,1,2]` and `[10,20]`, with unequal lengths.
- CSV/DAT numeric round trips at `1e-14` relative allowance; unchanged input hashes.
- Three real HTML CLI exports and eight static exports: three PNGs plus
  JPEG, JPG, WebP, SVG and PDF. PNG headers assert actual 700×500 and 600×400
  dimensions; SVG parses as XML with the requested dimensions; other formats
  have their expected binary headers/trailers. No image-export mocks are used.
- Same-path/hardlink input protection, malformed dimensions/options, unsupported
  EPS and formats, non-finite/empty data and clean nonzero CLI errors.
- A nonuniform grid with no 20 ps row and expected count sequence `[0,1,0,-1]`
  at `[21,36,59,90]` ps. These values follow the synthetic plateaus and a 10 ps
  quiet interval, independently of implementation row counts.
- Two-junction joint stability, zero start/stability, threshold boundaries,
  invalid phase columns/times/options and source preservation.
- Both retired preprocessors return 2 and preserve existing input/output files.

The final local suite took about 14 seconds. The receipt records the actual
Python and dependency versions, exact script/test hashes, CLI commands and
outcomes, exported-file hashes and unchanged source identities.

## Real output and rendering environment

The current GCC Release CLI ran `test/ex_jtl_basic.cir` with only its unrelated
model-IV output directive removed. Source/effective-deck/binary hashes are
recorded. The resulting mixed voltage/current/phase CSV was exported as HTML,
PNG and PDF. A phase-only copy produced SP rows `[0,0]`, `[1,1]`, `[2,2]` at
20, 326.5 and 626.5 ps. The 1200×1000 PNG was visually inspected: separate
voltage/current panels, phase panels in `2π` units, readable axes and two pulses.
This is a postprocessing check; independent numerical model qualification is
reported separately.

Local static exports used `/opt/brave-bin/brave` selected explicitly through
`BROWSER_PATH`, with fresh temporary profiles and Chromium sandbox enabled.
The first isolated probe failed because its long task-owned temporary path
exceeded Chromium's Unix socket path limit. A shorter task path fixed startup;
no browser flags, user profile or host security settings were changed.
Interactive browser display was not exercised. HTML and headless export were.

## Reproduction and evidence

Follow the [utility guide](../python_utilities.md). CI's `Python utilities` job
creates a fresh environment, records pip's wheel metadata and installed versions,
then requires the same suite with the runner's Chrome. Native CTest remains
independent of these optional packages; the tagged-release job requires both
native qualification and utility qualification.

Local evidence:

- `build/audit-resolution/python-utilities/probes/`: original failures, script
  identities and synthetic inputs; `red.json` and `additional-red.json`.
- `build/audit-resolution/python-utilities/`: clean install report/log,
  installed versions, browser-startup diagnostics and suite log.
- `artifacts/audit-resolution/python-utilities/report.json`: final test receipt.
- The same artifact directory contains `jtl-source.json`, the effective deck,
  CSV, HTML/PNG/PDF/SP outputs and `jtl-exports.json`.

The manifest pins direct packages; transitive versions remain pip-resolved and
are recorded rather than claimed as a universal lockfile. Python 3.12.12 on
Linux is the measured local environment. The CI job supplies its own exact
environment receipt. No GUI, older Python version or other native OS utility
runtime is inferred from these checks. C++ five-platform qualification remains
a separate requirement for each integrated revision.
