"""Render earned branch fixtures and exercise their real input path in packaged WebGL."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, QuietHandler, start_play, state, wait_for, wait_ready
from test_interiors_browser import expect


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=BUILD)
    parser.add_argument('--url-path', default='/starship_crawl.html')
    parser.add_argument('--mobile-only', action='store_true')
    args = parser.parse_args()
    ARTIFACTS.mkdir(parents=True, exist_ok=True)
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(QuietHandler, directory=str(args.site_dir.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    base = f'http://127.0.0.1:{server.server_port}{args.url_path}'
    try:
        with sync_playwright() as p:
            options = {'headless': True, 'args': ['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader']}
            options.update({'executable_path': os.environ['SC_BROWSER']} if os.environ.get('SC_BROWSER') else {'channel': 'chromium'})
            browser = p.chromium.launch(**options)
            context = browser.new_context(viewport={'width': 1280, 'height': 720})
            page = context.new_page()
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            for scene, condition in ([] if args.mobile_only else [(1, 's.relays === 3 && s.beams === 1 && s.flares === 1'),
                                     (2, 's.echo_life > 2.9 && !s.veil'),
                                     (3, 's.mech_charge === 100 && s.riding === 1')]):
                page.goto(base + f'?seed=713&scene=build{scene}')
                wait_ready(page)
                expect(page, f's.rules === 2 && s.module === {scene*2-1} && {condition}')
                # Hide the launch overlay through its normal Start control; the test
                # then exercises a new action rather than keeping a frozen mock image.
                start_play(page)
                if scene == 1:
                    page.mouse.down()
                    expect(page, 's.relays > 0 && s.beams > 0')
                elif scene == 2:
                    page.keyboard.press('v')
                    expect(page, 's.veil')
                    page.mouse.down()
                    expect(page, '!s.veil && s.echo_life > 2.5')
                    position = state(page)['z']
                    page.keyboard.down('s')
                    try:
                        expect(page, f's.z < {position-2}')
                    finally:
                        page.keyboard.up('s')
                else:
                    page.mouse.down()
                    expect(page, 's.mech_charge === 0 && s.projectiles > 0')
                page.screenshot(path=str(ARTIFACTS / f'coherent-build{scene}.png'))
                page.mouse.up()
                if scene != 3:
                    page.keyboard.press('F10')
                    expect(page, 's.journal_page === 5')
                    page.screenshot(path=str(ARTIFACTS / f'coherent-build{scene}-preview.png'))
                    for _ in range(5):
                        page.keyboard.press('e')
                    page.keyboard.press('x')
                    expect(page, f's.module === {scene*2-1} && s.branch_preview === 1')
                    page.keyboard.press('g')
                    expect(page, 's.module === 0 && !s.branch_preview')
                assert not state(page)['failed'] and not errors
            page.goto(base + '?seed=713')
            wait_ready(page)
            expect(page, 's.weapons === 1 && s.rules === 2 && !s.modules_owned && !s.powers_owned && !s.rigs_owned')
            assert not errors
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            touch = mobile.new_page()
            touch.goto(base + '?seed=713&scene=build1&touch=1')
            wait_ready(touch)
            start_play(touch)
            touch.locator('#menu').tap()
            touch.locator('[data-action="220"]').tap()
            touch.locator('#menu').tap()
            for _ in range(5):
                touch.locator('[data-action="222"]').tap()
            touch.locator('[data-action="223"]').tap()
            expect(touch, 's.module === 1 && s.branch_preview === 1')
            touch.locator('[data-action="229"]').tap()
            expect(touch, 's.module === 0 && !s.branch_preview')
            touch.screenshot(path=str(ARTIFACTS / 'coherent-mobile-refit.png'))
            mobile.close()
            browser.close()
            print('PASS packaged mobile preview/equip and fresh reset' if args.mobile_only else 'PASS packaged beam relay, cloak echo, charged mech, desktop/mobile branch previews and fresh reset')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
