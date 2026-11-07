# Building releases

The release workflow builds the command-line executable and C++ library for:

| Package | Minimum runtime |
|---|---|
| `linux-x64-rhel6` | x86_64, glibc 2.12 |
| `linux-arm64-rhel7` | AArch64, glibc 2.17 |
| `windows-x64` | Windows 10, x64 |
| `macos-arm64` | macOS 11.0, Apple Silicon |
| `macos-x86_64` | macOS 10.15, Intel |

Linux packages link the C++ runtime statically. Windows packages use the static
MSVC runtime and do not need a separate runtime installation. macOS packages
use system libraries. Linux tests run in CentOS-compatible userspaces on modern
kernels; macOS tests run on the available native CI runners.

## Build a package

`tools/build_release.py` configures, compiles, tests and packages JoSIM. Run it
on the target operating system. For example, on an Apple Silicon Mac:

```bash
python3 -B tools/build_release.py \
  --target macos-arm64 \
  --build-dir build/macos-arm64 --dependencies build/dependencies \
  --artifacts artifacts
```

On Windows, add `--generator "Visual Studio 17 2022"`. The Linux builds use the
container images and commands in
[build-release.yml](https://github.com/JoeyDelp/JoSIM/blob/master/.github/workflows/build-release.yml).
Dependencies are downloaded before entering the Linux containers.

Each package is installed into a clean directory, checked against its target
architecture and runtime, extracted and run. Library tests also build external
applications against static and shared installations.

The archive contains `MANIFEST.json`, which lists its files and hashes. The
release includes a separate manifest with source and compiler details, and a
`.sha256` checksum for each archive.

## Publish a version

Update the version in `CMakeLists.txt`, the changelog and the corresponding
notes under `docs/releases/`. Compatible bug fixes use a patch version;
compatible additions use a minor version. Changes to the public C++ interface
that require rebuilding applications use a major version.

A `v<version>` tag must match the version in CMake. GitHub Actions builds and
tests all targets before publishing their archives. It then downloads the
published files and runs the CLI and numerical tests on each platform.
Existing release files are not overwritten on a rerun.

To repeat the download checks locally, use `python3 -B tools/verify_release.py --help`. Linux can download the files first and execute them separately inside
the appropriate container.

## Publish the manual

Build and check the manual using the [development instructions](DEVELOPMENT.md#build-the-manual),
then publish it with MkDocs:

```bash
git fetch origin gh-pages
build/documentation-env/bin/mkdocs gh-deploy --strict \
  --site-dir build/documentation --remote-name origin --remote-branch gh-pages
```

Use the existing `gh-pages` history. A custom domain, if used, is set by `CNAME`.
After deployment, check the live pages and download links.
