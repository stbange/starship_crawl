# navigation Specification

## Purpose
Bounded deck schematics, player orientation, and mission guidance.

## Requirements

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

### Requirement: Actionable connection guidance
Each player SHALL receive an independent next connection cue derived from reciprocal generated doors and phase access. The HUD and maps SHALL distinguish this connection from the final objective and show vertical direction and destination deck. Guidance SHALL use bounded computation without generating chunks, discovering stories or modifying progress.

#### Scenario: Objective behind a bulkhead
- **WHEN** the objective is not directly connected to the player's cell
- **THEN** the next cue identifies an accessible connecting doorway along a bounded route instead of pointing straight through the bulkhead.

#### Scenario: Another deck or special space
- **WHEN** the objective requires vertical travel
- **THEN** the cue identifies the lift or service landing and destination deck, and ship cues are suppressed inside the shelter and matrix.
