# Expedition progression and directional worlds

Discussion draft, 5 November 2026, on `design/expedition-progression`.
This proposes the next design direction; it does not change current gameplay or
replace the implemented OpenSpec requirements. Numbers below are playtest targets.

## The experience to build

Enter a ship with a purpose and a workable character. Read the environment, choose
a promising route, overcome a threat, and take something from it that changes how
you play. Use that capability against a more demanding part of the ship. Finish a
generated story with a memorable confrontation, then extract or continue deeper.

The central loop is **discover -> choose a challenge -> earn a capability -> build
around it -> breach the next layer**. Exploration, combat, story, hacking and
equipment should contribute to that same loop.

The current game supplies much of the vocabulary: 20 personal weapons, four
psionic powers, two rigs, ten attributes, nine districts, generated stories, a
shelter and the matrix. Fresh profiles already receive the three starter guns,
beams and flare cannon; rigs and all four powers are immediately usable. Distance
largely controls enemy selection and health. This makes a good testing sandbox,
but provides too little anticipation or structure for earning the strongest toys.

## Power that lasts, and a build that develops

Recommended starting point: retain earned equipment and abilities between ships,
keep permanent numerical gains bounded, and earn powerful combinations during each
expedition. The player has been asked which persistence model they prefer; this
recommendation remains provisional until that discussion is resolved.

| Progression | What persists | How it is earned |
| --- | --- | --- |
| Capability ownership | Weapons, powers, rig chassis, programs and their discovered modifications | First victory over a suitable guardian or elite challenge; selected major missions |
| Character mastery | A finite allocation of meaningful attribute and specialization choices | Experience and completed challenges; freely reassign at the shelter |
| Expedition build | Installed artifacts, exceptional overcharges and local system advantages remain for this ship, including through death and reload | Exploration, elites, guardians and optional preparation |
| Career | Records, faction knowledge, cosmetics, ship discoveries and harder expedition options | Story conclusions and sustained exploration |

Owning a mech means you can bring it on a future ship. It should not require
unlocking the same chassis again. Starting loadout capacity creates a choice:
initially try two personal guns, two powers and one rig, plus basic utility tools.
Storage retains everything else; reconfigure at the shelter. Reward variants and
specialization modules should create tradeoffs rather than equip every passive
bonus simultaneously. These slot counts need testing, particularly for touch.

Keep the flashlight, map, shelter, basic matrix access and a reliable free-firing
weapon available from the start. A fresh character gets a simple combat kit and
earns its first additional capability early. Existing characters keep ownership;
they receive different variants or upgrades when a challenge would repeat an unlock.

Ordinary fights give useful supplies, experience and incremental improvements.
Exceptional victories give exceptional rewards. An illustrative first-clear offer
contains three visible, themed choices: a weapon, an ability or chassis, and a
module suited to an owned item. Choosing one grants it permanently and makes it
usable immediately at the nearby staging point. Unchosen choices stay discoverable
on later ships; duplicate protection guarantees a useful offer or a clearly priced
choice of mastery rewards when the catalog is complete.

Major rewards should be identifiable before committing to a route. A damaged
manifest might report a service walker in the forge; an anomalous signal could
advertise a psionic guardian. The map shows the likely reward family and threat,
while the exact modification can remain a discovery. Avoid random basic drops
that accidentally give away a phase's signature reward.

Keep numerical growth strong enough to feel: former elites eventually become
comfortable targets. Do not automatically raise enemy strength when the player
equips a better gun. Difficulty comes from the chosen expedition tier, layer and
encounter, with modest numerical growth and increasingly demanding combinations.
Experienced characters can select a higher starting challenge with better rewards;
they may still revisit easier ships for a power fantasy.

## Four phases with a finish, inside an endless ship

Start with four substantial phases after the safe loading bay. Target roughly
20–30 minutes per phase, with saving anywhere and optional detours. A complete
commission is roughly 90–120 minutes as an initial pacing hypothesis, not a timer.
Continuing beyond the finale remains optional and produces further expeditions.

| Phase | Spatial and dramatic role | Typical challenge | Reward and consequence |
| --- | --- | --- | --- |
| 0: Threshold | Safe bay, commission, first views and route survey | Learn controls and choose an initial lead | Starting loadout and a readable destination |
| 1: Foothold | Recognizable service and habitation spaces near the hull | Fight a local ruler with one clearly taught mechanic | First major capability, secure a checkpoint, open the next boundary |
| 2: Commitment | Stronger directional identity; more purposeful routes and scale changes | Choose between rival guardians; prepare through exploration or hacking | A major specialization or vehicle, reveal the target's complication |
| 3: Transformation | Extreme combinations of the chosen traits; unfamiliar infrastructure | A controller combines known mechanics with one new demand | A build-defining modification, secure an optional story solution |
| 4: Reckoning | A distinctive destination generated around the commission and history | Confront the entity controlling access to the objective | Final choice, permanent reward, extraction and debrief |

These are roles, not a fixed sector sequence. Phase 2 might cross a frozen luxury
district on one ship and burning botanical machinery on another. Every phase
should still contain quiet discovery, a strong view, optional danger and easier
horde fights where the new build feels powerful.

“Deeper” means more remote from the entry and more entrenched in the ship's
systems; it does not always mean downward. Use predetermined spatial bands with
monotonically increasing remoteness from the bay, then validate playable travel
distance through the macro route graph. Walking circles or loading chunks never
advances a phase. Physical boundaries make progress legible: sealed transit rings,
reactor isolation systems, quarantine walls or controlled lifts.

Each boundary offers alternative guardian approaches and optional side challenges.
Defeating a qualifying guardian restores or captures the boundary's access system;
the player can then take any opened route into the next band. Unchosen guardians
remain optional opportunities. Gate state is shared across doors, lifts and other
crossings, so flight or a random side passage cannot accidentally skip all phases.
Ordinary optional paths can reward flight, small size or hacking, but every required
route remains completable with the starter kit and rewards already guaranteed.

Plan the main route, boundary contracts and arena reservations before filling
individual chunks. Reveal destinations as the player learns about them. Never
move an observed boss or regenerate a visited room because the player chose a new
build or story outcome.

## Direction becomes a way to choose a world

Assign coherent opposing traits to ship-space directions once per seed. Strength
increases smoothly with displacement along the relevant axis. Layer depth raises
the range and extremity of the result; bounded local variation and historical
incidents prevent every neighboring room from being identical. Returning to the
same coordinate restores the same architecture and themes.

The entry is on the hull of a half-space: backward at the bay is space. Preserve
its five initial routes. Use up/down and port/starboard as two fully opposed axes,
with hullward/inward as the third trend. A north/south compass could label an
interior axis, but it must not promise another outward entrance through the hull.
Directions refer to the ship, never to where the player's camera currently faces.

| Example axis or trait pair | One extreme | Other extreme | Gameplay consequences |
| --- | --- | --- | --- |
| Up / down: order / chaos | Symmetric courts, regimented machines, working infrastructure | Ruptured passages, irregular colonies, unstable systems | Coordinated formations and surveillance versus flanks, swarms and shifting hazards |
| Port / starboard: cold / heat | Cryonics, brittle vessels, frost-filled galleries | Foundries, vent towers, thermal baths | Breakable frozen cover and coolant pulses versus vent timing, heat buildup and pressure releases |
| Hullward / inward: human / alien | Familiar workspaces, legible signage and machinery | Unfamiliar proportions, resonant organs and ancient grafts | Increasingly strange navigation landmarks, devices and enemy coordination |
| Alternative pair: mechanical / organic | Factories and clockwork maintenance networks | Living partitions, roots and nested chambers | Robotics and electrical interactions versus growth, regeneration and brood control |
| Alternative pair: gravity / suspension | Heavy foundations, compressed rooms and anchored foes | Hanging gardens, open shafts and floating debris | Grounded positioning versus vertical movement; accessible fallback routes |
| Alternative pair: abundance / scarcity | Crowded stores, working reservoirs and competing inhabitants | Abandoned vaults, stripped machinery and dormant threats | Different reward opportunities, population and preparation choices, without hunger chores |

Select compatible pairs and polarities from a curated pool based on ship identity
and catastrophe. Do not roll all thematic axes independently: incoherent results
should be rejected. Cold is a condition, order an organization, and machinery a
function; layering them is what creates interesting combinations.

Keep **sector function + directional traits + depth + history** distinct. A luxury
hotel remains recognizable as a hotel when frozen, overgrown or regimented. At a
boundary, two identities meet in a transition room rather than abruptly changing
at a chunk seam. Architecture, route shape, sound, lighting, encounter composition,
interactive systems and rewards should all reflect the same traits.

For example: ordered/cold luxury becomes immaculate cryogenic suites occupied by
silent robot attendants; chaotic/cold luxury becomes split pools and ice-grown
ballrooms with creatures inside broken pipes. Ordered/hot machinery becomes a
working furnace procession; chaotic/hot botany becomes a ruptured greenhouse around
a living boiler. At greater depth these grow into enormous coordinated complexes
or completely overtaken architectural landmarks.

Teach direction through windows, signage, ambient sound and two or three early
observations. Survey terminals reveal a coarse directional forecast and possible
rewards. The map should support a choice such as “toward the colder machine decks
for a shield upgrade” without revealing every room or collectible.

## Challenging fights and equipment worth earning

Build guardians from authored combat families with tested combinations: body,
movement pattern, two or three attacks, arena structure, directional modifier and
reward family. Randomize combinations within constraints. Randomly accumulating
armor, regeneration, speed and attacks is likely to produce unfair encounters.

Teach an attack on ordinary foes, complicate it on an elite, then combine it on a
guardian. Target a few minutes of purposeful combat with readable openings. Raise
pressure through positioning, support enemies, destructible parts and changing
space. Keep ordinary enemies quick to kill. Dark scenes still need visible attack
telegraphs, distinct audio and understandable projectile paths.

| Example challenge | How the fight works | Reward identity |
| --- | --- | --- |
| Foundry Marshal | An armored walker alternates sweeping fire with exposed cooling joints; cranes provide cover | Mech chassis or ram, shielding and heavy-weapon variants |
| Convoy Devourer | A fast creature intercepts a stranded cargo train; break its lunge and defend different approaches | Wagon, traction chassis or a later hover/skimmer variant |
| Choir of Glass | Several linked entities reveal the actual attacker through sound and light; interrupts expose it | Flight, veil or psionic combination modules |
| Cryogenic Custodian | Rotating barriers and coolant bursts create temporary firing lanes | Cryo weapons, shield conversion or brittle-target effects |
| Firewall Saint | Physical emitters create readable barriers; optional matrix work disables an extra hazard | Beam transformations or advanced hacking programs |

All required fights must have a direct solo solution. Hacking and preparation make
them safer or create another opening. They should not require a second player, an
unowned power, a mandatory damage type or an ability locked behind that same fight.
A vehicle encounter needs an on-foot route or a clearly marked temporary loaner;
permanent ownership is the reward. Avoid making every confrontation an arena lock:
mix mobile pursuits, holding positions, dismantling machinery and conventional duels.

Examples of expedition combinations to test:

- **Mobile storm:** flight, chain beam and a nova that charges while airborne.
- **Siege engineer:** mech, shield ram and flares that expose vulnerable machinery.
- **Silent hunter:** veil, a precision weapon and a bonus after breaking concealment.
- **Systems saboteur:** matrix recruitment, an electrical weapon and disruption of
  enemy support units.

Each must pay an opportunity cost and have situations that reward another approach.
Avoid universal advantages such as a vehicle that is faster, tougher, more damaging
and equally maneuverable everywhere. A wagon excels on long connected routes, a
mech in open fights, and a character on foot in tunnels and complex vertical spaces.

## Stakes without losing the character

Keep the forgiving checkpoint policy: death retains owned equipment, completed
objectives, confirmed rewards and the expedition build. The current challenge
resets, rather than making the player repeat a completed phase. Do not add permanent
injuries, an obligatory corpse run or a timer punishing exploration.

The shelter remains a real safe place to recover, cook, inspect discoveries and
change a build. Put a dependable staging space near each guardian. Crossing a
clearly signposted challenge boundary starts an attempt; leaving or retreating into
the shelter ends it when nobody remains engaged. An abandoned guardian returns
to its starting state, so repeated invulnerable recovery cannot chip it down.
Ordinary cleared rooms stay cleared. Temporary adds and attempt rewards cannot be
farmed by deliberately abandoning the fight.

In co-op, a partner can hold the encounter while the other recovers. Both eligible
participants earn their own major reward choice exactly once. Shared phase flags
and individual unlock receipts must survive join/leave and save/load. Encounter
scaling uses participating players, with bounded extra pressure; a distant partner
exploring elsewhere must not double a solo guardian's health. Save/load should
never reroll rewards, heal an active boss while retaining inflicted damage benefits,
or duplicate payouts. Exact mid-attempt resume rules belong in the implementation spec.

## Story and replay value

Generate why each guardian obstructs the commission from the same history as the
ship. A rescuer needs the shelter network protected by the Custodian; a salvager
needs the drive under the Marshal's control. Phase victories reveal new information
and open the next part of the existing narrative. The final fight secures access;
the existing moral or practical story choice decides what happens afterward.
Victory need not always kill an intelligent controller: disabling or capturing it
can support the generated conclusion.

Preparation should change a real situation: disable reinforcements in the matrix,
divert coolant, rescue an engineer, or restore a safe route. Offer tempting detours
without turning each phase into three compulsory errands. Keep the current ending
requirements understandable and achievable even when the player ignores loot routes.

Replay value comes from decisions with different consequences:

- Ship identity and directional assignments change where desirable opportunities lie.
- Branching guardians let the player pursue a particular capability or accept a
  riskier reward; revealed choices are stable rather than rerolled by loading.
- Enemy communities use different tactics; optional leaders can change local
  reinforcements or infrastructure when defeated.
- Environmental systems let builds exploit the same room differently.
- Limited loadouts and curated reward synergies create different ways to fight.
- Story dependencies and faction bargains change what “success” means.
- Optional expedition modifiers and post-finale depth support mastery without
  forcing newcomers through an ever-growing statistics wall.

Avoid introducing every feature at once. More rooms, enemies and weapons only help
after a short expedition already makes choosing, fighting and winning satisfying.

## Suggested implementation sequence

1. **One complete phase.** A fresh-character start, a few rooms with two optional
   approaches, one staged guardian, a choice of three useful rewards, a reliable
   retry and a checkpoint. Use existing geometry and content wherever possible.
   Validate a 15–25 minute playable slice before expanding the generator.
2. **Progression and ownership.** Separate capability ownership, loadout and
   expedition modifications; restrict new-profile starting rewards; define mastery
   bounds and the old-profile conversion policy. Exercise a second ship with the
   same character, co-op receipts and repeated rewards.
3. **Directional generation.** Introduce a versioned macro plan, trait assignment,
   coherent transitions and boundary contracts. First prove two pairs visibly
   change architecture and tactics; add further pairs after this works.
4. **Four-phase expedition.** Reserve deeper destinations, add a small guardian
   family library, connect phase consequences to current stories and matrix
   preparation, and give each phase a distinct rhythm and reward.
5. **Replay and balance.** Broaden tested combinations, additional vehicle roles,
   optional challenge tiers and post-finale progression using actual playtest data.

Before implementation, give each agreed milestone an OpenSpec proposal and
acceptance scenarios. New architecture/population rules require a new ship
generator version; phase routing may require a narrative version, and ownership,
loadouts and boss records require a save extension. Preserve existing expeditions
under their original rules. Do not silently revoke old ownership or reinterpret
spent attribute points: design an explicit conversion or retained legacy profile
before enabling a new progression ruleset for those characters.

Validation should retain the project's >=95% unit line coverage overall and per
core file. Add actual-control campaigns through phase transitions, failure/retry,
reward choice, optional routes, co-op and save/load; inspect native and packaged
browser presentation with keyboard, controller bindings and touch. Generate many
seeds to check coherent traits, connected mandatory routes, legal guardians,
starter-kit solvability, stable revealed choices, clearance, encounter budgets and
load-order independence. Keep geometry and simulation bounded for web and split-screen.

Human playtests decide whether the design is fun. Record time to the first useful
reward, time spent traveling versus fighting, guardian attempts, reason for damage
or failure, shelter use, route and reward choices, and whether the second ship
produces a substantially different build. Automated completion and coverage alone
cannot establish difficulty, enjoyment or replay value.
