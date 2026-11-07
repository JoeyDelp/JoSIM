"""Capacitor initial voltages, coupled startup and timestep reconstruction."""

import csv
import math
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest


BINARY = pathlib.Path(sys.argv.pop(1)).resolve()
SIGMA = 3.291059757e-16


class CapacitorInitialVoltage(unittest.TestCase):
    def run_deck(self, deck, mode, succeeds=True):
        with tempfile.TemporaryDirectory(prefix="capacitor-ic-", dir=BINARY.parent) as tmp:
            directory = pathlib.Path(tmp)
            result = subprocess.run([str(BINARY), "-m", "1", "-V", "1", "-a", str(mode),
                                     "-o", str(directory / "out.csv")], input=deck,
                                    cwd=directory, text=True, capture_output=True, timeout=15)
            log = result.stdout + result.stderr
            if not succeeds:
                self.assertNotEqual(result.returncode, 0, log)
                self.assertFalse((directory / "out.csv").exists())
                return log
            self.assertEqual(result.returncode, 0, log)
            with (directory / "out.csv").open() as stream:
                rows = [[float(v) for v in row] for row in list(csv.reader(stream))[1:]]
            self.assertTrue(rows and all(math.isfinite(v) for row in rows for v in row))
            return rows, log

    def test_rc_voltage_current_phase_and_refinement(self):
        for mode in (0, 1):
            for positive, negative, initial, resistance, network, sign in (
                    ("N", "0", 1e-3, 10, "R1 N 0 10\n", -1),
                    ("0", "N", -1e-3, 10, "R1 N 0 10\n", 1),
                    ("A", "B", -1e-3, 30, "R1 A 0 10\nR2 B 0 20\n", -1)):
                previous = previous_phase = None
                tau = resistance * 1e-12
                for h in (0.2e-12, 0.1e-12):
                    with self.subTest(mode=mode, nodes=(positive, negative), h=h):
                        deck = (network + f"C1 {positive} {negative} 1p IC={initial}\n"
                                + f".tran {h} 50p DST\n.print V(C1) I(C1) I(R1) P(C1)\n.end\n")
                        rows, _ = self.run_deck(deck, mode)
                        self.assertAlmostEqual(rows[0][1], initial, delta=1e-12)
                        self.assertAlmostEqual(rows[0][2], -initial / resistance, delta=1e-10)
                        self.assertAlmostEqual(rows[0][4], 0, delta=1e-12)
                        error = phase_error = 0.0
                        for t, voltage, current, resistor, phase in rows:
                            expected = initial * math.exp(-t / tau)
                            error = max(error, abs(voltage - expected))
                            self.assertAlmostEqual(current, -expected / resistance, delta=2e-7)
                            self.assertAlmostEqual(current, sign * resistor, delta=1e-10)
                            phase_error = max(phase_error,
                                abs(phase - initial * tau * (1 - math.exp(-t / tau)) / SIGMA))
                        self.assertLess(error, 2 * abs(initial) * (h / tau) ** 2 + 1e-10)
                        self.assertLess(phase_error, 2 * abs(initial) * tau / SIGMA * (h / tau) ** 2 + 1e-5)
                        if previous is not None:
                            self.assertLess(error, 0.4 * previous + 1e-10)
                            self.assertLess(phase_error, 0.4 * previous_phase + 1e-5)
                        previous = error
                        previous_phase = phase_error

    def test_dc_source_and_compatible_initial_constraint(self):
        for mode in (0, 1):
            deck = ("V1 IN 0 DC 1m\nR1 IN N 10\nC1 N 0 1p IC=0.5m\n"
                    ".tran 0.1p 30p DST\n.print V(N) I(C1) I(R1)\n.end\n")
            rows, _ = self.run_deck(deck, mode)
            for t, voltage, current, resistor in rows:
                expected = 1e-3 - 0.5e-3 * math.exp(-t / 10e-12)
                self.assertAlmostEqual(voltage, expected, delta=3e-8)
                self.assertAlmostEqual(current, (1e-3 - expected) / 10, delta=3e-9)
                self.assertAlmostEqual(current, resistor, delta=1e-10)
            compatible = deck.replace("IC=0.5m", "IC=1m").replace("C1 N 0", "C1 IN 0")
            rows, _ = self.run_deck(compatible, mode)
            self.assertTrue(all(abs(row[2]) < 1e-12 for row in rows))

    def test_parallel_capacitors_share_voltage_derivative(self):
        deck = ("R1 N 0 10\nC1 N 0 1p IC=1m\nC2 N 0 2p\n"
                ".tran 0.1p 30p DST\n.print V(C1) V(C2) I(C1) I(C2) I(R1)\n.end\n")
        for mode in (0, 1):
            rows, _ = self.run_deck(deck, mode)
            for t, v1, v2, i1, i2, ir in rows:
                self.assertAlmostEqual(v1, 1e-3 * math.exp(-t / 30e-12), delta=3e-8)
                self.assertAlmostEqual(v1, v2, delta=1e-12)
                self.assertAlmostEqual(i2, 2 * i1, delta=1e-10)
                self.assertAlmostEqual(i1 + i2 + ir, 0, delta=1e-10)

    def test_consistent_capacitor_loop(self):
        deck = ("R1 A 0 10\nR2 B 0 20\nC1 A 0 1p IC=1m\nC2 B 0 2p IC=0.4m\n"
                "C3 A B 0.5p IC=0.6m\n.tran 0.1p 20p DST\n"
                ".print V(A) V(B) I(C1) I(C2) I(C3) I(R1) I(R2)\n.end\n")
        # Independent two-node equation C*v'=-G*v; exact 2x2 exponential.
        c1, c2, c3 = 1e-12, 2e-12, 0.5e-12
        determinant = c1 * c2 + c3 * (c1 + c2)
        a, b = -(c2 + c3) / (10 * determinant), -c3 / (20 * determinant)
        c, d = -c3 / (10 * determinant), -(c1 + c3) / (20 * determinant)
        mean = (a + d) / 2
        delta = math.sqrt(((a - d) / 2) ** 2 + b * c)
        for mode in (0, 1):
            rows, _ = self.run_deck(deck, mode)
            for t, va, vb, i1, i2, i3, r1, r2 in rows:
                scale = math.exp(mean * t)
                co, si = math.cosh(delta * t), math.sinh(delta * t) / delta
                expected_a = scale * (co * 1e-3 + si * ((a - mean) * 1e-3 + b * 0.4e-3))
                expected_b = scale * (co * 0.4e-3 + si * (c * 1e-3 + (d - mean) * 0.4e-3))
                self.assertAlmostEqual(va, expected_a, delta=2e-8)
                self.assertAlmostEqual(vb, expected_b, delta=2e-8)
                self.assertAlmostEqual(r1 + i1 + i3, 0, delta=2e-10)
                self.assertAlmostEqual(r2 + i2 - i3, 0, delta=2e-10)

    def test_lc_initial_energy(self):
        deck = ("C1 N 0 1p IC=1m\nL1 N 0 100p\n.tran 0.1p 70p DST\n"
                ".print V(C1) I(C1) I(L1)\n.end\n")
        omega = 1 / math.sqrt(1e-12 * 100e-12)
        for mode in (0, 1):
            rows, _ = self.run_deck(deck, mode)
            self.assertAlmostEqual(rows[0][3], 0, delta=1e-15)
            for t, voltage, ic, il in rows:
                self.assertAlmostEqual(voltage, 1e-3 * math.cos(omega * t), delta=4e-7)
                self.assertAlmostEqual(il, 1e-4 * math.sin(omega * t), delta=4e-8)
                self.assertAlmostEqual(ic + il, 0, delta=1e-10)
                self.assertAlmostEqual((1e-12 * voltage ** 2 + 100e-12 * il ** 2) / 1e-18, 1, delta=0.002)

    def test_junction_shares_capacitor_state(self):
        deck = ("C1 N 0 1p IC=1m\nB1 N 0 JM\n"
                ".model JM JJ(IC=1u R0=30 RN=5 CAP=0.1p)\n"
                ".tran 0.1p 30p DST\n.print V(C1) V(B1) P(B1) I(C1) I(B1)\n.end\n")
        # Independent RCSJ integration; voltage remains in the subgap branch.
        def derivative(v, p):
            return -(v / 30 + 1e-6 * math.sin(p)) / 1.1e-12, v / SIGMA
        reference = []
        v, p, step = 1e-3, 0.0, 0.1e-12 / 16
        for i in range(300 * 16):
            if i % 16 == 0:
                dv, _ = derivative(v, p)
                reference.append((v, p, 1e-12 * dv, v / 30 + 1e-6 * math.sin(p) + 0.1e-12 * dv))
            k1 = derivative(v, p)
            k2 = derivative(v + step * k1[0] / 2, p + step * k1[1] / 2)
            k3 = derivative(v + step * k2[0] / 2, p + step * k2[1] / 2)
            k4 = derivative(v + step * k3[0], p + step * k3[1])
            v += step * (k1[0] + 2 * k2[0] + 2 * k3[0] + k4[0]) / 6
            p += step * (k1[1] + 2 * k2[1] + 2 * k3[1] + k4[1]) / 6
        for mode in (0, 1):
            rows, _ = self.run_deck(deck, mode)
            self.assertEqual(len(rows), len(reference))
            for (_, vc, vj, phase, ic, ij), (vr, pr, cr, jr) in zip(rows, reference):
                self.assertAlmostEqual(vc, vj, delta=1e-10)
                self.assertAlmostEqual(vc, vr, delta=1e-7)
                self.assertAlmostEqual(phase, pr, delta=0.002)
                self.assertAlmostEqual(ic, cr, delta=1e-8)
                self.assertAlmostEqual(ij, jr, delta=1e-8)
                self.assertAlmostEqual(ic + ij, 0, delta=1e-10)

    def test_mutual_inductors_start_with_zero_current(self):
        deck = ("C1 A 0 1p IC=1m\nC2 B 0 1p IC=0\nL1 A 0 100p\nL2 B 0 100p\n"
                "K1 L1 L2 0.3\n.tran 0.1p 40p DST\n.print V(A) V(B) I(L1) I(L2)\n.end\n")
        plus = 1 / math.sqrt(130e-12 * 1e-12)
        minus = 1 / math.sqrt(70e-12 * 1e-12)
        for mode in (0, 1):
            rows, _ = self.run_deck(deck, mode)
            self.assertEqual(rows[0][3:], [0, 0])
            for t, va, vb, i1, i2 in rows:
                self.assertAlmostEqual(va, 0.5e-3 * (math.cos(plus * t) + math.cos(minus * t)), delta=3e-7)
                self.assertAlmostEqual(vb, 0.5e-3 * (math.cos(plus * t) - math.cos(minus * t)), delta=3e-7)
                energy = 1e-12 * (va ** 2 + vb ** 2) + 100e-12 * (i1 ** 2 + i2 ** 2) + 60e-12 * i1 * i2
                self.assertAlmostEqual(energy / 1e-18, 1, delta=0.002)

    def test_matched_line_starts_without_incoming_wave(self):
        deck = ("C1 IN 0 1p IC=1m\nT1 IN 0 OUT 0 Z0=50 TD=10p\nRL OUT 0 50\n"
                ".tran 0.1p 30p DST\n.print V(IN) V(OUT) I(C1)\n.end\n")
        for mode in (0, 1):
            rows, _ = self.run_deck(deck, mode)
            for t, vin, vout, current in rows:
                self.assertAlmostEqual(vin, 1e-3 * math.exp(-t / 50e-12), delta=3e-8)
                expected = 0.0 if t < 10e-12 - 1e-18 else 1e-3 * math.exp(-(t - 10e-12) / 50e-12)
                self.assertAlmostEqual(vout, expected, delta=3e-8)
                self.assertAlmostEqual(current, -vin / 50, delta=1e-10)

    def test_scope_delayed_print_and_restart(self):
        scoped = (".subckt CELL P N START=2m\nR1 P N 10\nC1 P N 1p IC = {START / 2}\n.ends\n"
                  "X1 A 0 CELL\n.tran 0.1p 20p 10p 0.1p 0 DST\n.print V(A)\n.end\n")
        for mode in (0, 1):
            rows, _ = self.run_deck(scoped, mode)
            self.assertAlmostEqual(rows[0][0], 10e-12, delta=1e-18)
            self.assertAlmostEqual(rows[0][1], 1e-3 / math.e, delta=3e-8)
            deck = (".option seed=42\nC1 N 0 1p IC=1m\nB1 N 0 JM\n"
                    ".model JM JJ(RTYPE=0 IC=0 RN=100 CAP=0)\nI1 0 N NOISE(1e-13 0 0.5p)\n"
                    ".tran 5p 100p 0 5p 0 DST\n.print V(C1) P(B1) I(C1) I(B1) I(I1)\n.end\n")
            coarse, log = self.run_deck(deck, mode)
            h = float(re.search(r"Effective timestep \(s\):\s*(\S+)", log)[1])
            self.assertLess(h, 5e-12)
            fine, _ = self.run_deck(deck.replace(".tran 5p", f".tran {h:.17g}"), mode)
            self.assertEqual(coarse, fine)
            self.assertAlmostEqual(coarse[0][1], 1e-3, delta=1e-12)

    def test_invalid_initial_conditions(self):
        base = "R1 N 0 10\nC1 N 0 1p IC=1m\n.tran 0.1p 2p DST\n.print V(N)\n.end\n"
        cases = (
            (base.replace(" DST", ""), "requires .TRAN ... DST"),
            (base.replace("IC=1m", "IC="), "parameter"),
            (base.replace("IC=1m", "IC=1m IC=2m"), "parameter"),
            (base.replace("IC=1m", "IC={EXP(1000)}"), "EXP"),
            (base.replace("1p IC", "0 IC"), "positive capacitance"),
            ("V1 N 0 DC 0\n" + base, "inconsistent initial voltage"),
            ("C2 N 0 1p IC=2m\n" + base, "inconsistent initial voltage"),
            ("P1 Q 0 DC 0\nR2 Q 0 10\n" + base, "phase sources or controlled sources"),
            ("G1 Q 0 N 0 1\nR2 Q 0 10\n" + base, "phase sources or controlled sources"),
        )
        for deck, diagnostic in cases:
            with self.subTest(diagnostic=diagnostic):
                self.assertIn(diagnostic, self.run_deck(deck, 0, succeeds=False))


if __name__ == "__main__":
    unittest.main()
