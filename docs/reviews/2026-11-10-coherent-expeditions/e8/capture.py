"""Capture restored earned checkpoints from this review's packaged browser game."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import json
import os
from pathlib import Path
import sys
import threading

ROOT = Path(__file__).resolve().parents[4]
REVIEW = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'tests'))
from playwright.sync_api import sync_playwright
from test_browser import QuietHandler, start_play, state, wait_ready, wait_for
from test_story_browser import import_fixture


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=ROOT / 'build-pages-preview')
    args = parser.parse_args()
    output = REVIEW / 'captures'
    output.mkdir(exist_ok=True)
    server = ThreadingHTTPServer(('127.0.0.1', 0),
                                partial(QuietHandler, directory=str(args.site_dir.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    cases = [('machines-earned', 'after/run-00-on/unbound.sav', False),
             ('vents-earned', 'after/run-01-on/guardian-fight.sav', False),
             ('conduits-earned', 'after/run-04-on/optional-fight.sav', False),
             ('rejoined-party', 'extras/run-07-coop/minute-30.sav', True)]
    records = []
    try:
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(
                headless=True, executable_path=os.environ.get('SC_BROWSER'),
                args=['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'])
            for name, checkpoint, split in cases:
                page = browser.new_page(viewport={'width': 1280, 'height': 720})
                errors = []
                page.on('dialog', lambda dialog: dialog.accept())
                page.on('pageerror', lambda error: errors.append(str(error)))
                page.goto(f'http://127.0.0.1:{server.server_port}/starship_crawl/?seed=713')
                wait_ready(page)
                start_play(page)
                import_fixture(page, str(REVIEW / 'data' / checkpoint))
                wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).pending === 0")
                if split:
                    page.keyboard.press('F2')
                    wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).players === 2")
                wait_for(page, "(() => { const s = JSON.parse(Module.ccall('sc_web_state','string',[],[])); return !s.paused && s.pending === 0; })()")
                page.evaluate('() => new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve)))')
                snapshot = state(page)
                assert not snapshot['failed'] and not errors, (snapshot, errors)
                page.screenshot(path=str(output / f'{name}.png'))
                records.append(dict(name=name, checkpoint='data/' + checkpoint,
                                    restored_checkpoint_capture=True, rejoined_partner=split,
                                    state=snapshot))
                page.close()
                print(name, 'PASS', flush=True)
            browser.close()
        (output / 'checkpoints.json').write_text(json.dumps(records, indent=2) + '\n')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
