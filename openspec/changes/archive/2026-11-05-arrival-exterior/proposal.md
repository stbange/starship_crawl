# Proposal

## Why

The arrival currently shows a small patch of stars and a hull obscured by interior fog. It needs a clear contrast between the immense vessel and the player's small, ordinary boarding craft.

## What Changes

- Render a surrounding starfield with correct hull occlusion and sparse exterior lamps that reveal the ship's scale in every direction.
- Start new expeditions a little farther from the bay, immediately outside an open boarding skiff. Players can float back inside, explore its simple cabin, and leave through the same hatch.
- Share the skiff's visible geometry with movement and projectile collision; keep it stationary with no loot, services, missions, or controls.
- Introduce generator 8 for the new exterior, retaining generator 7's interior generation and the existing format-11 save layout. Older saves keep their original exterior collision and positions; visual star/hull improvements apply to all ships.

## Capabilities

### New Capabilities
- `space-exterior`: Arrival presentation, a surrounding sky, and a physically accessible boarding craft.

### Modified Capabilities
- `persistence`: New expeditions use generator 8 while generator-7 saves retain their original exterior. The format-11 payload stays unchanged.

## Impact

Core exterior geometry, world collision/rays, arrival position, renderer sky/fog/lamps, save generator validation, and browser inspection scenes. No dependencies or asset downloads. Validate with native unit and gameplay CTests, the >=95% per-file coverage gate, strict current/archived OpenSpec checks, packaged browser suites, and native Mesa render captures; compile the Windows target too.
