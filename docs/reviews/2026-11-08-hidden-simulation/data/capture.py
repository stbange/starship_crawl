"""Render earned checkpoints through normal import/look controls and capture the audio mix.

Use --site-dir build-pages-profile for the separately linked timing build. No
position, actor, resource or progression setters are used.
"""
import argparse
from array import array
from functools import partial
from http.server import ThreadingHTTPServer
import hashlib
import json
import math
import os
from pathlib import Path
import sys
import threading
import wave

ROOT = Path(__file__).resolve().parents[4]
REVIEW = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tests'))
from playwright.sync_api import sync_playwright
from test_browser import QuietHandler, start_play, state, wait_ready
from test_interiors_browser import expect
from test_regional_browser import aim_at
from test_story_browser import import_fixture
from test_foundation_browser import controls

# Observe the real WebAudio graph. The extra probe outputs silence; the original
# node-to-speaker connection is left intact. This code belongs only to the driver.
AUDIO_PROBE = """(() => {
  const connect = AudioNode.prototype.connect;
  window.scReviewSound = [[], []];
  AudioNode.prototype.connect = function(destination, ...args) {
    const result = connect.call(this, destination, ...args);
    if (destination instanceof AudioDestinationNode && !this.scReviewTapped) {
      this.scReviewTapped = true;
      const context = this.context;
      const probe = context.createScriptProcessor(2048, 2, 2);
      probe.scReviewTapped = true;
      window.scReviewSoundRate = context.sampleRate;
      probe.onaudioprocess = event => {
        if (!window.scReviewRecording || window.scReviewSound[0].length >= context.sampleRate * 4) return;
        for (let channel=0;channel<2;channel++)
          window.scReviewSound[channel].push(...event.inputBuffer.getChannelData(channel));
      };
      connect.call(this, probe);
      connect.call(probe, destination);
      this.scReviewProbe = probe;
    }
    return result;
  };
})();"""


def face(page, point, cursor):
    for _ in range(6):
        aim_at(page, point, cursor)
        s = state(page)
        yaw = math.atan2(point[0] - s['x'], point[2] - s['z']) - s['yaw']
        if abs(math.atan2(math.sin(yaw), math.cos(yaw))) < .04:
            return
    raise AssertionError('Pointer input did not face the subject')


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=ROOT / 'build-pages-preview')
    parser.add_argument('--profile', action='store_true')
    args = parser.parse_args()
    output = REVIEW / ('profile-captures' if args.profile else 'captures')
    output.mkdir(exist_ok=True)
    (output / 'build.json').write_text(json.dumps({
        'profiling_wrappers': args.profile,
        'wasm_sha256': hashlib.sha256((args.site_dir / 'starship_crawl/starship_crawl.wasm').read_bytes()).hexdigest(),
        'source_manifest': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                            for p in sorted((ROOT / 'src').iterdir()) if p.suffix in ('.c', '.h', '.inc')},
    }, indent=2) + '\n')
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(QuietHandler, directory=str(args.site_dir.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    cases = [
        ('convoy', REVIEW / 'convoy-journey-final/convoy-close.sav', None),
        ('retreat', REVIEW / 'raid-journey-final/followed-raid.sav', None),
        ('station-before', REVIEW / 'raid-journey-final/followed-raid.sav', [104, 1.7, 729]),
        ('station-after', REVIEW / 'raid-journey-final/occupied-station.sav', [104, 1.7, 729]),
        ('battle', REVIEW / 'raid-journey-final/revisited-battle.sav', [96, 1.4, 701]),
        ('quiet-recovery', REVIEW / 'data/run-00/shelter.sav', None),
        ('split-convoy', REVIEW / 'convoy-journey-final/convoy-close.sav', None),
    ]
    results = []
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True, executable_path=os.environ.get('SC_BROWSER', '/usr/bin/chromium'),
                args=['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'])
            context = browser.new_context(viewport={'width': 1280, 'height': 720})
            context.add_init_script(AUDIO_PROBE)
            page = context.new_page()
            page.on('dialog', lambda d: d.accept())
            errors = []
            page.on('pageerror', lambda e: errors.append(str(e)))
            page.goto(f'http://127.0.0.1:{server.server_port}/starship_crawl/?seed=713')
            wait_ready(page)
            start_play(page)
            for name, fixture, point in cases:
                import_fixture(page, str(fixture))
                before = state(page)
                cursor = [640, 360]
                page.mouse.move(*cursor)
                if name in ('convoy', 'retreat', 'split-convoy'):
                    forces = [f for f in before['forces'] if f['strength'] and f['physical'] and
                              (f['order'] == 6 if name == 'retreat' else f['cargo'])]
                    subject = min(forces, key=lambda f: math.dist(f['at'], [before['x'], before['y'], before['z']]))
                    point = [subject['at'][0], subject['at'][1] + 1.1, subject['at'][2]]
                if point:
                    face(page, point, cursor)
                if name == 'split-convoy':
                    page.keyboard.press('F2')
                    expect(page, 's.players===2')
                    began = state(page)['time']
                    page.keyboard.down('s')
                    try:
                        expect(page, f's.time>{began + 2}', timeout=120000)
                    finally:
                        page.keyboard.up('s')
                    position = state(page)
                    partner = controls(page)['p2'][:3]
                    assert math.dist([position['x'], position['y'], position['z']], partner) > 3
                    (output / 'split-positions.json').write_text(json.dumps({'first': [position['x'], position['y'], position['z']], 'second': partner}, indent=2) + '\n')
                initial = state(page)
                page.screenshot(path=str(output / f'{name}.png'))
                (output / f'{name}-initial.json').write_text(json.dumps(initial, indent=2) + '\n')
                page.evaluate('window.scReviewSound=[[],[]];window.scReviewRecording=true')
                expect(page, f's.time>{initial["time"] + (12 if name == "battle" else 4)}', timeout=120000)
                page.evaluate('window.scReviewRecording=false')
                current = state(page)
                page.screenshot(path=str(output / f'{name}-later.png'))
                (output / f'{name}.json').write_text(json.dumps(current, indent=2) + '\n')
                sound = page.evaluate('({rate:window.scReviewSoundRate, samples:window.scReviewSound})')
                channels = sound['samples']
                samples = [v for pair in zip(*channels) for v in pair]
                assert samples and max(abs(v) for v in samples) > .0001, (name, len(samples))
                assert all(math.isfinite(v) and abs(v) <= 1.001 for v in samples)
                pcm = array('h', (round(max(-1, min(1, v)) * 32767) for v in samples))
                if sys.byteorder != 'little':
                    pcm.byteswap()
                with wave.open(str(output / f'{name}.wav'), 'wb') as wav:
                    wav.setnchannels(2)
                    wav.setsampwidth(2)
                    wav.setframerate(sound['rate'])
                    wav.writeframes(pcm.tobytes())
                results.append({'name': name, 'fixture': str(fixture.relative_to(ROOT)),
                                'before': before, 'initial_capture': initial, 'after': current, 'audio_samples': len(samples),
                                'audio_rate': sound['rate'], 'audio_peak': max(abs(v) for v in samples),
                                'audio_rms': math.sqrt(sum(v*v for v in samples) / len(samples)),
                                'channel_difference_rms': math.sqrt(sum((a-b)**2 for a,b in zip(*channels)) / len(channels[0]))})
                print('CAPTURED ' + name, flush=True)
            if args.profile:
                import_fixture(page, str(REVIEW / 'raid-journey-final/occupied-station.sav'))
                page.evaluate('window.scReviewRequest=true')
                page.wait_for_function('window.scReviewProfile !== undefined')
                page.evaluate('window.scReviewProfile=undefined;window.scReviewFrames=[];window.scReviewSampling=true;'
                              'window.scReviewFramePrevious=performance.now();'
                              '(function sample(t){if(!window.scReviewSampling)return;'
                              'window.scReviewFrames.push(t-window.scReviewFramePrevious);'
                              'window.scReviewFramePrevious=t;requestAnimationFrame(sample)})(performance.now())')
                before = state(page)
                expect(page, f's.time>{before["time"]+60}', timeout=600000)
                page.evaluate('window.scReviewSampling=false;window.scReviewRequest=true')
                page.wait_for_function('window.scReviewProfile !== undefined')
                metrics = page.evaluate('({parts:window.scReviewProfile,memory:window.scReviewMemory,frames_ms:window.scReviewFrames})')
                metrics['before'] = before
                metrics['after'] = state(page)
                (output / 'performance.json').write_text(json.dumps(metrics, indent=2) + '\n')
            assert not errors, errors
            browser.close()
    finally:
        (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
