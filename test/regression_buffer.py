"""Stdlib-only native checks against independent #101 reference fixtures."""

import csv
import hashlib
import json
import math
import pathlib
import subprocess
import sys
import tempfile
import unittest


BINARY = pathlib.Path(sys.argv.pop(1)).resolve()
ROOT = pathlib.Path(__file__).resolve().parents[1]
REFERENCES = ROOT / "test/reference"
PHI0 = 6.62607015e-34 / (2 * 1.602176634e-19)
SIGMA = PHI0 / (2 * math.pi)


def text_hash(path):
    return hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()


def input_flux(time_ps):
    area = 0.0
    amplitude, width = .82713e-3, 2.5e-12
    for start in (300., 600.):
        x = (time_ps - start) * 1e-12
        if x >= 2 * width:
            area += amplitude * width
        elif x > width:
            falling = x - width
            area += .5 * amplitude * width + amplitude * falling - .5 * amplitude * falling * falling / width
        elif x > 0:
            area += .5 * amplitude * x * x / width
    return area


class BufferRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads((REFERENCES / "buffer-101-manifest.json").read_text())
        cls.acceptance = cls.manifest["acceptance"]

    def test_reference_provenance(self):
        self.assertEqual(self.manifest["text_hash_policy"], "SHA-256 after CRLF-to-LF normalization")
        self.assertEqual(text_hash(ROOT / "tools/qualify_buffer.py"), self.manifest["reference_owner_sha256"],
                         "Reference owner changed; requalify and regenerate the independent fixtures")
        for name, metadata in self.manifest["fixtures"].items():
            self.assertEqual(text_hash(REFERENCES / name), metadata["sha256"], name)

    def simulate(self, direction, h, mode):
        a = self.acceptance
        scale = (h / a["base_step_ps"]) ** 2
        voltage = a["voltage_base_V"] * scale + a["voltage_rounding_V"]
        current = a["current_base_A"] * scale + a["current_rounding_A"]
        phase = a["phase_base_rad"] * scale + a["phase_rounding_rad"]
        limits = [voltage] * 3 + [phase] * 2 + [current] * 2
        with (REFERENCES / ("buffer-101-" + direction + ".csv")).open() as stream:
            expected = [[float(value) for value in row] for row in list(csv.reader(stream))[1:]]
        deck = (REFERENCES / ("buffer-101-" + direction + ".cir")).read_text()
        self.assertEqual(deck.count(".tran 0.01p 800p"), 1)
        deck = deck.replace(".tran 0.01p 800p", ".tran " + str(h) + "p 800p")
        errors = [0.0] * 7
        areas = [0.0, 0.0]
        seen = 0
        previous = None
        with tempfile.TemporaryDirectory(prefix="buffer-", dir=pathlib.Path.cwd()) as tmp:
            directory = pathlib.Path(tmp)
            (directory / "input.cir").write_text(deck)
            result = subprocess.run([str(BINARY), "-m", "1", "-a", str(mode), "-o", "output.csv", "input.cir"],
                                    cwd=directory, capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            with (directory / "output.csv").open() as stream:
                reader = csv.reader(stream)
                self.assertEqual(len(next(reader)), 12)
                for fields in reader:
                    time_s, output = float(fields[0]), float(fields[2])
                    self.assertTrue(math.isfinite(time_s) and math.isfinite(output))
                    if previous is not None:
                        self.assertGreater(time_s, previous[0])
                        for index, (left, right) in enumerate(((290e-12, 350e-12), (590e-12, 650e-12))):
                            if previous[0] >= left - 1e-22 and time_s <= right + 1e-22:
                                areas[index] += .5 * (previous[1] + output) * (time_s - previous[0])
                    previous = time_s, output
                    if seen == len(expected) or time_s * 1e12 < expected[seen][0] - 1e-7:
                        continue
                    target = expected[seen]
                    self.assertAlmostEqual(time_s * 1e12, target[0], delta=1e-7)
                    row = [float(value) for value in fields]
                    self.assertTrue(all(math.isfinite(value) for value in row))
                    values = row[2:9]
                    for index, (actual, reference, limit) in enumerate(zip(values, target[1:], limits)):
                        error = abs(actual - reference)
                        errors[index] = max(errors[index], error)
                        self.assertLessEqual(error, limit, (direction, mode, h, target[0], index, actual, reference, limit))
                    # KCL at N2/N3, using independent orientation definitions.
                    kcl = max(abs(row[7] - row[9] - row[10]), abs(row[10] - row[8] - row[11]))
                    self.assertLessEqual(kcl, 3e-6 * max(abs(v) for v in row[7:]) + 1e-12)
                    # Quiet endpoints: the prescribed input integral fixes loop flux.
                    if target[0] in (50, 100, 200, 290, 350, 400, 590, 650, 700, 799):
                        linkage = (SIGMA * (row[5] - row[6]) - .125e-12 * row[8] if direction == "reverse" else
                                   SIGMA * row[5] + .125e-12 * row[7])
                        self.assertLess(abs(linkage - input_flux(target[0])) / SIGMA, 1e-5)
                    seen += 1
        self.assertEqual(seen, len(expected))
        for area, reference in zip(areas, self.manifest["pulses"][direction]):
            self.assertAlmostEqual(area / PHI0, reference["net_flux_Phi0"], delta=a["pulse_area_Phi0"])
        return errors

    def test_buffer_waveforms_conservation_and_refinement(self):
        for direction in ("reverse", "forward"):
            for mode in (0, 1):
                previous = None
                for h in (.02, .01, .005):
                    with self.subTest(direction=direction, mode=mode, h=h):
                        errors = self.simulate(direction, h, mode)
                        if previous is not None:
                            self.assertLess(max(errors[:3]), .4 * max(previous[:3]) + 2e-9)
                            self.assertLess(max(errors[5:]), .4 * max(previous[5:]) + 2e-9)
                        previous = errors


if __name__ == "__main__":
    unittest.main()
