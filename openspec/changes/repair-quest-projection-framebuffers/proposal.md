## Why

A physical Quest 3 on Oculus Browser 154 reports `FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT`
at the first XR binding, before any eye is drawn. The earlier emulator supplied
an attached opaque framebuffer and did not cover this runtime behavior.

## What Changes

- Prefer granted WebXR projection layers and attach their per-view textures to
  a Quest-owned framebuffer, retaining the legacy base-layer path when unavailable.
- Validate real attachments, supply application depth storage if the compositor
  omits depth, and release owned graphics resources on exit or failure.
- Reproduce the missing-attachment report externally and test both rendering paths.
- Identify the selected path in the troubleshooting report and publish the repair.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `immersive-vr`: reliable stereo targets and safe cleanup across supported XR APIs.

## Impact

Only the opt-in Quest JavaScript bridge, external browser tests and documentation
change. C simulation, ordinary targets, generator 16, campaign 2, save 17 and live
snapshot schema 2 remain unchanged. Validation uses Quest packaging tests, actual
WebGL stereo/controller/lifecycle regressions and ordinary-target isolation checks.
