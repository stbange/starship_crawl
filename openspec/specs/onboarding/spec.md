# onboarding Specification

## Purpose
Teach exploration and combat through earned systems, contextual optional instruction, and compatible saved expeditions.

## Requirements

### Requirement: Earned introductory systems
New expeditions SHALL begin with one usable personal weapon and a flashlight. Three kills
SHALL earn a second weapon. The first elite or guardian victory SHALL unlock the shared
shelter. An ordinary terminal clear of nearby visible threats in the second band SHALL offer matrix access after
the first phase victory. The first two guardian tiers SHALL defer rigs and specializations.
#### Scenario: New player
- **WHEN** a fresh character boards the ship
- **THEN** shelter, matrix, powers and vehicles are unavailable, while movement, combat,
  interaction, map, objective, settings and bay recovery remain available.
#### Scenario: Earn access
- **WHEN** the character recovers a matrix interface from an eligible terminal clear of nearby visible threats
- **THEN** access persists and a separate login introduces power restoration before full programs.

### Requirement: Optional contextual instruction
Guided, Minimal and Off SHALL be independently saved device preferences. Guided SHALL
present relevant basic lessons; Minimal SHALL retain new-equipment instruction; Off SHALL
suppress instructional hints without changing unlocks, navigation or combat information.
Only one lesson SHALL display at a time, deferred during danger, with device-appropriate
controls, dismissal, review and saved completion per character.
#### Scenario: Experienced player
- **WHEN** guidance is disabled and another ship begins
- **THEN** it remains disabled, the character starts fresh, and no tutorial wait blocks movement.
#### Scenario: Cooperative learning
- **WHEN** one player completes or dismisses a lesson
- **THEN** the other character's lesson receipt is unchanged.

### Requirement: Progressive presentation
Menus and HUD instructions SHALL reveal equipment controls when acquired and character
choices when relevant. Settings, help, saving and navigation SHALL remain accessible.
#### Scenario: Cross-platform opening
- **WHEN** a new player opens native, browser, touch or Quest controls
- **THEN** matrix, rig and power controls do not compete with basic exploration controls.

### Requirement: Procedural learning opportunities
New ships SHALL retain varied seeded architecture while bounding near-bay ordinary
encounters to readable threats. The invisible director SHALL protect the opening and
new-equipment practice from added pressure. Mandatory routes SHALL remain walkable.
#### Scenario: Revisit
- **WHEN** an opening room streams out and back in
- **THEN** its enemy identities and confirmed deaths remain stable.

### Requirement: Compatible saved expeditions
Existing expeditions SHALL retain their equipment, access, generated geometry and reward
tables. New saves SHALL preserve introduction receipts and reject invalid payloads without
replacing the current game. Device preferences SHALL remain independent of imports.
#### Scenario: Resume an older ship
- **WHEN** a generator-14 save is loaded
- **THEN** its shelter and matrix remain usable without new introductory prerequisites.
