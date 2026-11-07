"""Safe external sweeps: source preservation, analytic means and fresh circuits."""
import argparse
import hashlib
import importlib.util
import json
import math
import pathlib
import subprocess
import sys
import tempfile
import unittest

parser = argparse.ArgumentParser(add_help=False)
parser.add_argument("binary", type=pathlib.Path)
parser.add_argument("--report", type=pathlib.Path)
options, remaining = parser.parse_known_args()
BINARY = options.binary.resolve()
ROOT = pathlib.Path(__file__).resolve().parents[1]
OWNER = ROOT / "tools/circuit_sweep.py"
spec = importlib.util.spec_from_file_location("circuit_sweep", OWNER)
sweep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sweep)
MEASUREMENTS = []


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class SweepRegression(unittest.TestCase):
    def invoke(self, template, output, values="1,2,3,4,5", trace="V(N)", start="1e-11", stop="1.8e-11", analysis="voltage", timeout="60"):
        return subprocess.run([sys.executable, "-B", str(OWNER), "--binary", str(BINARY),
                               "--template", str(template), "--output", str(output), "--values=" + values,
                               "--trace", trace, "--start", start, "--stop", stop,
                               "--analysis", analysis, "--timeout", timeout],
                              capture_output=True, text=True, timeout=30)

    def test_five_resistances_and_existing_files_preserved(self):
        for mode in ("voltage", "phase"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(prefix="sweep ", dir=pathlib.Path.cwd()) as tmp:
                path = pathlib.Path(tmp)
                template = path / "Input Template.CIRDATA"
                template.write_bytes((ROOT / "test/param/sweep_resistance.cir.in").read_bytes())
                sentinel = path / "Input Template_tmp.cir"
                sentinel.write_text("unrelated existing user file\n")
                before = digest(template), digest(sentinel)
                result = self.invoke(template, path / "output", analysis=mode)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                report = json.loads((path / "output/report.json").read_text())
                self.assertTrue(report["passed"])
                self.assertEqual([p["value"] for p in report["points"]], [1, 2, 3, 4, 5])
                for p in report["points"]:
                    self.assertAlmostEqual(p["mean"], p["value"], delta=1e-9)
                    self.assertEqual(p["effective_seed"], 123)
                    self.assertEqual(p["computed_samples"], 20)
                self.assertEqual(before, (digest(template), digest(sentinel)))
                # An existing results directory must survive another invocation.
                original_report = digest(path / "output/report.json")
                again = self.invoke(template, path / "output")
                self.assertEqual(again.returncode, 2)
                self.assertEqual(original_report, digest(path / "output/report.json"))
                MEASUREMENTS.append({"kind": "five_resistances", "mode": mode, "points": report["points"],
                                     "source_and_existing_files_preserved": True})

    def test_complete_squid_circuit_iv_in_both_modes(self):
        expected = 8 * math.sqrt((200e-6) ** 2 - 2 * (100e-6) ** 2)
        for mode in ("voltage", "phase"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(prefix="circuit-iv-", dir=pathlib.Path.cwd()) as tmp:
                output = pathlib.Path(tmp) / "output"
                result = self.invoke(ROOT / "test/param/sweep_squid_bias.cir.in", output,
                                     values="-0.0002,0,0.0002", start="5e-10", stop="9.9e-10", analysis=mode)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                report = json.loads((output / "report.json").read_text())
                for point, reference in zip(report["points"], (-expected, 0, expected)):
                    # Endpoint phase uncertainty <=Phi0/(490ps)=4.23uV;
                    # timestep/CSV error is small at ~183 points/Josephson period.
                    self.assertAlmostEqual(point["mean"], reference, delta=.01 * abs(reference) + 1e-10)
                MEASUREMENTS.append({"kind": "complete_squid_iv", "mode": mode,
                                     "analytic_voltage_V": [-expected, 0, expected], "points": report["points"]})

    def test_includes_and_custom_files_resolve_from_template(self):
        with tempfile.TemporaryDirectory(prefix="sweep paths ", dir=pathlib.Path.cwd()) as tmp:
            root = pathlib.Path(tmp)
            (root / "parts").mkdir()
            (root / "parts/r.inc").write_text("R1 N 0 RX\n.end\n")
            (root / "Wave.Data").write_text("1 1 1 1 1\n")
            template = root / "input.cir"
            template.write_text(".param RX=@VALUE@\n.include parts/r.inc\nV1 N 0 CUS(\n+ Wave.Data 1p 1 0 0 0)\n"
                                ".option seed=321\n.tran .25p 4p DST\n.print I(R1)\n.end\n")
            files = (template, root / "parts/r.inc", root / "Wave.Data")
            before = [digest(p) for p in files]
            result = self.invoke(template, root / "output", values="1,2,5", trace="I(R1)", start="1e-12", stop="3e-12")
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            report = json.loads((root / "output/report.json").read_text())
            self.assertEqual(report["input_sha256"][str((root / "Wave.Data").resolve())], digest(root / "Wave.Data"))
            for point in report["points"]:
                self.assertAlmostEqual(point["mean"], 1 / point["value"], delta=1e-9)
                self.assertEqual(point["effective_seed"], 321)
            self.assertEqual(before, [digest(p) for p in files])

    def test_output_controls_rejected_before_any_run(self):
        for directive in (".file victim.csv", "FILE victim.csv", ".iv JM 100u victim.csv", ".F\rILE victim.csv"):
            with self.subTest(directive=directive), tempfile.TemporaryDirectory(prefix="sweep-reject-", dir=pathlib.Path.cwd()) as tmp:
                root = pathlib.Path(tmp)
                (root / "nested.inc").write_bytes((directive + "\nR1 N 0 1\n").encode())
                template = root / "input.cir"
                template.write_text(".param RX=@VALUE@\n.include nested.inc\nI1 0 N 1\n.tran 1p 20p\n.end\n")
                result = self.invoke(template, root / "output")
                self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                self.assertIn("remove", result.stderr)
                self.assertFalse((root / "output").exists())
                self.assertFalse((root / "victim.csv").exists())

    def test_simulator_failure_does_not_reuse_data(self):
        with tempfile.TemporaryDirectory(prefix="sweep-failure-", dir=pathlib.Path.cwd()) as tmp:
            root = pathlib.Path(tmp)
            template = root / "input.cir"
            template.write_text(".param RX=@VALUE@\nR1 N 0 DOES_NOT_EXIST\n.tran 1p 20p\n.print V(N)\n.end\n")
            result = self.invoke(template, root / "output")
            self.assertEqual(result.returncode, 1)
            report = json.loads((root / "output/report.json").read_text())
            self.assertFalse(report["passed"])
            self.assertEqual(report["points"], [])
            self.assertIn("failed with exit", report["error"])
            self.assertFalse((root / "output/point-0001").exists())
            self.assertFalse((root / "output/sweep.csv").exists())

    def test_retired_iv_script_cannot_modify_files(self):
        with tempfile.TemporaryDirectory(prefix="retired-iv-", dir=pathlib.Path.cwd()) as tmp:
            root = pathlib.Path(tmp)
            source, existing = root / "input.cir", root / "input_tmp.cir"
            source.write_text("source sentinel\n")
            existing.write_text("existing sentinel\n")
            before = digest(source), digest(existing)
            result = subprocess.run([sys.executable, "-B", str(ROOT / "scripts/ivcurve.py"), str(BINARY), str(source)],
                                    cwd=root, capture_output=True, text=True, timeout=5)
            self.assertEqual(result.returncode, 2)
            self.assertIn("retired", result.stderr)
            self.assertEqual(before, (digest(source), digest(existing)))
            self.assertEqual(len(list(root.iterdir())), 2)

    def test_timeout_preserves_failure_witness(self):
        with tempfile.TemporaryDirectory(prefix="sweep-timeout-", dir=pathlib.Path.cwd()) as tmp:
            output = pathlib.Path(tmp) / "output"
            result = self.invoke(ROOT / "test/param/sweep_resistance.cir.in", output, timeout="1e-9")
            self.assertEqual(result.returncode, 1)
            report = json.loads((output / "report.json").read_text())
            self.assertFalse(report["passed"])
            self.assertIn("timed out", report["error"])
            self.assertIn("timed out", (output / "point-0000/simulator.log").read_text())
            self.assertTrue((output / "point-0000/effective.cir").exists())
            self.assertFalse((output / "sweep.csv").exists())

    def test_time_weighted_mean_and_invalid_samples(self):
        # y=2*t on an intentionally nonuniform grid. Exact mean on [.5,3] is3.5.
        self.assertAlmostEqual(sweep.time_average([0, 1, 4], [0, 2, 8], .5, 3), 3.5, delta=1e-14)
        for times, data, start, stop in (([0, 0], [1, 2], 0, 1), ([0, 1], [1, math.nan], 0, 1),
                                          ([0, 1], [1, 2], 0, 2), ([0, 1], [1, 2], .5, .5)):
            with self.subTest(times=times, start=start, stop=stop), self.assertRaises(ValueError):
                sweep.time_average(times, data, start, stop)

    def test_invalid_requests_and_recursive_includes_fail_cleanly(self):
        with tempfile.TemporaryDirectory(prefix="sweep-invalid-", dir=pathlib.Path.cwd()) as tmp:
            root = pathlib.Path(tmp)
            template = root / "input.cir"
            template.write_bytes((ROOT / "test/param/sweep_resistance.cir.in").read_bytes())
            for index, settings in enumerate(({"values": "nan"}, {"values": "inf"},
                                               {"start": "2", "stop": "1"}, {"timeout": "0"})):
                output = root / f"bad-{index}"
                result = self.invoke(template, output, **settings)
                self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                self.assertFalse(output.exists())
            template.write_text(".param RX=@VALUE@\n.include loop.inc\n.end\n")
            (root / "loop.inc").write_text(".include input.cir\n")
            result = self.invoke(template, root / "cycle")
            self.assertEqual(result.returncode, 2)
            self.assertIn("Cyclic include", result.stderr)
            self.assertFalse((root / "cycle").exists())


if __name__ == "__main__":
    result = unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(SweepRegression))
    if options.report:
        options.report.parent.mkdir(parents=True, exist_ok=True)
        options.report.write_text(json.dumps({"passed": result.wasSuccessful(), "binary_sha256": digest(BINARY),
                                             "owner_sha256": digest(OWNER), "test_sha256": digest(pathlib.Path(__file__)),
                                             "cases": MEASUREMENTS}, indent=2) + "\n")
    raise SystemExit(0 if result.wasSuccessful() else 1)
