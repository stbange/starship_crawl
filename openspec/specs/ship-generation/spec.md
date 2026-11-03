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
