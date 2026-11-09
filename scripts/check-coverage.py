#!/usr/bin/env python3
"""Build, run unit tests, and enforce 95% core line coverage per file and overall."""
import argparse
import json
import re
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
THRESHOLD = 95


def run(args):
    print("+ " + " ".join(str(arg) for arg in args), flush=True)
    return subprocess.run([str(arg) for arg in args], cwd=ROOT, check=True)


def required_sources():
    manifest = (ROOT / "cmake" / "CoreSources.cmake").read_text()
    sources = set(re.findall(r"src/[\w.]+\.c", manifest.split("set(SC_CORE_DATA_SOURCES")[0]))
    data = set(re.findall(r"src/[\w.]+\.c", manifest.split("set(SC_CORE_DATA_SOURCES")[1]))
    if not sources or not data <= sources:
        raise SystemExit("Invalid core source manifest")
    # Data tables have no executable lines. Any reported lines are still measured.
    for name in data:
        source = re.sub(r"(?m)^\s*#.*$", "", (ROOT / name).read_text())
        if re.search(r"\)\s*\{", source):
            raise SystemExit("Executable code added to data-only source: " + name)
    return (sources - data) | {"src/math3d.h", "src/world.h"}


def validate_report(data, require_quest=False):
    failed = []
    names = set()
    for source in data["files"]:
        lines = source["lines"]
        if not lines:
            continue
        covered = sum(line["count"] > 0 for line in lines)
        percent = 100 * covered / len(lines)
        names.add(source["file"])
        print(f"{source['file']}: {covered}/{len(lines)} lines ({percent:.2f}%)")
        if covered * 100 < THRESHOLD * len(lines):
            failed.append(source["file"])
    # Do not accidentally pass with an empty, stale, or incorrectly filtered report.
    required = required_sources()
    if require_quest:
        required.add("src/quest.c")
    missing = required - names
    if missing or failed:
        raise SystemExit(f"Coverage gate failed. Below {THRESHOLD}%: {failed}; missing sources: {sorted(missing)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build-coverage")
    parser.add_argument("--quest", action="store_true", help="Also instrument and require the isolated Quest adapter/core tests")
    parser.add_argument("--split-campaigns", action="store_true", help="Register the same campaign cases separately for subsequent CI jobs")
    args = parser.parse_args()
    build = args.build_dir.resolve()
    run(["cmake", "-S", ROOT, "-B", build, "-DSC_BUILD_GAME=OFF", "-DSC_COVERAGE=ON", "-DCMAKE_BUILD_TYPE=Debug",
         "-DSC_SPLIT_CAMPAIGN_TESTS=" + ("ON" if args.split_campaigns else "OFF")] +
        (["-DSC_QUEST_TESTS=ON"] if args.quest else []))
    run(["cmake", "--build", build, "--parallel", "4"])
    # Reports describe this unit run alone, not accumulated campaign/browser work.
    for counter in build.rglob("*.gcda"):
        counter.unlink()
    run(["ctest", "--test-dir", build, "-L", "unit", "--output-on-failure"])
    report = build / "coverage"
    report.mkdir(exist_ok=True)
    run([sys.executable, "-m", "gcovr", "--root", ROOT, "--filter", "src/", "--object-directory", build,
         "--txt", report / "summary.txt", "--html-details", report / "index.html",
         "--json", report / "coverage.json", "--json-summary", report / "summary.json",
         "--cobertura", report / "coverage.xml", "--print-summary", "--fail-under-line", str(THRESHOLD), build])
    data = json.loads((report / "coverage.json").read_text())
    validate_report(data, require_quest=args.quest)
    print(f"PASS: >= {THRESHOLD}% unit line coverage overall and in each measured source. Report: {report / 'index.html'}")


if __name__ == "__main__":
    main()
