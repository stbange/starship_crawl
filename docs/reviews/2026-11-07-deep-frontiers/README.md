# Deep-frontier implementation evidence

This implements gameplay-review items **2, 4, 5 and 6**. The
[player guide](../../../DEEP_FRONTIERS.md) describes the rules; the
[testing guide](../../../TESTING.md) describes validation scope.
These are automated, accelerated journeys and inspection captures, not human
playtests or sixty-minute wall-clock expeditions.

## Earned-equipment comparisons

The [checkpoint provenance log](checkpoint-provenance.log) starts seed 713 in
space, earns all four phase victories and extracts. The retained checkpoint
precedes extraction. The [encounter log](encounters.log) continues from that
earned character, walks to new arenas, claims rewards, refits through a real
shelter and compares the same next arena twice. Both runs use the starter
carbine, with 0.35-second target reaction, bounded turning and angular aim error.
Only normal tool use differs. The matrix route casts Veil and runs SECURITY at
the linked physical room's console. No position, enemy, damage, health or
equipment injection supplies this progress.

| Arena | Ordinary approach: seconds / shots | Specialist approach: seconds / shots | Observation |
| --- | ---: | ---: | --- |
| Hanging Aerie | 2.80 / 7 | 3.33 / 5 | Flight saves shots but costs energy and time to rise/land |
| Coat Colony | 2.72 / 7 | 3.05 / 4 | Nova costs energy and needs a closer approach |
| Suppression Battery | 15.92 / 15 | 20.20 / 11 | Mech boarding, movement and dismounting outweigh a shot saving here |
| Courier Circuit | 10.67 / 11 | 8.27 / 11 | Both pylons reached at 6.85 s on foot versus 4.23 s in the wagon |
| Cipher Lock | 3.52 / 10 | 2.83 / 5 | The actual matrix service exposes the squad; time includes its short avatar walk |

Times include returning to claim. Circuit runs deliberately complete the same
course before firing. The on-foot circuit also kills a nearby fifth enemy; the
wagon run incurs 22 combined health/shield/armor loss. Other comparisons record
zero such loss; all ten finish without a respawn. Mech plating loss is separate
from the logged player defense total. Unit checks independently verify its armor
and the directional relic repair effect.

These strong late-game builds still clear some fights very quickly. Specialized
tools are alternative approaches, not universally faster solutions. Human tests
should assess fresh-character pressure, readable warnings, rewards worth detouring
for, the audio mix and whether loadout choices feel distinct. Broad combat/resource
balance was outside this selected implementation scope.

## Captures and provenance

Screenshots come from actual Chromium/WebGL frames. Architectural/creature views
use explicitly saving-disabled inspection scenes. The relic journal comes from
an imported gameplay-produced checkpoint after real shots and an earned claim.
None are rendered concept art.

- [Broken turbine](broken-turbine.png) and [rooted sun](rooted-sun.png): district focal structures.
- [Courier circuit](courier-circuit.png) and [mirror contract](mirror-contract.png): active optional arenas.
- [Colony](alien-3.png) and [molting sentinel](alien-4.png): distinct procedural bodies and readable hints.
- [Earned relic journal](relic-journal.png), [portrait journal](portrait-journal.png) and [mobile contract](mobile-portrait.png).

The [new browser suite log](browser.log) covers all nine generated focal views,
six encounter patterns, five contract hazards, ten specialist presentations,
quiet crouching, gaze freezing and actual molting combat. It also exercises
earned claims, relic evolution, duplicate-claim protection, IndexedDB resume,
split-screen participation and real CDP touch USE. The final package additionally
checks the revised creature labels and desktop/portrait journal after a saved
victory; [read-only final state](final-browser.json).

All sixteen browser suites passed; [combined output](browser-regressions.log).
The final mobile labels use wrapped text with a readable minimum glyph size:
[aerie tactics](portrait-front1-active.png), [courier tactics](portrait-front4-active.png)
and [landscape circuit](landscape-circuit.png). Native Mesa also passed the final
split-screen circuit smoke check.

All 33 ASan/UBSan core checks completed successfully after extending only the
four-profile instrumented run's wall-clock allowance. Its first 600-second run
timed out; the rerun passed in 925.64 seconds under a 1,200-second allowance.
Gameplay frame/death assertions remain identical. The final new-feature unit
suite and four-profile run also pass leak detection; the other 32-test sanitizer
results used leak detection disabled. [Sanitizer results](sanitizers.log).

Native unit/gameplay, coverage, portability and sanitizer results are recorded in
[TESTING.md](../../../TESTING.md). Software rendering and emulated touch do not
certify physical GPUs, Windows controllers, Safari, mobile performance or human
combat balance. Start a **new ship** for generator 12; older saves intentionally
retain their original architecture and populations.
