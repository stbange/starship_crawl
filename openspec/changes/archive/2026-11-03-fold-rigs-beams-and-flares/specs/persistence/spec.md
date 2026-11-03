## MODIFIED Requirements

### Requirement: Portable format and migration

Saves SHALL use a versioned checksum and explicit generator compatibility across native and web. Format 6 SHALL retain validated sanctuary and psionic state, rig deployment/plating/mounted state, flare upgrades and attached lights. Compatible format-3/4/5 expeditions SHALL retain their ship and progress, initializing unavailable equipment to defaults.

#### Scenario: Legacy profile

- **WHEN** a supported version-1 or version-2 file is loaded
- **THEN** both character profiles migrate to a visibly announced fresh version-3 ship.

#### Scenario: Unknown version

- **WHEN** an unsupported format or generator is imported
- **THEN** the active expedition is preserved unchanged.

#### Scenario: Compatible format-3 expedition

- **WHEN** a format-3 save using generator 3 is loaded
- **THEN** its ship, records, missions, and character progress are retained with no deployed sanctuary.

#### Scenario: Compatible format-5 expedition

- **WHEN** a format-5 save using generator 3 is loaded
- **THEN** the ship, progress, sanctuary and powers remain, the four new personal guns unlock, and rigs start packed with full plating and no flare upgrades.
