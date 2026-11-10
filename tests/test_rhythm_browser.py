"""Physical traces and retained outcomes, without director controls or a narrative overlay."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_browser import ROOT, ARTIFACTS, QuietHandler, state, wait_ready, start_play
from test_interiors_browser import expect
from test_quest_browser import ready as xr_ready, enter, capture


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
            page = browser.new_page(viewport={'width': 1280, 'height': 720})
            errors = []
            page.on('pageerror', lambda e: errors.append(str(e)))
            if args.quest:
                page.add_init_script(path=str(ROOT / 'tests/quest_device.js'))
                page.goto(base + '?seed=713')
                xr_ready(page)
                page.locator('#import').set_input_files(str(ROOT / 'tests/fixtures/coherent-rhythm.sav'))
                page.wait_for_function("document.getElementById('status').textContent.includes('Save imported')")
                enter(page)
                assert state(page)['rhythm_facts'] == 1
                capture(page, 'coherent-rhythm-stereo')
            else:
                for kind in range(4):
                    page.goto(base + f'?seed=713&scene=rhythm{kind}')
                    wait_ready(page)
                    start_play(page)
                    expect(page, f's.rhythm_facts === {1 << kind} && !s.journal')
                    page.screenshot(path=str(ARTIFACTS / f'coherent-rhythm{kind}.png'))
            assert not errors, errors
            browser.close()
            print('PASS four physical sequence cues and retained field evidence' + (' / isolated Quest stereo' if args.quest else ' / WebGL'))
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
