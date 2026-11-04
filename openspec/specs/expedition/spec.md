## Purpose

A completable Reclaimer story layered over endless ship exploration.

## Requirements

### Requirement: Six-stage route
Compatible legacy expeditions SHALL retain the bay, reactor, archive, relay, neural-heart and beacon route. New expeditions SHALL use generated motivations, discoveries, optional operations and branching extraction conclusions.

#### Scenario: End-to-end expedition
- **WHEN** a reclaimer resumes a legacy expedition and clears its encounters
- **THEN** all six original terminals can advance in order and extraction records one victory.

#### Scenario: New generated expedition
- **WHEN** a player begins a new ship
- **THEN** a generated briefing starts a completable story over the endless architecture.

### Requirement: Heart gate

The neural-heart terminal SHALL reject activation while its Warden is alive, including when lured away from the terminal.

#### Scenario: Warden survives

- **WHEN** a character attempts to sever the heart with a living Warden in its room
- **THEN** the mission remains unchanged until the Warden is defeated.

### Requirement: Shared rewards and contracts

Mission rewards SHALL go to participating characters and each side contract SHALL pay once per expedition.

#### Scenario: Repeated contract evaluation

- **WHEN** a completed contract is evaluated again or reloaded
- **THEN** its points and salvage are not granted a second time.

### Requirement: New expedition

Starting a new ship SHALL retain character builds, armory, lore, salvage, and victories while resetting ship changes, missions, contracts, and projectiles.

#### Scenario: Another ship

- **WHEN** the player confirms a new expedition
- **THEN** the ship seed and expedition state change while persistent character progression remains.
