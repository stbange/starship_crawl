## Why

The expedition reviews found coordinate-only guidance, a reproducible controller stall on seed 3, repeated lift travel from held input, and HUD text competing with the view. Ideal-aim campaign bots also cannot establish human difficulty.

## What Changes

- Guide each player to an actual connecting doorway or vertical connection, with an actionable HUD cue and map marker.
- Add readable landing signs, stair/ramp cues, lift destination previews and room focal accents. Require release before another lift ride; preserve free camera look.
- Reduce persistent HUD text and move detailed context into existing journals.
- Reproduce the retained navigation failure through ordinary movement, and add imperfect-input gameplay profiles with continuous resource/stall measurements.
- Provide reproducible human playtest checkpoints and a Windows/mobile results sheet.

## Capabilities

### Modified Capabilities
- `navigation`: bounded connection guidance.
- `vertical-traversal`: deliberate lift departures and readable landings.
- `presentation-input`: HUD hierarchy and world navigation accents.
- `quality-gates`: realistic play profiles and manual checkpoints.

## Impact

Core navigation, transient player state, rendering, read-only web diagnostics and tests. Generator 10 and save format 11 remain unchanged; no architecture, population, character or save reset. Existing ships receive navigation/presentation improvements. Native CTest, separately labelled gameplay, >=95% unit coverage per core file, packaged browser regressions, native render smoke and strict OpenSpec validation apply. Physical device results remain explicitly pending until performed.
