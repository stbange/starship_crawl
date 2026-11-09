## MODIFIED Requirements

### Requirement: Bounded streaming
Active-session streaming SHALL generate at most two chunks per fixed simulation tick.
Rendering SHALL upload at most two chunks per displayed frame independently of that
simulation budget. The default two-character core SHALL retain at most 192 resident
chunks; supported expanded-party cores SHALL retain at most 96 chunks per configured
party slot. Paused previews MAY populate geometry without advancing gameplay.

#### Scenario: Separated players
- **WHEN** active players explore distant regions
- **THEN** each neighborhood remains traversable within the configured shared cache budget.

#### Scenario: Incomplete frontier
- **WHEN** a requested room footprint is unfinished
- **THEN** collision prevents entering its missing floor until the complete footprint is ready.

#### Scenario: Different rendering rates
- **WHEN** equivalent active simulation ticks run at different display rates
- **THEN** simulation generation work agrees and GPU uploads remain separately bounded.
