## ADDED Requirements
### Requirement: Transient death state
Format-9 saves SHALL support generator 6 while retaining all previously supported generators and narrative versions. Successfully loading a save or starting a new ship SHALL clear death animations and death hazards. Failed imports SHALL leave active effects and the expedition unchanged.

#### Scenario: Reload during a death effect
- **WHEN** a valid expedition is loaded while a corpse or mine is active
- **THEN** transient effects clear and saved defeated-enemy records remain defeated.

## MODIFIED Requirements
### Requirement: Portable format and migration
Saves SHALL use checksums and explicit generator compatibility on native and web. Format 9 SHALL retain reactive events, faction support, chapter outcomes, fixed sites and careers. Formats 7 and 8 SHALL retain narrative 1/generator 4 and narrative 2/generator 5 respectively. Compatible formats 3–6 SHALL retain their ships and legacy campaign. New expeditions SHALL use generator 6/narrative 3. Generator-5 expeditions SHALL retain their original encounters and geometry.

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
- **THEN** the new ship uses generator 5 and narrative version 3 while keeping character progression.

#### Scenario: Narrative round trip
- **WHEN** a format-7 expedition resumes
- **THEN** its discoveries, assigned locations, tracked lead and conclusion agree with the saved state.

#### Scenario: History round trip
- **WHEN** a format-8 expedition resumes
- **THEN** its generator-5 history, discoveries, fixed sites, conclusion and persistent purge agree with the saved state.

#### Scenario: Reactive chapter round trip
- **WHEN** a format-9 expedition resumes during a follow-on story
- **THEN** effects, support, events, fixed leads, debriefs and both character careers remain consistent.
