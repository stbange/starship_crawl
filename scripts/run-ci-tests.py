#!/usr/bin/env python3
"""Run exhaustive, disjoint CI groups without overlapping browser/GPU processes."""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / "tests/ci-suites.json"
REMAINDER = "core-and-revisits"


def validate_partition(groups, inventory):
    if not groups or any(not re.fullmatch(r"[a-z][a-z0-9-]*", name) for name in groups):
        raise ValueError("Invalid CI group names")
    counts = Counter(name for group in groups.values() for name in group)
    duplicates = sorted(name for name, count in counts.items() if count != 1)
    missing = sorted(set(inventory) - counts.keys())
    unknown = sorted(counts.keys() - set(inventory))
    empty = sorted(name for name, group in groups.items() if not group)
    if duplicates or missing or unknown or empty:
        raise ValueError(f"Invalid CI partition: duplicates={duplicates}, missing={missing}, "
                         f"unknown={unknown}, empty={empty}")
    return groups


def browser_groups(config, inventory):
    # Quest has its own build and optional browser checks with a different launch path.
    ordinary = set(inventory) - {"test_quest_browser.py"}
    if "test_quest_browser.py" not in inventory:
        raise ValueError("The separately tested Quest suite is missing")
    return validate_partition(config["browser"], ordinary)


def ctest_groups(config, tests, label=None):
    names = []
    for test in tests:
        labels = next((p["value"] for p in test.get("properties", []) if p["name"] == "LABELS"), [])
        if label is None or label in labels:
            names.append(test["name"])
    if len(names) != len(set(names)):
        raise ValueError("Duplicate CTest names")
    groups = {name: list(items) for name, items in config["instrumented"].items()}
    if REMAINDER not in groups or groups[REMAINDER]:
        raise ValueError("The remainder group must exist and start empty")
    assigned = {name for items in groups.values() for name in items}
    groups[REMAINDER] = sorted(set(names) - assigned)
    return validate_partition(groups, names)


def report_failures(report):
    if not report.is_file() or os.environ.get("GITHUB_ACTIONS") != "true":
        return
    for case in ET.parse(report).iter("testcase"):
        if case.find("failure") is None and case.find("error") is None:
            continue
        detail = case.get("name", "CTest") + "\n" + (case.findtext("system-out") or "")[-9000:]
        detail = detail.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
        print("::error title=Instrumented regression::" + detail, flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kind", choices=("browser", "ctest"))
    parser.add_argument("--shard")
    output = parser.add_mutually_exclusive_group()
    output.add_argument("--check", action="store_true", help="Validate and list every group without running tests")
    output.add_argument("--matrix", action="store_true", help="Emit the validated group names for GITHUB_OUTPUT")
    parser.add_argument("--site-dir", type=Path, default=ROOT / "build-pages-preview")
    parser.add_argument("--url-path", default="/starship_crawl/")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--label", choices=("gameplay",))
    args = parser.parse_args()
    config = json.loads(CONFIG.read_text())
    if args.kind == "browser":
        groups = browser_groups(config, {p.name for p in (ROOT / "tests").glob("test_*browser.py")})
    else:
        if not args.build_dir:
            parser.error("ctest requires --build-dir")
        args.build_dir = args.build_dir.resolve()
        inventory = json.loads(subprocess.check_output(
            ["ctest", "--test-dir", str(args.build_dir), "--show-only=json-v1"], text=True))
        groups = ctest_groups(config, inventory["tests"], args.label)
    if args.check:
        print(json.dumps(groups, indent=2))
        return 0
    if args.matrix:
        print(("suites" if args.kind == "browser" else "shards") + "=" + json.dumps(list(groups)))
        return 0
    if args.shard not in groups:
        parser.error("--shard must name a group in tests/ci-suites.json")
    selected = groups[args.shard]
    print(f"{args.kind} / {args.shard}: {len(selected)} tests: " + ", ".join(selected), flush=True)
    if args.kind == "browser":
        failed = []
        for name in selected:
            result = subprocess.run([sys.executable, str(ROOT / "scripts/run-browser-test.py"),
                                     str(ROOT / "tests" / name), "--site-dir", str(args.site_dir),
                                     "--url-path", args.url_path], cwd=ROOT)
            if result.returncode:
                failed.append(name)
        if failed:
            print("FAILED browser suites: " + ", ".join(failed), flush=True)
        return int(bool(failed))
    # Keep each runner serial: several existing journeys use the same save filename.
    # Parallelism is between isolated Actions jobs, including the six long cases.
    regex = "^(" + "|".join(re.escape(name) for name in selected) + ")$"
    result = subprocess.run(["ctest", "--test-dir", str(args.build_dir), "-R", regex,
                             "--no-tests=error", "--output-on-failure", "--output-junit", "shard-tests.xml"])
    report_failures(args.build_dir / "shard-tests.xml")
    return int(bool(result.returncode))


if __name__ == "__main__":
    raise SystemExit(main())
