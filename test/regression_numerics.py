"""Independent circuit references, conservation and mesh refinement."""

import argparse
import cmath
import csv
import hashlib
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
MEASUREMENTS = []


def analytic(kind, t):
    """Zero-state response to vin=a*t; SI units, independent of JoSIM constants."""
    a, resistance, inductance, capacitance = 1e7, 10.0, 1e-10, 1e-12
    if kind == "RC":
        tau = resistance * capacitance
        voltage = a * (t - tau + tau * math.exp(-t / tau))
        current = a * capacitance * (1 - math.exp(-t / tau))
    elif kind == "RL":
        tau = inductance / resistance
        voltage = a * tau * (1 - math.exp(-t / tau))
        current = a / resistance * (t - tau + tau * math.exp(-t / tau))
    else:
        # LC*v'' + RC*v' + v = a*t, v(0)=v'(0)=0.
        discriminant = cmath.sqrt((resistance / inductance) ** 2 - 4 / (inductance * capacitance))
        r1 = (-resistance / inductance + discriminant) / 2
        r2 = (-resistance / inductance - discriminant) / 2
        c1 = (-a - r2 * a * resistance * capacitance) / (r1 - r2)
        c2 = a * resistance * capacitance - c1
        voltage = (a * (t - resistance * capacitance) + c1 * cmath.exp(r1 * t) + c2 * cmath.exp(r2 * t)).real
        current = capacitance * (a + r1 * c1 * cmath.exp(r1 * t) + r2 * c2 * cmath.exp(r2 * t)).real
    return voltage, current


class NumericalRegression(unittest.TestCase):
    def simulate(self, kind, mode, h, startup):
        parts = "R1 IN OUT 10\n"
        if kind == "RC":
            parts += "C1 OUT 0 1p\n"
        elif kind == "RL":
            parts += "L1 OUT 0 100p\n"
        else:
            parts += "L1 OUT CAP 100p\nC1 CAP 0 1p\n"
        terminal = "CAP" if kind == "RLC" else "OUT"
        device = "L1" if kind == "RL" else "C1"
        deck = (f"V1 IN 0 PWL(0 0 200p 2m)\n{parts}"
                f".tran {h}p 120p{' DST' if not startup else ''}\n"
                f".print V(IN) V({terminal}) V(R1) I(R1) I({device}) I(V1) V({device})"
                + (" V(L1)" if kind == "RLC" else "") + "\n.end\n")
        with tempfile.TemporaryDirectory(prefix="numeric-", dir=pathlib.Path.cwd()) as tmp:
            directory = pathlib.Path(tmp)
            (directory / "input.cir").write_text(deck)
            result = subprocess.run([str(BINARY), "-m", "1", "-a", str(mode), "-o", "output.csv", "input.cir"],
                                    cwd=directory, capture_output=True, text=True, timeout=15)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            with (directory / "output.csv").open() as stream:
                rows = [[float(value) for value in row] for row in list(csv.reader(stream))[1:]]
        self.assertGreater(len(rows), 100)
        self.assertEqual(rows[0][0], 0.0)
        self.assertTrue(all(math.isfinite(v) for row in rows for v in row))
        error_v = error_i = kcl = kvl = 0.0
        for row in rows:
            t, vin, voltage, vr, ir, current, source, device_v = row[:8]
            expected_v, expected_i = analytic(kind, t)
            error_v = max(error_v, abs(voltage - expected_v))
            error_i = max(error_i, abs(current - expected_i))
            kcl = max(kcl, abs(ir - current), abs(ir + source))
            residual = vin - vr - device_v - (row[8] if kind == "RLC" else 0)
            kvl = max(kvl, abs(residual))
            self.assertAlmostEqual(vin, 1e7 * t, delta=2e-9)
            self.assertAlmostEqual(vr, 10 * ir, delta=3e-6 * max(abs(vr), abs(10 * ir)) + 2e-15)
            self.assertLessEqual(max(abs(ir - current), abs(ir + source)), 3e-6 * abs(ir) + 1e-20)
            self.assertLessEqual(abs(residual), 3e-6 * max(abs(vin), abs(vr), abs(device_v)) + 2e-15)
        # h^2 bounds use the physical 10 ps circuit timescale. A separate
        # refinement assertion prevents a merely bounded, nonconvergent result.
        scale = 1e7 * (h * 1e-12) ** 2 / 1e-11
        self.assertLess(error_v, scale + 2e-9)
        self.assertLess(error_i, scale / 10 + 2e-10)
        MEASUREMENTS.append({"kind": kind, "mode": mode, "h_ps": h, "startup": startup,
                             "samples": len(rows), "maximum_voltage_error_V": error_v,
                             "maximum_current_error_A": error_i, "maximum_KCL_residual_A": kcl,
                             "maximum_KVL_residual_V": kvl, "deck": deck})
        return rows, error_v, error_i

    def test_biased_junction_inductor_equilibrium(self):
        # KCL: J2=s, J1=-s, IL2=s-Igate. Zero applied flux requires
        # sigma*(asin(-s/Ic1)-asin(s/Ic2)) = L2*(s-Igate).
        sigma = 6.62607015e-34 / (4 * math.pi * 1.602176634e-19)
        low, high = 0.0, 20e-6
        for _ in range(80):
            current = (low + high) / 2
            residual = (math.asin(-current / 2e-3) - math.asin(current / 1.5e-3)
                        - 0.125e-12 * (current - 20e-6) / sigma)
            if residual > 0:
                low = current
            else:
                high = current
        expected_phase1 = math.asin(-current / 2e-3)
        expected_phase2 = math.asin(current / 1.5e-3)
        expected_inductor = current - 20e-6
        for mode in (0, 1):
            for startup in (False, True):
                for h in (0.02, 0.01):
                    with self.subTest(mode=mode, startup=startup, h=h):
                        deck = ("L1 N1 N2 0.125p\nB01 N2 0 JM1\nB02 N2 N3 JM2\n"
                                ".model JM1 JJ(RTYPE=1 VG=2.8m CAP=0.007p R0=160m RN=16m ICRIT=2m)\n"
                                ".model JM2 JJ(RTYPE=1 VG=2.8m CAP=0.007p R0=160m RN=16m ICRIT=1.5m)\n"
                                "L2 N3 N4 0.125p\nRout N1 0 2\nIgate N3 0 20u\nVIN N4 0 DC 0\n"
                                f".tran {h}p 100p{' DST' if not startup else ''}\n"
                                ".print P(B01) P(B02) I(L2) V(Rout) I(B01) I(B02)\n.end\n")
                        with tempfile.TemporaryDirectory(prefix="jj-startup-", dir=pathlib.Path.cwd()) as tmp:
                            directory = pathlib.Path(tmp)
                            (directory / "input.cir").write_text(deck)
                            result = subprocess.run([str(BINARY), "-m", "1", "-a", str(mode), "-o", "output.csv", "input.cir"],
                                                    cwd=directory, capture_output=True, text=True, timeout=15)
                            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                            with (directory / "output.csv").open() as stream:
                                rows = [[float(v) for v in row] for row in list(csv.reader(stream))[1:]]
                        self.assertGreater(len(rows), 100)
                        for row in rows[-10:]:
                            _, phase1, phase2, inductor, output, j1, j2 = row
                            self.assertTrue(all(math.isfinite(v) for v in row))
                            self.assertAlmostEqual(phase1, expected_phase1, delta=1e-8)
                            self.assertAlmostEqual(phase2, expected_phase2, delta=1e-8)
                            self.assertAlmostEqual(inductor, expected_inductor, delta=2e-11)
                            self.assertAlmostEqual(j1, -current, delta=2e-11)
                            self.assertAlmostEqual(j2, current, delta=2e-11)
                            self.assertLess(abs(output), 1e-10)
                            self.assertLess(abs(phase1 - phase2 - .125e-12 * inductor / sigma), 1e-8)
                        MEASUREMENTS.append({"kind": "JJ_L_DC", "mode": mode, "startup": startup,
                                             "h_ps": h, "deck": deck, "last_sample": rows[-1],
                                             "expected_phase1_rad": expected_phase1, "expected_phase2_rad": expected_phase2,
                                             "expected_inductor_A": expected_inductor})

    def test_analytic_responses_conservation_and_convergence(self):
        for kind in ("RC", "RL", "RLC"):
            for startup in (False, True):
                previous = None
                for h in (1.0, 0.5, 0.25):
                    modes = []
                    for mode in (0, 1):
                        with self.subTest(kind=kind, startup=startup, h=h, mode=mode):
                            modes.append(self.simulate(kind, mode, h, startup))
                    self.assertEqual(len(modes[0][0]), len(modes[1][0]))
                    for left, right in zip(modes[0][0], modes[1][0]):
                        for a, b in zip(left, right):
                            self.assertAlmostEqual(a, b, delta=3e-6 * max(abs(a), abs(b)) + 1e-18)
                    if previous is not None:
                        for mode in (0, 1):
                            for quantity in (1, 2):
                                self.assertLess(modes[mode][quantity], 0.4 * previous[mode][quantity])
                    previous = modes


if __name__ == "__main__":
    result = unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(NumericalRegression))
    if options.report:
        options.report.parent.mkdir(parents=True, exist_ok=True)
        options.report.write_text(json.dumps({
            "passed": result.wasSuccessful(), "binary_sha256": hashlib.sha256(BINARY.read_bytes()).hexdigest(),
            "test_sha256": hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest(),
            "reference": "Closed-form R/L/C ramps and KCL/zero-flux JJ/L equilibrium; SI units",
            "scope": "Linear dynamics and static JJ/L equilibrium; switching and long-run studies have separate owners",
            "cases": MEASUREMENTS,
        }, indent=2) + "\n")
    sys.exit(not result.wasSuccessful())
