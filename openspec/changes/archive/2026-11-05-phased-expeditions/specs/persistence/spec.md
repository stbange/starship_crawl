## MODIFIED Requirements

### Requirement: Portable format and migration
Saves SHALL use checksums and explicit generator compatibility. Format 11 SHALL retain phase progress, guardian participation and rewards, loadouts, matrix state and narrative state. Compatible formats 1–10 SHALL remain supported without resetting their expeditions or profiles. New expeditions SHALL use ship generator 7, narrative 3 and matrix generator 1.

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
- **THEN** the new ship uses generator 7 and narrative version 3 with fresh characters and matrix profiles.

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
