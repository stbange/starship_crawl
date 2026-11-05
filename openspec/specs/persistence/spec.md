## Purpose

Portable validated saves that preserve the active game when an operation fails.

## Requirements

### Requirement: Portable format and migration
Saves SHALL use checksums and explicit generator compatibility. Format 10 SHALL retain matrix state and all format-9 reactive events, support, outcomes, sites and careers. Formats 7–8 SHALL retain their narrative 1/generator 4 and narrative 2/generator 5 expeditions; formats 3–6 SHALL retain compatible legacy ships. New expeditions SHALL use ship generator 6, narrative 3 and matrix generator 1. Generator-5 expeditions SHALL retain their original encounters and geometry.

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
- **THEN** the new ship uses generator 6 and narrative version 3 while keeping character progression.

#### Scenario: Narrative round trip
- **WHEN** a format-7 expedition resumes
- **THEN** its discoveries, assigned locations, tracked lead and conclusion agree with the saved state.

#### Scenario: History round trip
- **WHEN** a format-8 expedition resumes
- **THEN** its generator-5 history, discoveries, fixed sites, conclusion and persistent purge agree with the saved state.

#### Scenario: Reactive chapter round trip
- **WHEN** a format-9 expedition resumes during a follow-on story
- **THEN** effects, support, events, fixed leads, debriefs and both character careers remain consistent.

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

### Requirement: Transient death state
Format-9 saves SHALL support generator 6 while retaining all previously supported generators and narrative versions. Successfully loading a save or starting a new ship SHALL clear death animations and death hazards. Failed imports SHALL leave active effects and the expedition unchanged.

#### Scenario: Reload during a death effect
- **WHEN** a valid expedition is loaded while a corpse or mine is active
- **THEN** transient effects clear and saved defeated-enemy records remain defeated.

### Requirement: Matrix appendix and migration
Format 10 SHALL retain matrix profiles, fixed region records, equipped programs, avatar resources and coordinates, body return poses and active or reserved co-op links. Formats 1–9 SHALL remain supported and initialize matrix profiles safely. Unknown versions or invalid matrix fields SHALL leave all current spaces unchanged.
#### Scenario: Resume two links in solo
- **WHEN** a split-screen save contains divers in two different networks
- **THEN** solo loading restores the first diver and retains the second link for explicit co-op rejoin.
#### Scenario: Reject a corrupt network
- **WHEN** a checksummed save contains invalid graph masks, coordinates, program slots, effects or link references
- **THEN** the loader rejects it transactionally without replacing the current game.
