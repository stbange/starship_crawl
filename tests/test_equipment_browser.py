"""Play fold rigs, beam weapons and persistent flare lighting in packaged WebGL."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
import traceback
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, QuietHandler, start_play, state, wait_for, wait_ready

S = "JSON.parse(Module.ccall('sc_web_state','string',[],[]))"


def expect(page, condition, **kwargs):
    wait_for(page, f"(() => {{ const s={S}; return {condition}; }})()", **kwargs)


def drive_to(page, target):
    # Release at the target in the browser's RAF, independent of Python latency.
    page.evaluate("target => { window.equipmentArrival=false; const stop=()=>{const s=JSON.parse(Module.ccall('sc_web_state','string',[],[])); if(s.z>=target){window.dispatchEvent(new KeyboardEvent('keyup',{key:'w',code:'KeyW',keyCode:87,which:87,bubbles:true}));window.equipmentArrival=true;}else requestAnimationFrame(stop);};requestAnimationFrame(stop);}", target)
    page.keyboard.down('w')
    try:
        wait_for(page, 'window.equipmentArrival', timeout=20000)
    finally:
        page.keyboard.up('w')


def shoot(page, beam=False, capture=None):
    before = state(page)['shots']
    page.mouse.down()
    try:
        expect(page, f"s.shots > {before} && " + ('s.beams > 0' if beam else 's.projectiles > 0'))
        if beam:
            assert state(page)['projectiles'] == 0
        if capture:
            page.screenshot(path=str(ARTIFACTS / capture))
    finally:
        page.mouse.up()


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
            page.goto(base + '?seed=713&scene=bay')
            wait_ready(page)
            start_play(page)
            page.keyboard.press('t')
            expect(page, 's.mech_deployed && s.riding === 0')
            page.screenshot(path=str(ARTIFACTS / 'fold-mech-parked.png'))
            page.keyboard.press('g')
            expect(page, 's.riding === 1 && s.hull === 600')
            shoot(page)
            page.keyboard.press('x')
            expect(page, 's.rig_weapon === 1')
            shoot(page)
            expect(page, 's.projectiles === 0', timeout=15000)
            page.keyboard.press('x')
            expect(page, 's.rig_weapon === 2')
            shoot(page, beam=True, capture='mech-cockpit-beam.png')
            page.keyboard.press('F5')
            wait_for(page, 'window.scStorage.status === 3')
            page.goto(base)
            wait_ready(page)
            expect(page, 's.riding === 1 && s.rig_weapon === 2 && s.hull === 600')
            start_play(page)
            page.keyboard.press('g')
            expect(page, 's.riding === 0 && s.mech_deployed')
            page.keyboard.press('t')
            expect(page, '!s.mech_deployed')
            page.keyboard.press('y')
            expect(page, 's.wagon_deployed')
            page.screenshot(path=str(ARTIFACTS / 'fold-wagon-parked.png'))
            page.keyboard.press('g')
            expect(page, 's.riding === 2 && s.hull === 150')
            before = state(page)
            drive_to(page, before['z'] + 8)
            page.keyboard.down('Control')
            try:
                expect(page, 's.drive_speed < .2')
            finally:
                page.keyboard.up('Control')
            assert state(page)['z'] > before['z'] + 8
            page.screenshot(path=str(ARTIFACTS / 'wagon-dashboard.png'))
            page.keyboard.press('g')
            expect(page, 's.riding === 0')
            page.keyboard.press('y')
            expect(page, '!s.wagon_deployed')
            page.keyboard.press('k')
            expect(page, 's.weapon === 19')
            for gun in (18, 17, 16):
                page.keyboard.press('z')
                expect(page, f's.weapon === {gun}')
                shoot(page, beam=True)
            page.screenshot(path=str(ARTIFACTS / 'personal-beam.png'))
            # Earn upgrade currency by reaching and using the ordinary entry mission console.
            drive_to(page, 42)
            page.keyboard.press('g')
            expect(page, 's.mission === 1 && s.points >= 2')
            page.keyboard.press('o')
            page.keyboard.press('p')
            expect(page, 's.flare_lifetime === 1 && s.flare_strength === 1')
            page.keyboard.press('k')
            # Real pointer-lock mouse motion aims the flare at the deck.
            page.mouse.move(640, 360)
            page.mouse.move(640, 560)
            expect(page, 's.pitch < -.3')
            shoot(page)
            expect(page, 's.flares > 0 && s.flare_power === 3.25 && s.flare_life > 40')
            page.keyboard.press('f')
            expect(page, '!s.flashlight')
            page.screenshot(path=str(ARTIFACTS / 'attached-flare-light.png'))
            page.keyboard.press('F5')
            wait_for(page, 'window.scStorage.status === 3')
            saved = state(page)
            page.goto(base)
            wait_ready(page)
            loaded = state(page)
            assert loaded['flares'] == saved['flares'] and loaded['flare_life'] > 0
            assert loaded['flare_lifetime'] == 1 and loaded['flare_strength'] == 1
            assert not loaded['failed'] and not errors
            context.close()
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            page = mobile.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(base + '?seed=713&scene=bay')
            wait_ready(page)
            page.locator('#start').tap()
            expect(page, '!s.paused')
            page.locator('#menu').tap()
            page.locator("[data-action='15']").tap()
            expect(page, 's.mech_deployed')
            page.locator('#resume').tap()
            page.touchscreen.tap(844 * .90, 390 * .27)
            expect(page, 's.riding === 1')
            page.touchscreen.tap(844 * .77, 390 * .27)
            expect(page, 's.rig_weapon === 1')
            page.screenshot(path=str(ARTIFACTS / 'mobile-mech-hud.png'))
            assert not state(page)['failed'] and not errors
            mobile.close()
            browser.close()
            print('PASS equipment browser: mech deploy/mount/arsenal/cockpit/reload, wagon driving/braking, three personal beams, earned flare upgrades, attached lights/reload and touch deployment/boarding')
    except Exception:
        detail = traceback.format_exc().replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
        print('::error title=Equipment browser regression::' + detail, flush=True)
        raise
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
