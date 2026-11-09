"""Retain complete run evidence and build a timeline from actual gameplay events."""
import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_root", type=Path)
    parser.add_argument("inspector", type=Path)
    parser.add_argument("--output", type=Path, default=HERE / "data")
    args = parser.parse_args()
    rows = []
    for directory in sorted(args.run_root.glob("run-*")):
        metadata = json.loads((directory / "execution.json").read_text())
        if "returncode" not in metadata:
            raise SystemExit(f"Still running: {directory.name}")
        for relative, expected in metadata["source_manifest"].items():
            assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == expected, relative
        assert hashlib.sha256((ROOT / "tests/test_hidden_journey.c").read_bytes()).hexdigest() == metadata["driver_sha256"]
        raw = (directory / "events.jsonl").read_bytes()
        events = [json.loads(line) for line in raw.splitlines()]
        start = next(e for e in events if e["event"] == "start")
        end = next((e for e in events if e["event"] == "end"), None)
        destination = args.output / directory.name
        destination.mkdir(parents=True, exist_ok=True)
        (destination / "events.jsonl.gz").write_bytes(gzip.compress(raw, mtime=0))
        for name in ("execution.json", "stderr.log", "final.sav", "extraction.sav"):
            if (directory / name).is_file():
                shutil.copyfile(directory / name, destination / name)
        narrative = None
        if (directory / "final.sav").is_file():
            narrative = json.loads(subprocess.check_output(
                [str(args.inspector.resolve()), str(directory / "final.sav")], text=True))
            write_json(destination / "narrative.json", narrative)
        timeline = []
        story = guardians = 0
        for event in events:
            kind = event["event"]
            if kind == "story":
                for scene in (narrative or {}).get("scenes", []):
                    bit = 1 << scene["id"]
                    if event["done"] & bit and not story & bit:
                        timeline.append({"t": event["t"], "event": "story_completed",
                                         "scene": scene["id"], "title": scene["title"],
                                         "entry": scene["entry"]})
                story = event["done"]
            elif kind == "guardian_victory":
                for guardian in (narrative or {}).get("guardians", []):
                    bit = 1 << guardian["id"]
                    if event["defeated"] & bit and not guardians & bit:
                        timeline.append({"t": event["t"], "event": kind,
                                         "guardian": guardian["id"], "name": guardian["name"]})
                guardians = event["defeated"]
            elif kind in ("decision", "fight_start", "fight_end", "detour_claim",
                          "abandon_detour", "shelter_enter", "shelter_exit", "respawn", "resume"):
                timeline.append(event)
        write_json(destination / "timeline.json", timeline)
        cells = [e for e in events if e["event"] == "cell"]
        rooms = {tuple(e["owner"]): e for e in cells if e["cell"][2] >= 0}
        themes = Counter(e["theme"] for e in rooms.values())
        samples = [e for e in events if e["event"] == "sample"]
        final = end or (samples[-1] if samples else {})
        complete = bool(end and end["complete"] and not metadata["returncode"])
        if complete:
            assert end["frames"] == metadata["requested_simulated_seconds"] * 60
            assert abs(end["t"] - end["frames"] / 60) < .02
            assert not end["world_failed"] and not end["save_failures"]
            assert all(e["ok"] for e in events if e["event"] in ("save", "resume"))
        row = {"run": directory.name, "complete": complete, "start": start, "end": final,
               "wall_seconds": metadata["wall_seconds"], "game_commit": metadata["game_commit"],
               "rooms_entered": len(rooms),
               "last_cell": samples[-1]["cell"] if samples else None,
               "min_deck": min(e["cell"][1] for e in cells),
               "max_deck": max(e["cell"][1] for e in cells),
               "room_themes": {str(k): v for k, v in themes.items()},
               "generated_atmospheres": dict(Counter(str(e["weather"]) for e in rooms.values())),
               "extraction_seconds": next((e["t"] for e in timeline
                                           if e["event"] == "story_completed" and e["scene"] == 15), None),
               "optional_victories": sum(e["event"] == "detour_claim" and e["kind"] == 1 for e in events),
               "simulation": next((e for e in events if e["event"] == "simulation_summary"), None),
               "checkpoint_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                     for p in sorted(directory.glob("*.sav"))},
               "events_sha256": hashlib.sha256(raw).hexdigest(),
               "ship": narrative["ship"] if narrative else None,
               "outcome": narrative["debrief"] if narrative else None}
        rows.append(row)
    assert rows, "No attempted runs found"
    write_json(args.output / "summary.json", rows)
    print(f"Retained {len(rows)} attempts; {sum(r['complete'] for r in rows)} completed.")


if __name__ == "__main__":
    main()
