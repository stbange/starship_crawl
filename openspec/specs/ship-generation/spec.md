## Purpose

Coherent deterministic ship generation and bounded streaming.

## Requirements

### Requirement: Deterministic sectors

The ship SHALL reconstruct the same architecture, themes, portals, and encounter identities from a seed and coordinates on native and web targets.

#### Scenario: Platform and load-order parity

- **WHEN** the same region is generated in another order or through WASM
- **THEN** its semantic fingerprint and layout match the native reference.

### Requirement: Connected architecture

Generated regional rooms SHALL have connected routes, reciprocal portals, clear connector paths, and complete multi-chunk footprints.

#### Scenario: Landmark traversal

- **WHEN** a player crosses a chunk seam inside a landmark
- **THEN** the shared hall has a continuous floor and no internal seam wall.

### Requirement: Bounded streaming

Streaming SHALL generate and upload at most two chunks per displayed frame and retain at most 192 resident chunks shared by active players.

#### Scenario: Separated players

- **WHEN** two players explore distant regions
- **THEN** both neighborhoods remain traversable within the shared cache budget.

#### Scenario: Incomplete frontier

- **WHEN** a requested room footprint is unfinished
- **THEN** collision prevents entering its missing floor until the complete footprint is ready.

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

### Requirement: History-shaped spaces
Generator 5 SHALL bias room composition toward the seeded cause, reserve a compatible physical origin, and spread related incidents through connected rooms. Historical furnishings SHALL preserve central routes, lifts and interaction clearances.

#### Scenario: Follow the aftermath
- **WHEN** players explore the origin district
- **THEN** a connected damage trail leads to a room whose function matches the catastrophe.

#### Scenario: Existing ship
- **WHEN** a generator-3 or generator-4 expedition is loaded
- **THEN** its original rooms, geometry, portals and incidents reconstruct unchanged.
