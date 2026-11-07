# P0 human and physical-device playtest

Status: **pending human/physical execution**. Automated Linux/Mesa, Chromium,
virtual controllers, touch emulation and Windows cross-compilation are recorded
separately in TESTING.md. Copy this sheet for each session; leave unfinished
checks pending instead of marking them passed from automated results.

## Prepare

Use the latest main build or https://stbange.github.io/starship_crawl/ after its
deployment succeeds. Export your normal save first. Use a separate browser
profile or native `--save-dir` for these tests.

For a fresh first-play route, start `./build/starship_crawl --seed 713 --no-save`
or open the web game with `?seed=713` in an empty browser profile. Start solo.
Keep F3 diagnostics closed for the first five minutes. Record how long it takes
to identify the commission console, next door and first challenge without hints.

Inspection scenes (`--scene` or `scene=...`) disable saving. Arrival and imported
checkpoints use normal saves unless native `--no-save` is supplied.

| Checkpoint | Native option / web query | Check |
| --- | --- | --- |
| Arrival | `--seed 713` / `?seed=713` | Identify bay, enter, find and use the gold console, follow cyan next connection. |
| Lift landing | `--seed 713 --scene lift` / `?seed=713&scene=lift` | Read both destinations before departure; hold UP for at least six seconds; stop on deck +1. Release and return DOWN. |
| Stairs | `--seed 713 --scene stairs` / `?seed=713&scene=stairs` | Follow amber route all eight flights up and down without jumping; explain which direction reaches the room. |
| Ramp | `--seed 713 --scene ramp` / `?seed=713&scene=ramp` | Repeat on cyan route, then descend a deck below the bay. Check negative deck labels. |
| Scale/readability | `--seed 713 --scene volume1` / `?seed=713&scene=volume1` | Find the elevated focal fixture, exits and enemy silhouettes with lamp on/off; keep the dark atmosphere. |
| Earned build | Import `tests/fixtures/phase-midship.sav` through Menu → Import | Follow a new lead, inspect I/F6/F10, find unspent upgrades without central banner clutter. This retained fixture uses its original generator. |
| Roof regression | Import `docs/reviews/2026-11-06-expeditions/data/3/phase-minute-24.sav` | Reproduce the blocked westward movement. Move sideways off the roof, descend to supported space, find the pad and return to deck -3. Cross the doorway at floor level. |

Save imports reconstruct enemy populations and cancel interrupted guardian
attempts under the normal rules. They are checkpoints, not a recording of the
original fight. Native `--smoke` intentionally does not load save files.

## Human design checkpoints

At **5 minutes**, explain the distinction between cyan next connection and gold
final goal, and locate an alternative route using M. Record wrong turns and time
spent looking for exits.

At the **first earned reward**, explain what was unlocked, choose it and find the
build controls. Record confusing prompts, text overlap and whether the small
upgrade reminder is visible enough.

At **15 minutes**, record damage surprises, deaths, time lost, useful discoveries,
and which navigation cues helped. State whether signs and focal lights preserve
the gloomy atmosphere and whether lift arrivals encourage leaving the shaft.

At **25 minutes or extraction**, describe the next goal in your own words. Record
travel/fight/reward/reading time approximately, enjoyment and any repetitive or
frustrating section. A player needing help is useful evidence, not a failed test.

## Physical platforms

For Windows, use a Visual Studio developer PowerShell with CMake installed:

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel 4
.\build\Release\starship_crawl.exe --seed 713 --scene lift --no-save
```

On a phone, open the [lift checkpoint](https://stbange.github.io/starship_crawl/?seed=713&scene=lift)
after the main deployment succeeds, tap **Board the ship**, then hold the visible
**UP** button for six seconds. Release, then hold **DOWN** to return. Continue with
the [stairs checkpoint](https://stbange.github.io/starship_crawl/?seed=713&scene=stairs).


| Platform | Procedure | Expected result | Actual result |
| --- | --- | --- | --- |
| Windows native GPU | Build/run native; perform lift/stairs/roof checkpoints; resize to 640×480; Alt-Tab while moving/firing; save/restart. | Correct graphics, deliberate lift travel, readable navigation and no stale held input; saved character retained. | PENDING |
| Windows physical controller | Solo then optional F2; hold/release X/Y through a ride; unplug/replug during movement; change sensitivity. | One trip per deliberate request; correct ownership, pause and restored controls. | PENDING |
| Android Chrome | Actual handset, landscape, simultaneous move/look/fire; hold/release UP; rotate while holding; reopen after backgrounding. | Independent touches, clear controls/HUD, no repeated ride or stale input; usable fresh gestures. | PENDING |
| iPhone/iPad Safari | Repeat Android route; save/reload/import and compare Auto/180p. | WebGL scene, storage and touch behavior work on the physical device. | PENDING |
| Optional physical split-screen | One player rides while the other explores; open/pan one map; enter shelter with one player. | Independent cameras and cues; shared mission, no ship markers in shelter. | PENDING |

## Result record

- Date / tester / prior experience:
- Commit or Actions run / URL:
- OS / browser version / GPU / device / controller:
- Resolution / quality setting / solo or split:
- Seed / checkpoint / first-time or repeat:
- Results for each row (PASS / FAIL / PENDING):
- Times at checkpoints / deaths / assistance required:
- Wrong turns, unclear signs, HUD overlap (screenshot if useful):
- Stutter/input latency observations (measured FPS only if actually measured):
- Exact reproduction steps and exported save for any failure:
- Three changes that would most improve the next run:
