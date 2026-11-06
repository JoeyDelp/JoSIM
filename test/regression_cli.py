"""Process-level regressions for rejected input and small valid circuits."""

import csv
import os
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest


BINARY = pathlib.Path(sys.argv.pop(1)).resolve()


class CliRegression(unittest.TestCase):
    SIMPLE = "V1 N 0 DC 1\nR1 N 0 1k\n.tran 1p 5p DST\n.print V(N)\n"

    def run_deck(self, deck, timeout=5, files=None, extra=(), stdin=False):
        with tempfile.TemporaryDirectory(prefix="cli-", dir=pathlib.Path.cwd()) as tmp:
            root = pathlib.Path(tmp)
            (root / "input.cir").write_text(deck)
            for name, contents in (files or {}).items():
                (root / name).parent.mkdir(parents=True, exist_ok=True)
                (root / name).write_text(contents)
            result = subprocess.run(
                [str(BINARY), "-m", "1", *extra, "-o", "output.csv", *([] if stdin else ["input.cir"])],
                input=deck if stdin else None, cwd=root, capture_output=True, text=True, timeout=timeout,
            )
            rows = None
            if (root / "output.csv").exists():
                with (root / "output.csv").open() as stream:
                    rows = list(csv.reader(stream))
            return result, rows

    def test_custom_waveform_relative_path_and_stdin(self):
        deck = "V1 1 0 CUS(wave.dat 1p)\nR1 1 0 1k\n.tran 0.25p 2p DST\n.print V(1)\n.end\n"
        for stdin in (False, True):
            with self.subTest(stdin=stdin):
                result, rows = self.run_deck(deck, files={"wave.dat": "1 1 1 1\n"}, extra=("-a", "0"), stdin=stdin)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertGreater(len(rows), 1)
                for row in rows[1:]:
                    self.assertAlmostEqual(float(row[1]), 1.0, delta=1e-9)

    def test_custom_waveform_filename_case(self):
        for filename in ("Waveform.Data", "MixedDir/Waveform.Data"):
            for function in (f"CUS({filename} 1p)", f"cus ( {filename} 1p)",
                             f"CUS(\n+ {filename} 1p)", f"CUS(\n+{filename} 1p)"):
                with self.subTest(filename=filename, function=function):
                    deck = f"V1 N 0 {function}\nR1 N 0 1k\n.tran 0.25p 2p DST\n.print V(N)\n.end\n"
                    result, rows = self.run_deck(deck, files={filename: "1 1 1 1\n"}, extra=("-a", "0"))
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertGreater(len(rows), 1)
                    self.assertTrue(all(abs(float(row[1]) - 1.0) < 1e-9 for row in rows[1:]))

    def test_continuation_without_preceding_line(self):
        result, _ = self.run_deck("+ V1 N 0 DC 1\n" + self.SIMPLE + ".end\n", timeout=3)
        self.assertIn(result.returncode, (1, 255, 4294967295), result.stdout + result.stderr)
        self.assertIn("Continuation has no preceding line", result.stdout + result.stderr)

    def test_blank_and_indented_comment_lines(self):
        result, _ = self.run_deck("\t\t\n\t* ignored comment\n\t# ignored comment\n" + self.SIMPLE + ".end\n", timeout=3)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_invalid_command_line(self):
        for args in (("--analysis=2",), ("--zebra", "1"), ("--aardvark=0",),
                     ("-m", "2"), ("-m", "1junk"), ("-V", "999"), ("-V", "1junk"),
                     ("-V", "999999999999999999999"), ("",), ("-",), ("--", ""),
                     ("-p",), ("-x", "1"), ("--help=1",)):
            with self.subTest(args=args):
                result, rows = self.run_deck(self.SIMPLE + ".end\n", extra=args, timeout=3)
                self.assertIn(result.returncode, (1, 255, 4294967295), result.stdout + result.stderr)
                self.assertIn("Command Line", result.stdout + result.stderr)
                self.assertIsNone(rows)

    def test_output_preserves_input_files(self):
        for mode in ("relative", "hardlink", "file", "included", "iv"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(prefix="preserve-", dir=pathlib.Path.cwd()) as tmp:
                root = pathlib.Path(tmp)
                path = root / "INPUT.CSV"
                deck = self.SIMPLE
                output = "./INPUT.CSV"
                if mode == "hardlink":
                    output = "ALIAS.CSV"
                elif mode == "file":
                    deck += ".file INPUT.CSV\n"
                    output = "safe.csv"
                elif mode == "included":
                    (root / "SOURCE.CSV").write_text("R2 N 0 2k\n")
                    deck += ".include SOURCE.CSV\n"
                    output = "SOURCE.CSV"
                elif mode == "iv":
                    deck += ".model M JJ(RTYPE=0 ICrit=100u RN=10 CAP=0.1p)\n.iv M 1u INPUT.CSV STEP=1u\n"
                    output = "safe.csv"
                path.write_text(deck + ".end\n")
                if mode == "hardlink":
                    os.link(path, root / output)
                before = {p: p.read_bytes() for p in root.iterdir()}
                result = subprocess.run([str(BINARY), "-m", "1", "-o", output, "INPUT.CSV"],
                                        cwd=root, capture_output=True, text=True, timeout=5)
                self.assertIn(result.returncode, (1, 255, 4294967295), result.stdout + result.stderr)
                self.assertIn("input file", result.stdout + result.stderr)
                for p, contents in before.items():
                    self.assertEqual(p.read_bytes(), contents, str(p))

    def test_recursive_includes(self):
        for files in ({"a.cir": ".include a.cir\n"},
                      {"a.cir": ".include b.cir\n", "b.cir": ".include a.cir\n"}):
            with self.subTest(files=files):
                result, _ = self.run_deck(".include a.cir\n" + self.SIMPLE + ".end\n", files=files, timeout=3)
                self.assertIn(result.returncode, (1, 255, 4294967295), result.stdout + result.stderr)
                self.assertIn("Cyclic include", result.stdout + result.stderr)

    def test_seed_reporting_and_auto_replay(self):
        noisy = "I1 0 N DC 0\nR1 N 0 50\n.temp 4.2\n.neb 500g\n.tran 0.1p 20p DST\n.print V(N)\n"
        result, first = self.run_deck(noisy + ".option SEED=AUTO\n.end\n", extra=("-V", "1"))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        match = re.search(r"RNG seed:\s*(\d+)", result.stdout)
        self.assertIsNotNone(match, result.stdout)
        result, replay = self.run_deck(noisy + f".option SEED={match.group(1)}\n.end\n", extra=("--verbose=1",))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(first, replay)
        self.assertGreater(len({row[1] for row in first[1:]}), 1)
        result, _ = self.run_deck(self.SIMPLE + ".option SEED=18446744073709551615\n.end\n", extra=("-V", "1"))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("RNG seed: 18446744073709551615", result.stdout)
        for seed in ("18446744073709551616", "2^64", "1.5", "-1", "1e99"):
            with self.subTest(seed=seed):
                result, rows = self.run_deck(self.SIMPLE + f".option SEED={seed}\n.end\n")
                self.assertIn(result.returncode, (1, 255, 4294967295), result.stdout + result.stderr)
                self.assertIsNone(rows)

    def test_explicit_stdin_overrides_filename(self):
        # --input deliberately ignores a positional file, wherever it appears.
        with tempfile.TemporaryDirectory(prefix="stdin-", dir=pathlib.Path.cwd()) as tmp:
            result = subprocess.run([str(BINARY), "--input", "ignored.cir", "-m", "1", "-o", "out.csv"],
                                    input=self.SIMPLE + ".end\n", cwd=tmp, capture_output=True, text=True, timeout=3)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertTrue((pathlib.Path(tmp) / "out.csv").exists())

    def test_output_failure_and_unsupported_format(self):
        for filename, diagnostic in (("missing/output.csv", "Cannot open the requested file for output"),
                                     ("output.xlsx", "Unknown output type")):
            with self.subTest(filename=filename), tempfile.TemporaryDirectory(prefix="output-", dir=pathlib.Path.cwd()) as tmp:
                root = pathlib.Path(tmp)
                (root / "input.cir").write_text(self.SIMPLE + ".end\n")
                result = subprocess.run([str(BINARY), "-m", "1", "-o", filename, "input.cir"],
                                        cwd=root, capture_output=True, text=True, timeout=3)
                self.assertIn(result.returncode, (1, 255, 4294967295), result.stdout + result.stderr)
                self.assertIn(diagnostic, result.stdout + result.stderr)
                self.assertFalse((root / filename).exists())

    def test_invalid_transient(self):
        for parameters in ("-1p 5p", "0 5p", "1p -5p", "1p", "1p 5p 0 -1p",
                           "1p 5p 10p", "1p 5p 0 1p -1p", "1p 1E200", "1p 5p 4.9p", "1p 1.5p 1p"):
            with self.subTest(parameters=parameters):
                result, _ = self.run_deck(
                    f"V1 1 0 DC 1\nR1 1 0 1k\n.tran {parameters} DST\n.print V(1)\n.end\n"
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Invalid transient analysis", result.stdout + result.stderr)

    def test_last_sample_window_and_clamped_fir(self):
        deck = "V1 N 0 DC 1\nR1 N 0 10\n.tran {} DST\n.print V(N)\n.end\n"
        result, rows = self.run_deck(deck.format("1p 5p 4p"), extra=("-a", "0"))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(len(rows), 2)
        self.assertAlmostEqual(float(rows[1][0]), 4e-12, delta=1e-20)
        result, rows = self.run_deck(deck.format("1p 5p 0 1p 1e100"), extra=("-a", "0"))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue(all(abs(float(row[1]) - 1.0) < 1e-9 for row in rows[1:]))

    def test_invalid_parameter_expressions(self):
        for expression in ("{}", "1+", "1+*2", "(2)3", "SIN()", "(1]", "2/(3-"):
            with self.subTest(expression=expression):
                result, _ = self.run_deck(
                    f".param A={expression}\nV1 1 0 DC A\nR1 1 0 1k\n.tran 1p 5p DST\n.print V(1)\n.end\n",
                    timeout=2,
                )
                self.assertIn(result.returncode, (1, 255, 4294967295), result.stdout + result.stderr)
                self.assertIn("Parsing", result.stdout + result.stderr)

    def test_small_valid_transient(self):
        result, rows = self.run_deck("V1 1 0 DC 1\nR1 1 0 1k\n.tran 1p 5p DST\n.print V(1)\n.end\n")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertGreater(len(rows), 1)
        self.assertAlmostEqual(float(rows[-1][1]), 1.0, delta=1e-6)

    def test_legacy_uic_token(self):
        result, _ = self.run_deck("V1 1 0 DC 1\nR1 1 0 1k\n.tran 1p 5p 0 1p 0 UIC\n.print V(1)\n.end\n")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_recursive_subcircuits(self):
        for definitions in (
            ".subckt LOOP P N\nX2 LOOP P N\n.ends\n",
            ".subckt LOOP P N\nX2 OTHER P N\n.ends\n.subckt OTHER P N\nX3 LOOP P N\n.ends\n",
        ):
            with self.subTest(definitions=definitions):
                result, _ = self.run_deck(
                    "X1 LOOP 1 0\n" + definitions + ".tran 1p 5p DST\n.print V(1)\n.end\n", timeout=2
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Recursive subcircuit", result.stdout + result.stderr)

    def test_valid_nested_subcircuits(self):
        result, rows = self.run_deck(
            "V1 1 0 DC 1\nX1 OUTER 1 0\n"
            ".subckt OUTER P N\nX2 INNER P N\n.ends\n"
            ".subckt INNER P N\nR1 P N 1k\n.ends\n"
            ".tran 1p 5p DST\n.print V(1)\n.end\n"
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertAlmostEqual(float(rows[-1][1]), 1.0, delta=1e-6)

    def test_hierarchical_pin_alias_issue81(self):
        # Supplied #81 circuit: 1 A through 10 ohm gives 10 V.
        deck = ("X1 TEST 1 0\nI1 0 1 1.0\n.subckt TEST IN GND\nR1 IN GND 10.0\n.ends\n"
                ".tran 0.25p 5n 0 1p\n.print V(IN.X1) V(GND.X1) V(0,IN.X1)\n.end\n")
        for mode in (0, 1):
            with self.subTest(mode=mode):
                result, rows = self.run_deck(deck, extra=("-a", str(mode)))
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(len(rows[0]), 4)
                self.assertIn("IN|X1", rows[0][1])
                for value, expected in zip(map(float, rows[-1][1:]), (10, 0, -10)):
                    self.assertAlmostEqual(value, expected, delta=2e-6)

    def test_nested_and_independent_pin_aliases(self):
        deck = ("X1 OUTER A 0\nX3 OUTER B 0\nI1 0 A DC 1\nI2 0 B DC 2\n"
                ".subckt OUTER P N\nX2 INNER P N\n.ends\n.subckt INNER IN RET\nR1 IN RET 10\n.ends\n"
                ".tran 0.25p 20p\n.print V(IN.X2.X1) V(RET.X2.X1) V(P.X1) V(IN.X2.X3) I(R1.X2.X1)\n.end\n")
        result, rows = self.run_deck(deck, extra=("-a", "0"))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(len(rows[0]), 6)
        for value, expected in zip(map(float, rows[-1][1:]), (10, 0, 10, 20, 1)):
            self.assertAlmostEqual(value, expected, delta=2e-6)

    def test_formal_ground_pin_can_be_connected_to_a_node(self):
        deck = ("X1 TEST A B\nI1 0 A DC 1\nR0 B 0 5\n.subckt TEST IN GND\nR1 IN GND 10\n.ends\n"
                ".tran 0.25p 20p\n.print V(A) V(B) V(GND.X1) V(IN.X1,GND.X1)\n.end\n")
        result, rows = self.run_deck(deck, extra=("-a", "0"))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(len(rows[0]), 5)
        for value, expected in zip(map(float, rows[-1][1:]), (15, 5, 5, 10)):
            self.assertAlmostEqual(value, expected, delta=2e-6)

    def test_invalid_output_requests_fail(self):
        for request in ("V(NO_SUCH_NODE)", "I(NO_SUCH_DEVICE)", "V(IN.UNKNOWN)",
                        "Q(N)", "V()", "I()", "DEVV", "", "V(N,0,0)",
                        "NODEV N 0 0", "DEVI R1 R1", "PHASE R1 R1", "TRAN",
                        "V(N,)", "V(N,,0)", "I(R1,)"):
            with self.subTest(request=request):
                deck = self.SIMPLE[:self.SIMPLE.index(".print")] + f".print {request}\n.end\n"
                result, _ = self.run_deck(deck, timeout=3)
                self.assertIn(result.returncode, (1, 255, 4294967295), result.stdout + result.stderr)
                self.assertIn("Controls", result.stdout + result.stderr)

    def test_device_differences_and_current_alias(self):
        deck = ("V1 A 0 DC 1\nI1 A 0 DC 0.125\nR1 A B 1\nR3 B C 0.5\nR4 C 0 0.5\n"
                ".tran 0.25p 20p\n.print V(R1,R3) C(R1) I(R1,R3) V(I1) C(R1,I1) V(R1,A) I(V1)\n.end\n")
        for mode in (0, 1):
            with self.subTest(mode=mode):
                result, rows = self.run_deck(deck, extra=("-a", str(mode)))
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(len(rows[0]), 8)
                for value, expected in zip(map(float, rows[-1][1:]), (0.25, 0.5, 0, 1, 0.375, -0.5, -0.625)):
                    self.assertAlmostEqual(value, expected, delta=2e-6)

    def test_only_exact_ground_names_are_reserved(self):
        for name in ("GNDREF", "AGND", "LOCALGNDNODE"):
            for mode in (0, 1):
                with self.subTest(name=name, mode=mode):
                    deck = (f"V1 A 0 DC 1\nR1 A {name} 10\nR2 {name} 0 10\n"
                            f".tran 0.25p 20p\n.print V({name}) I(R1)\n.end\n")
                    result, rows = self.run_deck(deck, extra=("-a", str(mode)))
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertAlmostEqual(float(rows[-1][1]), 0.5, delta=2e-6)
                    self.assertAlmostEqual(float(rows[-1][2]), 0.05, delta=2e-6)

    def test_phase_trace_differences(self):
        deck = ("P1 A 0 DC 1.2\nP2 B 0 DC 0.3\nR1 A 0 10\nR2 B 0 10\n"
                ".tran 0.25p 20p\n.print P(R1,R2) P(A,B) P(P2,P1)\n.end\n")
        result, rows = self.run_deck(deck, extra=("-a", "1"))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(len(rows[0]), 4)
        for value, expected in zip(map(float, rows[-1][1:]), (0.9, 0.9, -0.9)):
            self.assertAlmostEqual(value, expected, delta=2e-6)

    def test_noisy_current_output_obeys_kcl(self):
        waves = ("NOISE(1p 0 1p)", "PNOISE(1p 1 0 1p 100p 16)")
        for waveform in waves:
            for mode in (0, 1):
                for terminal, sign in (("0 N", 1), ("N 0", -1)):
                    for filtering in ("0 1p", "0 3p 4p"):
                        with self.subTest(waveform=waveform, mode=mode, terminal=terminal, filtering=filtering):
                            deck = (f"I1 {terminal} {waveform}\nR1 N 0 10\n.option SEED=123\n"
                                    f".tran 1p 100p {filtering} DST\n.print I(I1) I(R1) I(I1,R1)\n.end\n")
                            result, rows = self.run_deck(deck, extra=("-a", str(mode)))
                            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                            self.assertGreater(len(rows), 2)
                            for row in rows[1:]:
                                source, resistor, difference = map(float, row[1:])
                                tolerance = 2e-6 * max(abs(source), abs(resistor)) + 1e-20
                                self.assertAlmostEqual(resistor, sign * source, delta=tolerance)
                                self.assertAlmostEqual(difference, source - resistor, delta=tolerance)

    def test_differential_source_history_without_single_source_trace(self):
        deck = ("I1 P N NOISE(1p 0 1p)\nRP P 0 10\nRN N 0 20\n.option SEED=321\n"
                ".tran 1p 100p 0 1p DST\n.print I(RP) I(RN) I(RP,I1) I(RN,I1)\n.end\n")
        for mode in (0, 1):
            with self.subTest(mode=mode):
                result, rows = self.run_deck(deck, extra=("-a", str(mode)))
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                for row in rows[1:]:
                    positive, negative, first, second = map(float, row[1:])
                    tolerance = 2e-6 * max(abs(positive), abs(negative)) + 1e-20
                    self.assertAlmostEqual(positive + negative, 0, delta=tolerance)
                    self.assertAlmostEqual(first, 2 * positive, delta=tolerance)
                    self.assertAlmostEqual(second, 0, delta=tolerance)

    def test_requesting_source_history_does_not_change_simulation(self):
        base = ("I1 0 N NOISE(1p 0 1p)\nR1 N 0 10\nI2 0 0 NOISE(2p 0 1p)\n"
                ".option SEED=991\n.tran 1p 100p 0 1p DST\n.print I(R1){}\n.end\n")
        result, plain = self.run_deck(base.format(""), extra=("-a", "0"))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        result, captured = self.run_deck(base.format(" I(I1) I(I2)"), extra=("-a", "0"))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual([row[:2] for row in plain], [row[:2] for row in captured])
        self.assertGreater(len({row[3] for row in captured[1:]}), 1)

    def test_named_subcircuit_parameters_issue_81(self):
        for mode in (0, 1):
            for defaults, override, expected in (("", "RES=10.0", 10),
                                                   (" RES=10", "RES=5.0", 5),
                                                   (" PARAMS: RES = 10", "", 10)):
                with self.subTest(mode=mode, defaults=defaults, override=override):
                    deck = (f"X1 TEST 1 0 {override}\nI1 0 1 1.0\n"
                            f".subckt TEST IN GND{defaults}\nR1 IN GND RES\n.ends\n"
                            ".tran 0.25p 5n 0 1p\n.print V(1) V(IN.X1)\n.end\n")
                    result, rows = self.run_deck(deck, extra=("-a", str(mode)))
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    for row in rows[1:]:
                        for value in map(float, row[1:]):
                            self.assertAlmostEqual(value, expected, delta=2e-6 * expected)

    def test_nested_parameter_scopes_and_independent_instances(self):
        deck = (".param RES=7\n.param EXTRA=1\n"
                "X1 A 0 OUTER RES={RES+3}\nX2 OUTER B 0 RES=20\n"
                "I1 0 A 1\nI2 0 B 1\n"
                ".subckt OUTER IN GND RES=1\n.param DERIVED=RES+EXTRA\n"
                "XCH IN GND CHILD RES = {RES*2}\n.ends\n"
                ".subckt CHILD IN GND RES=2\n.param TOTAL=RES+DERIVED\n"
                "R1 IN GND TOTAL\n.ends\n"
                ".tran 1p 8p\n.print V(A) V(B) V(IN.XCH.X1) V(IN.XCH.X2)\n.end\n")
        for mode in (0, 1):
            result, rows = self.run_deck(deck, extra=("-a", str(mode), "-V", "2"))
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            for row in rows[1:]:
                for value, expected in zip(map(float, row[1:]), (31, 61, 31, 61)):
                    self.assertAlmostEqual(value, expected, delta=2e-6 * expected)

    def test_legacy_component_overrides_and_parameter_precedence(self):
        deck = (".param SCALE=2\nX1 CELL A 0 R1={3*SCALE}\n"
                "X2 PARENT B 0 R1|XC=9\nX3 DECLARED C 0 R1=12\n"
                "I1 0 A 1\nI2 0 B 1\nI3 0 C 1\n"
                ".subckt CELL IN RET\nR1 IN RET 10\n.ends\n"
                ".subckt PARENT IN RET\nXC IN RET CELL\n.ends\n"
                ".subckt DECLARED IN RET R1=5\nR1 IN RET {2*R1}\n.ends\n"
                ".tran 1p 8p\n.print V(A) V(B) V(C)\n.end\n")
        for mode in (0, 1):
            result, rows = self.run_deck(deck, extra=("-a", str(mode)))
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            for row in rows[1:]:
                for value, expected in zip(map(float, row[1:]), (6, 9, 24)):
                    self.assertAlmostEqual(value, expected, delta=2e-6 * expected)

    def test_scoped_junction_models_and_global_shadowing(self):
        deck = (".param BASE=100u\n.model JM JJ(RTYPE=0 IC=900u RN=10 CAP=0.1p)\n"
                "P1 A 0 DC 1.5707963267948966\nP2 B 0 DC 1.5707963267948966\n"
                "X1 CELL A 0 CRIT={BASE}\nX2 CELL B 0 CRIT={2*BASE}\n"
                ".subckt CELL IN RET CRIT=300u\n.param SCALE=2\n"
                ".model JM JJ(RTYPE=0 IC={CRIT*SCALE} RN=10 CAP=0.1p)\n"
                "B1 IN RET JM\nXCH IN RET CHILD\n.ends\n"
                ".subckt CHILD IN RET\nB1 IN RET JM AREA=0.5\n.ends\n"
                ".tran 0.1p 20p\n.print I(B1.X1) I(B1.X2) I(B1.XCH.X1) I(B1.XCH.X2)\n.end\n")
        for mode in (0, 1):
            result, rows = self.run_deck(deck, extra=("-a", str(mode)))
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            for row in rows[-10:]:
                for value, expected in zip(map(float, row[1:]), (200e-6, 400e-6, 100e-6, 200e-6)):
                    self.assertAlmostEqual(value, expected, delta=2e-6 * expected)

    def test_instance_scope_does_not_collide_with_type_name(self):
        deck = ("X1 X1 N 0 RES=12\nI1 0 N 1\n"
                ".subckt X1 IN RET RES=5\nR1 IN RET RES\n.ends\n"
                ".subckt UNUSED IN RET\nR1 IN RET REQUIRED_AT_INSTANCE\n.ends\n"
                ".tran 1p 5p\n.print V(N)\n.end\n")
        result, rows = self.run_deck(deck, extra=("-V", "2"))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue(all(abs(float(row[1]) - 12) < 24e-6 for row in rows[1:]))

    def test_invalid_instance_parameters(self):
        base = ("X1 CELL N 0 {}\nI1 0 N 1\n.subckt CELL IN RET RES=10\n"
                "R1 IN RET RES\n.ends\n.tran 1p 5p\n.print V(N)\n.end\n")
        for assignment in ("RES=", "RES=1 RES=2", "RES=MISSING", "RES={1+}", "RES={1+2", "RES==1"):
            with self.subTest(assignment=assignment):
                result, rows = self.run_deck(base.format(assignment), timeout=3)
                self.assertIn(result.returncode, (1, 255, 4294967295), result.stdout + result.stderr)
                self.assertIsNone(rows)
                self.assertIn("Parsing", result.stdout + result.stderr)
        cycle = base.format("").replace("RES=10", "RES=OTHER OTHER=RES")
        result, rows = self.run_deck(cycle, timeout=3)
        self.assertIn(result.returncode, (1, 255, 4294967295), result.stdout + result.stderr)
        self.assertIn("RES", result.stdout + result.stderr)
        self.assertIsNone(rows)

    def test_initial_phase_voltage_output_matches_ideal_source(self):
        for startup in ("", " DST"):
            for timing in ("1p 5p", "1p 1p", "1p 5p 0 1p 2p", "1e-30 5e-30"):
                with self.subTest(startup=startup, timing=timing):
                    deck = ("V1 N 0 DC 1\nR1 N 0 10\n" + f".tran {timing}{startup}\n"
                            ".print V(N) V(R1) V(V1)\n.end\n")
                    result, rows = self.run_deck(deck, extra=("-a", "1"))
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertGreater(len(rows), 1)
                    for row in rows[1:]:
                        for value in map(float, row[1:]):
                            self.assertAlmostEqual(value, 1.0, delta=2e-6)

    def test_phase_voltage_conversion_for_time_varying_source(self):
        for startup in ("", " DST"):
            deck = ("V1 A B PWL(0 0 1p 1 2p 0.5 5p 0.5)\nR1 A B 10\nR0 B 0 5\n"
                    + f".tran 0.25p 5p{startup}\n.print V(A,B) V(R1) V(V1)\n.end\n")
            result, rows = self.run_deck(deck, extra=("-a", "1"))
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            for row in rows[1:]:
                t = float(row[0]) / 1e-12
                expected = t if t < 1 else (1 - 0.5 * (t - 1) if t < 2 else 0.5)
                for value in map(float, row[1:]):
                    self.assertAlmostEqual(value, expected, delta=2e-6)

    def test_unrepresentable_startup_interval(self):
        deck = "V1 N 0 DC 1\nR1 N 0 10\n.tran 1e308 1e308{}\n.print V(N)\n.end\n"
        result, _ = self.run_deck(deck.format(""), extra=("-a", "0"))
        self.assertIn(result.returncode, (1, 255, 4294967295), result.stdout + result.stderr)
        self.assertIn("Startup interval exceeds", result.stdout + result.stderr)
        result, rows = self.run_deck(deck.format(" DST"), extra=("-a", "0"))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertAlmostEqual(float(rows[1][1]), 1.0, delta=2e-6)

    def test_phase_step_flux_quantum_area(self):
        # Phi0=h/(2e), exact SI definitions; NIST physics.nist.gov/cuu/Constants/
        flux_quantum = 6.62607015e-34 / (2 * 1.602176634e-19)
        for mode in (0, 1):
            for cycles, phase in ((1, "6.283185307179586"), (3, "18.84955592153876")):
                with self.subTest(mode=mode, cycles=cycles):
                    deck = (f"P1 N 0 DC {phase}\nR1 N 0 10\n.tran 1p 5p DST\n"
                            ".print V(N) V(P1)\n.end\n")
                    result, rows = self.run_deck(deck, extra=("-a", str(mode)))
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    for column in (1, 2):
                        area = sum(float(row[column]) * 1e-12 for row in rows[1:])
                        self.assertAlmostEqual(area / flux_quantum, cycles, delta=2e-6 * cycles)


if __name__ == "__main__":
    unittest.main()
