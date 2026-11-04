"""Exercise the actual WASM/WebGL application with a local HTTP server."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import argparse
import json
import os
import threading
import subprocess
import traceback
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build-web"
ARTIFACTS = BUILD / "test-artifacts"


class QuietHandler(SimpleHTTPRequestHandler):
    def log_message(self, *_args):
        pass


def state(page):
    result = json.loads(page.evaluate("Module.ccall('sc_web_state','string',[],[])"))
    assert result["work"] <= 2 and result["uploads"] <= 2, result
    assert result["chunks"] <= 192, result
    return result


def wait_ready(page):
    wait_for(page, "typeof Module !== 'undefined' && !document.getElementById('start').disabled", timeout=60000)
    wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).frames > 2", timeout=30000)
    wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).pending === 0", timeout=30000)


def wait_for(page, predicate, **kwargs):
    try:
        return page.wait_for_function(predicate, **kwargs)
    except Exception as error:
        snapshot = state(page)
        snapshot["browser"] = page.evaluate("({focus:document.activeElement?.id,lock:document.pointerLockElement?.id,visible:document.visibilityState})")
        page.screenshot(path=str(ARTIFACTS / "last-failure.png"))
        raise AssertionError(f"Browser wait failed: {predicate}; state={snapshot}") from error


def start_play(page):
    # Pointer capture and SDL focus changes finish asynchronously after the
    # button click. Wait for real simulation ticks before sending held input.
    before = state(page)
    page.locator("#start").click()
    wait_for(page, "before => { const s=JSON.parse(Module.ccall('sc_web_state','string',[],[])); return !s.paused && s.time > before + .08; }", arg=before["time"], timeout=15000)
    after = state(page)
    assert abs(after["yaw"] - before["yaw"]) < .05 and abs(after["pitch"] - before["pitch"]) < .05, (before, after)


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--site-dir", type=Path, default=BUILD)
    parser.add_argument("--url-path", default="/starship_crawl.html")
    args = parser.parse_args()
    site = args.site_dir.resolve()
    entry = site / args.url_path.lstrip("/")
    if args.url_path.endswith("/"):
        entry = entry / "index.html"
    assert entry.is_file(), f"Missing site entry point: {entry}"
    ARTIFACTS.mkdir(parents=True, exist_ok=True)
    server = ThreadingHTTPServer(("127.0.0.1", 0), partial(QuietHandler, directory=str(site)))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    base = f"http://127.0.0.1:{server.server_port}{args.url_path}"
    try:
        with sync_playwright() as p:
            options = {"headless": True, "args": ["--no-sandbox", "--use-angle=swiftshader", "--enable-unsafe-swiftshader"]}
            if os.environ.get("SC_BROWSER"):
                options["executable_path"] = os.environ["SC_BROWSER"]
            else:
                # Exercise the full browser's headless mode for WebGL/pointer
                # capture, matching the desktop Chromium used in local checks.
                options["channel"] = "chromium"
            browser = p.chromium.launch(**options)
            context = browser.new_context(viewport={"width": 1280, "height": 720}, accept_downloads=True)
            page = context.new_page()
            errors = []
            page.on("pageerror", lambda error: errors.append(str(error)))
            page.goto(base + "?seed=713")
            wait_ready(page)
            assert state(page)["players"] == 1
            assert state(page)["generator"] == 4
            native = Path(os.environ.get("SC_CORE_TEST", str(ROOT / "build" / "sc_tests")))
            assert native.is_file(), "Build sc_tests or set SC_CORE_TEST for native/WASM generation parity"
            probes = json.loads(subprocess.check_output([str(native), "--fingerprints"]))
            for probe in probes:
                actual = page.evaluate("args => Module.ccall('sc_web_fingerprint','number',['number','number','number','number'],args) >>> 0", [probe["seed"], *probe["region"]])
                assert actual == probe["hash"], (probe, actual)
            page.screenshot(path=str(ARTIFACTS / "arrival-menu.png"))
            start_play(page)
            wait_for(page, "!JSON.parse(Module.ccall('sc_web_state','string',[],[])).paused")
            before = state(page)
            page.keyboard.down("w")
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).z > -26", timeout=20000)
            page.keyboard.up("w")
            assert state(page)["z"] > before["z"] + 5
            page.mouse.down()
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).shots > 0", timeout=10000)
            page.mouse.up()
            # Capture warps are discarded, but real movement must still aim.
            heading = state(page)["yaw"]
            page.mouse.move(650, 360)
            page.mouse.move(690, 360)
            wait_for(page, "heading => Math.abs(JSON.parse(Module.ccall('sc_web_state','string',[],[])).yaw-heading) > .01", arg=heading)
            page.screenshot(path=str(ARTIFACTS / "arrival-game.png"))
            page.evaluate("Module.ccall('sc_web_save',null,[],[])")
            wait_for(page, "window.scStorage.status === 3", timeout=10000)
            saved = state(page)
            page.goto(base)
            wait_ready(page)
            loaded = state(page)
            assert loaded["seed"] == 713
            assert abs(loaded["z"] - saved["z"]) < .1
            assert loaded["players"] == 1
            start_play(page)
            page.keyboard.press("F2")
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).players === 2")
            page.screenshot(path=str(ARTIFACTS / "optional-split.png"))
            page.keyboard.press("F2")
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).players === 1")
            assert state(page)["players"] == 1
            page.evaluate("Module.ccall('sc_web_action',null,['number'],[0])")
            compatible = list((ROOT / "tests" / "fixtures" / "generator-v2.sav").read_bytes())
            page.evaluate("data => Module.FS.writeFile('/persist/import.sav',new Uint8Array(data))", compatible)
            assert page.evaluate("Module.ccall('sc_web_import','number',[],[])") == 1
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).pending === 0")
            resumed = state(page)
            assert resumed["seed"] != 713 and resumed["rank"] == 7 and resumed["kills"] == 81
            assert abs(resumed["x"]) < .001 and abs(resumed["z"] + 34) < .001
            assert resumed["flashlight"] and resumed["projectiles"] == 0
            # A malformed import must not mutate the current expedition.
            page.evaluate("Module.FS.writeFile('/persist/import.sav',new Uint8Array([1,2,3]))")
            assert page.evaluate("Module.ccall('sc_web_import','number',[],[])") == 0
            assert state(page)["seed"] == resumed["seed"]
            for scene in ["hall", "engine", "crew", "tunnel", "atrium", "encounter", "luxury", "bridge", "lab", "cargo", "cryonics", "botany"]:
                page.goto(base + "?seed=713&scene=" + scene)
                wait_ready(page)
                if scene == "encounter":
                    # Observe from the first simulation tick; a screenshot can
                    # take longer than the initial climb on software rendering.
                    page.evaluate("""() => {
                        window.sawClimber = false;
                        const observer = setInterval(() => {
                            if (JSON.parse(Module.ccall('sc_web_state','string',[],[])).climbers > 0) {
                                window.sawClimber = true;
                                clearInterval(observer);
                            }
                        }, 25);
                    }""")
                start_play(page)
                wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).frames > 5")
                page.screenshot(path=str(ARTIFACTS / (scene + ".png")))
                assert not state(page)["failed"]
                if scene in ["luxury", "bridge", "lab", "cargo", "cryonics", "botany"]:
                    assert state(page)["theme"] >= 6
                if scene == "encounter":
                    wait_for(page, "window.sawClimber", timeout=15000)
                    wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).projectiles > 0", timeout=15000)
                    assert state(page)["enemy_shots"] > 0
                    # Freeze the same rendered scene to inspect both lighting states.
                    page.evaluate("Module.ccall('sc_web_action',null,['number'],[0])")
                    page.keyboard.press("f")
                    wait_for(page, "!JSON.parse(Module.ccall('sc_web_state','string',[],[])).flashlight")
                    page.screenshot(path=str(ARTIFACTS / "flashlight-off.png"))
                    page.locator("#menu").click()
                    page.locator("[data-action='7']").click()
                    wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).flashlight")
                    page.locator("#menu").click()
                    page.screenshot(path=str(ARTIFACTS / "flashlight-on.png"))
                if scene == "hall":
                    assert state(page)["span"] == 4 and state(page)["ready"]
                    # Cross a storage chunk boundary inside one continuous hall.
                    start_generated = state(page)["generated"]
                    page.keyboard.down("w")
                    page.keyboard.down("Shift")
                    wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).z > 195", timeout=30000)
                    page.keyboard.up("w")
                    page.keyboard.up("Shift")
                    current = state(page)
                    assert current["ready"] and current["span"] == 4 and current["y"] < .2
                    assert current["generated"] >= start_generated
                    page.screenshot(path=str(ARTIFACTS / "hall-across-chunks.png"))
            # Play the first mission and spend its reward, without test-only hooks.
            page.goto(base + "?seed=713&scene=bay")
            wait_ready(page)
            start_play(page)
            page.keyboard.down("w")
            try:
                wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).z > 40", timeout=15000)
            except Exception as error:
                page.screenshot(path=str(ARTIFACTS / "console-walk-failure.png"))
                raise AssertionError(f"Console walk failed: {state(page)}") from error
            page.keyboard.up("w")
            page.keyboard.press("g")
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).mission === 1")
            assert state(page)["credits"] >= 75
            page.keyboard.press("i")
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).paused")
            page.keyboard.press("F4")
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).journal_page === 0")
            page.keyboard.press("4")
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).armor > 40")
            page.keyboard.press("x")
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).weapon === 1")
            page.screenshot(path=str(ARTIFACTS / "character-journal.png"))
            page.keyboard.press("Enter")
            wait_for(page, "!JSON.parse(Module.ccall('sc_web_state','string',[],[])).paused")
            page.evaluate("Module.ccall('sc_web_save',null,[],[])")
            wait_for(page, "window.scStorage.status === 3")
            page.goto(base)
            wait_ready(page)
            assert state(page)["mission"] == 1 and state(page)["weapon"] == 1 and state(page)["armor"] > 40
            # Independent camera origins: one player deep inside a distant district,
            # the other at the entrance, sharing bounded generation and upload work.
            page.goto(base + "?seed=713&scene=distant&split")
            wait_ready(page)
            assert state(page)["players"] == 2 and state(page)["x"] > 287999000
            start_play(page)
            page.screenshot(path=str(ARTIFACTS / "distant-split.png"))
            assert not state(page)["failed"]
            page.evaluate("Module.ccall('sc_web_action',null,['number'],[0])")
            page.evaluate("Module.ccall('sc_web_save',null,[],[])")
            wait_for(page, "window.scStorage.status === 3", timeout=10000)
            far_saved = state(page)
            page.goto(base)
            wait_ready(page)
            assert state(page)["players"] == 1 and abs(state(page)["x"]-far_saved["x"]) < .1
            # A real save from generator v1 keeps character progress and starts a new ship.
            legacy = list((ROOT / "tests" / "fixtures" / "generator-v1.sav").read_bytes())
            page.evaluate("data => Module.FS.writeFile('/persist/import.sav',new Uint8Array(data))", legacy)
            assert page.evaluate("Module.ccall('sc_web_import','number',[],[])") == 1
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).pending === 0")
            migrated = state(page)
            assert migrated["rank"] == 7 and migrated["kills"] == 81 and migrated["z"] == -34
            assert migrated["players"] == 1 and migrated["seed"] != 123456
            assert not errors, errors
            context.close()

            mobile = browser.new_context(viewport={"width": 844, "height": 390}, device_scale_factor=1, is_mobile=True, has_touch=True)
            page = mobile.new_page()
            page.on("pageerror", lambda error: errors.append(str(error)))
            page.goto(base + "?seed=713")
            wait_ready(page)
            page.locator("#start").tap()
            client = mobile.new_cdp_session(page)
            # Two real simultaneous touches: move on the left, fire on the right.
            start_z = state(page)["z"]
            client.send("Input.dispatchTouchEvent", {"type": "touchStart", "touchPoints": [{"id": 1, "x": 110, "y": 280}, {"id": 2, "x": 760, "y": 280}]})
            client.send("Input.dispatchTouchEvent", {"type": "touchMove", "touchPoints": [{"id": 1, "x": 110, "y": 230}, {"id": 2, "x": 760, "y": 280}]})
            wait_for(page, f"JSON.parse(Module.ccall('sc_web_state','string',[],[])).z > {start_z + 3}", timeout=20000)
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).shots > 0", timeout=10000)
            client.send("Input.dispatchTouchEvent", {"type": "touchEnd", "touchPoints": []})
            assert state(page)["flashlight"]
            page.touchscreen.tap(844 * .64, 390 * .48)
            wait_for(page, "!JSON.parse(Module.ccall('sc_web_state','string',[],[])).flashlight")
            page.touchscreen.tap(844 * .64, 390 * .48)
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).flashlight")
            page.screenshot(path=str(ARTIFACTS / "mobile-touch.png"))
            page.touchscreen.tap(844 * .77, 390 * .27)
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).weapon === 1")
            page.set_viewport_size({"width": 390, "height": 844})
            page.wait_for_timeout(300)
            page.screenshot(path=str(ARTIFACTS / "mobile-portrait.png"))
            assert not state(page)["failed"]
            assert state(page)["players"] == 1
            assert not errors, errors
            mobile.close()
            browser.close()
            print("PASS browser: startup, solo default, movement, firing, IndexedDB reload, optional split, invalid import, eleven interiors, first mission and rewarded armor upgrade, weapon cycling, journal, v7 reload, hall traversal across chunks, wall climbers and projectiles, keyboard/menu/touch flashlight, distant split/save, v1 migration, native/WASM generation parity, bounded streaming/uploads, multitouch, resize")
    finally:
        server.shutdown()
        server.server_close()


if __name__ == "__main__":
    try:
        run()
    except Exception:
        if os.environ.get("GITHUB_ACTIONS"):
            detail = traceback.format_exc().replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
            print("::error title=Browser regression::" + detail, flush=True)
        raise
