## ADDED Requirements

### Requirement: Bounded deck maps
The game SHALL provide a north-up HUD minimap and an M map with panning, deck browsing, rooms and reciprocal doors derived from deterministic generation without generating world chunks.

#### Scenario: Read a map
- **WHEN** the player opens or pans the map or changes its displayed deck
- **THEN** the schematic matches generated room footprints and portals and leaves streaming work bounded.

### Requirement: Objective guidance
Both maps SHALL mark the player, current mission goal, lifts and deployed shelter. Off-map goals SHALL remain indicated at the edge with deck direction; completed expeditions SHALL omit obsolete goal markers.

#### Scenario: Another deck
- **WHEN** the next objective is outside the visible area or on another deck
- **THEN** the minimap shows its direction and deck difference and a lift cue.
