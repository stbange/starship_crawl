# Reliable exploration — P0

These improvements apply to existing expeditions. Generator **10** and save
format **11** are unchanged; characters, room geometry and enemies are retained.

## Find the next connection

The HUD now gives one local action: enter the bay, use the next door, reach a
mission site, follow a service landing, or take a lift to a named deck. **AHEAD,
LEFT, RIGHT and BEHIND** are relative to your camera. Distances refer to the next
approach point, rather than the distant mission destination.

On both the minimap and **M** schematic, **cyan is the next connection** and
**gold is the final goal**. A door cue follows reciprocal generated passages and
respects phase boundaries. It can initially lead away from the goal to get around
a bulkhead. From the side of a room it first directs you onto the central cross
route. Cyan strips mark the chosen doorway in the world. Each split-screen player
has independent guidance to the shared expedition goal.

Guidance searches a rolling 16 × 16-cell area, at most 256 cells and 16 pure
region plans, and caches results between updates. It does not generate chunks,
reveal story discoveries or change the ship. It is a local route aid: furniture,
combat, roof ledges and jumps still need player judgement. An elevated, supported
position outside a service route displays a backtrack-to-floor reminder. Ship
guidance clears in the shelter and matrix. **I** shows the mission; **F10/U**
selects guardian routes; the existing story lead controls remain available.

## Read the vertical route

Deck signs and floor chevrons identify exits from grav pads into the room.
**Amber STAIRS** and **cyan RAMPS** label service routes; intermediate landings
show the flight number and a turn instruction. Guidance partway up a shaft
follows its switchbacks to the appropriate deck instead of pointing through the
floor to the pad below. At deck height, the cue crosses the rear landing before
turning toward the pad, keeping the route clear of its inner rail. The signs
work on negative decks too.

At a pad, preview the **UP and DOWN destination decks and room names**. Sealed
destinations say so. Press **E/Q**, controller **X/Y**, or the touch **UP/DOWN**
button to travel. An animated ride takes three seconds. **Release after arriving
before requesting another trip**: holding the button cannot carry you past a
landing. The arrival message names the deck and room and confirms the checkpoint.
Camera look remains under your control. Existing generator-8-and-earlier ships
retain their instant lift behavior.

Tall occupied rooms receive a restrained elevated light fixture: a hanging ring,
industrial light bars or a suspended prism, with district coloring. These provide
a distant point of reference while leaving global ambient brightness unchanged.
They are decorative, with no new collision. The existing bounded light pool is
retained, and each view draws only its occupied room's focal fixture.

## Clearer HUD

Room title, layer/deck, local action, minimap, defenses and weapon remain prominent.
Unspent points use a small **+N UPGRADES / I** reminder beside the status panel.
Locked-power lists and constant central upgrade banners are gone. Active powers
still show their timers. Ship identity, environmental context and detailed build
information remain in **F3**, **F6** and **F10**. Narrow windows place the header
beside the minimap and move the weapon readout above the defenses.

## What the seed-3 stall established

The retained review checkpoint puts the player at approximately
`(-66.09, -186.30, 48)`: on a connector roof, 5.7 metres above deck -3. The bot
walks west into a wall above the opening. At floor level, that doorway is clear.

`navigation_recovery` loads the original save, reproduces five seconds of blocked
forward movement, then uses ordinary strafe/jump input, gravity, walking and a
lift to return. It crosses the same doorway in both directions at floor level,
without a respawn, teleport, equipment grant or enemy removal, and saves/resumes.
This diagnoses the retained case as a controller/route-height problem. It does
not certify every generated route. The imperfect-input driver now uses actual
connection approach points and a five-second progress detector with bounded
movement recovery and replanning.

## More informative automated expeditions

`sc_phase_journey --profiles` runs two input/build profiles on **each** of seeds
713 and 42 through all four guardian phases and extraction, including an earned
reward and ordinary mid-expedition save/reload. Both use the starter carbine so
weapon changes do not obscure the comparison.

| Profile | Reaction delay | Turn limit | Aim error amplitude | Earned upgrade preference |
| --- | --- | --- | --- | --- |
| 1 | 0.30 s | 2.8 rad/s | 0.025 rad | Power, vitality, armor |
| 2 | 0.55 s | 1.8 rad/s | 0.040 rad | Early vitality, power, vitality, shield |

| Seed | Profile | Simulated duration | Kills | Deaths | Minimum shield | Minimum energy |
| --- | --- | --- | --- | --- | --- | --- |
| 713 | 1 | 12:10 | 265 | 0 | 0 | 71 |
| 713 | 2 | 13:54 | 242 | 1 | 0 | 59 |
| 42 | 1 | 14:50 | 369 | 1 | 0 | 4.4 |
| 42 | 2 | 17:10 | 332 | 5 | 0 | 0 |

All four finished without navigation-recovery activation. Resource minima and
net defense reductions are sampled **every update**, not every thirty seconds.
`damage_frames` counts updates with an observed defense reduction; regeneration,
level rewards and same-update respawns can mask incoming damage, so these are
not total damage or exact hit counts. Simulated time is accelerated headless time,
not human playtime. Target selection and route knowledge remain automated.
These profiles expose pressure missed by ideal-aim runs and do not establish
human balance. [Raw profile results](docs/playtests/p0-profiles.json).

Run `ctest --test-dir build -R 'exploration_guidance|navigation_recovery|imperfect_expeditions' --output-on-failure`.
See [human checkpoints and device results](docs/playtests/P0_CHECKLIST.md) and
[the full verification guide](TESTING.md).
