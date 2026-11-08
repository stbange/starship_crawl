# Design

## Context

See [proposal.md](proposal.md). The existing renderer owns low-resolution flat
framebuffers per player, derives cameras from yaw/pitch and draws its HUD after
blitting to the default framebuffer. SDL's Emscripten loop owns simulation and
presentation. The core has no SDL dependency; save format 15 does not serialize
input actions. The regular web build mounts IDBFS at `/persist`.

## Goals / Non-Goals

Goals are a standalone single-player Quest 3 WebXR build, controller-operated
play from the space approach into the ship, reuse of normal combat and rewards,
and strict isolation of ordinary targets. Provide the existing game systems
through VR controls and panels while exposing device validation honestly.

Native Android/OpenXR APKs, store distribution, network multiplayer, hand-only
tracking and full physical room-walking locomotion are outside this build.
The initial locomotion model is seated/standing with sticks and bounded tracked
leaning. Comfort and performance on the owner's actual Quest remain device checks.

## Decisions

- `SC_BUILD_QUEST` defaults off. With Emscripten it adds
  `starship_crawl_quest`, a distinct `sc_quest_core`, and target-local
  `SC_WEBXR` definitions. Existing targets keep their definitions and link
  paths. A separate headless `SC_QUEST_TESTS` configuration exercises the XR
  core/adapter without introducing SDK dependencies into native applications.
- Keep guarded adapter entry points in `main.c` and rendering in a separate
  include/module so ordinary source paths remain intact. A small C pose/input
  module provides testable calibration, coordinate transforms and snap latches.
  Quest-only action fields allow a bounded controller ray; ordinary combat
  compiles its existing eye-based path. Avoid moving the simulated character
  to a hand or temporarily changing the camera to fire.
- A separate HTML shell and WebXR bridge own capability detection, deliberate
  session creation, `XRWebGLLayer`, reference space and controller poses. Pause
  the ordinary Emscripten scheduler while XR owns frames. Update the fixed-step
  game once per XR callback, then render each provided eye into the compositor
  framebuffer. Reuse streamed geometry and GPU uploads across eyes.
- Convert WebXR's negative-Z forward basis to the game's positive-Z forward
  basis through a calibrated rigid transform. Retain camera-relative world
  origins for distant sectors. Eye matrices include headset roll and eye
  separation. Invalid poses pause safely. Bound leaning and fade obstructed
  head positions instead of forcing headset orientation or exposing walls.
- Touch Plus sticks provide movement and snap turning; triggers/grips and
  buttons provide firing, use, jump/crouch, weapon selection, map and menu.
  A floating action panel invokes existing production actions for progression,
  choices, shelter, powers, vehicles and matrix. Existing journal/map content
  is rendered onto readable in-world panels; compact status/navigation lives
  on a tracked wrist display. No production test grants or pose setters beyond
  the real XR input bridge are added.
- Preserve normal saves and versions. Quest mounts `/quest-persist`, including
  its own expedition and preferences, and exposes explicit import/export from
  its launch page. Additional comfort preferences have a Quest-specific key.
- Rendering begins with a conservative headset resolution scale and an optional
  vignette. Request 72 Hz only when the runtime reports support. Frame rate is
  observed, not guaranteed; higher settings are device-driven follow-up work.
  Discontinuous body/space changes use short fades while ordinary tracking
  continues. Tracking/controller/visibility loss clears actions and pauses;
  context loss stops drawing and offers reload to the saved expedition.
- Publish the regular site unchanged at its root and the new package under
  `/quest/`. A separate CI job builds/tests the Quest package; the Pages staging
  job combines two validated artifacts. Keep every existing native, coverage,
  gameplay and browser gate, including on documentation-only pushes.

## Risks / Trade-offs

- Software WebGL cannot predict mobile GPU latency or comfort. Retain captures
  and counters, and provide concrete Quest 3 checks for stereo scale, aiming,
  frame rate, dark-room readability, controller loss and every major transition.
- VR increases pixel work even for a retro game. Reuse geometry and uploads,
  bound effects, and make render scale explicit rather than promising 90 Hz.
- State leaking between builds would violate the user's main constraint. Test
  target definitions/artifact contents and same-origin IndexedDB isolation;
  run the ordinary suites and compare deterministic generation/save behavior.
- Head leaning and body collision differ. Keep the initial supported tracking
  envelope bounded, validate controller origins against geometry and fade when
  the head leaves a safe region. Do not grant locomotion shortcuts or rewards.

## Migration Plan

No save migration or new generator version is needed. Ordinary build commands
remain valid. Enable the new target explicitly, package it separately, test it,
then stage it below the current Pages root. Removing the Quest package or
disabling its option leaves the original builds and stored expeditions usable.
