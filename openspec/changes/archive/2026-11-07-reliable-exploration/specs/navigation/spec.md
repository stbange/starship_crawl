## ADDED Requirements

### Requirement: Actionable connection guidance
Each player SHALL receive an independent next connection cue derived from reciprocal generated doors and phase access. The HUD and maps SHALL distinguish this connection from the final objective and show vertical direction and destination deck. Guidance SHALL use bounded computation without generating chunks, discovering stories or modifying progress.

#### Scenario: Objective behind a bulkhead
- **WHEN** the objective is not directly connected to the player's cell
- **THEN** the next cue identifies an accessible connecting doorway along a bounded route instead of pointing straight through the bulkhead.

#### Scenario: Another deck or special space
- **WHEN** the objective requires vertical travel
- **THEN** the cue identifies the lift or service landing and destination deck, and ship cues are suppressed inside the shelter and matrix.
