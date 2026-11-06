# Proposal

## Why

The first two original milestones have a playable foundation, but fixed control
settings and instant deck transfers limit comfort and architectural scale. Finish
their implementable gaps with tunable input, stronger platform checks, continuous
stairs and ramps, and animated grav-lift travel.

## What Changes

- Add persistent mouse, touch and gamepad sensitivity, stick deadzone/inversion,
  and per-view retro render resolution settings through native and web controls.
- Preserve controller assignments across disconnects, reset held input on focus
  and orientation changes, and exercise actual SDL input adapters in native CI.
- New generator-9 ships have deterministic service shafts with continuous
  switchback stairs and ramps between decks, supported by slope collision,
  step climbing, head clearance and matching ray geometry.
- New ships use visible, bounded grav-lift journeys with safe streaming,
  per-player transit state, phase barriers and reliable return/checkpoint behavior.
- Keep generator 3–8 geometry and existing expedition progress intact. Save
  format 11, narrative 3 and matrix generator 1 remain; saves made during transit
  resume safely on its departure pad. Device preferences live outside expedition saves.
- Document physical Windows graphics/gamepad/Safari/mobile checks separately
  from automated compilation, virtual-device tests and browser execution.

## Capabilities

### New Capabilities

- `vertical-traversal`: Continuous stairs, ramps, safe physical movement and animated deck travel.

### Modified Capabilities

- `presentation-input`: Adjustable controls/resolution and robust input lifecycle behavior.
- `ship-generation`: Seeded service shafts and bounded geometry shared across deck boundaries.
- `persistence`: Generator-9 opt-in, old-ship stability and safe interrupted-transit saves.

## Impact

Core traversal, generation, collision, rendering, SDL adapters, browser controls,
save snapshots, native/browser tests and CI. No new game framework or runtime
service. Validate strict current/archived OpenSpec, native unit and gameplay
CTest, >=95% unit line coverage overall and per core file, ASan/UBSan, native
render captures, Windows compilation/input execution and the packaged browser
suite. Real hardware remains a clearly documented user playtest, not an automated
result claimed on its behalf.
