"""Prevent false qualification from missing, duplicated or skipped test results."""

import json
import copy
import pathlib
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
import hashlib
import io
import tarfile
import zipfile
import subprocess
from unittest import mock
from types import SimpleNamespace

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from ctest_results import REQUIRED_TESTS, verify_ctest
from coverage import first_party_files, source_digest, validate_mapping
from check_coverage import evaluate
from verify_release import execute, extract_archive, prepare, public_release, verify_checksum, verify_payload


class QualificationRegression(unittest.TestCase):
    def test_public_preparation_downloads_anonymously_and_cannot_be_relabelled(self):
        with tempfile.TemporaryDirectory(prefix="public-prepare-") as temp:
            root = pathlib.Path(temp)
            tag, revision, target = "v3.0.0", "a" * 40, "linux-x64-rhel6"
            name = "JoSIM-" + tag + "-" + target
            archive = name + ".tar.gz"
            content = b"fixture, never executed"
            manifest = {"project": "JoSIM", "version": "3.0.0", "target": target,
                        "source": {"revision": revision, "dirty": False}, "tests": {"status": "passed"},
                        "files": [{"path": "bin/josim-cli", "size": len(content),
                                   "sha256": hashlib.sha256(content).hexdigest()}]}
            encoded = json.dumps(manifest).encode()
            buffer = io.BytesIO()
            with tarfile.open(fileobj=buffer, mode="w:gz") as stream:
                for filename, data in [("bin/josim-cli", content), ("MANIFEST.json", encoded)]:
                    entry = tarfile.TarInfo(filename);entry.size = len(data);entry.mode = 0o644
                    stream.addfile(entry, io.BytesIO(data))
            downloads = {archive: buffer.getvalue(), name + ".manifest.json": encoded}
            downloads[archive + ".sha256"] = (hashlib.sha256(downloads[archive]).hexdigest() + "  " + archive + "\n").encode()
            prefix = "https://github.com/JoeyDelp/JoSIM/releases/download/" + tag + "/"
            release = {"id": 1, "html_url": "https://github.com/JoeyDelp/JoSIM/releases/tag/" + tag,
                       "published_at": "fixture time", "assets": [{"name": n, "state": "uploaded",
                       "size": len(data), "digest": "sha256:" + hashlib.sha256(data).hexdigest(),
                       "browser_download_url": prefix + n} for n, data in downloads.items()]}

            def receive(request, **kwargs):
                self.assertIsNone(request.get_header("Authorization"))
                return io.BytesIO(downloads[request.full_url[len(prefix):]])

            with mock.patch("verify_release.public_release", return_value=release), \
                 mock.patch("verify_release.urllib.request.urlopen", side_effect=receive), \
                 mock.patch.dict("os.environ", {"JOSIM_GITHUB_API_TOKEN": "fixture-only"}):
                result = prepare(root / "work", tag, revision, target)
            self.assertEqual(result["status"], "prepared")
            self.assertEqual((root / "work/payload/bin/josim-cli").read_bytes(), content)
            output = root / "wrong-request.json"
            command = [sys.executable, "-B", str(pathlib.Path(__file__).resolve().parents[1] / "tools/verify_release.py"),
                       "--tag", tag, "--revision", revision, "--target", "windows-x64",
                       "--work-dir", str(root / "work"), "--output", str(output), "--execute-only"]
            run = subprocess.run(command, capture_output=True, text=True, timeout=10)
            self.assertNotEqual(run.returncode, 0)
            self.assertEqual(json.loads(output.read_text())["status"], "failed")
            self.assertIn("failed", output.with_suffix(".md").read_text())

    def test_public_tag_must_identify_expected_published_source(self):
        release = {"tag_name": "v3.0.0", "draft": False, "prerelease": False, "published_at": "2026-11-07"}
        revision = "a" * 40
        with mock.patch("verify_release.api", side_effect=[release, {"object": {"type": "commit", "sha": revision}}]):
            self.assertEqual(public_release("v3.0.0", revision), release)
        for damage in ("revision", "draft", "prerelease", "tag"):
            changed = copy.deepcopy(release)
            if damage in ("draft", "prerelease"):
                changed[damage] = True
            elif damage == "tag":
                changed["tag_name"] = "v2.7"
            with self.subTest(damage=damage), mock.patch("verify_release.api", side_effect=[
                    changed, {"object": {"type": "commit", "sha": "b" * 40}}]), self.assertRaises(RuntimeError):
                public_release("v3.0.0", revision)

    def test_public_package_payload_identity_and_inventory(self):
        with tempfile.TemporaryDirectory(prefix="public-payload-") as temp:
            root = pathlib.Path(temp)
            (root / "bin").mkdir()
            (root / "bin/josim-cli").write_bytes(b"fixture executable, never run")
            content = (root / "bin/josim-cli").read_bytes()
            manifest = {"project": "JoSIM", "version": "3.0.0", "target": "linux-x64-rhel6",
                        "source": {"revision": "a" * 40, "dirty": False}, "tests": {"status": "passed"},
                        "files": [{"path": "bin/josim-cli", "size": len(content), "sha256": hashlib.sha256(content).hexdigest()}]}

            def check(value):
                data = json.dumps(value).encode()
                (root / "MANIFEST.json").write_bytes(data)
                return verify_payload(root, data, "v3.0.0", "a" * 40, "linux-x64-rhel6")

            self.assertEqual(check(manifest), manifest)
            for damage in ("revision", "dirty", "target", "version", "tests", "hash", "size", "inventory", "duplicate", "escape"):
                value = copy.deepcopy(manifest)
                if damage == "revision": value["source"]["revision"] = "b" * 40
                elif damage == "dirty": value["source"]["dirty"] = True
                elif damage == "target": value["target"] = "windows-x64"
                elif damage == "version": value["version"] = "2.7"
                elif damage == "tests": value["tests"]["status"] = "skipped"
                elif damage == "hash": value["files"][0]["sha256"] = "0" * 64
                elif damage == "size": value["files"][0]["size"] += 1
                elif damage == "inventory": value["files"] = []
                elif damage == "duplicate": value["files"].append(value["files"][0])
                elif damage == "escape": value["files"][0]["path"] = "../outside"
                with self.subTest(damage=damage), self.assertRaises(RuntimeError): check(value)
            check(manifest)
            (root / "extra.txt").write_text("unlisted")
            with self.assertRaisesRegex(RuntimeError, "inventories"): check(manifest)
            (root / "extra.txt").unlink()
            with self.assertRaisesRegex(RuntimeError, "sidecar"):
                verify_payload(root, b"{}", "v3.0.0", "a" * 40, "linux-x64-rhel6")
            (root / "docs/context").mkdir(parents=True)
            (root / "docs/context/sentinel").write_bytes(b"local fixture")
            value = copy.deepcopy(manifest)
            value["files"].append({"path": "docs/context/sentinel", "size": 13,
                                    "sha256": hashlib.sha256(b"local fixture").hexdigest()})
            with self.assertRaisesRegex(RuntimeError, "local project context"): check(value)

    def test_public_execution_cannot_pass_empty_skipped_failed_or_incomplete_tests(self):
        with tempfile.TemporaryDirectory(prefix="public-execute-") as temp:
            root = pathlib.Path(temp)
            (root / "archive").write_bytes(b"fixture archive")
            (root / "payload/bin").mkdir(parents=True)
            (root / "payload/bin/josim-cli").write_bytes(b"never executed")
            manifest = {"project": "JoSIM", "version": "3.0.0", "target": "linux-x64-rhel6",
                        "source": {"revision": "a" * 40, "dirty": False}, "tests": {"status": "passed"},
                        "files": [{"path": "bin/josim-cli", "size": 14,
                                   "sha256": hashlib.sha256(b"never executed").hexdigest()}]}
            data = json.dumps(manifest).encode()
            (root / "sidecar").write_bytes(data)
            (root / "payload/MANIFEST.json").write_bytes(data)
            for outcome in ("pass", "empty", "skipped", "failed", "numerics"):
                result = {"status": "prepared", "tag": "v3.0.0", "revision": "a" * 40,
                          "target": "linux-x64-rhel6", "archive": "archive", "manifest": "sidecar",
                          "archive_sha256": hashlib.sha256(b"fixture archive").hexdigest()}
                output = root / outcome / "summary.json"

                def run(command, **kwargs):
                    stream = kwargs["stdout"]
                    if command[-1] == "-v":
                        stream.write("v3.0.0.aaaaaaa\n")
                    else:
                        count = 0 if outcome == "empty" else 1
                        stream.write("Ran {} test in 0.001s\nOK{}\n".format(count, " (skipped=1)" if outcome == "skipped" else ""))
                        if "--report" in command:
                            pathlib.Path(command[-1]).write_text(json.dumps({"passed": outcome != "numerics",
                                "binary_sha256": hashlib.sha256(b"never executed").hexdigest(), "cases": [{"fixture": True}]}))
                    return SimpleNamespace(returncode=1 if outcome == "failed" else 0)

                with mock.patch("verify_release.platform.system", return_value="Linux"), \
                     mock.patch("verify_release.platform.machine", return_value="x86_64"), \
                     mock.patch("verify_release.source_identity", return_value={"fixture": True}), \
                     mock.patch("verify_release.subprocess.run", side_effect=run):
                    if outcome == "pass":
                        execute(root, result, output)
                        self.assertEqual(result["status"], "passed")
                    else:
                        with self.subTest(outcome=outcome), self.assertRaises(RuntimeError):
                            execute(root, result, output)
                        self.assertNotEqual(result["status"], "passed")

    def test_public_archive_paths_links_and_checksums(self):
        with tempfile.TemporaryDirectory(prefix="public-archive-") as temp:
            root = pathlib.Path(temp)
            archive = root / "fixture.zip"
            with zipfile.ZipFile(archive, "w") as stream: stream.writestr("bin/example.txt", "payload")
            extract_archive(archive, root / "valid")
            self.assertEqual((root / "valid/bin/example.txt").read_text(), "payload")
            sidecar = root / "fixture.zip.sha256"
            sidecar.write_text(hashlib.sha256(archive.read_bytes()).hexdigest() + "  fixture.zip\n")
            verify_checksum(archive, sidecar)
            sidecar.write_text("0" * 64 + "  fixture.zip\n")
            with self.assertRaisesRegex(RuntimeError, "checksum"): verify_checksum(archive, sidecar)
            for i, name in enumerate(("../outside", "/absolute", "C:/drive", "bin\\escape")):
                # Keep the unsafe bytes: ZipInfo's constructor normalizes them on Windows.
                entry = zipfile.ZipInfo("fixture")
                entry.filename = entry.orig_filename = name
                with zipfile.ZipFile(archive, "w") as stream: stream.writestr(entry, "invalid")
                with self.subTest(name=name), self.assertRaisesRegex(RuntimeError, "Unsafe"):
                    extract_archive(archive, root / ("invalid" + str(i)))
                with mock.patch("zipfile.os.sep", "\\"), self.assertRaisesRegex(RuntimeError, "Unsafe"):
                    extract_archive(archive, root / ("windows-reader" + str(i)))
            tar = root / "fixture.tar.gz"
            with tarfile.open(tar, "w:gz") as stream:
                member = tarfile.TarInfo("bin/example.txt");member.size = 7;member.mode = 0o644
                stream.addfile(member, io.BytesIO(b"payload"))
            extract_archive(tar, root / "valid-tar")
            self.assertEqual((root / "valid-tar/bin/example.txt").read_text(), "payload")
            with tarfile.open(tar, "w:gz") as stream:
                member = tarfile.TarInfo("link");member.type = tarfile.SYMTYPE;member.linkname = "../outside"
                stream.addfile(member)
            with self.assertRaisesRegex(RuntimeError, "link"): extract_archive(tar, root / "invalid-link")

    def verify(self, declared, executed, status=None):
        with tempfile.TemporaryDirectory(prefix="qualification-") as temp:
            root = pathlib.Path(temp)
            inventory = root / "inventory.json"
            inventory.write_text(json.dumps({"tests": [{"name": n} for n in declared]}))
            suite = ET.Element("testsuite")
            for name in executed:
                case = ET.SubElement(suite, "testcase", name=name)
                if status and name == executed[0]:
                    ET.SubElement(case, status)
            junit = root / "tests.xml"
            ET.ElementTree(suite).write(junit)
            return verify_ctest(inventory, junit)

    def test_complete_inventory_includes_additional_tests(self):
        names = sorted(REQUIRED_TESTS) + ["integration::extra"]
        self.assertEqual(self.verify(names, names)["entries"], len(names))

    def test_missing_execution_is_rejected(self):
        names = sorted(REQUIRED_TESTS)
        with self.assertRaisesRegex(RuntimeError, "differ"):
            self.verify(names, names[:-1])

    def test_required_group_cannot_disappear_from_both(self):
        names = sorted(REQUIRED_TESTS)[:-1]
        with self.assertRaisesRegex(RuntimeError, "Required"):
            self.verify(names, names)

    def test_empty_inventory_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "empty"):
            self.verify([], [])

    def test_duplicates_are_rejected(self):
        names = sorted(REQUIRED_TESTS)
        for declared, executed in [(names + names[:1], names), (names, names + names[:1])]:
            with self.subTest(declared=len(declared), executed=len(executed)):
                with self.assertRaisesRegex(RuntimeError, "duplicate"):
                    self.verify(declared, executed)

    def test_failed_or_skipped_cases_are_rejected(self):
        names = sorted(REQUIRED_TESTS)
        for status in ("skipped", "failure", "error"):
            with self.subTest(status=status):
                with self.assertRaisesRegex(RuntimeError, "failed or was skipped"):
                    self.verify(names, names, status)

    def test_unused_mapping_requires_exact_live_count(self):
        diagnostic = "warning: 1 functions have mismatched data\nhash-mismatch: No profile record found for 'f' with hash = 0x0\n"
        profile = "Counters:\n  f:\n    Hash: 0x18\n    Function count: 7\n"
        data = {"functions": [{"name": "f", "count": 7}]}
        self.assertEqual(validate_mapping(data, diagnostic, profile)["status"], "validated")
        for bad_data, bad_diagnostic, bad_profile in [
                ({"functions": [{"name": "f", "count": 6}]}, diagnostic, profile),
                (data, diagnostic.replace("hash = 0x0", "hash = 0x19"), profile),
                (data, diagnostic, ""),
                (data, diagnostic + "warning: stale profile\n", profile)]:
            with self.assertRaises(RuntimeError):
                validate_mapping(bad_data, bad_diagnostic, bad_profile)

    def test_coverage_policy_rejects_stale_incomplete_and_regressed_data(self):
        with tempfile.TemporaryDirectory(prefix="coverage-policy-") as temp:
            source = pathlib.Path(temp)
            (source / "src").mkdir()
            (source / "src/Core.cpp").write_text("int f() { return 1; }\n")
            names = sorted(REQUIRED_TESTS)
            metrics = {key: {"count": 100, "covered": 100, "percent": 100.0}
                       for key in ("lines", "functions", "branches")}
            summary = {"modules": [{"path": "src/Core.cpp", "coverage": copy.deepcopy(metrics)}],
                "totals": metrics, "first_party_source_sha256": source_digest(source, first_party_files(source)),
                "scope": ["src/*.cpp", "include/JoSIM/*.hpp"], "mapping_diagnostics": "validated",
                "tests": {"status": "passed", "exit": 0, "entries": len(names), "declared": names, "executed": names},
                "tools": {"clang++": {"version": ["clang version 23.1.4"]}}}
            reference = {key: 100 for key in metrics}
            policy = {"scope": summary["scope"], "overall_minimum_percent": {"lines": 75, "functions": 80, "branches": 60},
                "critical_modules": ["src/Core.cpp"], "critical_minimum_percent": {"lines": 80, "branches": 65},
                "variation_pp": {"overall": .5, "module": 2},
                "references": {"23": {"overall": reference, "modules": {"src/Core.cpp": reference}}}}
            self.assertEqual(evaluate(summary, policy, source)["status"], "passed")
            for corruption in ("stale", "missing", "tests", "mapping", "regression", "unknown compiler"):
                damaged = copy.deepcopy(summary)
                if corruption == "stale":
                    damaged["first_party_source_sha256"] = "0" * 64
                elif corruption == "missing":
                    damaged["modules"] = []
                elif corruption == "tests":
                    damaged["tests"]["status"] = "failed"
                elif corruption == "mapping":
                    damaged["mapping_diagnostics"] = "unreviewed"
                elif corruption == "regression":
                    for counter in [damaged["totals"]["lines"], damaged["modules"][0]["coverage"]["lines"]]:
                        counter.update(covered=90, percent=90.0)
                else:
                    damaged["tools"]["clang++"]["version"] = ["clang version 18.1.8"]
                with self.subTest(corruption=corruption), self.assertRaises(RuntimeError):
                    evaluate(damaged, policy, source)


if __name__ == "__main__":
    unittest.main()
