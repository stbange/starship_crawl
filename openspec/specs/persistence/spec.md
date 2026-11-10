## Purpose

Portable validated saves that preserve the active game when an operation fails.

## Requirements

### Requirement: Portable format and migration
Saves SHALL use checksums and explicit generator and simulation compatibility.
Supported formats 3–14 SHALL preserve their expeditions and profiles; formats
1–2 SHALL retain their existing announced migration. New-system expeditions
SHALL use explicitly versioned hidden factions and direction with narrative 3
and matrix generator 1. Phase progress, rewards, loadouts and existing systems
SHALL remain durable.

#### Scenario: Legacy profile
- **WHEN** a supported version-1 or version-2 file is loaded
- **THEN** both profiles migrate to a visibly announced fresh generated expedition.

#### Scenario: Unknown version
- **WHEN** an unsupported format, generator, simulation or narrative version is imported
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
- **WHEN** a player explicitly starts a new ship after loading a legacy expedition
- **THEN** the new ship enables the new hidden systems with narrative 3, matrix generator 1 and fresh characters only after the normal new-ship confirmation.

#### Scenario: Narrative round trip
- **WHEN** a format-7 expedition resumes
- **THEN** its discoveries, assigned locations, tracked lead and conclusion agree with the saved state.

#### Scenario: History round trip
- **WHEN** a format-8 expedition resumes
- **THEN** its generator-5 history, discoveries, fixed sites, conclusion and persistent purge agree with the saved state.

#### Scenario: Reactive chapter round trip
- **WHEN** a format-9 expedition resumes during a follow-on story
- **THEN** effects, support, events, fixed leads, debriefs and both character careers remain consistent.

#### Scenario: Interrupted guardian attempt
- **WHEN** a phased save resumes during a guardian fight
- **THEN** participants return to staging with a reset attempt while completed guardians and confirmed rewards remain intact.

#### Scenario: Saved generator-7 arrival
- **WHEN** a format-11 generator-7 expedition resumes
- **THEN** its original exterior collision, coordinates, interior and earned build remain unchanged.

#### Scenario: Saved political expedition
- **WHEN** a format-14 generator-13 expedition resumes
- **THEN** its existing courts, offenses, settlements and dispatches retain their original rules alongside all other saved progress.

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

### Requirement: Safe transit snapshots
Saving during a grav-lift journey SHALL preserve the expedition and build while recording a safe departure-pad position for the rider and occupied rig. Resuming SHALL clear transient ride state and leave the player free to travel again. Failed saves or imports SHALL not interrupt the current ride.

#### Scenario: Reload during a ride
- **WHEN** a save captured between decks is reloaded
- **THEN** the rider resumes at the departure pad with the same equipment and progression, without falling into unfinished geometry.

### Requirement: Independent device preferences
Control and rendering preferences SHALL be stored separately from expedition progress. Importing a save or starting a new ship SHALL preserve the current device's preferences.

#### Scenario: Import on another device
- **WHEN** an expedition is imported on a device with different control settings
- **THEN** its character and world load while that device retains its own control and rendering preferences.

### Requirement: Durable campaigns and intervention state
New-system saves SHALL retain faction resources and relationships, occupation,
forces, casualties, orders, travel, hostility evidence, simulation clocks,
randomness state, director memory, pending interventions, environmental changes
and reward claims. Both players SHALL share one world state. Temporary visual
effects SHALL remain separate from durable facts.

#### Scenario: Resume a marching force
- **WHEN** a save is loaded during a force's travel or local battle
- **THEN** its identity, surviving strength, order and valid location are restored once without replaying casualties, supplies or rewards.

#### Scenario: Resume during recovery
- **WHEN** a saved director is deferring reinforcements after high player pressure
- **THEN** its saved recovery state, pending actions and elapsed game time resume without a catch-up burst or a fresh random draw for the same decision.

#### Scenario: Solo resume of a shared war
- **WHEN** a split-screen expedition resumes in solo and its partner later rejoins
- **THEN** both observe one consistent campaign and their previously attributed actions and rewards remain intact.

### Requirement: Transactional simulation validation
Loading SHALL validate simulation versions, identifiers, references, resource
bounds, routes, timing, intervention reservations and unique force ownership
before replacing the active game. Corruption or allocation failure SHALL leave
the current expedition unchanged. Invalid data SHALL NOT be repaired by silently
discarding casualties, occupation, earned items or player progress.

#### Scenario: Duplicate a force in a checksummed save
- **WHEN** a valid-checksum payload contains the same force as both an abstract army and an independent physical squad
- **THEN** loading rejects it without mutating the active expedition.

#### Scenario: Reject invalid campaign state
- **WHEN** a valid-checksum payload contains a missing faction reference, impossible route, invalid resource count or intervention with a duplicate claim
- **THEN** loading fails transactionally and preserves the current world and profiles.

### Requirement: Streaming and history durability
Streaming, region eviction and diagnostic-log compaction SHALL preserve current
occupation, resolved casualties, resource changes and confirmed claims. Reloading
a visited region SHALL reconstruct its current state rather than its initial
population. Persistent history SHALL NOT require retaining every transient
animation, tactical actor or diagnostic event.

#### Scenario: Revisit after distant exploration
- **WHEN** a player travels far enough to evict a captured station and later returns after saving and loading
- **THEN** the station retains its occupation and durable evidence without resurrecting defeated force members or replenishing a consumed reward.

### Requirement: Coherent expedition compatibility
New expedition behavior SHALL use explicit compatible ruleset, generator,
narrative, simulation and storage versions as applicable. Supported earlier ships
SHALL retain their architecture, progression and system behavior. A new ship
SHALL reset all expedition benefits and characters. Invalid versions or
inconsistent references SHALL reject loading transactionally.

#### Scenario: Resume before district journeys
- **WHEN** a supported generator-15 save resumes after the coherent-expedition implementation ships
- **THEN** it retains its original world, unlock schedule, earned equipment and saved story without acquiring newly generated district contracts or altered combat rules.

#### Scenario: Reject an inconsistent new district
- **WHEN** a checksummed save contains a resolved district referencing a missing guardian, duplicated claim or impossible recurring individual
- **THEN** loading fails without replacing or partially changing the active game.

### Requirement: Durable shared expedition consequences
Build branches, district changes, condition state, individual fates, campaign
consequences, director sequences, preparations and claims SHALL survive streaming
and saves. Co-op SHALL share one world while preserving personal choices and
eligibility. Checkpoint saves SHALL retain existing guardian-reset semantics;
portable live snapshots SHALL preserve active attempts and transient simulation
state under their separate contract.

#### Scenario: Save after a district victory
- **WHEN** players leave the district, save, resume in solo and later rejoin in co-op
- **THEN** both observe the same opened routes, inhabitants and story outcome, with each player's earned or unclaimed rewards retained exactly once.

#### Scenario: Compare save and live snapshot
- **WHEN** an active multi-stage guardian encounter is captured both as a checkpoint and as a portable live snapshot
- **THEN** checkpoint restoration returns participants to staging with preparation retained, while live-snapshot restoration retains the active stages, participants and systems without replaying rewards.

#### Scenario: Resume an endless expedition
- **WHEN** transient diagnostic history is compacted after long exploration
- **THEN** retained district outcomes, individual fates, ownership and confirmed claims remain authoritative without requiring every old animation or diagnostic event.
