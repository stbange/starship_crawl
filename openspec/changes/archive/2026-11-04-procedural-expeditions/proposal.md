## Why

Every ship currently tells the same six-terminal story. Generated expeditions need a reason to enter, consistent discoveries, and decisions that lead to a finish while exploration remains endless.

## What Changes

- Record the complete procedural narrative roadmap in DEVELOPMENT_PLAN.md.
- Implement narrative milestone 1: rescue, knowledge recovery and alien technology salvage, a seeded hidden history, sixteen reusable scenes, exploration discoveries, optional objectives, three conclusions per motivation and extraction.
- Provide contextual scene props, a journal of confirmed facts and unresolved leads, selectable map guidance and keyboard/controller/touch decisions.
- Save narrative events and assigned locations in format 7. Formats 3–6 retain the legacy campaign and generator; profiles migrated from formats 1–2 begin a new generated expedition. Geometry remains generator 4.

## Capabilities

### New Capabilities
- `narrative`: consistent seeded expedition stories driven by player discoveries and choices.

### Modified Capabilities
- `expedition`: generated arcs for new expeditions, with the original campaign retained for compatible older saves.
- `persistence`: format-7 story state and transactional validation.

## Impact

Core story/progression/save code, journal/rendering, SDL/browser controls, documentation and automated checks. Validate with strict OpenSpec, native unit and gameplay suites, >=95% core line coverage overall and per file, sanitizers, OpenGL smoke and packaged Chromium tests. No external story service or runtime dependency.
