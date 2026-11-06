#!/usr/bin/env python3
"""Build, run unit tests, and enforce 95% core line coverage per file and overall."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
THRESHOLD = 95


def run(args):
    print("+ " + " ".join(str(arg) for arg in args), flush=True)
    return subprocess.run([str(arg) for arg in args], cwd=ROOT, check=True)


def validate_report(data):
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
    required = {"src/game.c", "src/generation.c", "src/history.c", "src/interior.c", "src/world.c", "src/traversal.c", "src/exterior.c", "src/progression.c", "src/story.c", "src/expedition.c", "src/death.c", "src/save.c", "src/shelter.c", "src/navigation.c", "src/psionics.c", "src/equipment.c", "src/matrix.c", "src/matrix_gen.c", "src/matrix_state.c", "src/matrix_effects.c", "src/campaign.c", "src/campaign_gen.c", "src/campaign_combat.c", "src/math3d.h"}
    missing = required - names
    if missing or failed:
        raise SystemExit(f"Coverage gate failed. Below {THRESHOLD}%: {failed}; missing sources: {sorted(missing)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build-coverage")
    args = parser.parse_args()
    build = args.build_dir.resolve()
    run(["cmake", "-S", ROOT, "-B", build, "-DSC_BUILD_GAME=OFF", "-DSC_COVERAGE=ON", "-DCMAKE_BUILD_TYPE=Debug"])
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
    validate_report(data)
    print(f"PASS: >= {THRESHOLD}% unit line coverage overall and in each measured source. Report: {report / 'index.html'}")


if __name__ == "__main__":
    main()
