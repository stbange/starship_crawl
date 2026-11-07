"""Run independent, fresh-character expeditions and retain exact provenance."""
import argparse
import concurrent.futures
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=Path("/tmp/sc-long-review"))
    parser.add_argument("--runs", type=int, nargs="+", default=list(range(12)))
    parser.add_argument("--seconds", type=int, default=3600)
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--output", type=Path, default=HERE)
    args = parser.parse_args()
    binary = args.binary.resolve()
    common = {
        "game_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "binary_sha256": digest(binary),
        "driver_sha256": digest(HERE / "review-driver.c"),
        "library_sha256": digest(ROOT / "build/libsc_core.a"),
        "simulation_hz": 60,
        "requested_simulated_seconds": args.seconds,
        "parallel_processes": args.jobs,
    }

    def run(number):
        directory = args.output.resolve() / f"run-{number:02}"
        directory.mkdir(parents=True, exist_ok=False)
        command = [str(binary), str(number), str(directory), str(args.seconds)]
        started = time.monotonic()
        metadata = dict(common, command=command, started_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
        print(f"Starting run {number:02}", flush=True)
        with (directory / "events.jsonl").open("w") as out, (directory / "stderr.log").open("w") as err:
            result = subprocess.run(command, stdout=out, stderr=err, cwd=ROOT)
        metadata.update(returncode=result.returncode, wall_seconds=round(time.monotonic() - started, 3))
        (directory / "execution.json").write_text(json.dumps(metadata, indent=2) + "\n")
        print(f"Finished run {number:02}: exit {result.returncode}, {metadata['wall_seconds']:.1f}s wall", flush=True)
        return result.returncode

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        failures = list(pool.map(run, args.runs))
    raise SystemExit(any(failures))


if __name__ == "__main__":
    main()
