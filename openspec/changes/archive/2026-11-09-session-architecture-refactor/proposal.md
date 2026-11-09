## Why

Gameplay is already headless and fixed-step, but input dispatch, view state, room residency and portable persistence still assume one local application. The approved R1–R7 review calls for explicit boundaries before internet multiplayer is added.

## What Changes

- Introduce typed commands, validation and replayable execution shared by all input adapters.
- Separate character/simulation data from local view state and provide addressed presentation events.
- Extract a session clock, simulation streaming policy and stable entity handles.
- Separate player identity, local-view mapping and configurable bounded party capacity.
- Extract application, renderer and persistence modules with explicit private interfaces.
- Add a bounded portable live-state snapshot distinct from checkpoint saves.
- Consolidate build configuration and test the new contracts, including replay, malformed snapshots, player mapping, streaming and compatibility.

## Capabilities

### New Capabilities
- `simulation-sessions`: commands, state ownership, residency, identity, snapshots and module boundaries.

### Modified Capabilities

## Impact

The C core, SDL/browser/Quest adapters, renderer, save implementation and CMake configuration change. Existing ordinary and Quest targets remain isolated. Existing generator 3–15 geometry and save formats 1–16 remain compatible; the shipped solo/two-view experience remains unchanged. Networking, dedicated servers, hosting and online gameplay rules are later work. Validation includes unit-only 95% overall and per-source coverage, all gameplay cases, fault injection/sanitizers, geometry/save fixtures, actual packaged browser suites, Quest parity/lifecycle and strict specification validation.
