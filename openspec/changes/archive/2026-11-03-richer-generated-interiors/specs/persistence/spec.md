## MODIFIED Requirements

### Requirement: Portable format and migration

Saves SHALL use a versioned checksum and explicit generator compatibility across
native and web. Format 6 SHALL retain sanctuary, psionics, rigs, flare upgrades and
attached lights. Compatible format-3/4/5/6 expeditions using generator 3 SHALL retain
their original ship and progress. New expeditions SHALL use generator 4.

#### Scenario: Legacy profile

- **WHEN** a supported version-1 or version-2 file is loaded
- **THEN** both character profiles migrate to a visibly announced fresh current-generator ship.

#### Scenario: Unknown version

- **WHEN** an unsupported format or generator is imported
- **THEN** the active expedition is preserved unchanged.

#### Scenario: Compatible format-3 expedition

- **WHEN** a format-3 save using generator 3 is loaded
- **THEN** its original architecture, records, missions and character progress remain with no deployed sanctuary.

#### Scenario: Compatible format-5 expedition

- **WHEN** a format-5 save using generator 3 is loaded
- **THEN** ship, sanctuary and powers remain, new personal guns unlock, and rigs start packed with full plating and no flare upgrades.

#### Scenario: New generation opt-in

- **WHEN** a player starts a new ship after loading a generator-3 expedition
- **THEN** the new ship uses generator 4 and keeps character progression.
