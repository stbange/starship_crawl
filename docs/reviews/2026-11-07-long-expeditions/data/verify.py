"""Check the retained report's counts, links and checkpoint/screenshot provenance."""
import hashlib
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
REPORT = HERE.parent
REPO = HERE.parents[3]
runs = json.loads((HERE / "summary.json").read_text())
comparison = json.loads((HERE / "comparison-09/summary.json").read_text())
assert len(runs) == 12
assert len({r["start"]["seed"] for r in runs}) == 12
for r in runs + [comparison]:
    end = r["end"]
    assert end["frames"] == 216000 and end["complete"] and end["resumed"]
    assert r["execution"]["returncode"] == 0 and r["checkpoints_valid"]
    assert end["phase"] == 4 and r["extraction"] and r["settlement"]
    assert r["final"]["countries"][(r["start"]["run"] // 3) % 3]["stage"] == 3
    assert not end["world_failed"] and end["save_failures"] == 0
assert sum(r["end"]["kills"] for r in runs) == 10910
assert sum(r["end"]["deaths"] for r in runs + [comparison]) == 0
assert sum(r["checkpoint_count"] for r in runs + [comparison]) == 291
assert sum(r["end"]["upgrades"] == 120 for r in runs) == 7
assert sum(r["end"]["frontier_claims"] for r in runs) == 118
assert sum(r["end"]["local_claims"] for r in runs) == 49
driver = hashlib.sha256((HERE / "review-driver.c").read_bytes()).hexdigest()
assert all(r["execution"]["driver_sha256"] == driver for r in runs)
assert comparison["execution"]["original_sha256"] == driver
assert comparison["execution"]["identical_prefix_observations"] == 201
shots = list((REPORT / "screenshots").glob("*.png"))
assert len(shots) == 14
for shot in shots:
    d = json.loads(shot.with_suffix(".json").read_text())
    assert not d["errors"] and not d["after"]["failed"]
    assert d["before"]["seed"] == d["native"]["seed"]
    assert d["before"]["rank"] == d["native"]["rank"]
    assert hashlib.sha256((REPO / d["source"]).read_bytes()).hexdigest() == d["save_sha256"]
for doc in (REPORT / "README.md", REPORT / "screenshots/README.md"):
    for link in re.findall(r"\]\(([^)]+)\)", doc.read_text()):
        if "://" not in link:
            assert (doc.parent / link).exists(), (doc, link)
print("PASS: 13 x 216000 simulation updates; 291 long-run checkpoint loads; "
      "14 WebGL images with matching earned-save provenance; all report links resolve.")
