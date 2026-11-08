"""A normally joined co-op player taps fire at an earned neutral carrier."""
from array import array
from functools import partial
from http.server import ThreadingHTTPServer
import hashlib
import json
import math
from pathlib import Path
import sys
import subprocess
import threading
import wave

from capture import AUDIO_PROBE, REVIEW, ROOT, face
from playwright.sync_api import sync_playwright
from test_browser import QuietHandler, start_play, state, wait_ready
from test_interiors_browser import expect
from test_story_browser import import_fixture
from test_regional_browser import aim_at
from test_foundation_browser import VIRTUAL_PADS, controls, expect_controls

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
        context.add_init_script(VIRTUAL_PADS)
        page = context.new_page()
        errors = []
        page.on('pageerror', lambda e: errors.append(str(e)))
        page.on('dialog', lambda d: d.accept())
        page.goto(f'http://127.0.0.1:{server.server_port}/starship_crawl/?seed=713')
        wait_ready(page)
        start_play(page)
        page.evaluate('connectVirtualPad(0);connectVirtualPad(1)')
        import_fixture(page, str(REVIEW / 'convoy-journey-final/convoy-close.sav'))
        initial = state(page)
        carrier = min((f for f in initial['forces'] if f['physical'] and f['cargo']),
                      key=lambda f: math.dist(f['at'], [initial['x'], initial['y'], initial['z']]))
        cursor = [640, 360]
        page.mouse.move(*cursor)
        began = state(page)['time']
        page.keyboard.down('a')
        try:
            expect(page, f's.time>{began+.9}')
        finally:
            page.keyboard.up('a')
        page.screenshot(path=str(out / 'before.png'))
        for _ in range(3):
            current = state(page)
            carrier = next(f for f in current['forces'] if f['id'] == carrier['id'])
            aim_at(page, [carrier['at'][0], carrier['at'][1], carrier['at'][2] + .65], cursor)
        before = state(page)
        results['before'] = before
        page.keyboard.press('F2')
        expect(page, 's.players===2')
        expect_controls(page, 'c.pads[0]===0 && c.pads[1]===1')
        # Normal joining retains the fresh partner's starting carbine. The earned
        # lead character's damage kills this carrier in one shot (retained attempt).
        for _ in range(30):
            now = state(page)
            partner = controls(page)['p2']
            carrier = next(f for f in now['forces'] if f['id'] == carrier['id'])
            dx, dy, dz = (carrier['at'][0]-partner[0], carrier['at'][1]-partner[1]-1.6,
                          carrier['at'][2]+.65-partner[2])
            yaw = math.atan2(dx, dz)-partner[3]
            yaw = math.atan2(math.sin(yaw), math.cos(yaw))
            pitch = math.atan2(dy, math.hypot(dx, dz))-partner[4]
            if abs(yaw) < .03 and abs(pitch) < .03:
                break
            def stick(delta, rate):
                if abs(delta) < .02:
                    return 0
                return math.copysign(min(.8, .18 + abs(delta)/(rate*.25)*.82), delta)
            page.evaluate('axes=>{virtualPads[1].axes[2]=axes[0];virtualPads[1].axes[3]=axes[1]}',
                          [stick(yaw, 2.3), -stick(pitch, 1.8)])
            expect(page, f's.time>{now["time"]+.05}')
        page.evaluate('virtualPads[1].axes[2]=0;virtualPads[1].axes[3]=0')
        results['partner_before'] = controls(page)['p2']
        before = state(page)
        results['before'] = before
        page.evaluate('window.scReviewSound=[[],[]];window.scReviewRecording=true')
        for attempt in range(3):
            now = state(page)
            page.evaluate('virtualPads[1].buttons[7]={pressed:true,touched:true,value:1};'
                          'requestAnimationFrame(()=>{virtualPads[1].buttons[7]={pressed:false,touched:false,value:0}})')
            expect(page, f's.time>{now["time"]+.25}', timeout=30000)
            if state(page)['shots'] > before['shots']:
                break
        warning = state(page)
        results['warning'] = warning
        assert warning['kills'] == before['kills'] and warning['force_losses'] == before['force_losses']
        assert 1 <= warning['shots'] - before['shots'] <= 2, (before['shots'], warning['shots'])
        page.screenshot(path=str(out / 'warning.png'))
        page.keyboard.press('F5')
        # The ordinary save action writes the file; this read only retains evidence.
        data = page.evaluate("Array.from(Module.FS.readFile('/persist/expedition.sav'))")
        (out / 'warning.sav').write_bytes(bytes(data))
        inspection = subprocess.run([str(ROOT / 'build/sc_warning_inspect'), str(out / 'warning.sav'),
                                     str(carrier['country']), '1'], check=True, capture_output=True, text=True)
        results['incident'] = json.loads(inspection.stdout)
        assert results['incident']['strikes'] == 1 and results['incident']['reports'] == 0
        assert results['incident']['partner_strikes'] == 0
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
        print('PASS normal co-op join and emulated controller firing produce a nonlethal warning with separate incident attribution, shaped signal and stereo audio', flush=True)
        browser.close()
finally:
    (out / 'result.json').write_text(json.dumps(results, indent=2) + '\n')
    server.shutdown()
    server.server_close()
