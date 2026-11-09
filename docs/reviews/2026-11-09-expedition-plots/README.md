# Four expeditions: the missions and what followed

Four fresh characters each completed **60 simulated minutes** on game commit
[`906e938`](https://github.com/stbange/starship_crawl/commit/906e938b3e65df26e0b3331d1fc858fb124336d3).
These are accelerated, automated runs through the production C gameplay core,
not four hours of human or rendered play. The summaries below follow the recorded
missions, choices, room visits and outcomes. The two ships named *The Ninth Mercy*
are separate generated expeditions with different seeds.

**1. The stolen navigator — The Ninth Mercy, seed 713**

A search for a lost star chart led from a witness archive in the Reliquary Vault
to a living archive in the Ballroom of Ash. The explorer fought through the
Choir of Glass, Foundry Marshal and Cryogenic Custodian, then took the archive
without building a memory mirror. The commission succeeded, but the refuge lost
its navigator; 192 people remained aboard. After returning to the bay and resting
in the summoned shelter, the explorer pressed inward through cryosleep galleries,
ruined casinos and gilded atriums. The hour ended at difficulty layer 30:
**701 kills, no deaths**.

**2. Power at a price — The Ninth Mercy, seed 4**

A containment ledger in the Coolant Cathedral pointed the salvage expedition
toward an alien drive in the Iron Foundry. Firewall Saints and a Choir of Glass
stood along the route. The drive was recovered without restoring its backup
power supply: the refuge's life support went offline, with 219 people still
aboard. The explorer returned to the bay, took shelter, then descended through
reactor halls, workshops and frozen decks, relying heavily on the Funeral Rocket.
The run finished sixteen decks below the entrance, after three optional challenge
victories: **566 kills, no deaths**.

**3. Containment without answers — Mourning Glass, seed 14**

A survivor signal in a Cryosleep Gallery led toward the active catastrophe source
in the Reactor Choir. The expedition defeated a Firewall Saint, Choir of Glass,
Convoy Devourer and Foundry Marshal before purging the source. The threat was
contained, but its research was destroyed; 127 people remained aboard. After
extraction, the explorer pushed east and inward through sealed armories and
container mazes, using weapons including the Void Scalpel and Flechette Fan.
Six optional challenges were won before the hour ended at layer 26:
**770 kills, no deaths**.

**4. A partial dawn — Last Dawn, seed 9001**

A refuge signal in the Captain's Apartments led the rescue expedition to the
Crew Sepulchres. After defeating the threshold hunter and four phase guardians,
the explorer evacuated the local refuge: **153 people were rescued**, while
51 remained elsewhere aboard. The remote sleeper relay was not completed. With
the commission recorded at the bay, the explorer rested in the shelter and
continued westward and upward through command bridges, tactical theatres and
void observatories. Cryo Needler fire gave way to the Storm Relic later in the
run. The hour ended 33 decks above the entrance: **489 kills, no deaths**.

## Run record

| Run | Direction after extraction | Controller build | Extraction time | Distinct ship rooms entered | Optional victories | Final rank |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| [00](data/run-00/timeline.json), seed 713 | Inward | Offense | 09:35 | 186 | 2 | 43 |
| [04](data/run-04/timeline.json), seed 4 | Down, touring each deck | Survival | 14:24 | 173 | 3 | 32 |
| [05](data/run-05/timeline.json), seed 14 | East and inward | Toolbox | 15:55 | 246 | 6 | 46 |
| [10](data/run-10/timeline.json), seed 9001 | West, up and inward | Survival | 13:08 | 207 | 3 | 36 |

All four runs completed 216,000 updates each at 60 Hz: **864,000 updates, four
simulated hours, 2,526 kills and zero deaths**. Every run reached phase four,
extracted, used the shelter and passed its halfway save/resume. The longest
individual wall time was 395 seconds; two independent runs executed concurrently.
All **85 retained local checkpoints** loaded successfully through the ordinary
save loader. The shipped game code was unchanged for this review.

The existing controller uses ordinary movement, aiming, firing, lifts, interaction,
earned upgrades and equipment. Its profiles have bounded reaction time, turning
speed and aim error. It takes story choice **1** when offered; the different
outcomes above come from different generated commissions, not a sample of all
decision branches. It does not teleport, clear enemies or grant resources.
The hidden faction simulation and director remain enabled.

The runs also recorded navigation recovery and abandoned detours. Those are
controller observations, not scripted story incidents, and remain in the full
logs. Room counts deduplicate room owners, exclude the exterior and do not imply
that every entered room was fully cleared. Generated atmosphere descriptions
come from the room records; this was not a visual or physical-device test.

## Evidence and reproduction

- [Summary and exact counters](data/summary.json).
- [Checkpoint validation](data/checkpoint-validation.json).
- Each `data/run-*` directory contains a readable event timeline, narrative text
  extracted from its earned final save, exact execution/source hashes, compressed
  full JSONL log, and the extraction and final checkpoints.
- [Read-only narrative inspector](inspect.c) and [evidence collector](collect.py).
- The input controller is [tests/test_hidden_journey.c](../../../tests/test_hidden_journey.c).
  Reusing the older review's runner executes this current driver and current core;
  none of the older run results were reused.

Reproduce on Linux, choosing new output directories for each repeat:

```sh
cmake -S . -B build-playtest -DSC_BUILD_GAME=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-playtest --target sc_hidden_journey --parallel 4
python3 docs/reviews/2026-11-08-hidden-simulation/data/run.py \
  --binary build-playtest/sc_hidden_journey --runs 0 4 5 10 \
  --seconds 3600 --jobs 2 --output /tmp/starship-plots-new
cc -std=c17 -O2 -Isrc docs/reviews/2026-11-09-expedition-plots/inspect.c \
  build-playtest/libsc_core.a -lm -o build-playtest/plot-inspect
python3 docs/reviews/2026-11-09-expedition-plots/collect.py \
  /tmp/starship-plots-new build-playtest/plot-inspect \
  --output /tmp/starship-plots-report-new
```

Timelines take their timestamps from the running simulation. Checkpoint loading
into a fresh inspector does not restore elapsed wall/simulation time. The saved
narrative supplies titles and outcomes, while the live event log establishes
when each event happened.
