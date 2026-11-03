## MODIFIED Requirements

### Requirement: Weapon and enemy catalog

The game SHALL provide 16 usable weapons and 27 enemy species with their declared shared behavior systems and distance-based encounter gating. All 16 weapons SHALL emit finite visible projectiles with their original impact effects, and the catalog SHALL cover five body families with five structural variants each.

#### Scenario: Armory effects

- **WHEN** each weapon hits an in-range visible enemy with sufficient energy
- **THEN** damage or its declared projectile effect reaches the enemy.

#### Scenario: Energy exhaustion

- **WHEN** a weapon has insufficient energy or the projectile pool is full
- **THEN** it does not consume energy or create a partially emitted volley.
