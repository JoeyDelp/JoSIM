"""Optional Python utilities: trace values, safe output and real headless exports."""
import argparse
import hashlib
import importlib.metadata
import importlib.util
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
import xml.etree.ElementTree as ET

import numpy as np
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
SCRIPTS = ROOT / "scripts"
sys.path.insert(0, str(SCRIPTS))
from plot_common import read_trace

parser = argparse.ArgumentParser()
parser.add_argument("--report", type=Path)
options = parser.parse_args()
RECEIPTS = []


def load(name):
    spec = importlib.util.spec_from_file_location(name.replace("-", "_"), SCRIPTS / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


PLOT = load("josim-plot")
PLOT2 = load("josim-plot2")
COMPARE = load("plot-compare")
SPEC = load("sp_generator")
LAYOUTS = ("grid_layout", "square_layout", "stacked_layout", "combined_layout", "seperate_combined_layout")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class PythonUtilities(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="python-utils-", dir=Path.cwd())
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name)
        self.frame = pd.DataFrame({"time": [0., 1., 2.], "V(N)": [0., 2., -1.],
                                   "I(B1)": [0., .1, -.1], "P(B1)": [0., 2 * math.pi, -2 * math.pi]})
        self.csv = self.path / "Input.CSV"
        self.dat = self.path / "Input.DAT"
        self.frame.to_csv(self.csv, index=False)
        self.frame.to_csv(self.dat, index=False, sep=" ")

    def invoke(self, script, *args, code=0):
        command = [sys.executable, "-B", str(SCRIPTS / script), *map(str, args)]
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        RECEIPTS.append({"command": command, "returncode": result.returncode,
                         "stdout": result.stdout, "stderr": result.stderr})
        self.assertEqual(result.returncode, code, result.stdout + result.stderr)
        self.assertNotIn("Traceback", result.stderr)
        return result

    def test_csv_and_dat_preserve_values(self):
        for path in (self.csv, self.dat):
            before = digest(path)
            actual = read_trace(path)
            np.testing.assert_allclose(actual, self.frame, rtol=1e-14, atol=0)
            self.assertEqual(actual.columns.tolist(), self.frame.columns.tolist())
            self.assertEqual(digest(path), before)

    def test_all_layouts_preserve_values_and_subsets(self):
        for layout in LAYOUTS:
            for subset in (None, ["P(B1)", "V(N)"]):
                with self.subTest(layout=layout, subset=subset):
                    fig = getattr(PLOT, layout)(self.frame, subset)
                    names = subset or self.frame.columns[1:].tolist()
                    self.assertEqual({trace.name for trace in fig.data}, set(names))
                    for trace in fig.data:
                        np.testing.assert_array_equal(trace.x, self.frame["time"])
                        np.testing.assert_array_equal(trace.y, self.frame[trace.name])

    def test_phase_units_in_every_layout(self):
        for layout in LAYOUTS:
            for jump, factor in (("rad", 1), ("0.5pi", 2 / math.pi), ("pi", 1 / math.pi), ("2pi", 1 / (2 * math.pi))):
                with self.subTest(layout=layout, jump=jump):
                    fig = getattr(PLOT2, layout)(self.frame, SimpleNamespace(subset=None, jump=jump))
                    for trace in fig.data:
                        expected = self.frame[trace.name] * (factor if trace.name.startswith("P") else 1)
                        np.testing.assert_allclose(trace.y, expected, rtol=1e-14, atol=0)
                    self.assertEqual(PLOT2.y_axis_title("I(B1)", SimpleNamespace(jump=jump)), "Current (A)")

    def test_comparison_uses_independent_time_vectors(self):
        other = pd.DataFrame({"time": [10., 20.], "P(B1)": [3., 4.]})
        fig = COMPARE.stacked_layout(self.frame, other, ["P(B1)"])
        np.testing.assert_array_equal(fig.data[0].x, [0, 1, 2])
        np.testing.assert_array_equal(fig.data[1].x, [10, 20])
        np.testing.assert_array_equal(fig.data[1].y, [3, 4])

    def test_html_from_all_three_clis(self):
        for script, inputs in (("josim-plot.py", [self.dat]), ("josim-plot2.py", [self.csv]),
                               ("plot-compare.py", [self.csv, self.dat])):
            output = self.path / (script + ".html")
            self.invoke(script, *inputs, "-x", output)
            text = output.read_text()
            self.assertIn("Plotly.newPlot", text)
            self.assertIn("P(B1)", text)
            self.assertIn("plotly.js", text)

    def test_real_png_exports_and_dimensions(self):
        for script, inputs, size, flags in (
                ("josim-plot.py", [self.csv], (700, 500), []),
                ("josim-plot2.py", [self.dat], (700, 500), []),
                ("plot-compare.py", [self.csv, self.dat], (600, 400), ["-d", "600x400"])):
            output = self.path / (script + ".PNG")
            self.invoke(script, *inputs, "-o", output, *flags)
            data = output.read_bytes()
            self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")
            self.assertEqual(struct.unpack(">II", data[16:24]), size)
            self.assertGreater(len(data), 2000)
            RECEIPTS.append({"export": output.name, "sha256": digest(output), "dimensions": size, "bytes": len(data)})

    def test_other_advertised_image_formats(self):
        for extension in ("jpeg", "jpg", "webp", "svg", "pdf"):
            output = self.path / ("trace." + extension)
            self.invoke("josim-plot2.py", self.csv, "-o", output, "-d", "600x400", "-t", "combined", "-j", "2pi")
            data = output.read_bytes()
            if extension in ("jpeg", "jpg"):
                self.assertEqual(data[:3], b"\xff\xd8\xff")
                self.assertEqual(data[-2:], b"\xff\xd9")
            elif extension == "webp":
                self.assertEqual(data[:4], b"RIFF")
                self.assertEqual(data[8:12], b"WEBP")
            elif extension == "svg":
                svg = ET.fromstring(data)
                self.assertEqual((svg.attrib["width"], svg.attrib["height"]), ("600", "400"))
            else:
                self.assertTrue(data.startswith(b"%PDF-"))
                self.assertIn(b"%%EOF", data[-100:])
            self.assertGreater(len(data), 1000)
            RECEIPTS.append({"export": output.name, "sha256": digest(output), "bytes": len(data)})

    def test_plot_inputs_and_aliases_cannot_be_overwritten(self):
        alias = self.path / "alias.csv"
        alias.hardlink_to(self.csv)
        before = digest(self.csv), digest(self.dat)
        for script, inputs, destinations in (
                ("josim-plot.py", [self.csv], [self.csv, alias]),
                ("josim-plot2.py", [self.csv], [self.csv, alias]),
                ("plot-compare.py", [self.csv, self.dat], [self.csv, self.dat, alias])):
            for output in destinations:
                self.invoke(script, *inputs, "-x", output, code=2)
                self.assertEqual((digest(self.csv), digest(self.dat)), before)

    def test_invalid_plot_requests_fail_before_output(self):
        output = self.path / "invalid.png"
        for flags in (("-d", "0x2"), ("-d", "bad"), ("-t", "bad"), ("-c", "bad"),
                      ("-s", "not-a-trace"), ("-x", str(self.path / "also.html"))):
            self.invoke("josim-plot.py", self.csv, "-o", output, *flags, code=2)
            self.assertFalse(output.exists())
        self.invoke("josim-plot2.py", self.csv, "-j", "bad", "-o", output, code=2)
        self.invoke("plot-compare.py", self.csv, self.dat, "-o", self.path / "old.eps", code=2)
        self.invoke("plot-compare.py", self.csv, self.dat, "-o", self.path / "bad.xyz", code=2)
        self.assertFalse((self.path / "old.eps").exists())

    def test_nonfinite_and_empty_plot_data_rejected(self):
        for text in ("time,V(N)\n", "time\n0\n", "time,V(N)\n0,nan\n", "time,V(N)\ninf,0\n", "time,V(N)\n0,hello\n"):
            self.csv.write_text(text)
            with self.assertRaises(ValueError):
                read_trace(self.csv)

    def test_nonuniform_spec_missing_20ps_and_phase_reversals(self):
        times = [0, 19, 21, 25, 29, 36, 41, 48, 52, 59, 65, 72, 77, 90]
        cycles = [4, 4, 4, 5, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3]
        frame = pd.DataFrame({"time": np.array(times) * 1e-12, "P(B1)": np.array(cycles) * 2 * math.pi})
        result = SPEC.generate_spec(frame, stability=10e-12)
        np.testing.assert_allclose(result["time"], np.array([21, 36, 59, 90]) * 1e-12, rtol=1e-14, atol=0)
        np.testing.assert_array_equal(result["B1"], [0, 1, 0, -1])
        frame.to_csv(self.csv, index=False)
        before = digest(self.csv)
        output = self.path / "result.sp"
        self.invoke("sp_generator.py", self.csv, "-o", output, "-s", "1e-11", "-v")
        parsed = pd.read_csv(output, sep=r"\s+")
        np.testing.assert_array_equal(parsed["B1"], [0, 1, 0, -1])
        np.testing.assert_allclose(parsed["time"], result["time"], rtol=1e-14, atol=0)
        self.assertEqual(digest(self.csv), before)

    def test_joint_stability_zero_options_and_rounding(self):
        frame = pd.DataFrame({"time": np.array([0, 20, 30, 35, 40, 45, 50, 55]) * 1e-12,
                              "P(A)": np.array([0, 0, 1, 1, 1, 1, 1, 1]) * 2 * math.pi,
                              "P(B)": np.array([0, 0, 0, 0, 1, 1, 1, 1]) * 2 * math.pi})
        result = SPEC.generate_spec(frame, stability=10e-12)
        np.testing.assert_allclose(result["time"], [20e-12, 50e-12], rtol=1e-14, atol=0)
        np.testing.assert_array_equal(result[["A", "B"]], [[0, 0], [1, 1]])
        immediate = SPEC.generate_spec(frame, threshold=0, stability=0, start=0)
        np.testing.assert_allclose(immediate["time"], [0, 30e-12, 40e-12], rtol=1e-14, atol=0)
        for number, threshold, expected in ((.8, .8, 0), (.81, .8, 1), (-.81, .8, -1),
                                             (1.8, .8, 1), (.1, 0, 1), (.99, 1, 0)):
            self.assertEqual(SPEC.cust_round(number, threshold), expected)

    def test_invalid_spec_data_and_options(self):
        valid = pd.DataFrame({"time": [0, 30e-12], "P(B1)": [0, 0]})
        for options in ({"threshold": math.nan}, {"threshold": -1}, {"threshold": 2},
                        {"stability": -1}, {"stability": math.inf}, {"start": -1}, {"start": 1}):
            with self.assertRaises(ValueError):
                SPEC.generate_spec(valid, **options)
        for frame in (self.frame, valid.rename(columns={"time": "t"}), valid.iloc[:0],
                      pd.DataFrame({"time": [0, 0], "P(B1)": [0, 1]}),
                      pd.DataFrame({"time": [1, 0], "P(B1)": [0, 1]}),
                      pd.DataFrame({"time": [0, 30e-12], "P(B1)": [0, math.nan]})):
            with self.assertRaises(ValueError):
                SPEC.generate_spec(frame)
        valid.to_csv(self.csv, index=False)
        before = digest(self.csv)
        self.invoke("sp_generator.py", self.csv, "-o", self.csv, code=2)
        self.assertEqual(digest(self.csv), before)
        self.invoke("sp_generator.py", self.csv, "-s", "-1", "-o", self.path / "bad.sp", code=2)
        self.assertFalse((self.path / "bad.sp").exists())

    def test_retired_preprocessors_change_no_files(self):
        deck = self.path / "test.cir"
        deck.write_text("\nR1 N 0 10k\n.end\n")
        output = self.path / "existing.cir"
        output.write_text("user content\n")
        before = {p.name: digest(p) for p in self.path.iterdir()}
        result = self.invoke("noise_insert.py", deck, "-o", output, code=2)
        self.assertIn(".TEMP", result.stderr)
        self.invoke("ivcurve.py", deck, code=2)
        self.assertEqual({p.name: digest(p) for p in self.path.iterdir()}, before)


if __name__ == "__main__":
    sources = [*SCRIPTS.glob("*.py"), Path(__file__)]
    before = {str(p.relative_to(ROOT)): digest(p) for p in sources}
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(PythonUtilities))
    unchanged = before == {str(p.relative_to(ROOT)): digest(p) for p in sources}
    if options.report:
        options.report.parent.mkdir(parents=True, exist_ok=True)
        options.report.write_text(json.dumps({"passed": result.wasSuccessful() and unchanged,
            "tests_run": result.testsRun, "source_unchanged": unchanged, "source_sha256": before,
            "python": sys.version, "dependencies": {name: importlib.metadata.version(name)
                for name in ("numpy", "pandas", "plotly", "kaleido", "tabulate", "choreographer")},
            "receipts": RECEIPTS}, indent=2) + "\n")
    raise SystemExit(0 if result.wasSuccessful() and unchanged else 1)
