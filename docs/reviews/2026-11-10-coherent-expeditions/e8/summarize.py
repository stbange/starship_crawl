#!/usr/bin/env python3
"""Rebuild the retained simulation tables and figures from JSONL or gzip traces."""
import argparse
import gzip
import json
from pathlib import Path


def read_events(directory):
    path = directory / 'events.jsonl'
    if path.is_file():
        return [json.loads(line) for line in path.read_text().splitlines()]
    with gzip.open(str(path) + '.gz', 'rt') as trace:
        return [json.loads(line) for line in trace]


def summarize(directory):
    events = read_events(directory)
    execution = json.loads((directory / 'execution.json').read_text())
    single = {e['event']: e for e in events if e['event'] in (
        'start', 'configuration', 'end', 'breadth_summary', 'sequence_summary', 'simulation_summary')}
    samples = [e for e in events if e['event'] == 'sample']
    builds = [e for e in events if e['event'] == 'build_sample']
    first = next((e for e in samples if e['phase'] == 4), None)
    phase_build = next((b['invested'] for b in builds if first and b['t'] >= first['t']), None)
    cells = [e['cell'] for e in events if e['event'] == 'cell']
    return dict(name=directory.name, execution=execution, **single,
                phase4_first_sample=first, phase4_invested=phase_build,
                extents=[[min(c[i] for c in cells), max(c[i] for c in cells)] for i in range(3)],
                guardians=[e for e in events if e['event'] == 'fight_end' and e['guardian'] < 8],
                guardian_stages=[e for e in events if e['event'] == 'guardian_stage'],
                rewards=[e for e in events if e['event'] in ('reward', 'reward_deferred')],
                navigation_events=[e for e in events if e['event'] in (
                    'abandon_waypoint', 'abandon_detour', 'detour_claim', 'recovery')],
                performance=[e for e in events if e['event'] == 'performance'],
                samples=samples, builds=builds)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', type=Path, required=True)
    parser.add_argument('--after', type=Path, required=True)
    parser.add_argument('--extras', type=Path, required=True)
    parser.add_argument('--navigation', type=Path)
    parser.add_argument('--specialists', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    groups = {name: [summarize(p) for p in sorted(getattr(args, name).glob('run-*'))]
              for name in ('before', 'after', 'extras', 'navigation', 'specialists')
              if getattr(args, name) is not None}
    (args.output / 'summary.json').write_text(json.dumps(groups, indent=2) + '\n')
    table = ['| Batch / seed / route | Condition / motive | Phase 4 / final investment | Kills / deaths | Skipped goals | Wall seconds |',
             '| --- | --- | ---: | ---: | ---: | ---: |']
    conditions = {1: 'machines', 2: 'vents', 3: 'conduits'}
    motives = {0: 'rescue', 1: 'knowledge', 2: 'salvage', 3: 'containment'}
    for batch, runs in groups.items():
        for r in runs:
            start, end, breadth = r['start'], r['end'], r['breadth_summary']
            table.append(f"| {batch}/{r['name']} · {start['seed']} · {start['route']} | "
                         f"{conditions[r['configuration']['condition']]} / {motives[start['motivation']]} | "
                         f"{r['phase4_invested']} / {breadth['invested']} | {end['kills']} / {end['deaths']} | "
                         f"{end['skipped']} | {r['execution']['wall_seconds']} |")
    (args.output / 'runs.md').write_text('\n'.join(table) + '\n')

    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    plt.rcParams.update({'font.size': 9, 'axes.spines.top': False, 'axes.spines.right': False,
                         'savefig.dpi': 160})
    colors = {0: '#3877a8', 1: '#b25c17', 4: '#727c26'}
    fig, axes = plt.subplots(1, 3, figsize=(13, 3.8), sharey=True)
    for ax, run in zip(axes, (0, 1, 4)):
        for batch, label, style in (('before', 'E7 baseline', '--'), ('after', 'E8 tuned', '-')):
            r = next(r for r in groups[batch] if r['name'] == f'run-{run:02d}-on')
            ax.step([b['t'] / 60 for b in r['builds']], [b['invested'] for b in r['builds']],
                    where='post', color=colors[run], linestyle=style, label=label)
            ax.plot(r['phase4_first_sample']['t'] / 60, r['phase4_invested'], 'o', color=colors[run])
        ax.axhspan(30, 40, color='#cbd5df', alpha=.3, label='main-route hypothesis' if run == 0 else None)
        ax.set(title=f"Seed {r['start']['seed']} · {conditions[r['configuration']['condition']]}",
               xlabel='Simulated minutes', xlim=(0, 60))
        ax.grid(alpha=.15)
    axes[0].set_ylabel('Invested attribute points')
    axes[0].legend(fontsize=8)
    fig.suptitle('Matched growth curves · dot = first phase-four sample · automated policies')
    fig.tight_layout()
    fig.savefig(args.output / 'growth.png')
    plt.close(fig)

    fig, axes = plt.subplots(2, 1, figsize=(10, 6), sharex=True)
    for ax, run in zip(axes, (1, 4)):
        for group, suffix, label, color in (('after', 'on', 'preferred weapon', '#b25c17'),
                                          ('extras', 'resource-aware', 'switch to carbine when low', '#3877a8')):
            r = next(r for r in groups[group] if r['name'] == f'run-{run:02d}-{suffix}')
            ax.plot([s['t'] / 60 for s in r['samples']], [s['energy'] for s in r['samples']],
                    label=f"{label} ({r['end']['energy_starved_seconds']:.1f}s unaffordable fire)",
                    color=color, linewidth=1)
        ax.set(title=f"Seed {r['start']['seed']} · same game rules, different input policy", ylabel='Energy')
        ax.legend(fontsize=8)
        ax.grid(alpha=.15)
    axes[-1].set_xlabel('Simulated minutes · sampled every 30 seconds')
    fig.tight_layout()
    fig.savefig(args.output / 'energy.png')
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(10, 6))
    runs = groups['before']
    for i, r in enumerate(runs):
        combat = r['end']['combat_seconds'] / 60
        ax.barh(i, combat, color='#b25c17')
        ax.barh(i, 60 - combat, left=combat, color='#a9c6d8')
        ax.text(combat + .4, i, f"{r['end']['deaths']} deaths · {r['end']['skipped']} skipped goals", va='center', fontsize=8)
    ax.set(yticks=range(len(runs)), yticklabels=[f"{r['start']['seed']} · {r['start']['route']}" for r in runs],
           xlabel='Simulated minutes: orange = target in firing policy; blue = other activity', xlim=(0, 67),
           title='Twelve one-hour baseline expeditions · other activity includes travel, waiting and rest')
    fig.tight_layout()
    fig.savefig(args.output / 'activity.png')
    plt.close(fig)


if __name__ == '__main__':
    main()
