## Why

The gameplay reviews found uniform weather at directional extremes, similar enemy
decisions, and few reasons to use earned tools or leave the main route. P1 adds
playable regional encounters while preserving fast horde combat and forgiving
progression.

## What Changes

- Vary extreme hot/cold environments by room function and incident, with several
  compatible ecologies rather than one climate forcing one population.
- Add backwards pilgrims, pendulum gazers and surface-gripping seam-runners with
  readable awareness, approach, commitment and recovery states.
- Add optional nest/relay encounters with visible support links, staggered
  commitments, interruptible support and useful ship-local equipment rewards.
- Provide safe earned-tool drills, quiet/trace/fight/staging room pacing, and
  small local-story actions that visibly change lights, weather or machines.
- Add deterministic, combat, save/co-op, real-input browser and gameplay checks.

## Capabilities

### Added Capabilities
- `regional-encounters`: P1 environmental variety, alien behavior, optional
  coordinated challenges, rewards, local consequences and pacing.

## Impact

New expeditions use generator 11. Generator 3–10 plans, geometry and population
remain available; immutable generator-10 fixtures are captured before changes.
Save format 12 appends local encounter/action records and loads earlier saves
transactionally. No character reset on load; new ships still start fresh.
Native tests, separate gameplay, >=95% unit coverage overall and per core source,
sanitizers, packaged browser suites and supported-target builds apply.
