# P1: regions, alien behavior and optional encounters

The generator-12 [Deep frontiers follow-up](DEEP_FRONTIERS.md) implements review
items 2, 4, 5 and 6. The evidence below records the earlier P1 release; its retained
generator-11 fixtures continue to protect those worlds and mechanics.

This implements the agreed P1 scope from the [first gameplay review](docs/reviews/2026-11-06-expeditions/README.md)
and [deep directional review](docs/reviews/2026-11-06-deep-expeditions/README.md).
P1 introduced generator 11; current new ships include P1 and its follow-up. Existing ships retain their generator,
geometry, populations and progression. P1 used format 12; the current format-13
writer still reads supported earlier saves. New ships start with fresh characters.

## Implemented checklist

| Review point | Shipped behavior | Checks / human checkpoint |
| --- | --- | --- |
| Hot/cold variety | Fourteen conditions, adding boiling reservoirs, pulsing pressure vents, ash, baked silence, ice crystals, frozen runoff and brittle stillness. District function, incident and seed choose variants within the larger heat/cold direction. Six compatible ecologies remain represented in both extremes. | Seed sampling and immutable generator-10 references; browser weather tour; compare `--scene biome7` through `biome13`. |
| Three alien prototypes | Backwards pilgrim approaches with its body facing away while its head tracks you; it visibly prepares a fixed rush. Pendulum gazer swings around a steady eye, locks an aim line and fires a finite projectile. Seam-runner grips actual walls with alternating planted limbs, pauses, then commits to a rush/drop. | Collision, planted contacts, locked aim, evasion, interruption and one hit per commitment tests; observe `--scene relay` and `nest`. |
| Coordinated nest/relay | A support and three different guards occupy four existing enemy slots. Visible links shield guards and stagger attacks. Beam pressure disrupts links for two seconds. Destroying support disrupts the pack before its survivors attack independently. | Activation, linking/occlusion, stagger, interruption and collapse unit checks; real-input browser combat; earned-tool comparison journey. |
| Useful optional rewards | White map beacons preview an unowned gun, power, rig or module. Activate with **G / touch USE**; exits stay open. Return after winning to claim. Each participating player gets one durable claim. Exhausted reward pools yield two points and 100 salvage. | Co-op, late participation, reload, revisit, duplicate refusal and earned checkpoint checks. |
| Reward teaching | A 90-second contextual prompt explains the real earned tool near the cleared site. Firing its gun, activating its power, boarding its rig or using its equipped module completes the practice prompt. **F10** manages loadouts. | Per-player input tests ensure another player's fire cannot complete your prompt. Practice grants no extra equipment or repeat loot. |
| Tension and release | Quiet rooms, small trace encounters, ordinary hordes, optional challenges and quiet staging near guardian rooms. Main-story/survivor routes keep their existing roles. | Seeded pacing distribution, supported spawn and console checks; compare a normal expedition with a challenge detour. |
| Small story consequences | Eight local actions link room evidence to repairs, food, medical care, wards or machine shutdown. Restored consoles turn green, floor markers light, power returns and moving atmospheric effects subside. Completion and its one-time reward persist. | Every tale tested through save/reload; real-input grid repair before/after captures. |

## Playing the encounters

White squares on the map mark optional challenges. Their beacon names and reward
preview distinguish a **Choir Relay** from an organic **Nest Conductor**. Approach
its console and press G. Dormant squads cannot attack or be farmed. You can leave;
previously defeated identities remain defeated, while a new activation resets
unfinished attack timing. Shelter or matrix withdrawal also suspends an unattended
encounter. Living support exposes the next attacker with an amber link and pulse.
Shoot the support with any weapon, or pressure it/a linked guard with a beam.
Dodge the amber attack line; the creature cannot instantly retarget its commitment.

Reward practice names the controls: V/B/J/H for powers, T/Y then G for rigs,
and CAST or MENU/USE on touch screens.

The archetypes also appear among compatible normal species. Existing species
abilities such as brood spawning remain available outside the bounded support
encounter. This adds behavior to the existing roster, rather than introducing
additional species names. Surface grips use collision rays, not teleportation.

Local amber consoles sit beside the existing small-story evidence; F6 still opens
the room account. Clear nearby hostiles and use the console. Evacuation markers,
the auxiliary grid and containment valve restore power; the galley/garden provide
nourishment; triage restores health and armor; pilgrim/garden/triage wards activate
safe beacons; the mutiny relay silences local machines. All award 35 salvage once.
The main expedition objective remains available.

## Automated combat observations

The retained `regional-expedition.sav` is an earned seed-713 checkpoint from the
normal phase journey: all four boundaries open, rank 7, damage attribute 10.
`sc_regional_journey --prepare` walks from there to `(13,-3,5)`, earns the Prism
Lance through normal combat, then walks to `(12,-3,8)`. The corresponding relay and
comparison checkpoints are importable in the browser. No movement, equipment,
health or enemy mutation creates these fixtures.

The comparison reloads the same second checkpoint for each run: rank 15, damage
attribute 10, after earning additional ranks along the approach. Both guns are
already owned; selection uses ordinary weapon cycling. The driver moves, aims,
fires and interacts through production APIs, with limited turning, reaction delay
and aim error. It records health/shield/armor loss, energy, shots, kills and recovery.

| Reaction / angular aim error | Equipped gun | Simulated fight + claim | Shots | Lowest energy | Disrupted frames |
| --- | --- | ---: | ---: | ---: | ---: |
| 0.30 s / 0.025 rad | Starter carbine | 2.10 s | 5 | 100 | 28 |
| 0.30 s / 0.025 rad | Earned Prism Lance | 2.00 s | 6 | 82 | 89 |
| 0.55 s / 0.040 rad | Starter carbine | 3.27 s | 6 | 100 | 41 |
| 0.55 s / 0.040 rad | Earned Prism Lance | 4.03 s | 9 | 81 | 183 |

All four killed the squad without damage or respawn. Disrupted frames include the
support's death collapse as well as beam pressure. These observations demonstrate
working tools and their tradeoffs, **not validated human difficulty**. This strong
late-game build still clears optional fights quickly. Fodder health remains capped;
a broad balance increase is not inferred from a bot. Early-game activation,
telegraph readability, voluntary detours and the value of rewards need human play.

## Reproduction and remaining human checks

```sh
cmake --build build --parallel 4
ctest --test-dir build -R 'regional_encounters|regional_journey' --output-on-failure
mkdir -p /tmp/phase-checkpoints
./build/sc_phase_journey --fixtures /tmp/phase-checkpoints
# Fixture generation is explicit; routine tests only read retained checkpoints.
./build/sc_regional_journey --prepare
python3 tests/test_regional_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
python3 tests/test_habitat_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
```

Native inspection: `--seed 713 --scene relay`, `nest`, `local0`…`local7`, or
`biome0`…`biome13`. Web inspection uses `?seed=713&scene=relay`, etc. These inspection
scenes are saving-disabled views; they do not grant normal expedition progress.
Import `tests/fixtures/regional-relay.sav` for an earned, saveable challenge start,
or `regional-comparison.sav` for the earned beam comparison.

Human checks still pending: start with a fresh weak character, read the support
pulse without prior explanation, evade each prototype, decide whether the preview
justifies a detour, try a reward without opening documentation, and revisit a local
repair after exploring elsewhere. Repeat in split-screen and on a physical mobile
GPU. Automated Chromium touch and software rendering are not physical-device or
human playtests. Screenshots and comparison logs are in the [P1 evidence](docs/reviews/2026-11-07-p1/README.md). Full technical validation is recorded in [TESTING.md](TESTING.md).
