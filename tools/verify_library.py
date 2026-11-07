#!/usr/bin/env python3
"""Test JoSIM library builds and applications using a relocated installation."""

import argparse
import datetime
import json
import pathlib
import platform
import subprocess
import sys

from build_release import compiler_metadata, sha256, source_identity
from ctest_results import verify_ctest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-root", type=pathlib.Path, required=True)
    parser.add_argument("--dependencies", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--generator")
    parser.add_argument("--revision")
    parser.add_argument("--parallel", type=int, default=4)
    parser.add_argument("--modes", nargs="+", choices=("default", "shared", "static"), default=("default", "shared"))
    args = parser.parse_args()
    if args.parallel < 1 or len(set(args.modes)) != len(args.modes):
        parser.error("Use positive parallelism and unique modes")
    source = pathlib.Path(__file__).resolve().parent.parent
    root, deps, output = (p.resolve() for p in (args.build_root, args.dependencies, args.output))
    output.mkdir(parents=True, exist_ok=True)
    root.mkdir(parents=True, exist_ok=True)
    report = {"source": None, "status": "running", "owner_sha256": sha256(pathlib.Path(__file__)),
              "host": {"system": platform.system(), "machine": platform.machine()},
              "started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "commands": [], "modes": {}}

    def save():
        (output / "summary.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    def fail(message):
        report["status"] = "failed"
        save()
        raise RuntimeError(message)

    def run(command, log):
        command = [str(value) for value in command]
        with log.open("w", encoding="utf-8") as stream:
            result = subprocess.run(command, cwd=source, stdout=stream, stderr=subprocess.STDOUT)
        report["commands"].append({"command": command, "exit": result.returncode, "log": str(log)})
        save()
        if result.returncode:
            fail("Command failed; see {}\n{}".format(log, log.read_text(errors="replace")[-8000:]))

    save()
    identity = source_identity(source, args.revision)
    report["source"] = identity
    run(["cmake", "-DJOSIM_DEPENDENCY_DIR=" + str(deps), "-P", source / "cmake/prepare_dependencies.cmake"], output / "dependencies.log")
    for mode in args.modes:
        build = root / mode
        configure = ["cmake", "-S", source, "-B", build, "-C", deps / "sources.cmake",
                     "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_TESTING=ON", "-DJOSIM_COVERAGE=OFF", "-DJOSIM_SANITIZERS=OFF",
                     "-DBUILD_SHARED_LIBS=" + ("ON" if mode == "shared" else "OFF"),
                     "-DMAKING_STATIC_BUILD=" + ("ON" if mode == "static" else "OFF")]
        if args.generator:
            configure += ["-G", args.generator]
            if platform.system() == "Windows" and args.generator.startswith("Visual Studio"):
                configure += ["-A", "x64"]
        run(configure, output / (mode + "-configure.log"))
        run(["cmake", "--build", build, "--config", "Release", "--parallel", args.parallel], output / (mode + "-build.log"))
        inventory = output / (mode + "-inventory.json")
        run(["ctest", "--test-dir", build, "-C", "Release", "--show-only=json-v1"], inventory)
        junit = output / (mode + "-tests.xml")
        run(["ctest", "--test-dir", build, "-C", "Release", "--output-on-failure", "--timeout", "120",
             "--no-tests=error", "--output-junit", junit], output / (mode + "-tests.log"))
        try:
            tests = verify_ctest(inventory, junit)
        except RuntimeError as error:
            fail(str(error))
        report["modes"][mode] = {"status": "passed", "tests": tests["entries"], "test_inventory": tests,
                                  "toolchain": compiler_metadata(build),
                                  "installation": "Moved to a path with spaces before fresh consumer discovery; original prefix absent"}
        save()
    if source_identity(source, args.revision) != identity or sha256(pathlib.Path(__file__)) != report["owner_sha256"]:
        fail("The source or test script changed while the library tests were running")
    report["status"] = "passed"
    save()
    print("Library modes passed:", ", ".join(args.modes))


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, ValueError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
