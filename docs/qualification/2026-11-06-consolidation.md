# JoSIM consolidation and qualification

Master is consolidated and published at `53b6a426570eafb53d3673f79254157a4ae2c001`.
[Master CI](https://github.com/JoeyDelp/JoSIM/actions/runs/42193847486) passed all
five targets. Each target runs the 27-test suite, tests a fresh installed-library
consumer, checks binary architecture/runtime requirements, then extracts and
runs the packaged binary. The fresh local GNU 16.2.1 build also passed 27/27.

## Changes delivered

- Gate-controlled junctions and configurable IV sweeps, with regression coverage.
- Validated power-law, flux and fractional critical-current noise. Corrected
  noise timing, thermal amplitude storage and out-of-band normalization.
- Focused fixes for RNG replay, current C compilers, old-glibc timing linkage,
  invalid transient settings, recursive subcircuits and installed CMake packages.
- Pinned dependency archives and a shared five-platform release workflow.
- Public [changelog](https://github.com/JoeyDelp/JoSIM/blob/master/CHANGELOG.md),
  development/release procedures and updated source documentation.

## Current CI binaries

The links below download GitHub Actions artifact ZIPs. Their digests identify
those outer ZIPs. Inside each ZIP, the platform archive has its own SHA-256
sidecar and payload/toolchain manifest.

| CI artifact | Actions ZIP SHA-256 |
|---|---|
| [JoSIM-linux-arm64-rhel7](https://github.com/JoeyDelp/JoSIM/actions/runs/42193847486/artifacts/13490727990) | 0b93c764e069f0c9ca829b277c27a7bc6b876dd98b491e3d55d57949ccd26583 |
| [JoSIM-linux-x64-rhel6](https://github.com/JoeyDelp/JoSIM/actions/runs/42193847486/artifacts/13490542603) | a19277c1221b5c2fbd152d2662549879ad6ccae34eb24f7f3ca6943b2da4dc15 |
| [JoSIM-macos-arm64](https://github.com/JoeyDelp/JoSIM/actions/runs/42193847486/artifacts/13491126533) | fe3a12112b5662ec9af0f7ffbb5fec094bf131f6773fa240f75ded3b7fce7e30 |
| [JoSIM-macos-x86_64](https://github.com/JoeyDelp/JoSIM/actions/runs/42193847486/artifacts/13490886911) | 0ee7633a76f501fd1537aebd8a53f8ceebb8e6ce97f954a92c975060145f51d3 |
| [JoSIM-windows-x64](https://github.com/JoeyDelp/JoSIM/actions/runs/42193847486/artifacts/13491630615) | ae1020b4c0f13fa948384ae13b9b06644149cca08b7eb2662d3c94dd5ec8591f |

Targets: Linux x86_64/glibc 2.12, Linux AArch64/glibc 2.17, Windows x64,
macOS arm64/minimum 11.0, and macOS x86_64/minimum 10.15. RHEL6 userspace was
tested on a modern kernel; original RHEL6 kernels and the oldest declared
macOS releases were not exercised.

The four local `JoSIM-2.7-dev.1866d03-*` packages retain their exact pre-push
qualification identity. Subsequent commit `53b6a42` changes CI launch settings
and the changelog. Current five-target binaries are the CI artifacts above.
The in-app browser blocked copying those ZIPs locally; their publication,
source identity and outer digests were verified through GitHub's API.

## History and cleanup

GitHub and local development branches are now `master` and `gh-pages`.
Superseded feature/testing/task branches were deleted after verifying ancestry
and these preservation tags:

- `archive/noise-research-2026-11-06` → `c7c2de3`: full original research inputs.
- `archive/gate-stash-2025-12-15` → `3aac073`: full original gate stash.
- `archive/single-precision-2021-02-03` → `90921bc`: archived float experiment.

The original root checkout's exact tracked diff and Debug files are preserved
in local stash `43382d51e9ed728a50e42daa55426bb6f8050c8f`. The old gate stash is
also retained. Existing local configuration and the untracked `optional/` input
were not published or removed. The tracked working tree is clean.

GitHub accepted the approved master push using the account's existing owner
bypass of its pull-request rule. No protection settings were changed. Version
2.7 and its tag remain unchanged; no new release or version tag was created.
The generated `gh-pages` website was retained without deployment.

Superseded task builds/source exports were removed after publication. Compact
logs, failure witnesses, research methods/data, source identities and current
payloads/manifests remain. Pinned Docker images are reusable caches (~2.90 GiB).
The project build directory is about 0.08 GiB; no budget exception is retained.

The remaining open numerical and SPICE-compatibility reports need separate
investigation. SuperLU is still non-selectable; KLU is the qualified backend.
