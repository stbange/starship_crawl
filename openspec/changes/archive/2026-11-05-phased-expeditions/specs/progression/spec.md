## MODIFIED Requirements

### Requirement: Ten attributes

Characters SHALL support damage, vitality, haste, armor, shield, mobility, precision, recovery, reactor, and salvage upgrades, capped at 12 for phased expeditions and 50 for compatible legacy expeditions.

#### Scenario: Upgrade payment

- **WHEN** an eligible character selects an attribute
- **THEN** one point is spent, or the listed salvage price is paid when no point remains, and the chosen stat improves.

#### Scenario: Invalid upgrade

- **WHEN** an index is invalid, a stat is capped, or payment is unavailable
- **THEN** the request leaves the profile unchanged.

### Requirement: Armory and records

Weapon ownership and 12 collectible ship records SHALL persist with each character through checkpoint death, saves and additional commissions on the same ship. Starting a new ship SHALL reset both profiles.

#### Scenario: Duplicate weapon

- **WHEN** a character finds an already owned weapon
- **THEN** it becomes salvage instead of a second inventory copy.
