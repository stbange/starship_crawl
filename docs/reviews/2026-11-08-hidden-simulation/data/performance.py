"""Summarize the measured headless and instrumented packaged-browser timings."""
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
names = ['strategy', 'forces', 'director', 'strategy_active_tick', 'faction_tactics', 'game_step']
labels = ['Strategy call, including idle frames', 'Physical force movement', 'Director',
          'Actual two-second strategy update', 'Faction targeting/combat', 'Complete game step']
events = [json.loads(line) for line in (HERE / 'native-benchmark/run-00/events.jsonl').read_text().splitlines()]
native = {e['part']: e for e in events if e['event'] == 'performance'}
web = json.loads((HERE.parent / 'profile-captures/performance.json').read_text())
parts = {names[p['part']]: p for p in web['parts']}
assert set(native) == set(parts) == set(names), (native.keys(), parts.keys())
lines = ['# Measured update costs', '',
         'All update times below are microseconds. Native uses one 600-second simulated expedition; browser uses 60 simulated seconds at an earned occupied-station checkpoint. Review-only linker wrappers time the ordinary core. Component percentiles cannot be summed; the complete game step includes ordinary enemies, projectiles, streaming and the component calls.', '',
         '| Build / measurement | Samples | p50 µs | p95 µs | p99 µs | Max µs |',
         '| --- | ---: | ---: | ---: | ---: | ---: |']
for build, source in [('Native', native), ('Chromium / WebGL', parts)]:
    for name, label in zip(names, labels):
        p = source[name]
        lines.append(f'| {build}: {label} | {p["samples"]} | {p["p50_us"]:.2f} | {p["p95_us"]:.2f} | {p["p99_us"]:.2f} | {p["max_us"]:.2f} |')
frames = sorted(f for f in web['frames_ms'] if f > 0)
lines += ['', f'Browser animation-frame intervals (excluding the initial zero sample): {len(frames)} samples; p50 {frames[len(frames)//2]:.2f} ms, p95 {frames[len(frames)*95//100]:.2f} ms, p99 {frames[len(frames)*99//100]:.2f} ms. These are software-rendered Chromium measurements under the cloud CPU quota, not a physical GPU/mobile frame-rate claim.', '',
          'The browser timer is quantized at approximately 100 microseconds here: zero-valued samples mean below its resolution, not free work. Only 31 actual strategy ticks occur in the web probe, so its high percentiles are exploratory. The native and browser probes cover different scenes and are not a controlled platform-speed comparison.', '',
          'Browser memory and state: `' + json.dumps(web['memory'], sort_keys=True) + '`.', '',
          'Native harness/launcher process measurement: `' + (HERE / 'native-benchmark/measurement.json').read_text().strip().replace('\n', ' ') + '`.', '',
          'The native probe starts after the browser queue; sanitizer checks can run concurrently. The earlier hour-run timings were taken during parallel testing and include scheduler contention. Neither set is a latency guarantee. The 32-force, 20-actor-per-room and fourteen-site work limits were retained after bounded-work, full-room and separated co-op tests; physical Windows/mobile profiling remains pending.']
(HERE / 'performance.md').write_text('\n'.join(lines) + '\n')
