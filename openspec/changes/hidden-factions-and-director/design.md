# Design

## Context

See [proposal.md](proposal.md) for motivation and scope. The replacement is now
implemented as generator 14 / simulation 1 / save 15. The decisions below record
the original design; the [implementation review](../../../docs/reviews/2026-11-08-hidden-simulation/README.md)
records actual limits, tuning, compatibility and verification evidence.

The preceding `politics_gen.c` assigned three powers and reserved audience/conduit/
finale rooms. `politics.c` advances their disputes through player commissions;
its patrol combat and movement remain local to a room. `director.c` selects four
timed, visible dispatches, and `render_politics.inc`, `render.c`, `main.c` and the
browser menus expose them and the political journal. Save format 14 persists
these small state machines.

`Room.enemies[SC_MAX_ENEMIES]` currently holds up to 20 actors, and room records
use local kill masks. Moving an army by copying those actors between rooms would
break identity, deaths and rewards. World streaming already shares a bounded
192-chunk cache across two players. Those constraints determine the first major
engine change; adding more political dialogue would not address it.

The latest long-run review also found frequent incidental patrol offenses and
dispatch repetition. It identified broader power-growth and reward-balance
issues. This change addresses faction hostility, encounter variety and reward
attribution; general character-growth tuning remains separate.

## Goals / Non-Goals

**Goals:**

- A player can watch, follow, avoid, interrupt and later revisit alien activity.
- Government and resource differences produce distinct military behaviour.
- The director chooses when to intervene and when to allow quiet; its decisions
  have traceable causes for developers and no explicit player interface.
- Strategic simulation, local combat, generation and saves agree about what
  exists, what died and what changed, including separated split-screen players.

**Non-Goals:**

- An exhaustive real-time simulation of an infinite ship or thousands of rendered
  soldiers. Armies are persistent formations with bounded nearby squads.
- A diplomacy screen, court quest tree, player kingdom management, or automatic
  explanations of wars. Physical headquarters, rulers and discovered evidence
  remain useful content.
- New external services, runtime language models, online multiplayer, or a
  wholesale replacement of the existing story/guardian progression.
- Making every faction aggressive or increasing enemy health to counter a strong
  build. Depth and phase remain the source of base difficulty.

## Decisions

### 1. Separate world causes, pacing decisions and player evidence

Use three responsibilities with explicit event boundaries:

| Responsibility | Owns | Feeds into |
| --- | --- | --- |
| Faction simulation | Resources, doctrine, relationships, orders, forces, occupation | Candidate activities and durable world events |
| Director | Recent experience, pacing state, intervention reservations and budgets | Eligible adjustments to upcoming activities/content |
| Presentation and gameplay | Physical actors, combat, traces, environmental changes | Observations, casualties and player-caused events |

The faction simulation must still produce wars and movement with the director
disabled in a test harness. The director can work with ambient infestation,
hazards and discoveries when no faction opportunity fits. Neither owns the
expedition story scheduler or may move an already revealed story destination.

Retain current code paths for legacy generators. Put strategic state/decisions
and force lifecycle in focused core modules, likely `faction_sim.c` and
`faction_forces.c`, rather than further enlarging `game.c`. Leave exact file
boundaries to implementation as long as ownership and tests remain clear.

Alternative rejected: merely hiding the current menus. That would still leave
player-triggered disputes, stationary patrols and a four-offer director.

### 2. Start with three powers and distinguish their behaviour

Reuse Divided Crown, Molting Court and Parliament of Echoes. Give each a seeded
appetite for power, biomass and fabrication material; strategic sites provide
those resources. Habitat, ship history, depth and directional traits bias where
they prosper and which existing creatures/machines serve them. Preserve neutral
areas and infestation that does not belong to a state.

| Government | Strategic behaviour | Observable encounter possibilities |
| --- | --- | --- |
| Divided Crown | Competing command bodies, rival priorities, temporary succession disputes | A column splits; royal guards defend one body while rivals seize supply |
| Molting Court | Protect a vulnerable ruler and accumulate biomass for a cycle | Escort processions, shell barricades, aggressive raids before a molt |
| Parliament of Echoes | Coordinate through relays and inherited orders | Synchronized assaults, relay escorts, confused or withdrawn squads after a relay falls |

Use a small authored action library scored against current needs, reachable
objectives and costs. Decisions consume real resources; recruitment has capacity
and cooldown limits. Use minimum commitment and relationship cooldowns so wars
do not oscillate every tick. Truces, tribute, symbiosis, losses and succession
alter opportunities and doctrine rather than opening a player dialogue.

Do not force a winner or secretly restore a defeated faction for variety. Loss
of influence, a depleted war and empty frontier regions are valid outcomes;
unexplored districts and infestation still provide alternatives.

Alternative rejected: implement many new governments before movement works.
Three visibly different behaviours provide a better first test than more names.

### 3. Represent armies independently from streamed rooms

Give each force and surviving member stable IDs, a home faction, order, objective,
route progress, roster, supply and loss records. Keep static generated enemies on
their existing room-record path. A mobile actor references a force member; room
membership is a temporary location, not its persistence or reward identity.

Use a regional portal graph from existing generation contracts for strategic
routes and local collision-aware waypoints for nearby movement. Include stairs,
ramps and valid lifts with formation clearance/reservations; phase barriers,
closed connections and occupied landings constrain routes. Failure to route
causes waiting, a new route or withdrawal, never a wall-crossing teleport.

Forces begin as patrol, raid, convoy/escort, reinforcement, occupation and retreat
orders. Large armies contain several squads arriving in waves. Share the
existing per-room actor limit; reserve space for a passing formation and defer
excess arrivals. Do not evict a visible enemy to fit another one.

Use coarse travel/combat outside the physical simulation area. Within the union
of both players' interaction areas, one materialization transition hands combat
to actual actors. An observed, engaged or projectile-targeted actor stays physical
until it can leave safely. Transfer surviving roster/health/supply back once,
and suspend abstract resolution throughout that interval. Resolve collisions,
death effects, neutral allegiance and damage attribution through the same combat
rules used by ordinary enemies. Abstract casualties must not detonate an effect
beside a player when a region loads.

Far-off wars use bounded coarse resolutions consistent with strength, resources
and terrain; they need not reproduce every hypothetical bullet. Given the same
seed, player inputs and active-game timeline, results must be independent of
rendering, map queries and cache arrival order. Entering a fight legitimately
changes it by switching to the physical encounter.

Alternative rejected: respawn a generic squad whenever the player crosses a
border. It cannot support following an army or preserving casualties.

### 4. Limit simulation work without pretending to simulate infinity

Advance strategy on a fixed active-game cadence, initially two seconds, with
deterministic ordering and dedicated random state. Maintain an interest area
around both players plus a margin, and a bounded queue for retained campaigns
travelling outside it. Instantiate new strategic regions from the seed on entry
to that area, not on a renderer/map request. Do not retroactively invent an entire
war history for every unexplored sector.

Set and test hard limits on active forces, tactical actors, strategic work per
update and recent event history. Initial profiling candidates are 32 strategic
forces and 128 recent diagnostic events shared by both players, with physical
actors inside current room/cache caps. These are tuning candidates, not measured
capacity claims. When a limit is reached, defer optional work fairly; never drop
committed casualties or silently delete a force.

Keep sparse durable region summaries and claims after cache eviction. Compact
old events into current occupation/resource facts; prune only replaceable
diagnostic history. Save growth can follow explored changes even though live
simulation work stays capped. Measure both, including allocation-failure paths.

Time policy is deliberate: menus that pause play, suspended tabs and all-player
shelter stop strategic time. If any physical body remains outside shelter,
including an exposed matrix diver, normal world simulation continues once.
The director stops creating physical encounters while nobody explores physically.
There is no wall-clock catch-up while the app is closed. A safe pause must not
return the player to hours of unobserved destruction.

Alternative rejected: one global simulation step for every generated region,
or independent simulations per viewport. Both scale poorly and duplicate facts.

### 5. Give the director explicit, bounded editing powers

Track actual nearby threats, health/shield/armor pressure, energy availability,
recent combat duration, travel without discoveries, repeated encounter families,
deaths, objectives and observable player engagement/avoidance. Treat the two
players' local pressure separately while sharing reservations and a global cap.
Do not direct pressure at a struggling partner just because the other is strong.

Choose a pacing state (build-up, pressure, recovery, discovery) before selecting
content. Use hysteresis, minimum recovery windows, cooldowns and multi-event
memory. Consider an empty decision valid. Do not target a death quota or turn
every quiet period into an attack. Adaptive composition/timing must not change
health, damage or loot already committed to an observed enemy.

| Action | Initial examples | Preconditions |
| --- | --- | --- |
| Add | A resource-funded scouting patrol; distant battle audio from an actual conflict; an abandoned supply stash; an upcoming optional nest | Valid cause/location, budget, reserved identities and neither player's observation |
| Change | Reroute an unengaged patrol; swap an unseen encounter's composition; change an upcoming room's lighting/fog; delay reinforcements | Valid route/ecology, no committed combat or previously established static geometry change |
| Remove/defer | Cancel a proposed ambush; omit an unseen optional nest; postpone steam or a blackout; withdraw a squad through a real exit | No deletion in view, no erased victory/claim, no suppression of required progression |
| Leave alone | Let factions fight, allow a quiet walk, retain an earned safe route | Used whenever alternatives are repetitive, unsafe or unnecessary |

Represent each intervention with prerequisites, source/cause, region, cost,
reservation, proposed/committed/completed/cancelled state and expiry. Commit world
effects and budget together. Cancellation releases reservations exactly once;
it cannot become free salvage or resurrect units. Expiry is internal and never
shown as a dispatch countdown.

Union both cameras' observation, nearby combat/projectiles and remembered facts.
Moving troops and changing powered lights in known rooms are possible through
actual world events. Static topology, known objectives, confirmed deaths and
claimed rewards are protected. Candidate placement revalidates if either player
approaches before commit. If performance or validity checks fail, skip the
intervention and retain the coherent base world.

Alternative rejected: unrestricted spawning/despawning around the camera. It
undermines both the simulation and player trust. A purely observational director
is also insufficient; the intervention library must demonstrably alter play.

### 6. Communicate through encounters and preserve agency

Remove new-system audiences, commissions, territory-coloured maps, court tracking,
numeric standings, cyan dispatch seals, expiry hints and system announcements
from keyboard/controller/touch flows. Keep the ordinary mission map and story
journal. An optional discovered note can describe a witnessed event but cannot
reveal the whole hidden strategy. Developer diagnostics stay outside normal UI.

Reuse banners, headquarters, ruler projections and alien forms as physical
evidence. Add readable travel posture, weapon preparation, retreat signals and
directional footsteps/communications; allegiance must not depend on colour alone.
Neutrality must remain understandable in darkness. Damaged supplies, corpses,
discarded shells, new guards, repairs and infestation show changing occupation.
Keep corpses and sprite detail bounded while retaining their outcome in saves.

Replace the permanent first-hit offense with local incident memory: warning,
self-defence, witnessed/reported escalation and eventual disengagement. Repeated
deliberate attacks remain consequential. Distinguish per-player offenses from
faction-to-faction relationships and existing matrix-recruited allies. Killing
witnesses cannot erase already transmitted reports. De-escalation uses time and
behaviour, with no restitution menu required.

Players can observe, slip through, interfere, scavenge or fight. Killing a supply
escort, restoring power or hacking a relevant relay emits a concrete world event;
it need not assign the player a political quest. Preserve ordinary weapon routes,
the shelter and extraction. Protect phase guardians and their approach sites
from autonomous resolution, including battles in unloaded space.

No experience or kill credit for NPC-only casualties. Ordinary battlefield
salvage is finite and accounted for. Rare weapons, rigs and psionics continue to
require an attributable challenge with persistent co-op participation and
once-only claims. Simply delivering a final hit to an almost-dead patrol must
not grant a guardian-tier unlock; participation and reward tier need focused
tests. New resource sinks and the overall stat curve remain separate balance work.

### 7. Build six playable milestones

Dependencies follow M1 → M2 → M3 → M4 → M5 → M6. Each milestone includes relevant
unit, gameplay, persistence and presentation checks; M6 expands the evidence and
tuning rather than postponing all testing until the end.

| Milestone | Deliverable | Acceptance experience |
| --- | --- | --- |
| M1 — Hidden foundations | Versioned simulation state, strategic sites, shared clock, hidden new-system UI, observable local rival patrols and private diagnostics | A new ship shows recognisable groups fighting/tolerating each other without a commission or dispatch; old courts remain playable |
| M2 — Mobile forces | Persistent squads, connected routes, formation orders, faction combat and coarse/physical handoff | Follow a raid across rooms, a deck connection and a sector, interrupt it, leave and return without duplicated or revived soldiers |
| M3 — Autonomous politics | Resource needs, supply convoys, recruitment, doctrine, wars/truces/succession, capture and graded hostility | Ignore a conflict and it develops; destroy a convoy and subsequent deployments change; one stray hit does not condemn the player forever |
| M4 — Invisible director | Pressure memory, pacing states and tested add/change/remove/defer actions | A hard encounter can be followed by recovery, a quiet stretch by a plausible discovery or conflict, with no visible scheduling machinery |
| M5 — Consequences and rewards | Persistent battlefield evidence, occupied/abandoned sites, light/infestation overlays, sound/body cues and story/hacking/reward integration | Revisit a power station and understand that a battle changed it; exploit the result while preserving earned progression |
| M6 — Validation and tuning | Long expeditions, paired director comparisons, worst-case streaming/co-op/save checks, browser/device evidence and final tuning | Different routes/seeds produce different, coherent encounters within measured performance limits |

The first strong gameplay checkpoint is M2: a real expedition can contain an
unprompted moving battle. M3 gives it independent political causes; M4 then
adjusts its timing and context without taking over faction ownership.

## Risks / Trade-offs

- **Contradictory simulation and director ownership** → One force owner, explicit
  reservation/commit events and tests with the director disabled.
- **Armies stall in narrow or vertical routes** → Reuse portal contracts, test
  clearance, reserve landings and permit deliberate waiting/retreat.
- **Hidden rules become unreadable** → Repeated visual/audio evidence, neutral
  warnings and human observation tests without revealing political statistics.
- **Constant war crowds out exploration** → Capacity/recruitment costs, local
  pressure caps, recovery time and a valid no-intervention result.
- **One force is counted in distant and local combat** → Exclusive simulation
  ownership, stable identities and transition/save corruption tests.
- **Long-run snowballing, empty worlds or reward farming** → Track resources,
  population, territory turnover, casualties and claims; accept local defeat,
  use fresh frontier content, and keep NPC victories out of player progression.
- **Mobile/web or split-screen overload** → Shared caps, incremental updates and
  profiling on actual builds; reduce detail/work before increasing budgets.
- **Compatibility work consumes the feature** → Keep old paths and fixtures,
  enable the new system on new ships, and version each changed serialized layout.

## Migration Plan

1. Synchronize the approved delta specifications after implementation, retaining
   the earlier court requirements in their archived change and legacy fixtures.
2. M1 reserves the next free generator/save versions (currently 14/15) and
   introduces simulation-version validation. Subsequent releases bump versions
   whenever serialized meaning or generated layouts change; never reinterpret a
   previously shipped payload to avoid a migration.
3. Preserve format-14/generator-13 courts, controls, dispatches, fixtures and
   settlements. Maintain existing supported migrations, including the already
   documented format-1/2 fresh-world migration. New ships start fresh characters.
4. Add new-state saves, mid-travel/mid-combat handoff, event cancellation,
   occupation and split-screen fixtures as milestones land. Validate all
   identities, references and bounds before replacing a running expedition.
5. Reconcile the older default-generator wording in the persistence specification
   through this change's complete modified requirement; do not change historical
   fixture expectations. Mark each milestone's actual shipped versions in docs.
6. Retain previous builds and saves for rollback. An older binary must reject a
   newer unsupported save, never overwrite it with a partial import. Downgrade
   uses a pre-upgrade save backup; no silent conversion is proposed.

## Validation and release evidence

Run the existing strict specifications, native unit/gameplay tests, >=95% overall
and per-file core coverage, sanitizer checks, packaged browser suites, native
render smoke and portable builds for implementation milestones as applicable.
Meaningful new tests exercise strategic decisions, identity conservation,
streaming independence, both-player observation, resource/reward conservation,
failure rollback and protected progression, not just private helper functions.

For M6, run twelve fresh-character expeditions of 60 simulated minutes across
inward, horizontal, vertical and mixed routes, with several build/input profiles.
Repeat four with director stepping disabled in the test harness for paired
comparisons; keep the faction simulation and production gameplay unchanged.
Retain actual failed routes, seeds, input profiles, timings, saves and screenshots.
Use normal movement/fire/use for journeys, and label inspection fixtures as such.

Record faction dispatch/arrival/capture/retreat counts, resource and casualty
balances, intervention causes/rejections, time under pressure, recovery windows,
content repetition, progression time, stuck routes, update-time percentiles,
active actor counts and save growth. Hard failures include duplicate identities,
resurrected casualties, visible arbitrary removal, lost objectives, duplicate
claims or a resource/work cap violation. Balance targets are hypotheses until
human tests establish whether the encounters feel intriguing and fair.

Capture actual rendered convoys, rival battles, retreat, a changed site and quiet
recovery. Human solo/split-screen readability, physical Windows/mobile graphics
and controllers remain separately reported checks. Push validated work to main
through the existing full CI and Pages workflow, without skipping documentation
commits or claiming a deployment before its job succeeds.

## Open Questions

Only tuning choices remain: the initial force/work caps, government resource
weights, doctrine cooldowns, warning grace periods, director pressure thresholds
and environmental evidence density. Measure these during the named milestones;
none requires exposing the hidden simulation or changing the ownership model.
