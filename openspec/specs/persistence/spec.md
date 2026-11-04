## Purpose

Portable validated saves that preserve the active game when an operation fails.

## Requirements

### Requirement: Portable format and migration
Saves SHALL use a checksum and explicit generator compatibility across native and web. Format 8 SHALL retain narrative events, history discoveries, sites and choices alongside equipment, sanctuary and powers. Format 7 SHALL retain generator 4 and narrative version 1. Compatible formats 3–6 SHALL retain their original ship and legacy campaign. New expeditions SHALL use generator 5 and narrative version 2.

#### Scenario: Legacy profile
- **WHEN** a supported version-1 or version-2 file is loaded
- **THEN** both profiles migrate to a visibly announced fresh generated expedition.

#### Scenario: Unknown version
- **WHEN** an unsupported format, generator or narrative version is imported
- **THEN** the active expedition remains unchanged.

#### Scenario: Compatible expedition
- **WHEN** a format-3 through format-6 save is loaded
- **THEN** its original geometry, records, campaign and existing systems remain intact.

#### Scenario: Compatible format-3 expedition
- **WHEN** a format-3 save using generator 3 is loaded
- **THEN** its original architecture, records, missions and character progress remain with no deployed sanctuary.

#### Scenario: Compatible format-5 expedition
- **WHEN** a format-5 save using generator 3 is loaded
- **THEN** ship, sanctuary and powers remain, new personal guns unlock, and rigs start packed with full plating and no flare upgrades.

#### Scenario: New generation opt-in
- **WHEN** a player starts a new ship after loading a generator-3 expedition
- **THEN** the new ship uses generator 5 and narrative version 2 while keeping character progression.

#### Scenario: Narrative round trip
- **WHEN** a format-7 expedition resumes
- **THEN** its discoveries, assigned locations, tracked lead and conclusion agree with the saved state.

#### Scenario: History round trip
- **WHEN** a format-8 expedition resumes
- **THEN** its generator-5 history, discoveries, fixed sites, conclusion and persistent purge agree with the saved state.

### Requirement: Transactional validation

The loader SHALL validate the complete payload, coordinates, stat limits, masks, resource values, records, and checksum before replacing the active game.

#### Scenario: Corrupt but checksummed data

- **WHEN** a payload contains an invalid field with a valid checksum
- **THEN** the loader rejects it without mutating the active game.

#### Scenario: Allocation failure

- **WHEN** memory allocation fails while loading a replacement world
- **THEN** temporary resources are freed and the existing game remains valid.

### Requirement: Atomic native writes

Native saves SHALL preserve the previous destination when allocation, flush, close, or replacement fails.

#### Scenario: Write failure

- **WHEN** a save cannot be flushed, closed, or atomically replaced
- **THEN** the previous valid save remains loadable and temporary output is removed.

### Requirement: Browser durability

The browser SHALL confirm persistence only after IndexedDB synchronization and support validated import/export.

#### Scenario: Reload

- **WHEN** a completed browser save is followed by a page reload
- **THEN** profiles, expedition state, resources, equipment, and consumed caches are restored in solo mode.
