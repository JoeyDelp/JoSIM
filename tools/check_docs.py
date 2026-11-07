#!/usr/bin/env python3
"""Check local files and fragment targets in a built MkDocs site."""

import argparse
import json
import pathlib
from html.parser import HTMLParser
from urllib.parse import unquote, urlsplit


class Page(HTMLParser):
    def __init__(self, path):
        super().__init__(convert_charrefs=True)
        self.ids = set()
        self.links = []
        self.feed(path.read_text(encoding="utf-8"))

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if "id" in attrs:
            self.ids.add(attrs["id"])
        if tag == "a" and "name" in attrs:
            self.ids.add(attrs["name"])
        for field in ("href", "src"):
            if attrs.get(field):
                self.links.append(attrs[field])


def check_site(site, base_path):
    pages = {path.resolve(): Page(path) for path in site.rglob("*.html")}
    if not pages or not (site / "index.html").is_file():
        raise RuntimeError("The site has no homepage or HTML pages")
    if (site / "context").exists():
        raise RuntimeError("Ignored local docs/context must never be published")
    failures = []
    checked = 0
    external = set()
    for path, page in pages.items():
        for link in page.links:
            url = urlsplit(link)
            if url.netloc or url.scheme:
                external.add(link)
                continue
            name = unquote(url.path)
            if name.startswith("/"):
                if not name.startswith(base_path):
                    failures.append([path.relative_to(site).as_posix(), link, "outside site base path"])
                    continue
                target = site / name[len(base_path):]
            else:
                target = path.parent / name if name else path
            target = target.resolve()
            if target.is_dir():
                target /= "index.html"
            checked += 1
            if not target.is_relative_to(site) or not target.is_file():
                reason = "missing local file"
            elif url.fragment and target in pages and unquote(url.fragment) not in pages[target].ids:
                reason = "missing fragment"
            else:
                continue
            failures.append([path.relative_to(site).as_posix(), link, reason])
    return {"pages": len(pages), "local_links": checked, "external_links": sorted(external),
            "failures": failures, "passed": not failures}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--site-dir", type=pathlib.Path, required=True)
    parser.add_argument("--base-path", default="/JoSIM/")
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    if not args.base_path.startswith("/") or not args.base_path.endswith("/"):
        parser.error("--base-path must start and end with /")
    result = check_site(args.site_dir.resolve(), args.base_path)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("{} pages, {} local links, {} failures".format(
        result["pages"], result["local_links"], len(result["failures"])))
    raise SystemExit(0 if result["passed"] else 1)


if __name__ == "__main__":
    main()
