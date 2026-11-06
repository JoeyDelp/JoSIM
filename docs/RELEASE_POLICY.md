# Building and publishing JoSIM binaries

`tools/build_release.py` owns configuration, compilation, tests, installation,
binary inspection and packaging. GitHub Actions calls the same script used for
local qualification.

| Artifact target | Architecture | Declared runtime baseline |
|---|---|---|
| `linux-x64-rhel6` | x86_64 | glibc 2.12 |
| `linux-arm64-rhel7` | AArch64 | glibc 2.17 |
| `windows-x64` | x86_64 | Windows 10; static MSVC runtime |
| `macos-arm64` | arm64 | macOS 11.0 deployment target |
| `macos-x86_64` | x86_64 | macOS 10.15 deployment target |

The Linux jobs use digest-pinned manylinux images. Dependencies are downloaded
and verified on the modern runner before the legacy container runs without
network access. Linux packages link the C++ runtime statically; macOS packages
use system libraries. Windows packages must not require a separate MSVC runtime.

These baselines are checked from binary metadata. RHEL6 qualification uses a
CentOS 6.10 userspace on a modern kernel. It does not establish behavior on an
original RHEL6 kernel. Intel macOS local tests run through Rosetta; CI uses an
Intel runner. Deployment metadata alone does not establish execution on every
older macOS release.

## Local qualification

Run on the target operating system with CMake 3.21+, Python 3 and a supported
compiler. For example, on the macOS testing host:

```bash
python3 -B tools/build_release.py \
  --target macos-arm64 \
  --build-dir build/macos-arm64 \
  --dependencies build/dependencies \
  --artifacts artifacts
```

Use `--generator "Visual Studio 17 2022"` for Windows. Run the Linux targets in
their exact images and mount paths from `.github/workflows/build-release.yml`.
A current Linux distribution cannot establish the older glibc baseline.

Each successful run must:

1. Pass CTest, including every required regression group and a fresh installed-library
   consumer. Skipped tests fail qualification.
2. Match the requested architecture and runtime baseline.
3. Install into a clean staging directory and hash every payload file.
4. Extract the resulting archive, verify its hashes and run a numerical smoke
   test using the extracted executable.

The workflow also requires default/shared library-mode qualification on all
five native targets, including fresh consumers after installation relocation.
`tools/verify_library.py` owns these checks. Released archives use the static
JoSIM/runtime configuration stated above; shared-library source-build checks
do not change the published binary format or runtime baseline.

Archives include `MANIFEST.json`. A sidecar manifest records source identity,
compiler versions, dependency hashes, configuration, test count and binary ABI
inspection. A `.sha256` file identifies the archive itself.

## Publication

Branch builds and pull requests upload development artifacts. They do not
publish a release. Development names include the Git revision and mark dirty
source trees explicitly.

Before a release, review the changelog, set the intended version in
`CMakeLists.txt`, and qualify all five targets. A `v<version>` tag must match the
declared version and a clean source tree. The tagged workflow publishes only
after every target passes. A tag is a separate publication decision; passing
tests does not authorize creating one.

Source archives used outside Git require a sibling `bundle-manifest.json` with
the full revision, source state and bundle hash. The script accepts this only
with a matching `--revision`; it never silently assigns a release identity.
