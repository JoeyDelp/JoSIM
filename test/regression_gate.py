"""Numerical checks for gate-controlled junctions and model IV sweeps."""

import csv
import math
import pathlib
import subprocess
import sys
import tempfile
import unittest


BINARY = pathlib.Path(sys.argv.pop(1)).resolve()


class GateRegression(unittest.TestCase):
    def simulate(self, deck, mode=0, success=True):
        with tempfile.TemporaryDirectory(prefix="gate-", dir=pathlib.Path.cwd()) as tmp:
            directory = pathlib.Path(tmp)
            (directory / "input.cir").write_text(deck)
            result = subprocess.run(
                [str(BINARY), "-m", "1", "-a", str(mode), "-o", "output.csv", "input.cir"],
                cwd=directory, capture_output=True, text=True, timeout=30,
            )
            if not success:
                self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn("E:", result.stdout + result.stderr)
                return
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            with (directory / "output.csv").open() as stream:
                rows = list(csv.reader(stream))
            traces = [[float(value) for value in row] for row in rows[1:]]
            iv = None
            if (directory / "IV.CSV").exists():
                with (directory / "IV.CSV").open() as stream:
                    iv = [[float(v) for v in row] for row in list(csv.reader(stream))[1:]]
            return traces, iv

    def deck(self, gate="5m", junction="B1 n 0 jm GC=g", width="10m", temperature=""):
        return (
            f"P1 n 0 DC 1.5707963267948966\nVG g 0 DC {gate}\n{junction}\n"
            f".model jm jj(rtype=0, ic=100u, rn=16, r0=160, cap=0.07p, gwidth={width}{temperature})\n"
            ".tran 0.1p 20p\n.print I(B1) V(g)\n.end\n"
        )

    def test_gate_current_and_modes(self):
        for mode in (0, 1):
            for gate, expected in (("0", 100e-6), ("5m", 50e-6), ("-5m", 50e-6), ("20m", 0.0)):
                with self.subTest(mode=mode, gate=gate):
                    traces, _ = self.simulate(self.deck(gate=gate), mode)
                    for row in traces[-10:]:
                        self.assertAlmostEqual(row[1], expected, delta=1e-8)

    def test_historical_positional_gate(self):
        for mode in (0, 1):
            traces, _ = self.simulate(self.deck(junction="B1 n 0 g jm"), mode)
            self.assertAlmostEqual(traces[-1][1], 50e-6, delta=1e-8)

    def test_legacy_phase_node_is_not_a_gate(self):
        deck = self.deck(junction="B1 n 0 g jm").replace(", gwidth=10m", "")
        for mode in (0, 1):
            traces, _ = self.simulate(deck, mode)
            self.assertAlmostEqual(traces[-1][1], 100e-6, delta=1e-8)

    def test_temperature_dependent_gate(self):
        for mode in (0, 1):
            traces, _ = self.simulate(self.deck(temperature=", t=4.2, tc=9.1, d=0"), mode)
            self.assertAlmostEqual(traces[-1][1], 50e-6, delta=1e-8)

    def test_time_dependent_gate(self):
        deck = self.deck().replace("DC 5m", "SIN(5m 3m 1G)").replace("0.1p 20p", "0.5p 1n")
        for mode in (0, 1):
            traces, _ = self.simulate(deck, mode)
            for _, current, voltage in traces[100:]:
                expected = 100e-6 * max(0.0, 1.0 - abs(voltage) / 0.01)
                self.assertAlmostEqual(current, expected, delta=2e-7)

    def test_subcircuit_gate_mapping(self):
        for mode in (0, 1):
            for declaration in ("B1 p 0 jm GC=control", "B1 p 0 control jm"):
                deck = self.deck(junction="X1 cell n g")
                deck = deck.replace(".print I(B1)", ".print I(B1.X1)")
                deck = deck.replace(".end\n", f".subckt cell p control\n{declaration}\n.ends\n.end\n")
                traces, _ = self.simulate(deck, mode)
                self.assertAlmostEqual(traces[-1][1], 50e-6, delta=1e-8)

    def test_invalid_gate(self):
        for junction in ("B1 n 0 jm GC=missing", "B1 n 0 jm GC=g GC=g"):
            self.simulate(self.deck(junction=junction), success=False)
        self.simulate(self.deck(width="0"), success=False)

    def test_model_assignment_whitespace(self):
        deck = self.deck().replace("ic=100u", "ic = 100u").replace("cap=0.07p", "cap= 0.07p")
        traces, _ = self.simulate(deck)
        self.assertAlmostEqual(traces[-1][1], 50e-6, delta=1e-8)

    def test_iv_step_and_endpoints(self):
        for option in ("STEP=3u", "ISTEP=3u"):
            deck = self.deck().replace(".end\n", f".iv jm 5u iv.csv {option} GTJJ=5m\n.end\n")
            _, iv = self.simulate(deck)
            expected = [3e-6, 5e-6, 2e-6, 0, -3e-6, -5e-6, -2e-6, 0]
            self.assertEqual(len(iv), len(expected))
            for row, current in zip(iv, expected):
                self.assertTrue(all(math.isfinite(v) for v in row))
                self.assertAlmostEqual(row[1], current, delta=1e-11)

    def test_invalid_iv(self):
        for maximum, option in (("5u", "STEP=0"), ("5u", "UNKNOWN=1"), ("-5u", "STEP=1u")):
            deck = self.deck().replace(".end\n", f".iv jm {maximum} iv.csv {option}\n.end\n")
            self.simulate(deck, success=False)


if __name__ == "__main__":
    unittest.main()
