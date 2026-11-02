"""Exercise the actual WASM/WebGL application with a local HTTP server."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import argparse
import json
import os
import threading
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build-web"
ARTIFACTS = BUILD / "test-artifacts"


class QuietHandler(SimpleHTTPRequestHandler):
    def log_message(self, *_args):
        pass


def state(page):
    return json.loads(page.evaluate("Module.ccall('sc_web_state','string',[],[])"))


def wait_ready(page):
    page.wait_for_function("typeof Module !== 'undefined' && !document.getElementById('start').disabled", timeout=60000)
    page.wait_for_function("JSON.parse(Module.ccall('sc_web_state','string',[],[])).frames > 2", timeout=30000)


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
            browser = p.chromium.launch(**options)
            context = browser.new_context(viewport={"width": 1280, "height": 720}, accept_downloads=True)
            page = context.new_page()
            errors = []
            page.on("pageerror", lambda error: errors.append(str(error)))
            page.goto(base + "?seed=713")
            wait_ready(page)
            assert state(page)["players"] == 1
            page.screenshot(path=str(ARTIFACTS / "arrival-menu.png"))
            page.locator("#start").click()
            page.wait_for_function("!JSON.parse(Module.ccall('sc_web_state','string',[],[])).paused")
            before = state(page)
            page.keyboard.down("w")
            page.wait_for_function("JSON.parse(Module.ccall('sc_web_state','string',[],[])).z > -26", timeout=20000)
            page.keyboard.up("w")
            assert state(page)["z"] > before["z"] + 5
            page.mouse.down()
            page.wait_for_function("JSON.parse(Module.ccall('sc_web_state','string',[],[])).shots > 0", timeout=10000)
            page.mouse.up()
            page.screenshot(path=str(ARTIFACTS / "arrival-game.png"))
            page.evaluate("Module.ccall('sc_web_save',null,[],[])")
            page.wait_for_function("window.scStorage.status === 3", timeout=10000)
            saved = state(page)
            page.goto(base)
            wait_ready(page)
            loaded = state(page)
            assert loaded["seed"] == 713
            assert abs(loaded["z"] - saved["z"]) < .1
            assert loaded["players"] == 1
            page.locator("#start").click()
            page.keyboard.press("F2")
            page.wait_for_function("JSON.parse(Module.ccall('sc_web_state','string',[],[])).players === 2")
            page.screenshot(path=str(ARTIFACTS / "optional-split.png"))
            page.keyboard.press("F2")
            assert state(page)["players"] == 1
            page.evaluate("Module.ccall('sc_web_action',null,['number'],[0])")
            # A malformed import must not mutate the current expedition.
            page.evaluate("Module.FS.writeFile('/persist/import.sav',new Uint8Array([1,2,3]))")
            assert page.evaluate("Module.ccall('sc_web_import','number',[],[])") == 0
            assert state(page)["seed"] == 713
            for scene in ["hall", "engine", "crew", "tunnel", "atrium"]:
                page.goto(base + "?seed=713&scene=" + scene)
                wait_ready(page)
                page.locator("#start").click()
                page.wait_for_function("JSON.parse(Module.ccall('sc_web_state','string',[],[])).frames > 5")
                page.screenshot(path=str(ARTIFACTS / (scene + ".png")))
                assert not state(page)["failed"]
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
            page.wait_for_function(f"JSON.parse(Module.ccall('sc_web_state','string',[],[])).z > {start_z + 3}", timeout=20000)
            page.wait_for_function("JSON.parse(Module.ccall('sc_web_state','string',[],[])).shots > 0", timeout=10000)
            client.send("Input.dispatchTouchEvent", {"type": "touchEnd", "touchPoints": []})
            page.screenshot(path=str(ARTIFACTS / "mobile-touch.png"))
            page.set_viewport_size({"width": 390, "height": 844})
            page.wait_for_timeout(300)
            page.screenshot(path=str(ARTIFACTS / "mobile-portrait.png"))
            assert not state(page)["failed"]
            assert state(page)["players"] == 1
            assert not errors, errors
            mobile.close()
            browser.close()
            print("PASS browser: startup, solo default, movement, firing, IndexedDB reload, optional split, invalid import, five interiors, multitouch, resize")
    finally:
        server.shutdown()
        server.server_close()


if __name__ == "__main__":
    run()
