# vertical-traversal Specification

## Purpose
Let players navigate the ship's vertical architecture with physical stairs,
ramps and readable grav-lift journeys while retaining safe streaming and saves.

## Requirements

### Requirement: Continuous stairs and ramps
New ships SHALL provide connected service shafts containing stairs and sloped ramps between adjacent decks. The loading bay SHALL have access to both routes. These routes SHALL be traversable and backtrackable under normal gravity without jumping, flight powers or lift commands.

#### Scenario: Walk between decks
- **WHEN** a player follows a stair or ramp from its marked landing
- **THEN** the player's height changes continuously and the next deck is reachable, with a walkable route back to the starting deck.

### Requirement: Step and slope collision
Walking bodies SHALL climb ordinary steps, follow traversable slopes downwards, and respect ceilings, railings, steep obstacles and incomplete rooms. Displayed stair/ramp surfaces SHALL block movement and projectiles consistently.

#### Scenario: Clearance and edges
- **WHEN** a player walks onto a step under a low ceiling or tries to cross a tall obstacle
- **THEN** movement remains blocked instead of pushing the body through geometry.

#### Scenario: Crouching passage
- **WHEN** a crouched player attempts to stand in insufficient headroom
- **THEN** the body remains crouched and can move back to a clear standing area.

### Requirement: Animated grav lift
New-ship grav lifts SHALL move the player visibly and continuously between neighboring deck pads, with a clear in-transit indicator and a finite journey. Camera look SHALL remain available. Movement and conflicting interactions SHALL not eject the rider midway through the shaft.

#### Scenario: Observe the journey
- **WHEN** a player activates an available lift
- **THEN** intermediate vertical positions are visible before landing and checkpoint synchronization occurs on arrival.

### Requirement: Safe shared traversal
Vertical travel SHALL respect phase barriers, wait for loaded destinations and retain both active players' neighborhoods. Each player SHALL be able to travel independently without moving the other's camera. Interrupted rides SHALL resolve to a safe pad.

#### Scenario: Locked or unloaded destination
- **WHEN** the next deck is phase-locked or not ready
- **THEN** the player stays safely on the current pad and receives an explanation.

#### Scenario: Independent passengers
- **WHEN** one local player rides while the other explores a different room
- **THEN** both retain their own position, camera and traversable streamed neighborhood.

### Requirement: Deliberate lift departures
An animated lift SHALL require release after arrival before accepting another trip. Adjacent destination decks, their room identities and sealed access SHALL be previewed at the pad. Arrival SHALL identify the deck and room and provide an exit cue without taking camera control away from the player.

#### Scenario: Held travel button
- **WHEN** the player holds a lift button throughout a ride and after arrival
- **THEN** the player remains at that landing until releasing and requesting another trip.

### Requirement: Readable service landings
Service landings SHALL distinguish stairs, ramps, deck exits and continuing vertical routes with spatial signs and consistent colors, including negative decks.

#### Scenario: Between floors
- **WHEN** a player is partway along a service route
- **THEN** navigation identifies the next physical landing rather than an unreachable pad at another height.
