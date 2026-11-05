"""Board and revisit the arrival skiff through the packaged WASM game's controls."""
import argparse
import base64
from functools import partial
from http.server import ThreadingHTTPServer
import math
import os
from pathlib import Path
import threading
import traceback
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, QuietHandler, start_play, state, wait_for, wait_ready
from test_interiors_browser import expect, face_forward


def drift(page, axis, target, key):
    """Release via the real keyboard listener on arrival, without changing game state."""
    before = state(page)
    if abs(before[axis] - target) < .3:
        return
    if abs(before[axis] - target) > 16:
        drift(page, axis, before[axis] + math.copysign(12, target - before[axis]), key)
        drift(page, axis, target, key)
        return
    increasing = target > before[axis]
    page.evaluate("""args => {
        window.arrivalReached=false;
        const watch=()=>{
            const s=JSON.parse(Module.ccall('sc_web_state','string',[],[]));
            if(args.increasing ? s[args.axis]>=args.target : s[args.axis]<=args.target) {
                window.dispatchEvent(new KeyboardEvent('keyup', {
                    key:args.key,code:args.code,keyCode:args.number,which:args.number,bubbles:true
                }));
                window.arrivalReached=true;
            } else requestAnimationFrame(watch);
        };
        requestAnimationFrame(watch);
    }""", {'axis': axis, 'target': target - .1 if increasing else target + .1,
            'increasing': increasing, 'key': ' ' if key == 'Space' else key,
            'code': 'Space' if key == 'Space' else 'Key' + key.upper(),
            'number': 32 if key == 'Space' else ord(key.upper())})
    page.keyboard.down(key)
    try:
        wait_for(page, 'window.arrivalReached', timeout=20000)
    finally:
        page.keyboard.up(key)
    # The exterior moves at 14 m/s and the game caps a slow frame at 0.1 s.
    # Allow one simulation step; collision/hatch checks still use fixed bounds.
    arrived = state(page)
    assert abs(arrived[axis] - target) < 1.5, (axis, target, arrived)


def capture(page, name, crop=None):
    png = page.screenshot(path=str(ARTIFACTS / ('exterior-' + name + '.png')))
    if crop is None:
        return
    # Inspect the actual screenshot, independent of the GL drawing buffer's
    # preservation settings. No imaging package or renderer test hook is needed.
    return page.evaluate("""async args => {
        const blob=await (await fetch('data:image/png;base64,'+args.png)).blob();
        const bitmap=await createImageBitmap(blob), canvas=document.createElement('canvas');
        canvas.width=args.crop[2]; canvas.height=args.crop[3];
        const ctx=canvas.getContext('2d');
        ctx.drawImage(bitmap,...args.crop,0,0,canvas.width,canvas.height);
        const p=ctx.getImageData(0,0,canvas.width,canvas.height).data;
        let stars=0, amber=0, lit=0;
        for(let i=0;i<p.length;i+=4) {
            if(p[i+2]>65 && p[i+2]>=p[i] && p[i+1]>45) stars++;
            if(p[i]>140 && p[i+1]>80 && p[i]>p[i+2]*1.5) amber++;
            if(p[i]+p[i+1]+p[i+2]>120) lit++;
        }
        bitmap.close(); return {stars,amber,lit};
    }""", {'png': base64.b64encode(png).decode(), 'crop': crop})


def touch_drift(page, client, target, forward):
    points = [{'id': 1, 'x': 110, 'y': 275}]
    client.send('Input.dispatchTouchEvent', {'type': 'touchStart', 'touchPoints': points})
    points[0]['y'] += -44 if forward else 44
    client.send('Input.dispatchTouchEvent', {'type': 'touchMove', 'touchPoints': points})
    try:
        expect(page, f"s.z {'>' if forward else '<'} {target}", timeout=20000)
    finally:
        client.send('Input.dispatchTouchEvent', {'type': 'touchEnd', 'touchPoints': []})


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
            page.goto(base + '?seed=713')
            wait_ready(page)
            initial = state(page)
            assert initial['generator'] == 8 and abs(initial['z'] + 84) < .01
            start_play(page)
            face_forward(page)
            drift(page, 'z', -98, 's')
            capture(page, 'cabin-return')
            page.keyboard.down('d')
            expect(page, f"s.time > {state(page)['time'] + 1.2}")
            page.keyboard.up('d')
            assert 3.2 < state(page)['x'] < 3.7, state(page)
            drift(page, 'x', 0, 'a')
            page.keyboard.press('g')
            assert state(page)['health'] == initial['health']
            page.keyboard.press('F5')
            wait_for(page, 'window.scStorage.status === 3')
            saved = state(page)
            page.goto(base)
            wait_ready(page)
            loaded = state(page)
            assert loaded['generator'] == 8 and abs(loaded['z'] - saved['z']) < .1
            start_play(page)
            face_forward(page)
            page.keyboard.press('F2')
            expect(page, 's.players === 2')
            capture(page, 'split-cabin')
            drift(page, 'z', -80, 'w')
            capture(page, 'split-separate-spaces')
            page.keyboard.press('F2')
            expect(page, 's.players === 1')
            drift(page, 'z', 12, 'w')
            expect(page, 's.y < .2')
            capture(page, 'bay-reached')
            drift(page, 'z', -80, 's')
            drift(page, 'y', 4, 'Space')
            drift(page, 'z', -98, 's')
            assert state(page)['health'] == initial['health']
            capture(page, 'round-trip')
            print('PASS exterior keyboard: cabin collision, save/resume, split views, bay and return', flush=True)
            for scene in ('arrival', 'hull', 'space', 'skiff', 'cabin'):
                page.goto(base + '?seed=713&scene=' + scene)
                wait_ready(page)
                start_play(page)
                page.keyboard.press('f')
                expect(page, '!s.flashlight')
                if scene == 'space':
                    pixels = capture(page, scene, [220, 200, 1000, 220])
                    assert pixels['stars'] > 80, pixels
                elif scene == 'arrival':
                    pixels = capture(page, scene, [220, 180, 800, 200])
                    assert pixels['amber'] > 60, pixels
                elif scene == 'hull':
                    sky = capture(page, scene, [1050, 180, 210, 220])
                    wall = capture(page, scene + '-wall', [220, 180, 350, 220])
                    assert sky['stars'] > 15 and wall['stars'] < 8, (sky, wall)
                elif scene == 'cabin':
                    pixels = capture(page, scene, [200, 230, 650, 190])
                    assert pixels['lit'] > 10000, pixels
                else:
                    capture(page, scene)
                assert not state(page)['failed']
                print('PASS exterior render ' + scene, flush=True)
            context.close()
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            page = mobile.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(base + '?seed=713')
            wait_ready(page)
            page.locator('#start').tap()
            expect(page, '!s.paused')
            client = mobile.new_cdp_session(page)
            touch_drift(page, client, -96, False)
            assert -105 < state(page)['z'] < -90 and abs(state(page)['x']) < .2
            capture(page, 'touch-cabin')
            touch_drift(page, client, -80, True)
            capture(page, 'touch-departure')
            assert state(page)['z'] > -84 and not state(page)['failed'] and not errors, errors
            mobile.close()
            browser.close()
            print('PASS exterior browser: surrounding stars, opaque hull, lamps without flashlight, lit cabin, touch entry and exit', flush=True)
    except Exception:
        detail = traceback.format_exc().replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
        print('::error title=Arrival exterior browser regression::' + detail, flush=True)
        raise
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
