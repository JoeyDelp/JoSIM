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
                           "1p 5p 10p", "1p 5p 0 1p -1p", "1p 1E200"):
            with self.subTest(parameters=parameters):
                result, _ = self.run_deck(
                    f"V1 1 0 DC 1\nR1 1 0 1k\n.tran {parameters} DST\n.print V(1)\n.end\n"
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Invalid transient analysis", result.stdout + result.stderr)

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


if __name__ == "__main__":
    unittest.main()
