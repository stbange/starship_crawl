"""Explore generated interiors in the packaged WASM game using ordinary controls."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import math
import os
from pathlib import Path
import threading
import traceback
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, ROOT, QuietHandler, start_play, state, wait_for, wait_ready

S = "JSON.parse(Module.ccall('sc_web_state','string',[],[]))"


def expect(page, condition, **kwargs):
    wait_for(page, f"(() => {{ const s={S}; return {condition}; }})()", **kwargs)


def face_forward(page):
    # Real pointer-lock mouse input, with state feedback for focus/warp filtering.
    page.mouse.move(640, 360)
    cursor = [640, 360]
    for _ in range(8):
        s = state(page)
        yaw = math.atan2(math.sin(s['yaw']), math.cos(s['yaw']))
        if abs(yaw) < .004 and abs(s['pitch']) < .004:
            return
        cursor[0] -= yaw / .0023
        cursor[1] += s['pitch'] / .0023
        page.mouse.move(*cursor)
        expect(page, f"s.time > {s['time'] + .05}")
    raise AssertionError(f"Could not face forward: {state(page)}")


def walk_axis(page, axis, target, key):
    before = state(page)
    if abs(before[axis] - target) < .4:
        return
    # Long crouched walks can outlast one CDP deadline on a software renderer.
    # Keep each segment short and retain the same exact arrival/collision checks.
    if abs(before[axis] - target) > 16:
        waypoint = before[axis] + (12 if target > before[axis] else -12)
        walk_axis(page, axis, waypoint, key)
        walk_axis(page, axis, target, key)
        return
    increasing = target > before[axis]
    threshold = target - .12 if increasing else target + .12
    # Observe arrival before pressing the key; CDP can deliver the next Python
    # command several game frames later on a software renderer.
    page.evaluate("""args => {
        window.interiorArrival=false;
        const watch=()=>{
            const s=JSON.parse(Module.ccall('sc_web_state','string',[],[]));
            const reached=args.increasing ? s[args.axis]>=args.target : s[args.axis]<=args.target;
            if(reached) {
                const code=args.key.toUpperCase().charCodeAt(0);
                window.dispatchEvent(new KeyboardEvent('keyup', {
                    key:args.key,code:'Key'+args.key.toUpperCase(),keyCode:code,which:code,bubbles:true
                }));
                window.interiorArrival=true;
            } else requestAnimationFrame(watch);
        };
        requestAnimationFrame(watch);
    }""", {'axis': axis, 'target': threshold, 'increasing': increasing, 'key': key})
    page.keyboard.down('Control')
    page.keyboard.down(key)
    try:
        wait_for(page, 'window.interiorArrival', timeout=20000)
    finally:
        page.keyboard.up(key)
        page.keyboard.up('Control')
    assert abs(state(page)[axis] - target) < .5, state(page)


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=BUILD)
    parser.add_argument('--url-path', default='/starship_crawl.html')
    args = parser.parse_args()
    ARTIFACTS.mkdir(parents=True, exist_ok=True)
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(QuietHandler, directory=str(args.site_dir.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    base = f'http://127.0.0.1:{server.server_port}{args.url_path}'
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
            for category, values, field in [('layout', range(3), 'layout'), ('history', range(6), 'incident'), ('landmark', range(1, 5), 'landmark')]:
                for value in values:
                    scene = category + str(value)
                    seed = {1: 22, 2: 11, 3: 22, 4: 1, 5: 10}.get(value, 713) if category == 'history' else 713
                    page.goto(base + f'?seed={seed}&scene=' + scene)
                    wait_ready(page)
                    s = state(page)
                    assert s['generator'] == 6 and s[field] == value and s['ready'] and not s['failed'], s
                    if category == 'landmark':
                        assert s['span'] == 4
                    page.keyboard.press('v')
                    expect(page, 's.veil')
                    start_play(page)
                    page.screenshot(path=str(ARTIFACTS / ('interior-' + scene + '.png')))
                    if category == 'landmark':
                        page.keyboard.press('m')
                        expect(page, 's.map')
                        page.screenshot(path=str(ARTIFACTS / ('interior-' + scene + '-map.png')))
                        page.keyboard.press('m')
                        expect(page, '!s.map')
                    if category == 'layout':
                        face_forward(page)
                        s = state(page)
                        walk_axis(page, 'x', s['cache_x'], 'd')
                        walk_axis(page, 'z', s['cache_z'], 'w')
                        expect(page, f"s.pickups > {s['pickups']}")
                        page.screenshot(path=str(ARTIFACTS / ('interior-' + scene + '-cache.png')))
                        page.keyboard.press('F5')
                        wait_for(page, 'window.scStorage.status === 3')
                        saved = state(page)
                        page.goto(base)
                        wait_ready(page)
                        loaded = state(page)
                        assert loaded['generator'] == 6 and loaded['layout'] == value
                        assert loaded['pickups'] == saved['pickups'] and loaded['weapons'] == saved['weapons']
                        assert abs(loaded['x'] - saved['x']) < .1 and abs(loaded['z'] - saved['z']) < .1
                        start_play(page)
                        expect(page, f"s.time > {loaded['time'] + .2}")
                        assert state(page)['pickups'] == saved['pickups'], state(page)
                        # Leave through the other entrance, proving an actual loop.
                        origin_x = round(loaded['x'] / 96) * 96
                        origin_z = math.floor(loaded['z'] / 96) * 96 + 48
                        walk_axis(page, 'z', origin_z + 12, 'w' if loaded['z'] < origin_z + 12 else 's')
                        walk_axis(page, 'x', origin_x + 10.8, 'a')
                        side_mid = (9 + min(44, loaded['room_depth'] * .5 - 3)) * .5
                        walk_axis(page, 'z', origin_z + side_mid, 'w')
                        walk_axis(page, 'x', origin_x, 'a')
                    print('PASS generated interior ' + scene, flush=True)
            # A real pre-change format-6 save retains generator 3 on WebAssembly.
            legacy = list((ROOT / 'tests' / 'fixtures' / 'generator-v6.sav').read_bytes())
            page.evaluate("data => Module.FS.writeFile('/persist/import.sav',new Uint8Array(data))", legacy)
            assert page.evaluate("Module.ccall('sc_web_import','number',[],[])") == 1
            wait_ready(page)
            expect(page, 's.generator === 3 && s.rank === 7 && s.sheltered && s.mission === 2')
            page.keyboard.press('n')
            page.keyboard.press('Enter')
            expect(page, 's.generator === 13 && s.rank === 1 && s.mission === 0')
            assert not state(page)['failed'] and not errors
            context.close()
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            page = mobile.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(base + '?seed=713&scene=landmark3')
            wait_ready(page)
            page.locator('#start').tap()
            expect(page, '!s.paused && s.landmark === 3')
            page.screenshot(path=str(ARTIFACTS / 'interior-mobile-landmark.png'))
            assert not state(page)['failed'] and not errors
            mobile.close()
            browser.close()
            print('PASS generated interiors browser: all layouts/history/landmarks, maps, reachable caches, persistence, legacy/new-ship transition, mobile rendering')
    except Exception:
        detail = traceback.format_exc().replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
        print('::error title=Generated interiors browser regression::' + detail, flush=True)
        raise
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
