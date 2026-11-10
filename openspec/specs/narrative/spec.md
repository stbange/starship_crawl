# narrative Specification

## Purpose
Seeded expedition motivations, consistent discoveries and consequential player decisions over an endless ship.

## Requirements

### Requirement: Generated motivation and history
New expeditions SHALL select rescue, knowledge recovery, alien technology salvage or containment from a versioned ship seed. Narrative versions 2 and 3 SHALL support direct commissions and targets with dependencies, using the same history as generator 5. Briefings and discoveries SHALL derive from one consistent hidden history and identify a goal and reward.

#### Scenario: Repeat a ship
- **WHEN** the same seed and narrative version begin again
- **THEN** motivation, ship identity and hidden history agree.

### Requirement: Discovery director
The director SHALL reveal reusable scenes through actual player exploration and interaction, with stable compatible locations and persistent discoveries. Chunk preloading and map browsing SHALL NOT advance the story.

#### Scenario: Alternate discovery
- **WHEN** a player discovers environmental evidence before following the initial lead
- **THEN** an alternative investigation becomes available and previously revealed sites stay fixed.

#### Scenario: Physical history discovery
- **WHEN** a player enters an origin, survivor waystation or controller notice room after accepting the commission
- **THEN** its matching record appears once in the dossier without granting mission rewards.

### Requirement: Consequential conclusions
Each motivation SHALL offer two conclusions and an additional conclusion enabled by its optional operation. Choices SHALL require the player at a cleared target. Extraction SHALL award a shared victory once and allow continued exploration.

#### Scenario: Optional preparation
- **WHEN** players secure the optional operation before resolving the target
- **THEN** the journal explains the newly available outcome and extraction reflects their chosen result. Narrative 3 SHALL also support preparation supplied by a negotiated faction agreement.

#### Scenario: Containment alternatives
- **WHEN** a containment target is resolved
- **THEN** the player may purge it, seal it aboard, or preserve research after completing isolation preparation; a purge persists the deaths of creatures at the origin.

#### Scenario: Direct commission
- **WHEN** a direct rescue, knowledge or salvage target is resolved
- **THEN** the primary conclusion completes the job without inventing a dependency cost, and optional preparation enables an enhanced outcome.

### Requirement: Expedition journal
The journal SHALL distinguish the briefing, confirmed facts, discovered evidence, unresolved leads and outcome. A ship dossier SHALL reveal history facts only after relevant evidence or physical notices are discovered. Players SHALL be able to track a known lead on both maps and make choices through keyboard, controller and browser touch menus.

#### Scenario: Resume a lead
- **WHEN** players save after discovering evidence and resume
- **THEN** their facts, choices, fixed destinations and tracked objective remain available without duplicate rewards.

### Requirement: Recurring procedural individuals
Each coherent expedition SHALL select two or three identifiable recurring individuals
whose roles connect its motivation, ship condition and local conflicts. Identity,
knowledge and fate SHALL persist. Reappearances SHALL follow a plausible reachable
location and recorded events. Essential progression SHALL survive a missed
meeting, departure or death through an established alternative source.

#### Scenario: Meet an engineer again
- **WHEN** a player helps an engineer restore a district and later meets that individual elsewhere
- **THEN** their identity and response acknowledge that action and their location or physical traces explain the transition.

#### Scenario: Lose a recurring individual
- **WHEN** recorded events kill an individual before a planned meeting
- **THEN** later scenes use the recorded aftermath without duplicating or resurrecting that individual, and required information remains obtainable from a compatible alternative.

### Requirement: Understandable local stakes
Coherent-expedition choices SHALL connect observable local needs to the expedition's
motivation. Discovered facts SHALL distinguish known costs from uncertainty before
confirmation. Subsequent scenes, shelter objects and the conclusion SHALL reflect
recorded outcomes without requiring an optional dialogue, escort or faction menu.

#### Scenario: Redirect refuge power
- **WHEN** a player considers diverting power away from an inhabited refuge
- **THEN** observed dependencies and an available warning explain the known cost before confirmation, and the subsequent refuge and expedition conclusion reflect the chosen state.

#### Scenario: Continue without a conversation
- **WHEN** a player bypasses an optional individual's interaction
- **THEN** physical evidence and required objective guidance still support the main expedition without automatically resolving the individual's optional request.
