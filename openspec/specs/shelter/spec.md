# shelter Specification

## Purpose
A summonable pocket sanctuary for safe recovery, shared local co-op, and persistent expeditions.

## Requirements

### Requirement: Supported summonable doorway
The player SHALL summon a compact sanctuary doorway on nearby clear, ready ship flooring without spending character resources. The doorway SHALL NOT relocate while any player occupies its sanctuary. Pressing summon again SHALL pack away an unoccupied deployed doorway; occupied shelters SHALL remain deployed.

#### Scenario: Invalid placement
- **WHEN** a player summons in space, an unfinished room, an obstructed passage, or while the sanctuary is occupied
- **THEN** the existing doorway and occupants remain unchanged and the player receives an explanation.

### Requirement: Separate safe interior
Door interaction SHALL transfer the player into a bounded 18 by 16 metre sanctuary, retaining an exact per-player return anchor. Sheltered players SHALL NOT be targeted, damaged, collect ship pickups, activate ship missions, or fire into the ship. Exterior simulation SHALL continue for outside players.

#### Scenario: Co-op refuge
- **WHEN** one player enters while the second remains outside
- **THEN** the first is safe inside a separately rendered sanctuary and the second continues normal combat.

#### Scenario: Return
- **WHEN** a sheltered player interacts near the interior doorway and the return location is ready and clear
- **THEN** that player returns to their retained ship position and camera orientation.

### Requirement: Useful furnished recovery
The sanctuary SHALL contain collidable furnishings, steady local lights, a bed, a medical station, and a kitchen with proximity interactions. Rest SHALL restore resources over time and stop on movement. Medical treatment SHALL restore health and armor. Cooking SHALL provide partial healing and a 120-second recovery benefit without a hunger penalty.

#### Scenario: Recovery facilities
- **WHEN** an injured player uses the medical station, rests at the bed, or cooks stocked rations at the kitchen
- **THEN** the corresponding resources or timed recovery benefit update within character capacities and the HUD identifies the effect.

### Requirement: Persistent sanctuary
Format-4 saves SHALL retain the deployed doorway, each player's interior state and return anchor, and recovery timers with transactional validation. Loading format 3 SHALL preserve its ship seed, records, missions, and character progress while initializing no sanctuary.

#### Scenario: Save while inside
- **WHEN** the player saves and reloads from within the sanctuary
- **THEN** they resume safely inside and can return to the same ship without losing expedition progress.

#### Scenario: Invalid sanctuary payload
- **WHEN** an appended sanctuary field is nonfinite, out of bounds, inconsistent, or corrupt
- **THEN** loading fails without replacing the active game.
