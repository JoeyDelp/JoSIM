#!/usr/bin/env python3
"""Build, test, install and verify one JoSIM release artifact."""

import argparse
import csv
import datetime
import hashlib
import json
import os
import pathlib
import platform
import re
import shutil
import struct
import subprocess
import sys
import tarfile
import tempfile
import zipfile
import xml.etree.ElementTree as ET


TARGETS = {
    "linux-x64-rhel6": {"system": "Linux", "machine": 62, "glibc": "2.12"},
    "linux-arm64-rhel7": {"system": "Linux", "machine": 183, "glibc": "2.17"},
    "windows-x64": {"system": "Windows", "machine": 0x8664},
    "macos-arm64": {"system": "Darwin", "arch": "arm64", "minimum": "11.0"},
    "macos-x86_64": {"system": "Darwin", "arch": "x86_64", "minimum": "10.15"},
}


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def command(args, log=None, cwd=None):
    args = [str(arg) for arg in args]
    if log:
        with log.open("w", encoding="utf-8") as stream:
            result = subprocess.run(args, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError("Command failed: {}\n{}".format(args, log.read_text(errors="replace")[-10000:]))
        return ""
    result = subprocess.run(args, cwd=cwd, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError("Command failed: {}\n{}{}".format(args, result.stdout, result.stderr))
    return result.stdout


def version_tuple(value):
    parts = tuple(int(part) for part in value.split("."))
    return parts + (0,) * max(0, 3 - len(parts))


def compiler_metadata(build):
    result = {}
    for language in ("C", "CXX"):
        candidates = list((build / "CMakeFiles").glob("*/CMake{}Compiler.cmake".format(language)))
        if len(candidates) != 1:
            raise RuntimeError("Cannot identify the configured {} compiler".format(language))
        text = candidates[0].read_text()
        fields = {}
        for field in ("COMPILER", "COMPILER_ID", "COMPILER_VERSION"):
            value = re.search(r'set\(CMAKE_' + language + '_' + field + r' "([^"]*)"\)', text)
            if not value:
                raise RuntimeError("Missing compiler metadata: " + language + "_" + field)
            fields[field.lower()] = value.group(1)
        result[language] = fields
    return result


def inspect_binary(binary, target, cache):
    data = binary.read_bytes()
    if target["system"] == "Linux":
        if data[:5] != b"\x7fELF\x02" or struct.unpack_from("<H", data, 18)[0] != target["machine"]:
            raise RuntimeError("ELF architecture does not match the artifact target")
        symbols = command(["objdump", "-T", binary])
        versions = re.findall(r"GLIBC_([0-9.]+)", symbols)
        if not versions:
            raise RuntimeError("No dynamic glibc versions found; ABI inspection is incomplete")
        maximum = max(versions, key=version_tuple)
        if version_tuple(maximum) > version_tuple(target["glibc"]):
            raise RuntimeError("Binary requires glibc {}, exceeding {}".format(maximum, target["glibc"]))
        dependencies = re.findall(r"NEEDED\s+(\S+)", command(["objdump", "-p", binary]))
        if any("libstdc++" in name or "libgcc_s" in name for name in dependencies):
            raise RuntimeError("Portable Linux artifact still depends on a shared C++ runtime")
        return {"maximum_glibc_symbol": maximum, "dynamic_libraries": dependencies}
    if target["system"] == "Darwin":
        architecture = command(["lipo", "-archs", binary]).strip()
        if architecture != target["arch"]:
            raise RuntimeError("Mach-O architecture differs from the requested target: " + architecture)
        load_commands = command(["otool", "-l", binary])
        minimum = re.search(r"\bminos\s+([0-9.]+)", load_commands)
        if not minimum:
            minimum = re.search(r"cmd LC_VERSION_MIN_MACOSX.*?\bversion\s+([0-9.]+)", load_commands, re.S)
        if not minimum or version_tuple(minimum.group(1)) > version_tuple(target["minimum"]):
            raise RuntimeError("Mach-O minimum macOS version exceeds the requested deployment target")
        dependencies = [line.strip().split(" (")[0] for line in command(["otool", "-L", binary]).splitlines()[1:]]
        if any(not name.startswith(("/usr/lib/", "/System/Library/")) for name in dependencies):
            raise RuntimeError("macOS artifact has an unbundled private dynamic dependency")
        return {"minimum_macos": minimum.group(1), "dynamic_libraries": dependencies}
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset:pe_offset + 4] != b"PE\0\0" or struct.unpack_from("<H", data, pe_offset + 4)[0] != target["machine"]:
        raise RuntimeError("PE architecture does not match Windows x64")
    linker = pathlib.Path(cache.get("CMAKE_LINKER", ""))
    dumpbin = linker.with_name("dumpbin.exe")
    if not dumpbin.exists():
        found = shutil.which("dumpbin.exe")
        if not found:
            raise RuntimeError("dumpbin is required to verify Windows runtime dependencies")
        dumpbin = pathlib.Path(found)
    imports = command([dumpbin, "/DEPENDENTS", binary])
    libraries = sorted(set(re.findall(r"\b[A-Za-z0-9_.-]+\.dll\b", imports, re.I)))
    if any(re.search(r"msvcp|vcruntime|ucrtbase|api-ms-win-crt", name, re.I) for name in libraries):
        raise RuntimeError("Windows artifact still requires a redistributable C/C++ runtime: " + str(libraries))
    return {"dynamic_libraries": libraries}


def source_identity(source, revision):
    try:
        actual_revision = command(["git", "rev-parse", "HEAD"], cwd=source).strip()
    except RuntimeError:
        actual_revision = None
    revision = revision or actual_revision
    if not revision or not re.fullmatch(r"[0-9a-fA-F]{40}", revision):
        raise RuntimeError("Provide the full source Git revision with --revision")
    if actual_revision:
        if actual_revision != revision:
            raise RuntimeError("Requested revision differs from the source checkout")
        names = command(["git", "ls-files", "-z"], cwd=source).split("\0")
        digest = hashlib.sha256()
        for name in sorted(filter(None, names)):
            digest.update(name.encode() + b"\0" + (source / name).read_bytes() + b"\0")
        dirty = bool(command(["git", "status", "--porcelain", "--untracked-files=no"], cwd=source).strip())
        return {"revision": revision, "working_tree_sha256": digest.hexdigest(), "dirty": dirty}
    manifest_path = source.parent / "bundle-manifest.json"
    if not manifest_path.exists():
        raise RuntimeError("Archive build requires bundle-manifest.json beside the source directory")
    metadata = json.loads(manifest_path.read_text())
    if metadata.get("revision", metadata.get("base")) != revision:
        raise RuntimeError("Source bundle revision differs from --revision")
    return {"revision": revision, "bundle_content_sha256": metadata["content_sha256"],
            "dirty": metadata.get("source_dirty", True)}


def package(stage, artifact_dir, build, name, manifest):
    manifest["files"] = [{"path": str(path.relative_to(stage)).replace(os.sep, "/"),
                          "size": path.stat().st_size, "sha256": sha256(path)}
                         for path in sorted(stage.rglob("*")) if path.is_file()]
    manifest_text = json.dumps(manifest, indent=2) + "\n"
    (stage / "MANIFEST.json").write_text(manifest_text, encoding="utf-8")
    extension = ".zip" if manifest["target"] == "windows-x64" else ".tar.gz"
    candidate = build / (name + ".candidate")
    if extension == ".zip":
        with zipfile.ZipFile(candidate, "w", zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(stage.rglob("*")):
                if path.is_file():
                    archive.write(path, str(path.relative_to(stage)))
    else:
        with tarfile.open(candidate, "w:gz") as archive:
            archive.add(stage, arcname=".")
    extracted = build / "package-check"
    if extracted.exists():
        shutil.rmtree(extracted)
    extracted.mkdir()
    if extension == ".zip":
        with zipfile.ZipFile(candidate) as archive:
            archive.extractall(extracted)
    else:
        with tarfile.open(candidate) as archive:
            for member in archive.getmembers():
                if pathlib.PurePosixPath(member.name).is_absolute() or ".." in pathlib.PurePosixPath(member.name).parts:
                    raise RuntimeError("Unsafe path in generated archive")
            archive.extractall(extracted)
    for item in manifest["files"]:
        if sha256(extracted / item["path"]) != item["sha256"]:
            raise RuntimeError("Packaged payload hash mismatch: " + item["path"])
    deck = extracted / "package-smoke.cir"
    deck.write_text("V1 1 0 DC 1\nR1 1 0 1k\n.tran 1p 5p DST\n.print V(1)\n.end\n")
    executable = extracted / "bin" / ("josim-cli.exe" if extension == ".zip" else "josim-cli")
    command([executable, "-m", "1", "-o", "smoke.csv", deck], log=build / "package-smoke.log", cwd=extracted)
    with (extracted / "smoke.csv").open() as stream:
        rows = list(csv.reader(stream))
    if len(rows) < 2 or abs(float(rows[-1][1]) - 1.0) > 1e-6:
        raise RuntimeError("Extracted binary failed its numerical smoke test")
    artifact_dir.mkdir(parents=True, exist_ok=True)
    output = artifact_dir / (name + extension)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=artifact_dir, prefix="." + name, suffix=".partial", delete=False) as stream:
            temporary = pathlib.Path(stream.name)
            with candidate.open("rb") as source_stream:
                shutil.copyfileobj(source_stream, stream)
        if sha256(temporary) != sha256(candidate):
            raise RuntimeError("Verified archive changed during final artifact copy")
        temporary.replace(output)
        candidate.unlink()
    finally:
        if temporary and temporary.exists():
            temporary.unlink()
    (artifact_dir / (name + ".manifest.json")).write_text(manifest_text, encoding="utf-8")
    (artifact_dir / (name + extension + ".sha256")).write_text(sha256(output) + "  " + output.name + "\n")
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", required=True, choices=sorted(TARGETS))
    parser.add_argument("--build-dir", required=True, type=pathlib.Path)
    parser.add_argument("--dependencies", required=True, type=pathlib.Path)
    parser.add_argument("--artifacts", required=True, type=pathlib.Path)
    parser.add_argument("--revision")
    parser.add_argument("--release-tag")
    parser.add_argument("--generator")
    parser.add_argument("--parallel", type=int, default=4)
    args = parser.parse_args()
    target = TARGETS[args.target]
    if platform.system() != target["system"]:
        raise RuntimeError("This target must be built on " + target["system"])
    source = pathlib.Path(__file__).resolve().parents[1]
    build = args.build_dir.resolve()
    deps = args.dependencies.resolve()
    artifacts = args.artifacts.resolve()
    build.mkdir(parents=True, exist_ok=True)
    identity = source_identity(source, args.revision)
    cmake = shutil.which("cmake")
    if not cmake:
        raise RuntimeError("CMake is required")
    command([cmake, "-DJOSIM_DEPENDENCY_DIR=" + str(deps), "-P", source / "cmake/prepare_dependencies.cmake"],
            log=build / "dependencies.log")
    configure = [cmake, "-S", source, "-B", build, "-C", deps / "sources.cmake",
                 "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=OFF", "-DUSING_OPENMP=OFF", "-DBUILD_TESTING=ON",
                 "-DMAKING_STATIC_BUILD=" + ("OFF" if target["system"] == "Darwin" else "ON"),
                 "-DJOSIM_SOURCE_REVISION=" + identity["revision"]]
    if args.generator:
        configure += ["-G", args.generator]
    if target["system"] == "Windows":
        configure += ["-A", "x64"]
    if target["system"] == "Darwin":
        configure += ["-DCMAKE_OSX_ARCHITECTURES=" + target["arch"],
                      "-DCMAKE_OSX_DEPLOYMENT_TARGET=" + target["minimum"]]
    command(configure, log=build / "configure.log")
    command([cmake, "--build", build, "--config", "Release", "--parallel", args.parallel], log=build / "build.log")
    ctest = pathlib.Path(cmake).with_name("ctest.exe" if os.name == "nt" else "ctest")
    junit = build / "test-results.xml"
    command([ctest, "--test-dir", build, "-C", "Release", "--output-on-failure", "--timeout", "90",
             "--output-junit", junit], log=build / "test.log")
    stage = build / "stage"
    if stage.exists():
        shutil.rmtree(stage)
    command([cmake, "--install", build, "--config", "Release", "--prefix", stage], log=build / "install.log")
    cache = {}
    for line in (build / "CMakeCache.txt").read_text().splitlines():
        match = re.fullmatch(r"([^/#:=][^:=]*):[^=]*=(.*)", line)
        if match:
            cache[match.group(1)] = match.group(2)
    binary = stage / "bin" / ("josim-cli.exe" if os.name == "nt" else "josim-cli")
    abi = inspect_binary(binary, target, cache)
    declared_version = cache["CMAKE_PROJECT_VERSION"]
    if args.release_tag:
        if args.release_tag != "v" + declared_version or identity.get("dirty"):
            raise RuntimeError("Release tag must match the declared version and a clean source tree")
        label = args.release_tag
    else:
        label = declared_version + "-dev." + identity["revision"][:7]
        if identity.get("dirty"):
            label += ".dirty"
    tests = ET.parse(junit).getroot()
    cases = tests.findall(".//testcase")
    required = {"regression::noise", "regression::gate", "regression::cli", "regression::installed_package"}
    if not required.issubset({case.attrib.get("name") for case in cases}):
        raise RuntimeError("Required numerical, CLI or installed-package regressions did not run")
    if any(case.find("skipped") is not None or case.find("failure") is not None or case.find("error") is not None for case in cases):
        raise RuntimeError("A qualification test was skipped or failed")
    registry = (source / "cmake/dependency_sources.cmake").read_text()
    dependencies = {}
    for name, url in re.findall(r'set\(JOSIM_(\w+)_URL "([^"]+)"\)', registry):
        checksum = re.search(r'set\(JOSIM_' + name + r'_SHA256 "([0-9a-f]{64})"\)', registry)
        if not checksum:
            raise RuntimeError("Missing dependency checksum: " + name)
        dependencies[name] = {"url": url, "sha256": checksum.group(1)}
    manifest = {"project": "JoSIM", "version": declared_version, "target": args.target, "source": identity,
                "built_at_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                "host": {"system": platform.system(), "machine": platform.machine(), "release": platform.release(), "libc": platform.libc_ver()},
                "toolchain": {"cmake": command([cmake, "--version"]).splitlines()[0],
                              "compilers": compiler_metadata(build)},
                "dependencies": dependencies,
                "configuration": {key: cache.get(key) for key in ["CMAKE_GENERATOR", "CMAKE_BUILD_TYPE", "BUILD_SHARED_LIBS", "MAKING_STATIC_BUILD", "USING_OPENMP", "CMAKE_OSX_ARCHITECTURES", "CMAKE_OSX_DEPLOYMENT_TARGET"]},
                "tests": {"entries": len(cases), "status": "passed"}, "abi": abi}
    output = package(stage, artifacts, build, "JoSIM-" + label + "-" + args.target, manifest)
    print("Verified artifact:", output)
    print("SHA256:", sha256(output))


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, ValueError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
