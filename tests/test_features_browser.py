"""Exercise navigation, body variants, projectiles and psionics in the real game."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
import traceback
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, QuietHandler, start_play, state, wait_for, wait_ready


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
            for form in range(5):
                page.goto(base + f'?seed=713&scene=form{form}')
                wait_ready(page)
                mask = 31 << (form * 5)
                wait_for(page, f"(JSON.parse(Module.ccall('sc_web_state','string',[],[])).bodies & {mask}) === {mask}")
                start_play(page)
                page.screenshot(path=str(ARTIFACTS / f'body-family-{form}.png'))
                assert not state(page)['failed']
            page.goto(base + '?seed=713&scene=bay')
            wait_ready(page)
            start_play(page)
            page.screenshot(path=str(ARTIFACTS / 'minimap-objective.png'))
            page.keyboard.press('m')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).map")
            origin = state(page)
            page.keyboard.press('ArrowRight')
            page.keyboard.press('PageUp')
            wait_for(page, f"JSON.parse(Module.ccall('sc_web_state','string',[],[])).map_x === {origin['map_x'] + 1} && JSON.parse(Module.ccall('sc_web_state','string',[],[])).map_y === {origin['map_y'] + 1}")
            page.screenshot(path=str(ARTIFACTS / 'deck-map.png'))
            page.keyboard.press('Home')
            page.keyboard.press('Escape')
            assert not state(page)['map'] and not state(page)['paused']
            page.keyboard.press('v')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).veil")
            page.screenshot(path=str(ARTIFACTS / 'psionic-veil.png'))
            shots = state(page)['shots']
            page.mouse.down()
            try:
                wait_for(page, f"JSON.parse(Module.ccall('sc_web_state','string',[],[])).shots > {shots} && !JSON.parse(Module.ccall('sc_web_state','string',[],[])).veil && JSON.parse(Module.ccall('sc_web_state','string',[],[])).projectiles > 0")
                page.screenshot(path=str(ARTIFACTS / 'visible-projectiles.png'))
            finally:
                page.mouse.up()
            page.keyboard.press('b')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).flight > 0")
            floor = state(page)['y']
            page.keyboard.down('Space')
            try:
                wait_for(page, f"JSON.parse(Module.ccall('sc_web_state','string',[],[])).y > {floor + 2.5}")
            finally:
                page.keyboard.up('Space')
            page.screenshot(path=str(ARTIFACTS / 'psionic-flight.png'))
            page.keyboard.press('b')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).flight === 0 && JSON.parse(Module.ccall('sc_web_state','string',[],[])).y < .2", timeout=15000)
            page.keyboard.press('j')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).leap > 0")
            page.keyboard.down('Space')
            try:
                wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).y > 2")
            finally:
                page.keyboard.up('Space')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).y < .2", timeout=15000)
            page.keyboard.press('h')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).nova > 0")
            page.screenshot(path=str(ARTIFACTS / 'psionic-nova.png'))
            page.keyboard.press('F5')
            wait_for(page, 'window.scStorage.status === 3')
            saved = state(page)
            page.goto(base)
            wait_ready(page)
            loaded = state(page)
            assert loaded['seed'] == saved['seed'] and loaded['leap'] > 0 and loaded['nova'] > 0
            assert not loaded['map'] and not loaded['failed']
            start_play(page)
            page.keyboard.press('r')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).shelter_deployed")
            page.keyboard.press('r')
            wait_for(page, "!JSON.parse(Module.ccall('sc_web_state','string',[],[])).shelter_deployed")
            page.keyboard.press('Escape')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).paused && !document.pointerLockElement")
            page.locator('#menu').click()
            page.locator("[data-action='14']").click()
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).map && !JSON.parse(Module.ccall('sc_web_state','string',[],[])).paused")
            page.keyboard.press('m')
            assert not errors, errors
            context.close()
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            page = mobile.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(base + '?seed=713&scene=bay')
            wait_ready(page)
            page.locator('#start').tap()
            page.touchscreen.tap(844 * .51, 390 * .27)
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).veil")
            page.touchscreen.tap(844 * .51, 390 * .48)
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).power === 1")
            page.touchscreen.tap(844 * .51, 390 * .27)
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).flight > 0")
            page.touchscreen.tap(844 * .51, 390 * .1)
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).map")
            page.screenshot(path=str(ARTIFACTS / 'mobile-map-powers.png'))
            page.touchscreen.tap(844 * .51, 390 * .27)
            assert state(page)['flight'] > 0  # Covered CAST control must not cancel it.
            page.touchscreen.tap(844 * .51, 390 * .1)
            wait_for(page, "!JSON.parse(Module.ccall('sc_web_state','string',[],[])).map")
            assert not state(page)['failed'] and not errors
            mobile.close()
            browser.close()
            print('PASS features browser: 25 body variants, HUD/deck maps and objective, pan/deck/recenter, visible shots reveal veil, flight, charged jump, nova, v5 reload, shelter toggle, mobile powers/map')
    except Exception:
        detail = traceback.format_exc().replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
        print('::error title=Navigation and psionics browser regression::' + detail, flush=True)
        raise
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
