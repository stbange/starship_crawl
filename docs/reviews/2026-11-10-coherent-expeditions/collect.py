"""Retain compact, reproducible expedition traces, earned checkpoints and build curves."""
import argparse
import gzip
import json
from pathlib import Path
import shutil


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    summary = []
    for directory in sorted(args.input.glob('run-*')):
        destination = args.output / directory.name
        destination.mkdir(exist_ok=True)
        metadata = json.loads((directory / 'execution.json').read_text())
        if 'returncode' not in metadata:
            raise SystemExit(f'{directory.name}: expedition is still running')
        shutil.copyfile(directory / 'execution.json', destination / 'execution.json')
        raw = (directory / 'events.jsonl').read_bytes()
        (destination / 'events.jsonl.gz').write_bytes(gzip.compress(raw, mtime=0))
        rows = [json.loads(line) for line in raw.splitlines()]
        end = next((row for row in rows if row['event'] == 'end'), None)
        stages = []
        for label, phase in [('starter', 0), ('middle', 2), ('late', 4)]:
            row = next((row for row in rows if row['event'] == 'sample' and row['phase'] == phase), None)
            if row:
                stages.append(dict(label=label, **row, invested=sum(
                    event['event'] == 'upgrade' and event['t'] <= row['t'] for event in rows)))
        summary.append(dict(start=rows[0], stages=stages, end=end, guardian_fights=[
            row for row in rows if row['event'] == 'fight_end' and row['guardian'] < 8]))
        for name in ['guardian-fight.sav', 'unbound.sav', 'final.sav']:
            if (directory / name).exists():
                (destination / (name + '.gz')).write_bytes(gzip.compress(
                    (directory / name).read_bytes(), mtime=0))
        print(directory.name, 'exit', metadata['returncode'], 'wall', metadata['wall_seconds'],
              'seconds', end and end['t'], 'invested', end and end['upgrades'],
              'deaths', end and end['deaths'])
    (args.output / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')


if __name__ == '__main__':
    main()
