## Context

The approved R1–R7 survey identified direct frontend mutation, mixed local/player state, frame-dependent streaming, fixed two-player storage and large translation units. Existing generation fingerprints, save fixtures and the ordinary/Quest parity test are the compatibility oracle.

## Goals / Non-Goals

Goals: executable command/session boundaries, separate local views, stable identity, bounded party generalization, independent streaming and rendering budgets, portable live snapshots and smaller modules. Preserve the shipped solo and optional two-view experience, save migrations and generator geometry. Internet transports, server hosting, matchmaking and online gameplay policy are later milestones.

## Decisions

- Typed gameplay commands use an explicit player identity and centralized argument/capability checks. Adapters translate physical controls and existing browser action numbers; presentation-only operations stay local. Domain functions remain usable by simulation systems and unit tests.
- Local view state and preference selection have separate storage from player body/progression. Presentation events carry a recipient and bounded sequence; legacy broadcast behavior is retained where required.
- A headless session owns fixed-step accumulation, input validation, commands and simulation streaming. Paused preview loading is allowed; active simulation generation is tick-budgeted. GPU uploads retain their own budget.
- Player capacity is an explicit core configuration, distinct from the two local views and historical two-profile save layout. A four-player headless configuration proves the boundary. Stable player/entity handles never identify objects by their position in a compacted room array.
- Application and renderer modules use private headers and explicit exported helpers. OpenGL and SDL stay outside the core. Quest-specific compilation remains restricted to its target.
- Checkpoint codecs retain their explicit little-endian fields and transactional replacement. Live snapshot encoding has its own header, bounds and version, retains active simulation state, and excludes local UI preferences. It is a state contract for later networking, not a network service.
- Shared CMake helpers configure normal, Quest and fault variants consistently. Coverage derives its required source set from the core source manifest and retains the 95% per-source threshold.

## Risks / Trade-offs

- Input ordering and menu side effects: compare existing browser/controller journeys and deterministic command traces.
- Player capacity and saved masks: explicitly retain historical two-profile codecs and test expanded-party state separately.
- Module extraction changes conditional compilation: build native, web and Quest and keep parity/isolation checks.
- Snapshot allocation or malformed data: decode transactionally with bounded readers and fault tests; never serialize C padding or pointers.
- Streaming timing: verify equal tick traces across frame schedules, and preserve generation/content fingerprints.

## Migration Plan

Land the complete refactor on main after local validation. No existing expedition is reset. Preserve immutable fixtures and run all required CI and Pages gates. Document the new module/API ownership and the remaining multiplayer milestones.
