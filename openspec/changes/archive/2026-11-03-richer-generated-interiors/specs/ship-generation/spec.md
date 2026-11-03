## ADDED Requirements

### Requirement: Varied functional interiors

Generator 4 SHALL derive a ship profile, interior plan and sector furnishings from
the seed. Suites, work bays and arcades SHALL differ in collision geometry, with
accessible side spaces and clear central routes, lift pads and mission consoles.

#### Scenario: Interior exploration

- **WHEN** different generated rooms are explored
- **THEN** functional furniture and partitions vary while foot and rig routes remain open.

### Requirement: Connected incident history

Generator 4 SHALL spread blackout, frost, quarantine, fire or bloom incidents along
open regional room connections. Distance from a source SHALL determine intensity,
and fixtures and surface details SHALL reflect that history.

#### Scenario: Following damage

- **WHEN** an affected room is generated away from the source
- **THEN** an open path of affected rooms leads back to the incident source.

### Requirement: Landmark discoveries

Some large footprints SHALL contain a banquet, conservatory, suspended engine or
reliquary. Their silhouettes SHALL span the hall and map markers SHALL identify
them. Weapon and record caches in ordinary and landmark interiors SHALL reward
side-space exploration while retaining stable collection identities.

#### Scenario: Returning to a discovered cache

- **WHEN** a side-space cache is collected and its room unloads or the game reloads
- **THEN** the same room reconstructs with that cache still consumed.
