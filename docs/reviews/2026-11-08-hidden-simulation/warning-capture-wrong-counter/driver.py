"""Fire once at an earned neutral carrier and retain the real warning output."""
from array import array
from functools import partial
from http.server import ThreadingHTTPServer
import hashlib
import json
import math
from pathlib import Path
import sys
import threading
import wave

from capture import AUDIO_PROBE, REVIEW, ROOT, face
from playwright.sync_api import sync_playwright
from test_browser import QuietHandler, start_play, state, wait_ready
from test_interiors_browser import expect
from test_story_browser import import_fixture

site = ROOT / 'build-pages-warning'
out = REVIEW / 'warning-capture'
out.mkdir(exist_ok=False)
server = ThreadingHTTPServer(('127.0.0.1', 0), partial(QuietHandler, directory=str(site)))
threading.Thread(target=server.serve_forever, daemon=True).start()
results = {'wasm_sha256': hashlib.sha256((site / 'starship_crawl/starship_crawl.wasm').read_bytes()).hexdigest(),
           'source_manifest': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in sorted((ROOT / 'src').iterdir()) if p.suffix in ('.c', '.h', '.inc')}}
try:
    with sync_playwright() as pw:
        browser = pw.chromium.launch(headless=True, executable_path='/usr/bin/chromium',
            args=['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'])
        context = browser.new_context(viewport={'width': 1280, 'height': 720})
        context.add_init_script(AUDIO_PROBE)
        page = context.new_page()
        errors = []
        page.on('pageerror', lambda e: errors.append(str(e)))
        page.on('dialog', lambda d: d.accept())
        page.goto(f'http://127.0.0.1:{server.server_port}/starship_crawl/?seed=713')
        wait_ready(page)
        start_play(page)
        import_fixture(page, str(REVIEW / 'convoy-journey-final/convoy-close.sav'))
        initial = state(page)
        carrier = min((f for f in initial['forces'] if f['physical'] and f['cargo']),
                      key=lambda f: math.dist(f['at'], [initial['x'], initial['y'], initial['z']]))
        cursor = [640, 360]
        page.mouse.move(*cursor)
        face(page, carrier['at'], cursor)
        before = state(page)
        results['before'] = before
        page.screenshot(path=str(out / 'before.png'))
        page.evaluate('window.scReviewSound=[[],[]];window.scReviewRecording=true')
        page.mouse.down()
        try:
            expect(page, f's.shots>{before["shots"]}', timeout=30000)
        finally:
            page.mouse.up()
        expect(page, f's.projectile_hits>{before["projectile_hits"]}', timeout=30000)
        warning = state(page)
        results['warning'] = warning
        assert warning['kills'] == before['kills'] and warning['force_losses'] == before['force_losses']
        assert warning['shots'] == before['shots'] + 1, (before['shots'], warning['shots'])
        page.screenshot(path=str(out / 'warning.png'))
        page.keyboard.press('F5')
        # The ordinary save action writes the file; this read only retains evidence.
        data = page.evaluate("Array.from(Module.FS.readFile('/persist/expedition.sav'))")
        (out / 'warning.sav').write_bytes(bytes(data))
        expect(page, f's.time>{warning["time"]+4}', timeout=120000)
        page.evaluate('window.scReviewRecording=false')
        results['after'] = state(page)
        page.screenshot(path=str(out / 'after.png'))
        sound = page.evaluate('({rate:window.scReviewSoundRate,samples:window.scReviewSound})')
        samples = [v for pair in zip(*sound['samples']) for v in pair]
        assert samples and all(math.isfinite(v) and abs(v) <= 1.001 for v in samples)
        results['audio_peak'] = max(abs(v) for v in samples)
        assert results['audio_peak'] > .0001
        pcm = array('h', (round(max(-1, min(1, v)) * 32767) for v in samples))
        if sys.byteorder != 'little':
            pcm.byteswap()
        with wave.open(str(out / 'warning.wav'), 'wb') as wav:
            wav.setnchannels(2)
            wav.setsampwidth(2)
            wav.setframerate(sound['rate'])
            wav.writeframes(pcm.tobytes())
        assert not errors, errors
        results['passed'] = True
        browser.close()
finally:
    (out / 'result.json').write_text(json.dumps(results, indent=2) + '\n')
    server.shutdown()
    server.server_close()
