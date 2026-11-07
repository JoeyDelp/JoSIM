# Manual, package version and migration — 7 November 2026

The source version is **3.0.0**. The public C++ layouts changed in Input,
Netlist/Subcircuit, Results/Simulation, PhaseSource, RelevantTrace and ProgressBar.
The existing CMake package policy is `SameMajorVersion`; a major version change
makes that boundary explicit. Consumers request major 3 and recompile against
matching headers/libraries. Unix shared libraries carry `SOVERSION 3`; the local
shared object reports `SONAME libjosim.so.3`. The historical `v2.7` tag is retained.
This document is a preparation record, not a claim that release/Pages publication
or the final integrated-source gates have completed.

## Manual and examples

The README, English and Japanese manuals now describe the actual build commands,
KLU-only support, retired parallel/solver switches, source/model validation,
noise bandwidth, fixed-step/full-restart mechanism and public API migration.
The Japanese pages link current English model/scientific guides; they do not
claim to translate every new study. Obsolete Azure/testing-branch badges,
versioned setup instructions, unqualified 10 ms timing and a broken duplicate
plotting implementation are removed. Exponential signs/default tables and
unreferenced footnotes/links are corrected. Existing scientific references and
historical release entries remain.

The English/Japanese JTL netlists are identical and run in both analysis modes:

- 4,000 finite samples at 0.25 ps over the declared 1 ns interval.
- Each triangular input integrates to 0.9999957293 Phi0, within the predeclared
  10 ppm allowance for its rounded 827.13 µV amplitude.
- Each of the two junctions advances by one 2π phase increment per input pulse.
  Maximum deviation is about 1.24e-5 rad, below 1e-4 rad including finite settling
  and six-digit CSV rounding. Both modes agree on those printed increments.

These invariants check the example's intended pulse transport; copied output is
not used as an independent physical reference. The current example CSV excerpt
is regenerated. The README/index quickstart uses the existing sink example,
which has no incidental model-IV output. Its 4,000 output samples are finite.
The maintained plotting command creates an actual self-contained HTML export.
Earlier terminal/plot images remain labelled illustrations where applicable.

## Site checks and local context

An isolated Python 3.12 environment installs pinned MkDocs 1.6.1 and Material
9.8.0. The strict build and the local HTML file/fragment checker pass. The initial
52-page build exposed ten broken links/anchors; none is suppressed. The checker
also rejects missing assets/fragments and a generated `context/` directory in
controlled fixtures. The final inventory and exact counts are in links.json.

`mkdocs.yml` explicitly excludes ignored local `docs/context/` notes. Package
installation now includes the public manual and also excludes that directory;
installed-consumer checks require linked migration/model/sweep/performance guides
and reject installed local context. Before the repair, all four guides were
absent from the installed payload (F30).

The old polyfill.io script is removed and MathJax is pinned to 3.2.2, following
[its modern-browser guidance](https://docs.mathjax.org/en/v3.2/web/start.html#browser-compatibility).
The generated syntax page renders 382 MathJax containers with no math-error
elements or browser console errors/warnings. The Japanese home page visibly
shows 3.0.0, C++17/C11, CMake 3.21 and the supported/retired choices. These are
local browser checks; the deployed site still requires separate verification.

## Build and API checks

Local package/version qualification uses exact CMake/CTest 3.21.0 and fresh
installed consumers requesting `josim 3.0`. Default and shared library-mode
qualification includes every declared CTest group and installation relocation.
Current receipts record the source revision plus working-tree identity; final
clean-source/native checks remain separate.

The help menu also advertised `--sanitycheck` and `--Verbose=1`, although both
failed with unknown-switch status 255 (F29). It now prints accepted lowercase
`--sanity` and `--verbose`. A process regression extracts all eight advertised
long options and executes every one successfully. It does not broaden parsing
or revive the retired switches. CLI and fresh-consumer checks pass 2/2.

Evidence is retained in artifacts/audit-resolution/documentation/ (dependency
report, strict build, checker fixtures, browser observations, actual example
commands/data and old help/package failures), cmake-min-3-final/ (minimum tools)
and library-3-final/ (default/shared modes). Those machine-readable receipts
provide exact source/tool/configuration identities and executed test results.
