# Deep frontiers

## Why

The gameplay review found that different builds needed more useful encounters,
large spaces needed clearer focal views, and deep exploration needed rewards and
creatures beyond capped regional traits.

## What Changes

Implement gameplay review items 2, 4, 5 and 6: useful build-specific encounters,
architectural reveals, continuing directional contracts and the remaining alien
behaviors. New ships use generator 12; generators 3–11 retain their layouts and
populations. Save format 13 appends ship-local relic evolution for each player;
formats 1–12 remain readable. Fresh ships reset this progression.

Validate with strict OpenSpec, native unit and separate gameplay tests, unit-only
95% overall/per-file coverage, sanitizers, packaged browser tests and native
render captures. Automated checks do not establish human difficulty or physical
device performance.
