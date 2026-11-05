# equipment Specification

## Purpose
Deployable armored rigs, visible beam weapons and upgradeable persistent surface lighting.

## Requirements

### Requirement: Fold-deployed personal rigs
Each player SHALL deploy and pack their owned and equipped rig on clear supported ship floors, mount it nearby, and exit to a clear position. Phased expeditions SHALL equip one earned mech or wagon; compatible legacy expeditions SHALL retain access to both. Occupied rigs SHALL resist packing. Larger collision bodies SHALL respect doors, ceilings, obstacles, unopened phase boundaries and unfinished rooms.

#### Scenario: Mech deployment
- **WHEN** the player deploys and mounts a mech
- **THEN** they receive a distinct cockpit HUD, heavy persistent plating and a three-weapon built-in arsenal.

#### Scenario: Wagon transport
- **WHEN** the player mounts a wagon and accelerates
- **THEN** it travels faster than on-foot movement, brakes when released, and allows personal weapons.

#### Scenario: Damage and repair
- **WHEN** damage penetrates personal shields while mounted
- **THEN** rig plating absorbs it before pilot damage, depleted plating ejects the pilot, and packing preserves depletion until repair at the sanctuary medical station.

### Requirement: Beam arsenal
The game SHALL provide three personal beam weapons whose visible beams hit immediately along bounded traces, respect intervening walls, retain energy/status/piercing rules and reveal a cloaked shooter. Mech armament SHALL include a beam weapon.

#### Scenario: Beam fire
- **WHEN** a beam weapon is fired with sufficient energy
- **THEN** a visible segment ends at its blocking wall or target and applies damage without creating a travelling bullet.

### Requirement: Upgradable attached flare lights
The flare cannon SHALL fire finite utility shots that attach to solid surfaces and illuminate nearby geometry. Players SHALL upgrade future flare lifetime and light strength/radius using earned resources, within stated caps. At most 32 active flares SHALL exist without retaining distant chunks.

#### Scenario: Fire and upgrade
- **WHEN** a fired flare reaches a wall after the player upgrades lifetime or strength
- **THEN** it remains attached, emits an upgraded light and expires after its remaining burn time.

#### Scenario: Capacity
- **WHEN** all flare slots are occupied
- **THEN** attaching a new flare replaces the nearest-to-expiry record and memory/lighting budgets remain bounded.
