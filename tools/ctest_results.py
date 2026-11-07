"""Check that every JoSIM CTest test ran and passed."""

import json
import xml.etree.ElementTree as ET


REQUIRED_TESTS = frozenset({
    "regression::sweep", "regression::buffer", "regression::junction_numerics",
    "regression::numerics", "regression::noise", "regression::gate",
    "regression::cli", "regression::functions", "regression::build_options",
    "regression::library", "regression::installed_package",
    "regression::qualification",
})


def verify_ctest(inventory, junit):
    declared = [test["name"] for test in json.loads(inventory.read_text())["tests"]]
    cases = list(ET.parse(junit).getroot().iter("testcase"))
    executed = [case.attrib["name"] for case in cases]
    if not declared or len(set(declared)) != len(declared):
        raise RuntimeError("CTest inventory is empty or contains duplicate names")
    if len(set(executed)) != len(executed) or set(executed) != set(declared):
        raise RuntimeError("Declared and executed CTest results differ or contain duplicates")
    missing = REQUIRED_TESTS - set(executed)
    if missing:
        raise RuntimeError("Required tests are missing: " + ", ".join(sorted(missing)))
    if any(case.find(tag) is not None for case in cases for tag in ("skipped", "failure", "error")):
        raise RuntimeError("A test failed or was skipped")
    return {"status": "passed", "entries": len(cases), "declared": sorted(declared),
            "executed": sorted(executed), "required": sorted(REQUIRED_TESTS)}
