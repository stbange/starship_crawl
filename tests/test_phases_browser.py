"""Earned builds, guardian retry, mobile choices and fresh characters in WebGL."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, QuietHandler, start_play, state, wait_for, wait_ready
from test_interiors_browser import expect, face_forward, walk_axis
from test_story_browser import import_fixture, save


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
            page.on('dialog', lambda dialog: dialog.accept())
            page.goto(base + '?seed=713')
            wait_ready(page)
            expect(page, 's.generator === 11 && s.campaign === 1 && s.weapons === 3 && s.phase === 0 && s.programs === 7')
            start_play(page)
            page.keyboard.press('v')
            page.keyboard.press('t')
            expect(page, '!s.veil && !s.mech_deployed && s.energy === 100')
            page.keyboard.press('F10')
            expect(page, 's.paused && s.journal_page === 5 && s.gun_slots[0] === 0 && s.gun_slots[1] === 1')
            page.screenshot(path=str(ARTIFACTS / 'phase-initial-build.png'))
            for target in (1, 8, 9, 0):
                page.keyboard.press('u')
                expect(page, f's.guardian === {target}')
            page.keyboard.press('Enter')
            # These saves were reached by the ordinary-control C campaign driver.
            import_fixture(page, 'phase-reward.sav')
            page.keyboard.press('F10')
            expect(page, 's.reward_pending === 9 && s.claimed === 0')
            page.screenshot(path=str(ARTIFACTS / 'phase-reward-preview.png'))
            page.keyboard.press('1')
            expect(page, 's.claimed === 256 && s.weapons !== 3 && s.reward_pending === 0')
            chosen = state(page)
            page.keyboard.press('1')
            assert state(page)['weapons'] == chosen['weapons']
            page.keyboard.press('Enter')
            face_forward(page)
            current = state(page)
            walk_axis(page, 'x', 0, 'a' if current['x'] > 0 else 'd')
            walk_axis(page, 'z', 43, 's' if state(page)['z'] > 43 else 'w')
            page.keyboard.press('F10')
            page.keyboard.press('e')
            page.keyboard.press('z')
            expect(page, 's.build_slot === 1 && s.gun_slots[1] !== ' + str(chosen['gun_slots'][1]))
            save(page)
            page.goto(base)
            wait_ready(page)
            expect(page, f's.claimed === 256 && s.weapons === {chosen["weapons"]}')
            start_play(page)
            import_fixture(page, 'phase-attempt.sav')
            # Loading a live attempt resets it at its staging console.
            expect(page, '!s.guardian_active && s.health === 100 && s.defeated === 0')
            page.keyboard.press('F10')
            expect(page, 's.guardian === 8 && s.reward_pending === 0')
            page.keyboard.press('Enter')
            page.keyboard.press('g')
            expect(page, 's.guardian_active')
            expect(page, 's.enemy_shots > 0', timeout=15000)
            page.screenshot(path=str(ARTIFACTS / 'phase-guardian-combat.png'))
            import_fixture(page, 'phase-midship.sav')
            expect(page, 's.phase === 2 && s.rank > 1 && s.claimed !== 0')
            page.keyboard.press('m')
            expect(page, 's.map')
            page.screenshot(path=str(ARTIFACTS / 'phase-midship-map.png'))
            page.keyboard.press('m')
            import_fixture(page, 'phase-unbound.sav')
            expect(page, 's.phase === 4 && s.rank > 1')
            page.keyboard.press('F10')
            page.screenshot(path=str(ARTIFACTS / 'phase-complete-build.png'))
            page.keyboard.press('u')
            expect(page, 's.guardian === 1 && s.goal_x === -3 && s.phase === 4')
            page.keyboard.press('Enter')
            page.keyboard.press('n')
            page.keyboard.press('Enter')
            expect(page, 's.rank === 1 && s.weapons === 3 && s.phase === 0 && s.claimed === 0 && s.programs === 7 && s.powers_owned === 0 && s.rigs_owned === 0')
            import_fixture(page, 'generator-v6-format10.sav')
            expect(page, 's.generator === 6 && s.campaign === 0 && s.weapons !== 3')
            page.keyboard.press('n')
            page.keyboard.press('Enter')
            expect(page, 's.generator === 11 && s.campaign === 1 && s.weapons === 3')
            # Emulated mobile input uses the same earned reward and refit actions.
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            touch = mobile.new_page()
            touch.on('dialog', lambda dialog: dialog.accept())
            touch.goto(base + '?seed=713&touch=1')
            wait_ready(touch)
            start_play(touch)
            import_fixture(touch, 'phase-reward.sav')
            touch.locator('#menu').tap()
            touch.locator('[data-action="220"]').tap()
            expect(touch, 's.journal_page === 5 && s.reward_pending === 9')
            touch.screenshot(path=str(ARTIFACTS / 'phase-mobile-build.png'))
            touch.locator('#menu').tap()
            touch.locator('[data-action="227"]').tap()
            expect(touch, 's.claimed === 256 && (s.powers_owned > 0 || s.rigs_owned > 0)')
            touch.locator('[data-action="222"]').tap()
            expect(touch, 's.build_slot === 1')
            touch.locator('#save').tap()
            wait_for(touch, 'window.scStorage.status === 3')
            touch.goto(base)
            wait_ready(touch)
            expect(touch, 's.claimed === 256 && (s.powers_owned > 0 || s.rigs_owned > 0)')
            touch.set_viewport_size({'width': 390, 'height': 844})
            start_play(touch)
            touch.locator('#menu').tap()
            touch.locator('[data-action="220"]').tap()
            expect(touch, 's.journal_page === 5 && s.paused')
            touch.screenshot(path=str(ARTIFACTS / 'phase-mobile-portrait.png'))
            assert not errors, errors
            browser.close()
            print('PASS phased WebGL: fresh builds, keyboard routes/rewards/refit, retry, phase map, legacy/new ship and mobile reward persistence')
    finally:
        server.shutdown()


if __name__ == '__main__':
    run()
