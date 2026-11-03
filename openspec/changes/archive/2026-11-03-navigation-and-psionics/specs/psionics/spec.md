## ADDED Requirements

### Requirement: Four psionic powers
Each character SHALL activate veil, flight, charged long jumps and a surrounding nova using rechargeable energy and explicit controls with visible feedback. Invalid activations SHALL preserve resources.

#### Scenario: Veil
- **WHEN** veil is active
- **THEN** enemies cannot acquire that player, existing projectiles can still hit, and a successful weapon shot or offensive nova reveals them.

#### Scenario: Flight and jumps
- **WHEN** flight or long-jump charge is active
- **THEN** flight supports ascent/descent and charged jumps travel farther while floors, ceilings, walls and unfinished rooms still block movement.

#### Scenario: Nova
- **WHEN** the nova is active outside the sanctuary
- **THEN** bounded pulses damage nearby visible enemies without friendly fire or damage through walls.

### Requirement: Sanctuary safety and expiry
Timed powers SHALL expire normally, and sanctuary occupants SHALL neither activate powers nor attack the exterior with them.

#### Scenario: Rest safely
- **WHEN** a powered character enters the sanctuary
- **THEN** their effects cannot hit ship enemies or affect other players while sheltered.
