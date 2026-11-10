# Spec Delta

## ADDED Requirements

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
