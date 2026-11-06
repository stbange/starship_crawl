## ADDED Requirements

### Requirement: Living tentacle infestation
The renderer SHALL add tapered, curling animated tentacles with readable surface detail to seeded floor growths and wall nests, keeping the loading bay and low conduits clear. Floating gazers SHALL carry trailing tentacles. Environmental tentacles SHALL remain decorative and preserve existing routes, enemy capabilities and saved expeditions.

#### Scenario: Inspect an infestation
- **WHEN** a player approaches a generated growth or nest with the flashlight
- **THEN** rooted tentacles, suckers and faint luminous tips are visible and their movement stops when the game pauses.

#### Scenario: Revisit an existing ship
- **WHEN** an existing expedition is loaded or a room is streamed again
- **THEN** tentacles reconstruct from its existing growth anchors without changing architecture, collision or encounter identities.

### Requirement: Bounded tentacle views
Environmental tentacles SHALL use a bounded per-camera detail budget, favor nearby growths and retain stable world anchors. Tentacles from the ship SHALL not appear in sheltered, matrix or exterior views.

#### Scenario: Separate local players
- **WHEN** one player enters the shelter while the other observes an infestation
- **THEN** the first view contains no ship tentacles and the second continues to animate them within its own budget.
