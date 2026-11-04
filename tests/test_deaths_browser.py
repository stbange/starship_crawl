"""Actual keyboard/mouse/touch combat, robot deaths and finite death hazards in WebGL."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
import traceback
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, QuietHandler, start_play, state, wait_ready
from test_interiors_browser import expect, face_forward


def equip(page):
    if page.evaluate('!!document.pointerLockElement'):
        page.keyboard.press('Escape')
        expect(page, 's.paused')
    page.locator('#menu').click()
    page.locator('[data-action="57"]').click()
    page.locator('#resume').click()
    expect(page, 's.weapon === 17 && !s.paused')


def fire_until(page, condition):
    page.mouse.down()
    try:
        expect(page, condition, timeout=20000)
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

            page.goto(base + '?seed=713&scene=form0')
            wait_ready(page)
            assert state(page)['generator'] == 6
            start_play(page)
            equip(page)
            face_forward(page)
            page.mouse.move(640, 410)
            fire_until(page, '(s.death_forms & 1) !== 0 && s.shots > 0')
            page.screenshot(path=str(ARTIFACTS / 'death-husk-collapse.png'))
            page.keyboard.press('Escape')
            expect(page, 's.paused && s.corpses > 0')
            first = state(page)
            expect(page, f's.frames > {first["frames"] + 3}')
            assert state(page)['death_age'] == first['death_age']
            assert state(page)['kills'] == first['kills']
            page.keyboard.press('Escape')
            page.keyboard.down('s')
            try:
                expect(page, 's.corpses === 0 && s.death_age === 0', timeout=20000)
            finally:
                page.keyboard.up('s')
            print('PASS finite corpse animation, pause and duplicate-reward refusal', flush=True)

            page.goto(base + '?seed=713&scene=form1')
            wait_ready(page)
            start_play(page)
            equip(page)
            face_forward(page)
            page.mouse.move(640, 410)
            fire_until(page, '(s.hazard_kinds & 2) !== 0')
            page.screenshot(path=str(ARTIFACTS / 'death-acid-residue.png'))
            page.keyboard.press('Escape')
            expect(page, 's.paused && (s.death_forms & 2) !== 0 && s.hazards > 0')
            print('PASS organic death and acid residue through real beam fire', flush=True)

            page.goto(base + '?seed=713&scene=form5')
            wait_ready(page)
            start_play(page)
            page.screenshot(path=str(ARTIFACTS / 'death-five-machines.png'))
            equip(page)
            face_forward(page)
            page.mouse.move(640, 410)
            fire_until(page, '(s.hazard_kinds & 8) !== 0 && (s.death_forms & 32) !== 0')
            page.screenshot(path=str(ARTIFACTS / 'death-robot-wreck-and-mine.png'))
            page.keyboard.press('Escape')
            expect(page, 's.paused && s.hazards > 0')
            page.keyboard.press('Escape')
            page.keyboard.press('v')
            expect(page, 's.veil')
            # Drive ordinary keyboard listeners from RAF so runner latency cannot consume a warning.
            page.evaluate("""() => {
              window.mineTriggered=false;window.mineBurst=false;window.mineBackDone=false;
              const key=(type,k,n)=>window.dispatchEvent(new KeyboardEvent(type,{key:k,code:'Key'+k.toUpperCase(),keyCode:n,which:n,bubbles:true}));
              let retreatAt=null;
              const step=()=>{const s=JSON.parse(Module.ccall('sc_web_state','string',[],[]));
                if(s.hazard_triggered && retreatAt===null){window.mineTriggered=true;retreatAt=s.time;key('keyup','w',87);key('keydown','s',83);}
                if(s.hazard_spent)window.mineBurst=true;
                if(retreatAt!==null && s.time>retreatAt+1.25){key('keyup','s',83);window.mineBackDone=true;return;}
                if(retreatAt===null && s.z>34){key('keyup','w',87);window.mineBackDone=true;return;}
                requestAnimationFrame(step);};requestAnimationFrame(step);
            }""")
            page.keyboard.down('w')
            try:
                page.wait_for_function('window.mineBackDone', timeout=20000)
            finally:
                page.keyboard.up('w')
                page.keyboard.up('s')
            assert page.evaluate('window.mineTriggered && window.mineBurst'), state(page)
            assert state(page)['respawns'] == 0
            page.screenshot(path=str(ARTIFACTS / 'death-mine-dodge.png'))
            print('PASS robot wrecks, dropped mine, cloak detection and moving out of a triggered blast', flush=True)

            context.close()
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            page = mobile.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(base + '?seed=713&scene=form1&touch=1')
            wait_ready(page)
            start_play(page)
            equip(page)
            client = mobile.new_cdp_session(page)
            client.send('Input.dispatchTouchEvent', {'type': 'touchStart', 'touchPoints': [{'id': 1, 'x': 760, 'y': 280}]})
            try:
                expect(page, '(s.hazard_kinds & 2) !== 0 && s.shots > 0', timeout=20000)
            finally:
                client.send('Input.dispatchTouchEvent', {'type': 'touchEnd', 'touchPoints': []})
            page.screenshot(path=str(ARTIFACTS / 'death-mobile-combat.png'))
            assert not errors, errors
            mobile.close()
            browser.close()
        print('PASS death browser: animated corpses, acid, machine silhouettes/wrecks, armed mine and dodge, keyboard/mouse and touch combat')
    except Exception:
        try:
            (ARTIFACTS / 'death-failure-state.txt').write_text(str(state(page)))
            page.screenshot(path=str(ARTIFACTS / 'death-failure.png'))
        except Exception:
            pass
        traceback.print_exc()
        raise
    finally:
        server.shutdown()


if __name__ == '__main__':
    run()
