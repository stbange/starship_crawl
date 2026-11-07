## Purpose

Responsive combat with distinct enemy abilities, weapon effects, and collision.

## Requirements

### Requirement: Weapon and enemy catalog

The game SHALL provide 20 personal weapons, three mech weapons and 42 enemy species with shared behavior systems and distance-based gating. The original 16 guns SHALL retain visible projectiles and impact effects. Three added guns SHALL fire visible wall-bounded beams; the flare cannon SHALL attach non-damaging surface lights. Enemy models SHALL cover the original five body families and a sixth robotic family, with five structural variants each.

#### Scenario: Armory effects

- **WHEN** each damaging weapon hits an in-range visible enemy with sufficient energy
- **THEN** damage or its declared projectile effect reaches the enemy.

#### Scenario: Energy exhaustion

- **WHEN** a weapon has insufficient energy, or a projectile weapon cannot fit its entire volley in the pool
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

### Requirement: Readable bounded deaths
Every defeated enemy SHALL have a finite body-family death animation and debris. Defeat SHALL immediately remove its targeting, attacks and objective obstruction and award its defeat at most once. Visual snapshots SHALL remain valid across room streaming and use bounded storage.

#### Scenario: Defeat during streaming
- **WHEN** an enemy dies and its source room unloads
- **THEN** its detached death presentation expires safely and reloading the room does not revive it or repeat its rewards.

### Requirement: Avoidable death hazards
Selected enemies SHALL leave finite acid residue, a visibly delayed explosion or a dropped proximity mine with visible arming and triggered countdowns. Hazards SHALL respect walls, decks, shelter isolation and powered wards. Dangerous effects SHALL use storage separate from visual effects; saturation SHALL NOT silently evict armed hazards.

#### Scenario: Dodge an exploding corpse
- **WHEN** an explosive enemy dies near a player
- **THEN** a visible warning precedes the blast and moving outside its radius or behind cover avoids damage.

#### Scenario: Trap cleanup
- **WHEN** a dropped trap expires or a save successfully reloads
- **THEN** it can no longer damage the player and the defeated encounter remains defeated.

### Requirement: Robotic encounters
New generator-6 ships SHALL include five robotic species with drone, loader, custodian, tracked-platform and mobile-mine silhouettes. They SHALL use existing collision, encounter budgets and ability systems and remain distinguishable from the five original body families. Generator-3 through generator-5 ships SHALL retain their original encounter identities and placement.

#### Scenario: Resume an older ship
- **WHEN** a generator-5 expedition resumes after the update
- **THEN** its original enemy roster, positions and defeated identities remain intact.
