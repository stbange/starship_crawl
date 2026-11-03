## Why

The existing core suite measures about 93% line coverage and the successful solo campaign simulation is not yet a permanent regression test. The project needs executable specification checks, a reproducible 95% unit coverage gate, and automated gameplay verification before publishing.

## What Changes

- Initialize pinned OpenSpec tooling and Codex skills, and specify current shipped gameplay.
- Add component unit tests for uncovered behaviors, invalid inputs, and allocation/I/O failure safety.
- Require at least 95% unit line coverage overall and per executable core source file, with downloadable HTML/JSON/XML reports.
- Add a deterministic solo campaign test with movement, combat, lifts, all mission stages, the Warden, and extraction.
- Run specifications, unit coverage, native tests, and the browser game suite before Pages deployment.

## Capabilities

### New Capabilities
- `quality-gates`: Specification, unit coverage, and automated gameplay acceptance gates.

## Impact

CMake, tests, CI, development dependencies, and project documentation change. Shipped gameplay, save format 3, generator 3, existing character profiles, and expeditions remain compatible. Unit coverage covers all core logic and shared math; SDL/OpenGL/browser adapters have separate integration tests and are not part of the unit metric.
