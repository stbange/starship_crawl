## Why

Players need a dependable place to recover during long expeditions. A compact summonable doorway into a spacious sanctuary supports the forgiving combat loop and reinforces the ship's impossible scale.

## What Changes

- Summon a fold shelter on clear, supported ship flooring with R, a controller binding, or touch/menu controls.
- Enter a separate 18 by 16 metre furnished interior through the small doorway; enemies and attacks remain outside.
- Use a bed for rest, a medical station for healing/armor repair, and a stocked kitchen for food and a timed recovery benefit.
- Share one sanctuary in optional co-op, prevent relocating it while occupied, and return each player to their own entry position.
- Persist sanctuary state in save format 4; read format 3 without regenerating its ship. Generator remains version 3; v1/v2 migrations remain supported.

## Capabilities

### New Capabilities
- `shelter`: Summoning, separate interior, recovery facilities, co-op, and persistence.

### Modified Capabilities
- `persistence`: Format 4 stores shelter state and migrates compatible format 3 expeditions intact.

## Impact

Core, renderer, input, web controls, portable saves, unit/browser/gameplay tests, and player documentation. No new runtime dependencies. Preserve the 95% unit line coverage gate overall and per measured source.
