"""Play the fold shelter through the packaged WASM/WebGL game and real controls."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import json
import os
from pathlib import Path
import threading
import traceback
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, QuietHandler, start_play, state, wait_for, wait_ready


def move_to(page, x, z):
    # Shelter entry faces +Z, so ordinary keyboard strafing walks these corridors.
    # Slow walking plus feedback tolerates delayed CDP input on software GPUs.
    page.keyboard.down('Control')
    try:
        for axis, target, positive, negative in [('sx', x, 'd', 'a'), ('sz', z, 'w', 's')]:
            for attempt in range(8):
                current = state(page)[axis]
                if abs(current - target) < .4:
                    break
                key = positive if current < target else negative
                op = '>=' if current < target else '<='
                threshold = target - .1 if current < target else target + .1
                page.keyboard.down(key)
                try:
                    wait_for(page, f"JSON.parse(Module.ccall('sc_web_state','string',[],[])).{axis} {op} {threshold}", timeout=15000)
                finally:
                    page.keyboard.up(key)
            else:
                raise AssertionError(f'Could not settle at {axis}={target}: {state(page)}')
            assert state(page)['sheltered'], state(page)
    finally:
        page.keyboard.up('Control')


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=BUILD)
    parser.add_argument('--url-path', default='/starship_crawl.html')
    args = parser.parse_args()
    ARTIFACTS.mkdir(parents=True, exist_ok=True)
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(QuietHandler, directory=str(args.site_dir.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    base = f'http://127.0.0.1:{server.server_port}{args.url_path}'
    page = None
    try:
        with sync_playwright() as p:
            options = {'headless': True, 'args': ['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader']}
            if os.environ.get('SC_BROWSER'):
                options['executable_path'] = os.environ['SC_BROWSER']
            else:
                options['channel'] = 'chromium'
            browser = p.chromium.launch(**options)
            context = browser.new_context(viewport={'width': 1280, 'height': 720})
            page = context.new_page()
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(base + '?seed=713&scene=bay')
            wait_ready(page)
            start_play(page)
            page.keyboard.down('w')
            try:
                wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).z > 12", timeout=15000)
            finally:
                page.keyboard.up('w')
            anchor = state(page)
            page.keyboard.press('r')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).shelter_deployed")
            page.screenshot(path=str(ARTIFACTS / 'shelter-door.png'))
            page.keyboard.press('F2')
            page.keyboard.press('g')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).sheltered")
            page.screenshot(path=str(ARTIFACTS / 'shelter-split.png'))
            page.keyboard.press('F2')
            page.screenshot(path=str(ARTIFACTS / 'shelter-interior.png'))
            before = state(page)
            page.mouse.down()
            wait_for(page, f"JSON.parse(Module.ccall('sc_web_state','string',[],[])).time > {before['time'] + .2}")
            page.mouse.up()
            assert state(page)['shots'] == before['shots']
            move_to(page, -3.3, -5)
            move_to(page, -6.2, -3.5)
            page.keyboard.press('g')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).nourished > 110")
            page.screenshot(path=str(ARTIFACTS / 'shelter-kitchen.png'))
            move_to(page, -3.3, -3.5)
            move_to(page, -3.3, 3.3)
            move_to(page, -6.3, 3.3)
            page.keyboard.press('g')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).resting > 0")
            page.screenshot(path=str(ARTIFACTS / 'shelter-bed.png'))
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).resting === 0", timeout=25000)
            page.keyboard.press('F5')
            wait_for(page, 'window.scStorage.status === 3')
            saved = state(page)
            page.goto(base)
            wait_ready(page)
            loaded = state(page)
            assert loaded['sheltered'] and loaded['shelter_deployed'], loaded
            assert abs(loaded['sx'] - saved['sx']) < .1 and abs(loaded['sz'] - saved['sz']) < .1
            assert loaded['seed'] == saved['seed'] and loaded['nourished'] > 0
            start_play(page)
            move_to(page, -3.3, 3.3)
            move_to(page, -3.3, -.3)
            move_to(page, 3.3, -.3)
            move_to(page, 3.3, 4.1)
            move_to(page, 6.5, 4.1)
            assert state(page)['facility'] == 3
            page.keyboard.press('g')
            page.screenshot(path=str(ARTIFACTS / 'shelter-medical.png'))
            move_to(page, 3.3, 4.1)
            move_to(page, 3.3, -.3)
            move_to(page, 0, -5.6)
            page.keyboard.press('g')
            wait_for(page, "!JSON.parse(Module.ccall('sc_web_state','string',[],[])).sheltered")
            returned = state(page)
            assert abs(returned['x'] - anchor['x']) < .1 and abs(returned['z'] - anchor['z']) < .1
            assert abs(returned['yaw'] - anchor['yaw']) < .01 and abs(returned['pitch'] - anchor['pitch']) < .01
            assert not returned['failed'] and not errors, (returned, errors)
            context.close()
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            page = mobile.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            # Existing room-inspection scene starts on a supported bay floor.
            page.goto(base + '?seed=713&scene=bay')
            wait_ready(page)
            page.locator('#start').tap()
            client = mobile.new_cdp_session(page)
            client.send('Input.dispatchTouchEvent', {'type': 'touchStart', 'touchPoints': [{'id': 1, 'x': 110, 'y': 280}]})
            client.send('Input.dispatchTouchEvent', {'type': 'touchMove', 'touchPoints': [{'id': 1, 'x': 110, 'y': 220}]})
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).z > 12", timeout=15000)
            client.send('Input.dispatchTouchEvent', {'type': 'touchEnd', 'touchPoints': []})
            page.touchscreen.tap(844 * .64, 390 * .27)
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).shelter_deployed")
            page.touchscreen.tap(844 * .90, 390 * .27)
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).sheltered")
            page.screenshot(path=str(ARTIFACTS / 'shelter-mobile.png'))
            page.touchscreen.tap(844 * .90, 390 * .27)
            wait_for(page, "!JSON.parse(Module.ccall('sc_web_state','string',[],[])).sheltered")
            assert not state(page)['failed'] and not errors
            mobile.close()
            browser.close()
            print('PASS shelter browser: portal, separate split-screen, no firing inside, kitchen, bed, med station, v4 IndexedDB reload, exact return, mobile summon/enter/exit')
    except Exception:
        detail = traceback.format_exc()
        if page and not page.is_closed():
            try:
                detail += '\nGame state: ' + json.dumps(state(page))
                page.screenshot(path=str(ARTIFACTS / 'shelter-failure.png'))
            except Exception:
                pass
        print('::error title=Shelter browser regression::' + detail.replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A'), flush=True)
        raise
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
