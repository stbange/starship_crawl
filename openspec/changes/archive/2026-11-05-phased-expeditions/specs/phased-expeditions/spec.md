## Purpose

Fresh-character expeditions with spatial phases, earned builds and directional ship identity.

## ADDED Requirements

### Requirement: Fresh ship characters
Each newly created ship SHALL start both characters with baseline attributes, basic weapons, no earned psionics or rigs, and initial matrix profiles. Checkpoint death, save/resume and another commission aboard the same ship SHALL retain the current characters.
#### Scenario: New ship after progression
- **WHEN** either character has earned equipment, attributes and matrix upgrades and the player starts another ship
- **THEN** both characters, including an inactive partner, start from baseline without those gains.
#### Scenario: Recover on this ship
- **WHEN** a character dies, reloads a valid save or accepts a same-ship commission
- **THEN** confirmed equipment, attributes and rewards remain owned.

### Requirement: Physical phases
New expeditions SHALL have four increasingly remote spatial phases, two guardian approaches per phase and an early optional elite. A qualifying victory SHALL open the next boundary for both players. Movement, rigs, flight and lifts SHALL obey the same boundary. Map inspection and streaming SHALL not advance progress.
#### Scenario: Alternate approach
- **WHEN** either guardian in the current phase is defeated
- **THEN** access advances once and the other guardian remains an optional challenge.
#### Scenario: Closed boundary
- **WHEN** a player approaches a later layer before its prerequisite victory
- **THEN** a visible boundary prevents entry through every movement method.

### Requirement: Seeded directional identity
Generator 7 SHALL assign thermal and order directions and polarities per seed, increase their intensity with displacement, and apply them to room layouts, environmental dressing and enemy composition while preserving connected routes, recognizable functions and bounded generation.
#### Scenario: Stable directional exploration
- **WHEN** the same ship region is generated on another platform or after backtracking
- **THEN** its traits, geometry and encounter identities agree regardless of load order.

### Requirement: Guardian attempts
Guardians SHALL have visible attack warnings, dodgeable projectiles, opening periods, distinct family patterns and a direct solo solution using available basic equipment. Leaving all participants outside the encounter, entering shelter or checkpoint death SHALL reset an unfinished attempt without losing earned progression. Loading an unfinished attempt SHALL return its participants to staging and reset that attempt.
#### Scenario: Retreat and retry
- **WHEN** the last engaged player retreats into shelter
- **THEN** the guardian returns to full health and cannot be defeated by accumulating damage across safe retreats.
#### Scenario: Co-op support
- **WHEN** one partner remains engaged while the other recovers
- **THEN** the encounter remains active with its original participation budget.

### Requirement: Earned capabilities and refitting
Guardian victories SHALL provide three stable reward choices with individual exactly-once receipts. New expeditions SHALL require owned and equipped weapons, powers and rigs. Refitting SHALL be available at safe staging and in the shelter, with two personal gun slots, two power slots, one rig slot and one specialization slot. Duplicate offers SHALL provide a useful explicitly described improvement.
#### Scenario: Claim and reload
- **WHEN** an eligible player chooses a reward and reloads
- **THEN** the chosen capability remains available and the encounter cannot pay that player again.
#### Scenario: Unowned ability
- **WHEN** a player activates an unowned or unequipped power or rig
- **THEN** the action explains the requirement and consumes no resource.

### Requirement: Story and preparation
New generated story targets SHALL lead to deeper phases and require the final guardian victory. Newly reserved targets SHALL follow an established foothold wing, or use a seeded wing when neither foothold guardian has been defeated; revealed destinations SHALL remain fixed. Optional power and security preparation SHALL change guardian pressure; every required confrontation SHALL remain completable without hacking or a particular reward choice.
#### Scenario: Final target
- **WHEN** a character reaches a generated target before defeating the last phase guardian
- **THEN** its final decision stays unavailable with clear phase guidance.

### Requirement: Portable phase interface
Keyboard, controller and touch/menu actions SHALL expose phase goals, alternative approaches, reward choices and loadout changes. The HUD and maps SHALL identify the current goal and local traits.
#### Scenario: Choose a build on touch
- **WHEN** a touch player earns a guardian reward
- **THEN** menu controls allow inspecting and claiming it and safely changing the equipped build.
