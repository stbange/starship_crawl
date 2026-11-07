# Verification — 7 November 2026

The completed implementation retains generator 10 and save format 11. Guidance,
lift input latching and arrival feedback use transient player state. New signs
and focal fixtures change rendering only.

## Core and supported builds

- All 30 native CTests passed (16 unit, 13 gameplay, one SDL platform test).
  The 13 gameplay tests also passed separately after the final landing correction.
- Unit-only coverage passed at **6897/7000 lines (98.53%)**, with every executable
  core source above 95%. Navigation reached **167/167 (100%)**. Coverage-tool
  self-tests passed; no coverage exemptions or weaker thresholds were added.
- ASan, UBSan and LeakSanitizer passed the four focused guidance, recovery,
  traversal and imperfect-expedition suites. The final rear-landing correction
  passed the three affected guidance/recovery/traversal suites again.
  Leak inspection required execution outside the restricted cloud sandbox.
- Linux, Windows x86-64 cross-compilation and Emscripten builds succeeded without
  compiler warnings. Cross-compilation is not a physical Windows graphics test.
- Strict OpenSpec validation passed; all four automated profile records match
  the final gameplay output in `docs/playtests/p0-profiles.json`.

## Rendering and browser input

All fourteen packaged Chromium/WebGL suites passed during this change: baseline,
foundation, guidance, phases, exterior, features, shelter, matrix, equipment,
interiors, habitat, story, aftermath and deaths. The publishing workflow now
includes guidance alongside the existing thirteen suites.

The final focused guidance suite passed on `build-pages-p0-release`, after the
mobile header, sign-height and rear-landing refinements. It uses real keyboard,
map and route-selection input, imports an earned save, checks a held lift button
past arrival, requires release before returning, and observes independent
split-screen guidance. CDP touch input repeats the held-button behavior.
Wide, narrow and mobile portrait captures were inspected. Earlier broad browser
suites used preceding packaged candidates; CI reruns all suites on the commit.

Native Mesa smoke covered lift, stairs, ramps, tall room/engine, shelter and
matrix scenes. Lift/stairs/ramp smoke was repeated after the final correction.
The retained screenshot `docs/screenshots/p0-lift-landing.png` shows readable,
unobstructed deck signage. These checks use software rendering and emulated
mobile input, not physical handheld GPUs or a human first-play session.

## Design evidence and limits

The retained seed-3 checkpoint reproduces blocked westward movement while the
player stands 5.7 metres above the deck on a connector roof. Ordinary movement,
gravity and lift travel recover the player, then cross the same doorway both
ways at floor level and save/resume. This establishes the cause of this retained
case, not universal proof that all furnished routes are clear.

Two imperfect input/build profiles completed four phases and extraction on each
of seeds 713 and 42. Deaths were 0/1 and 1/5 respectively; all four reached zero
shield at some point, and none activated navigation recovery. Targets and routes
remain informed automation, and simulated time is accelerated. Post-update
resource reductions can miss damage masked by regeneration or respawn. These
results expose pressure absent from ideal-aim bots without establishing human
balance. No combat balance changes are included.

Human checkpoints and Windows GPU/controller, Android and iOS results remain
explicitly **pending** in `docs/playtests/P0_CHECKLIST.md`. Multi-stop express
travel remains a later extension; P0 implements deliberate adjacent-deck travel.
