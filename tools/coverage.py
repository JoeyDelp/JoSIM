#!/usr/bin/env python3
"""Build and measure first-party C++ coverage using matching Clang/LLVM tools."""

import argparse
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys

from ctest_results import verify_ctest


def first_party_files(source):
    return sorted((source / "src").glob("*.cpp")) + sorted((source / "include/JoSIM").glob("*.hpp"))


def source_digest(source, files):
    digest = hashlib.sha256()
    for path in files:
        digest.update(str(path.relative_to(source)).encode() + b"\0" + path.read_bytes() + b"\0")
    return digest.hexdigest()


def validate_mapping(data, diagnostics, profile_text):
    mismatches = re.findall(r"hash-mismatch: No profile record found for '(.*?)' with hash = (0x[0-9a-fA-F]+)", diagnostics)
    warnings = re.findall(r"warning: ([^\n]*)", diagnostics)
    expected = sum(int(match.group(1)) for warning in warnings
                   if (match := re.fullmatch(r"(\d+) functions have mismatched data", warning)))
    if len(warnings) > 1 or (warnings and not expected) or expected != len(mismatches):
        raise RuntimeError("Unclassified LLVM coverage diagnostic; inspect coverage.json.stderr")
    records = {}
    for name, body in re.findall(r"^  (\S[^\n]*):\n(.*?)(?=^  \S[^\n]*:\n|\Z)", profile_text, re.M | re.S):
        hash_match = re.search(r"Hash: (0x[0-9a-fA-F]+)", body)
        count = re.search(r"Function count: (\d+)", body)
        if hash_match and count and int(hash_match.group(1), 16):
            records.setdefault(name, []).append({"hash": hash_match.group(1), "count": int(count.group(1))})
    reviewed = []
    for name, missing_hash in sorted(set(mismatches)):
        live = records.get(name, [])
        exported = [function["count"] for function in data["functions"] if function["name"] == name]
        if int(missing_hash, 16) or len(live) != 1 or not exported or set(exported) != {live[0]["count"]}:
            raise RuntimeError("Unresolved coverage mapping mismatch: " + name)
        reviewed.append({"function": name, "unused_mapping_hash": missing_hash,
                         "live_record": live[0], "exported_counts": exported})
    return {"status": "validated", "warning_occurrences": len(mismatches), "functions": reviewed}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--initial-cache", type=pathlib.Path)
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--llvm-cov", default="llvm-cov")
    parser.add_argument("--llvm-profdata", default="llvm-profdata")
    parser.add_argument("--parallel", type=int, default=4)
    args = parser.parse_args()
    source = pathlib.Path(__file__).resolve().parents[1]
    files = first_party_files(source)
    initial_digest = source_digest(source, files)
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip()
    source_diff = hashlib.sha256(subprocess.check_output(["git", "diff", "HEAD", "--binary"], cwd=source)).hexdigest()
    owners = {str(path.relative_to(source)): hashlib.sha256(path.read_bytes()).hexdigest()
              for path in (pathlib.Path(__file__).resolve(), source / "tools/ctest_results.py")}
    build, output = args.build_dir.resolve(), args.output.resolve()
    build.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    profiles = build / "profiles"
    if profiles.exists() and any(profiles.iterdir()):
        raise RuntimeError("Profiles already exist; preserve the prior campaign and use a fresh build directory")
    profiles.mkdir(exist_ok=True)
    commands = []

    def run(command, filename, env=None, required=True):
        command = [str(x) for x in command]
        diagnostics = filename + ".stderr"
        with (output / filename).open("w", encoding="utf-8") as stream, (output / diagnostics).open("w", encoding="utf-8") as errors:
            result = subprocess.run(command, cwd=source, env=env, stdout=stream, stderr=errors)
        commands.append({"command": command, "exit": result.returncode, "log": filename, "diagnostics": diagnostics})
        (output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        if required and result.returncode:
            raise RuntimeError("Command failed; see " + str(output / filename))
        return result.returncode

    tools = {}
    for name in (args.cc, args.cxx, args.llvm_cov, args.llvm_profdata, "cmake", "ctest"):
        executable = shutil.which(name)
        if not executable:
            raise RuntimeError("Required executable is missing: " + name)
        text = subprocess.check_output([executable, "--version"], text=True)
        tools[name] = {"executable": executable, "version": text.splitlines()[:3]}
    versions = []
    for name in (args.cxx, args.llvm_cov, args.llvm_profdata):
        match = re.search(r"(?:clang|LLVM) version (\d+)", "\n".join(tools[name]["version"]))
        if not match:
            raise RuntimeError("Cannot identify LLVM tool version: " + name)
        versions.append(match.group(1))
    if len(set(versions)) != 1:
        raise RuntimeError("Clang, llvm-cov and llvm-profdata must have matching major versions")

    configure = ["cmake", "-S", source, "-B", build, "-DCMAKE_BUILD_TYPE=Debug",
                 "-DCMAKE_C_COMPILER=" + tools[args.cc]["executable"],
                 "-DCMAKE_CXX_COMPILER=" + tools[args.cxx]["executable"],
                 "-DJOSIM_COVERAGE=ON", "-DJOSIM_SANITIZERS=OFF", "-DBUILD_TESTING=ON",
                 "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"]
    if args.initial_cache:
        configure += ["-C", args.initial_cache.resolve()]
    run(configure, "configure.log")
    run(["cmake", "--build", build, "--parallel", args.parallel], "build.log")
    inventory = output / "test-inventory.json"
    run(["ctest", "--test-dir", build, "--show-only=json-v1"], "test-inventory.json")
    environment = os.environ.copy()
    environment["LLVM_PROFILE_FILE"] = str(profiles / "%m-%p.profraw")
    junit = output / "tests.xml"
    test_status = run(["ctest", "--test-dir", build, "--output-on-failure", "--no-tests=error", "--timeout", "120",
                       "--output-junit", junit], "test.log", env=environment, required=False)
    try:
        tests = verify_ctest(inventory, junit)
    except RuntimeError as error:
        tests = {"status": "failed", "error": str(error)}
    tests["exit"] = test_status
    raw = sorted(profiles.glob("*.profraw"))
    if not raw:
        raise RuntimeError("No execution profiles were produced")
    profile = output / "coverage.profdata"
    run([args.llvm_profdata, "merge", "-sparse", *raw, "-o", profile], "merge.log")
    executable = build / "josim-cli"
    regressions = sorted(path for path in (build / "test").glob("josim-*-regression") if path.is_file())
    options = [executable, *["-object=" + str(path) for path in regressions], "-instr-profile=" + str(profile)]
    run([args.llvm_cov, "export", *options, *files], "coverage.json")
    run([args.llvm_cov, "report", "-dump", *options, *files], "mapping-dump.txt")
    run([args.llvm_cov, "report", *options, *files], "coverage.txt")
    run([args.llvm_cov, "show", *options, "-format=html", "-output-dir=" + str(output / "html"),
         "-show-branches=count", "-show-line-counts-or-regions", *files], "html.log")
    data = json.loads((output / "coverage.json").read_text())["data"][0]
    run([args.llvm_profdata, "show", "--all-functions", "--counts", profile], "profile-counts.txt")
    mapping = validate_mapping(data, (output / "mapping-dump.txt.stderr").read_text(),
                               (output / "profile-counts.txt").read_text())
    (output / "mapping-diagnostics.json").write_text(json.dumps(mapping, indent=2) + "\n")
    measured = {str(pathlib.Path(item["filename"]).resolve()): item for item in data["files"]}
    unexpected = set(measured) - {str(path) for path in files}
    if unexpected:
        raise RuntimeError("Coverage included files outside the declared scope: " + str(sorted(unexpected)))
    modules = []
    for path in files:
        item = measured.get(str(path))
        modules.append({"path": str(path.relative_to(source)),
                        "coverage": item["summary"] if item else None})
    if source_digest(source, files) != initial_digest:
        raise RuntimeError("First-party source changed during coverage collection")
    summary = {
        "revision": revision, "source_diff_sha256": source_diff, "owners_sha256": owners,
        "dirty": bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=source, text=True).strip()),
        "first_party_source_sha256": initial_digest, "tools": tools,
        "scope": ["src/*.cpp", "include/JoSIM/*.hpp"],
        "exclusions": ["vendored solver sources", "vendored include/spline.h", "generated code", "test implementation"],
        "limitations": ["LLVM measures compiled functions and branches; declarations, disabled preprocessor paths and uninstantiated templates do not acquire counters.",
                        "Files with no recorded mapping are listed with null coverage; they are not silently treated as covered."],
        "tests": tests, "mapping_diagnostics": mapping["status"],
        "totals": data["totals"], "modules": modules,
        "diagnostics": [item["diagnostics"] for item in commands if (output / item["diagnostics"]).stat().st_size],
    }
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    text = ["# First-party C++ coverage", "", "Source: `" + summary["revision"] + "`.", "",
            "Scope: all `src/*.cpp` and `include/JoSIM/*.hpp`; excluded dependencies, vendored spline, generated files and test implementation.",
            "", "| Module | Lines | Functions | Branches |", "|---|---:|---:|---:|"]
    for item in modules:
        measured_summary = item["coverage"]
        values = [("{:.2f}%".format(measured_summary[key]["percent"]) if measured_summary[key]["count"] else "n/a")
                  for key in ("lines", "functions", "branches")] if measured_summary else ["no mapping"] * 3
        text.append("| " + item["path"] + " | " + " | ".join(values) + " |")
    text += ["", "Overall: " + ", ".join("{} {:.2f}%".format(key, data["totals"][key]["percent"]) for key in ("lines", "functions", "branches")) + ".", "",
             "Collection does not adopt thresholds. Run tools/check_coverage.py against the reviewed policy; see docs/qualification/2026-11-07-coverage.md for risks and limits.",
             "See `coverage.json` for LLVM counters, `summary.json` for inventory/tool identities, and `html/index.html` for annotated source.", ""]
    (output / "README.md").write_text("\n".join(text))
    print(text[-5])
    print("Coverage report:", output / "README.md")
    return test_status or int(tests["status"] != "passed")


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, OSError, ValueError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
