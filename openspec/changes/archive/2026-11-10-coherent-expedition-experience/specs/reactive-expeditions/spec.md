# Spec Delta

## ADDED Requirements

### Requirement: Learnable environmental interactions
Coherent-expedition systems SHALL expose consistent triggers, warnings, affected areas
and durations. Weapons, physical controls, earned powers and hacking SHALL use the
same declared rules when affecting the same system. Hazards SHALL affect eligible
actors consistently and provide a basic avoidance route. Required objectives
SHALL remain reachable without owning an optional interaction tool.

#### Scenario: Electrify shallow water
- **WHEN** an active source energizes a connected conductive surface
- **THEN** the visible affected surface warns before damage, damage obeys the declared conductivity and actor rules, and raised or disconnected ground remains a valid escape.

#### Scenario: Redirect machinery
- **WHEN** a physical control or an unlocked matrix operation changes the same machine's routing
- **THEN** both update the same durable machine state and subsequent hazards, access and story observations agree.

#### Scenario: Enter a cold-cycle district
- **WHEN** the district approaches its next freeze or vent transition during active simulation
- **THEN** physical cues precede the bounded transition, previously taught timing remains consistent and a required route does not become permanently impassable.
