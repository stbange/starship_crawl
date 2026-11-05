# matrix Specification

## Purpose
A seeded virtual ship layer turns terminal exploration and programs into persistent physical and narrative consequences.

## Requirements

### Requirement: Separate linked player spaces
Players SHALL enter a first-person network at physical terminals and a shelter workstation while retaining their body and return pose. Each player SHALL be able to explore independently in split-screen. Matrix and physical attacks SHALL remain isolated except for explicit system operations and body-danger ejection.
#### Scenario: Guard a diver
- **WHEN** one player enters a field terminal and the other remains aboard
- **THEN** the avatar explores separately, the physical partner can guard the body, and body damage disconnects the diver.

### Requirement: Seeded bounded networks
Networks SHALL derive stable nodes and connections from physical ship regions and canonical room owners. At most two resident networks SHALL hold bounded geometry, guards and shots. Sector composition SHALL change scale, palette and structures; previews SHALL NOT advance story state.
#### Scenario: Return through a terminal
- **WHEN** a player revisits a terminal after streaming or reloading
- **THEN** its layout and committed discoveries, operations and defeated security identities remain consistent.

### Requirement: Solvable hacking encounters
Players SHALL have a visible pulse, integrity, bandwidth and trace. Guards SHALL telegraph finite swept shots. Credentials, a routing pair and explicit force access SHALL keep controller routes achievable alone. Failure SHALL preserve confirmed work and character progress while ejecting with a short terminal lock.
#### Scenario: Reach a protected controller
- **WHEN** a solo player lacks an advanced program
- **THEN** credentials, ordinary movement and switches or a declared force-access cost still provide a route.

### Requirement: Equipped programs
The matrix SHALL offer Veil, Echo, Interrupt, Bridge, Fork and Siphon, two equipped slots and persistent upgrades. Programs SHALL respect ownership, costs, caps and cooldowns, and unsuccessful activation SHALL NOT charge resources.
#### Scenario: Unlock an alternative
- **WHEN** a player finds a program cache and equips its program
- **THEN** its described stealth, distraction, interruption, bridge, routing or siphon effect becomes available and remains unlocked across new ships.

### Requirement: Durable shared consequences
Hacking SHALL affect mapped physical power, beacons, security, robots, distractions and story evidence. Operations and physical preparation SHALL share completion and exactly-once rewards. Final physical mission decisions SHALL remain at their targets.
#### Scenario: Complete preparation remotely
- **WHEN** a diver authenticates a mapped mission operation
- **THEN** its prerequisites, world effect and shared completion persist, and visiting its physical console does not pay again.

### Requirement: Generated controller stories
Networks SHALL provide finite controller conclusions, including seeded ORISON fragments, optional vault preparation, faction responses and persistent event records. Confirmed outcomes SHALL be awarded once per network.
#### Scenario: Preserve a controller
- **WHEN** a player resolves a prepared controller without destroying its archive
- **THEN** the preserved knowledge and faction response appear in persistent records and revisiting cannot duplicate the outcome.
