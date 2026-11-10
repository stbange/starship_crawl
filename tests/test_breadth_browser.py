"""Inspect current district rules and all remaining earned branches in WebGL."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_browser import ROOT, ARTIFACTS, QuietHandler, start_play, state, wait_ready
from test_interiors_browser import expect


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=ROOT / 'build-pages-preview')
    parser.add_argument('--url-path', default='/starship_crawl/')
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
            page = browser.new_page(viewport={'width': 1280, 'height': 720})
            errors = []
            page.on('pageerror', lambda e: errors.append(str(e)))
            for seed, family, condition in ((713, 1, 1), (27, 2, 2), (3, 3, 2), (7, 4, 3)):
                for mode in (0, 5, 6):
                    page.goto(base + f'?seed={seed}&scene=foundry{mode}')
                    wait_ready(page)
                    expect(page, f's.district_family === {family} && s.district_condition === {condition}')
                    start_play(page)
                    page.keyboard.press('Escape')
                    page.screenshot(path=str(ARTIFACTS / f'coherent-district-{family}-{mode}.png'))
                    assert not state(page)['failed']
            for scene, branch, effect in ((4, 2, 's.relays > 0'), (5, 4, 's.mark_life > 5'),
                                         (6, 6, 's.brace_charge === 50'), (7, 7, 's.build_pulse > 0'),
                                         (8, 8, 's.build_pulse > 0')):
                page.goto(base + f'?seed=713&scene=build{scene}')
                wait_ready(page)
                expect(page, f's.module === {branch} && {effect}')
                start_play(page)
                page.screenshot(path=str(ARTIFACTS / f'coherent-build{scene}.png'))
            assert not errors, errors
            browser.close()
            print('PASS four district landmarks, condition warning/live views and eight-branch earned inventory / WebGL')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
