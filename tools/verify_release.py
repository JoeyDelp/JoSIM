#!/usr/bin/env python3
"""Download, identify and execute actual public JoSIM release packages."""

import argparse
import datetime
import json
import os
import pathlib
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import urllib.parse
import urllib.request
import zipfile

from build_release import TARGETS, sha256, source_identity


API = "https://api.github.com/repos/JoeyDelp/JoSIM"
PUBLIC = "https://github.com/JoeyDelp/JoSIM"


def api(path):
    headers = {
        "User-Agent": "JoSIM-public-release-qualification",
        "Accept": "application/vnd.github+json", "X-GitHub-Api-Version": "2026-03-10"}
    if os.environ.get("JOSIM_GITHUB_API_TOKEN"):
        headers["Authorization"] = "Bearer " + os.environ["JOSIM_GITHUB_API_TOKEN"]
    request = urllib.request.Request(API + path, headers=headers)
    with urllib.request.urlopen(request, timeout=30) as stream:
        return json.load(stream)


def public_release(tag, revision):
    release = api("/releases/tags/" + urllib.parse.quote(tag, safe=""))
    if (release.get("tag_name") != tag or release.get("draft") is not False
            or release.get("prerelease") is not False or not release.get("published_at")):
        raise RuntimeError("Expected a published release for the requested tag")
    obj = api("/git/ref/tags/" + urllib.parse.quote(tag, safe=""))["object"]
    for _ in range(8):
        if obj["type"] != "tag":
            break
        obj = api("/git/tags/" + obj["sha"])["object"]
    if obj.get("type") != "commit" or obj.get("sha") != revision:
        raise RuntimeError("The public tag does not identify the expected source commit")
    return release


def relative_path(name):
    path = pathlib.PurePosixPath(name)
    if not name or "\\" in name or path.is_absolute() or ".." in path.parts or pathlib.PureWindowsPath(name).drive:
        raise RuntimeError("Unsafe package path: " + name)
    return path


def extract_archive(archive, destination):
    destination.mkdir()
    seen = set()

    def target(name, directory):
        path = relative_path(name)
        if path == pathlib.PurePosixPath(".") and directory:
            return destination
        if path.as_posix() in seen:
            raise RuntimeError("Duplicate package path: " + name)
        seen.add(path.as_posix())
        result = destination.joinpath(*path.parts)
        result.parent.mkdir(parents=True, exist_ok=True)
        return result

    if archive.suffix == ".zip":
        with zipfile.ZipFile(archive) as source:
            for entry in source.infolist():
                kind = (entry.external_attr >> 16) & 0o170000
                if kind not in (0, 0o100000, 0o040000):
                    raise RuntimeError("Package contains a link or special file")
                # ZipInfo normalizes backslashes on Windows; validate the stored name.
                path = target(entry.orig_filename, entry.is_dir())
                if entry.is_dir():
                    path.mkdir(exist_ok=True)
                else:
                    with source.open(entry) as incoming, path.open("xb") as outgoing:
                        shutil.copyfileobj(incoming, outgoing)
    else:
        with tarfile.open(archive) as source:
            for entry in source.getmembers():
                if not (entry.isfile() or entry.isdir()):
                    raise RuntimeError("Package contains a link or special file")
                path = target(entry.name, entry.isdir())
                if entry.isdir():
                    path.mkdir(exist_ok=True)
                else:
                    with source.extractfile(entry) as incoming, path.open("xb") as outgoing:
                        shutil.copyfileobj(incoming, outgoing)
                    path.chmod(entry.mode)


def verify_checksum(archive, sidecar):
    fields = sidecar.read_text(encoding="utf-8").split()
    if len(fields) != 2 or fields[1] != archive.name or fields[0] != sha256(archive):
        raise RuntimeError("Public archive checksum or filename differs")
    return fields[0]


def verify_payload(payload, manifest_bytes, tag, revision, target):
    if (payload / "MANIFEST.json").is_symlink():
        raise RuntimeError("Extracted manifest is a link")
    if (payload / "MANIFEST.json").read_bytes() != manifest_bytes:
        raise RuntimeError("Embedded and public sidecar manifests differ")
    manifest = json.loads(manifest_bytes)
    source = manifest.get("source", {})
    if (manifest.get("project") != "JoSIM" or manifest.get("version") != tag[1:]
            or manifest.get("target") != target or source.get("revision") != revision
            or source.get("dirty") is not False or manifest.get("tests", {}).get("status") != "passed"):
        raise RuntimeError("Package manifest does not match the clean source, target and version")
    expected = {}
    for item in manifest["files"]:
        name = relative_path(item["path"]).as_posix()
        if name in expected or name == "MANIFEST.json":
            raise RuntimeError("Duplicate or recursive manifest file")
        expected[name] = item
    actual = {}
    for path in payload.rglob("*"):
        if path.is_symlink():
            raise RuntimeError("Extracted payload contains a link")
        if path.is_file():
            name = path.relative_to(payload).as_posix()
            if name != "MANIFEST.json":
                actual[name] = path
    if not expected or set(expected) != set(actual):
        raise RuntimeError("Manifest and actual payload inventories differ")
    for name, path in actual.items():
        if name.lower().startswith("docs/context/"):
            raise RuntimeError("Package contains local project context")
        if path.stat().st_size != expected[name]["size"] or sha256(path) != expected[name]["sha256"]:
            raise RuntimeError("Downloaded payload differs: " + name)
    return manifest


def prepare(work, tag, revision, target):
    if work.exists() and any(work.iterdir()):
        raise RuntimeError("Use a fresh work directory for a new public download")
    work.mkdir(parents=True, exist_ok=True)
    release = public_release(tag, revision)
    name = "JoSIM-" + tag + "-" + target
    archive_name = name + (".zip" if target == "windows-x64" else ".tar.gz")
    names = [archive_name, archive_name + ".sha256", name + ".manifest.json"]
    downloads = []
    for filename in names:
        matches = [asset for asset in release["assets"] if asset["name"] == filename]
        if len(matches) != 1 or matches[0].get("state") != "uploaded":
            raise RuntimeError("Missing or ambiguous public asset: " + filename)
        asset = matches[0]
        url = PUBLIC + "/releases/download/" + urllib.parse.quote(tag, safe="") + "/" + filename
        if asset["browser_download_url"] != url:
            raise RuntimeError("Unexpected public download destination")
        request = urllib.request.Request(url, headers={"User-Agent": "JoSIM-public-release-qualification"})
        path = work / filename
        with urllib.request.urlopen(request, timeout=30) as incoming, path.open("xb") as outgoing:
            shutil.copyfileobj(incoming, outgoing)
        digest = sha256(path)
        if path.stat().st_size != asset["size"] or (asset.get("digest") and asset["digest"] != "sha256:" + digest):
            raise RuntimeError("Public API asset identity differs: " + filename)
        downloads.append({"name": filename, "url": url, "size": path.stat().st_size,
                          "sha256": digest, "api_digest": asset.get("digest")})
    archive = work / archive_name
    digest = verify_checksum(archive, work / (archive_name + ".sha256"))
    payload = work / "payload"
    extract_archive(archive, payload)
    verify_payload(payload, (work / (name + ".manifest.json")).read_bytes(), tag, revision, target)
    result = {"status": "prepared", "tag": tag, "revision": revision, "target": target,
              "release_url": release["html_url"], "release_id": release["id"],
              "published_at": release["published_at"],
              "prepared_at_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "archive": archive_name, "archive_sha256": digest,
              "manifest": name + ".manifest.json", "downloads": downloads}
    (work / "prepared.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


def execute(work, result, output):
    target = result["target"]
    machine = platform.machine().lower()
    expected = ("x86_64", "amd64") if "x64" in target or "x86_64" in target else ("arm64", "aarch64")
    if platform.system() != TARGETS[target]["system"] or machine not in expected:
        raise RuntimeError("Execution host does not match the requested native target")
    if sha256(work / result["archive"]) != result["archive_sha256"]:
        raise RuntimeError("Prepared archive changed before native execution")
    payload = work / "payload"
    verify_payload(payload, (work / result["manifest"]).read_bytes(), result["tag"], result["revision"], target)
    binary = payload / "bin" / ("josim-cli.exe" if target == "windows-x64" else "josim-cli")
    binary_digest = sha256(binary)
    root = pathlib.Path(__file__).resolve().parent.parent
    result["verification_source"] = source_identity(root, None)
    result["owners_sha256"] = {name: sha256(root / name) for name in
                              ("tools/verify_release.py", "test/regression_cli.py", "test/regression_numerics.py")}
    output.parent.mkdir(parents=True, exist_ok=True)
    commands = [("version", [str(binary), "-v"]),
                ("cli", [sys.executable, "-B", str(root / "test/regression_cli.py"), str(binary)]),
                ("numerics", [sys.executable, "-B", str(root / "test/regression_numerics.py"), str(binary),
                              "--report", str(output.parent / "numerics.json")])]
    result["commands"] = []
    result["runtime"] = {"system": platform.system(), "machine": platform.machine(),
                         "release": platform.release(), "libc": platform.libc_ver(), "python": sys.version}
    for label, command in commands:
        log = output.parent / (label + ".log")
        with log.open("w", encoding="utf-8") as stream:
            run = subprocess.run(command, cwd=work, stdout=stream, stderr=subprocess.STDOUT, timeout=180)
        result["commands"].append({"name": label, "command": command, "exit": run.returncode, "log": str(log)})
        if run.returncode:
            raise RuntimeError("Downloaded executable failed " + label + "; see " + str(log))
        if label == "version" and "v" + result["tag"][1:] + "." + result["revision"][:7] not in log.read_text():
            raise RuntimeError("Downloaded executable version/source banner differs")
        if label != "version":
            text = log.read_text(encoding="utf-8")
            count = re.search(r"Ran (\d+) tests? in", text)
            if not count or int(count.group(1)) == 0 or re.search(r"\bskipped[= ]", text):
                raise RuntimeError("Downloaded executable test run is empty or skipped")
            result["commands"][-1]["test_methods"] = int(count.group(1))
    numerics = json.loads((output.parent / "numerics.json").read_text(encoding="utf-8"))
    if numerics.get("passed") is not True or numerics.get("binary_sha256") != sha256(binary) or not numerics.get("cases"):
        raise RuntimeError("Numerical report is incomplete or identifies another binary")
    result["numerical_cases"] = len(numerics["cases"])
    if source_identity(root, None) != result["verification_source"] or sha256(binary) != binary_digest:
        raise RuntimeError("Verification source or downloaded executable changed during execution")
    result["binary_sha256"] = binary_digest
    result["status"] = "passed"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--target", choices=sorted(TARGETS), required=True)
    parser.add_argument("--work-dir", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--download-only", action="store_true")
    mode.add_argument("--execute-only", action="store_true")
    args = parser.parse_args()
    if not re.fullmatch(r"v\d+\.\d+(?:\.\d+)?", args.tag) or not re.fullmatch(r"[0-9a-f]{40}", args.revision):
        parser.error("Use an existing v<version> tag and its full lowercase source revision")
    work, output = args.work_dir.resolve(), args.output.resolve()
    result = {"status": "running", "tag": args.tag, "revision": args.revision, "target": args.target,
              "started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat()}
    try:
        if args.execute_only:
            result.update(json.loads((work / "prepared.json").read_text(encoding="utf-8")))
            if (result.get("status") != "prepared" or result.get("tag") != args.tag
                    or result.get("revision") != args.revision or result.get("target") != args.target):
                raise RuntimeError("Prepared download identity differs from the requested execution")
        else:
            result.update(prepare(work, args.tag, args.revision, args.target))
        if not args.download_only:
            execute(work, result, output)
    except (OSError, ValueError, RuntimeError, KeyError, tarfile.TarError,
            zipfile.BadZipFile, EOFError, subprocess.TimeoutExpired) as error:
        result.update(status="failed", error=str(error))
        print(str(error), file=sys.stderr)
    result["finished_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    output.parent.mkdir(parents=True, exist_ok=True)
    encoded = json.dumps(result, indent=2) + "\n"
    output.write_text(encoded, encoding="utf-8")
    summary = "# Public release verification\n\nTarget: `{}`. Status: **{}**.\n\n".format(args.target, result["status"])
    if result["status"] == "prepared":
        summary += "Download identified; native execution has not run.\n\n"
    summary += "<details><summary>Test details (JSON)</summary>\n\n```json\n" + encoded + "```\n</details>\n"
    output.with_suffix(".md").write_text(summary, encoding="utf-8")
    print("Public release verification:", result["status"])
    raise SystemExit(0 if result["status"] in ("prepared", "passed") else 1)


if __name__ == "__main__":
    main()
