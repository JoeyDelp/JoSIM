# Runtime and storage qualification — 6 November 2026

This study addresses D06 through measured changes. The first stage reserves
retained histories; separately profiled per-step vector copies and progress
reporting remain distinct follow-up work. No simulation equation or operation
order changes in the reservation stage.

## Storage change

`Simulation::setup` knows the complete positive-time sample count. It now
reserves that count for time and each engaged matrix/source-current history.
It does not instantiate unrequested histories. Output decimation, trace labels,
history contents, retry reconstruction and public class layouts remain.

The [memory guide](../performance.md) derives the raw payload
`8*N*(1+U+S)`. Vector growth can add transient copies and allocator overhead;
unused capacity is not necessarily resident memory. The results below measure
actual peak RSS rather than treating a capacity ratio as a RAM reduction.

`std::bad_alloc` and `std::length_error` from reservation become a recoverable
`STORAGE_ALLOCATION_FAILED` diagnostic with the requested sample count. The enum
value is appended, retaining existing values. A count of `8e18` fits `int64`
but exceeds any supported `vector<double>::max_size()`; CLI and repeated C++
recovery tests reject it without attempting an enormous allocation or inducing
physical-memory pressure. This check does not promise recovery from an OS OOM kill.

## Method

The clean source baseline is `8e9515ec5c1cac7e5c2d7e3cfbd81f8dfe67e503`.
The local Linux run uses GCC 16.2.1, C++17, Release `-O3 -DNDEBUG`, default
static JoSIM/KLU libraries and the host's dynamic C++ runtime. Cache, compiler,
kernel, available CPU set, binary hash and candidate diff identity are recorded.

Eight fixtures exercise:

- 64 independent DC nodes with 1/8/32 retained voltages, 600,000 internal samples.
- The same source network with eight requested applied-current histories.
- The qualified zero-loop-inductance noisy SQUID: 5.1 million samples at 1 ps,
  fixed seed 1729, 200 MHz bandwidth and 20 ps output sampling.
- 128 mutually coupled RL pairs, 128 RLC cells and 128 parallel JJs, each with
  100,000 internal samples. Dyadic synthetic timesteps make grid counts exact.

Each binary gets a warmup, followed by **seven paired repetitions per fixture**,
with baseline/candidate order reversed on alternate pairs. All use minimal CLI
mode and the same selected CPU. No builds or profiles run concurrently with
the final timed campaign. Monotonic time surrounds the child command; captured
pipe EOF wakes the supervisor promptly. GNU time independently records wall,
user/system CPU and peak RSS. The archive retains medians, quartiles and ranges.

A provisional wrapper used timeout polling with file-directed stdout, which
quantized its supervisor wall time by up to 50 ms. Those wall results are
excluded. The final campaign was repeated with the corrected method; its
owner hashes distinguish it from the preserved provisional evidence.

The owner validates finite, strictly increasing samples, exact grids/seeds,
analytic DC voltages/source currents, RL/RLC equilibria and symmetric-JJ KCL.
The noisy mean has a 3 nV allowance around the independently derived weak-noise
RSJ mean; the longer scientific study establishes its model/statistical scope.
Every paired CSV hash must match exactly. All **112 timed runs** pass these
unchanged assertions and all paired outputs are byte-identical.

## Reservation results

Median wall time and peak resident memory:

| Workload | Baseline seconds | Reserved seconds | Baseline MiB | Reserved MiB |
|---|---:|---:|---:|---:|
| 128 JJs | 0.3098 | 0.3110 | 10.13 | 10.05 |
| 128 coupled RL pairs | 0.6166 | 0.6166 | 11.66 | 10.71 |
| 128 RLC cells | 0.3868 | 0.3866 | 9.94 | 9.70 |
| Eight applied-source histories | 0.4318 | 0.4141 | 59.63 | 47.66 |
| Long noisy SQUID | 0.5219 | 0.5115 | 102.77 | 89.45 |
| One voltage history | 0.4055 | 0.4002 | 18.97 | 15.15 |
| 32 voltage histories | 0.4780 | 0.4453 | 199.77 | 159.49 |
| Eight voltage histories | 0.4220 | 0.4132 | 56.80 | 47.68 |

Peak memory falls about **13%** for the noisy run and **20%** for 32 voltages
or eight applied-current histories. The widest case's wall median falls 6.8%;
the worst median wall change is +0.4%. Baseline allocator-dependent RSS varies
across repetitions, particularly the eight-history cases; full quartiles are
retained rather than presenting one favorable run.

The initial engineering heuristic sought a 15% noisy-run RSS reduction. That
single-workload heuristic was **not met**. The change is retained for the
measured broader 20% storage benefit, 13% noisy-run benefit and negligible
runtime cost. The decision receipt states the missed heuristic explicitly;
no numerical assertion or performance-regression limit was relaxed. This is a
bounded engineering result, not a universal speed or memory guarantee.

## Verification and evidence

All four local GCC/Clang Debug/Release configurations pass 34/34 CTest groups,
including new CLI and repeated embedding storage-limit checks. Full Clang Debug
ASan/UBSan and leak detection pass **34/34 in 229.86 s**. Native-platform
qualification remains required for the pushed revision; it is not inferred
from the benchmark's Release runs.

`artifacts/audit-resolution/performance/reserve/` contains `summary.json`, the
explicit `decision.json`, all per-repetition receipts, fixtures and exact
`measure.py`/`paired_reserve.py` owners. Detailed timing logs and CSV witnesses
remain under `build/audit-resolution/performance/paired-reserve-final/`.
Baseline/candidate binary and source identities are retained separately.

To reproduce the archived owner, run from a source checkout. Place the two
corresponding Release executables at
`build/audit-resolution/performance/bin/{baseline,reserve-candidate}`, copy its
eight fixtures into `build/audit-resolution/performance/fixtures/`, and place
the owners in that performance directory. Run:

```bash
python -B build/audit-resolution/performance/paired_reserve.py
```

The result directory must be new; prior results are preserved. This task-owned
Linux measurement owner uses GNU time and taskset. It is not part of portable
CTest or a claim about runtime on other operating systems.
