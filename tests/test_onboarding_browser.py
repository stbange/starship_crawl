"""Progressive controls, optional instruction and an earned first matrix link in WebGL."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import json
import os
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, QuietHandler, start_play, state, wait_ready, wait_for
from test_interiors_browser import expect
from test_story_browser import import_fixture, save
from test_foundation_browser import VIRTUAL_PADS


def open_menu(page):
    if not state(page)['paused']:
        page.keyboard.press('Escape')
        expect(page, 's.paused')
    wait_for(page, '!document.pointerLockElement && !window.scPointer?.pending')
    if not page.locator('#drawer').is_visible():
        page.locator('#menu').click()


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
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True, executable_path=os.environ.get('SC_BROWSER', '/usr/bin/chromium'),
                args=['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'])
            context = browser.new_context(viewport={'width': 1280, 'height': 720})
            page = context.new_page()
            page.add_init_script(VIRTUAL_PADS)
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.on('dialog', lambda dialog: dialog.accept())
            page.goto(base + '?seed=713')
            wait_ready(page)
            expect(page, 's.generator === 15 && s.weapons === 1 && s.programs === 1 && s.capabilities === 0')
            start_play(page)
            # Retain five simulated seconds even when several software-rendered suites share the CPU.
            expect(page, 's.lesson === 1 && s.time > 5', timeout=90000)
            page.screenshot(path=str(ARTIFACTS / 'onboarding-arrival.png'))
            page.keyboard.press('Backspace')
            expect(page, 's.lesson === 0 && (s.lessons & 2) !== 0')
            open_menu(page)
            for action in (13, 15, 16, 60, 84, 200, 206):
                assert not page.locator(f'[data-action="{action}"]').is_visible(), action
            assert page.locator('[data-action="14"]').is_visible()
            page.locator('#preferences summary').click()
            page.locator('#pref-guidance').select_option('2')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_preferences','string',[],[])).views[0][6] === 2")
            page.screenshot(path=str(ARTIFACTS / 'onboarding-menu.png'))
            page.locator('#resume').click()
            for key in ('r', 'c', 'v', 't', 'F10', 'F8', 'F4'):
                page.keyboard.press(key)
            expect(page, '!s.paused && !s.journal && !s.shelter_deployed && !s.matrix && !s.veil && !s.mech_deployed && s.guidance === 2')
            page.keyboard.press('/')
            expect(page, 's.paused && s.journal_page === 7')
            page.screenshot(path=str(ARTIFACTS / 'onboarding-field-guide.png'))
            page.evaluate('connectVirtualPad(0)')
            def pad_press(index):
                before = state(page)['frames']
                page.evaluate('i=>Object.assign(virtualPads[0].buttons[i],{pressed:true,value:1})', index)
                expect(page, f's.frames > {before+3}')
                page.evaluate('i=>Object.assign(virtualPads[0].buttons[i],{pressed:false,value:0})', index)
                expect(page, f's.frames > {before+6}')
            pad_press(5)
            expect(page, 's.journal_page === 7')
            pad_press(9)
            expect(page, '!s.paused')
            page.keyboard.press('i')
            expect(page, 's.journal_page === 1 && s.paused')
            pad_press(5)
            expect(page, 's.journal_page === 1')
            pad_press(9)
            expect(page, '!s.paused')
            save(page)
            page.goto(base)
            wait_ready(page)
            expect(page, 's.guidance === 2 && (s.lessons & 2) !== 0')
            start_play(page)
            page.keyboard.press('n')
            page.keyboard.press('Enter')
            expect(page, 's.guidance === 2 && s.weapons === 1 && s.intro === 0 && s.lesson === 0')
            assert page.evaluate("JSON.parse(Module.ccall('sc_web_preferences','string',[],[])).views[1][6]") == 0

            # This checkpoint is earned by sc_phase_journey --onboarding, without grants or teleports.
            import_fixture(page, 'onboarding-interface.sav')
            expect(page, 's.generator === 15 && s.intro === 1 && s.programs === 1 && !s.matrix')
            page.keyboard.press('F8')
            expect(page, '!s.paused && !s.journal')
            assert 200 in state(page)['actions'] and 206 not in state(page)['actions']
            page.keyboard.press('g')
            expect(page, 's.matrix && s.programs === 1')
            page.screenshot(path=str(ARTIFACTS / 'onboarding-first-link.png'))
            page.keyboard.press('g')
            expect(page, '!s.matrix && s.intro === 3 && s.programs === 7 && (s.systems & 1) !== 0')
            page.screenshot(path=str(ARTIFACTS / 'onboarding-restored-power.png'))
            save(page)
            page.goto(base)
            wait_ready(page)
            expect(page, 's.intro === 3 && s.guidance === 2 && s.programs === 7')
            start_play(page)
            open_menu(page)
            assert page.locator('[data-action="13"]').is_visible()
            assert page.locator('[data-action="200"]').is_visible()
            page.locator('#resume').click()
            import_fixture(page, 'phase-unbound.sav')
            expect(page, 's.generator === 7 && (s.capabilities & 3) === 3 && s.programs === 7')
            assert not errors, errors
            context.close()

            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            page = mobile.new_page()
            page.goto(base + '?seed=42')
            wait_ready(page)
            start_play(page)
            expect(page, 's.weapons === 1 && s.capabilities === 0')
            assert page.evaluate('navigator.maxTouchPoints > 0')
            page.screenshot(path=str(ARTIFACTS / 'onboarding-mobile.png'))
            open_menu(page)
            assert not page.locator('[data-action="13"]').is_visible()
            page.locator('#preferences summary').click()
            page.locator('#pref-guidance').select_option('1')
            expect(page, 's.guidance === 1')
            page.locator('#resume').click()
            page.set_viewport_size({'width': 390, 'height': 844})
            page.screenshot(path=str(ARTIFACTS / 'onboarding-mobile-portrait.png'))
            mobile.close()
            browser.close()
            (ARTIFACTS / 'onboarding-results.json').write_text(json.dumps({
                'checks': ['fresh loadout', 'progressive menus', 'dismiss/review', 'preferences and save/import independence',
                           'earned matrix introduction', 'physical power restoration', 'legacy access', 'mobile guidance'],
                'physical_devices_tested': False}, indent=2) + '\n')
            print('PASS staged onboarding WebGL: progressive controls, optional saved guidance, earned safe matrix link, legacy and touch')
    finally:
        server.shutdown()


if __name__ == '__main__':
    run()
