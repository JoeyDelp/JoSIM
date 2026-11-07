#!/usr/bin/env python3
"""Check complete first-party coverage against reviewed risk and regression limits."""

import argparse
import hashlib
import json
import math
import pathlib
import re
import sys

from coverage import first_party_files, source_digest
from ctest_results import REQUIRED_TESTS


def evaluate(summary, policy, source):
    files = first_party_files(source)
    modules = {item["path"]: item["coverage"] for item in summary["modules"]}
    if len(modules) != len(summary["modules"]) or set(modules) != {path.relative_to(source).as_posix() for path in files}:
        raise RuntimeError("Coverage inventory omits or adds first-party files")
    if summary["first_party_source_sha256"] != source_digest(source, files):
        raise RuntimeError("Coverage is stale for the current first-party source")
    if summary["scope"] != policy["scope"] or summary["mapping_diagnostics"] != "validated":
        raise RuntimeError("Coverage scope or LLVM mapping validation is incomplete")
    tests = summary["tests"]
    executed = tests["executed"]
    if (tests["status"] != "passed" or tests["exit"] or len(set(executed)) != len(executed)
            or tests["entries"] != len(executed) or sorted(tests["declared"]) != sorted(executed)
            or not REQUIRED_TESTS.issubset(executed)):
        raise RuntimeError("Every test must run and pass before checking coverage")
    majors = {match.group(1) for tool in summary["tools"].values()
              if (match := re.search(r"(?:clang|LLVM) version (\d+)", "\n".join(tool["version"])))}
    if len(majors) != 1:
        raise RuntimeError("Coverage LLVM tool majors do not match")
    major = majors.pop()
    checks = []
    for metric in ("lines", "functions", "branches"):
        for field in ("count", "covered"):
            if sum(module[metric][field] for module in modules.values() if module) != summary["totals"][metric][field]:
                raise RuntimeError("Coverage totals disagree with the module inventory")

    def check(label, coverage, floors, reference, tolerance):
        if coverage is None:
            raise RuntimeError("Required coverage mapping is missing: " + label)
        for metric in ("lines", "functions", "branches"):
            counter = coverage[metric]
            count, covered = counter["count"], counter["covered"]
            if not isinstance(count, int) or not isinstance(covered, int) or not 0 <= covered <= count:
                raise RuntimeError("Invalid coverage counter: " + label + "/" + metric)
            actual = 100 * covered / count if count else 0
            if not math.isfinite(counter["percent"]) or abs(actual - counter["percent"]) > 1e-8:
                raise RuntimeError("Coverage percent disagrees with counters: " + label + "/" + metric)
            minimum = max(floors.get(metric, 0), reference.get(metric, 0) - tolerance)
            if not count or actual + 1e-9 < minimum:
                raise RuntimeError(f"Coverage regression: {label}/{metric} {actual:.2f}% < {minimum:.2f}%")
            checks.append({"module": label, "metric": metric, "percent": actual, "minimum": minimum})

    reference = policy["references"].get(major)
    # Apply independent risk floors even before a new compiler's reference is reviewed.
    check("overall", summary["totals"], policy["overall_minimum_percent"],
          reference["overall"] if reference else {}, policy["variation_pp"]["overall"])
    for name in policy["critical_modules"]:
        check(name, modules.get(name), policy["critical_minimum_percent"],
              reference["modules"][name] if reference else {}, policy["variation_pp"]["module"])
    if reference is None:
        raise RuntimeError("LLVM " + major + " coverage reference requires review; retain the generated reports")
    return {"status": "passed", "llvm_major": major, "checks": checks}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--summary", type=pathlib.Path, required=True)
    parser.add_argument("--policy", type=pathlib.Path, default=pathlib.Path(__file__).with_name("coverage-policy.json"))
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    summary = json.loads(args.summary.read_text())
    policy = json.loads(args.policy.read_text())
    try:
        result = evaluate(summary, policy, pathlib.Path(__file__).resolve().parents[1])
    except RuntimeError as error:
        result = {"status": "failed", "error": str(error)}
    result.update({"revision": summary["revision"], "policy_sha256": hashlib.sha256(args.policy.read_bytes()).hexdigest()})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(result.get("error", "Coverage policy passed"))
    return int(result["status"] != "passed")


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, OSError, ValueError, KeyError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
