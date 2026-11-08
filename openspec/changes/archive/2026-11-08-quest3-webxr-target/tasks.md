# Tasks

## 1. Separate build and input foundations

- [x] 1.1 Add opt-in Quest application/core and headless Quest-test targets with target-local definitions; verify default Linux/Windows/web configurations retain their existing target flags, dependencies and artifact names, and document the new build command.
- [x] 1.2 Implement validated pose transforms, calibration, bounded leaning and snap-turn latches; cover their geometry and invalid/tracking-loss cases with native adapter tests.
- [x] 1.3 Add Quest-only controller-ray combat input preserving ordinary weapon rules; test independent aim, beams/projectiles, blocked and invalid origins, energy/cooldown, and unchanged zero-override behavior.

## 2. Immersive presentation and lifecycle

- [x] 2.1 Add the separate launch shell and WebXR bridge with capability/permission handling and XR-owned frame scheduling; verify success, denial, unsupported browser, end/re-entry and single simulation updates with test-side emulated devices.
- [x] 2.2 Render tracked stereo eyes, controller weapons/flashlight, wrist information and floating controller-operated panels; retain actual WebGL captures and verify distinct eye output, independent aiming and menu access to ordinary progression/story/navigation actions.
- [x] 2.3 Integrate stick movement, crouch/jump/lifts, seated/standing recalibration, optional vignette and transition fades; exercise approach/boarding and representative shelter, matrix and vehicle transitions without altering earned capabilities.
- [x] 2.4 Clear and pause input on controller, tracking, visibility and context loss; browser-test deliberate recovery, no accumulated simulation time, no stale firing and a usable reload path for graphics loss.

## 3. Storage and distribution

- [x] 3.1 Add separate Quest persistence/settings and compatible import/export; verify same-origin isolation, reload, supported legacy/current saves and transactional rejection without touching ordinary storage.
- [x] 3.2 Package the Quest application independently and add its own CI artifact/test job and Pages subdirectory; verify missing/invalid artifacts fail clearly and the ordinary root package is preserved.
- [x] 3.3 Document controller bindings, launch/build instructions, actual limitations and a physical Quest 3 checklist; verify every documented control has a production implementation and distinguish emulation from hardware testing.

## 4. Integration and release

- [x] 4.1 Run strict specs, native unit/gameplay and unit-only >=95% overall/per-file coverage gates, focused Quest sanitizers/tests and Linux/Windows/ordinary-web builds; retain results without coverage exclusions.
- [x] 4.2 Run the packaged ordinary browser suites and Quest WebXR/device-emulation journeys, retain screenshots and storage/target-isolation evidence, and record unperformed hardware checks explicitly.
- [x] 4.3 Update the roadmap and release record, push validated work to main and verify the complete CI/Pages publication with both the ordinary game and Quest entry point. Verified by successful run 42520975264 at f7ece01, published 2026-11-08 21:54 Europe/Warsaw.

## Workflow follow-up

- Synchronize specifications and archive after implementation and verification.
