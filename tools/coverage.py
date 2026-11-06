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
import xml.etree.ElementTree as ET


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
    environment = os.environ.copy()
    environment["LLVM_PROFILE_FILE"] = str(profiles / "%m-%p.profraw")
    junit = output / "tests.xml"
    test_status = run(["ctest", "--test-dir", build, "--output-on-failure", "--timeout", "120",
                       "--output-junit", junit], "test.log", env=environment, required=False)
    raw = sorted(profiles.glob("*.profraw"))
    if not raw:
        raise RuntimeError("No execution profiles were produced")
    profile = output / "coverage.profdata"
    run([args.llvm_profdata, "merge", "-sparse", *raw, "-o", profile], "merge.log")
    executable = build / "josim-cli"
    regression = build / "test/josim-noise-regression"
    files = sorted((source / "src").glob("*.cpp")) + sorted((source / "include/JoSIM").glob("*.hpp"))
    options = [executable, "-object=" + str(regression), "-instr-profile=" + str(profile)]
    run([args.llvm_cov, "export", *options, *files], "coverage.json")
    run([args.llvm_cov, "report", *options, *files], "coverage.txt")
    run([args.llvm_cov, "show", *options, "-format=html", "-output-dir=" + str(output / "html"),
         "-show-branches=count", "-show-line-counts-or-regions", *files], "html.log")
    data = json.loads((output / "coverage.json").read_text())["data"][0]
    measured = {str(pathlib.Path(item["filename"]).resolve()): item for item in data["files"]}
    unexpected = set(measured) - {str(path) for path in files}
    if unexpected:
        raise RuntimeError("Coverage included files outside the declared scope: " + str(sorted(unexpected)))
    modules = []
    for path in files:
        item = measured.get(str(path))
        modules.append({"path": str(path.relative_to(source)),
                        "coverage": item["summary"] if item else None})
    digest = hashlib.sha256()
    for path in files:
        digest.update(str(path.relative_to(source)).encode() + b"\0" + path.read_bytes() + b"\0")
    cases = ET.parse(junit).getroot().findall(".//testcase")
    summary = {
        "revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip(),
        "dirty": bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=source, text=True).strip()),
        "first_party_source_sha256": digest.hexdigest(), "tools": tools,
        "scope": ["src/*.cpp", "include/JoSIM/*.hpp"],
        "exclusions": ["vendored solver sources", "vendored include/spline.h", "generated code", "test implementation"],
        "limitations": ["LLVM measures compiled functions and branches; declarations, disabled preprocessor paths and uninstantiated templates do not acquire counters.",
                        "Files with no recorded mapping are listed with null coverage; they are not silently treated as covered."],
        "tests": {"entries": len(cases), "exit": test_status},
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
             "No thresholds are inferred from this result. Assess missing mappings and uncovered high-risk paths before defining regression requirements.",
             "See `coverage.json` for LLVM counters, `summary.json` for inventory/tool identities, and `html/index.html` for annotated source.", ""]
    (output / "README.md").write_text("\n".join(text))
    print(text[-5])
    print("Coverage report:", output / "README.md")
    return test_status


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, OSError, ValueError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
