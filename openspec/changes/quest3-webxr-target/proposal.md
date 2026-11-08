# Proposal

## Why

The owner wants to play Starship Crawl in immersive VR on a Meta Quest 3 without
a store release. The current C/WebGL game provides the simulation and scenery,
but needs tracked stereo views, motion controllers and an immersive interface.

## What Changes

- Add an opt-in `starship_crawl_quest` Emscripten build target using WebXR, with
  its own launch page, JavaScript bridge, artifact and `/quest/` Pages address.
- Keep the existing Linux, Windows and ordinary web targets, their defaults,
  controls, rendering, save storage and public URLs unchanged. Compile the XR
  adapters and controller-ray combat support only into the Quest variant.
- Support solo tracked head/hand views, controller movement and aiming, snap
  turning, seated/standing calibration, a wrist display and floating menus.
- Reuse ordinary weapons, interactions, progression and save rules. Provide
  comfort controls and fades for disruptive transitions; preserve tracking
  while paused and stop held input when tracking/controllers are unavailable.
- Add isolated Quest unit/browser checks, packaging and CI, ordinary-target
  regression checks, and a reproducible physical Quest 3 playtest checklist.

## Capabilities

### New Capabilities

- `immersive-vr`: Optional standalone VR builds, tracked presentation and input,
  immersive controls, lifecycle, persistence isolation and headset validation.

### Modified Capabilities

None. Existing presentation/input and persistence contracts continue to govern
the Linux, Windows and ordinary web targets.

## Impact

Target-local CMake configuration, optional C platform/render adapters, a
Quest-only combat input extension, WebXR shell/bridge/storage, packaging,
GitHub Actions and documentation. No Unity, Godot, native Android APK or new
runtime dependency is introduced. Generator 14, simulation 1, narrative 3,
matrix 1 and save format 15 remain unchanged; existing supported saves remain
portable through explicit import/export. Quest browser storage is separate.

Validation includes strict OpenSpec checks, existing native/unit/gameplay and
unit-only >=95% overall/per-source coverage gates, isolated Quest adapter and
combat tests, real WebGL browser execution with test-side WebXR devices,
ordinary packaged browser regression suites and Linux/Windows/web builds.
Physical Quest frame rate, comfort and controller ergonomics require the
owner's headset and will be reported separately from emulation.
