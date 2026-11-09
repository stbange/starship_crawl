# simulation-sessions Specification

## Purpose

Provide reusable headless expedition sessions with validated player commands, stable identity and portable live state across presentation adapters.

## Requirements

### Requirement: Validated player commands
A session SHALL execute supported gameplay intents through a common typed command contract. Invalid identities, non-finite values, unsupported kinds and invalid arguments SHALL be rejected without applying gameplay effects.

#### Scenario: Equivalent controls
- **WHEN** keyboard, controller, touch or Quest submits the same valid gameplay intent
- **THEN** the same validation and gameplay transition occur.

#### Scenario: Invalid command
- **WHEN** a malformed command or stale player identity is submitted
- **THEN** it is rejected and the expedition remains valid.

### Requirement: Independent local presentation
Local menus, maps, guidance settings and view selection SHALL be separate from authoritative body and character progression. Presentation feedback SHALL identify its intended recipient or the party.

#### Scenario: Independent views
- **WHEN** one local view changes its menu, map or guidance preference
- **THEN** another player's view and character progression are unchanged.

### Requirement: Simulation clock and residency
Given identical initial state and tick inputs, active-session simulation and chunk-generation work SHALL be independent of rendering frequency. Rendering uploads SHALL have an independent bounded budget.

#### Scenario: Unequal frame rates
- **WHEN** equivalent tick inputs are delivered through 30, 60 and 120 FPS schedules
- **THEN** simulation time, loaded simulation chunks and gameplay state agree at equal ticks.

### Requirement: Stable bounded identity
Sessions SHALL distinguish players from local views and use stable actor references across room-array compaction. Configured party capacity SHALL be independent of local view capacity.

#### Scenario: Relocated room storage
- **WHEN** eviction compacts room storage
- **THEN** an existing actor reference resolves to the same actor or becomes invalid, never another actor.

#### Scenario: Expanded headless party
- **WHEN** a supported four-player headless session runs with at most two local view bindings
- **THEN** commands, streaming and per-player progression address each participant independently.

### Requirement: Portable live state
A versioned live-state snapshot SHALL preserve active simulation state independently of checkpoint-save reconstruction. Invalid or truncated snapshots SHALL leave the destination unchanged. Local preferences SHALL remain local.

#### Scenario: Mid-combat snapshot
- **WHEN** live state is captured and restored during combat
- **THEN** active actors, projectiles, timers and participating players continue consistently.

#### Scenario: Corrupted snapshot
- **WHEN** a payload has an invalid version, count, field or length
- **THEN** restoration fails transactionally.

### Requirement: Existing expedition compatibility
The refactored applications SHALL retain supported save migrations, generator geometry, ordinary control behavior and separate Quest storage/build defaults.

#### Scenario: Legacy expedition
- **WHEN** an existing fixture is loaded after the refactor
- **THEN** its established geometry, progress and migration behavior match the pre-refactor implementation.
