# regional-encounters Specification

## Purpose
Provide varied regional conditions, readable alien behavior, optional coordinated fights, earned-tool practice and persistent local consequences while retaining existing expeditions.

## Requirements

### Requirement: Functional directional variety
Generator 11 SHALL retain strong seeded directional traits while choosing multiple
hot and cold expressions from room function, incident and seed. Compatible
ecologies SHALL vary within those extremes. Generator 3–10 architecture and
population SHALL remain unchanged.

#### Scenario: Deep directional travel
- **WHEN** several hot or cold districts are generated
- **THEN** their weather, physical evidence and populations vary without losing the larger directional identity.

### Requirement: Alien commitments
Pilgrims, gazers and seam-runners SHALL use shared world states for awareness,
approach, visibly warned attack commitments and recovery. Committed attacks SHALL
retain a finite aim or movement direction, respect walls and permit evasion.
Surface movement SHALL attach only to actual geometry; animation SHALL reflect
movement and pause with simulation.

#### Scenario: Evade a warned attack
- **WHEN** a player moves aside after the attack locks
- **THEN** the attack follows its commitment and cannot instantly retarget the player.

### Requirement: Optional coordinated encounters
Optional nest/relay fights SHALL require deliberate activation, retain open exits,
use bounded existing encounter slots and provide visible support links. A living
support SHALL stagger attack commitments. Beams SHALL interrupt support, and all
damaging weapons SHALL be able to defeat the encounter. Killing support SHALL
visibly disrupt the remaining group.

#### Scenario: Break a support network
- **WHEN** a player disrupts or destroys the linked support
- **THEN** its shielding and coordinated commitments stop for the declared recovery period.

### Requirement: Earned useful rewards
Optional victories SHALL preview and award ship-local equipment or abilities,
preserve participation and allow each eligible player one claim. Claims and kills
SHALL survive save/load and streaming without duplication. Earned tools SHALL
have contextual instructions and an opportunity for safe practice.

#### Scenario: Resume a completed challenge
- **WHEN** an eligible player reloads before claiming or revisits after claiming
- **THEN** the unclaimed reward remains available or the claimed reward remains consumed respectively.

### Requirement: Room rhythm and local consequences
Generated routes SHALL mix quiet/staging rooms, trace encounters, hordes and
optional challenges. Local story evidence SHALL offer small actions with visible
persistent effects on nearby lights, atmosphere or machines, without blocking the
main expedition or requiring a particular weapon.

#### Scenario: Repair a local scene
- **WHEN** a player resolves an eligible local action and later reloads the room
- **THEN** its environmental consequence persists and its one-time reward cannot repeat.

### Requirement: Honest comparative verification
Automated comparisons SHALL exercise the real combat and interaction APIs with
starter and earned tools, limited aim/reaction and resource observations. Reports
SHALL distinguish simulated gameplay, software rendering and human/device play.

#### Scenario: Assess P1 completion
- **WHEN** the implementation is reported complete
- **THEN** each review item has executed automated checks and a reproducible human checkpoint, with unperformed device tests marked pending.
