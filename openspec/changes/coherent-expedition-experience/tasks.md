# Tasks

Implementation is planned; no task is completed by the planning commit. Execute
E1–E8 in order, using the [design](design.md) and [delta specifications](specs/).
The remaining open questions are tuning values within the selected mechanics.

Each milestone gate includes `npm run spec:validate`, relevant unit CTest,
separate gameplay CTest and `python3 scripts/check-coverage.py` with **>=95%
overall and per executable core source**, without excluding uncovered lines.
Run native GL, packaged browser and isolated Quest checks for affected rendering,
input or shared behavior, following [TESTING.md](../../../TESTING.md). Keep
physical device and human results distinct from simulation. Version new state
and update save/snapshot validation alongside each feature that introduces it.

## 1. E1 — Combat and build identity

- [x] 1.1 Capture matched starter, middle and late earned-build baselines through existing command/session APIs; retain seeds, build/resource curves, encounter traces and guardian timings in a reproducible review artifact.
- [x] 1.2 Introduce the new expedition gameplay revision and branch ownership/equip state in campaign, save and live-snapshot boundaries; verify old fixtures retain their rules, new-ship reset clears both profiles, and corrupt or unknown state fails transactionally.
- [x] 1.3 Tune point supply, marginal growth and declared level-up recovery; remove purchase-triggered full recovery in the new rules, and verify resource bounds, resuming old expeditions and matched progression curves.
- [x] 1.4 Implement the prerequisite-gated beam/flare relay prototype; test bounded target selection, walls, finite energy/effects, no recursive relay and observable native/browser beam paths.
- [x] 1.5 Implement the cloak-break decoy prototype; test lifetime, enemy response, player attribution, no decoy loot/rewards and consistent pause/snapshot behavior.
- [x] 1.6 Implement absorbed-attack mech charge; test hostile-source eligibility, charge caps, discharge, rig packing, shelter isolation and earned-tool locks.
- [x] 1.7 Add the three encounter recipes with role compatibility and staggered commitments; verify actor budgets, readable support disruption, valid movement responses and starter/middle/late victories without stat changes during engagement.
- [x] 1.8 Add branch previews, equip/refit actions and useful duplicate/deferred reward handling through shared commands and all existing input adapters; verify mutual exclusion, invalid-action rejection and per-player exactly-once claims.
- [x] 1.9 Run the E1 gate, onboarding and save/snapshot regressions; update implemented behavior docs and publish rendered comparison checkpoints showing three distinct build actions and the unchanged fresh-character opening.

## 2. E2 — Memorable guardians

- [x] 2.1 Add typed guardian subsystem and stage state with clear attempt-local versus durable preparation ownership; verify validation, checkpoint staging resets and exact active live-snapshot round trips.
- [x] 2.2 Implement the suspended-machine pattern with at least two actionable assemblies and two attack stages; verify destroying each system changes the declared attack and strong legal damage is accepted without hidden scaling.
- [x] 2.3 Implement the infested-relay pattern with a different movement/priority problem; verify its warnings, support openings and starter-kit path in a reproducible encounter fixture.
- [x] 2.4 Add staged arena machinery, collision-safe transitions and a visible victory breach; test every occupied platform/route transition for solo, co-op and earned rigs, including interruption and failed readiness.
- [x] 2.5 Bind physical preparation and an optional earned matrix alternative to the same subsystem facts; test prepared and unprepared victories, no early matrix access and no duplicated advantage or reward.
- [x] 2.6 Exercise defeat, retreat, all-player shelter, one partner remaining, checkpoint resume and live-snapshot continuation; verify attempt reset, preparation retention and participation/claim accounting.
- [x] 2.7 Run the E2 gate and render/browser checks; document the two fights and retain warning, system-destruction and breach captures for human readability review.

## 3. E3 — A coherent district

- [ ] 3.1 Add deterministic district identity and route/content contracts to regional planning, with explicit generator compatibility; verify native/WASM fingerprints, reverse load order, reciprocal boundaries and unchanged old generators.
- [ ] 3.2 Add the awakening-machine ship condition and durable power transitions; verify matching generation, enemy responses and reserved mission opportunities, with cues before harmful activation.
- [ ] 3.3 Implement bounded conductive surfaces and machine-routing interactions shared by physical controls, weapons and optional matrix effects; test affected actors, warning duration, disconnected surfaces and a basic avoidance route.
- [ ] 3.4 Build Drowned Foundry geometry with a reactor vista, scale cues, raised/lower approaches, preparation site and return shortcut; verify collision, basic-route reachability and valid alternate earned-tool routes across a retained seed set.
- [ ] 3.5 Connect encounter recipes, a guardian, condition state and persistent aftermath to the district contract; verify bypasses, victory and backtracking after cache eviction and save/resume.
- [ ] 3.6 Add reservation/fallback and bounded-work cases; prove maximum room actors, chunk lights/dressing/sprites, generation/upload limits and objective access with incompatible optional content.
- [ ] 3.7 Run the E3 gate, including a fresh-character journey and separately labelled earned-build fixture; update world-generation docs and capture the entrance vista, narrow route, warning surface and reopened shortcut.

## 4. E4 — Inhabitants with a continuing story

- [ ] 4.1 Extend existing force orders into supply/siege and factory/reinforcement campaign patterns; verify resource costs, routed travel, persistent identities and near/far handoff without duplicated combat.
- [ ] 4.2 Connect supply interruption and occupation to later orders and physical evidence; run paired intervene/ignore paths proving a visible causal difference without exposing politics or awarding NPC-only progression.
- [ ] 4.3 Add stable recurring-individual identity, knowledge, location and fate, beginning with two roles; verify repeated meetings, plausible transitions, death/missed-meeting fallbacks and persistence.
- [ ] 4.4 Bind district choices and known costs to the rescue and salvage slice variants using existing story interactions; verify the direct route, optional preparation and alternate information sources all preserve a completable expedition.
- [ ] 4.5 Render recognizable individual markers, local aftermath and contextual responses; verify guidance-off navigation, shared co-op facts, maps/journals and keyboard/controller/touch interactions without a political-management interface.
- [ ] 4.6 Run the E4 gate and long-distance revisit campaign; update story/faction documentation and retain a short illustrated account tracing one earlier decision to a later meeting and changed occupation.

## 5. E5 — A useful home

- [ ] 5.1 Implement three optional salvage-funded meals with one active benefit per character; verify costs, replacement, duration, save/resume and free existing recovery at zero salvage.
- [ ] 5.2 Implement the shared refit-bench project and personal safe attribute redistribution; verify total spent points are conserved, capacities remain valid, eligibility is respected and failed/duplicate purchases charge nothing.
- [ ] 5.3 Implement bounded flare-calibration and observed-condition research projects; test prerequisite locks, limited preparation charges, recurring salvage usefulness and personal/shared purchase accounting.
- [ ] 5.4 Add up to six fixed keepsake displays derived from real district outcomes, retaining excess discoveries in the journal; verify collision/interaction access, no duplicate reward and fresh-ship clearing.
- [ ] 5.5 Exercise shelter entry/exit, two occupants, a partner outside, matrix access, save/import and paused clocks with the new facilities; verify sanctuary safety and exact return anchors.
- [ ] 5.6 Run the E5 gate and native/browser facility checks; update shelter controls and preparation documentation, with a zero-resource recovery checkpoint and a late-expedition useful-salvage checkpoint.

## 6. E6 — An expedition rhythm

- [ ] 6.1 Add bounded persistent sequence state and reservations to the existing hidden director; verify at most two active sequences, one shared pressure budget, deterministic resume and valid cancellation accounting.
- [ ] 6.2 Implement stalking-hunter and approaching-siege sequences using established actors and campaigns; test early threat death, ignored clues, player departure and retreat without resurrection or forced replacement.
- [ ] 6.3 Implement awakening-machine and recovery/discovery sequences; test observed-state protection, genuine quiet windows and progression based on exploration rather than cache generation or map browsing.
- [ ] 6.4 Bind physical sound/light cues and restrained ambient intensity to sequence facts; inspect native/browser captures for identifiable anticipation without visible director state or compulsory dialogue.
- [ ] 6.5 Stress both players' observations, separated exploration, shelter, matrix, pause/suspension and snapshot restoration; verify no unseen-to-one-player rewrite of the other's active scene and no resume burst.
- [ ] 6.6 Run paired director-on/off gameplay comparisons and the E6 gate; publish the complete Drowned Foundry slice report, current human-review findings or explicitly pending sessions, and tune demonstrated contradictions before breadth expansion.

## 7. E7 — Ships worth replaying

- [ ] 7.1 Add predictable freeze/vent cycles and conduit-linked infestation as the other two dominant conditions; verify each rule's physical cues, basic counterplay, persistence and effect on enemies/rewards/motivation.
- [ ] 7.2 Add the frozen luxury terrace, command reliquary and infested transit garden district families; verify landmarks, routes, local stories and shortcuts against the same bounded district contracts.
- [ ] 7.3 Complete four specialization families with the eight branches specified in the design; verify each branch's exclusive effects, prerequisite gates, limits and different useful action in matched earned-build fixtures.
- [ ] 7.4 Expand to six encounter recipes and four guardian patterns across four phases and alternative sites; test distinct responses, starter-route viability, optional preparation and no mandatory randomized tool dependency.
- [ ] 7.5 Expand to six recurring roles and bind all four motivations to compatible districts and conditions; verify selection of two or three individuals, established alternatives and coherent consequences through extraction.
- [ ] 7.6 Build a documented compatibility matrix in which every condition supports multiple district families and every family multiple conditions; test seeded directional expressions, deterministic fallbacks and actual travel along both ends of each axis.
- [ ] 7.7 Exercise post-finale exploration, repeated commissions, capped build rewards, record compaction and new-ship reset; verify continuing useful choices without duplicated unique individuals, claims or victories.
- [ ] 7.8 Run the E7 gate and publish the implemented content inventory plus two contrasting ship/build walkthroughs; verify documentation counts match generated content and keep human judgments separate from automated coverage.

## 8. E8 — Balance and release evidence

- [ ] 8.1 Run at least twelve 60-minute simulated expeditions with the condition/motivation/direction/build matrix in the design, varied reward choices and paired controls; retain replay configuration, event plots, screenshots, simulated duration and wall time.
- [ ] 8.2 Compare resource/build curves, specialist responses, guardian stages, optional detours, quiet periods and navigation stalls against E1; tune demonstrated outliers and rerun the affected comparison seeds.
- [ ] 8.3 Conduct the guided/experienced two-ship human protocol and co-op/device checks where available; deliver observations on rule learning, upgrade use, causal understanding and retry appeal, clearly listing unavailable human or physical-device sessions.
- [ ] 8.4 Measure frame percentiles, streaming stalls, memory, effect/actor peaks and save growth on declared hardware or software rendering; resolve regressions and keep physical Quest performance/comfort status explicit.
- [ ] 8.5 Run full strict specifications, unit/gameplay CTest, sanitizers, >=95% per-file/overall core coverage, legacy-save/native-WASM checks, native builds, packaged browser suite and isolated Quest suite; retain green results without weakening thresholds or exclusions.
- [ ] 8.6 Update README, DEVELOPMENT_PLAN, behavior docs and a final implementation review with the ten-proposal mapping, current versions and remaining evidence gaps; push main and verify the complete GitHub Actions and Pages deployment.

## Workflow follow-up

- Archive the change after implementation and the project's review requirements
  are satisfied; do not archive or mark implementation complete for this plan.
- Revalidate active and archived OpenSpec documents after archiving.
