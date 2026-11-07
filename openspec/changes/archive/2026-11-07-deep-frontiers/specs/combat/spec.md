## MODIFIED Requirements

### Requirement: Weapon and enemy catalog

The game SHALL provide 20 personal weapons, three mech weapons and 42 enemy species with shared behavior systems and distance-based gating. The original 16 guns SHALL retain visible projectiles and impact effects. Three added guns SHALL fire visible wall-bounded beams; the flare cannon SHALL attach non-damaging surface lights. Enemy models SHALL cover the original five body families and a sixth robotic family, with five structural variants each.

#### Scenario: Armory effects

- **WHEN** each damaging weapon hits an in-range visible enemy with sufficient energy
- **THEN** damage or its declared projectile effect reaches the enemy.

#### Scenario: Energy exhaustion

- **WHEN** a weapon has insufficient energy, or a projectile weapon cannot fit its entire volley in the pool
- **THEN** it does not consume energy or create a partially emitted volley.
