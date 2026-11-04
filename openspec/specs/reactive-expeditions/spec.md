# reactive-expeditions Specification

## Purpose
Persistent missions and survivor priorities change the ship through actual player actions.

## Requirements

### Requirement: Event-driven scene director
Narrative 3 SHALL reserve compatible stable scene locations using prerequisites, architecture and sector tags. Actual observations and confirmed interactions SHALL append persistent events and contextual transmissions. Cache scheduling and map inspection SHALL NOT advance missions.

#### Scenario: Discover an operation
- **WHEN** a player discovers evidence or investigates a witness
- **THEN** compatible operations become known once and stay at their assigned locations.

### Requirement: Physical mission effects
Restoring power SHALL improve local lighting and recovery. A secured beacon SHALL protect players in its local ward. Disabling a controller SHALL silence its local turrets. Both players SHALL share these persistent effects across streaming and save/load.

#### Scenario: Restore a route
- **WHEN** power and a beacon are secured
- **THEN** players can recover safely inside the marked ward while threats outside remain active.

### Requirement: Survivor priorities
Two survivor groups SHALL have distinct priorities, persistent support and contextual responses. Council choices SHALL require the player at a cleared site and optional evidence SHALL enable a negotiated outcome and an additional way to prepare the main conclusion.

#### Scenario: Broker an agreement
- **WHEN** players restore power and recover the cipher before the council decision
- **THEN** they can satisfy both groups and unlock their assistance without duplicate rewards.
