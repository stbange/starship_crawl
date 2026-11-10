# Proposal

## Why

Starship Crawl has the systems for a memorable expedition, but its rewards,
encounters, generated places and stories need to reinforce one another. Recent
automated reviews show rapid power saturation and short guardian fights; the next
development pass should make growing powerful create new decisions while giving
each ship a recognizable journey, with human playtests deciding the balance.

## What Changes

Build the ten accepted design proposals into one loop: **discover a district,
understand its problem, choose a challenge, earn a distinctive capability, prepare,
overcome its guardian, and return to a visibly changed place**.

- Replace percentage-only specialization as the main build identity with earned,
  mutually exclusive mechanical branches. Tune stat growth and recovery so early,
  middle and late fights retain decisions without removing forgiving retries.
- Compose hordes from complementary roles and give guardians destructible systems,
  readable arena changes and optional physical or matrix preparation.
- Generate district journeys around landmarks, connected approaches, local stories
  and return shortcuts. A dominant ship condition links these places to consistent
  environmental interactions, enemy behavior, rewards and the main motivation.
- Extend existing hidden campaigns into recognizable preparations, sieges and
  retreats; introduce two or three recurring individuals whose circumstances follow
  the same events. Players see actions and consequences, never diplomacy tables.
- Make the safe shelter a place to refit, choose an optional meal, fund expedition
  projects and display recovered objects. Preserve its free recovery and sanctuary.
- Extend the invisible director with persistent anticipation-to-aftermath sequences
  that respond to exploration and pressure without rewriting witnessed events.
- Deliver a focused combat pass, one integrated district, then broader combinations
  across the existing four phases. Preserve the staged opening and optional systems.

This change is a **planning package**. Its requirements describe proposed future
behavior; the accompanying unchecked milestones do not claim it is implemented.

## Capabilities

### New Capabilities

None. Extend the existing gameplay boundaries rather than introduce a parallel
progression, faction, narrative or generation framework.

### Modified Capabilities

- `progression`: transformative, bounded build branches and useful challenge rewards.
- `combat`: deliberately composed, readable encounters across progression bands.
- `phased-expeditions`: staged guardians with arena systems and durable victories.
- `ship-generation`: coherent district journeys and a defining operational condition.
- `reactive-expeditions`: predictable environmental interactions shared by tools and actors.
- `political-expeditions`: sustained causal campaigns and persistent director sequences.
- `narrative`: recurring individuals connected to local events and the main motivation.
- `shelter`: optional preparation, expedition projects and recovered objects.
- `persistence`: versioned expedition identity and durable cross-system consequences.

## Impact

Retain C17, SDL3, OpenGL/WebGL 2 and the existing authoritative fixed-step core.
Extend campaign/progression/combat modules; generation and interior planning;
faction forces/orders and the hidden director; story, shelter, save and snapshot
modules; and their presentation/input adapters. No new engine, runtime service,
online multiplayer or audio-generation dependency is required.

The baseline is generator 15, narrative 3, hidden simulation 1, matrix generator 1
and save format 16. New shipped behavior needs explicit compatible ruleset and
format versions; changes to geometry, narrative contracts or simulation semantics
must version their respective components. Do not assign speculative version
numbers now or reuse a published version for a later incompatible milestone.
Existing expeditions retain their original rules and earned progress. New ships
still create fresh characters. Checkpoint saves and portable live snapshots keep
their distinct guardian-resume semantics. This planning-only commit changes none
of those formats.

Implementation gates include `npm run spec:validate`, native unit CTest, separate
gameplay CTest, `python3 scripts/check-coverage.py` (at least 95% overall and per
executable core source), sanitizer checks, packaged browser/native rendering
checks and the isolated Quest suite. Seed/load-order invariants, long simulated
expeditions and reproducible human playtests will assess coherence, challenge and
replay value separately. The detailed milestone gates are in `design.md` and
`tasks.md`.
