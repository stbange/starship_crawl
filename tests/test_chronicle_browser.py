"""Optional inhabitant choices through keyboard, controller and touch."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_browser import ROOT, ARTIFACTS, QuietHandler, start_play, state, wait_ready
from test_interiors_browser import expect
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
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, executable_path=os.environ.get('SC_BROWSER', '/usr/bin/chromium'),
                args=['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'])
            context = browser.new_context(viewport={'width': 1280, 'height': 720})
            page = context.new_page()
            if args.quest:
                page.add_init_script(path=str(ROOT / 'tests/quest_device.js'))
                page.goto(base + '?seed=713')
                xr_ready(page)
                page.locator('#import').set_input_files(str(ROOT / 'tests/fixtures/coherent-person0.sav'))
                page.wait_for_function("document.getElementById('status').textContent.includes('Save imported')")
                enter(page)
                press(page, 'right', 1)
                assert state(page)['story_decision'] and state(page)['person'] == 0
                capture(page, 'coherent-refuge-choices-stereo')
                choose(page, 'STRIP 90 SALVAGE')
                assert state(page)['person_fate'] == 4 and state(page)['credits'] == 90
                capture(page, 'coherent-refuge-aftermath-stereo')
                browser.close()
                print('PASS isolated Quest inhabitant grip interaction, declared choices and stereo aftermath')
                return
            page.add_init_script(VIRTUAL_PADS)
            errors = []
            page.on('pageerror', lambda e: errors.append(str(e)))
            page.goto(base + '?seed=713&scene=person0')
            wait_ready(page)
            start_play(page)
            expect(page, 's.person === 0 && s.person_knowledge === 0')
            page.screenshot(path=str(ARTIFACTS / 'coherent-engineer.png'))
            page.keyboard.press('g')
            expect(page, 's.paused && s.story_decision && s.journal_page === 1')
            page.screenshot(path=str(ARTIFACTS / 'coherent-refuge-options.png'))
            page.keyboard.press('1')
            expect(page, '!s.paused && !s.story_decision && s.person_knowledge === 1')
            page.keyboard.press('g')
            page.evaluate('connectVirtualPad(0)')
            before = state(page)['frames']
            page.evaluate('Object.assign(virtualPads[0].buttons[12],{pressed:true,value:1})')
            expect(page, f's.frames > {before+4} && s.person_fate === 4 && !s.story_decision')
            page.evaluate('Object.assign(virtualPads[0].buttons[12],{pressed:false,value:0})')
            assert state(page)['credits'] == 90
            page.screenshot(path=str(ARTIFACTS / 'coherent-refuge-abandoned.png'))
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            touch = mobile.new_page()
            touch.goto(base + '?seed=713&scene=person1&touch=1')
            wait_ready(touch)
            start_play(touch)
            touch.touchscreen.tap(844 * .90, 390 * .27)
            expect(touch, 's.story_decision && s.person === 1')
            touch.locator('#menu').tap()
            touch.locator('[data-action="91"]').tap()
            expect(touch, 's.person_fate === 4 && !s.story_decision')
            assert state(touch)['credits'] == 90
            touch.screenshot(path=str(ARTIFACTS / 'coherent-mobile-refuge.png'))
            assert not errors, errors
            browser.close()
            print('PASS recurring people, declared costs, free record, controller choice and mobile shared action')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
