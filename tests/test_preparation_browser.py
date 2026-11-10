"""Optional shelter facilities through actual shared keyboard, pad, touch and Quest actions."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_browser import ROOT, ARTIFACTS, QuietHandler, start_play, state, wait_ready
from test_interiors_browser import expect
from test_story_browser import import_fixture
from test_foundation_browser import VIRTUAL_PADS
from test_quest_browser import ready as xr_ready, enter, press, choose, capture


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=ROOT / 'build-pages-preview')
    parser.add_argument('--url-path', default='/starship_crawl/')
    parser.add_argument('--quest', action='store_true')
    args = parser.parse_args()
    ARTIFACTS.mkdir(parents=True, exist_ok=True)
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(QuietHandler, directory=str(args.site_dir.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    base = f'http://127.0.0.1:{server.server_port}{args.url_path}'
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True, executable_path=os.environ.get('SC_BROWSER', '/usr/bin/chromium'),
                args=['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'])
            context = browser.new_context(viewport={'width': 1280, 'height': 720})
            page = context.new_page()
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.on('dialog', lambda dialog: dialog.accept())
            if args.quest:
                page.add_init_script(path=str(ROOT / 'tests/quest_device.js'))
                page.goto(base + '?seed=713')
                xr_ready(page)
                page.locator('#import').set_input_files(str(ROOT / 'tests/fixtures/coherent-home1.sav'))
                page.wait_for_function("document.getElementById('status').textContent.includes('Save imported')")
                enter(page)
                press(page, 'right', 1)
                assert state(page)['story_decision']
                capture(page, 'coherent-home-meals-stereo')
                choose(page, 'INSULATED BROTH')
                assert state(page)['meal'] == 1 and state(page)['credits'] == 2970
                capture(page, 'coherent-home-prepared-stereo')
            else:
                page.add_init_script(VIRTUAL_PADS)
                page.goto(base + '?seed=713')
                wait_ready(page)
                start_play(page)
                import_fixture(page, 'coherent-home0.sav')
                page.keyboard.press('g')
                expect(page, 's.story_decision && s.credits === 0')
                page.keyboard.press('1')
                expect(page, 's.meal === 0 && s.credits === 0')
                page.screenshot(path=str(ARTIFACTS / 'coherent-home-zero.png'))
                import_fixture(page, 'coherent-home1.sav')
                page.keyboard.press('g')
                page.keyboard.press('1')
                expect(page, 's.meal === 1 && s.credits === 2970 && s.meal_seconds === 300')
                page.screenshot(path=str(ARTIFACTS / 'coherent-home-meal.png'))
                import_fixture(page, 'coherent-home2.sav')
                page.keyboard.press('g')
                page.evaluate('connectVirtualPad(0)')
                page.evaluate('Object.assign(virtualPads[0].buttons[15],{pressed:true,value:1})')
                expect(page, 's.home_research === 2 && s.credits === 2900')
                page.evaluate('Object.assign(virtualPads[0].buttons[15],{pressed:false,value:0})')
                page.keyboard.press('g')
                page.screenshot(path=str(ARTIFACTS / 'coherent-home-projects.png'))
                mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
                touch = mobile.new_page()
                touch.on('dialog', lambda dialog: dialog.accept())
                touch.goto(base + '?seed=713&touch=1')
                wait_ready(touch)
                start_play(touch)
                import_fixture(touch, 'coherent-home1.sav')
                touch.touchscreen.tap(844 * .90, 390 * .27)
                expect(touch, 's.story_decision')
                touch.locator('#menu').tap()
                touch.locator('[data-action="92"]').tap()
                expect(touch, 's.meal === 3 && s.credits === 2970')
                touch.screenshot(path=str(ARTIFACTS / 'coherent-home-mobile.png'))
            assert not errors, errors
            browser.close()
            print('PASS optional shelter costs, preparation and shared facility controls' + (' / isolated Quest stereo' if args.quest else ' / keyboard, pad and touch'))
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
