"""Numerical checks for gate-controlled junctions and model IV sweeps."""

import csv
import math
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest


BINARY = pathlib.Path(sys.argv.pop(1)).resolve()


class GateRegression(unittest.TestCase):
    def simulate(self, deck, mode=0, success=True, iv_filename="iv.csv", details=False):
        with tempfile.TemporaryDirectory(prefix="gate-", dir=pathlib.Path.cwd()) as tmp:
            directory = pathlib.Path(tmp)
            (directory / "input.cir").write_text(deck)
            result = subprocess.run(
                [str(BINARY), "-m", "1", "-a", str(mode), "-o", "output.csv",
                 *(["-V", "1"] if details else []), "input.cir"],
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
            if (directory / iv_filename).exists():
                with (directory / iv_filename).open() as stream:
                    iv = [[float(v) for v in row] for row in list(csv.reader(stream))[1:]]
            return (traces, iv, result.stdout) if details else (traces, iv)

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

    def test_positional_gate_uses_the_scoped_model(self):
        deck = ("P1 A 0 DC 1.5707963267948966\nP2 B 0 DC 1.5707963267948966\n"
                "VG G 0 DC 5m\nX1 GATED A G\nX2 PLAIN B G\n"
                ".model JM JJ(RTYPE=0 IC=900u RN=10 CAP=0.1p)\n"
                ".subckt GATED IN CTRL WIDTH=10m\n"
                ".model JM JJ(RTYPE=0 IC=100u RN=10 CAP=0.1p GWIDTH=WIDTH)\n"
                "B1 IN 0 CTRL JM\n.ends\n"
                ".subckt PLAIN IN CTRL\n.model JM JJ(RTYPE=0 IC=100u RN=10 CAP=0.1p)\n"
                "B1 IN 0 CTRL JM\n.ends\n"
                ".tran 0.1p 20p\n.print I(B1.X1) I(B1.X2)\n.end\n")
        for mode in (0, 1):
            traces, _ = self.simulate(deck, mode)
            for row in traces[-10:]:
                self.assertAlmostEqual(row[1], 50e-6, delta=1e-10)
                self.assertAlmostEqual(row[2], 100e-6, delta=2e-10)

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
            for filename in ("iv.csv", "MixedCase.CSV"):
                with self.subTest(option=option, filename=filename):
                    deck = self.deck().replace(".end\n", f".iv jm 5u {filename} {option} GTJJ=5m\n.end\n")
                    _, iv = self.simulate(deck, iv_filename=filename)
                    expected = [3e-6, 5e-6, 2e-6, 0, -3e-6, -5e-6, -2e-6, 0]
                    self.assertIsNotNone(iv)
                    self.assertEqual(len(iv), len(expected))
                    for row, current in zip(iv, expected):
                        self.assertTrue(all(math.isfinite(v) for v in row))
                        self.assertAlmostEqual(row[1], current, delta=1e-11)

    def test_local_model_iv_uses_definition_defaults(self):
        deck = ("P1 N 0 DC 1.5707963267948966\nX1 CELL N CRIT=100u\n"
                ".subckt CELL IN CRIT=300u\n"
                ".model JM JJ(RTYPE=0 IC=CRIT RN=10 CAP=0.1p)\nB1 IN 0 JM\n.ends\n"
                ".tran 0.1p 20p\n.print I(B1.X1)\n.iv JM.CELL 350u iv.csv STEP=175u\n.end\n")
        traces, iv = self.simulate(deck)
        self.assertAlmostEqual(traces[-1][1], 100e-6, delta=1e-10)
        reference = deck.replace(".iv JM.CELL", ".iv DEFAULT").replace(
            ".end\n", ".model DEFAULT JJ(RTYPE=0 IC=300u RN=10 CAP=0.1p)\n.end\n")
        _, reference_iv = self.simulate(reference)
        self.assertEqual(iv, reference_iv)

    def test_invalid_iv(self):
        for maximum, option in (("5u", "STEP=0"), ("5u", "UNKNOWN=1"), ("-5u", "STEP=1u")):
            deck = self.deck().replace(".end\n", f".iv jm {maximum} iv.csv {option}\n.end\n")
            self.simulate(deck, success=False)

    def test_ohmic_iv_preserves_bias_through_timestep_retry(self):
        # Ic=C=0 gives the independent constitutive law V=RN*I. At 100mV,
        # h=.05ps advances ~15.2rad, forcing several .2-cycle guard retries.
        for gate in (False, True):
            model_gate = " GWIDTH=10m" if gate else ""
            option = " GTJJ=5m" if gate else ""
            deck = ("RMAIN N 0 1\n"
                    f".model M JJ(RTYPE=0 ICrit=0 RN=100 CAP=0{model_gate})\n"
                    f".iv M 1m iv.csv STEP=1m{option}\n"
                    ".tran 1p 5p DST\n.print V(N)\n.end\n")
            for mode in (0, 1):
                with self.subTest(gate=gate, caller_mode=mode):
                    _, rows = self.simulate(deck, mode)
                    self.assertEqual(len(rows), 4)
                    for (voltage, current), expected in zip(rows, (1e-3, 0, -1e-3, 0)):
                        self.assertAlmostEqual(current, expected, delta=1e-12)
                        # CSV has six digits; the exact Ohmic steady state is
                        # reached long before the latter-half averaging window.
                        self.assertAlmostEqual(voltage, 100 * expected, delta=1e-8)

    def test_model_iv_points_start_from_sign_symmetric_state(self):
        # The zero-state RSJ ODE and both generated ramps are odd under
        # (I,phase,V)->(-I,-phase,-V), even before a DC mean has converged.
        deck = ("RMAIN N 0 1\n.model M JJ(RTYPE=0 ICrit=100u RN=.001 CAP=0)\n"
                ".iv M 150u iv.csv STEP=150u\n.tran 1p 5p DST\n.print V(N)\n.end\n")
        for mode in (0, 1):
            _, rows = self.simulate(deck, mode)
            self.assertAlmostEqual(rows[0][0], -rows[2][0], delta=1e-13)
            self.assertAlmostEqual(rows[1][0], -rows[3][0], delta=1e-13)

    def test_slow_model_iv_timing_refinement_and_gate_physics(self):
        observed = []
        cases = ((None, "20p"), (None, "10p"), ("0", "20p"),
                 ("5m", "20p"), ("-5m", "20p"), ("10m", "20p"))
        for index, (gate, step) in enumerate(cases):
            with self.subTest(gate=gate, step=step):
                option = "" if gate is None else " GTJJ=" + gate
                deck = ("RMAIN N 0 1\n.model M JJ(RTYPE=0 ICrit=100u RN=.001 CAP=0 GWIDTH=10m)\n"
                        f".iv M 150u iv.csv STEP=150u TSTEP={step} TSTOP=5u TSTART=1u{option}\n"
                        ".tran 1p 5p DST\n.print V(N)\n.end\n")
                _, rows, log = self.simulate(deck, index % 2, details=True)
                critical = 100e-6 * max(0., 1 - abs(float(gate.rstrip("m")) / 10)) if gate is not None else 100e-6
                expected = .001 * math.sqrt((150e-6) ** 2 - critical ** 2)
                # RSJ's periodic phase correction spans <2pi, so a finite
                # plateau mean differs by at most Phi0/window. Add .1nV for
                # timestep/quadrature error, separately checked by h halving.
                bound = 2.067833831e-15 / (4e-6 - 20e-12) + 1e-10
                for (voltage, current), target in zip(rows, (150e-6, 0, -150e-6, 0)):
                    self.assertAlmostEqual(current, target, delta=1e-12)
                    self.assertAlmostEqual(voltage, math.copysign(expected, target) if target else 0., delta=bound)
                metadata = re.findall(r"Model IV point: current \(A\): ([^;]+); effective timestep \(s\): ([^;]+); average samples: (\d+)", log)
                self.assertEqual(len(metadata), 4, log)
                wanted_step = (20 if step == "20p" else 10) * 1e-12
                for _, effective, count in metadata:
                    self.assertAlmostEqual(float(effective), wanted_step, delta=1e-25)
                    self.assertGreater(int(count), 190000)
                observed.append(rows)
        for coarse, fine in zip(observed[0], observed[1]):
            self.assertAlmostEqual(coarse[0], fine[0], delta=1e-10)
        for ordinary, zero_gate in zip(observed[0], observed[2]):
            self.assertAlmostEqual(ordinary[0], zero_gate[0], delta=1e-13)
        for positive, negative in zip(observed[3], observed[4]):
            self.assertAlmostEqual(positive[0], negative[0], delta=1e-13)

    def test_invalid_iv_timing(self):
        for option in ("TSTEP=0", "TSTEP=-1p", "TSTEP=nan", "TSTOP=0", "TSTOP=50p",
                       "TSTART=0", "TSTART=-1p", "TSTART=500p", "TSTART=499.99p",
                       "TSTEP=100p TSTART=400p", "TSTOP=1e200"):
            with self.subTest(option=option):
                # The Ohmic model prevents a phase-guard retry from refining
                # the one-sample case into a valid multi-sample window.
                deck = self.deck().replace("ic=100u", "ic=0").replace("rn=16", "rn=.001").replace("cap=0.07p", "cap=0")
                deck = deck.replace(".end\n", f".iv jm 5u iv.csv STEP=5u {option}\n.end\n")
                self.simulate(deck, success=False)


if __name__ == "__main__":
    unittest.main()
