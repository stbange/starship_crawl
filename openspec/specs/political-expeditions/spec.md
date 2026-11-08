# political-expeditions Specification

## Purpose
Define seeded alien governments, playable political settlements and a bounded,
persistent encounter director that respects expedition history and compatibility.

## Requirements

### Requirement: Seeded political geography
New-system expeditions SHALL contain three seeded alien governments with
multi-sector territories, resource sites, headquarters, wilderness and initial
relationships. Initial geography SHALL be stable; subsequent occupation SHALL
follow recorded events. Map, rendering and cache queries SHALL NOT discover
sites or advance politics. Generator-13 expeditions SHALL retain their geography.

#### Scenario: Revisit a country
- **WHEN** players approach the same territory through different streaming orders with the same seed and simulation history
- **THEN** its government, sites, initial borders and recorded changes agree.

#### Scenario: Discover an occupied site
- **WHEN** a victorious force occupies a resource site and the player later visits
- **THEN** its inhabitants and physical evidence reflect the recorded occupation without a political map revealing the event in advance.

### Requirement: Playable disputes and consequences
New-system disputes SHALL progress through faction actions without player
commissions. Players SHALL influence them through combat, interference with
supplies and existing ship systems. Headquarters SHALL remain physical places
to explore. Required progression and confirmed rewards SHALL survive faction
conflict. Generator-13 courts SHALL retain their commissions, settlement choices
and once-only rewards.

#### Scenario: Settle a dispute
- **WHEN** a commissioned player in a generator-13 expedition secures the conduit and defeats its royal guard
- **THEN** the pledged settlement takes effect and survives save/resume.

#### Scenario: Ignore a war
- **WHEN** rival factions can reach a contested supply site and the player continues exploring elsewhere during active play
- **THEN** the factions can dispatch, fight, retreat or occupy without a player interaction opening the encounter.

#### Scenario: Disrupt a convoy
- **WHEN** the player destroys supplies carried by a reinforcement convoy
- **THEN** the receiving faction loses those supplies and its subsequent activity reflects the loss once.

### Requirement: Readable political interaction
New-system faction identities and intentions SHALL be expressed through bodies,
markings, sounds and actions. Normal play SHALL NOT expose relationship tables,
reputation scores, political tracking, court commissions or director prompts.
Neutral patrols SHALL leave peaceful players and story terminals alone. Hostile
patrols SHALL contest terminals under existing proximity/visibility rules.
Generator-13 controls and political presentation SHALL remain available.

#### Scenario: Decline an audience
- **WHEN** a generator-13 player closes an audience without choosing
- **THEN** no pledge or relationship changes.

#### Scenario: Investigate beside a peaceful patrol
- **WHEN** a story terminal has a visible neutral patrol nearby and no hostile threats
- **THEN** the player can investigate without attacking that patrol.

#### Scenario: Use new-system menus
- **WHEN** a new-system player uses keyboard, controller or touch menus, maps and journals
- **THEN** ordinary story guidance remains usable without revealing hidden diplomacy, army orders or director decisions.

### Requirement: Bounded causal director
The new-system director SHALL choose build-up, pressure, recovery and discovery
using player activity, resources, recent threats, repetition memory and faction
opportunities. It SHALL add, modify, defer or remove eligible upcoming content
within budgets, including choosing no intervention. Its decisions SHALL remain
hidden during normal play and preserve continuity. Generator-13 ships SHALL
retain their saved physical dispatch behaviour.

#### Scenario: Pause a dispatch
- **WHEN** a generator-13 expedition pauses or both players enter shelter or matrix
- **THEN** its dispatch clock and budget stop advancing.

#### Scenario: Recover after a difficult encounter
- **WHEN** an outside player has sustained high pressure and a reinforcement has not entered an observed encounter
- **THEN** the director can defer or divert that reinforcement and select a plausible recovery opportunity without displaying a timed dispatch or changing enemies already engaged.

#### Scenario: Break repetition
- **WHEN** recent encounters repeatedly use the same faction and attack pattern
- **THEN** subsequent eligible interventions favour a different experience or quiet time rather than merely alternating two repeated offers.

### Requirement: Expedition compatibility
The new system SHALL be explicitly versioned and enabled for new ships. Supported
earlier expeditions SHALL retain their architecture, progression and political
rules, including generator-13 courts and dispatches. Malformed state SHALL be
rejected transactionally. A new ship SHALL reset factions, campaigns and the
director alongside both fresh characters.

#### Scenario: Resume before politics
- **WHEN** a format-13 generator-12 expedition loads
- **THEN** its architecture, population and progression retain their prior rules.

#### Scenario: Resume a court commission
- **WHEN** a generator-13 expedition with an unfinished commission resumes
- **THEN** its pledge, tracking, settlement options and pending dispatch remain playable without acquiring autonomous armies.

### Requirement: Autonomous faction agendas
Factions SHALL select objectives from resources, reachable sites, losses,
relationships and government doctrine. Wars, truces, tribute, symbiosis and
succession SHALL have persistent causes and consequences. Resource costs,
recruitment limits and decision cooldowns SHALL constrain activity; factions
SHALL NOT exist solely to attack the player.

#### Scenario: Raid for a shortage
- **WHEN** a faction lacks a resource and has a reachable rival depot and an available force
- **THEN** it can launch a raid tied to that shortage and account for the force and supply cost.

#### Scenario: Different alien governments
- **WHEN** different governments face the same resource and military situation
- **THEN** doctrine can produce different deployments or responses, such as conflicting royal orders, protection of a molting ruler or coordinated relay defence.

### Requirement: Persistent mobile forces
Forces SHALL have persistent identities, allegiance, strength, supplies, orders
and routes. They SHALL travel through valid connections, fight other factions,
escort, reinforce, occupy and retreat. Nearby movement and combat SHALL use
physical actors. Distant resolution SHALL preserve force identity and outcomes
without counting the same combat twice or reviving losses during streaming.

#### Scenario: Follow a column
- **WHEN** a player follows a force across rooms, a deck connection and a sector boundary
- **THEN** the same surviving force travels through traversable entrances towards its objective without teleporting through walls or resetting casualties.

#### Scenario: Observe a distant battle
- **WHEN** either player approaches an abstractly simulated conflict
- **THEN** physical combat takes over once using the remaining participants, with no simultaneous abstract damage or duplicated troops.

#### Scenario: Leave a damaged squad behind
- **WHEN** a damaged squad leaves both players' active areas and later returns
- **THEN** its surviving strength and resolved losses remain consistent with its intervening orders and recorded events.

### Requirement: Deliberate player hostility
New-system factions SHALL distinguish incidental damage, local self-defence and
deliberate repeated aggression. An isolated nonlethal stray hit SHALL NOT create
permanent ship-wide hostility. Escalation SHALL have readable physical warnings
and bounded memory. Sufficient disengagement from an incidental confrontation
SHALL permit de-escalation. Co-op offenses SHALL be attributed to their source.

#### Scenario: Accidental crossfire
- **WHEN** a previously peaceful player grazes a neutral patrol once during a battle and stops attacking it
- **THEN** the patrol warns or responds locally, and the incident can de-escalate without a diplomacy menu or permanent hostility from every patrol.

#### Scenario: Repeated deliberate aggression
- **WHEN** a player continues attacking after warnings or repeatedly kills faction members
- **THEN** surviving witnesses or functioning communications can propagate a lasting threat response with visible behavioural consequences.

### Requirement: Causal interventions and continuity
Director interventions SHALL declare prerequisites, location, pressure cost,
effects, expiry and cancellation rules. Additions/removals SHALL respect both
players' observation and combat commitments. Established geometry, discovered
goals, confirmed kills, rewards and enemy statistics SHALL remain stable.
Cancelling an unobserved proposal SHALL release reservations without reward or
casualty effects; committed changes SHALL use plausible world actions.

#### Scenario: Remove an upcoming ambush
- **WHEN** an optional ambush is proposed but unobserved and the nearby player needs recovery
- **THEN** the director can cancel it without deleting an engaged enemy, granting kills or spending its budget twice.

#### Scenario: Preserve a partner's battle
- **WHEN** one co-op player is recovering while the other can observe a proposed intervention's location
- **THEN** the director treats that location as observed and cannot replace or remove its visible combatants.

#### Scenario: Change an unexplored encounter
- **WHEN** an unobserved optional room has an eligible generated encounter
- **THEN** the director can replace its composition or defer its ambient hazard while preserving connections, reserved objectives and local resource limits.

### Requirement: Shared simulation time and bounded work
Faction decisions SHALL use deterministic active-game time and a bounded
simulation area covering both players, with coarse progress for retained distant
campaigns. Pause, suspension and all-player shelter SHALL freeze faction time;
matrix play SHALL continue time for exposed bodies. The director SHALL stop
issuing physical encounters when nobody is exploring physically. Map browsing,
rendering and streaming order SHALL NOT advance either system.

#### Scenario: Return after closing the game
- **WHEN** a player resumes after hours away or a suspended browser tab resumes
- **THEN** no wall-clock war catch-up or accumulated encounter burst occurs.

#### Scenario: One partner stays outside
- **WHEN** one player enters shelter and the other remains in the ship
- **THEN** the shared faction simulation continues once and the shelter remains safe.

#### Scenario: Dive into the matrix
- **WHEN** the only player hacks from an exposed physical body
- **THEN** existing world activity continues and can threaten that body, while the director does not add a new physical encounter just because the avatar is elsewhere.

### Requirement: Observable consequences and earned progression
Conflict SHALL leave bounded persistent occupation and evidence changes such as
damaged supplies, altered lighting, nests or abandoned positions. Players SHALL
be able to bypass or exploit faction conflict. NPC victories SHALL NOT award
player kills, experience, guardian unlocks or exclusive equipment. Optional
high-value rewards SHALL require attributable player challenge participation
and remain exactly-once across travel, co-op and saves.

#### Scenario: Revisit a captured station
- **WHEN** a player revisits a station after a recorded capture
- **THEN** its new occupants and retained environmental evidence agree with the outcome while known entrances and required story interactions remain accessible.

#### Scenario: Watch others fight
- **WHEN** two NPC factions defeat each other without player participation
- **THEN** the player can exploit the opening and find bounded ordinary salvage, but gains no kill experience, phase unlock or exclusive equipment claim.

#### Scenario: Protect a guardian route
- **WHEN** a campaign or director intervention would occupy a required guardian approach or extraction route
- **THEN** it cannot invalidate the required encounter, make an essential interaction permanently unavailable or grant progression on the player's behalf.

### Requirement: Private diagnostic evidence
Development diagnostics SHALL record faction causes, force transitions,
director choices and rejected interventions without adding normal-play UI.
Automated reviews SHALL distinguish strategic outcomes, rendered observations
and human judgements, and expose resource limits and simulation failures.

#### Scenario: Explain an encounter during testing
- **WHEN** a developer inspects an encountered raid's diagnostic history
- **THEN** its source faction, objective, travel, losses and any director intervention are traceable while the player's HUD contains none of those internal decisions.
