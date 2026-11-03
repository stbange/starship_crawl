## Why

Players need readable navigation, more varied enemy silhouettes, visible weapon fire, and powerful mobility/combat abilities.

## What Changes

- Add an always-visible top-left minimap and a pannable M deck map with objective, lift, player and shelter markers.
- Make R toggle an empty shelter between deployed and packed states; protect occupied shelters.
- Supply five enemy body families with five structural variants each, retaining all 27 species identities and behavior capabilities.
- Replace hitscan weapons with finite, visible swept projectiles while retaining pellets, piercing, blast, chain, burn, chill and leech.
- Add invisibility until firing, collision-aware flight, enhanced jumps, and a surrounding damage aura using rechargeable energy.
- Save format 5 preserves powers and reads compatible v3/v4 expeditions intact. Generator 3 and encounter IDs remain unchanged.

## Capabilities

### New Capabilities
- `navigation`: Deck maps and objective guidance.
- `psionics`: Four usable abilities with energy costs, durations and safety rules.

### Modified Capabilities
- `combat`: Every weapon uses visible projectiles and enemies have distinct body families.
- `shelter`: R packs away an unoccupied deployed shelter.
- `persistence`: Format-5 powers and compatible expedition retention.

## Impact

Core, renderer, desktop/controller/touch controls, saves, specifications and tests. No new runtime dependency. Retain >=95% core unit line coverage overall and per measured source.
