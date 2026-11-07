## ADDED Requirements

### Requirement: Bounded realistic-input journeys
Automated gameplay SHALL include deterministic reaction delays, limited turn speed, aim error and different earned-upgrade policies. It SHALL record minimum defenses and damage/stall observations every simulation tick, report simulated duration, and distinguish controller recovery from a demonstrated geometry fault. These runs SHALL remain separate from unit coverage.

#### Scenario: Review a journey
- **WHEN** an imperfect-input profile finishes
- **THEN** its report identifies its seed, input limits, build policy, simulation duration, progress, damage and navigation stalls without claiming human difficulty validation.

### Requirement: Human validation checkpoints
The project SHALL provide reproducible human playtest routes and a result template for first-time navigation, lift operation, HUD clarity, physical Windows and mobile web. Unperformed device checks SHALL remain explicitly pending.

#### Scenario: Perform a device check
- **WHEN** a tester opens the checklist
- **THEN** it provides build/checkpoint instructions, expected behavior and fields for actual device observations and failures.
