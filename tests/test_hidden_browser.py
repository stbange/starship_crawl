"""New ships hide politics/director controls; real saved convoys retain their identity."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import json
import math
import os
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, ROOT, QuietHandler, start_play, state, wait_ready, wait_for
from test_interiors_browser import expect
from test_regional_browser import aim_at
from test_habitat_browser import capture
from test_story_browser import import_fixture


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=BUILD)
    parser.add_argument('--url-path', default='/starship_crawl.html')
    parser.add_argument('--fixture', type=Path, default=ROOT / 'tests/fixtures/hidden-convoy-close.sav')
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
            page.on('pageerror', lambda e: errors.append(str(e)))
            page.on('dialog', lambda d: d.accept())
            page.goto(base + '?seed=713')
            wait_ready(page)
            start_play(page)
            s = state(page)
            assert s['generator'] == 15 and s['simulation'] == 1 and s['politics'] == 0, s
            assert s['director_kind'] == 0 and s['political_goal'] == 0, s
            page.keyboard.press('F6')
            expect(page, '!s.paused && !s.journal')
            page.keyboard.press('i')
            expect(page, 's.paused && s.journal_page===1')
            before = state(page)
            expect(page, f's.frames>{before["frames"]+5}')
            assert state(page)['strategy_tick'] == before['strategy_tick']
            page.keyboard.press('F6')
            assert state(page)['journal_page'] != 6
            wait_for(page, '!document.pointerLockElement')
            page.locator('#menu').click()
            assert not page.locator('#legacy-courts').is_visible()
            page.screenshot(path=str(ARTIFACTS / 'hidden-new-ship-menu.png'))
            resumed_at = state(page)['time']
            page.locator('#resume').click()
            expect(page, f'!s.paused && s.time>{resumed_at+.08}')
            import_fixture(page, str(args.fixture.resolve()))
            s = state(page)
            assert s['simulation'] == 1 and s['forces_launched'] > 0 and s['politics'] == 0, s
            # Older earned expeditions retain dossier access, without exposing the hidden courts.
            page.keyboard.press('F6')
            expect(page, 's.paused && s.journal_page===2')
            page.keyboard.press('F6')
            expect(page, 's.journal_page===2')
            page.keyboard.press('Enter')
            expect(page, '!s.paused')
            visible = [f for f in s['forces'] if f['physical'] and f['strength'] and
                       math.dist(f['at'], [s['x'], s['y'], s['z']]) < 25]
            assert visible, s
            convoy = next((f for f in visible if f['cargo']), visible[0])
            cursor = [640, 360]
            page.mouse.move(*cursor)
            aim_at(page, [convoy['at'][0], convoy['at'][1]+1.1, convoy['at'][2]], cursor)
            pixels = capture(page, 'hidden-convoy')
            assert pixels['lit'] > 80 and pixels['colored'] > 20, pixels
            observed = state(page)
            (ARTIFACTS / 'hidden-convoy-state.json').write_text(json.dumps(observed, indent=2) + '\n')
            page.keyboard.press('m')
            expect(page, 's.map')
            page.screenshot(path=str(ARTIFACTS / 'hidden-story-map.png'))
            assert state(page)['political_goal'] == 0 and state(page)['director_kind'] == 0
            page.keyboard.press('m')
            page.keyboard.press('F2')
            expect(page, 's.players===2')
            capture(page, 'hidden-split-convoy')
            page.keyboard.press('F5')
            wait_for(page, 'window.scStorage.status === 3')
            saved = state(page)
            page.goto(base)
            wait_ready(page)
            start_play(page)
            expect(page, 's.pending===0')
            loaded = state(page)
            assert loaded['players'] == 1 and loaded['forces_launched'] >= saved['forces_launched'], loaded
            assert loaded['force_losses'] >= saved['force_losses'], (saved, loaded)
            old = {f['id']: f for f in saved['forces']}
            for f in loaded['forces']:
                if f['id'] in old:
                    assert f['strength'] <= old[f['id']]['strength'], (old, f)
            page.keyboard.press('F6')
            expect(page, 's.paused && s.journal_page===2')
            paused = state(page)
            expect(page, f's.frames>{paused["frames"]+10}')
            assert state(page)['strategy_tick'] == paused['strategy_tick']
            context.close()
            mobile = browser.new_context(viewport={'width': 390, 'height': 844}, has_touch=True, is_mobile=True)
            touch = mobile.new_page()
            touch.goto(base + '?seed=42')
            wait_ready(touch)
            start_play(touch)
            touch.locator('#menu').tap()
            assert not touch.locator('#legacy-courts').is_visible()
            touch.screenshot(path=str(ARTIFACTS / 'hidden-mobile-menu.png'))
            assert not errors, errors
            browser.close()
            print('PASS hidden desktop/touch controls, normal dossier/map, earned convoy, co-op, identity-preserving resume and pause', flush=True)
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
