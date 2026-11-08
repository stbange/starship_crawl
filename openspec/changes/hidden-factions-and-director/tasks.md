# Tasks

Implementation and verification are in progress. Each group owns
its tests and documentation. Use the existing unit-only >=95% overall/per-file
coverage gate without exclusions. Reference [design.md](design.md) for acceptance
experiences and the final comparison protocol.

## 1. M1 — Hidden foundations

- [x] 1.1 Add an explicitly versioned simulation state and legacy dispatch path; verify all existing migration fixtures and rejection of unknown versions.
- [x] 1.2 Seed headquarters, resource sites, initial relationships and doctrine for the three powers; verify reproducibility and unchanged legacy geography across shuffled map/cache queries.
- [x] 1.3 Add the shared strategic clock, deterministic decision state, interest-region bookkeeping and private diagnostics; verify pause, suspension, shelter, exposed matrix bodies and separated co-op timing.
- [x] 1.4 Remove new-system court commissions, political tracking/maps and director seals/prompts from keyboard, controller and touch flows; verify normal story guidance and legacy court controls in packaged browser tests.
- [x] 1.5 Present distinct local faction patrols and unprompted rival encounters using existing models/markings; capture a real rendered encounter and verify neutral-terminal access and NPC targeting.
- [x] 1.6 Save/load the foundation state transactionally; verify round trips, corrupted references and allocation failure without changing the active expedition.
- [x] 1.7 Update current-behaviour docs and milestone status, then run strict specs, native unit/gameplay, coverage and relevant packaged browser/render checks; retain results and actual shipped versions.

## 2. M2 — Mobile forces

- [x] 2.1 Introduce stable force/member identities separate from static room kill masks; verify single ownership, casualty retention and nonduplicated reward attribution.
- [x] 2.2 Build strategic paths from connected room/region portals and local movement waypoints; verify corridors, narrow rooms, multi-deck routes, phase barriers and failure-to-route waiting/withdrawal.
- [x] 2.3 Add formation travel through doors, stairs, ramps and eligible lifts with clearance and landing reservations; verify that player/co-op traffic remains traversable and stalled troops do not teleport.
- [x] 2.4 Implement patrol, raid, escort, reinforcement, occupation and retreat orders; verify a force follows and completes each order through actual movement and combat.
- [x] 2.5 Implement exclusive abstract/physical simulation handoff across both players' areas; verify no double damage, revived members or duplication under streaming churn, projectiles and co-op separation.
- [x] 2.6 Enforce shared force/work/room limits and fair deferred arrivals; stress-test capacity and verify visible actors are never evicted to make space.
- [x] 2.7 Persist mid-route and mid-combat forces with schema/version updates; verify resume, repeated visits and malformed duplicate-ownership rejection.
- [x] 2.8 Add an ordinary-input journey that follows, interrupts and revisits a cross-sector raid; retain saves/rendered evidence, update docs and pass native/coverage/browser checks relevant to the new actors.

## 3. M3 — Autonomous politics

- [x] 3.1 Add power, biomass and fabrication-resource production, transport, loss and recruitment costs; verify conservation, capacity and shortage-driven decisions over many strategic ticks.
- [x] 3.2 Score reachable patrol, raid, convoy, reinforcement and defence objectives without player commissions; verify recorded causes and autonomous activity with director stepping disabled in the harness.
- [x] 3.3 Implement Divided Crown competing orders, Molting Court protection cycles and Echo relay coordination; verify distinct deployments from equivalent starting situations and capture each behaviour.
- [x] 3.4 Add war, truce, tribute, symbiosis, succession and occupation transitions with commitment/cooldown rules; verify relationships, resource effects and no rapid state oscillation.
- [x] 3.5 Tie destroyed convoys, captured sites and lost relays to subsequent decisions; verify changed deployments and durable outcomes after leaving the area and saving/resuming.
- [x] 3.6 Replace immediate permanent offense for new ships with warning, local retaliation, witnessed/reported escalation and disengagement; verify isolated hits, lethal/repeated attacks, beams, splash, matrix allies and per-player co-op attribution.
- [x] 3.7 Enforce shelter, extraction, guardian and required-interaction protection at campaign planning and combat boundaries; verify autonomous fights cannot unlock phases, lose required goals or grant NPC-only kill credit.
- [x] 3.8 Add autonomous-war and player-interference journeys, save regressions and observable warnings; update docs and pass strict specs, native gameplay/unit, coverage and relevant browser checks.

## 4. M4 — Invisible director

- [x] 4.1 Track per-player pressure, combat duration, exploration, resource availability and repeated/avoided content; verify only gameplay events affect observations and isolated strong/weak co-op players remain distinguishable.
- [x] 4.2 Implement build-up, pressure, recovery, discovery and no-intervention decisions with shared budgets, cooldowns and longer memory; verify minimum recovery and suppression of repetitive event alternation.
- [x] 4.3 Add intervention reservations and proposed/committed/completed/cancelled states; verify atomic costs/effects, expiry, save/resume and exactly-once cancellation under allocation failures.
- [x] 4.4 Implement additions such as a funded scout patrol, plausible stash and eligible optional nest; verify causality, physical placement, identity limits and both-camera observation rejection.
- [x] 4.5 Implement rerouting, reinforcement delays, unseen encounter substitutions and eligible lighting/weather changes; verify protected objectives, static geometry and committed enemy stats remain stable.
- [x] 4.6 Implement removal/defer actions for proposed ambushes, unseen optional content and upcoming hazards, plus physical withdrawals for committed forces; verify no visible disappearance, fake casualty, free reward or released claim.
- [x] 4.7 Revalidate interventions against both players' cameras, known facts, combat/projectiles, shelter/matrix state and reserved progression; verify a recovering player's partner cannot lose an observed battle.
- [x] 4.8 Add deterministic journey comparisons demonstrating an actual intervention effect and periods of deliberate quiet; update docs/diagnostics and pass strict specs, unit/gameplay, coverage, persistence and packaged browser checks without player-facing director UI.

## 5. M5 — Consequences, atmosphere and rewards

- [x] 5.1 Apply bounded occupation overlays for damaged supplies, discarded shells, corpses, guards, barricades and abandoned positions; verify collision clearance, persistent facts and no resurrection on region eviction.
- [x] 5.2 Connect occupation and resources to light/power, infestation and compatible existing atmosphere/sprites; verify shared causal state and capture before/after views of the same station.
- [x] 5.3 Add travel, warning, engagement and retreat visual/audio cues that do not depend on colour alone; verify actual rendered/audio output in dark rooms and separated views.
- [x] 5.4 Connect relevant existing power, hacking and convoy interactions to faction events; verify each action changes the correct faction/site once without adding diplomacy menus or moving story destinations.
- [x] 5.5 Add bounded ordinary battlefield salvage and challenge participation rules for exceptional rewards; verify NPC-only combat, last-hit exploits, death/revisit and late co-op joins cannot duplicate or bypass unlocks.
- [x] 5.6 Create linked environmental episodes such as convoy loss, a supply raid and later occupation; verify they emerge from shared events and can be bypassed without accepting a political quest.
- [x] 5.7 Run ordinary-input revisit/extraction journeys, strict specs, native/coverage, packaged browser and render checks; document current cues, legacy behaviour and retained screenshots with accurate provenance.

## 6. M6 — Integration, long runs and tuning

- [x] 6.1 Run twelve fresh-character 60-minute simulated expeditions across seeds, directions and input/build profiles; retain all outcomes, including failed routes, with resource, movement, casualty, progression and pacing observations.
- [x] 6.2 Repeat four selected runs without director stepping in the test harness; compare pressure/recovery, repetition and encountered faction events while preserving the same production faction/gameplay rules.
- [x] 6.3 Stress separated co-op, streaming boundaries, repeated shelter/matrix transitions, region revisits and save/import cycles; verify force/resource/claim conservation and protected progress at every transition.
- [x] 6.4 Measure strategic/tactical update percentiles, actor/work limits, memory and save growth in native and packaged web builds; tune the proposed caps and publish measurements without claiming untested mobile performance.
- [x] 6.5 Tune encounter cadence, government priorities, hostility grace periods and evidence density from the recorded comparisons; rerun affected checks and document changes without altering the separate character-growth plan.
- [x] 6.6 Prepare repeatable human solo/split-screen and physical-device checkpoints for neutrality, surprise, recovery and readability; record performed sessions and explicitly list unavailable/pending checks.
- [x] 6.7 Run the full strict-spec, native unit/gameplay, >=95% unit-only coverage, sanitizer, packaged browser, render and Linux/Windows/web build gates; retain captures of a convoy, battle, retreat, changed site and quiet recovery.
- [ ] 6.8 Update the roadmap, player docs, compatibility notes and review with final implemented versions and limitations; push to main and verify the complete CI/Pages deployment before marking publication complete.

## Workflow follow-up

- Synchronize and archive the change after its implementation and verification
  tasks are complete; keep historical generator-13 specifications and fixtures
  available for compatibility review.
