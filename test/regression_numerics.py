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
ROOT = pathlib.Path(__file__).resolve().parents[1]


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
    def run_example(self, deck, mode):
        with tempfile.TemporaryDirectory(prefix="numeric-example-", dir=pathlib.Path.cwd()) as tmp:
            directory = pathlib.Path(tmp)
            (directory / "input.cir").write_text(deck)
            result = subprocess.run([str(BINARY), "-m", "1", "-a", str(mode), "-o", "output.csv", "input.cir"],
                                    cwd=directory, capture_output=True, text=True, timeout=15)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            with (directory / "output.csv").open() as stream:
                rows = [[float(value) for value in row] for row in list(csv.reader(stream))[1:]]
        self.assertTrue(rows and all(math.isfinite(v) for row in rows for v in row))
        return rows

    def test_matched_transmission_line_delay_and_port_orientations(self):
        # A lossless line terminated in Z0 has no reflected wave. With a source
        # resistance Z0, each port voltage is half the drive, delayed by TD.
        def drive(t):
            if t <= 10e-12 or t >= 40e-12:
                return 0.0
            if t < 20e-12:
                return 1e8 * (t - 10e-12)
            if t <= 30e-12:
                return 1e-3
            return 1e8 * (40e-12 - t)
        ports = [("A", "0", ""), ("0", "A", ""), ("A", "AR", "VRA AR 0 DC 0.0003\n")]
        loads = [("B", "0", ""), ("0", "B", ""), ("B", "BR", "VRB BR 0 DC -0.0002\n")]
        for mode in (0, 1):
            for h in (.5, .25):
                for positive, negative, reference in ports:
                    for out_positive, out_negative, out_reference in loads:
                        with self.subTest(mode=mode, h=h, input=(positive, negative),
                                          output=(out_positive, out_negative)):
                            deck = (reference + out_reference
                                + f"V1 D {negative} PWL(0 0 10P 0 20P 1M 30P 1M 40P 0 100P 0)\n"
                                + f"RS D {positive} 50\n"
                                + f"T1 {positive} {negative} {out_positive} {out_negative} Z0=50 TD=8P\n"
                                + f"RL {out_positive} {out_negative} 50\n"
                                + f".tran {h}P 100P 0 {h}P 0 DST\n"
                                + f".print V({positive},{negative}) V({out_positive},{out_negative}) I(RS) I(RL)\n.end\n")
                            rows = self.run_example(deck, mode)
                            error = 0.0
                            for t, vin, vout, source_current, load_current in rows:
                                expected_in, expected_out = .5 * drive(t), .5 * drive(t - 8e-12)
                                error = max(error, abs(vin - expected_in), abs(vout - expected_out))
                                self.assertAlmostEqual(vin, expected_in, delta=2e-9)
                                self.assertAlmostEqual(vout, expected_out, delta=2e-9)
                                self.assertAlmostEqual(source_current, vin / 50, delta=1e-10)
                                self.assertAlmostEqual(load_current, vout / 50, delta=1e-10)
                            MEASUREMENTS.append({"case": "matched-transmission-line", "mode": mode,
                                "h_ps": h, "input": [positive, negative], "output": [out_positive, out_negative],
                                "delay_ps": 8, "voltage_error_V": error, "bound_V": 2e-9})

    def test_capacitor_reversed_and_floating_ports(self):
        for mode in (0, 1):
            for floating, reversed_port in [(False, True), (True, False), (True, True)]:
                previous = None
                for h in (.5, .25):
                    reference = "REF" if floating else "0"
                    positive, negative = (reference, "OUT") if reversed_port else ("OUT", reference)
                    deck = (("VREF REF 0 DC 0.0003\n" if floating else "")
                        + f"V1 IN {reference} PWL(0 0 200P 2M)\nR1 IN OUT 10\n"
                        + f"C1 {positive} {negative} 1P\n.tran {h}P 120P 0 {h}P 0 DST\n"
                        + f".print V(OUT,{reference}) I(R1) I(C1)\n.end\n")
                    with self.subTest(mode=mode, floating=floating, reversed=reversed_port, h=h):
                        error = 0.0
                        bound = 2 * 1e-4 * (h / 10) ** 2 + 1e-9
                        for t, voltage, resistor, capacitor in self.run_example(deck, mode):
                            expected_voltage, expected_current = analytic("RC", t)
                            error = max(error, abs(voltage - expected_voltage))
                            self.assertAlmostEqual(voltage, expected_voltage, delta=bound)
                            self.assertAlmostEqual(resistor, expected_current, delta=bound / 10)
                            self.assertAlmostEqual(capacitor, (-1 if reversed_port else 1) * resistor, delta=1e-15)
                        if previous is not None:
                            self.assertLess(error, .4 * previous + 1e-9)
                        previous = error
                        MEASUREMENTS.append({"case": "capacitor-port", "mode": mode, "floating": floating,
                            "reversed": reversed_port, "h_ps": h, "voltage_error_V": error, "bound_V": bound})

    def test_controlled_source_gain_and_port_orientations(self):
        control_ports = [("C", "0", ""), ("0", "C", ""), ("C", "CR", "VRC CR 0 DC 0.0003\n")]
        output_ports = [("O", "0", ""), ("0", "O", ""), ("O", "OR", "VRO OR 0 DC -0.0002\n")]
        for kind, gain in [("E", 2.), ("G", .002), ("F", 2.), ("H", 2000.)]:
            for mode in (0, 1):
                for cp, cn, reference in control_ports:
                    for op, on, out_reference in output_ports:
                        with self.subTest(kind=kind, mode=mode, control=(cp, cn), output=(op, on)):
                            drive = (f"V1 {cp} {cn}" if kind in "EG" else f"I1 {cn} {cp}")
                            deck = (reference + out_reference + drive + " PWL(0 0 10P 1M 20P -1M 30P 0 40P 0)\n"
                                + f"{kind}1 {op} {on} {cp} {cn} {gain}\nRL {op} {on} 1000\n"
                                + f".tran 0.5P 40P 0 0.5P 0 DST\n.print V({cp},{cn}) V({op},{on}) I(RL)\n.end\n")
                            error = 0.0
                            for t, control_voltage, voltage, current in self.run_example(deck, mode):
                                if t < 10e-12:
                                    source = 1e8 * t
                                elif t < 20e-12:
                                    source = 1e-3 - 2e8 * (t - 10e-12)
                                elif t < 30e-12:
                                    source = -1e-3 + 1e8 * (t - 20e-12)
                                else:
                                    source = 0.0
                                expected = gain * source * (-1000 if kind in "FG" else 1)
                                bound = 2e-6 * abs(expected) + 1e-10
                                self.assertAlmostEqual(voltage, expected, delta=bound)
                                self.assertAlmostEqual(current, expected / 1000, delta=bound / 1000)
                                self.assertAlmostEqual(control_voltage, source if kind in "EG" else 0., delta=2e-9)
                                error = max(error, abs(voltage - expected))
                            MEASUREMENTS.append({"case": "controlled-source", "kind": kind, "mode": mode,
                                "control": [cp, cn], "output": [op, on], "gain": gain,
                                "voltage_error_V": error, "relative_bound": 2e-6, "absolute_bound_V": 1e-10})

    def test_explicit_capacitor_precharge_and_discharge(self):
        # RC response to A - (A/d)*(t-t1)*H(t-t1) + (A/d)*(t-t2)*H(t-t2).
        # The initial step transient has decayed before the measurement window.
        amplitude, tau, capacitance = 1e-3, 10e-12, 1e-12
        ramp = amplitude / 2e-12
        def g(t):
            return t - tau + tau * math.exp(-t / tau) if t > 0 else 0
        def dg(t):
            return 1 - math.exp(-t / tau) if t > 0 else 0
        original = (ROOT / "test/comp/capacitor_precharge.cir").read_text()
        for mode in (0, 1):
            previous = None
            for h in (.1, .05, .025):
                with self.subTest(mode=mode, h=h):
                    deck = original.replace(".tran 0.1p", f".tran {h}p")
                    rows = self.run_example(deck, mode)
                    error_v = error_i = kcl = 0.0
                    for t, vin, voltage, resistor, current, source in rows:
                        if t < 190e-12:
                            continue
                        expected_v = (amplitude * (1 - math.exp(-t / tau))
                                      - ramp * g(t - 200e-12) + ramp * g(t - 202e-12))
                        expected_i = capacitance * (amplitude / tau * math.exp(-t / tau)
                                                   - ramp * dg(t - 200e-12) + ramp * dg(t - 202e-12))
                        error_v = max(error_v, abs(voltage - expected_v))
                        error_i = max(error_i, abs(current - expected_i))
                        kcl = max(kcl, abs(resistor - current), abs(resistor + source))
                        self.assertAlmostEqual(vin - voltage, 10 * resistor, delta=2e-9)
                    bound = 2 * amplitude * (h * 1e-12 / tau) ** 2
                    self.assertLess(error_v, bound + 1e-9)
                    self.assertLess(error_i, bound / 10 + 1e-10)
                    self.assertLess(kcl, 1e-15)
                    if previous is not None:
                        self.assertLess(error_v, .4 * previous[0] + 1e-9)
                        self.assertLess(error_i, .4 * previous[1] + 1e-10)
                    previous = error_v, error_i
                    MEASUREMENTS.append({"kind": "RC_PRECHARGE", "mode": mode, "h_ps": h, "deck": deck,
                                         "maximum_voltage_error_V": error_v, "maximum_current_error_A": error_i,
                                         "maximum_KCL_residual_A": kcl, "window_start_s": 190e-12})

    def test_beta_c_resistance_conversion(self):
        # beta_c = Ic*C*R^2/sigma, for the constant resistance in RTYPE=0.
        sigma = 6.62607015e-34 / (4 * math.pi * 1.602176634e-19)
        ic, capacitance, ramp = 1e-6, .1e-12, 1e7
        original = (ROOT / "test/comp/jj_beta_c.cir").read_text()
        for beta in (.2, 1., 5.):
            resistance = math.sqrt(beta * sigma / (ic * capacitance))
            equivalent_resistance = math.sqrt(beta * 3.291059757e-16 / (ic * capacitance))
            for attributes, area in (("", 1), (" AREA={2}", 2), (" IC={3*ICVAL}", 3)):
                deck = original.replace("BETA=1", f"BETA={beta}").replace("B1 N 0 JM", "B1 N 0 JM" + attributes)
                explicit = deck.replace("BETA_C=BETA", f"RN={equivalent_resistance:.17g}")
                for mode in (0, 1):
                    with self.subTest(beta=beta, mode=mode, attributes=attributes):
                        rows = self.run_example(deck, mode)
                        reference = self.run_example(explicit, mode)
                        self.assertEqual(len(rows), len(reference))
                        for direct, converted in zip(rows, reference):
                            for actual, expected in zip(direct, converted):
                                # CSV traces retain seven significant digits.
                                self.assertAlmostEqual(actual, expected, delta=1e-6 * abs(expected) + 1e-18)
                        error = kcl = 0.0
                        for t, voltage, phase, current, source in rows:
                            if t < 1e-12:
                                continue
                            expected = area * (ic * math.sin(phase) + voltage / resistance + capacitance * ramp)
                            error = max(error, abs(current - expected))
                            kcl = max(kcl, abs(current + source))
                        self.assertLess(error, area * 1e-10)
                        self.assertLess(kcl, 1e-15)
                        MEASUREMENTS.append({"kind": "JJ_BETA_C", "mode": mode, "beta_c": beta, "deck": deck,
                                             "area": area, "resistance_ohm": resistance / area,
                                             "maximum_constitutive_error_A": error, "maximum_KCL_residual_A": kcl})

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

    def test_floating_primary_and_applied_loop_flux(self):
        phi0 = 6.62607015e-34 / (2 * 1.602176634e-19)
        sigma, flux = phi0 / (2 * math.pi), .25 * phi0
        expected_primary = flux / 10e-12
        # Zero-bias KCL gives I1=-I2=I. Subtracting the two flux-linkage
        # equations gives 2*sigma*asin(I/Ic)+(L1+L2)*I=(M2-M1)*Ip.
        low, high = 0., 1e-3
        for _ in range(80):
            current = (low + high) / 2
            if 2 * sigma * math.asin(current / 1e-3) + 10e-12 * current > flux:
                high = current
            else:
                low = current
        expected_current = (low + high) / 2
        expected_phase = math.asin(expected_current / 1e-3)
        original = (ROOT / "test/comp/squid_floating_flux.cir").read_text()
        for mode in (0, 1):
            for h in (1., .5):
                for reference in ("1", "1meg", "1g"):
                    for sign in (-1, 1):
                        with self.subTest(mode=mode, h=h, reference=reference, sign=sign):
                            deck = original.replace(".tran 1p", f".tran {h}p")
                            deck = deck.replace("RREF REF 0 1g", f"RREF REF 0 {reference}")
                            if sign < 0:
                                deck = deck.replace("51.69584621154824u", "-51.69584621154824u")
                            rows = self.run_example(deck, mode)
                            max_flux_error = 0
                            for _, primary, j1, j2, phase1, phase2, drive, leakage in rows[-10:]:
                                self.assertAlmostEqual(primary, sign * expected_primary, delta=2e-11)
                                self.assertAlmostEqual(drive, sign * expected_primary, delta=2e-11)
                                self.assertAlmostEqual(j1, sign * expected_current, delta=2e-11)
                                self.assertAlmostEqual(j2, -sign * expected_current, delta=2e-11)
                                self.assertAlmostEqual(phase1, sign * expected_phase, delta=1e-8)
                                self.assertAlmostEqual(phase2, -sign * expected_phase, delta=1e-8)
                                self.assertLess(abs(leakage), 1e-15)
                                self.assertLess(abs(j1 + j2), 1e-15)
                                flux_error = abs(sigma * (phase1 - phase2) + 10e-12 * j1 - 10e-12 * primary)
                                max_flux_error = max(max_flux_error, flux_error)
                                self.assertLess(flux_error, 5e-7 * phi0)
                            MEASUREMENTS.append({"kind": "floating_flux_loop", "mode": mode, "h_ps": h,
                                                 "reference_ohm": reference, "flux_sign": sign,
                                                 "expected_primary_A": sign * expected_primary,
                                                 "expected_circulating_A": sign * expected_current,
                                                 "expected_phase_rad": sign * expected_phase,
                                                 "maximum_flux_error_Wb": max_flux_error,
                                                 "last_sample": rows[-1], "deck": deck})
            # Electrical isolation leaves one unconstrained common-mode voltage.
            # The voltage source can float; the entire unreferenced island cannot.
            deck = original.replace("RREF REF 0 1g TEMP=0", "* reference removed").replace(" I(RREF)", "")
            with tempfile.TemporaryDirectory(prefix="floating-island-", dir=pathlib.Path.cwd()) as tmp:
                directory = pathlib.Path(tmp)
                (directory / "input.cir").write_text(deck)
                result = subprocess.run([str(BINARY), "-m", "1", "-a", str(mode), "input.cir"],
                                        cwd=directory, capture_output=True, text=True, timeout=15)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("singular", (result.stdout + result.stderr).lower())

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
            "reference": "Closed-form R/L/C ramps, matched-line delay, controlled-source gains, RC precharge, KCL/zero-flux JJ/L equilibrium and beta_c conversion; SI units",
            "scope": "Linear dynamics/port orientations, static JJ/L equilibrium and specified constitutive response; switching/long-run studies have separate owners",
            "cases": MEASUREMENTS,
        }, indent=2) + "\n")
    sys.exit(not result.wasSuccessful())
