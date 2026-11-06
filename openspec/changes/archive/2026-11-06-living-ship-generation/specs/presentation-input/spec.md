## ADDED Requirements

### Requirement: Environmental sprite detail
Generated rooms SHALL support bounded depth-tested decorative sprites, steam, shallow water, colored rain, strange glows and local fog, selected by their environmental conditions.

#### Scenario: Explore damaged rooms
- **WHEN** a player views a leaking, venting or infested room
- **THEN** animated effects and anchored details express that condition without obscuring the HUD or removing combat telegraphs.

### Requirement: Independent environmental views
Atmosphere SHALL use stable world anchors, bounded per-view work and independent camera settings in optional split-screen. It SHALL not leak into the shelter, matrix or space.

#### Scenario: Separated co-op
- **WHEN** two players view different conditions or one enters the shelter
- **THEN** each view retains its appropriate fog and effects within the shared resource limits.
