## Why

Players need sustained beam fire, deployable heavy combat and fast transport, and persistent controllable lighting for gloomy interiors.

## What Changes

- Add three personal beam weapons and a flare cannon, available to new and existing characters.
- Give each player a deployable mech and wagon, with parked models, mounting, safe exit and packing.
- Supply a mech cockpit HUD, heavy persistent plating and three built-in weapons. The smaller wagon accelerates and travels faster using personal weapons.
- Attach fired flares to solid surfaces, with capped persistent light instances and lifetime/strength upgrades.
- Preserve existing profiles and expeditions through save format 6, with generator 3 and existing random loot identities retained.

## Capabilities

### New Capabilities
- `equipment`: Fold rigs, beam fire and surface-attached lighting.

### Modified Capabilities
- `combat`: Beam and utility weapons extend the existing projectile arsenal.
- `persistence`: Compatible format-6 equipment and flare state.

## Impact

Core, rendering, desktop/controller/touch controls, saves, documentation, tests and CI. No new runtime dependency. Maintain >=95% core unit line coverage per file and overall.
