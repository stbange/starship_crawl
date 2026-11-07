#!/usr/bin/env python3
"""Retain browser diagnostics and expose failures in the Actions summary."""
import os
from pathlib import Path
import subprocess
import sys

if len(sys.argv) < 2:
    raise SystemExit("Usage: run-browser-test.py tests/test_NAME_browser.py [arguments]")

test = Path(sys.argv[1])
result = subprocess.run([sys.executable, *sys.argv[1:]], stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT, text=True, errors="replace")
print(result.stdout, end="", flush=True)
artifacts = Path("build-web/test-artifacts")
artifacts.mkdir(parents=True, exist_ok=True)
(artifacts / (test.stem + ".log")).write_text(result.stdout)
if result.returncode and os.environ.get("GITHUB_ACTIONS") == "true":
    detail = result.stdout[-10000:].replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
    print(f"::error title=Browser regression ({test.name})::{detail}", flush=True)
raise SystemExit(result.returncode)
