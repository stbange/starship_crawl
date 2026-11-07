"""Replay a prefix unchanged, adding only a save at the first controller recovery."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=int)
    parser.add_argument("seconds", type=int)
    args = parser.parse_args()
    original = (HERE / "review-driver.c").read_text()
    marker = "m.recoveries++;"
    assert original.count(marker) == 1
    source = original.replace(marker, marker + '\n            if (m.recoveries == 1)\n'
                              '                save_at(g, directory, "first-recovery", &m);')
    temporary = Path("/tmp") / f"sc-recovery-{args.run}.c"
    temporary.write_text(source)
    binary = temporary.with_suffix("")
    subprocess.run(["cc", "-std=c17", "-O2", "-Wall", "-Wextra", "-Werror", "-Isrc",
                    str(temporary), "build/libsc_core.a", "-lm", "-o", str(binary)], cwd=ROOT, check=True)
    output = HERE / f"probe-{args.run:02}"
    output.mkdir(exist_ok=False)
    before = time.monotonic()
    with (output / "events.jsonl").open("w") as out, (output / "stderr.log").open("w") as err:
        result = subprocess.run([str(binary), str(args.run), str(output), str(args.seconds)],
                                cwd=ROOT, stdout=out, stderr=err)
    (output / "execution.json").write_text(json.dumps(dict(
        purpose="diagnostic prefix replay, excluded from the 12 long-run totals",
        modification="save_at at the first recovery; no changes to actions or simulation",
        driver_sha256=hashlib.sha256(original.encode()).hexdigest(),
        instrumented_sha256=hashlib.sha256(source.encode()).hexdigest(),
        run=args.run, seconds=args.seconds, returncode=result.returncode,
        wall_seconds=time.monotonic() - before), indent=2) + "\n")
    expected = [json.loads(x) for x in (HERE / f"run-{args.run:02}" / "events.jsonl").read_text().splitlines()]
    actual = [json.loads(x) for x in (output / "events.jsonl").read_text().splitlines()]
    # Exact path/combat checkpoints establish that the extra save is observational.
    expected = [x for x in expected if x["event"] in ("cell", "sample", "recovery") and x["t"] < args.seconds - 1]
    actual = [x for x in actual if x["event"] in ("cell", "sample", "recovery") and x["t"] < args.seconds - 1]
    assert actual == expected, "Diagnostic replay diverged from the reviewed run"
    print(f"Probe {args.run}: {len(actual)} identical path/combat observations", flush=True)


if __name__ == "__main__":
    main()
