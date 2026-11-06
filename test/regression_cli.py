"""Process-level regressions for rejected input and small valid circuits."""

import csv
import pathlib
import subprocess
import sys
import tempfile
import unittest


BINARY = pathlib.Path(sys.argv.pop(1)).resolve()


class CliRegression(unittest.TestCase):
    def run_deck(self, deck, timeout=5):
        with tempfile.TemporaryDirectory(prefix="cli-", dir=pathlib.Path.cwd()) as tmp:
            root = pathlib.Path(tmp)
            (root / "input.cir").write_text(deck)
            result = subprocess.run(
                [str(BINARY), "-m", "1", "-o", "output.csv", "input.cir"],
                cwd=root, capture_output=True, text=True, timeout=timeout,
            )
            rows = None
            if (root / "output.csv").exists():
                with (root / "output.csv").open() as stream:
                    rows = list(csv.reader(stream))
            return result, rows

    def test_invalid_transient(self):
        for parameters in ("-1p 5p", "0 5p", "1p -5p", "1p", "1p 5p 0 -1p",
                           "1p 5p 10p", "1p 5p 0 1p -1p", "1p 1E200"):
            with self.subTest(parameters=parameters):
                result, _ = self.run_deck(
                    f"V1 1 0 DC 1\nR1 1 0 1k\n.tran {parameters} DST\n.print V(1)\n.end\n"
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Invalid transient analysis", result.stdout + result.stderr)

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
