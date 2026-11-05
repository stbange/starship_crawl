"""Exercise hacking through packaged WASM, real input and played checkpoints."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import subprocess
import threading
import traceback
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, ROOT, QuietHandler, start_play, state, wait_for, wait_ready
from test_interiors_browser import expect, face_forward, walk_axis
from test_shelter_browser import move_to
from test_story_browser import import_fixture


def menu_action(page, action):
    page.locator('#menu').click()
    page.locator(f'[data-action="{action}"]').click()


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=BUILD)
    parser.add_argument('--url-path', default='/starship_crawl.html')
    args = parser.parse_args()
    ARTIFACTS.mkdir(parents=True, exist_ok=True)
    checkpoints = ARTIFACTS / 'matrix-checkpoints'
    checkpoints.mkdir(exist_ok=True)
    driver = Path(os.environ.get('SC_MATRIX_JOURNEY', ROOT / 'build/sc_matrix_journey'))
    if not driver.exists():
        driver = ROOT / 'build-parity/sc_matrix_journey'
    subprocess.run([str(driver), '--checkpoints', str(checkpoints)], check=True, cwd=ROOT)
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
            if os.environ.get('SC_CPU_THROTTLE'):
                context.new_cdp_session(page).send('Emulation.setCPUThrottlingRate', {'rate': float(os.environ['SC_CPU_THROTTLE'])})
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.on('dialog', lambda dialog: dialog.accept())
            page.goto(base + '?seed=713&scene=bay')
            wait_ready(page)
            start_play(page)
            face_forward(page)
            walk_axis(page, 'x', 3, 'd')
            walk_axis(page, 'z', 47, 'w')
            body = state(page)
            assert body['matrix_near']
            page.screenshot(path=str(ARTIFACTS / 'matrix-terminal.png'))
            page.keyboard.press('c')
            expect(page, 's.matrix && s.matrix_nodes.length >= 3')
            before = state(page)
            page.screenshot(path=str(ARTIFACTS / 'matrix-cathedral.png'))
            page.keyboard.press('f')
            expect(page, '(s.systems & 1) !== 0 && s.matrix_data > 0')
            paid = state(page)['credits']
            page.keyboard.press('f')
            assert state(page)['credits'] == paid
            page.keyboard.press('m')
            expect(page, 's.matrix_map')
            preview = state(page)
            page.keyboard.press('1')
            page.keyboard.press('w')
            page.screenshot(path=str(ARTIFACTS / 'matrix-schematic.png'))
            now = state(page)
            assert now['matrix_seen'] == preview['matrix_seen'] and now['story_seen'] == preview['story_seen']
            assert abs(now['mz'] - preview['mz']) < .1 and now['matrix_veil'] == 0
            page.keyboard.press('Escape')
            expect(page, '!s.matrix_map && !s.paused')
            page.keyboard.press('1')
            expect(page, 's.matrix_veil > 0')
            page.mouse.down()
            try:
                expect(page, f"s.shots > {before['shots']} && s.matrix_shots > 0 && s.matrix_veil === 0")
            finally:
                page.mouse.up()
            page.screenshot(path=str(ARTIFACTS / 'matrix-pulse.png'))
            page.keyboard.press('F8')
            expect(page, 's.paused && s.journal_page === 4')
            page.keyboard.press('x')
            page.keyboard.press('2')
            expect(page, 's.slot1 === 1')
            page.screenshot(path=str(ARTIFACTS / 'matrix-programs.png'))
            page.keyboard.press('Enter')
            page.keyboard.press('2')
            page.keyboard.press('F2')
            expect(page, 's.players === 2 && s.matrix')
            page.screenshot(path=str(ARTIFACTS / 'matrix-split.png'))
            page.keyboard.press('F2')
            page.keyboard.press('F5')
            wait_for(page, 'window.scStorage.status === 3')
            saved = state(page)
            page.goto(base)
            wait_ready(page)
            loaded = state(page)
            assert loaded['matrix'] and loaded['slot1'] == 1 and loaded['players'] == 1
            assert abs(loaded['mx'] - saved['mx']) < .1 and abs(loaded['mz'] - saved['mz']) < .1
            start_play(page)
            page.keyboard.press('c')
            expect(page, '!s.matrix')
            returned = state(page)
            assert abs(returned['x'] - body['x']) < .1 and abs(returned['z'] - body['z']) < .1
            assert abs(returned['yaw'] - body['yaw']) < .01
            # These saves were reached by normal movement/combat in the core journey.
            for choice in (1, 2, 3):
                import_fixture(page, checkpoints / f'matrix-choice{choice}.sav')
                expect(page, 's.matrix && !s.paused')
                page.keyboard.press('g')
                expect(page, 's.matrix_choice && s.paused')
                page.screenshot(path=str(ARTIFACTS / f'matrix-controller{choice}.png'))
                page.keyboard.press(str(choice))
                expect(page, f's.matrix_outcome === {choice} && !s.paused')
                data = state(page)['matrix_data']
                page.keyboard.press('g')
                assert state(page)['matrix_data'] == data
                page.keyboard.press('F5')
                wait_for(page, 'window.scStorage.status === 3')
                page.goto(base)
                wait_ready(page)
                assert state(page)['matrix_outcome'] == choice
                start_play(page)
            # Old format-9 files retain the physical expedition and start a fresh network.
            import_fixture(page, ROOT / 'tests/fixtures/generator-v6-format9.sav')
            assert not state(page)['matrix'] and state(page)['programs'] == 7
            page.close()
            context.close()
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, has_touch=True, is_mobile=True)
            page = mobile.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.on('dialog', lambda dialog: dialog.accept())
            page.goto(base + '?seed=713&scene=shelter')
            wait_ready(page)
            start_play(page)
            # The shelter inspection starts inside; walk to its actual workstation.
            expect(page, 's.sheltered')
            move_to(page, 3, -5)
            menu_action(page, 200)
            expect(page, 's.matrix && !s.paused')
            page.touchscreen.tap(844 * .64, 390 * .48)
            expect(page, '(s.systems & 1) !== 0')
            page.touchscreen.tap(844 * .51, 390 * .27)
            expect(page, 's.matrix_veil > 0')
            page.touchscreen.tap(844 * .51, 390 * .1)
            expect(page, 's.matrix_map')
            protected = state(page)
            page.touchscreen.tap(844 * .51, 390 * .27)
            assert state(page)['matrix_veil'] <= protected['matrix_veil']
            page.screenshot(path=str(ARTIFACTS / 'matrix-touch.png'))
            page.touchscreen.tap(844 * .51, 390 * .1)
            expect(page, '!s.matrix_map')
            page.touchscreen.tap(844 * .64, 390 * .27)
            expect(page, '!s.matrix && s.sheltered')
            assert not errors, errors
            assert not state(page)['failed']
            browser.close()
            print('PASS packaged matrix controls, power, programs, schematic, co-op, active saves, 3 outcomes, legacy and touch')
    except Exception:
        traceback.print_exc()
        raise
    finally:
        server.shutdown()


if __name__ == '__main__':
    run()
