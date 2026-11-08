"""Summarize all attempted runs; incomplete or failed runs remain in the output."""
from collections import Counter
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
binary = ROOT / 'build-tuning/sc_review_inspect'
subprocess.run(['cc', '-std=c17', '-O2', '-I' + str(ROOT / 'src'), str(HERE / 'inspect.c'),
                str(ROOT / 'build-tuning/libsc_core.a'), '-lm', '-o', str(binary)], check=True)
rows = []
for directory in sorted([*HERE.glob('run-*'), *HERE.glob('without-director-*'), *HERE.glob('tuned/run-*'), *HERE.glob('footprint-fixed/run-*')]):
    events = [json.loads(line) for line in (directory / 'events.jsonl').read_text().splitlines()]
    by_type = {e['event']: e for e in events}
    row = {'run': str(directory.relative_to(HERE)), 'execution': json.loads((directory / 'execution.json').read_text()),
           'start': by_type.get('start'), 'end': by_type.get('end'), 'simulation': by_type.get('simulation_summary'),
           'performance': [e for e in events if e['event'] == 'performance'],
           'intervention_kinds': dict(Counter(str(e['detail']) for e in events if e['event'] == 'simulation' and e['kind'] == 9)),
           'save_bytes': {p.name: p.stat().st_size for p in sorted(directory.glob('*.sav'))}}
    if (directory / 'final.sav').is_file():
        inspected = subprocess.run([str(binary), str(directory / 'final.sav')], text=True, capture_output=True)
        row['inspection_returncode'] = inspected.returncode
        row['final_state'] = json.loads(inspected.stdout) if not inspected.returncode else {'error': inspected.stderr}
    rows.append(row)
(HERE / 'summary.json').write_text(json.dumps(rows, indent=2) + '\n')
lines = ['| Run | Result | Seed / route / build | Minutes | Kills / deaths | Max layer | Forces / nearby actors | Captures / losses | Interventions | Recovery / discovery | Save KiB |',
         '| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |']
for row in rows:
    start, end, sim = row['start'], row['end'], row['simulation']
    if not end or not sim:
        lines.append(f'| {row["run"]} | **incomplete** | {start["seed"]} / {start["route"]} | | | | | | | | |')
        continue
    frames = sum(sim['pacing_frames'])
    status = 'PASS' if row['execution'].get('returncode') == 0 and not row.get('inspection_returncode') else '**FAILED**'
    lines.append(f'| {row["run"]} | {status} | {start["seed"]} / {start["route"]} / {start["profile"]} | {end["t"]/60:.1f} | '
                 f'{end["kills"]} / {end["deaths"]} | {end["max_layer"]} | {sim["max_forces"]} / {sim["max_actors"]} | '
                 f'{sim["captures"]} / {sim["losses"]} | {sim["interventions"]} | '
                 f'{100*sim["pacing_frames"][2]/frames:.1f}% / {100*sim["pacing_frames"][3]/frames:.1f}% | '
                 f'{row["save_bytes"].get("final.sav",0)/1024:.1f} |')
(HERE / 'results.md').write_text('\n'.join(lines) + '\n')
print(f'Summarized {len(rows)} attempts; {sum(r["execution"].get("returncode") == 0 for r in rows)} completed successfully.')

indexed = {r['run']: r for r in rows}
comparison = ['| Route | Run | Combat seconds | Health lost | Arrivals / captures | Interventions | Recovery / discovery state |',
              '| --- | --- | ---: | ---: | ---: | ---: | ---: |']
for number in [0, 3, 5, 9]:
    for variant, key in [('baseline', f'run-{number:02}'), ('director disabled', f'without-director-{number:02}'),
                         ('tuned', f'footprint-fixed/run-{number:02}' if number == 9 else f'tuned/run-{number:02}')]:
        r = indexed.get(key)
        if not r or r['execution'].get('returncode') != 0:
            continue
        s, e = r['simulation'], r['end']
        total = sum(s['pacing_frames'])
        pacing = 'disabled' if s['disabled'] else f"{100*s['pacing_frames'][2]/total:.1f}% / {100*s['pacing_frames'][3]/total:.1f}%"
        comparison.append(f"| {r['start']['route']} | {variant} | {e['combat_seconds']:.1f} | {e['health_loss']:.1f} | "
                          f"{s['arrivals']} / {s['captures']} | {s['interventions']} | {pacing} |")
comparison += ['', 'The same seed, route and controller profile are repeated. The controller reacts to the world, so subsequent input sequences and paths can diverge. Director pacing states are internal modes, not a measurement of player enjoyment; disabled runs have no such states. Combat seconds measure the controller tracking a target, including guardian encounters. Arrivals count order completions, not unique formations.']
(HERE / 'comparisons.md').write_text('\n'.join(comparison) + '\n')
