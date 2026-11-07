`generator-v1.sav` was written by the unmodified version-1 save writer from commit
3433429. It contains seed 123456, a defeated encounter, and two characters:

- Player 1: rank 7, 31 XP, 81 kills, 4 points, damage 3, vitality 2, tint 2.
- Player 2: rank 4, haste 2.

Migration must preserve these profiles, discard incompatible expedition geometry
and encounter records, announce a new expedition, and resume in solo mode.

`generator-v2.sav` was written by the unmodified version-2 core from commit
038c64b, before the infestation update. It contains seed 713, a defeated enemy
(ID 1) in room `(1, 0, 0)`, and two characters:

- Player 1: rank 7, damage 3, 81 kills, position `(96, 0.02, 23)`, checkpoint at that room's lift.
- Player 2: rank 4, haste 2.

Generator 3 must retain both character profiles and announce a fresh expedition,
discarding the incompatible old ship, positions, checkpoint, and encounter records.
The new profile receives starter weapons, armor, and a shield.

`generator-v3.sav` was written with the unmodified format-3 writer from commit
79014a0. It contains seed 713, mission stage 2, a defeated encounter (ID 0) in
room `(1, 0, 0)`, and player 1 at rank 7 with 81 kills and 123 salvage at `(96, 0.02, 24)`.
Format 4 must retain this expedition, character, and record without changing the
seed or mission. The sanctuary starts undeployed.

`generator-v4.sav` was written by commit 5e1e9f6's format-4 writer. It holds
seed 713, mission stage 2, rank 7, and a deployed shelter with player 1 inside.
Format 5 retains the expedition and sanctuary, initializing psionic effects inactive.

`generator-v5.sav` was written with commit cd4d0a5's unmodified format-5 core,
starting from the v4 fixture and activating player 1's veil and ten seconds of
flight. It retains seed 713, mission 2, rank 7 and the occupied deployed shelter.
Format 6 retains those states and records, grants the four new personal guns, and
initializes both fold rigs packed with full plating and flare upgrades at zero.

`generator-v6.sav` was written by the unmodified format-6 core from commit
e3f5013, loading the v5 fixture first. It retains the occupied shelter, powers,
rank 7, mission 2 and generator 3. Generator 4 must keep this expedition on the
old generator, including save/reload, until the player requests a new ship.

`generator-v3-fingerprints.txt` contains 12 region fingerprints captured from
e3f5013 before generator changes. `generator-v3-geometry.txt` contains 14 chunk
geometry/fixture/dressing fingerprints generated from that commit's source.
The quantized field hashing is in `geometry_fingerprint.h`; it never hashes
struct padding. Tests compare the supported generator-3 path against these
pre-change references, including the bay, introduction rooms and all nine decks.

`generator-v4-format6.sav` was captured from the unmodified core at commit
414ec58 before narrative work. It contains generator 4, seed 713, legacy mission
2 and a rank-7 character with 123 salvage. Format 7 must retain that story and
geometry, then start a generated story only on an explicit new-ship action.

`story-0-choice1.sav`, `story-0-choice3.sav`, `story-1-choice3.sav` and
`story-2-choice3.sav` are format-7/narrative-1 decision checkpoints. They were
produced by `./build/sc_campaign_tests --story-fixtures` using normal movement,
combat, lifts, progression and interaction, including save/resume mid-journey.
The motivations use seeds 1, 2 and 7 respectively. `choice3` files include the
optional operation; `choice1` has no optional preparation. The choice itself is
still uncommitted. Browser tests import these through the ordinary save UI to
exercise decisions, disabled options and persistence without runtime test hooks.

`generator-v4-fingerprints.txt` and `generator-v4-geometry.txt` contain 24
reference rows each captured from the unmodified core at commit 2cd9b89 before
generator-5 changes. They use seeds 713 and 41 across introductory and adjacent
regions and decks. The semantic geometry hashing excludes struct padding.

`history-0-arc0-choice1.sav`, `history-0-arc0-choice3.sav`,
`history-1-arc0-choice3.sav`, `history-2-arc0-choice3.sav`,
`history-3-arc0-choice1.sav` and `history-3-arc0-choice3.sav` are
format-8/narrative-2 checkpoints from `./build/sc_campaign_tests --history-fixtures`.
The ordinary movement/combat/lift driver saves after reaching the target, before
committing a decision. Arc 0 means direct commission; seeds are 11, 10, 5 and 14
for rescue, knowledge, salvage and containment. Choice-3 checkpoints include
optional preparation; choice-1 checkpoints do not. They exercise browser
imports, direct consequences, origin history, purge, prepared isolation and
keyboard/touch UI. Original format-7 fixtures remain unchanged; the old
`--story-fixtures` capture command is historical and has been removed to avoid
replacing their provenance.

The eight `saga-*.sav` files are format-9/narrative-3 snapshots produced by
`./build/sc_campaign_tests --saga-fixtures` with ordinary movement, combat,
upgrades, lifts and interactions. Seed 1 performs its primary rescue, brokers
the council agreement, and extracts two subsequent commissions. `power`,
`beacon` and `controller` are captured immediately before the corresponding
operation; `council` before its decision; `main` before the initial conclusion;
`continuation` after primary extraction; `follow` at a relay-prepared chapter
target; and `extracted` after chapter-one extraction. The fixture run completes
optional follow-on relays before capturing decisions, while its main target
uses council assistance without the original evacuation operation. The initial
and later choices themselves remain available in the decision snapshots.
Existing format-7 and format-8 fixtures stay unchanged.

`generator-v5-enemies.txt` was captured from the unmodified native core at commit `44ffa10` before adding machines. Its 16 rows cover four seeds and four locations, including the Warden room. Each row records seed, requested cell, encounter count and FNV-1a fingerprint of stable species IDs, millimetre-rounded absolute spawn coordinates and health. It is immutable compatibility evidence for generator 5, independent of the new generator-6 roster.

`generator-v6-format9.sav` was captured from the unmodified core at commit `382301d`, before matrix headers or format-10 code were introduced. It is a fresh seed-713, generator-6/narrative-3 expedition with both default profiles. Matrix migration tests require retention of the existing ship and progression, a fresh network record store, and default owned/equipped programs. It remains an immutable format-9 fixture. Matrix controller browser checkpoints are generated separately by the ordinary movement/combat driver `sc_matrix_journey --checkpoints DIRECTORY`, never by rewriting this fixture.


`generator-v6-format10.sav` was written by the unmodified core at commit
16f41b3, using `game_init(713)` and its original format-10 writer. It verifies
that the matrix-era sandbox resumes with its original character kit and rules.
Explicit new-ship creation then selects generator 7 with fresh profiles.

`phase-reward.sav`, `phase-attempt.sav`, `phase-midship.sav`,
`phase-unbound.sav` and `phase-decision.sav` are format-11/generator-7 checkpoints
from `./build/sc_phase_journey --fixtures tests/fixtures`. The seed-713 driver
starts in space, accepts the commission, walks/lifts/fights with the starter gun,
spends earned points, defeats the early elite and four guardians, investigates
the witness, reaches the generated target and extracts. These files capture an
unclaimed elite reward, an active attempt, the second phase after resuming, all
four boundaries open, and the generated final decision. Browser tests import
through the ordinary file UI; no runtime test control grants their progress.
Historical `--scene` catalog views remain generator-6 diagnostic sandboxes;
new progression tests start normal generator-7 games and import played saves.


`generator-v8-format11.sav` and `generator-v8-geometry.txt` were captured from
main's generator-8 core before implementing traversal (baseline 981983b). The save
places a seed-713 character at (6, .02, 24), inside the footprint a *new* ship will
use for stairs, with 11 XP and 23 salvage. The 20 semantic geometry fingerprints
cover four seeds and positive/negative deck and horizontal coordinates. Do not
regenerate these from generator 9. `vertical_traversal` checks exact old geometry,
profile/position retention and legacy lift behavior.


`generator-v9-format11.sav` and `generator-v9-geometry.txt` were captured from the
unmodified `ef568f5` core before generator 10. The save has seed 713, XP 17 and
29 salvage. Twenty rows contain seed/cell, semantic geometry hash and population
kind/position hash. `living_ship` verifies them exactly; never regenerate them
from the new generator.


`generator-v10-format11.sav` and `generator-v10-geometry.txt` were captured from
unmodified main `b2323b3e` before P1 code changes. The save has seed 713, XP 17 and
29 salvage. Twenty rows contain seed/cell, plan, geometry, enemy kind/position and
sprite kind/position fingerprints. These immutable references verify generator-10
retention and format-11 to format-12 migration.

`regional-expedition.sav` was produced by the generator-11
`sc_phase_journey --fixtures /tmp/p1-earned` run (its `phase-unbound.sav`), with
ordinary movement, combat, upgrades and guardian reward claims. The full run
completed extraction after 578.6 simulated seconds, 38 kills and zero deaths;
this checkpoint precedes extraction. It is rank 7 with damage attribute 10 and
all four boundaries open. `sc_regional_journey --prepare` loads it and walks to
relay `(13,-3,5)`, saving `regional-relay.sav` before activation. It fights with
the starter carbine, claims the earned Prism Lance, then walks to `(12,-3,8)` and
saves `regional-comparison.sav`. These format-12 checkpoints use no position,
health, inventory, damage or enemy injection. Unit fixtures and saving-disabled
inspection scenes are separate. Normal tests read these played checkpoints;
regeneration requires the explicit `--prepare` command.
