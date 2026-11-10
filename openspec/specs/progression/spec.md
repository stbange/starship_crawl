## Purpose

Character builds, defensive resources, shared caches, and persistent equipment.

## Requirements

### Requirement: Ten attributes

Characters SHALL support damage, vitality, haste, armor, shield, mobility, precision, recovery, reactor, and salvage upgrades, capped at 12 for phased expeditions and 50 for compatible legacy expeditions.

#### Scenario: Upgrade payment

- **WHEN** an eligible character selects an attribute
- **THEN** one point is spent, or the listed salvage price is paid when no point remains, and the chosen stat improves.

#### Scenario: Invalid upgrade

- **WHEN** an index is invalid, a stat is capped, or payment is unavailable
- **THEN** the request leaves the profile unchanged.

### Requirement: Defense layers

Incoming damage SHALL consume shield before armor absorption and health; defensive and weapon resources SHALL remain within their capacities.

#### Scenario: Shield absorption

- **WHEN** shield fully absorbs a hit
- **THEN** health and armor remain unchanged.

#### Scenario: Recharge

- **WHEN** the character avoids damage long enough
- **THEN** shield and health regenerate, while weapon energy recharges continuously.

### Requirement: Single-owner persistent caches

A cache SHALL have one recipient per expedition, persist consumption, and leave full-capacity supplies available until useful.

#### Scenario: Shared pickup

- **WHEN** two local players overlap a cache
- **THEN** only one receives its item or currency.

#### Scenario: Backtracking and reload

- **WHEN** a consumed cache is reconstructed after unloading or save/load
- **THEN** it remains consumed and yields no duplicate reward.

### Requirement: Armory and records

Weapon ownership and 12 collectible ship records SHALL persist with each character through checkpoint death, saves and additional commissions on the same ship. Starting a new ship SHALL reset both profiles.

#### Scenario: Duplicate weapon

- **WHEN** a character finds an already owned weapon
- **THEN** it becomes salvage instead of a second inventory copy.

### Requirement: Transformative specialization branches
Coherent-expedition specializations SHALL offer mutually exclusive branches with different
observable combat behavior. Only one specialization branch SHALL operate at a
time, within the existing loadout limits. Safe refitting SHALL explain the effect,
requirements and tradeoff before confirmation. Locked tools SHALL remain locked.

#### Scenario: Choose a beam relay branch
- **WHEN** an eligible character equips the relay branch and fires a compatible beam through an attached flare
- **THEN** the declared bounded relay effect occurs, while an unequipped alternative branch contributes no effects.

#### Scenario: Preview an unusable upgrade
- **WHEN** a character inspects a branch requiring an unearned power or rig
- **THEN** its requirement is clear and inspection neither unlocks that tool nor spends resources.

### Requirement: Bounded growth and useful rewards
Coherent-expedition growth SHALL preserve increasing power, bounded resources
and forgiving checkpoint death. Purchasing an attribute SHALL NOT provide an
undeclared full-resource combat reset. Challenge rewards SHALL preview usable
choices or explicit useful alternatives for owned items, with persistent
per-participant claims and no duplicate payment.

#### Scenario: Upgrade while injured
- **WHEN** an injured character purchases an attribute
- **THEN** only its declared capacity or resource effect occurs, with health, armor, shield and energy remaining within their new bounds.

#### Scenario: Own every offered item
- **WHEN** a participant qualifies for a challenge whose item choices are already owned
- **THEN** the preview offers a defined upgrade or preparation resource with an available use, and the participant can claim one reward once.

#### Scenario: Earn phase growth with a temporarily absent partner
- **WHEN** a phase guardian is defeated under the coherent rules and a participating character has temporarily left the local view
- **THEN** each participant's stored profile receives four attribute points once, capped at 1,000, while the separate tool choice remains claimable later; the opening elite and archived rules retain their existing awards.
