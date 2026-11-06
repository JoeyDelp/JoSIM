# Python plotting and phase specifications

The maintained postprocessing tools use Python 3.11 or newer. Keep the `scripts`
directory together: the three plotters and the specification generator share
input/output checks in `plot_common.py`. None of these tools needs SciPy.
The optional scientific reference tools have their own dependency manifest in
the [development guide](DEVELOPMENT.md).

## Install

From the source checkout, create an isolated environment:

```bash
python3 -m venv build/python-utilities
build/python-utilities/bin/python -m pip install -r tools/requirements-utilities.txt
```

The manifest pins the direct dependencies: NumPy 2.4.1, Pandas 3.0.0,
Plotly 7.1.0, Kaleido 1.5.0 and Tabulate 0.10.0. Transitive dependencies are
resolved by pip; qualification records their installed versions and wheel hashes.
On Windows use `build\python-utilities\Scripts\python.exe` in place of
`build/python-utilities/bin/python` below.

Interactive plots open in a browser. HTML export embeds Plotly and needs no
image renderer. Static image export additionally requires a compatible installed
Chrome/Chromium browser, as described by
[Plotly](https://plotly.com/python/static-image-export/).
Kaleido 1 does not bundle Chrome. `BROWSER_PATH` can select an installed browser;
the tools retain its security sandbox. Browser startup errors fail the export.

## Plot CSV or DAT output

```bash
build/python-utilities/bin/python scripts/josim-plot.py output.csv -x output.html
build/python-utilities/bin/python scripts/josim-plot2.py output.dat \
  -t stacked -j 2pi -c light -o output.png -d 1200x800
build/python-utilities/bin/python scripts/plot-compare.py first.csv second.dat \
  -s 'P(B1)' -x comparison.html
```

- `.csv` uses commas; `.dat` uses whitespace. The first column is time in
  seconds; the remaining columns are traces. Samples must be numeric and finite.
- The single-file plotters offer `grid`, `stacked`, `combined`, `square` and
  `sep_comb` layouts. `-s` selects trace names exactly as written in the header.
- `josim-plot2.py -j` scales phase traces by `rad`, `0.5pi`, `pi` or `2pi` in
  every layout. Voltage and current traces retain volts and amperes.
- The comparison plotter uses each file's own time samples. It overlays matching
  columns from the second file without resampling; first-file traces missing
  from the second file remain visible on their own.
- `-o` supports PNG, JPEG/JPG, WebP, SVG and PDF, including uppercase extensions.
  EPS export is retired because the supported Kaleido renderer has no EPS
  output. Use SVG or PDF for vector output.
- `-d WIDTHxHEIGHT` requires positive integers. Defaults are 700×500 for the
  single-file plotters and the existing 3508×2480 for comparisons.
- Use either `-x` for HTML or `-o` for an image. With neither option, the plot
  opens interactively. Invalid requests and export failures return status 2.
  Output cannot overwrite either input, including a filesystem alias.

## Generate a phase-count specification

Provide a CSV containing `time` followed only by `P(junction)` phase columns,
in radians. Time must be finite and strictly increasing. For example:

```bash
build/python-utilities/bin/python scripts/sp_generator.py phases.csv \
  -o phases.sp --start 20e-12 --stability 20e-12 --threshold 0.8
```

The first sample at or after `--start` defines each junction's zero-count phase
offset. An exact 20 ps sample is unnecessary. For phase displacement
\(u=(\phi-\phi_0)/(2\pi)\), the count starts at truncation toward zero;
its magnitude increases by one when the remaining fractional magnitude is
**strictly greater** than `--threshold`. The threshold must lie in [0,1].

A row is written when the complete quantized count vector has remained unchanged
for `--stability` seconds and differs from the last stored vector. The initial
zero vector is always written. Reversals and returns toward zero are recorded.
Stability uses elapsed time, so nonuniform output grids are supported. Both time
options use **seconds**; zero stability and a start of zero are valid.

This is a sampled phase-count summary. It does not infer events between samples
or prove that a physical waveform has settled within a count bin. A final state
without enough observed quiet time is omitted. The whitespace-separated SP
format retains time and junction-name columns; output cannot overwrite input.

## Retired preprocessors

`noise_insert.py` predates native thermal-noise handling. It crashed on blank
lines and engineering suffixes, omitted junction-state conductance and used a
noise update interval inconsistent with its bandwidth argument. Its entry point
now returns status 2, explains the replacement and changes no files.

Use native `.TEMP` (kelvin) and `.NEB` (hertz) controls, or component `TEMP=` and
`NEB=` overrides. Global temperature enables thermal noise in **both resistors
and junctions**; per-component controls permit narrower selection. Remove
sources created by the old script before enabling native thermal noise to avoid
counting the same resistor noise twice. See [noise sources](noise_sources.md)
for normalization, timestep limits and the classical physical assumptions.

The old `ivcurve.py` prototype is also retired. Use the tested
[model-IV and circuit-sweep workflows](sweeps.md).

## Verification

```bash
build/python-utilities/bin/python -B test/regression_python_utilities.py \
  --report artifacts/python-utilities/report.json
```

The suite requires a working image renderer. It checks all layouts, phase units,
independent comparison timestamps, file preservation, nonuniform phase counts,
invalid inputs, three HTML exports and actual exports in every advertised image
format. The CI utility job installs a fresh environment and preserves the
dependency, browser and test receipts. Release publication requires this job
alongside native binary and library qualification. See the
[qualification study](qualification/2026-11-06-python-utilities.md) for the
measured scope.
