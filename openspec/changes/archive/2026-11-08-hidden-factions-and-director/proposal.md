# Proposal

## Why

The current courts and timed dispatches expose the machinery that should make
the ship feel inhabited. The intended experience is discovering alien wars,
movements and consequences through play, while an unseen director shapes pacing.

## What Changes

- Replace explicit court commissions, political tracking and director dispatch
  prompts in new ships with observable behaviour, physical evidence and ordinary
  exploration/combat interactions. Keep the expedition's story and guardian goals.
- Give the three existing governments hidden needs, strategic sites, relationships
  and doctrines. Let them dispatch forces and change relationships without a
  player accepting a quest.
- Move persistent squads through connected rooms and sectors to patrol, escort,
  raid, reinforce, occupy and retreat. Resolve distant activity coarsely and nearby
  activity through the actual combat simulation without duplicating either.
- Separate alien decision-making from the director. Give the director explicit
  ways to add, change, defer and remove eligible upcoming content, with recovery,
  variety, continuity and shared split-screen limits.
- Make conflict leave persistent changes to occupation, supplies, lighting,
  infestation and environmental evidence; retain readable alien silhouettes and
  intentions without disclosing internal standings or diplomacy.
- Protect earned progression, shelter safety, required routes and completed
  victories. Introduce local warnings and retaliation before enduring hostility
  for incidental friendly fire.
- Deliver six milestones: hidden foundations; mobile forces; autonomous politics;
  invisible direction; environmental consequences/rewards; long-run validation.
- Version new-ship behaviour and saves explicitly. Generator-13 expeditions keep
  their existing courts and dispatches; older supported expeditions remain usable.
  No running ship silently adopts the new simulation.

This change is a plan. The checked-in implementation and shipped specifications
continue to describe generator 13 / save format 14 until the milestones land.

## Capabilities

### New Capabilities

None. Extend the existing political-expeditions capability, which already owns
faction behaviour and the director, and the existing persistence contract.

### Modified Capabilities

- `political-expeditions`: hidden political presentation, autonomous campaigns,
  persistent mobile forces, incidental player influence, and an unseen director
  with causal interventions and observable consequences.
- `persistence`: new versioned simulation state, entity ownership, clocks and
  interventions, plus transactional loading and legacy expedition preservation.

## Impact

Primary integration points are `politics.c`, `politics_gen.c`, `director.c`,
`world.c`/`world.h`, combat in `game.c`, `save.c`, `render_politics.inc`, map/HUD
rendering, input/browser menus and existing political tests. Separate new
strategic simulation and mobile-force ownership from room generation; static
room kill masks cannot own troops that move between rooms. Reuse the current
C17/SDL3/OpenGL/WebGL engine and existing content; no runtime AI service is needed.

Implementation validation includes `npm run spec:validate`, native CTest,
`ctest --test-dir build -L gameplay --output-on-failure`,
`python3 scripts/check-coverage.py` (at least 95% overall and per core source),
sanitizers, packaged browser suites, render smoke, Linux/Windows/web builds and
paired long expeditions. Human pacing/readability and physical-device checks
remain distinct from accelerated simulation evidence.
