# Design

## Context

See proposal.md for scope. The game uses chunk-local axis-aligned boxes, 96-metre
horizontal cells, 64-metre decks and a fixed 60 Hz simulation. Current movement
slides along boxes but cannot climb a normal stair. Lift input immediately changes
decks. Interior generators 3–8 are save-compatible and must retain their geometry.
SDL input and browser controls already exist, but settings are fixed and controller
ownership is inferred from currently connected slots.

## Goals / Non-Goals

Goals: implement the remaining engine/input and traversal features with measured
budgets, testable collision and old-save stability. Provide explicit physical
device instructions where cloud automation cannot establish hardware behavior.

Non-goals: redesign every room footprint, arbitrarily rotated gravity, item
inventories, new story content, or claim real Safari/mobile/Windows GPU testing
from a simulated browser or cross-compiler.

## Decisions

1. Keep preferences and SDL device ownership in a platform adapter, outside the
   expedition format. Use bounded per-player sensitivity/deadzone/inversion and
   render-resolution settings, a native keyboard/controller options panel and
   accessible browser form controls. Save a small versioned preferences file;
   browser storage synchronizes it alongside, but independently of, expedition data.
   Controller owners change only on explicit player-count changes or attachment,
   not as a consequence of another controller disappearing.
2. Retain the current defaults and native HUD resolution. Select an internal
   pixel height for each view and clamp render targets. Reset pending touches,
   held mouse/keyboard state and catch-up time at relevant lifecycle boundaries.
   Test SDL virtual controllers in a headless input executable on Linux/Windows,
   and exercise the packaged browser's actual controls, focus and resizing.
3. Introduce generator 9 while retaining the versioned old generators. Reserve
   one deterministic service column per horizontal region, with a guaranteed
   boarding-bay column. Column choice is independent of deck height. Cut that
   shaft out of generated boxes and add aligned landings, enclosure, route
   markers, stairs and ramps. Other pads retain compact grav-lift shafts. Keep
   horizontal routes and terminals accessible. Geometry limits remain explicit.
4. Represent each ramp or stair flight as a compact bounded surface. Continuous
   ramps use a sloped plane; stairs quantize that surface into real treads.
   Rendering, support tests, body overlap and projectile rays use the same
   definition. Eight switchback flights span a deck, with landings at each turn.
   Grounded movement attempts a small step only with clear raised body space and
   snaps down short drops; jumps, flight and steep walls do not gain free ascent.
5. New-ship grav-lift activation validates destination clearance and phase access
   before latching a per-player ride. A finite eased journey moves actual world
   coordinates through the shaft. A lit cradle and shaft markings show motion;
   field-grate landings support walkers while the powered cradle crosses them.
   Movement/conflicting interactions are locked during transit, but looking stays
   active. Checkpoints update only at arrival. No global co-op camera transition.
6. Ride state is transient. A save snapshots riders and occupied rigs on their
   departure pad; loading clears transit. Join, leave, death, new-ship and failed
   save/import paths are tested explicitly. Save format 11 remains unchanged.
   Generator 3–8 ships retain original lift geometry and legacy behavior.

## Risks / Trade-offs

- Shaft carving can sever furniture or routes → reserve the complete footprint,
  protect main horizontal crossings, verify many seeds and interaction sites.
- Ramps or steps can disagree with collision → share their definition and test
  ascent, descent, headroom, sides, rays and negative deck seams.
- Streaming at a deck seam can strand a rider → validate destination first,
  preserve ordinary two-player streaming and hold safely if a frontier is pending.
- More architecture adds cost → compact flights, explicit per-chunk limits,
  cached meshes, unchanged chunk work budgets and selectable internal resolution.
- Physical hardware is unavailable here → label automated evidence accurately and
  provide a concise repeatable test procedure for Windows/gamepad/Safari/mobile.

## Migration Plan

Existing saves continue to load their recorded generator. Starting a new ship
selects generator 9 and fresh characters. Preference files default safely if
missing or invalid. Retain a generator-8 fixture before modifying generation.
Validate current and archived specifications, coverage, all campaigns, sanitizers,
native rendering, Windows compilation/input CI and packaged browser controls.
Commit and push the completed work directly to main; its workflow publishes Pages.


## Validation notes

The final apron includes a ground-level back landing and exits above its walking
clearance, so diagonal departures from a lift cannot fall into the stairwell.
Front landings join the central aisle without a floor gap. The full guardian
campaign and dedicated up/down action journeys verify these routes.

Browser focus reset is exercised at the DOM blur boundary because headless
Chromium keeps background pages focused. Hardware focus behavior remains in the
manual checklist. Automated Windows SDL input executes in the hosted native job;
local Windows evidence is cross-compilation. `TESTING.md` records coverage and
runtime evidence, and `FOUNDATIONS.md` gives the player/device procedure.
