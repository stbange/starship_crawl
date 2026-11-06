## MODIFIED Requirements

### Requirement: Portable format and migration
Saves SHALL use checksums and explicit generator compatibility. Format 11 SHALL retain phase progress, guardian participation and rewards, loadouts, matrix state and narrative state. Compatible formats 1–10 SHALL remain supported without resetting their expeditions or profiles. New expeditions SHALL use ship generator 9, narrative 3 and matrix generator 1.

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
- **THEN** the new ship uses generator 9 and narrative version 3 with fresh characters and matrix profiles.

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

## ADDED Requirements

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
