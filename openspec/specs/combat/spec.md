## Purpose

Responsive combat with distinct enemy abilities, weapon effects, and collision.

## Requirements

### Requirement: Weapon and enemy catalog

The game SHALL provide 16 usable weapons and 27 enemy species with their declared shared behavior systems and distance-based encounter gating.

#### Scenario: Armory effects

- **WHEN** each weapon hits an in-range visible enemy with sufficient energy
- **THEN** damage or its declared projectile effect reaches the enemy.

#### Scenario: Energy exhaustion

- **WHEN** a weapon has insufficient energy or the projectile pool is full
- **THEN** it does not consume energy or create a partially emitted volley.

### Requirement: Avoidable enemy projectiles

Ranged enemies SHALL telegraph attacks and fire finite-lived visible projectiles that collide with walls and player bodies.

#### Scenario: Swept collision

- **WHEN** a shot travels through a player between simulation ticks
- **THEN** the nearest intersected player is hit once and a nearer wall blocks it.

#### Scenario: Dodge

- **WHEN** the player leaves the locked aim line before a charged shot arrives
- **THEN** the shot misses the player.

### Requirement: Surface locomotion and abilities

Enemy abilities SHALL respect collision and encounter budgets, including climbing, leaping, charging, healing, regeneration, and brood spawning.

#### Scenario: Crawler ascent

- **WHEN** a climbing creature reaches a suitable wall
- **THEN** it ascends a real collision surface and later detaches.

#### Scenario: Brood limit

- **WHEN** a brood spawner emits offspring
- **THEN** its logical room never exceeds 20 encounter slots or recreates defeated identities for repeated rewards.

### Requirement: Forgiving respawn

Death SHALL restore the character at its checkpoint while retaining character progress and defeated encounter records.

#### Scenario: Combat death

- **WHEN** health reaches zero during an expedition
- **THEN** health, armor, shield, and energy are restored at the checkpoint and learned upgrades remain.
