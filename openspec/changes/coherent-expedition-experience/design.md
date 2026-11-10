# Coherent expedition experience

Status: approved design direction, **implementation planned**, 10 November 2026.
All implementation tasks start unchecked. See the [proposal](proposal.md),
[behavior contracts](specs/) and [execution checklist](tasks.md).

## Context

The existing game already has four expedition phases, earned tools, regional
ecologies, multi-deck landmarks, generated stories, a safe shelter, autonomous
factions and an invisible director. The [proposal](proposal.md#why) explains the
reason for connecting and deepening them. Current behavior is documented in
[onboarding](../../../ONBOARDING.md), [phases](../../../PHASED_EXPEDITIONS.md),
[the living ship](../../../LIVING_SHIP.md) and
[hidden factions](../../../HIDDEN_FACTIONS.md).

The [7 November review](../../../docs/reviews/2026-11-07-long-expeditions/README.md)
found seven of twelve main automated expeditions maxing all attributes and a
median guardian fight of roughly five seconds. The
[9 November plots](../../../docs/reviews/2026-11-09-expedition-plots/README.md)
recorded four simulated hours with no deaths and main-story extraction between
9:35 and 15:55. These are scripted agents, not human pacing measurements. They
justify testing growth and encounter structure; they do not establish that human
players need harsher punishment or a particular completion time.

The baseline is generator 15 / narrative 3 / hidden simulation 1 / matrix 1 /
save 16. C17, SDL3, OpenGL/WebGL 2, fixed-step simulation and the separate Quest
target remain the platform foundation.

## Goals / Non-Goals

**Goals:** make a run recognizable by what happened, where it happened and the
character the player built. Connect exploration, combat and consequences through
shared world facts. Preserve dark alien atmosphere, generous horde combat,
dramatic changes of scale, readable threats and recoverable mistakes. Establish a
small tested content vocabulary before multiplying its combinations.

**Non-goals:** online multiplayer, a new engine, a diplomacy management screen,
mandatory escort missions, survival upkeep, freeform destruction, general fluid
simulation, live generated dialogue or a larger starting hub. The boarding craft
remains a quiet returnable craft. Music uses restrained instrumental ambience;
new licensed or generated soundtrack assets are a separate production task.

## Decisions

### 1. Organize the experience around district journeys

The repeated player loop is:

**See a destination → discover its problem → choose a challenge → earn a useful
capability → prepare → overcome a guardian → see the consequences → go deeper.**

Ordinary districts use smaller versions of this loop. Phase-ending districts
contain the guardian culmination. Quiet routes, local stories and optional
challenges make the journey flexible; the four existing phase boundaries provide
its larger structure.

| Phase | Player experience | Systems introduced or deepened |
| --- | --- | --- |
| 1. Intrusion | Learn the ship by looking, moving and shooting; follow a landmark; win the first breach | Carbine, flashlight, map; earned shotgun; physical interactions; shelter from the existing elite/guardian milestone |
| 2. Interpretation | Recognize the ship's condition and inhabitants; revisit a changed place; choose useful detours | Protected first matrix operation after phase one; more environmental combinations; powers at the existing tier-two gate |
| 3. Commitment | Build a distinctive fighting style and prepare for competing hazards | Rigs and specialization at the existing tier-three gate; deeper campaign consequences, refitting and advanced routes |
| 4. Consequence | Apply learned rules, confront a prepared finale, choose an informed outcome | A culmination of established mechanics, recurring individuals and the original motivation; extract or keep exploring |

These are experience goals, not new unlock timings. Keep Guided / Minimal / Off,
discovered map goals and the field guide. Required routes never require players
to have rolled a particular gun, power or vehicle. Death retains the existing
checkpoint recovery and earned progress. A new ship always starts new characters.

Alternative considered: distribute ten independent feature expansions across
the game. A district provides a concrete place to verify that their interactions
create understandable choices instead of additional menus.

### 2. Prove the whole loop in one district

The first integrated slice is **the Drowned Foundry**, a workers' district beneath
a suspended reactor, on a ship where restored power wakes dormant machines.
This is a template with seeded occupants, layout, causes and outcomes, not one
fixed story assigned to every seed.

1. From a narrow access tunnel, the player sees the reactor above flooded homes.
   A raised maintenance route and a lower workers' route lead toward it. Steam,
   tiny windows and moving machinery provide near, middle and distant scale cues.
2. Evidence establishes what the region needs. In a rescue commission, an engineer
   is trying to power a refuge. A salvage commission uses the same physical
   dependencies for a different target and understandable stakes.
3. A support creature shelters a swarm while a ceiling hunter threatens an escape
   lane. Removing support creates a clear opportunity to mow down the swarm.
4. The player can restore a pump, interrupt a supply convoy or pursue the direct
   route. Power drains part of the approach but wakes machines; damaging a marked
   conductor can briefly electrify water, with a visible warning and dry escape.
5. An earned challenge offers useful choices. The shelter provides recovery and
   optional preparation. An advanced earned checkpoint can demonstrate a beam
   relayed through a flare; a new character uses the available basic tools.
6. The guardian controls a coolant assembly and a weapons platform. Destroying
   either visibly changes attacks and access. Physical sabotage helps; after its
   unlock, hacking supplies an alternative preparation route. Neither is required.
7. Victory opens a lift and a return shortcut. Where the player directs restored
   power determines the refuge's condition. Known costs are communicated before
   confirmation. Later visits show the result, and an earned shelter keepsake
   recalls it. The engineer's next appearance acknowledges what happened.

E3 supplies the physical journey; E4–E6 add its full cast, preparation and pacing.
E6 is the **complete integrated slice checkpoint** before breadth work. Keep a
fresh-character version and separately labelled earned-build fixtures so testing
advanced toys never disguises a broken unlock journey.

### 3. Share a deterministic plan and durable facts

Extend the existing regional planner with a compact district description: stable
identity, dominant condition, directional expression, landmark, route contracts,
environmental systems, encounter roles, optional preparation, story reservations,
guardian relationship and aftermath alternatives. Initially fit a journey into
one existing 4×4 region plus supported deck connections. Neighboring regions agree
on portals through canonical boundary keys, independent of generation order.

Generate only compatible combinations. For example, conductive water needs a
source, warning surface and safe ground; a recurring individual needs a valid
meeting site; a guardian needs staging and a basic route. Use a deterministic
fallback when a secondary feature does not fit. Keep the ship's randomized
directional identities; the operational condition modifies their expression
instead of replacing them.

Store generated facts separately from changes caused by active simulation. A
landmark location is stable; its power, occupation and damage follow recorded
events. Rendering, map browsing and chunk preloading read this state. They do not
create story progress. Keep the existing small event types and stable actor IDs;
add narrow typed events for the new facts rather than a second global scripting
engine or a second world simulation.

| Existing boundary | Planned responsibility |
| --- | --- |
| `generation.c`, `world.h`, `interior.c`, `habitat.c` | District contracts, compatible geometry and ecological presentation; extract a focused district module if needed |
| `campaign*.c`, `progression.c`, `equipment.c`, `psionics.c` | Branch ownership, loadouts, growth, guardian systems and earned effects |
| `encounters.c`, `alien*.c`, `death.c` | Role compositions, commitments and causal combat effects |
| `faction_sim.c`, `faction_orders.c`, `faction_forces.c` | Campaign stages using existing supplies, orders, travel and casualties |
| `hidden_director.c` | Bounded sequence memory and legal interventions |
| `story.c`, `expedition.c`, `shelter.c` | Recurring people, local decisions, preparation and keepsakes |
| `commands.c`, `session.c`, `presentation.c`, render/input adapters | Shared validated actions; device-specific views and controls |
| `save*.c`, `snapshot*.c`, snapshot schema scripts | Durable facts, validation, migration and distinct checkpoint/live restore |

Alternative considered: a general quest graph or large procedural grammar rewrite.
Small authored templates selected by shared constraints make the first slice
testable and preserve the existing streaming architecture.

### 4. Make power change tactics

Retain two guns, two powers, one rig and one specialization. Tune attribute supply
and marginal growth before adding more stats. An initial **playtest hypothesis**
is roughly 30–40 invested attribute points on a typical main-route completion,
allowing strong specialties while leaving room for optional exploration. This is
not a fixed completion time or a new global cap. Compare earned routes against the
existing 120-point saturation and keep post-finale growth bounded and useful.

Remove the full-resource reset from attribute purchases in the new ruleset.
Specify any level-up recovery explicitly and test it separately from capacity
growth. Preserve ordinary supplies, recovery, shelter safety and forgiving death.
Tune response windows and composition before increasing enemy health.

The initial branch prototypes are **beam relay through planted flares**, **a decoy
left when cloak breaks**, and **a mech cannon charged by absorbed attacks**.
Their prerequisite tools must be earned first. E7 expands to four specialization
families with two mutually exclusive branches each: conduit, veil, bastion and
rift. The fourth family develops existing flight/long-jump/area effects rather
than adding another ability menu.

| Family | Branch A | Branch B | Choice created |
| --- | --- | --- | --- |
| Conduit | Relay a beam through attached flares to a bounded set of targets | Maintain beam contact to charge a piercing pulse | Prepare a firing network or commit to a firing lane |
| Veil | Leave a short-lived decoy when cloak breaks | Mark the first target hit from cloak; its defeat releases a bounded burst | Redirect pursuit or commit to a priority kill |
| Bastion | Absorbed hostile attacks charge the next mech cannon shot | Brace a stationary mech for frontal protection and a short-range counterwave | Remain mobile while charging or hold a position |
| Rift | A committed long-jump landing releases an area pulse | Flight sustains a close-range field at increased energy cost | Time a landing or manage airtime and energy |

Each branch has a measurable benefit, an
opportunity cost and bounded targets, duration, charges or energy use. Relays
cannot recursively trigger themselves; decoys cannot produce loot or kill credit;
mech charge cannot be farmed from sanctuary effects or harmless friendly damage.

Three initial encounter recipes use current creatures: shielded swarm plus
ceiling pursuit; ranged suppressor plus flanking rushers; and machine support plus
slow area denial. Each has recognizable priority and movement responses. Preserve
horde payoff after the specialist falls. Warnings need silhouette, motion or audio
as well as color, and must survive fog, particle effects and the flashlight.

Guardian patterns begin with a suspended machine and an infested relay organism.
Each has at least two actionable systems and two attack stages. Strong builds can
accelerate or skip an avoidable stage by destroying its source. Use telegraphed
openings and spatial problems, without hidden damage caps or long invulnerable
waiting periods. Preparation changes a declared system; victory physically opens
the next route. Attempt-local destruction resets on retry; earlier preparation
and completed victories persist.

Alternative considered: scale all enemy health to the player's current damage.
Fixed, declared encounter rules preserve the reward of a strong build and let
players understand why their tactics work.

### 5. Keep inhabitants, personal stories and pacing causal

Extend two campaign patterns first: **supply → siege → occupation/retreat** and
**repair → factory production → reinforcement/evacuation**. Existing force IDs,
resources, routes and hostility rules remain authoritative. A diverted convoy
changes a later order or available strength; the player can see that consequence
without a strategic map. NPC-only victories still grant no player experience,
exclusive equipment or guardian completion.

Choose two or three recurring individuals from an initial six-role library.
Engineer, archivist and territorial caretaker are starting examples. Give each
a recognizable name/visual marker, a need, knowledge and a fate. Use existing
short interactions, environmental evidence and transmissions. Do not require
escorting them. Migration follows valid locations and events; use traces or a
message when physical travel is not simulated. Missed or dead individuals have
authored alternative sources for essential information. Optional losses can
matter without making the main story impossible.

The director sequences existing opportunities: **trace → approach → pressure →
confrontation → quiet aftermath**. Four initial templates cover a stalking hunter,
an approaching siege, awakening machines and a recovery/discovery trail. These
are conditional sequences, not compulsory scripts. If the player kills the
hunter early, the later beat becomes its aftermath. If they leave, release
unobserved reservations and retain established evidence.

Start with at most two active sequences across the expedition, sharing the
existing pressure budget. Both players' views and combat commitments protect
observed content. Sequence time follows the existing active simulation and
shelter/pause rules. Ambient sound, light and quiet support causality; music can
follow intensity without revealing the hidden state machine.

Alternative considered: place events around the player regardless of faction or
story state. Reusing real opportunities creates continuity and allows bypasses,
surprises and early victories to count.

### 6. Make preparation useful without chores

Keep the furnished safe shelter and its free bed, medicine and stocked stew.
Add three optional meal choices (electrical resistance, cold resistance, energy
recovery), one active meal benefit per character, with displayed duration and
cost. Optional meals use existing salvage; no hunger meter, ingredient inventory
or sanctuary upkeep is introduced. Preserve the existing basic stew behavior.

Add three bounded expedition projects: a refit bench for redistributing spent
attribute points safely, flare calibration for one limited prepared charge, and
condition research that explains an observed hazard's timings/counters. Projects
never grant a locked tool or exclusive challenge reward. Define co-op ownership
and charging once: shared facility unlocks, personal consumable preparation and
personal respec. Safe refit retains spent-point totals and validates resource
capacities. Useful consumable preparation provides a late salvage outlet after
permanent project caps. Failed purchases spend nothing.

Display up to six keepsakes in fixed safe positions, selected from persistent
discovery records. Older discoveries remain in the journal when display space is
full. No arbitrary construction editor or larger interior is needed. Returning
home should take a short intentional stop, not repeated station maintenance.

Alternative considered: ingredient collection and expanded crafting trees.
Existing salvage and fixed facilities supply decisions without competing with
exploration and combat for attention.

### 7. Expand combinations after the slice works

The planned first release has a deliberate content ceiling:

| Content | Initial slice | End of E7 |
| --- | --- | --- |
| Dominant conditions | Restoring power wakes machines | Three: awakening machinery; predictable freeze/vent cycles; infestation spreading along active conduits |
| District journey families | Drowned Foundry | Four: foundry, frozen luxury terrace, command reliquary, infested transit garden |
| Encounter recipes | Three | Six, using the existing roster and ecological variants |
| Guardian patterns | Two | Four, mapped to the existing four phases and alternative guardian sites |
| Transformative branches | Three prerequisite-gated prototypes | Four families × two alternatives |
| Recurring cast | Two roles in the slice | Six authored roles, selecting two or three per expedition |
| Campaigns / director sequences | Two / four | Reuse these with condition, route and outcome variation |
| Shelter additions | Three meals, three projects, fixed keepsake display | Same scope, tuned for late usefulness |

All four existing motivations receive compatible district and cast bindings.
Support a tested compatibility matrix, not every Cartesian combination: cold
cycles need appropriate vents, for example. Every condition must occur in more
than one district family, and every family must support more than one condition.
Required counterplay is available physically. Powers, rigs and matrix routes
offer advantages when earned. Endless post-finale exploration reuses completed
story facts and new district variants without respawning unique dead characters
or duplicating the main conclusion.

### 8. Deliver in eight reviewable milestones

Each milestone includes its persistence, presentation, tests and documentation.
Do not postpone these to E8. Each can ship for new expeditions once its gates pass;
partial delivery must say which content is present.

| Milestone | Depends on | Implementation and playable result | Exit gate |
| --- | --- | --- | --- |
| **E1 — Combat and build identity** | Current main | Version new rules; tune growth/recovery; three branch prototypes and three encounter recipes using existing geometry | Starter and earned builds complete matched fixtures; branches produce distinct actions; no duplicate rewards, resource overflow or locked-tool access; first-phase onboarding still works |
| **E2 — Memorable guardians** | E1 | Two system-based guardian patterns, physical preparation, optional earned matrix preparation, staged arenas and visible breach | Prepared/unprepared solo and co-op victories, retreat/death/retry, checkpoint reset and exact live-snapshot continuation; two patterns readable in rendered play |
| **E3 — A coherent district** | E2 | Shared district plan; Drowned Foundry; awakening-machine condition; conductivity and machinery rules; landmark, routes and shortcut | Deterministic/load-order-safe generation; fresh-character basic route and earned-tool alternatives; persistent physical aftermath; vista and warning captures |
| **E4 — Inhabitants with a continuing story** | E3 | Two campaign patterns, first two recurring roles, local choice and main-motivation binding | Intervene/ignore/bypass/lose-an-individual paths remain coherent; consequences survive distant travel; no exposed political UI or NPC-granted progression |
| **E5 — A useful home** | E4 | Three optional meals, three bounded projects, safe respec and keepsakes reflecting district outcomes | No-cost recovery works; personal/shared purchases charge once; no buff stacking or sanctuary leaks; returns and co-op saves are safe |
| **E6 — An expedition rhythm** | E5 | Four conditional director sequences, observation-aware reservations, sensory anticipation and quiet aftermath | Sequence interruption, early victory, bypass and split-party tests; paired director-on/off runs; complete integrated slice review |
| **E7 — Ships worth replaying** | E6 | Expand to the stated content ceiling; all four motivations and phases; seeded directional variants and post-finale continuation | Compatibility matrix covers every condition/family and both basic/advanced routes; matched seeds produce different legal builds and decisions without broken combinations |
| **E8 — Balance and release evidence** | E7 | Sustained simulations, human sessions, performance/readability tuning and platform release checks | Reproducible report, required automated gates green, measured budgets, human/device gaps explicitly recorded |

The default execution order is E1 → E8. Build the physical foundation before
adding narrative or pacing dependencies. No milestone requires waiting for a
content generator, external service or new asset pipeline.

| Accepted proposal | Delivery |
| --- | --- |
| 1. Divergent builds | E1, E7 |
| 2. Deliberate horde compositions | E1, E7 |
| 3. Transforming guardian arenas | E2, E7 |
| 4. District journeys | E3, E7 |
| 5. Consistent environmental rules | E3, E7 |
| 6. Sustained hidden campaigns | E4 |
| 7. Recurring individuals | E4, E7 |
| 8. Shelter preparation | E5 |
| 9. Longer director sequences | E6 |
| 10. Defining ship complications | E3, E7 |

### 9. Validate behavior and enjoyment separately

Keep the commands and platform procedures in [TESTING.md](../../../TESTING.md)
authoritative. For each affected milestone run strict OpenSpec validation, unit
CTest, separate gameplay-labelled campaigns, sanitizers and unit-only coverage
of at least **95% overall and per executable core source**, with no exclusions.
Add new modules to the actual core inventory. Exercise typed gameplay commands;
test-only fixtures must not introduce production behavior switches.

Automated acceptance includes deterministic native/WASM plans, reciprocal portals,
starter-route reachability, valid attack warnings, budget compliance, attributed
claims, transactional malformed saves, active snapshots, old-format fixtures,
solo/co-op joins, director cancellation and observation continuity. Test all
three reward indices and refusal/defer paths, not only the first choice. For
movement and presentation changes, run native GL smoke and packaged browser
checks with keyboard, controller bindings and touch. Run isolated Quest build,
packaging and simulated WebXR checks; physical Quest remains a separate check.

E8 retains at least **twelve 60-minute simulated expeditions**: cover all three
conditions, four motivations, both ends of each directional axis, several build
families and a varied reward policy. Keep paired control seeds for growth and
director comparisons; include a starter-only required-route run, an optional
challenge route, a save/resume run and separated co-op exploration. Log seed,
versions, simulated duration, actual wall time, settings, choices, event summary,
resource/build curves, guardian phases, deaths, navigation stalls, campaign causes
and screenshots. Never describe accelerated simulations as hours of human play.

Human review uses at least one new player with guidance and one experienced player
without it, each trying two different ships, plus co-op and physical device checks
when available. Assess these questions from observed actions and brief interviews:

- Can the player identify the landmark and describe the district's problem?
- Can they explain one enemy warning and use the learned environmental rule again?
- Does the chosen upgrade change a later decision, rather than only a damage number?
- Can they connect a returning individual or occupation change to an earlier event?
- Does a guardian victory feel earned while a failed attempt invites another try?
- Does the second ship suggest a different route, preparation or build?

Record time to useful rewards, travel/combat/quiet proportions, specialist response,
guardian stage duration, recovery use and optional-route decisions. Target values
are tuning hypotheses until human evidence exists. A green suite establishes
correctness and reproducibility; it does not establish fun or headset comfort.

## Risks / Trade-offs

- **Too many interacting systems** → enforce the content ceiling and E6 slice
  checkpoint. Simplify a branch or sequence that cannot explain its consequence
  before adding more variants.
- **Power fantasy becomes tedious attrition** → tune composition and commitment
  windows first; keep weak hordes, strong earned combinations and soft retries.
  Avoid compulsory repeated farming or hidden enemy scaling.
- **Procedural logic creates inaccessible scenes** → reserve required connections
  first, validate compatibility and use deterministic fallbacks; keep physical
  solutions to all required interactions.
- **Darkness conceals important information** → combine localized light, silhouette,
  motion and audio; inspect entry viewpoints and target devices, including VR.
- **Director erases player agency** → treat observations, kills, preparations and
  outcomes as facts; permit early resolution and choosing no intervention.
- **Streaming or memory costs grow** → retain two chunk generations per fixed tick,
  two GPU uploads per frame, 20 actors per room, six chunk lights and existing
  dressing/sprite caps. Preserve the current 96 chunks per configured party slot
  (192 for the default two-slot build). Bound active systems and director sequences;
  profile frame percentiles, peak memory and save growth against the baseline.
- **Endless play conflicts with durable history** → compact facts into the existing
  retained record model, discard transient logs, and measure bytes added per hour.
  Never reclaim memory by forgetting claimed rewards, deaths or observed changes.
- **Platform drift** → keep authority in the core, commands shared and local UI
  outside authoritative state; keep `SC_WEBXR`, storage and packaging isolated.

## Migration Plan

1. At E1, add an explicit expedition gameplay revision if existing component
   versions cannot safely distinguish progression semantics. Use typed fields
   and validated readers/writers, not a loose feature mask accepted unchecked.
   Assign concrete format/version numbers only against the then-current main.
2. Preserve all supported older readers, generator fingerprints and behavior paths,
   including generator 13 courts, generator 14 hidden systems and generator 15
   onboarding. Do not retrofit their rooms or rebalance their earned characters.
3. Every milestone that changes published generation or state semantics allocates
   a compatible new revision as needed. Update checkpoint and live-snapshot
   schemas together; preserve their different handling of active guardian fights.
4. Expose the new rules only when creating a new ship. Existing save/resume and
   browser import/export retain the current expedition and local device settings.
   New-ship confirmation continues to explain the fresh-character reset.
5. Ship reviewable increments on main after their tests. Full CI and Pages remain
   required for every push. Report implemented content separately from this plan.
   On a faulty release, stop selecting the new revision for new ships and repair
   it while retaining its reader; do not silently downgrade or overwrite newer
   saves with an older binary. Preserve exportable saves before any rollback.

## Open Questions

These are tuning questions, not prerequisites for starting E1: exact point supply
and level-up recovery; attack timings and guardian stage lengths; meal prices and
durations; how much ambient audio best supports anticipation; and numerical
strengths and costs for the eight specified branches. Resolve them with
the specified fixtures and human sessions within the fixed behavior contracts.
