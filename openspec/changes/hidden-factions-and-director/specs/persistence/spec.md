# Spec Delta

## MODIFIED Requirements

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

## ADDED Requirements

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
