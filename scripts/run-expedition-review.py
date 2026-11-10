#!/usr/bin/env python3
"""Retain one-hour, accelerated expedition evidence (never human playtest claims).

Build sc_hidden_journey first. A frozen executable allows matched before/after
comparisons while the working tree changes. Each child gets its own checkpoint
directory, stdout trace, exit status, wall time and Linux peak RSS measurement.
"""
import argparse
import concurrent.futures
import hashlib
import json
from pathlib import Path
import resource
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    if len(sys.argv) > 1 and sys.argv[1] == '--measure-child':
        result = subprocess.run(sys.argv[3:], cwd=ROOT)
        Path(sys.argv[2]).write_text(str(resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss))
        return result.returncode
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--mode', choices=('matrix', 'matched', 'extras', 'navigation', 'specialists'), default='matrix')
    parser.add_argument('--jobs', type=int, choices=(1, 2), default=2)
    args = parser.parse_args()
    binary, output = args.binary.resolve(), args.output.resolve()
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    if args.mode == 'matrix':
        cases = [(i, []) for i in range(12)]
    elif args.mode == 'matched':
        cases = [(i, extra) for i in (0, 1, 4) for extra in ([], ['--no-director'])]
    elif args.mode == 'navigation':
        cases = [(1, [])]
    elif args.mode == 'specialists':
        cases = [(0, ['--conduit']), (11, ['--bastion'])]
    else:
        cases = [(0, ['--starter']), (7, ['--coop']),
                 (1, ['--resource-aware']), (4, ['--resource-aware'])]

    def run(case):
        i, extras = case
        label = '-'.join(e.removeprefix('--') for e in extras) or 'on'
        directory = output / f'run-{i:02d}-{label}'
        directory.mkdir(parents=True, exist_ok=False)
        command = [str(binary), str(i), str(directory), '3600', '--review', *extras]
        record = dict(command=command, executable_sha256=digest, simulated_seconds=3600,
                      generator=16, checkpoint=17, campaign=2,
                      concurrent_simulations=args.jobs)
        (directory / 'execution.json').write_text(json.dumps(record, indent=2) + '\n')
        start = time.monotonic()
        with (directory / 'events.jsonl').open('w') as trace, (directory / 'stderr.txt').open('w') as err:
            result = subprocess.run([sys.executable, str(Path(__file__).resolve()), '--measure-child',
                                     str(directory / 'max-rss-kib.txt'), *command], stdout=trace, stderr=err)
        record.update(returncode=result.returncode, wall_seconds=round(time.monotonic() - start, 3),
                      max_rss_kib=int((directory / 'max-rss-kib.txt').read_text()))
        record['checkpoints'] = {p.name: p.stat().st_size for p in directory.glob('*.sav')}
        (directory / 'execution.json').write_text(json.dumps(record, indent=2) + '\n')
        events = [json.loads(line) for line in (directory / 'events.jsonl').read_text().splitlines()
                  if line.startswith('{')]
        record['results'] = [e for e in events if e['event'] in (
            'start', 'configuration', 'end', 'breadth_summary', 'sequence_summary',
            'performance', 'simulation_summary')]
        print(directory.name, record['returncode'], record['wall_seconds'],
              next((e for e in record['results'] if e['event'] == 'end'), {}).get('phase'), flush=True)
        return record

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        records = list(pool.map(run, cases))
    (output / 'summary.json').write_text(json.dumps(records, indent=2) + '\n')
    return int(any(record['returncode'] for record in records))


if __name__ == '__main__':
    raise SystemExit(main())
