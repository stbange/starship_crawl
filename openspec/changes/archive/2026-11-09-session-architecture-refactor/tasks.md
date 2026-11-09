## 1. R5 module boundaries
- [x] 1.1 Extract application input, actions, audio, telemetry, inspection and Quest modules with private interfaces; verify native/web/Quest compilation and document ownership.
- [x] 1.2 Extract renderer mesh, world/actors and HUD modules with private interfaces; verify native render smoke and browser captures.

## 2. R1 commands
- [x] 2.1 Add typed commands, player validation and named action metadata; test invalid inputs and equivalent gameplay transitions.
- [x] 2.2 Route keyboard, controller, touch and Quest gameplay actions through the common dispatcher; verify packaged platform journeys and document the adapter contract.

## 3. R2 state ownership
- [x] 3.1 Separate local view state from player simulation/progression and add addressed presentation events; test view isolation and save/pref preservation.

## 4. R3 clock and residency
- [x] 4.1 Add fixed-step headless sessions and independent simulation/render budgets; test replay across frame schedules and preserve generation fingerprints.
- [x] 4.2 Add stable actor handles across room compaction and disposal; test compaction, stale handles and world replacement.

## 5. R4 identity and capacity
- [x] 5.1 Separate player identities, view bindings, core capacity and legacy save counts; test four-player headless movement, progression and distant streaming plus default split-screen behavior.

## 6. R6 persistence
- [x] 6.1 Split checkpoint I/O, codecs, validation and restoration modules; verify all historical fixtures, atomic-write faults and default save parity.
- [x] 6.2 Add bounded portable live snapshots with transactional validation; test active combat/matrix state, malformed input, allocation failure and expanded-party round trips.

## 7. R7 architecture validation
- [x] 7.1 Consolidate CMake variant configuration and source coverage inventory; verify core/Quest/expanded-party/fault builds and unchanged 95% per-source coverage.
- [x] 7.2 Document the new architecture, retain completed local validation results, and require complete native, gameplay, sanitizer, browser, Quest and specification checks before publication.

## Workflow follow-up
- Archive the completed OpenSpec change and validate canonical specifications.
- Push the validated change to main and monitor the complete CI workflow through Pages deployment.
