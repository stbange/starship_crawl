"""Derive review tables and plots from retained tick-observed run events."""
import collections
import json
from pathlib import Path
import statistics
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent


def median(values):
    return round(statistics.median(values), 3) if values else None


def summarize(directory):
    rows = [json.loads(line) for line in (directory / "events.jsonl").read_text().splitlines()]
    start = rows[0]
    ends = [r for r in rows if r["event"] == "end"]
    if not ends or not (directory / "execution.json").exists():
        return None
    end = ends[-1]
    execution = json.loads((directory / "execution.json").read_text())
    cells = [r for r in rows if r["event"] == "cell" and r["cell"][2] >= 0]
    owners = {tuple(r["owner"]): r for r in cells}
    late = {tuple(r["owner"]): r for r in cells if r["t"] >= 1800}
    fights = []
    active = None
    for r in rows:
        if r["event"] == "fight_start":
            active = r
        if r["event"] == "fight_end" and active:
            fights.append(dict(active, duration=r["seconds"], damage=r["damage"]))
            active = None
    dispatch = collections.Counter(r["kind"] for r in rows if r["event"] == "dispatch")
    claimed = collections.Counter(r["kind"] for r in rows if r["event"] == "dispatch_claim")
    detours = [r for r in rows if r["event"] == "detour_claim"]
    samples = [r for r in rows if r["event"] == "sample"]
    checkpoints = {}
    for source in sorted(directory.glob("*.sav")):
        result = subprocess.run(["/tmp/sc-review-inspect", str(source)], capture_output=True, text=True)
        checkpoints[source.name] = dict(returncode=result.returncode,
                                        state=json.loads(result.stdout) if result.returncode == 0 else None,
                                        stderr=result.stderr)
    (directory / "checkpoints.json").write_text(json.dumps(checkpoints, indent=2) + "\n")
    times = lambda event: [r["t"] for r in rows if r["event"] == event]
    first = lambda values: round(values[0], 3) if values else None
    theme_counts = collections.Counter(r["theme"] for r in owners.values())
    late_weather = collections.Counter(r["weather"] for r in late.values())
    late_ecology = collections.Counter(r["ecology"] for r in late.values())
    late_forms = collections.Counter(r["form"] for r in late.values())
    return dict(start=start, end=end, execution=execution,
                checkpoints_valid=all(c["returncode"] == 0 for c in checkpoints.values()),
                checkpoint_count=len(checkpoints), final=checkpoints["final.sav"]["state"],
                cells=len({tuple(r["cell"]) for r in cells}), owners=len(owners), late_owners=len(late),
                themes=dict(theme_counts), weather=dict(late_weather), ecology=dict(late_ecology),
                forms=dict(late_forms), max_height=max(r["height"] for r in cells),
                theme_count=len(theme_counts), weather_count=len(late_weather), ecology_count=len(late_ecology),
                form_count=len({r["form"] for r in owners.values()}),
                unbound=first([r["t"] for r in rows if r["event"] == "save" and r["tag"] == "unbound"]),
                extraction=first([r["t"] for r in rows if r["event"] == "save" and r["tag"] == "extraction"]),
                settlement=first([r["t"] for r in rows if r["event"] == "politics" and r["stage"] == 3]),
                dispatch=dict(dispatch), dispatch_claims=dict(claimed),
                guardian_fights=[f for f in fights if f["guardian"] >= 0],
                court_fights=[f for f in fights if f["court"]],
                ordinary_fights=[f for f in fights if f["guardian"] < 0 and not f["court"]],
                detours=detours, samples=samples,
                aborted_detours=times("abandon_detour"), aborted_waypoints=times("abandon_waypoint"))


def main():
    runs = [r for d in sorted(HERE.glob("run-*")) if (r := summarize(d))]
    (HERE / "summary.json").write_text(json.dumps(runs, indent=2) + "\n")
    print("run seed route profile minutes phase layer owners kills deaths health_loss min_hp_ratio optional recoveries")
    for r in runs:
        s, e = r["start"], r["end"]
        print(s["run"], s["seed"], s["route"], s["profile"], round(e["t"] / 60, 2), e["phase"],
              e["max_layer"], r["owners"], e["kills"], e["deaths"], e["health_loss"], e["min_health_ratio"],
              e["frontier_claims"], e["recoveries"])
    if not runs:
        return
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    plt.style.use("dark_background")
    colors = {"offense": "#edb65d", "survival": "#72c4ef", "toolbox": "#c79ce6"}
    fig, axes = plt.subplots(3, 1, figsize=(12, 10), sharex=True, layout="constrained")
    for r in runs:
        s = r["start"]
        x = [p["t"] / 60 for p in r["samples"]]
        label = f'{s["run"] + 1:02} {s["route"]}'
        for ax, field in zip(axes, ("kills", "rank", "health_loss")):
            ax.plot(x, [p[field] for p in r["samples"]], label=label, alpha=.8,
                    color=colors[s["profile"]], linestyle=("-", "--", ":", "-.")[s["run"] // 3])
    for ax, label in zip(axes, ("Cumulative kills", "Character rank", "Observed cumulative health loss")):
        ax.set_ylabel(label)
        ax.grid(alpha=.15)
    axes[0].legend(ncol=4, fontsize=8, loc="upper left")
    axes[-1].set_xlabel("Simulated minutes (not wall-clock playtime)")
    fig.suptitle("12 fresh-character expeditions · generator 13 · bounded automated input\n"
                 "Gold: offense   Blue: survival   Purple: toolbox · seeds and routes differ")
    fig.savefig(ROOT / "pacing.png", dpi=140)
    plt.close(fig)
    aggregate = {
        "completed": sum(r["end"]["complete"] for r in runs),
        "simulated_hours": sum(r["end"]["frames"] for r in runs) / 216000,
        "kills": sum(r["end"]["kills"] for r in runs),
        "deaths": sum(r["end"]["deaths"] for r in runs),
        "court_finale_seconds": [f["duration"] for r in runs for f in r["court_fights"] if f["court"] == 3],
        "court_finale_damage": [f["damage"] for r in runs for f in r["court_fights"] if f["court"] == 3],
        "guardian_median_seconds": median([f["duration"] for r in runs for f in r["guardian_fights"]]),
        "optional_median_seconds": median([f["duration"] for r in runs for f in r["ordinary_fights"]]),
        "all_checkpoints_valid": all(r["checkpoints_valid"] for r in runs),
        "checkpoint_count": sum(r["checkpoint_count"] for r in runs),
    }
    (HERE / "aggregate.json").write_text(json.dumps(aggregate, indent=2) + "\n")
    comparison = HERE / "comparison-09"
    if (comparison / "execution.json").exists():
        (comparison / "summary.json").write_text(json.dumps(summarize(comparison), indent=2) + "\n")
    print(json.dumps(aggregate, indent=2))


if __name__ == "__main__":
    main()
