# Focused static-analysis review — 7 November 2026

The initial audit ran Clang-Tidy on Input, Netlist, RelevantTrace, Output,
Simulation, IV, Function, Noise and Rng. The rerun adds Inductor after F25.
Checks cover Clang's analyzers, integer division, suspicious semicolons,
use-after-move and unchecked optional access. Exact commands, tool identity,
source hashes, raw diagnostics and individual dispositions are retained under
`artifacts/audit-resolution/static-analysis/`.

## Results

The initial source, `53b6a426570eafb53d3673f79254157a4ae2c001`, produced
49 distinct locations. Every location has a disposition:

| Initial group | Count | Resolution |
|---|---:|---|
| Constructor/topology or immediate-guard invariants | 44 | Reviewed at callers and constructors |
| Generated IV testbench precondition | 1 | Reviewed; helper boundary documented |
| CUS parent path and requested-current trace | 2 | Repaired under F01/F04/F05 |
| Unused output/source-history initializers | 2 | Removed during F06/F07 repairs |

The current ten-owner run completes without analysis-command errors and reports
45 distinct optional-access locations. All are reviewed invariants or the IV
helper precondition; the two old defects and two dead stores are absent. No
`NOLINT` or broad suppression was added to make these results disappear.

## Invariants and their limits

- Every concrete component assigns `currentIndex_ = bi++` before node/stamp
  setup. `BasicComponent::set_node_indices` fills exactly the optional node
  indices selected by `NodeConfig`; the corresponding simulation branches use
  those same topologies. F25's component tests exercise all four configurations;
  numerical/noise/source tests exercise both modes and orientations.
- Retained-result accesses in Simulation are immediately guarded by the same
  vector optional. No intervening mutation occurs between the guard and access.
- Model-IV generation builds a single-junction testbench without explicit trace
  selection. Simulation engages every result unknown, including its first node.
  `IV::do_simulate` is scoped to this generated testbench, as the API guide states.
  It is not a general arbitrary-matrix measurement API.

The public component/index containers remain mutable for compatibility. Direct
callers must preserve their constructor invariants; arbitrary corruption of
component fields is outside the supported embedding contract. Sparse-array
dimensions/indices and solved finite values receive separate runtime checks.

This is a focused review of high-risk owners, not exhaustive static proof.
Compiler warnings and executed sanitizer/numerical tests provide separate
evidence. Passing an optional-access review does not establish physical accuracy.

`initial-dispositions.json` maps all 49 original locations;
`current/dispositions.json` maps all 45 current locations. `review-summary.json`
records identities, counts and limits. The current raw reports retain warnings
so future reviewers can inspect the evidence directly.
