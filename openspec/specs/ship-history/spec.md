# ship-history Specification

## Purpose
Consistent seeded ship histories connect physical catastrophe evidence, survivor and controller notices, and the expedition dossier.

## Requirements

### Requirement: Coherent seeded history
A new ship SHALL derive its catastrophe, controlling entity, survivor agenda, population and origin from a fixed versioned seed. Its physical evidence, dossier and mission accounts SHALL agree.

#### Scenario: Discover an origin
- **WHEN** a player enters the catastrophe's origin
- **THEN** its room, damaged machinery and recorded account identify the same cause.

### Requirement: Historical discovery
Visiting an origin, survivor waystation or controller notice SHALL add a persistent field record after accepting the commission. Dossier facts SHALL distinguish confirmed discoveries from unresolved information.

#### Scenario: Remember a notice
- **WHEN** a discovered notice is revisited or the expedition resumes
- **THEN** its account and location remain unchanged and it does not award duplicate progress.

### Requirement: Readable dossier
Players SHALL be able to open a ship dossier through keyboard, gamepad and browser touch menus, including while sheltered.

#### Scenario: Review the ship
- **WHEN** a player opens the dossier
- **THEN** it shows the known ship purpose, population, history and available evidence without advancing the mission.

### Requirement: Local environmental accounts
Generated local tales SHALL link room function and incident conditions to physical evidence and a readable account in the dossier. They SHALL reconstruct deterministically without overwriting expedition story progression.

#### Scenario: Inspect a local trace
- **WHEN** a player opens the dossier in a room with environmental evidence
- **THEN** the account describes the same incident and evidence visible in that room.
