"""One additional hour: identical seed/route/build, only recovery inputs differ."""
import hashlib
import json
from pathlib import Path
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
original = (HERE / "review-driver.c").read_text()
old = "actions[0].strafe = m.recoveries % 2 ? 1 : -1;\n            actions[0].forward = .3f;"
new = "actions[0].strafe = recovery_ticks > 60 ? 0 : (m.recoveries % 2 ? 1 : -1);\n            actions[0].forward = recovery_ticks > 60 ? -1 : 0;"
assert original.count(old) == 1
source = original.replace(old, new)
temporary = Path("/tmp/sc-review-recovery-comparison.c")
temporary.write_text(source)
binary = temporary.with_suffix("")
subprocess.run(["cc", "-std=c17", "-O2", "-Wall", "-Wextra", "-Werror", "-Isrc",
                str(temporary), "build/libsc_core.a", "-lm", "-o", str(binary)], cwd=ROOT, check=True)
output = HERE / "comparison-09"
output.mkdir(exist_ok=False)
before = time.monotonic()
with (output / "events.jsonl").open("w") as out, (output / "stderr.log").open("w") as err:
    result = subprocess.run([str(binary), "9", str(output), "3600"], cwd=ROOT, stdout=out, stderr=err)
expected = [json.loads(x) for x in (HERE / "run-09/events.jsonl").read_text().splitlines()]
actual = [json.loads(x) for x in (output / "events.jsonl").read_text().splitlines()]
first_recovery = next(x["t"] for x in expected if x["event"] == "recovery")
prefix = lambda rows: [x for x in rows if x["event"] in ("cell", "sample", "recovery") and x["t"] <= first_recovery]
assert prefix(actual) == prefix(expected), "Comparison diverged before the changed recovery input"
(output / "execution.json").write_text(json.dumps(dict(
    purpose="13th hour; paired recovery-controller comparison, retained separately from primary 12",
    modification="two seconds of recovery: first back away, then strafe; normal controls only",
    game_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
    original_sha256=hashlib.sha256(original.encode()).hexdigest(),
    variant_sha256=hashlib.sha256(source.encode()).hexdigest(),
    binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
    identical_prefix_observations=len(prefix(actual)), first_changed_input_seconds=first_recovery,
    run=9, requested_simulated_seconds=3600, returncode=result.returncode,
    wall_seconds=round(time.monotonic() - before, 3)), indent=2) + "\n")
print(f"Recovery comparison finished: exit {result.returncode}", flush=True)
raise SystemExit(result.returncode)
