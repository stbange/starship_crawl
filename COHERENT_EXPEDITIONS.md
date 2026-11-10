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
combat effects; live snapshots preserve them. Other branch names reserve the
approved E7 content and are not offered by E1 rewards.

Attributes still have twelve ranks each. For the new rules, ranks above four
contribute half as much to damage, firing cadence and health/shield/armor/energy
capacity. A salvage purchase costs `200 + 60 × total invested + 40 × this rank`;
an available upgrade point pays instead. **Purchases restore no resources.**
Level requirements are `80 + 80n + 10n²`, where `n = level - 1`. Ascension grants
one point, 15 health, 10 shield and 10 energy, capped at current capacities; it
does not restore armor. Supplies, free sanctuary recovery and forgiving checkpoint
death remain available. Duplicate challenge rewards grant two points and 100
salvage, once per eligible participant, with deferred claims retained.

Three optional encounter combinations use the existing support-link warnings and
recovery windows: shielded swarm with ceiling pursuit, suppressor with flankers,
and machine support with slow area denial. They occupy four actor slots and retain
the room's twenty-actor limit. Destroying support collapses its visible links;
ordinary ranged and melee commitments also obey the group's attack schedule.

See the [implementation evidence](docs/reviews/2026-11-10-coherent-expeditions/README.md)
for matched simulated runs, isolated earned-build fixtures, captures and test scope.


## E2 — guardian systems

New ships use two initial patterns. The suspended furnace projects slow cyan
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
