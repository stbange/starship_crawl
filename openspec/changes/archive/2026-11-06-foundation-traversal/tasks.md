# Tasks

## 1. Controls and platform foundation

- [x] 1.1 Add bounded persistent per-player input/render preferences and native/browser controls; verify settings, reset, reload and expedition independence with platform and browser tests, and document the controls.
- [x] 1.2 Preserve controller owners across disconnects and clear stale input at lifecycle boundaries; verify actual SDL virtual-device input on native targets plus browser focus, touch cancellation and orientation tests, and provide physical-device instructions.

## 2. Continuous architecture

- [x] 2.1 Retain a generator-8 compatibility fixture and implement generator-9 aligned stair/ramp shafts with shared render/collision/ray surfaces; verify old geometry, seeded alignment, budget limits and representative room routes with unit tests and render captures.
- [x] 2.2 Implement step climbing, slope descent and head-clearance handling; verify real-action stair/ramp ascent and descent, negative-deck seams, crouching, obstacles, phase barriers and pending frontiers, and document traversal controls.

## 3. Animated deck travel

- [x] 3.1 Add per-player visible grav-lift journeys, safe checkpoints, interaction guards and interrupted-ride snapshots; verify intermediate positions, co-op independence, rigs, death/join/leave, unloaded/locked destinations and save/import behavior with unit and gameplay tests.
- [x] 3.2 Add lift/shaft presentation, guidance and inspection scenes; verify packaged browser controls, save/reload, touch and split-screen travel plus native captures, and update arrival/traversal documentation.

## 4. Integration

- [x] 4.1 Run strict specifications, all native unit/gameplay tests, >=95% core unit coverage per file and overall, sanitizers, packaged browser suites and Windows compilation/input CI; retain results and clearly label unperformed physical-device checks.
- [x] 4.2 Update the implementation plan and current version documentation, verify old expeditions remain compatible, and commit/push the completed changes directly to main with automatic Pages workflow verification.

## Workflow follow-up

- Sync and archive the completed OpenSpec change after validation, retaining the physical-device checklist as unverified user playtesting rather than a claimed result.
