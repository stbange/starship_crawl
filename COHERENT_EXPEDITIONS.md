# Coherent expeditions

The E1–E8 implementation is in progress. This document describes completed
mechanics as they pass their milestone checks. Generator 15 and older saves keep
their published rules. Generator 16 / campaign 2 / checkpoint 17 is the unreleased
new ruleset; live snapshots use version 2 with a checked schema fingerprint.

## E1: build choices and growth

The opening still grants only the carbine, flashlight and maps. The earned
shotgun, shelter, protected first matrix link, powers and rigs follow the existing
[onboarding](ONBOARDING.md) schedule. A specialization never grants its required
weapon, power or rig. Each character can equip two guns, two powers, one rig and
one specialization branch. Owned equipment stays in storage.

At a safe refit location, select the specialization slot in F10 and cycle owned
branches. The preview explains its requirement and cost. **G** equips the preview;
**Enter** cancels it when closing the journal. Gamepad uses **A**, touch has **Equip
previewed specialization**, and the Quest build menu has the same action. The
server-independent command validates ownership and safety again at confirmation.

| Branch | Requirement | Behavior and limit |
|---|---|---|
| Conduit / Flare Relay | Equip a beam and the flare cannon | A beam passing through one of your attached flares sends up to three visible relays to different visible enemies within 18 m. Each deals 45% beam damage and costs 3 extra energy. Walls block them; they do not relay again. |
| Veil / Echo | Equip Veil | Firing from cloak leaves a three-second lure. Nearby ordinary hostiles pursue it when they can see it. It gives no damage bonus, loot or kill credit. |
| Bastion / Capacitor | Equip the mech | Enemy attacks absorbed by mech plating charge the next autocannon shot, up to 120 charge and 180 extra damage. Environment damage gives no charge. Packing/refitting clears it. |

Only the selected branch operates. Guardians and faction formations keep their
own commitments rather than being erased by a decoy. Echoes freeze when everyone
is sheltered and resume without a catch-up burst. Checkpoints clear transient
combat effects; live snapshots preserve them. E7 completes the remaining branches
listed below; reward selection covers all eight.

Attributes still have twelve ranks each. For the new rules, ranks above four
contribute half as much to damage, firing cadence and health/shield/armor/energy
capacity. A salvage purchase costs `200 + 60 × total invested + 40 × this rank`;
an available upgrade point pays instead. **Purchases restore no resources.**
Level requirements are `80 + 80n + 10n²`, where `n = level - 1`. Ascension grants
one point, 15 health, 10 shield and 10 energy, capped at current capacities; it
does not restore armor. Supplies, free sanctuary recovery and forgiving checkpoint
death remain available. Duplicate challenge rewards grant two points and 100
salvage, once per eligible participant, with deferred claims retained.

The initial three optional encounter combinations use the existing support-link warnings and
recovery windows: shielded swarm with ceiling pursuit, suppressor with flankers,
and machine support with slow area denial. They occupy four actor slots and retain
the room's twenty-actor limit. Destroying support collapses its visible links;
ordinary ranged and melee commitments also obey the group's attack schedule.

See the [implementation evidence](docs/reviews/2026-11-10-coherent-expeditions/README.md)
for matched simulated runs, isolated earned-build fixtures, captures and test scope.


## E2 — guardian systems

The first two patterns establish the common rules. The suspended furnace projects slow cyan
rings from its coolant assembly and orange fans from its gun platform. The
infested relay circles the player while its root heart is intact; its spore lung
creates a second firing origin. Shoot the lit assemblies or concentrate legal
burst damage on the core. Destroying the first stops rings (and the relay's
pursuit); destroying the second stops cross-fire. Each destruction exposes the
core for five seconds. There are no phase damage caps or replacement health.
At 55% core health, or with both systems broken, the warned overrun stage changes
the volley width and cadence. Warnings lock their aim for 1.2 seconds. Ordinary
shots remain slow enough to dodge with the starting movement controls.

Two marked bypass controls beside the arena's staging area offer physical
preparation with the normal interact action. Coolant bypass and weapons-feed
sabotage persist through retries. After earning matrix access, the corresponding
Power and Security services set the same room facts; doing both methods grants
no second advantage. Neither preparation nor hacking is required to win.

Assemblies belong to the attempt and award no independent XP, loot or kill.
Retreat, all participants sheltering or dying, and loading an active checkpoint
reset them. A partner remaining in the arena keeps the attempt active. Live
snapshots retain exact health, warnings, projectiles and broken-system state.
Preparation and victory remain durable. Victory clears guardian shots, preserves
per-player reward eligibility, raises the overhead machinery and illuminates the
breach frame while opening the next phase boundary. The floor and cover platforms
remain fixed throughout, including under vehicles and both co-op players.

`guardian0warn`, `guardian0break`, `guardian0breach` and the equivalent `guardian1`
scenes are explicit render inspection fixtures. They are not fresh-character
journeys or evidence of human readability. The guardian journeys use Session
input with only the starting carbine and no in-fight stat edits.


## E3 — the Drowned Foundry

Generator 16 reserves a regional vista, a preparation control and a workers’
refuge in the existing connected room graph. The Foundry hangs a reactor above
small homes, with flooded lower ground, a normal-gravity stair and catwalk route,
and a sealed return conduit. Optional incompatible content yields to existing
mission and passage reservations; small compatible rooms use a compact version.
The opening remains an introductory exploration/shooting route.

Use **G / interact** at the pump to restore district power. Amber indicators give
two seconds’ warning before ordinary dormant robots wake. Guardian assemblies,
faction forces and already engaged enemies retain their own behavior. Power
opens a dry center channel; the raised path is always usable without a power.
Shoot the marked conductor to warn, then electrify only its connected left pool
for five seconds. The opposite pool and high ground remain safe. Existing matrix
Power operations set the same durable power fact after matrix access is earned.
A severed conductor stays severed through saving, eviction and backtracking.

Victory opens the local return conduit across the vista. It checks destination
readiness, collision and support before moving either a walker or an earned rig.
No floor or required exit disappears. District identity and outcomes persist;
active electrical warnings are exact in live snapshots and reset at a checkpoint.
`foundry0` through `foundry4` are explicitly prepared visual inspection scenes.


## E4 — inhabitants and consequences

New expeditions begin with an engineer and an archivist at reachable workers’
refuges. Their names, knowledge and fates belong to this ship. Use the normal
interact action to read a short record and review optional choices. Copying the
route information is free. Repair costs 60 salvage, or nothing when district
power is already available. Stripping the refuge grants 90 salvage once and
forces its inhabitant to evacuate. The menu declares these costs before selection;
co-op shares the outcome while charging or rewarding only the acting character.
Required objectives never depend on a conversation or escort.

A helped individual can move to a deeper refuge after a guardian opens its band.
Migration follows the connected navigation graph, leaves a service-route note,
and waits to arrive when neither player observes the destination. Their later
record acknowledges the earlier repair. A captured refuge can lose an unhelped
inhabitant; an emergency recorder preserves the information. Known individuals
appear as violet map markers, with field records available through I and L.
Evacuated or lost individuals retain a grey marker and a recorder. The debrief
counts local repairs, evacuations and known losses.

Hidden campaigns now connect supply to siege and repair to factory production.
The supply crew keeps its force identity, health and casualties when it begins
its siege; supplies are consumed once. Heavy losses cause retreat. A repaired
factory waits 30 active seconds before paying the normal resource cost to launch
a separately identified reinforcement force. Blackout or hostile occupation
interrupts production. A repair crew returns independently. Existing physical
combat, distant travel, ownership, wreckage and damaged/repaired machinery show
these outcomes; no strategy-management interface is added. Both physical and
matrix power operations update the same faction resource facts.

## E5 — useful shelter preparation

The earned fold keeps its free bed, medicine and basic stew. Use G at the dining
place for optional meals, or the locker-side bench for projects. Each meal costs
30 salvage and lasts five minutes outside the fold: insulated broth halves
conductive electrical damage, ember stew halves cold damage and slowing duration,
and charged tea restores six additional energy per second. Only one meal is
active per character; replacing it is a deliberate purchase. Shelter time pauses
its duration. An identical active meal cannot charge again.

The shared refit bench costs 250 salvage to build. A personal refit then costs 25,
returns every invested attribute point and clamps resources to their new
capacities, without healing or unlocking equipment. The shared flare calibrator
costs 200 and requires an earned flare cannon. Subsequent personal preparations
cost 75: the next successfully attached flare lasts twice as long and shines 50%
more strongly, with the existing light count and radius limits. A miss retains
the preparation. Research costs 100 per physically observed ship condition and
records its counterplay. Unknown conditions cannot be researched. Shared projects
charge only their purchaser; a partner with an outdated menu must review the
updated choices before confirming.

Six fixed shelves recall actual district outcomes. Repeated visits update the
same keepsake, without rewards; additional outcomes remain in the I/L field
records. Projects, meals, preparations, research and keepsakes belong to the
current expedition and survive save/import, then clear with a new ship.
`home0`, `home1` and `home2` are explicitly earned visual fixtures for free
recovery, optional meals and late-expedition preparation.

## E6 — hidden expedition rhythm

Generator 16 connects existing opportunities into at most two concurrent
sequences: a stalking creature, an approaching formation, a dormant machine and
a recovery supply trail. Scratches, spent shells, maintenance indicators and
field-kit lamps provide physical clues. Existing creatures retain their identity,
health and commitments. The machine sequence follows actual restored power; it
does not secretly bypass the pump. Supplies use the existing field-kit claim.
Ambient intensity changes subtly with locally witnessed anticipation and aftermath.

Reservation costs come from the existing three-point director budget. Before a
commitment, departure for 30 active seconds or recovery cancels the reservation
and refunds it once. Known clues remain in room records. A defeated creature or
retreating formation leads to aftermath instead of a replacement encounter.
Aftermath withholds new directed pressure for 45 active seconds; autonomous
creatures and wars continue their existing simulation. Both players' views,
engagements, required objectives and already observed rooms protect content.
Selection favors nearby rooms on the player's deck; it never generates rooms.

Only actual exploration and simulation advance these sequences. Map inspection
and rendering are read-only; all-player shelter, matrix-only exploration and
session suspension pause their clocks. Checkpoints preserve sequence and budget
state, and live snapshots preserve the exact active state. The normal interface
has no sequence stages, pressure scores or director controls. `rhythm0` through
`rhythm3` are labelled cue-inspection fixtures, not gameplay evidence.

## E7 — ships worth replaying

A new seed selects one dominant condition. Its identity and 24-second cycle are
shared by districts and survive checkpoints; time pauses when nobody is outside
in the physical ship. Region offsets are deterministic, so rooms have distinct
phases without changing their rule. The center strip and raised approaches remain
usable with starting movement and weapons.

| Condition | Rule and ordinary counterplay |
|---|---|
| Awakening machines | Restoring power warns for two seconds, wakes robots and drains the center route. Shooting the powered conductor warns for two seconds, then electrifies the left pool for five. Use dry ground. |
| Freeze / vent cycle | Three seconds of flashing amber precede a five-second frost sweep; sixteen seconds remain clear. Exposed floor deals 12 damage/second and slows creatures. Shoot the vent feed, isolate it through earned matrix access, or use the permanent safe strip. |
| Living conduits | Powered growth warns for four seconds, feeds for eight, then rests for twelve. Exposed floor drains the player at 10 damage/second; resident organic creatures recover 8 health/second, within their existing natural maximum. Burning creatures, robots, guardians and travelling forces receive no regeneration. Cut the powered living feed, isolate it, or take the safe route. |

| District family | Machines | Vents | Conduits | Landmark |
|---|---|---|---|---|
| Drowned Foundry | Yes | — | Yes | Suspended reactor above workers' homes |
| Frozen Terrace | Yes | Yes | — | Tiered chandelier over abandoned luxury cabins |
| Command Reliquary | Yes | Yes | Yes | Archive fins, towering shelves and a suspended reliquary |
| Transit Garden | — | Yes | Yes | Overgrown transit frames, hanging cloth and fungal growth |

Directional order, heat and seeded variation choose compatible families. A
fixed fallback preserves required connections when a preferred family does not
fit. Compact rooms combine the preparation/refuge contracts; incompatible small
rooms omit the optional district. The existing chunk, actor, sprite and light
budgets remain unchanged. One existing overhead lamp reveals the landmark.

| Additional branch | Requirement | Behavior and limit |
|---|---|---|
| Conduit / Focus | Equipped beam | Maintain contact with one target for 1.2 seconds; spend 12 energy for a double-damage piercing pulse through at most three aligned hostiles. Changing targets or missing the firing window loses focus. Walls bound the lane. |
| Veil / Ambush | Equipped Veil | The first hit from a cloaked shot marks one creature for six seconds. Its defeat releases one 8 m burst, up to eight visible hostiles, for 70 plus 8 per effective power rank. No recursive bursts. |
| Bastion / Brace | Equipped mech | Stand grounded and still for 0.75 seconds: frontal hostile damage is halved and stores up to 100 counterwave charge. Firing with at least 20 charge releases it within 6 m ahead. Moving or packing clears it; protected refuges cannot charge it. |
| Rift / Landing | Equipped long jump | A committed powered jump arms one landing pulse: 65 damage within 7 m, at most eight visible hostiles. Ordinary walking/falling, flight and rigs do not arm it. |
| Rift / Field | Equipped flight | While flying, spend 2 extra energy every quarter-second for an 8-damage field within 3.5 m, at most eight visible hostiles. Insufficient energy ends flight. |

Only one branch operates at a time. Pulses cannot damage allies or creatures
through walls. Live snapshots rebind marked/focused actor identities; checkpoint
loads clear these attempt-local effects. Inspection scenes `build1`–`build8`
explicitly grant earned equipment and are not fresh-character progression.

The three additional four-actor encounters are **cold mirror / brittle pack**,
**root colony / carrion wave**, and **listeners / return fire**. Cold districts
favor brittle and suppressor compositions; conduit districts favor colonies and
listeners. Destroy support first, break a brittle creature's commitment, interrupt
a colony, or stop firing while repositioning against return-fire specialists.
Existing staggered attacks and support openings remain in force.

Four guardian patterns cover the four primary phases; alternate sites rotate the
pattern. The **Cryo Processional** projects a rotating, warned cross from its vent
array and a second fan from its shard mirror. Destroy the array to remove the
cross or the mirror to remove cross-fire. The **Garden Locomotive** commits to a
locked charge after its warning, with a drain shot and seed mortar. Destroy its
root drive to stop charges, or its mortar to remove the second firing origin.
Both retain two stages, physical preparation, starter-carbine viability and
persistent victory breaches.

Each ship selects two or three unique recurring individuals from six roles:
engineer, archivist, caretaker, deserter, pilgrim and scavenger. Their needs,
condition advice and four motivation-specific route alternatives share the same
power, refuge and fate facts. Individuals keep their names and identities through
later meetings, repeated commissions and extraction; a new ship clears them.
Compacted event histories retain outcome totals and current facts. Duplicate
branch rewards cap points at 1,000 and salvage at 1,000,000, without a repeat claim.
