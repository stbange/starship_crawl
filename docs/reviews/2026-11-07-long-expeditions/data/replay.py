"""Photograph earned saves in the real packaged WASM/WebGL game.

Optional camera changes use pointer-lock mouse input. No scene presets, state
injection, relocation, equipment grants or enemy deletion are used.
"""
import argparse
from functools import partial
import hashlib
from http.server import ThreadingHTTPServer
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import threading

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
sys.path.insert(0, str(ROOT / "tests"))
from playwright.sync_api import sync_playwright
from test_browser import QuietHandler, start_play, state, wait_ready
from test_story_browser import import_fixture
from test_interiors_browser import expect


def look(page, yaw, pitch):
    page.mouse.move(640, 360)
    cursor = [640, 360]
    for _ in range(12):
        s = state(page)
        dy = math.atan2(math.sin(yaw - s["yaw"]), math.cos(yaw - s["yaw"]))
        dp = pitch - s["pitch"]
        if abs(dy) < .015 and abs(dp) < .015:
            return
        cursor[0] += dy / .0023
        cursor[1] -= dp / .0023
        page.mouse.move(*cursor)
        expect(page, f"s.time > {s['time'] + .05}")
    raise AssertionError("Camera input did not reach requested view")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--site", type=Path, default=ROOT / "build-pages-courts-final")
    parser.add_argument("--shots", nargs="+", required=True,
                        help="run:save-tag[:yaw:pitch], radians; omit angles to retain saved view")
    parser.add_argument("--advance", type=float, default=0,
                        help="seconds of ordinary forward/backward walking before turning")
    parser.add_argument("--settle", type=float, default=4.2,
                        help="minimum simulation seconds after import, to let its notice fade")
    parser.add_argument("--focus", action="store_true", help="look toward the room's actual focal structure")
    parser.add_argument("--probe", action="store_true", help="use a diagnostic prefix checkpoint")
    parser.add_argument("--engage", action="store_true", help="press the normal USE key at an encounter seal")
    parser.add_argument("--enemy", action="store_true", help="look at the nearest living creature")
    args = parser.parse_args()
    output = HERE.parent / "screenshots"
    output.mkdir(exist_ok=True)
    site = args.site.resolve()
    server = ThreadingHTTPServer(("127.0.0.1", 0), partial(QuietHandler, directory=str(site)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    wasm_hash = hashlib.sha256((site / "starship_crawl/starship_crawl.wasm").read_bytes()).hexdigest()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, executable_path=os.environ.get("SC_BROWSER", "/usr/lib/chromium/chromium"),
                                        args=["--no-sandbox", "--use-angle=swiftshader", "--enable-unsafe-swiftshader"])
            for item in args.shots:
                parts = item.split(":")
                run, tag = int(parts[0]), parts[1]
                prefix = "probe" if args.probe else "run"
                source = HERE / f"{prefix}-{run:02}" / f"{tag}.sav"
                checked = subprocess.run(["/tmp/sc-review-inspect", str(source)], capture_output=True, text=True, check=True)
                native = json.loads(checked.stdout)
                context = browser.new_context(viewport={"width": 1280, "height": 720})
                page = context.new_page()
                errors = []
                page.on("pageerror", lambda error: errors.append(str(error)))
                page.on("dialog", lambda dialog: dialog.accept())
                page.goto(f"http://127.0.0.1:{server.server_port}/starship_crawl/?seed={native['seed']}")
                wait_ready(page)
                start_play(page)
                import_fixture(page, source)
                before = state(page)
                assert before["seed"] == native["seed"] and before["rank"] == native["rank"]
                assert not before["failed"] and before["generator"] == 13
                if args.engage:
                    page.keyboard.press("g")
                if args.advance:
                    key = "w" if args.advance > 0 else "s"
                    page.keyboard.down(key)
                    try:
                        expect(page, f"s.time > {before['time'] + abs(args.advance)}", timeout=60000)
                    finally:
                        page.keyboard.up(key)
                if args.enemy:
                    current = state(page)
                    living = [e for e in current["specialists"] if e["alive"]]
                    assert living, "No creature at this earned checkpoint"
                    at = min(living, key=lambda e: sum((e["at"][i] - current[k]) ** 2
                             for i, k in enumerate(("x", "y", "z"))))["at"]
                    dx, dy, dz = at[0] - current["x"], at[1] - current["y"] - 1.62, at[2] - current["z"]
                    look(page, math.atan2(dx, dz), math.atan2(dy, math.hypot(dx, dz)))
                elif len(parts) == 4:
                    look(page, float(parts[2]), float(parts[3]))
                elif args.focus:
                    current = state(page)
                    dx = native["focus"][0] - current["x"]
                    dy = native["focus"][1] - current["y"] - 1.62
                    dz = native["focus"][2] - current["z"]
                    look(page, math.atan2(dx, dz), math.atan2(dy, math.hypot(dx, dz)))
                expect(page, f"s.time > {before['time'] + args.settle}", timeout=60000)
                suffix = "-enemy" if args.enemy else "-look" if len(parts) == 4 or args.focus else ""
                target = output / f"{'probe-' if args.probe else ''}{run:02}-{tag}{suffix}"
                page.screenshot(path=str(target.with_suffix(".png")))
                after = state(page)
                assert not errors, errors
                target.with_suffix(".json").write_text(json.dumps(dict(
                    source=str(source.relative_to(ROOT)), save_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                    wasm_sha256=wasm_hash, command=item, native=native, before=before, after=after,
                    advance_seconds=args.advance, settle_seconds=args.settle,
                    use_pressed=args.engage, nearest_enemy=args.enemy,
                    camera="ordinary mouse input" if len(parts) == 4 or args.focus else "saved player view",
                    renderer="Chromium WebGL / SwiftShader", errors=errors), indent=2) + "\n")
                print(target.name, native["seed"], native["theme"], native["height"], after["time"] - before["time"], flush=True)
                context.close()
            browser.close()
    finally:
        server.shutdown()


if __name__ == "__main__":
    main()
