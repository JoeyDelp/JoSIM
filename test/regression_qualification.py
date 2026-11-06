"""Prevent false qualification from missing, duplicated or skipped test results."""

import json
import copy
import pathlib
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from ctest_results import REQUIRED_TESTS, verify_ctest
from coverage import first_party_files, source_digest, validate_mapping
from check_coverage import evaluate


class QualificationRegression(unittest.TestCase):
    def verify(self, declared, executed, status=None):
        with tempfile.TemporaryDirectory(prefix="qualification-") as temp:
            root = pathlib.Path(temp)
            inventory = root / "inventory.json"
            inventory.write_text(json.dumps({"tests": [{"name": n} for n in declared]}))
            suite = ET.Element("testsuite")
            for name in executed:
                case = ET.SubElement(suite, "testcase", name=name)
                if status and name == executed[0]:
                    ET.SubElement(case, status)
            junit = root / "tests.xml"
            ET.ElementTree(suite).write(junit)
            return verify_ctest(inventory, junit)

    def test_complete_inventory_includes_additional_tests(self):
        names = sorted(REQUIRED_TESTS) + ["integration::extra"]
        self.assertEqual(self.verify(names, names)["entries"], len(names))

    def test_missing_execution_is_rejected(self):
        names = sorted(REQUIRED_TESTS)
        with self.assertRaisesRegex(RuntimeError, "differ"):
            self.verify(names, names[:-1])

    def test_required_group_cannot_disappear_from_both(self):
        names = sorted(REQUIRED_TESTS)[:-1]
        with self.assertRaisesRegex(RuntimeError, "Required"):
            self.verify(names, names)

    def test_empty_inventory_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "empty"):
            self.verify([], [])

    def test_duplicates_are_rejected(self):
        names = sorted(REQUIRED_TESTS)
        for declared, executed in [(names + names[:1], names), (names, names + names[:1])]:
            with self.subTest(declared=len(declared), executed=len(executed)):
                with self.assertRaisesRegex(RuntimeError, "duplicate"):
                    self.verify(declared, executed)

    def test_failed_or_skipped_cases_are_rejected(self):
        names = sorted(REQUIRED_TESTS)
        for status in ("skipped", "failure", "error"):
            with self.subTest(status=status):
                with self.assertRaisesRegex(RuntimeError, "failed or was skipped"):
                    self.verify(names, names, status)

    def test_unused_mapping_requires_exact_live_count(self):
        diagnostic = "warning: 1 functions have mismatched data\nhash-mismatch: No profile record found for 'f' with hash = 0x0\n"
        profile = "Counters:\n  f:\n    Hash: 0x18\n    Function count: 7\n"
        data = {"functions": [{"name": "f", "count": 7}]}
        self.assertEqual(validate_mapping(data, diagnostic, profile)["status"], "validated")
        for bad_data, bad_diagnostic, bad_profile in [
                ({"functions": [{"name": "f", "count": 6}]}, diagnostic, profile),
                (data, diagnostic.replace("hash = 0x0", "hash = 0x19"), profile),
                (data, diagnostic, ""),
                (data, diagnostic + "warning: stale profile\n", profile)]:
            with self.assertRaises(RuntimeError):
                validate_mapping(bad_data, bad_diagnostic, bad_profile)

    def test_coverage_policy_rejects_stale_incomplete_and_regressed_data(self):
        with tempfile.TemporaryDirectory(prefix="coverage-policy-") as temp:
            source = pathlib.Path(temp)
            (source / "src").mkdir()
            (source / "src/Core.cpp").write_text("int f() { return 1; }\n")
            names = sorted(REQUIRED_TESTS)
            metrics = {key: {"count": 100, "covered": 100, "percent": 100.0}
                       for key in ("lines", "functions", "branches")}
            summary = {"modules": [{"path": "src/Core.cpp", "coverage": copy.deepcopy(metrics)}],
                "totals": metrics, "first_party_source_sha256": source_digest(source, first_party_files(source)),
                "scope": ["src/*.cpp", "include/JoSIM/*.hpp"], "mapping_diagnostics": "validated",
                "tests": {"status": "passed", "exit": 0, "entries": len(names), "declared": names, "executed": names},
                "tools": {"clang++": {"version": ["clang version 23.1.4"]}}}
            reference = {key: 100 for key in metrics}
            policy = {"scope": summary["scope"], "overall_minimum_percent": {"lines": 75, "functions": 80, "branches": 60},
                "critical_modules": ["src/Core.cpp"], "critical_minimum_percent": {"lines": 80, "branches": 65},
                "variation_pp": {"overall": .5, "module": 2},
                "references": {"23": {"overall": reference, "modules": {"src/Core.cpp": reference}}}}
            self.assertEqual(evaluate(summary, policy, source)["status"], "passed")
            for corruption in ("stale", "missing", "tests", "mapping", "regression", "unknown compiler"):
                damaged = copy.deepcopy(summary)
                if corruption == "stale":
                    damaged["first_party_source_sha256"] = "0" * 64
                elif corruption == "missing":
                    damaged["modules"] = []
                elif corruption == "tests":
                    damaged["tests"]["status"] = "failed"
                elif corruption == "mapping":
                    damaged["mapping_diagnostics"] = "unreviewed"
                elif corruption == "regression":
                    for counter in [damaged["totals"]["lines"], damaged["modules"][0]["coverage"]["lines"]]:
                        counter.update(covered=90, percent=90.0)
                else:
                    damaged["tools"]["clang++"]["version"] = ["clang version 18.1.8"]
                with self.subTest(corruption=corruption), self.assertRaises(RuntimeError):
                    evaluate(damaged, policy, source)


if __name__ == "__main__":
    unittest.main()
