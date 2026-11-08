#!/usr/bin/env python3
"""Retain browser diagnostics and expose failures in the Actions summary."""
import json
import os
from pathlib import Path
import subprocess
import sys
import time

if len(sys.argv) < 2:
    raise SystemExit("Usage: run-browser-test.py tests/test_NAME_browser.py [arguments]")

test = Path(sys.argv[1])
artifacts = Path("build-web/test-artifacts")
artifacts.mkdir(parents=True, exist_ok=True)
started = time.monotonic()
tail = ""
print(f"RUN {test.name}", flush=True)
with (artifacts / (test.stem + ".log")).open("w") as log:
    result = subprocess.Popen([sys.executable, "-u", *sys.argv[1:]], stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True, errors="replace", bufsize=1)
    for line in result.stdout:
        print(line, end="", flush=True)
        log.write(line)
        log.flush()
        tail = (tail + line)[-10000:]
    result.wait()
elapsed = time.monotonic() - started
(artifacts / (test.stem + "-result.json")).write_text(
    json.dumps({"test": test.name, "exit_code": result.returncode,
                "seconds": round(elapsed, 3)}, indent=2) + "\n")
print(f"{'FAIL' if result.returncode else 'PASS'} {test.name} ({elapsed:.1f}s)", flush=True)
if result.returncode and os.environ.get("GITHUB_ACTIONS") == "true":
    detail = tail.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
    print(f"::error title=Browser regression ({test.name})::{detail}", flush=True)
raise SystemExit(result.returncode)
