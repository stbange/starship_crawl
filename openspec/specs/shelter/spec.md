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

### Requirement: Safe matrix workstation
The shelter SHALL provide a clearly marked network workstation linked to its physical ship region. Login, save/resume and disconnect SHALL preserve the shelter position and exterior return anchor, and hostile physical effects SHALL NOT cross into the sanctuary.
#### Scenario: Research from shelter
- **WHEN** a player uses the workstation during an expedition
- **THEN** the avatar explores the corresponding network and disconnecting returns to the same safe interior position.

### Requirement: Optional expedition preparation
Coherent-expedition shelters SHALL offer safe refitting, regional meals and bounded
salvage-funded expedition projects with explicit costs and effects. Basic recovery
SHALL remain free and effective, without hunger or upkeep penalties. Preparations
SHALL be optional for required routes. Only one meal benefit SHALL apply per
character at a time, and project effects SHALL obey declared limits.

#### Scenario: Prepare without salvage
- **WHEN** an injured character enters the shelter with no salvage or special ingredients
- **THEN** the bed, medical station and stocked basic meal remain usable and the character can continue the required expedition.

#### Scenario: Replace a meal
- **WHEN** a character confirms a different available meal
- **THEN** its displayed benefit replaces the previous meal benefit, charges only its declared cost and cannot stack through repeated use or reload.

#### Scenario: Purchase a capped project
- **WHEN** a player attempts to buy an already completed or unaffordable expedition project
- **THEN** no resources are spent and no additional effect or reward is granted.

### Requirement: Shelter reflects the expedition
Coherent-expedition objects and projects SHALL provide a bounded persistent record
of this ship's discoveries and decisions. Display objects SHALL preserve interior
collision and interaction access. Starting a new ship SHALL reset these objects
and project benefits with the fresh characters; they SHALL NOT transfer combat
power between expeditions.

#### Scenario: Bring a recovered object home
- **WHEN** a player earns a district keepsake and returns to the shelter after saving and loading
- **THEN** its display and description reflect the actual recorded outcome, without blocking the doorway or duplicating the associated reward.
