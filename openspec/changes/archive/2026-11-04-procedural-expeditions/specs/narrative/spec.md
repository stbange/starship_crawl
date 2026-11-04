## ADDED Requirements

### Requirement: Generated motivation and history
New expeditions SHALL select rescue, knowledge recovery or alien technology salvage from a versioned ship seed. Briefings and discoveries SHALL derive from one consistent hidden history and identify a goal and reward.

#### Scenario: Repeat a ship
- **WHEN** the same seed and narrative version begin again
- **THEN** motivation, ship identity and hidden history agree.

### Requirement: Discovery director
The director SHALL reveal reusable scenes through actual player exploration and interaction, with stable compatible locations and persistent discoveries. Chunk preloading and map browsing SHALL NOT advance the story.

#### Scenario: Alternate discovery
- **WHEN** a player discovers environmental evidence before following the initial lead
- **THEN** an alternative investigation becomes available and previously revealed sites stay fixed.

### Requirement: Consequential conclusions
Each motivation SHALL offer two conclusions and an additional conclusion enabled by its optional operation. Choices SHALL require the player at a cleared target. Extraction SHALL award a shared victory once and allow continued exploration.

#### Scenario: Optional preparation
- **WHEN** players secure the optional operation before resolving the target
- **THEN** the journal explains the newly available outcome and extraction reflects their chosen result.

### Requirement: Expedition journal
The journal SHALL distinguish the briefing, confirmed facts, discovered evidence, unresolved leads and outcome. Players SHALL be able to track a known lead on both maps and make choices through keyboard, controller and browser touch menus.

#### Scenario: Resume a lead
- **WHEN** players save after discovering evidence and resume
- **THEN** their facts, choices, fixed destinations and tracked objective remain available without duplicate rewards.
