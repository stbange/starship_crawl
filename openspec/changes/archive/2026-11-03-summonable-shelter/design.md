## Context

The world core uses sector/local positions and shares one bounded stream cache across two players. Sanctuary coordinates must never be treated as generated ship cells.

## Decisions

Keep each sheltered player's world position as its exact return anchor, and maintain a separate bounded local sanctuary position for movement and rendering. Exclude sheltered players from hostile targeting, direct damage, ship pickups, mission interaction, and weapon firing. The exterior world continues for other players.

A shared sanctuary is free to summon on ready, supported, sufficiently clear ship floors. Summoning cannot relocate an occupied sanctuary. Door use is proximity based; exit validates the retained return location. Interior walls and furnishings use shared collision geometry. Interior rendering and lights are isolated from ship meshes, enemies, projectiles, and lamps.

The bed restores resources over six seconds of uninterrupted rest. Movement cancels rest. The medical station heals and repairs armor; the kitchen cooks stocked rations, heals partially, and grants a 120-second recovery boost. No hunger penalty or resource cost is added.

Save format 4 appends shelter data after the existing record array. Format 3 preserves the expedition and initializes no shelter; v1/v2 retain their existing new-ship migration. Validate all appended booleans, finite positions, bounds, return geometry invariants, and resource timers before replacing the game.

## Validation

Unit tests cover supported/blocked placement, proximity, collision, recovery/cancellation, damage and targeting isolation, co-op, new ships, v3 migration, v4 round trips, and malformed appended fields. A gameplay driver visits all facilities and returns using movement/actions. Browser tests exercise keyboard entry/exit, rendering, persistence, and touch controls. Run coverage, sanitizers, native rendering, Windows compilation/CI, and strict specifications before publishing.
