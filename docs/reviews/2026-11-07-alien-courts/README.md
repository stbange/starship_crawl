# Political expeditions: implementation evidence

The [player guide](../../../POLITICS.md) explains the controls and consequences.
This release implements the four proposed milestones: seeded political geography,
playable disputes, a bounded director, and royal-guard settlements. It contains
three government templates and one finite dispute per country per expedition.
The political finales supplement the four existing campaign guardians.

## Earned journeys

The [checkpoint provenance](checkpoint-provenance.log) begins with a fresh
character in space and earns all four phase victories through the normal game.
The [political journeys](journeys.log) continue that character through all three
outcomes. They move, aim, fire, use lifts and claim at physical seals; they do not
inject position, equipment, health, damage or defeated enemies.

| Country / outcome | Conduit: seconds / shots | Royal guard: seconds / shots | Saved result |
| --- | ---: | ---: | --- |
| Vesper Crown / shared supply | 55.72 / 20 | 3.25 / 8 | Peace and a powered protective conduit |
| Chitin Court / successor | 71.23 / 36 | 2.57 / 2 | New ruler, tribute and court workshop |
| Dead Concord / annexation | 63.72 / 35 | 5.05 / 18 | Changed territorial ownership and hostile rival patrols |

Conduit times include approach and nearby combat; finale times include the claim.
All three record zero deaths and zero combined health/shield/armor loss. The
successor route auto-equips its earned Gravity Mortar before the finale; others
use the carbine. These are accelerated automated runs with a strong late-game
build and bounded aim/reaction. They demonstrate functioning missions and saved
consequences, not human difficulty or fresh-character balance.

## Validation scope

All [36 native CTests](native.log) pass: 19 unit, 16 gameplay and one SDL platform
test. The [16 gameplay-labelled checks](gameplay.log) also pass separately,
including a complete rerun after the final neutral-objective correction.

[Unit coverage](coverage.log) measures every executable core source, with no
uncovered-line exclusions: 8483/8620 lines (98.41%), each source above 95%.
Unit tests cover every government/commission pair, neutral patrols, individual
standing, cooperative claims, director cards/budgets/expiry and save rollback.
A final compatibility check found that neutral patrols could contest story
terminals. The shared hostility check now leaves those objectives accessible;
[dedicated regressions](neutral-objectives.log) check peaceful, offended, co-op, sheltered and hacked
states. [Reward-bound checks](reward-bounds.log) also verify that settlement
and survey rewards preserve the stat/currency limits needed for saved profiles;
that final change also passes [ASan/UBSan/LeakSanitizer](sanitizers-bounds.log).
A pending recovery dispatch also survives save/resume with identical clocks,
budget, history and claims before it is collected.

The [focused sanitizer run](sanitizers.log) covers seven political, frontier,
living-ship, save and campaign checks with ASan/UBSan and leak detection off.
The [final focused run](sanitizers-final.log) adds LeakSanitizer for the political,
matrix fault and phased-expedition unit checks. These are not claims of complete
suite leak coverage. The final neutral-objective change also passes
[three focused ASan/UBSan/LeakSanitizer checks](sanitizers-neutral.log). [Native Mesa renders](native-render.log) cover all three
courts, including split-screen.

The optimized WebAssembly build initially exceeded its default 64 KiB stack in
`world_plan_version -> game_map_build -> deck_map -> hud -> renderer_draw`.
An optimized checked build identified that overflow. The release reserves 1 MiB
and keeps stack guards enabled. Linux, Windows x86-64 cross-compilation and
[Emscripten compile successfully](builds.log); Windows execution and physical GPU/controller
checks are still pending.

## Screenshots and human follow-up

Court architecture and audience images come from actual saving-disabled seeded
inspection scenes. Settlement journals come from imported gameplay-earned
checkpoints after real browser combat and claims. Mobile captures use Chromium
with emulated touch and viewport rotation. All seventeen packaged Chromium/WebGL suites pass across this implementation.
The [regression tour](browser-regressions.log) passes the fifteen existing feature
suites. Its first main-game check timed out during approach while two software
browsers and native checks competed for CPU; the [isolated final-package rerun](browser-main-final.log)
passes with the original assertions and timing limits. The final neutral-objective
and reward-cap changes received new unit coverage, focused sanitizers and portable
builds; the neutral-objective change also received another complete gameplay run.
Story, aftermath and death checks followed the neutral-objective correction;
the final main browser check ran against the final release package.

The [political browser suite](browser.log) passes all three physical courts,
three earned settlements, shared rewards, IndexedDB resume and touch controls.
None are concept art.

- [Divided Crown](divided-crown.png), [Molting Court](molting-court.png) and
  [Parliament of Echoes](echo-parliament.png): the three government projections.
- [Royal audience](audience.png) and [political map](political-map.png): decisions
  and tracked physical destinations.
- [Portrait audience](portrait-audience.png) and [landscape journal](landscape-journal.png).
- [Earned succession](succession.png): the new ruler, tribute relationship and
  repair workshop after a real guard victory.

Human playtesting should assess whether the country boundaries are legible,
whether commissions justify their travel, whether the royal guards need distinct
combat moves, and whether dispatches provide useful pacing without becoming
repetitive. Separated co-op should also check whether the shared dispatch location
and three-minute window feel fair. Three governments and four director cards are the initial content
library. Large autonomous wars, migration and additional governments remain
future expansions; existing settlements are never erased by an offline simulator.
