# Spec Delta

## ADDED Requirements

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
