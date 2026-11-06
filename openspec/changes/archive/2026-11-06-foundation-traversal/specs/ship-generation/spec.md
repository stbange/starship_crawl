## ADDED Requirements

### Requirement: Aligned service shafts
Generator 9 SHALL reserve deterministic vertical service shafts with matching footprints across adjacent decks. A boarding-bay shaft and seeded regional shafts SHALL connect stairs and ramps to existing room routes without severing horizontal exits, lift access or mission sites.

#### Scenario: Generate neighboring decks independently
- **WHEN** adjacent decks of a shaft are generated in either order
- **THEN** their stair/ramp landings align and players can traverse the seam in both directions.

### Requirement: Bounded traversal geometry
Traversal geometry SHALL remain within explicit per-chunk surface limits and the existing resident-chunk and generation/upload budgets. Pending destinations and phase-locked decks SHALL remain physically inaccessible through stairs and ramps.

#### Scenario: Walk towards a pending deck
- **WHEN** a player reaches the end of a shaft whose next deck is still generating
- **THEN** the player stays on supported geometry until the destination is ready.

### Requirement: Stable old architecture
Adding generator 9 SHALL preserve generator 3–8 room plans, solid geometry and encounter identities. Loading an existing expedition SHALL not insert shafts or relocate its characters.

#### Scenario: Resume an older ship
- **WHEN** a saved generator-8 expedition resumes
- **THEN** its rooms and skiff remain unchanged and its earned character build is retained.
