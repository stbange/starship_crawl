# political-expeditions Specification

## Purpose
Define seeded alien governments, playable political settlements and a bounded,
persistent encounter director that respects expedition history and compatibility.

## Requirements

### Requirement: Seeded political geography
New expeditions SHALL contain three alien governments with stable multi-sector
territories, wilderness gaps, courts, marked patrols and symmetric relationships.
Map and cache queries SHALL NOT count as player discoveries.

#### Scenario: Revisit a country
- **WHEN** players approach the same territory through different streaming orders
- **THEN** its government, sites and initial borders agree.

### Requirement: Playable disputes and consequences
Courts SHALL offer supply restoration, succession and annexation commissions.
Conduits and royal guards SHALL require combat victories accessible with ordinary
weapons. Shared outcomes SHALL alter relationships, rulers, territory or systems;
eligible participants SHALL receive each reward once.

#### Scenario: Settle a dispute
- **WHEN** a commissioned player secures the conduit and defeats its royal guard
- **THEN** the pledged settlement takes effect and survives save/resume.

### Requirement: Readable political interaction
Courts, ownership and patrol allegiance SHALL be visible in the world and maps.
Keyboard, gamepad and touch SHALL support audiences, decisions and goal tracking.
Peaceful players SHALL NOT be attacked by neutral patrols. Neutral patrols SHALL
NOT block story terminals; patrols hostile to active physical players SHALL
contest nearby terminals under the existing distance and visibility rules.

#### Scenario: Decline an audience
- **WHEN** a player closes an audience without choosing
- **THEN** no pledge or relationship changes.

#### Scenario: Investigate beside a peaceful patrol
- **WHEN** a story terminal has a visible neutral patrol nearby and no hostile threats
- **THEN** the player can investigate without attacking that patrol.

### Requirement: Bounded causal director
The director SHALL select physical recovery, diplomatic, discovery or conflict
dispatches using actual player presence, resources, pressure, cooldowns, budgets
and repetition limits. It SHALL preserve geography, enemy statistics and victories.
Its state and diagnostic reason history SHALL persist.

#### Scenario: Pause a dispatch
- **WHEN** play pauses or both players enter shelter or matrix
- **THEN** the dispatch clock and budget stop advancing.

### Requirement: Expedition compatibility
Generator 13 and save format 14 SHALL preserve older supported expeditions,
reject malformed state transactionally and reset politics on a new ship.

#### Scenario: Resume before politics
- **WHEN** a format-13 generator-12 expedition loads
- **THEN** its architecture, population and progression retain their prior rules.
