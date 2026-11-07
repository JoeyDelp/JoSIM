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
            ("E1 N 0 Q 0 2\nV1 Q 0 DC 1m\n" + base, "inconsistent initial voltage"),
            ("P1 N 0 DC 0.7\n" + base, "inconsistent initial voltage"),
            ("P1 Q 0 DC 0.7\nL1 Q 0 100p\n" + base, "zero initial inductor current"),
        )
        for deck, diagnostic in cases:
            with self.subTest(diagnostic=diagnostic):
                self.assertIn(diagnostic, self.run_deck(deck, 0, succeeds=False))

    def test_phase_ramp_preserves_offset_and_rc_state(self):
        deck = (".param SIGMA=3.291059757e-16 PHI0=0.7 DRIVE=1m\n"
                ".param PHIEND={PHI0+DRIVE*100p/SIGMA}\nP1 IN 0 PWL(0 PHI0 100p PHIEND)\n"
                "E1 DRIVE 0 IN 0 2\nR1 DRIVE N 10\nC1 N 0 1p IC=0.5m\n.tran 0.1p 30p DST\n"
                ".print V(IN) P(IN) V(N) I(C1) I(R1)\n.end\n")
        for mode in (0, 1):
            rows, _ = self.run_deck(deck, mode)
            for t, vin, phase, voltage, current, resistor in rows:
                expected = 2e-3 - 1.5e-3 * math.exp(-t / 10e-12)
                self.assertAlmostEqual(vin, 1e-3, delta=2e-9)
                self.assertAlmostEqual(phase, 0.7 + 1e-3 * t / SIGMA, delta=3e-5)
                self.assertAlmostEqual(voltage, expected, delta=8e-8)
                self.assertAlmostEqual(current, (2e-3 - expected) / 10, delta=8e-9)
                self.assertAlmostEqual(current, resistor, delta=1e-10)

    def test_phase_curvature_and_junction_initial_phase(self):
        amplitude, omega, angle = 0.2, 2 * math.pi * 1e10, math.pi / 3
        initial_voltage = SIGMA * amplitude * omega * math.cos(angle)
        deck = (f"P1 N 0 SIN(0.7 {amplitude} 10G 0 0 60)\n"
                f"C1 N 0 1p IC={initial_voltage:.17g}\n"
                "B1 N 0 JM\n.model JM JJ(RTYPE=0 IC=1u RN=100 C=0.2p)\n"
                ".tran 0.1p 10p DST\n.print V(N) P(B1) I(C1) I(B1) I(P1)\n.end\n")
        for mode in (0, 1):
            previous = None
            for h in (0.1e-12, 0.05e-12):
                rows, _ = self.run_deck(deck.replace(".tran 0.1p", f".tran {h:.17g}"), mode)
                error = 0.0
                for t, voltage, phase, current, junction, source in rows:
                    expected_phase = 0.7 + amplitude * math.sin(omega * t + angle)
                    expected_voltage = SIGMA * amplitude * omega * math.cos(omega * t + angle)
                    derivative = -SIGMA * amplitude * omega ** 2 * math.sin(omega * t + angle)
                    self.assertAlmostEqual(phase, expected_phase, delta=2e-6)
                    self.assertAlmostEqual(voltage, expected_voltage, delta=2e-10)
                    current_error = abs(current - 1e-12 * derivative)
                    if t == 0:
                        self.assertLess(current_error, 5e-12)
                    error = max(error, current_error)
                    expected_junction = 1e-6 * math.sin(expected_phase) + expected_voltage / 100 + 0.2e-12 * derivative
                    self.assertAlmostEqual(junction, expected_junction, delta=5e-10)
                    self.assertAlmostEqual(current + junction + source, 0, delta=1e-11)
                # Quadratic phase prehistory gives an O(h) current error at the
                # first differentiated sample; its initial value is constrained.
                self.assertLess(error, 0.5e-12 * SIGMA * amplitude * omega ** 3 * h + 3e-12)
                if previous is not None:
                    self.assertLess(error, 0.6 * previous + 3e-12)
                previous = error

    def test_controlled_sources_with_ramped_drive(self):
        cases = (
            ("E1 DRIVE 0 IN 0 -2\nR1 DRIVE N 10\n", "", -2, 10),
            ("G1 N 0 IN 0 0.01\nR1 N 0 100\n", "", -1, 100),
            ("F1 N 0 S 0 -2\nR1 N 0 100\n", "RC IN S 10\n", 20, 100),
            ("H1 DRIVE 0 S 0 20\nR1 DRIVE N 10\n", "RC IN S 10\n", 2, 10),
        )
        for mode in (0, 1):
            for circuit, control, gain, resistance in cases:
                with self.subTest(mode=mode, source=circuit[0]):
                    deck = ("V1 IN 0 PWL(0 1m 100p 3m)\n" + circuit + control
                            + "C1 N 0 1p IC=0.5m\n.tran 0.1p 30p DST\n"
                              ".print V(N) I(C1)\n.end\n")
                    rows, _ = self.run_deck(deck, mode)
                    tau, a, b = resistance * 1e-12, gain * 1e-3, gain * 2e7
                    for t, voltage, current in rows:
                        expected = a + b * (t - tau) + (0.5e-3 - a + b * tau) * math.exp(-t / tau)
                        self.assertAlmostEqual(voltage, expected, delta=8e-8)
                        self.assertAlmostEqual(current, (a + b * t - expected) / resistance, delta=8e-9)

    def test_controlled_feedback_and_implied_capacitor_voltage(self):
        deck = ("C1 N 0 1p IC=1m\nC2 A 0 2p\nR1 N A 10\nE1 A 0 N 0 -1\n"
                ".tran 0.1p 20p DST\n.print V(N) V(A) I(C1) I(C2) I(R1)\n.end\n")
        for mode in (0, 1):
            previous = None
            for h in (0.1e-12, 0.05e-12):
                rows, _ = self.run_deck(deck.replace(".tran 0.1p", f".tran {h:.17g}"), mode)
                self.assertAlmostEqual(rows[0][2], -1e-3, delta=1e-12)
                error = 0.0
                for t, voltage, inverted, ic, other, resistor in rows:
                    error = max(error, abs(voltage - 1e-3 * math.exp(-t / 5e-12)))
                    self.assertAlmostEqual(inverted, -voltage, delta=1e-12)
                    self.assertAlmostEqual(other, -2 * ic, delta=1e-9)
                    self.assertAlmostEqual(ic + resistor, 0, delta=1e-10)
                self.assertLess(error, 1e-3 * (h / 5e-12) ** 2 + 1e-10)
                if previous is not None:
                    self.assertLess(error, 0.4 * previous + 1e-10)
                previous = error
            gm = ("C1 N 0 1p IC=1m\nG1 N 0 N 0 0.1\n.tran 0.1p 20p DST\n"
                  ".print V(N) I(C1) I(G1)\n.end\n")
            rows, _ = self.run_deck(gm, mode)
            for t, voltage, ic, controlled in rows:
                self.assertAlmostEqual(voltage, 1e-3 * math.exp(-t / 10e-12), delta=3e-8)
                self.assertAlmostEqual(ic + controlled, 0, delta=1e-10)

    def test_phase_and_controlled_sources_restart(self):
        deck = (".option seed=42\nP1 CTRL 0 PWL(0 0.7 200p 30)\n"
                "E1 E 0 CTRL 0 2\nG1 N 0 E 0 0.001\n"
                "F1 F 0 S 0 -2\nIS 0 S DC 1u\nRF F 0 100\n"
                "H1 H 0 T 0 10\nIT 0 T DC 1u\nRH H 0 100\n"
                "C1 N 0 1p IC=1m\nB1 N 0 JM\n"
                ".model JM JJ(RTYPE=0 IC=0 RN=100 CAP=0)\n"
                "I1 0 N NOISE(1e-13 0 0.5p)\n.tran 5p 100p 0 5p 0 DST\n"
                ".print V(C1) P(B1) I(C1) I(B1) I(I1) V(E) P(CTRL) V(F) V(H)\n.end\n")
        for mode in (0, 1):
            coarse, log = self.run_deck(deck, mode)
            h = float(re.search(r"Effective timestep \(s\):\s*(\S+)", log)[1])
            self.assertLess(h, 5e-12)
            fine, _ = self.run_deck(deck.replace(".tran 5p", f".tran {h:.17g}"), mode)
            self.assertEqual(coarse, fine)

    def test_controlled_voltage_constraints_and_sensing_ports(self):
        for mode in (0, 1):
            for source in ("V1 IN 0 DC 0.5m\nE1 N 0 IN 0 2\n",
                           "I1 0 S DC 1u\nH1 N 0 S 0 1k\nC2 S 0 2p IC=0\n"):
                deck = (source + "C1 N 0 1p IC=1m\nR1 N 0 10\n"
                        ".tran 0.1p 2p DST\n.print V(N) I(C1)\n.end\n")
                rows, _ = self.run_deck(deck, mode)
                self.assertTrue(all(abs(v - 1e-3) < 1e-12 and abs(i) < 1e-12 for _, v, i in rows))
            invalid = (
                ("P1 N 0 DC 0.7\nP2 N 0 DC 0.8\n", "Matrix is singular"),
                ("V1 IN 0 DC 0.5m\nE1 N 0 IN 0 3\n", "Capacitor IC:"),
                ("I1 0 S DC 2u\nH1 N 0 S 0 1k\n", "Capacitor IC:"),
                ("E1 A B N 0 2\nR2 A B 10\n", "Matrix is singular"),  # Floating output reference.
            )
            for source, diagnostic in invalid:
                deck = (source + "C1 N 0 1p IC=1m\nR1 N 0 10\n"
                        ".tran 0.1p 2p DST\n.print V(N)\n.end\n")
                self.assertIn(diagnostic, self.run_deck(deck, mode, succeeds=False))


if __name__ == "__main__":
    unittest.main()
