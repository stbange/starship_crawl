"""Foundry routes and controls in the shipped WebGL application."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import json
import os
from pathlib import Path
import subprocess
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
            page.on('pageerror', lambda error: errors.append(str(error)))
            for scene in range(5):
                page.goto(base + f'?seed=713&scene=foundry{scene}')
                wait_ready(page)
                expect(page, 's.generator === 16 && s.district_family === 1 && s.district_condition === 1')
                if scene == 0:
                    native = Path(os.environ.get('SC_CORE_TEST', str(ROOT / 'build/sc_tests')))
                    for probe in json.loads(subprocess.check_output([str(native), '--fingerprints'])):
                        actual = page.evaluate("a => Module.ccall('sc_web_fingerprint','number',['number','number','number','number'],a) >>> 0", [probe['seed'], *probe['region']])
                        assert actual == probe['hash'], (probe, actual)
                page.screenshot(path=str(ARTIFACTS / f'coherent-foundry{scene}.png'))
                start_play(page)
                if scene == 3:
                    before = state(page)
                    page.keyboard.down('w')
                    expect(page, f's.z > {before["z"] + 4.2}')
                    page.keyboard.up('w')
                    page.keyboard.press('g')
                    expect(page, f's.x > {before["x"] + 40} && s.z > {before["z"] + 45}')
                elif scene == 4:
                    page.keyboard.press('g')
                    expect(page, 's.district_power && s.district_warning > 0')
                    page.screenshot(path=str(ARTIFACTS / 'coherent-foundry-pump.png'))
                assert not state(page)['failed'] and not errors, errors
            browser.close()
            print('PASS Foundry WebGL vistas, shared power, physical shortcut and native/WASM district parity')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
