# JoSIM repository assessment — 6 November 2026

## Assessment

JoSIM has a compact, understandable simulator core that is worth maintaining.
The priority is a reliable public baseline: numerical regression coverage,
focused noise and input fixes, reproducible dependencies, working library
packaging, and accurate documentation. A rewrite or broad feature expansion
would increase the work before addressing the confirmed problems.

This was a single-agent investigation requested by Johannes Delport. No
simulation code, branches, tags, stashes, issues, PRs or releases were changed.
Only ignored setup/context and audit outputs were written.

## Source and public state

- Public master and v2.7 are both
  `02a34ee5e7a3a6952b21ccc726fbf7a6d5e2b224`, dated 19 December 2025.
- Public GitHub API read on 6 November 2026: 11 open issues, no open PRs,
  112 stars, 41 forks. Last source push was 19 December 2025; repository metadata
  updates do not imply source activity.
- Latest release: [v2.7](https://github.com/JoeyDelp/JoSIM/releases/tag/v2.7).
  Its five assets totalled 1,671 download events at inspection. These are asset
  downloads, not unique users or installed-user counts.
- Last tagged [build/release workflow](https://github.com/JoeyDelp/JoSIM/actions/runs/20365213718)
  succeeded. Targets: Linux x64/arm64, macOS arm64/x64, Windows x64.
- Public master is marked protected in the branch API; protection settings
  themselves were not retrieved.
- Local `noise_research` is at the public commit, with no separate committed
  research history. It contains existing substantive edits and untracked files.
  Most of the 130 modified tracked paths differ only in line endings; ignoring
  end-of-line whitespace leaves 13 substantively changed tracked paths.
- Local work includes PNOISE/FNOISE/ICNOISE, model parsing, phase-step logic,
  resistor bandwidth changes, Debug code and ignored test_internal experiments.
  These are separate from released JoSIM and were not rebuilt or qualified.
- A gate-tunable-JJ stash also exists. It was not applied or modified.

The public commit was exported into build/audit-2026-11-06/public-source for
isolated checks. Tests therefore did not exercise uncommitted research code.

## Branches

| Branch | Last commit | Behind / ahead of master | Assessment |
|---|---|---:|---|
| master | 02a34ee, 2025-12-19 | 0 / 0 | Current public baseline, tagged v2.7. |
| gate-tunable-jj | a74ae37, 2025-11-26 | 12 / 4 | Only public feature branch with recent independent work. |
| single_precision | 90921bc, 2021-02-03 | 207 / 1 | Old float-conversion experiment; unsuitable as a current baseline. |
| testing | e0c03a6, 2022-08-17 | 45 / 0 | Fully contained in master; no unmerged commits. |
| gh-pages | 8a51dfd, 2025-12-18 | Separate generated history | Published MkDocs output, not development source. |
| noise_research, local only | 02a34ee | 0 / 0 committed | Existing uncommitted research described above. |

The gate branch's substantive delta is eight paths, 110 insertions and 43
deletions after ignoring line endings. It adds gate-controlled critical current,
GWIDTH/GC handling and IV changes. The new gtjj_iv.cir is not registered in the
CTest suite. Gate indexing silently catches lookup failures; the simulation
reads the gate node directly from x_, which needs an explicit voltage/phase-mode
review before integration. This is a code-review concern, not a reproduced
physical-model failure. No experimental branch was compiled in this audit.

## Open issue triage

Every open issue body and every available comment was read through the public
GitHub API. A title alone is often an inaccurate description of the remaining
work.

| Issue | Classification | Evidence and action |
|---|---|---|
| [#110 Seed](https://github.com/JoeyDelp/JoSIM/issues/110) | Resolved support request | Reporter used 2.6.10 and confirmed v2.7 worked. Two fresh-process seeded runs matched here. Close after a final triage note; a separate same-process RNG bug is described below. |
| [#109 Capacitor initial voltage](https://github.com/JoeyDelp/JoSIM/issues/109) | Feature request | No capacitor IC implementation found. Define startup/history semantics before adding it. |
| [#108 Sine stop](https://github.com/JoeyDelp/JoSIM/issues/108) | Implemented request plus separate noise problem | Finite-cycle sine support is in v2.7 and passed a focused probe. Later discussion concerns switching-dependent JJ thermal noise; that concern remains valid. |
| [#107 Floating reference and SQUID noise](https://github.com/JoeyDelp/JoSIM/issues/107) | Support/documentation plus numerical validation | Floating-reference explanation was supplied. Follow-up about .NEB, sampling and Josephson frequencies remains unanswered. Needs a validated example, not a blanket bandwidth prescription. |
| [#101 JJ buffer](https://github.com/JoeyDelp/JoSIM/issues/101) | Untriaged circuit/model report | Includes a netlist but no independent expected trace or comparison. Do not classify the reported reverse response as a solver bug yet. |
| [#93 Stewart–McCumber parameter](https://github.com/JoeyDelp/JoSIM/issues/93) | Feature request | Useful parameter convenience. Need to define which resistance enters beta_c for nonlinear RCSJ models. |
| [#91 Long-run JJ artifacts](https://github.com/JoeyDelp/JoSIM/issues/91) | Numerical investigation priority | No reproducer or quantitative tolerance attached. Long phase accumulation, timestep convergence and voltage/phase agreement need measured regressions. Root cause not established. |
| [#88 Nonlinear inductor](https://github.com/JoeyDelp/JoSIM/issues/88) | Substantial physics/solver feature | Requires correct flux linkage and nonlinear solve/stamping semantics; not simply substituting L(I) into a constant-L stamp. |
| [#84 Parameter sweep](https://github.com/JoeyDelp/JoSIM/issues/84) | Previously declined feature | Existing answer delegates sweeps to external scripts. Clarify support policy and provide a current example if that remains the intended boundary. |
| [#82 ivcurve.py](https://github.com/JoeyDelp/JoSIM/issues/82) | Mixed support/documentation | Original script usage was answered; .iv supersedes model-only work. Later circuit-IV and solver/error-control questions should be separated. Script rewrites its input file. |
| [#81 Subcircuits](https://github.com/JoeyDelp/JoSIM/issues/81) | Reproduced compatibility limitations | Named RES overrides fail; .print V(IN.X1) warns and drops the request. Component-name overrides are not equivalent to named SPICE parameters. |

No issue was closed, relabelled or commented on during the audit.

## Architecture and code condition

The public source contains about 9,500 lines across src/*.cpp and
include/JoSIM/*.hpp, excluding the bundled spline header and external solvers.
The major owners are already distinct:

1. Input, Netlist, Parameters and Model parse controls and flatten subcircuits.
2. Device classes own matrix stamps; Matrix builds sparse CSR arrays.
3. Simulation uses second-order Gear/BDF-style history and KLU sparse solves.
4. Output converts requested quantities and writes CSV, DAT or RAW.
5. IV owns junction-model IV generation; Python scripts support plotting.

This is a useful basis for targeted maintenance. Device equations are documented
next to stamps and in the technical manual. The runtime avoids storing
unrequested node traces, and deterministic noise/spread streams already exist.

Important limits:

- It is a specialized transient simulator with SPICE-like syntax, not a general
  SPICE implementation. No general OP/DC/AC analysis, .STEP or general initial
  condition facility was found in the public baseline.
- The timestep mechanism restarts the whole run at half the previous fixed step.
  It is not a conventional variable-step solver with RelTol/AbsTol control.
  The phase-change check starts after the first 1% of the run; this is not a
  numerical error bound. The cause of #91 remains unproven.
- Every internal timestep is retained for each selected trace before output
  decimation. Memory scales approximately as 8*Nsteps*(Ntraces+1) bytes for
  double payloads, plus container/output overhead. Long noisy runs matter here.
- C++ library users assemble low-level objects and can encounter global RNG
  state. Some error paths call exit(), which is unsuitable for an embeddable
  recoverable API. No Python binding/package source is present in this checkout,
  despite historical pyjosim references in issue replies.
- CTest always uses minimal output and checks process status. It does not assert
  waveforms, circuit laws, pulse counts, noise spectra, numerical convergence,
  cross-mode agreement, or installed-library consumption. Several shipped
  example circuits are not registered as tests.

## Reproduced findings

### Noise state and amplitude

1. Rng::start_run() reseeds engines, but normal01_noise() keeps a separate
   thread_local Box–Muller spare sample. Reseeding after an odd draw therefore
   does not reproduce the first sample. With seed 123, the first draw was
   0.79079247321936319; reseeding then returned -0.27958039327489437.
   Rng::rewind() has the same reset ownership issue. This matters for in-process
   repeats and timestep retries; fresh-process determinism passed.
   Owners: public src/Rng.cpp:20 and :88.

2. Function::ampValues() returns a vector by value. JJ::update_value() assigns
   into that copy at the subgap and normal transitions. A direct public-library
   probe changed the junction to 10 mV with R0=160 ohm and RN=16 ohm. Thermal
   amplitude stayed at 1.2040269706281501e-12 A/sqrt(Hz); the intended normal-state
   amplitude ratio is sqrt(160/16)=3.1622776601683795, while the measured stored
   ratio was 1. Owners: include/JoSIM/Function.hpp:42; src/JJ.cpp:238 and :277.
   This is an implementation bug independently of the remaining physical-model
   questions about transition-region noise.

### Build and package compatibility

3. Default GCC 16.2.1 fails while compiling legacy SuperLU's cgstrs.c:
   cprint_soln() is declared without parameters, then defined with three.
   Modern default C semantics expose the conflict. Explicit
   -DCMAKE_C_STANDARD=17 built successfully and passed all 23 CTests.
   SuperLU is built even when the default KLU backend is used.

4. Installing the Clang build succeeds. A fresh consumer calling
   find_package(josim CONFIG REQUIRED) fails because the exported configuration
   references missing imported targets suitesparse::klu, superlu::superlu and
   cblas::cblas. The config needs to load its dependencies. A successful CLI
   package does not establish a usable libjosim development package.

5. -DSLU=ON configures but does not define the SLU compiler macro. Forcing the
   macro causes a syntax error because Simulation.hpp also declares a member
   named SLU. The advertised alternative backend is not selectable through the
   existing option as written. KLU was the backend exercised here.

6. CMakeSuiteSparse is fetched from a moving master.zip without a hash; a
   VERSION label does not pin that URL. SuperLU and CBLAS are commit-pinned.
   The source minimum is CMake 3.14, C++17; the C dialect is unspecified.
   Legacy static-build flags and unconditional stdc++fs linkage also deserve
   cleanup, but were not reproduced as separate portability failures.

### Input behavior

7. .tran -1p 5p DST on a one-resistor circuit terminates with SIGSEGV rather
   than a validation error. Transient.cpp:64 checks zero but not finite positive
   bounds. A self-recursive subcircuit fails to terminate within a 2-second
   bounded probe; Netlist.cpp:146 has no cycle/progress rejection.

8. The #81 named parameter example exits 255 with undefined RES. The pin-alias
   print example exits 0 with a warning and ignores the requested trace.

9. The manual promises the run seed in -V 1, but Verbose.cpp contains no seed
   printout, and the fresh-process probes did not print it. Seed selection itself
   works. This is a remaining documentation/observability mismatch.

Output failure was also checked: requesting missing/output.csv correctly failed
with exit 255 and a useful error. It is not a finding against output handling.

## Documentation and maintenance

- The published homepage and docs/index.md still identify v2.6.5, refer to old
  Azure build badges and state that the current release is 2.5 elsewhere.
  Some syntax content is newer, so the manual mixes generations.
- Build instructions focus on CentOS 7. Current GitHub Actions already provides
  a better source for the supported platform matrix.
- Plotting scripts have no dependency manifest. All six Python files passed
  Python syntax parsing; imports, plotting behavior and modern NumPy/SciPy
  runtime compatibility were not exercised.
- ivcurve.py assumes fixed netlist/output positions and rewrites its input file.
  Document/deprecate it in favour of .iv for supported use cases; do not promise
  that .iv replaces arbitrary whole-circuit IV sweeps.
- PROJECT.md was a blank local template; referenced docs/DEVELOPMENT.md and
  docs/RELEASE_POLICY.md do not exist. PROJECT.md now records discovered commands
  and ownership. These setup/context files are ignored local material.

## Verification and evidence

Environment: local Linux, CMake 4.4.5, Ninja, Clang 23.1.4, GCC 16.2.1. Public
source unmodified; Release builds; OpenMP off; static JoSIM library; default KLU.
Cached dependencies were supplied through FETCHCONTENT_SOURCE_DIR_CMAKESUITESPARSE,
FETCHCONTENT_SOURCE_DIR_CMAKESUPERLU and FETCHCONTENT_SOURCE_DIR_CMAKECBLAS.
This is not a clean-network dependency-download test.

| Check | Result |
|---|---|
| Public baseline / Clang default C dialect | Build passed; 23/23 CTests passed. |
| Public baseline / GCC default C dialect | Failed in SuperLU cgstrs.c. |
| Same GCC build / explicit C17 | Build passed; 23/23 CTests passed. |
| Two seeded CLI processes | Byte-identical CSV output. |
| Finite-cycle sine | Zero output after specified stop in focused linear circuit. |
| 1 kohm / 1 pF RC response, 1 ps step | Maximum absolute error after 10 ps approximately 6.18e-7 V against 1-exp(-(t-0.5 ps)/1 ns), including CSV rounding. A sanity check, not broad qualification. |
| Same-process reseed and JJ amplitude transitions | Bugs reproduced by direct C++ library probe. |
| Named subcircuit parameters and pin aliases | Reported limitations reproduced. |
| Negative timestep and recursive subcircuit | SIGSEGV and bounded timeout reproduced; core dumps disabled. |
| Installed CMake consumer | Configure failed on missing dependency targets. |
| SLU option / forced macro | Option ineffective; forced macro fails syntax check. |
| Python scripts | Six syntax checks passed; runtime unverified. |

Raw receipts: build/audit-2026-11-06/ contains compiler/configuration logs,
results.json, public GitHub API snapshots and comments, probes.py/probes.json,
runtime-probe.cpp/runtime-probe.log, consumer failure, dependency tree identities
and binary SHA-256 values. The original GCC failure log is retained as
gcc/default-build.log. Existing build/ and test_internal/ work was preserved.

No Windows/macOS/ARM runtime, fresh-network build, binary download execution,
sanitizer campaign, large-circuit benchmark, full stochastic PSD qualification,
or long-run JJ correctness study was performed. Historical CI success and these
smoke checks do not establish those properties.

## Recommended order

1. Preserve and isolate the existing research. Keep master/v2.7 as the public
   reference; resolve line-ending noise independently of physical changes.
2. Add a small numerical regression layer: analytic RLC behavior, KCL/KVL,
   phase/voltage agreement, JJ pulses and long runs, seeded retries, noise
   variance/PSD, invalid inputs, and installed-library consumption.
3. Fix the reproduced RNG/JJ noise defects, timestep validation and recursive
   expansion. Address #91 using a reproducer and convergence evidence.
4. Pin dependencies, select supported C/C++ standards explicitly, repair the
   exported CMake package and either repair or clearly retire stale options.
5. Refresh version/build/example documentation and triage resolved support
   issues. Retain scientific questions separately from answered usage requests.
6. Review gate-tunable-JJ and local noise research against that baseline before
   adding further model or sweep features. No public version change is implied.

All audit acceptance items are complete: current branch/issue inventory,
architecture assessment, isolated compatibility checks, concrete reproductions,
evidence identities, and recommendations. Repairs remain outside this request.
