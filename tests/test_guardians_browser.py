"""Verify guardian machinery with shipped WebGL and optional emulated WebXR input."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_browser import ROOT, ARTIFACTS, QuietHandler, start_play, state, wait_ready
from test_interiors_browser import expect
from test_quest_browser import ready as xr_ready, enter, step, press, capture, xr_state


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
            options = {'headless': True, 'args': ['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader']}
            options.update({'executable_path': os.environ['SC_BROWSER']} if os.environ.get('SC_BROWSER') else {'channel': 'chromium'})
            browser = p.chromium.launch(**options)
            context = browser.new_context(viewport={'width': 1280, 'height': 720})
            page = context.new_page()
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            if args.quest:
                page.add_init_script(path=str(ROOT / 'tests/quest_device.js'))
                for pattern in range(2):
                    page.goto(base + '?seed=713')
                    xr_ready(page)
                    page.locator('#import').set_input_files(str(ROOT / f'tests/fixtures/coherent-guardian{pattern}.sav'))
                    page.wait_for_function("document.getElementById('status').textContent.includes('Save imported')")
                    enter(page)
                    press(page, 'right', 1)
                    assert state(page)['guardian_active'] and state(page)['guardian_stage'] == 1
                    step(page, 80)
                    capture(page, f'coherent-guardian{pattern}-stereo')
                    assert xr_state(page)['active'] and not errors
            else:
                for pattern in range(2):
                    page.goto(base + f'?seed=713&scene=guardian{pattern}warn')
                    wait_ready(page)
                    expect(page, 's.guardian_active && s.guardian_stage === 1 && s.guardian_broken === 0')
                    start_play(page)
                    page.screenshot(path=str(ARTIFACTS / f'coherent-guardian{pattern}-warning.png'))
                    page.mouse.down()
                    try:
                        expect(page, '(s.guardian_broken & 1) !== 0')
                    finally:
                        page.mouse.up()
                    page.screenshot(path=str(ARTIFACTS / f'coherent-guardian{pattern}-destroyed.png'))
                    assert state(page)['kills'] == 0 and not errors
                    page.goto(base + f'?seed=713&scene=guardian{pattern}breach')
                    wait_ready(page)
                    expect(page, f'!s.guardian_active && (s.defeated & {1 << pattern}) !== 0 && s.phase === 1')
                    start_play(page)
                    page.screenshot(path=str(ARTIFACTS / f'coherent-guardian{pattern}-breach.png'))
            browser.close()
            print('PASS guardian patterns, real input, assembly destruction and visible aftermath' + (' / stereo Quest emulation' if args.quest else ' / packaged WebGL'))
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
