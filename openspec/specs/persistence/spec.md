## Purpose

Portable validated saves that preserve the active game when an operation fails.

## Requirements

### Requirement: Portable format and migration

Saves SHALL use a versioned checksummed representation shared across native and web, with explicit generator compatibility.

#### Scenario: Legacy profile

- **WHEN** a supported version-1 or version-2 file is loaded
- **THEN** both character profiles migrate to a visibly announced fresh version-3 ship.

#### Scenario: Unknown version

- **WHEN** an unsupported format or generator is imported
- **THEN** the active expedition is preserved unchanged.

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
