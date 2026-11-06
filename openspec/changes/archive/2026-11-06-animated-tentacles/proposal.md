# Proposal

## Why
The infestation needs visible living motion and recognizable tentacles. Existing seeded floor growths, wall nests and gazer bodies provide coherent places for this detail.

## What Changes
- Add tapered, curling 3D tentacles to existing growths and nests, with suckers, subdued glowing tips and seeded motion.
- Give floating gazers longer animated trailing tentacles using the same visual language.
- Keep environmental growth decorative and bounded per view; freeze animation with the game and isolate special interiors.

## Capabilities
### Modified Capabilities
- `presentation-input`: living tentacle presentation and bounded independent rendering.

## Impact
Renderer and inspection/browser diagnostics only; no gameplay collision, enemy capabilities, saved data or generation identities change. Generator 10 and format 11 remain current, and existing expeditions receive the visual improvement. Validate with native CTest, separate gameplay tests, native render smoke, packaged browser regression and strict OpenSpec validation. Rendering is outside the core unit coverage metric; no new core code or exclusions are needed.
