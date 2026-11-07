"""Quadratic differential inductance: flux, dynamics, coupling and solver state."""

import csv
import math
import pathlib
import re
import subprocess
import sys
import tempfile
import time
import unittest


BINARY = pathlib.Path(sys.argv.pop(1)).resolve()
SIGMA = 3.291059757e-16


def rk4(derivative, initial, times, max_step):
    state, previous, result = list(initial), 0.0, []
    for target in times:
        count = max(1, math.ceil((target - previous) / max_step))
        h = (target - previous) / count
        for _ in range(count):
            k1 = derivative(previous, state)
            k2 = derivative(previous + h / 2, [x + h * k / 2 for x, k in zip(state, k1)])
            k3 = derivative(previous + h / 2, [x + h * k / 2 for x, k in zip(state, k2)])
            k4 = derivative(previous + h, [x + h * k for x, k in zip(state, k3)])
            state = [x + h * (a + 2 * b + 2 * c + d) / 6
                     for x, a, b, c, d in zip(state, k1, k2, k3, k4)]
            previous += h
        previous = target
        result.append(tuple(state))
    return result


class NonlinearInductor(unittest.TestCase):
    def run_deck(self, deck, mode, succeeds=True):
        with tempfile.TemporaryDirectory(prefix="nonlinear-l-", dir=BINARY.parent) as tmp:
            output = pathlib.Path(tmp) / "out.csv"
            started = time.perf_counter()
            result = subprocess.run([str(BINARY), "-m", "1", "-V", "1", "-a", str(mode), "-o", str(output)],
                                    input=deck, cwd=tmp, text=True, capture_output=True, timeout=20)
            elapsed = time.perf_counter() - started
            log = result.stdout + result.stderr
            if not succeeds:
                self.assertNotEqual(result.returncode, 0, log)
                self.assertFalse(output.exists())
                return log
            self.assertEqual(result.returncode, 0, log)
            with output.open() as stream:
                rows = [[float(v) for v in row] for row in list(csv.reader(stream))[1:]]
            self.assertTrue(rows and all(math.isfinite(v) for row in rows for v in row))
            return rows, log, elapsed

    def test_zero_alpha_preserves_linear_results(self):
        deck = ("C1 N 0 1p IC=1m\nR1 N 0 100\nL1 N 0 100p\n"
                ".tran 0.1p 40p DST\n.print V(N) I(L1) P(L1)\n.end\n")
        for mode in (0, 1):
            linear, _, _ = self.run_deck(deck, mode)
            zero, _, _ = self.run_deck(deck.replace("100p\n", "100p ALPHA=0\n"), mode)
            self.assertEqual(linear, zero)

    def test_current_ramp_flux_voltage_and_refinement(self):
        inductance, alpha, rate = 100e-12, 1e8, 1e7
        for mode in (0, 1):
            previous = None
            for h in (0.2e-12, 0.1e-12, 0.05e-12):
                deck = ("I1 0 N PWL(0 0 100p 1m)\nL1 N 0 100p ALPHA=1e8\n"
                        f".tran {h:.17g} 50p DST\n.print I(L1) V(L1) P(L1) I(I1)\n.end\n")
                rows, _, _ = self.run_deck(deck, mode)
                error = 0.0
                for t, current, voltage, phase, source in rows:
                    self.assertAlmostEqual(current, rate * t, delta=1e-10)
                    self.assertAlmostEqual(current, source, delta=1e-12)
                    flux = inductance * (current + alpha * current ** 3 / 3)
                    self.assertAlmostEqual(phase * SIGMA, flux, delta=2e-6 * abs(flux) + 2e-23)
                    if t > 1e-12:
                        expected = inductance * (1 + alpha * (rate * t) ** 2) * rate
                        error = max(error, abs(voltage - expected))
                bound = 2 / 3 * inductance * alpha * rate ** 3 * h ** 2
                self.assertLess(error, 1.1 * bound + 1e-8)
                if previous is not None:
                    self.assertLess(error, 0.4 * previous + 1e-8)
                previous = error

    def test_voltage_driven_cubic_and_scoped_parameters(self):
        # Integrating prescribed voltage gives an independent scalar cubic.
        for mode in (0, 1):
            h, alpha, inductance = 1e-12, 1e12, 100e-12
            deck = ("V1 N 0 PWL(0 0 100p 1)\nX1 N 0 COIL A=1e12\n"
                    ".subckt COIL P N A=0\nL1 P N 100p ALPHA = {A}\n.ends\n"
                    ".tran 1p 30p DST\n.print I(L1|X1) V(L1|X1) P(L1|X1)\n.end\n")
            rows, _, _ = self.run_deck(deck, mode)
            before, before2 = 0.0, 0.0
            for t, current, voltage, phase in rows:
                flux = 2 * h / 3 * (1e10 * t) + 4 / 3 * before - 1 / 3 * before2
                expected = 2 / math.sqrt(alpha) * math.sinh(math.asinh(1.5 * math.sqrt(alpha) * flux / inductance) / 3)
                self.assertAlmostEqual(current, expected, delta=2e-6 * abs(expected) + 1e-12)
                self.assertAlmostEqual(phase * SIGMA, flux, delta=2e-6 * abs(flux) + 1e-23)
                self.assertAlmostEqual(voltage, 1e10 * t, delta=1e-8)
                before2, before = before, flux

    def test_biased_small_signal_uses_differential_inductance(self):
        deck = ("I1 0 N SINE(200u 1u 1g)\nL1 N 0 100p ALPHA=1e8\n"
                ".tran 0.1p 100p\n.print I(L1) V(L1)\n.end\n")
        for mode in (0, 1):
            rows, _, _ = self.run_deck(deck, mode)
            for t, current, voltage in rows:
                if t < 1e-12:
                    continue
                omega = 2 * math.pi * 1e9
                expected_i = 200e-6 + 1e-6 * math.sin(omega * t)
                expected_v = 100e-12 * (1 + 1e8 * expected_i ** 2) * 1e-6 * omega * math.cos(omega * t)
                self.assertAlmostEqual(current, expected_i, delta=1e-10)
                self.assertAlmostEqual(voltage, expected_v, delta=2e-9)

    def test_nonlinear_lc_reference_energy_and_refinement(self):
        inductance, capacitance, alpha = 100e-12, 1e-12, 1e8
        def derivative(t, state):
            voltage, current = state
            return -current / capacitance, voltage / (inductance * (1 + alpha * current ** 2))
        for mode in (0, 1):
            previous = None
            for h in (0.2e-12, 0.1e-12, 0.05e-12):
                deck = ("C1 N 0 1p IC=1m\nL1 N 0 100p ALPHA=1e8\n"
                        f".tran {h:.17g} 100p DST\n.print V(N) I(L1) P(L1)\n.end\n")
                rows, _, _ = self.run_deck(deck, mode)
                reference = rk4(derivative, (1e-3, 0), [r[0] for r in rows], h / 8)
                error = 0.0
                for (_, voltage, current, phase), (vr, ir) in zip(rows, reference):
                    error = max(error, abs(voltage - vr), 10 * abs(current - ir))
                    energy = 0.5 * capacitance * voltage ** 2 + inductance * (current ** 2 / 2 + alpha * current ** 4 / 4)
                    self.assertAlmostEqual(energy / 0.5e-18, 1, delta=0.004)
                    flux = inductance * (current + alpha * current ** 3 / 3)
                    self.assertAlmostEqual(phase * SIGMA, flux, delta=2e-6 * abs(flux) + 1e-22)
                self.assertLess(error, 2e-6)
                if previous is not None:
                    self.assertLess(error, 0.4 * previous + 3e-10)
                previous = error

    def test_reciprocal_mutual_coupling(self):
        for coupling, alpha2 in ((0.3, 2e8), (-0.3, 0)):
            l1, l2, c1, c2, alpha1 = 100e-12, 150e-12, 1e-12, 1.5e-12, 1e8
            mutual = coupling * math.sqrt(l1 * l2)
            def derivative(t, state):
                v1, v2, i1, i2 = state
                d1, d2 = l1 * (1 + alpha1 * i1 ** 2), l2 * (1 + alpha2 * i2 ** 2)
                determinant = d1 * d2 - mutual ** 2
                return -i1 / c1, -i2 / c2, (d2 * v1 - mutual * v2) / determinant, (d1 * v2 - mutual * v1) / determinant
            deck = ("C1 A 0 1p IC=1m\nC2 B 0 1.5p IC=-0.3m\nL1 A 0 100p ALPHA=1e8\n"
                    f"L2 B 0 150p ALPHA={alpha2}\nK1 L1 L2 {coupling}\n.tran 0.1p 80p DST\n"
                    ".print V(A) V(B) I(L1) I(L2) P(L1) P(L2)\n.end\n")
            for mode in (0, 1):
                rows, _, _ = self.run_deck(deck, mode)
                reference = rk4(derivative, (1e-3, -0.3e-3, 0, 0), [r[0] for r in rows], 0.0125e-12)
                for (_, v1, v2, i1, i2, p1, p2), expected in zip(rows, reference):
                    for actual, target in zip((v1, v2, 10 * i1, 10 * i2), (expected[0], expected[1], 10 * expected[2], 10 * expected[3])):
                        self.assertAlmostEqual(actual, target, delta=1e-6)
                    self1, self2 = l1 * (i1 + alpha1 * i1 ** 3 / 3), l2 * (i2 + alpha2 * i2 ** 3 / 3)
                    flux1, flux2 = self1 + mutual * i2, self2 + mutual * i1
                    # CSV rounding applies to each current before cancellation.
                    self.assertAlmostEqual(p1 * SIGMA, flux1, delta=2e-6 * (abs(self1) + abs(mutual * i2)) + 1e-22)
                    self.assertAlmostEqual(p2 * SIGMA, flux2, delta=2e-6 * (abs(self2) + abs(mutual * i1)) + 1e-22)
                    energy = (0.5 * (c1 * v1 ** 2 + c2 * v2 ** 2) + l1 * (i1 ** 2 / 2 + alpha1 * i1 ** 4 / 4)
                              + l2 * (i2 ** 2 / 2 + alpha2 * i2 ** 4 / 4) + mutual * i1 * i2)
                    self.assertAlmostEqual(energy / (0.5 * (c1 * 1e-6 + c2 * 0.3e-3 ** 2)), 1, delta=0.004)

    def test_noise_sampling_and_seeded_restart(self):
        base = (".option seed=42\nI1 0 N NOISE(1e-12 0 0.2p)\nR1 N 0 10\n"
                ".tran 0.1p 20p DST\n.print I(I1)\n.end\n")
        for mode in (0, 1):
            plain, _, _ = self.run_deck(base, mode)
            extra = "V2 M 0 PWL(0 0 20p 1m)\nL2 M 0 100p ALPHA=1e12\n"
            nonlinear, _, _ = self.run_deck(base.replace(".tran", extra + ".tran"), mode)
            self.assertEqual(plain, nonlinear)
            deck = (".option seed=42\nC1 N 0 1p IC=1m\nB1 N 0 JM\nL1 N 0 100p ALPHA=1e8\n"
                    ".model JM JJ(RTYPE=0 IC=0 RN=100 CAP=0)\nI1 0 N NOISE(1e-13 0 0.5p)\n"
                    ".tran 5p 100p 0 5p 0 DST\n.print V(N) I(L1) P(B1) P(L1) I(I1)\n.end\n")
            coarse, log, _ = self.run_deck(deck, mode)
            h = float(re.search(r"Effective timestep \(s\):\s*(\S+)", log)[1])
            self.assertLess(h, 5e-12)
            fine, _, _ = self.run_deck(deck.replace(".tran 5p", f".tran {h:.17g}"), mode)
            self.assertEqual(coarse, fine)

    def test_nonlinear_rl_with_matched_transmission_line(self):
        deck = ("V1 D 0 PWL(0 0 10p 1m 40p 1m)\nL1 D IN 100p ALPHA=1e10\n"
                "T1 IN 0 OUT 0 Z0=50 TD=5p\nR1 OUT 0 50\n.tran 0.05p 40p DST\n"
                ".print I(L1) V(IN) V(OUT)\n.end\n")
        def derivative(t, state):
            current = state[0]
            drive = min(1e-3, 1e8 * t)
            return ((drive - 50 * current) / (100e-12 * (1 + 1e10 * current ** 2)),)
        for mode in (0, 1):
            rows, _, _ = self.run_deck(deck, mode)
            reference = rk4(derivative, (0,), [r[0] for r in rows], 0.00625e-12)
            for index, ((t, current, vin, vout), (expected,)) in enumerate(zip(rows, reference)):
                self.assertAlmostEqual(current, expected, delta=5e-9)
                self.assertAlmostEqual(vin, 50 * expected, delta=3e-7)
                delayed = 0 if index < 100 else 50 * reference[index - 100][0]
                self.assertAlmostEqual(vout, delayed, delta=3e-7)

    def test_short_nonlinear_ladder(self):
        cells, inductance, capacitance, alpha = 8, 10e-12, 0.1e-12, 1e9
        parts = ["V1 N0 0 PWL(0 0 10p 0.1m 40p 0.1m)"]
        for cell in range(1, cells + 1):
            parts.extend((f"L{cell} N{cell-1} N{cell} 10p ALPHA=1e9", f"C{cell} N{cell} 0 0.1p"))
        deck = "\n".join(parts) + f"\nR1 N{cells} 0 10\n.tran 0.1p 40p DST\n.print V(N8) I(L1)\n.end\n"
        def derivative(t, state):
            voltages, currents = state[:cells], state[cells:]
            incoming = [min(0.1e-3, 1e7 * t)] + list(voltages[:-1])
            outgoing = list(currents[1:]) + [voltages[-1] / 10]
            return ([(a - b) / capacitance for a, b in zip(currents, outgoing)]
                    + [(a - b) / (inductance * (1 + alpha * current ** 2))
                       for a, b, current in zip(incoming, voltages, currents)])
        for mode in (0, 1):
            rows, _, elapsed = self.run_deck(deck, mode)
            reference = rk4(derivative, [0] * (2 * cells), [r[0] for r in rows], 0.0125e-12)
            for (_, voltage, current), state in zip(rows, reference):
                self.assertAlmostEqual(voltage, state[cells-1], delta=4e-7)
                self.assertAlmostEqual(current, state[cells], delta=4e-8)
            print(f"Nonlinear ladder: {cells} cells, mode={mode}, {elapsed:.3f} s")

    def test_invalid_parameters_coupling_and_bounded_failure(self):
        base = "I1 0 N DC 0\nL1 N 0 100p ALPHA=1e8\n.tran 0.1p 1p DST\n.print V(N)\n.end\n"
        for declaration, diagnostic in (("ALPHA=-1", "ALPHA must be finite and non-negative"),
                                         ("ALPHA=1 ALPHA=2", "parameter"), ("ALPHA=", "parameter"),
                                         ("ALPHA={EXP(1000)}", "EXP"), ("UNKNOWN=1", "unknown inductor parameter")):
            self.assertIn(diagnostic, self.run_deck(base.replace("ALPHA=1e8", declaration), 0, False))
        self.assertIn("positive L0", self.run_deck(base.replace("100p ALPHA", "-100p ALPHA"), 0, False))
        for coupling in (1, 1.1, -1):
            deck = base.replace(".tran", f"L2 M 0 100p\nK1 L1 L2 {coupling}\n.tran")
            self.assertIn("positive-definite", self.run_deck(deck, 0, False))
        indefinite = base.replace(".tran", "L2 M 0 100p\nL3 P 0 100p\nK1 L1 L2 .9\nK2 L1 L3 .9\nK3 L2 L3 -.9\n.tran")
        self.assertIn("positive-definite", self.run_deck(indefinite, 0, False))
        self.assertIn("self-coupling", self.run_deck(base.replace(".tran", "K1 L1 L1 .1\n.tran"), 0, False))
        for mode in (0, 1):
            impossible = base.replace("DC 0", "DC 1e100").replace("ALPHA=1e8", "ALPHA=1e308")
            self.assertIn("Nonlinear inductor timestep retry limit", self.run_deck(impossible, mode, False))


if __name__ == "__main__":
    unittest.main()
