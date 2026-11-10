"""Render generated weather, local traces and vaults in the packaged WebGL game."""
import argparse
import base64
from functools import partial
from http.server import ThreadingHTTPServer
import os
import math
from pathlib import Path
import threading
import traceback
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, QuietHandler, start_play, state, wait_for, wait_ready

S = "JSON.parse(Module.ccall('sc_web_state','string',[],[]))"

def expect(page, condition, **kwargs):
    wait_for(page, f"(() => {{ const s={S}; return {condition}; }})()", **kwargs)

def turn_from_growth(page):
    # Inspection places the camera looking into a real growth cluster. Fold
    # equipment needs clear floor; turn toward the room using actual mouse input.
    target = state(page)['yaw'] + math.pi
    cursor = [640, 360]
    page.mouse.move(*cursor)
    for _ in range(8):
        current = state(page)
        delta = target - current['yaw']
        delta = math.atan2(math.sin(delta), math.cos(delta))
        if abs(delta) < .01:
            return
        cursor[0] += delta / .0023
        page.mouse.move(*cursor)
        expect(page, f"s.time > {current['time'] + .05}")
    raise AssertionError(f"Cannot face clear floor: {state(page)}")

def capture(page, name):
    path = ARTIFACTS / f'habitat-{name}.png'
    png = page.screenshot(path=str(path))
    # Check the actual rendered world, omitting top/bottom HUD and the weapon.
    return page.evaluate("""async encoded => {
        const blob=await(await fetch('data:image/png;base64,'+encoded)).blob();
        const bitmap=await createImageBitmap(blob);
        const out=document.createElement('canvas');out.width=640;out.height=360;
        const c=out.getContext('2d');c.drawImage(bitmap,0,0,640,360);bitmap.close();
        const p=c.getImageData(110,85,210,160).data;let lit=0,colored=0,sum=0;
        for(let i=0;i<p.length;i+=4){const a=p[i],b=p[i+1],d=p[i+2];
            if(Math.max(a,b,d)>20)lit++;if(Math.max(a,b,d)-Math.min(a,b,d)>12)colored++;
            sum=(sum+a*3+b*5+d*7)>>>0;
        }return {lit,colored,sum};
    }""", base64.b64encode(png).decode())

def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=BUILD)
    parser.add_argument('--url-path', default='/starship_crawl.html')
    parser.add_argument('--tentacles-only', action='store_true', help='Run the tentacle and view-isolation checks without the weather tour')
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
            hashes = set()
            tour = [(f'biome{i}', 'atmosphere', i) for i in range(14)] + [
                    ('volume1', 'volume', 1), ('volume2', 'volume', 2), ('shape4', 'form', 4), ('shape6', 'form', 6)]
            for scene, field, value in ([] if args.tentacles_only else tour):
                page.goto(base + f'?seed=713&scene={scene}')
                wait_ready(page)
                s = state(page)
                assert s['generator'] == 16 and s[field] == value and s['ready'] and not s['failed'], s
                assert 0 < s['sprites'] <= 1536, s
                start_play(page)
                expect(page, f"s.time > {s['time'] + .4}")
                pixels = capture(page, scene)
                assert pixels['lit'] > 80 and pixels['colored'] > 20, (scene, pixels)
                hashes.add(pixels['sum'])
                if scene == 'biome2':
                    before = state(page)
                    expect(page, f"s.time > {before['time'] + .7}")
                    animated = capture(page, 'runoff-motion')
                    assert animated['sum'] != pixels['sum']
                    page.keyboard.press('F6')
                    expect(page, 's.journal && s.journal_page === 2')
                    capture(page, 'local-account')
                    page.keyboard.press('Enter')
                    expect(page, '!s.journal')
                if scene == 'shape4':
                    before = state(page)
                    page.keyboard.down('w')
                    expect(page, f"Math.hypot(s.x-{before['x']},s.z-{before['z']}) > 2")
                    page.keyboard.up('w')
                    assert abs(state(page)['y'] - before['y']) < .2
                    page.keyboard.press('m')
                    expect(page, 's.map')
                    capture(page, 'bent-arcade-map')
                    page.keyboard.press('m')
                assert not state(page)['failed'], state(page)
                print(f'PASS {scene}: bounded sprites, correct profile, rendered detail', flush=True)
            assert len(hashes) == (0 if args.tentacles_only else len(tour)), hashes
            # Inspect actual seeded growth, then freeze the rendered game rather
            # than altering time or replacing the production animation.
            page.goto(base + '?seed=713&scene=tentacles')
            wait_ready(page)
            expect(page, 's.tentacles > 0 && s.tentacles <= 96 && s.tentacles2 === 0')
            start_play(page)
            # This renderer-focused sandbox exposes ordinary Veil; keep the
            # growing population from interrupting pause/view-isolation checks.
            page.keyboard.press('v')
            expect(page, 's.veil')
            before = state(page)
            still = capture(page, 'tentacles')
            expect(page, f"s.time > {before['time'] + 1.2}")
            moving = capture(page, 'tentacles-motion')
            assert moving['sum'] != still['sum'], (still, moving)
            page.keyboard.press('Escape')
            expect(page, 's.paused')
            frozen = state(page)
            first = capture(page, 'tentacles-paused')
            expect(page, f"s.frames > {frozen['frames'] + 8}")
            second = capture(page, 'tentacles-paused-again')
            assert state(page)['time'] == frozen['time'] and first == second, (first, second)
            page.keyboard.press('Enter')
            expect(page, '!s.paused')
            # Two independently rendered cameras retain their own sprite budgets.
            page.keyboard.press('F2')
            expect(page, 's.players === 2 && s.sprites > 0 && s.sprites2 > 0 && s.tentacles > 0 && s.tentacles2 > 0')
            assert state(page)['sprites2'] <= 1536
            assert state(page)['tentacles'] <= 96 and state(page)['tentacles2'] <= 96
            capture(page, 'split')
            turn_from_growth(page)
            page.keyboard.press('r')
            expect(page, 's.shelter_deployed')
            page.keyboard.press('g')
            expect(page, 's.sheltered && s.sprites === 0 && s.sprites2 > 0 && s.tentacles === 0 && s.tentacles2 > 0')
            capture(page, 'independent-shelter-weather')
            # Existing special interiors must not inherit atmosphere from the last view.
            for scene in ['shelter', 'matrix', 'cabin', 'arrival']:
                page.goto(base + f'?seed=713&scene={scene}')
                wait_ready(page)
                assert state(page)['sprites'] == 0 and state(page)['tentacles'] == 0, state(page)
                capture(page, scene)
            context.close()
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            page = mobile.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(base + '?seed=713&scene=tentacles')
            wait_ready(page)
            page.locator('#start').tap()
            expect(page, '!s.paused && s.sprites > 0 && s.tentacles > 0')
            capture(page, 'mobile-spores')
            page.set_viewport_size({'width': 390, 'height': 844})
            expect(page, 's.frames > 20 && !s.failed')
            assert state(page)['sprites'] <= 1536
            assert state(page)['tentacles'] <= 96
            assert not errors, errors
            mobile.close()
            browser.close()
        print('PASS generated atmospheres, vaults, dossier, map, split budgets and touch-sized rendering')
    finally:
        server.shutdown()

if __name__ == '__main__':
    try:
        run()
    except Exception:
        traceback.print_exc()
        raise
