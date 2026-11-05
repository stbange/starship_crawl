## Purpose

Establish the enormous ship's scale against surrounding space and give the player an accessible, modest point of departure.

## ADDED Requirements

### Requirement: Surrounding space
The scene SHALL show a stable starfield in every unobstructed direction into space. The hull and cabin SHALL occlude stars, and translation SHALL NOT move nearby-looking stars past the player. Each split-screen camera SHALL use its own view.

#### Scenario: Look around outside
- **WHEN** the player looks sideways, away from the ship, upward or downward during the approach
- **THEN** uncovered sky contains stars and the opaque ship conceals stars behind it.

#### Scenario: Looking out from shelter or matrix
- **WHEN** a player occupies the fold shelter or ship matrix
- **THEN** the exterior starfield does not appear through those spaces' walls.

### Requirement: Vast illuminated hull
The exterior SHALL remain dark while sparse fixed lamps and structural ribs reveal its extent above, below and beside the loading bay. Exterior lights SHALL remain visible at distances beyond the interior fog range, and the hull SHALL conceal sky even at grazing angles.

#### Scenario: Arrival scale
- **WHEN** a new expedition begins or the player looks along the hull
- **THEN** the loading bay sits in a much larger illuminated wall with progressively smaller distant lights and no nearby visible hull edge.

### Requirement: Returnable boarding skiff
New expeditions SHALL begin floating immediately outside a small stationary spacecraft facing the ship. The craft SHALL have an open hatch, a modest furnished cabin, and physical walls. Players SHALL be able to enter, leave and return using ordinary movement, without a teleport or special interaction.

#### Scenario: Return from the bay
- **WHEN** a player floats back from the loading bay and enters the craft's open hatch
- **THEN** the cabin can be explored and exited again; its shell blocks movement and weapon shots.

#### Scenario: Modest interior
- **WHEN** players explore or use controls inside the craft
- **THEN** its seating and inactive furnishings give context but provide no loot, missions, upgrades, vehicle controls or recovery services.

#### Scenario: Saved visit and co-op
- **WHEN** a player saves inside the skiff, reloads, or another player joins there
- **THEN** the occupants remain in traversable space and can leave the craft normally.

### Requirement: Exterior compatibility
Generator 8 SHALL add the boarding skiff while reproducing generator 7's interior plans and encounters for the same seed. Older expeditions SHALL retain their exterior collision and stored positions. Cosmetic hull and sky presentation SHALL apply to both.

#### Scenario: Existing expedition
- **WHEN** a generator-7 save is loaded, including a position occupied by new skiff geometry
- **THEN** the expedition retains its original unobstructed exterior and progression without relocating the player.
