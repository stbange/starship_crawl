"""Play P0 navigation/lift controls in the packaged WebGL game, using real input."""
from functools import partial
from http.server import ThreadingHTTPServer
from pathlib import Path
import argparse
import json
import os
import threading
from playwright.sync_api import sync_playwright
from test_browser import BUILD, ARTIFACTS, QuietHandler, state, wait_ready, wait_for, start_play
from test_foundation_browser import controls, expect_controls
from test_story_browser import import_fixture


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
            context = browser.new_context(viewport={'width': 1280, 'height': 720}, accept_downloads=True)
            page = context.new_page()
            errors = []
            page.on('pageerror', lambda e: errors.append(str(e)))
            page.on('dialog', lambda dialog: dialog.accept())
            page.goto(base + '?seed=713&scene=lift')
            wait_ready(page)
            start_play(page)
            page.keyboard.press('F2')
            page.keyboard.down('e')
            expect_controls(page, 'c.transit')
            expect_controls(page, '!c.transit && c.progress===1')
            arrived = state(page)
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).time > " + str(arrived['time'] + 2))
            held = state(page)
            assert abs(held['y'] - 64.02) < .05 and held['lift_latched'], held
            assert abs(controls(page)['p2'][1]) < .05
            assert held['nav_kind'] == 3 and held['nav2_kind'] == 4, held
            page.screenshot(path=str(ARTIFACTS / 'guidance-held-lift-split.png'))
            page.keyboard.up('e')
            wait_for(page, "!JSON.parse(Module.ccall('sc_web_state','string',[],[])).lift_latched")
            page.keyboard.down('q')
            expect_controls(page, 'c.transit && c.deck===0')
            page.keyboard.up('q')
            expect_controls(page, '!c.transit && c.progress===1')
            page.keyboard.press('F2')
            page.keyboard.press('m')
            mapped = state(page)
            page.keyboard.press('PageUp')
            page.keyboard.press('ArrowRight')
            after = state(page)
            assert after['generated'] == mapped['generated'] and after['nav_searched'] <= 256
            page.screenshot(path=str(ARTIFACTS / 'guidance-deck-map.png'))
            page.keyboard.press('m')
            # Import an earned phase checkpoint to exercise portal guidance and compact upgrades.
            from test_browser import ROOT
            page.goto(base + '?seed=713')
            wait_ready(page)
            start_play(page)
            import_fixture(page, ROOT / 'tests/fixtures/phase-unbound.sav')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).nav_kind > 0")
            page.keyboard.press('F10')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).journal_page === 5")
            page.keyboard.press('u')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).goal_x === -3")
            page.keyboard.press('Enter')
            before = state(page)['time']
            wait_for(page, "() => {const s=JSON.parse(Module.ccall('sc_web_state','string',[],[])); return !s.paused && s.nav_kind === 2 && s.time > " + str(before + .1) + ";}")
            route = state(page)
            assert route['nav_searched'] <= 256 and route['nav_next'][0] != route['goal_x'], route
            (ARTIFACTS / 'guidance-route-state.json').write_text(json.dumps(route, indent=2))
            page.screenshot(path=str(ARTIFACTS / 'guidance-expedition-hud.png'))
            page.keyboard.press('m')
            page.screenshot(path=str(ARTIFACTS / 'guidance-route-map.png'))
            page.keyboard.press('m')
            for size in ({'width': 844, 'height': 390}, {'width': 390, 'height': 844}):
                page.set_viewport_size(size)
                page.screenshot(path=str(ARTIFACTS / f"guidance-hud-{size['width']}.png"))
                assert not state(page)['failed']
            assert not errors, errors
            context.close()
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            page = mobile.new_page()
            page.goto(base + '?seed=713&scene=lift')
            wait_ready(page)
            start_play(page)
            # The same visible UP button exercised by the foundation suite.
            client = mobile.new_cdp_session(page)
            client.send('Input.dispatchTouchEvent', {'type': 'touchStart', 'touchPoints': [{'x': 760, 'y': 187, 'id': 1}]})
            expect_controls(page, 'c.transit && c.deck===1')
            expect_controls(page, '!c.transit && c.progress===1')
            arrived = state(page)
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).time > " + str(arrived['time'] + 2))
            assert abs(state(page)['y'] - 64.02) < .05
            client.send('Input.dispatchTouchEvent', {'type': 'touchEnd', 'touchPoints': []})
            page.screenshot(path=str(ARTIFACTS / 'guidance-mobile-landing.png'))
            page.set_viewport_size({'width': 390, 'height': 844})
            page.screenshot(path=str(ARTIFACTS / 'guidance-mobile-portrait.png'))
            assert not state(page)['failed']
            mobile.close()
            browser.close()
            print('PASS navigation WebGL: held lift, release and return, independent co-op, bounded maps and responsive HUD')
    finally:
        server.shutdown()


if __name__ == '__main__':
    run()
