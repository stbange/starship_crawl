## MODIFIED Requirements

### Requirement: Portable format and migration

Saves SHALL use a versioned checksummed representation shared across native and web, with explicit generator compatibility. Format 4 SHALL retain sanctuary state. Format 3 SHALL retain its compatible expedition without regenerating the ship. Format 5 SHALL retain validated psionic state and load compatible format-3 and format-4 expeditions without changing their ship or progress.

#### Scenario: Legacy profile

- **WHEN** a supported version-1 or version-2 file is loaded
- **THEN** both character profiles migrate to a visibly announced fresh version-3 ship.

#### Scenario: Unknown version

- **WHEN** an unsupported format or generator is imported
- **THEN** the active expedition is preserved unchanged.

#### Scenario: Compatible format-3 expedition

- **WHEN** a format-3 save using generator 3 is loaded
- **THEN** its ship, records, missions, and character progress are retained with no deployed sanctuary.
