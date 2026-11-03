## Context

The headless core owns world generation, streaming, collision, combat, progression, and save validation. SDL input and OpenGL rendering are adapters exercised by native smoke and actual WASM/WebGL browser tests. There are no new runtime dependencies.

## Goals / Non-Goals

Provide measurable unit coverage and automated player-visible acceptance checks. Maintain all executable core sources in the metric and publish the exact coverage scope. Do not equate line coverage with all branches covered or proof of gameplay balance.

## Decisions

Use native GCC/gcov with pinned gcovr for line, branch, and function reports. Enforce 95% line coverage both overall and per measured file. The unit report is generated from the unit-labelled CTest suites before campaign integration tests run, so gameplay coverage does not inflate the unit number.

Compile a second test-only core with forced allocator and I/O wrappers to verify failure paths. The shipped core and its API remain unchanged. Run the same tests on Linux and Windows and under address/undefined-behavior sanitizers on Linux.

Promote the deterministic campaign driver into a CTest gameplay test. It navigates connector routes, moves through space and ship gravity, shoots real enemies, spends earned points, uses lifts and terminals, and checks extraction. Retain the existing full-browser tests, pointer capture checks, mobile controls, screenshots, and native/WASM generation parity.

Use exact development dependency versions and strict OpenSpec validation. Gate Pages on native, web, and quality jobs together.

## Risks / Trade-offs

GCC line coverage is a native measurement, not WebGL shader or browser JavaScript coverage. Browser and render smoke tests validate those adapters separately. Rare defensive branches may remain unexecuted; no coverage exclusions or threshold reductions are used to hide them. Fault injection is confined to a test library so production allocation behavior is unchanged.

## Migration Plan

Install development tools, run specification validation and test commands, and require green CI before publishing. No game save migration is needed.
