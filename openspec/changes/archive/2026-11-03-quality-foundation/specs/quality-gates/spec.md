## ADDED Requirements

### Requirement: Executable specification validation

The repository SHALL provide pinned OpenSpec tooling, checked-in capability specifications, Codex integration, and a noninteractive strict specification validation command in CI.

#### Scenario: Specification check

- **WHEN** a contributor runs `npm ci` followed by `npm run spec:validate`
- **THEN** the checked-in specifications and active changes pass strict validation without interactive input.

### Requirement: Measured unit coverage

Unit tests SHALL cover at least 95% of executable lines overall and in each measured core source file. The metric SHALL include every `sc_core` source and shared math, exclude third-party code, and declare SDL/OpenGL/browser adapters as a separate integration-test scope. Reports SHALL include line, branch, and function results without claiming that each has the same threshold.

#### Scenario: Coverage regression

- **WHEN** unit line coverage falls below 95% overall or in any measured core source file
- **THEN** the coverage command and CI fail and retain the report for inspection.

#### Scenario: Honest unit measurement

- **WHEN** the unit coverage report is generated
- **THEN** only unit-labelled tests contribute, no uncovered core lines are excluded, and HTML plus machine-readable reports are produced.

### Requirement: Full campaign regression

The project SHALL automatically exercise a reproducible solo expedition using movement, combat, lift travel, earned upgrades, and mission interactions through extraction.

#### Scenario: Complete campaign

- **WHEN** the seeded campaign test runs from the space arrival
- **THEN** it follows connected routes, defeats encounters including the Warden, advances all six objectives, records a victory, and stays within streaming and projectile budgets.

### Requirement: Publication gates

Pages publication SHALL require passing native, unit coverage, strict specification, and actual browser game checks.

#### Scenario: Failed verification

- **WHEN** any required quality, native, or web job fails
- **THEN** Pages is not deployed and test or coverage artifacts remain available.
