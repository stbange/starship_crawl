The quality gates combine strict OpenSpec validation, component unit tests, deterministic campaign simulations, native rendering checks, and actual WASM/WebGL browser tests. Pages deployment waits for all required native, instrumented, browser, Quest and quality jobs together.

**Session architecture (R1–R7).**

The default native build registers 51 tests (30 unit, 20 gameplay and one SDL
platform check). The headless Quest configuration has 37 unit tests. Final
unit-only coverage is **98.1% (11,204/11,420 lines)**, with every measured
source above 95%. Renderer and platform integration remain outside this metric.
The ordinary-control replay retains the pre-refactor checkpoint bytes exactly.

`session_core`, `session_party_core`, and `session_quest` exercise semantic command
validation, addressed messages, view/connection identity, stale handles, room
compaction, bounded streaming and a command/input replay at 30/60/120 display Hz.
The four-player variant also exercises distant anchors, progression and reward masks
while retaining exactly two local view bindings. The ordinary/Quest control/save
parity test and historical save fixtures remain compatibility gates.

`snapshot_core`, `snapshot_party_core`, `snapshot_quest`, and `snapshot_faults` cover
live combat/matrix restoration, subsequent tick equivalence, view preference
isolation, malformed/truncated/schema-mismatched data and transactional allocation
failure. These snapshots are separate from checkpoint `.sav` files.

Run `python3 scripts/generate-snapshot-schema.py --check` after changing core data
structures. The quality job runs this check using the pinned `pycparser` development
dependency. To change the wire contract, review field ownership and bounds in
`scripts/snapshot-fields.json`, then regenerate the schema. A changed fingerprint
rejects incompatible live snapshots; it does not change historical checkpoints.

Coverage's required source inventory comes from `cmake/CoreSources.cmake` and remains
at least 95% per measured source and overall. Immutable table-only compilation units
have no executable gcov lines; the gate rejects functions added to that declaration
and measures every reported executable line, including newly added source files.


**Staged onboarding (generator 15 / save 16).** `staged_onboarding` checks earned
access, reward tiers, independent saved lessons, optional guidance, malformed-save
rollback, opening-room populations and the protected first matrix operation.
`staged_intro_journey` uses ordinary movement/combat/interaction to earn an interface,
restore power, finish all four phases and extract; it never grants equipment or
teleports the player. Its earned checkpoint is documented in
[fixture provenance](tests/fixtures/README.md).

Local validation includes 26 native component/platform checks, all 20 separately
labelled gameplay cases, 30 sanitizer unit checks including Quest, and the full
introductory expedition under ASan/UBSan/LeakSanitizer. Unit-only coverage is
**98.0% (10,624/10,843 lines)**, with every measured core source above 95%.
The final menu rules also pass the focused sanitizer case. Rendering, browser
JavaScript and platform adapters remain outside that unit coverage percentage.

`test_onboarding_browser.py` uses the packaged WASM for progressive menus,
keyboard/controller help, dismissal, saved guidance across new ships/imports,
mobile layouts and the earned first link. The Quest suite covers guidance menus
and repeats that first link with actual WebXR API inputs supplied by the virtual
device; `--intro-only` selects that case during development. The complete native,
ordinary-web and Quest workflow remains required before Pages deployment.
Physical Quest hardware and first-time human comprehension are not established
by these automated checks. [Onboarding evidence](docs/reviews/2026-11-09-onboarding/README.md).


**CI scheduling.** Every main push still runs the complete publication gates.
The ordinary web application is compiled and packaged once; six isolated browser
jobs download that exact artifact and run all 19 suites. Each runner uses one
browser test at a time. The browser launcher streams progress immediately and
retains full logs plus per-suite timing/exit-status JSON in its capture artifact.

Coverage is measured from the 37 unit tests alone, at the same 95% overall
and per-file threshold. Afterwards, six separate jobs run all instrumented
campaign cases from the coverage build, and six run all address/undefined-behavior/
leak sanitizer cases. `SC_SPLIT_CAMPAIGN_TESTS=ON` registers the existing two phase
routes and four imperfect-control expeditions individually, without changing their
inputs or assertions. That produces 24 campaign cases and 61 sanitizer tests.
The default registration retains the combined campaign cases. Runners use separate
filesystems so journeys with the same temporary save filename cannot collide.

`tests/ci-suites.json` assigns the groups. The runner rejects missing, duplicate,
stale or empty assignments before executing them; newly added CTests go into the
required remainder group. Pages requires all groups, the native platforms, strict
specifications, unit coverage and Quest checks. A failed group cannot publish.

```sh
python3 -m unittest discover -s tests -p 'test_ci_tools.py'
python3 scripts/run-ci-tests.py browser --check
python3 scripts/run-ci-tests.py browser --shard combat --site-dir build-pages-preview
cmake -S . -B build-ci-checks -DSC_BUILD_GAME=OFF -DSC_QUEST_TESTS=ON -DSC_SPLIT_CAMPAIGN_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-ci-checks --parallel 4
python3 scripts/run-ci-tests.py ctest --build-dir build-ci-checks --check
python3 scripts/run-ci-tests.py ctest --build-dir build-ci-checks --shard starboard
```

Court, regional and frontier browser combat checks allow the encounter-completion
tick after the last projectile kills a guard. They still require the real victory,
earned reward and save/reload checks. `SC_CPU_THROTTLE=4` on the court browser suite
reproduces the slow-renderer scheduling gap without changing the simulation.


**Quest 3 target.** `SC_QUEST_TESTS=ON` adds isolated tracking/combat tests and a
byte-for-byte saved-expedition comparison between ordinary and Quest cores.
`python3 scripts/check-coverage.py --quest` measures both variants and explicitly
requires `quest.c` in the unchanged 95% overall/per-file unit-only gate. The
ordinary coverage command and target defaults are unchanged. CI also runs the
Quest tests under address/undefined-behavior/leak sanitizers.

Package both pages at their production-relative paths before testing:

```sh
python3 -m pip install -r tests/requirements-quest.txt
python3 scripts/package-web.py --output build-pages-preview/starship_crawl
python3 scripts/package-quest.py --output build-pages-preview/starship_crawl/quest
python3 -m unittest discover -s tests -p 'test_quest_packaging.py'
python3 tests/test_quest_browser.py --site-dir build-pages-preview --url-path /starship_crawl/quest/
```

The test-only `tests/quest_device.js` supplies standard WebXR APIs, valid poses,
controllers and a real WebGL framebuffer. The browser test exercises the shipped
WASM, distinct eye captures, one simulation update per XR frame, head/controller
independence, menu navigation, progression, ordinary-control boarding, saved
legacy facilities, session permission/tracking/visibility/context loss,
re-entry, and same-origin save isolation. No emulated device or gameplay grants
are shipped in the application. Results and stereo captures are retained in
`build-quest/test-artifacts/` and CI's `quest-test-captures` artifact. The normal
browser suites remain separate required gates. Physical Quest 3 frame rate,
comfort, input and audio checks remain [explicitly pending](docs/playtests/QUEST3_CHECKLIST.md).

The Quest suite also starts with a missing controller, null or emulated hand
poses, a vertical head pose and an unsupported controller mapping. It requires
nonblank stereo recovery images, frozen gameplay, deliberate release-plus-A
recovery, visible lean/recenter guidance, and usable return/reporting when startup
stalls. Timeout cases advance the external browser clock without changing game
state. `--startup-only` runs these focused cases during development; CI always
runs them as part of the complete Quest suite.

The framebuffer regression additionally enforces opaque XR framebuffer lifetime
in the external device. It queues a real WebGL `INVALID_FRAMEBUFFER_OPERATION`
during graphics setup and requires nonblank, distinct stereo recovery views with
both controllers absent, followed by deliberate resume and session re-entry.
Incomplete targets, actual draw/clear failures (including after successful entry),
and unrelated setup GL errors must still end the session with diagnostics and
usable save/retry controls. These cases run in `--startup-only` and the full CI
suite. Captures and structured reports use the `framebuffer-*` / `quest-framebuffer-*`
names in `build-quest/test-artifacts/`. This reproduces the reported error signature;
it does not establish which operation originally failed on physical Quest hardware.

**Hidden factions (generator 14 / simulation 1 / format 15).** Five focused unit
suites cover the shared clock and geography, persistent forces and capacity,
autonomous orders/resources/hostility, invisible intervention effects and memory,
and malformed/allocation-failed saves. Crowded-room leases, separated co-op,
exposed matrix bodies, shelter suspension, repeated imports, casualties and cargo
have dedicated checks. The coverage gate includes all four new C sources and
`save_factions.c`, with no uncovered core exclusions.

The final native release suite passes **44/44** (24 unit, 19 gameplay, one SDL
platform); all 19 gameplay-labelled tests also pass separately. Unit-only core
coverage is **97.92% (10,119/10,334)**, with every source at least 95%. All 18
packaged browser suites pass across the retained batches/reruns. The final
nonlethal-warning capture adds a normal co-op join, emulated controller shot,
separate incident attribution, actual signal bars and stereo output. All 43
core ASan/UBSan tests pass with leak detection off in the sandbox; the five new
unit suites also pass leak detection outside it. CI publication status is
recorded in the linked review.

`faction_revisit_journey` follows an earned cross-sector raid, shoots a real
member, leaves and revisits, sees occupation and returns to the bay.
`faction_convoy_journey` approaches a real supply carrier through a doorway.
`hidden_simulation_journey` starts a fresh character and uses ordinary movement,
aiming, combat, upgrades, shelter and save/resume. Linux linker wrappers used by
that driver record timings and permit paired director-disabled comparisons;
the shipped game/core has no disable switch, test grants or timing hooks.

The new packaged `test_hidden_browser.py` imports the close earned convoy through
the ordinary UI and checks hidden desktop/touch controls, normal dossier/map
access, actual WebGL output, optional co-op, IndexedDB resume and frozen strategy
while paused. Existing legacy-court browser controls stay covered separately.

```sh
ctest --test-dir build -R 'hidden_|mobile_forces|autonomous_politics|faction_' --output-on-failure
python3 tests/test_hidden_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
python3 docs/reviews/2026-11-08-hidden-simulation/data/run.py --jobs 2
# Use a separate output directory when repeating an existing retained batch.
python3 docs/reviews/2026-11-08-hidden-simulation/data/run.py --runs 0 3 5 9 --no-director --jobs 1
```

The [current implementation review](docs/reviews/2026-11-08-hidden-simulation/README.md)
retains all attempted long runs, exact build/source hashes, failed routes,
comparisons, native/web measurements and actual captures. It records final gate
results and publication status; historical results below describe their own
release. Human readability/audio judgement and physical-device checks are
tracked in the [repeatable checklist](docs/playtests/HIDDEN_FACTIONS_CHECKLIST.md).

**Alien courts and the director (generator 13 / format 14).** The suite contains
36 native CTests: 19 unit, 16 gameplay and one SDL platform test. All 36 pass;
all 16 gameplay-labelled tests also pass separately.
`political_expeditions` checks 48 seeded territories, nine sites per ship,
pure map queries, all nine government/settlement combinations, neutral patrols,
personal hostility, shared co-op claims, workshops and annexation. A dedicated check verifies that
neutral patrols leave story terminals accessible while hostile patrols contest
them, including co-op and matrix allegiance. It tests all
four director cards, pressure from hostile patrols, budgets, repetition limits,
paused clocks, expiry, pending-dispatch save/resume and transactional corruption
rejection. Existing generators and format-13 expedition fixtures remain supported.

Unit-only core line coverage is **98.41% (8483/8620)**; every executable core file
exceeds 95%. Political logic measures 98.68%, political generation 97.67%, and
the director reaches 100%. All three new sources are required by the same
coverage gate. Six gate self-tests
pass. Renderer, platform and browser adapters remain outside this unit percentage.

`political_journeys` loads a fresh-character, gameplay-earned phase checkpoint,
walks to three courts, accepts commissions, fights and claims each conduit and
royal guard, then saves/resumes each settlement. Its shared movement/aim/fire
helper never grants equipment, teleports or injects damage. Strong late-game
builds finish these finales quickly; this is functional evidence, not human
balance validation. See [POLITICS.md](POLITICS.md),
[fixture provenance](tests/fixtures/README.md) and
[retained evidence](docs/reviews/2026-11-07-alien-courts/README.md).

The new packaged browser suite passes with real audience keys, deliberate choices,
co-op combat, earned fixtures, IndexedDB resume, and CDP touch USE. An optimized
checked build exposed a WebAssembly stack overflow while building the HUD map;
the release now reserves 1 MiB and enables stack-overflow guards. Court inspection
captures use saving-disabled scenes; settlement captures follow actual combat.
All seventeen packaged browser suites pass across the implementation. The main
suite's initial concurrent-rendering timeout passes when rerun alone with the
same assertions and deadlines. Final core refinements also pass fresh unit
coverage, focused sanitizer checks, portable builds and the relevant gameplay
and browser checks; details and logs are linked in the retained evidence.
Physical Windows/mobile graphics, physical controllers and human difficulty
remain playtest work.

```sh
ctest --test-dir build -R 'political_expeditions|political_journeys' --output-on-failure
ctest --test-dir build -L gameplay --output-on-failure
python3 scripts/check-coverage.py
python3 tests/test_politics_browser.py --site-dir build-pages-courts --url-path /starship_crawl/
```

**Deep frontiers (review items 2, 4, 5 and 6).** All **34 native CTests** pass:
18 unit, 15 gameplay and one SDL platform test. All 15 gameplay-labelled tests
also pass separately. `deep_frontiers` retains 24 immutable generator-11
plan/geometry/population references and format-12 migration, samples all six
encounter patterns, nine focal structures, five contracts and ten new species,
and checks physical spawns, lift clearance and bounded geometry (355/512 peak
boxes in this sample). It covers actual fragment/tail bullet and beam hits,
shared deaths, one molt, co-op sight, warned hazards, relic attacks/mobility,
both saved profiles, once-only claims and malformed format-13 rollback.
The final focused checks also cover crouch on the first movement frame and a
real wall-to-ceiling transition followed by a locked, avoidable drop.

Unit-only line coverage is **98.43% (7884/8010)**, with every core source above
95%. `frontier.c` is 98.52%, `alien_special.c` 97.30% and `reveals.c` 100%; all
three are required by the coverage gate. Six gate self-tests pass. Renderer,
audio and input adapters remain outside this core unit percentage.

`frontier_journeys` loads an earned generator-12 checkpoint, walks to five new
patterns, fights and claims through production APIs, and refits owned equipment
inside an actual summoned shelter. Paired comparisons reload the same next
arena and use a carbine with or without the relevant capability. The cipher
route links at its physical terminal and uses real matrix Veil/SECURITY.
It does not inject position, inventory, health, damage or enemy state.
These are strong late-game builds: fast clears remain possible and these
observations do not establish human balance. See [Deep frontiers](DEEP_FRONTIERS.md).

Linux, Windows x86-64 cross-compilation and Emscripten builds pass. Native Mesa
captures cover focal structures, split-screen courier circuits, colonies and
molters. All sixteen packaged Chromium/WebGL suites pass. The final presentation
refinements additionally pass earned-claim/save checks, creature HUD inspection,
desktop/portrait journals, wrapped mobile contract previews and tactics, rotation,
and a native split-screen smoke check.

All 33 core ASan/UBSan checks pass, combining the original 32 successful tests
with the rerun of `imperfect_expeditions`. That four-profile run initially hit
600 seconds, then passed in 925.64 seconds with a sanitizer-only 1,200-second
allowance; its frame/death assertions and the native allowance are unchanged.
The final deep-frontier unit test and four-profile journey also pass
LeakSanitizer outside the sandbox. The full 33-test run used leak detection off;
do not interpret this as full-suite leak coverage. Browser failure logs now
appear in retained artifacts and public Actions annotations.
See the retained [follow-up evidence](docs/reviews/2026-11-07-deep-frontiers/README.md).
Physical Windows graphics/controllers, mobile GPUs, Safari, human difficulty
and the final sound mix remain playtest work.

```sh
ctest --test-dir build -R 'deep_frontiers|frontier_journeys' --output-on-failure
ctest --test-dir build -L gameplay --output-on-failure
python3 scripts/check-coverage.py
python3 tests/test_frontier_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
```

**Regional encounters (P1).** There are **32 native CTests: 17 unit, 14 gameplay
and one SDL platform test**. All 32 passed, followed by all 14 gameplay-labelled
tests separately. `regional_encounters` verifies immutable generator-10 plans,
geometry, populations/sprites and format-11 migration; functional weather and
pacing; supported squads across generated rooms; all eight saved local actions;
locked alien commitments, evasion and planted wall contacts; support coordination,
beam pressure/collapse; early/late co-op participation and durable claims;
per-player earned-tool practice; malformed format-12 claims and fresh-ship reset.
The fault library also checks record-allocation failure without awarding progress.

Unit-only core line coverage is **98.51% (7340/7451)**; every source exceeds 95%.
The new `alien.c` measures 98.39% and `encounters.c` 98.16%. Both are explicitly
required by the gate. All six coverage-gate self-tests pass. The metric includes
all core sources with no uncovered-line exclusions; renderer/input/browser code
is covered by execution checks, not this percentage.

`regional_journey` loads played generator-11 checkpoints, earns a beam through
ordinary combat, and compares the starter carbine with that earned beam on the
same next encounter under two limited reaction/aim profiles. It never grants
inventory or health, teleports, deletes enemies or injects damage. The strong
rank-15 build wins all four comparisons without damage; this validates mechanics
and resource observations, not human difficulty. [Results, provenance, images
and pending human checks](REGIONAL_ENCOUNTERS.md).

The packaged `test_regional_browser.py` exercises keyboard activation, dormant
safety, visible warnings, paused animation, shared co-op state, local repair,
beam disruption, claim, the actual earned Veil practice, IndexedDB resume and CDP
touch USE. The habitat tour now checks all fourteen weather conditions. All fifteen packaged
browser suites passed. Its view-isolation driver now uses ordinary Veil and turns
away from the inspected growth before deploying shelter on clear floor; the full
suite passed after that correction. The final P1 package also passed the regional
suite and focused desktop/portrait HUD checks. Linux,
Windows x86-64 cross-build and Emscripten builds pass. All 31 core ASan/UBSan/LeakSanitizer tests passed; the final conductor-health and
reward refinements also passed the five focused regional/living-ship/fault checks.
Native Mesa smoke checks
cover relay, nest, local repair, boiling reservoirs, ice crystals and frozen runoff,
including split-screen. These are software OpenGL/WebGL and emulated-touch checks;
physical Windows/mobile devices and human play remain pending.

```sh
ctest --test-dir build -R 'regional_encounters|regional_journey' --output-on-failure
python3 tests/test_regional_browser.py --site-dir build-pages-p1-final --url-path /starship_crawl/
python3 tests/test_habitat_browser.py --site-dir build-pages-p1-final --url-path /starship_crawl/
```



**Reliable exploration (P0).** There are now **30 native CTests: 16 unit, 13
gameplay and one SDL platform test**. `exploration_guidance` covers bounded
reciprocal routes and detours across twelve seeds, phase gates, service landing
approaches, remote coordinates, pure generation queries, independent views,
held/released lift controls, unchanged camera orientation and save compatibility.
`navigation_recovery` loads the retained seed-3 review save, reproduces its roof
stall, and recovers through normal movement/gravity/lifts before crossing the
affected doorway both ways at floor level. No teleport or combat bypass is used.

`imperfect_expeditions` runs **two profiles on each of seeds 713 and 42** through
all phases and extraction. Reaction delay, turn speed, aim error and earned
upgrade policy differ; resource/stall observations are sampled each update.
All four finish, with 0/1 deaths on seed 713 and 1/5 on seed 42. They remain
accelerated, informed bots, not human difficulty evidence. [Results and limits](NAVIGATION.md).

All 30 native tests and the 13 separately labelled gameplay tests passed.
Unit-only core coverage is **98.53% (6897/7000 lines)**; every source exceeds 95%,
and `navigation.c` reaches **100% (167/167)**. The four focused traversal/navigation/
imperfect-input suites also passed ASan, UBSan and LeakSanitizer. Leak inspection
was initially blocked by the cloud sandbox and succeeded outside that sandbox.
Linux, Windows x86-64 cross-compilation and Emscripten builds succeeded.

The packaged `test_guidance_browser.py` holds keyboard and real CDP touch lift
controls past arrival, releases/returns, checks independent split-screen cues,
uses real map/route-selection input, imports an earned checkpoint and captures
narrow and mobile layouts. All fourteen packaged browser suites passed during
this change. The final landing/HUD refinements passed the focused guidance suite
again on the release package. Native Mesa checks cover lift/stairs/ramps, tall
rooms, shelter and matrix. Detailed execution scope is recorded with the completed
OpenSpec change. Physical Windows GPUs/controllers and mobile devices remain
pending in the [human checkpoint/result sheet](docs/playtests/P0_CHECKLIST.md).

```sh
ctest --test-dir build -R 'exploration_guidance|navigation_recovery|imperfect_expeditions' --output-on-failure
python3 tests/test_guidance_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
```

**Animated tentacles.** The habitat browser suite now inspects rooted tentacles in
an actual seeded room, checks changing rendered pixels while running and identical
pixels/time while paused, and verifies the 96-limb per-view limit. It checks both
split views, one player sheltered while the other retains tentacles, no ship limbs
in matrix/shelter/exterior views, and mobile-sized rotation. The existing anatomy
and death suites exercise the gazer variants and their corpse poses. Native CI
also renders the `tentacles` scene in split-screen.

All 27 native CTests and the 11 separately labelled gameplay campaigns passed.
The packaged main-game, anatomy, death, interior, shelter and exterior suites
passed, alongside the habitat tour and focused tentacle/view-isolation checks.
Native Mesa also rendered floor tentacles, the gazer family, split-screen and
special interiors; the final curve refinement passed repeated native captures
and browser checks. Linux, Windows x86-64 cross-compilation and Emscripten builds
succeeded, and strict OpenSpec validation passed.

The change is confined to presentation and inspection; core executable code,
generator 10 and format 11 are unchanged. Its geometry is validated through native
OpenGL and real browser WebGL execution, outside the core unit coverage metric.
See [captures](docs/screenshots/README.md#living-tentacles) and the
[player guide](LIVING_SHIP.md). Physical GPU/mobile performance remains a device
playtest task.

**Living ship generation (generator 10).** The suite now contains 27 native
CTests: 15 unit, 11 gameplay and one SDL platform test. `living_ship` checks eight
macro forms, matching paired decks, masked ownership, actual open floor volumes,
rails, supported enemies (including narrow conduits), all climates/tales/ecologies,
clear portals, spawn load-order/phase independence and unchanged generator-9
geometry/populations. It also verifies brood reproduction with inherited coloration
and separated co-op at adverse region corners. The desired union is bounded at
174 chunks inside the 192-resident cache; the sampled geometry peak is 302/512
boxes. `living_ship_journey` walks across both vault levels and saves/resumes
through the production APIs after unlocking the required campaign phases.

Unit-only coverage is **98.50% (6776/6879 lines)**, with every executable core
source above 95% and `habitat.c` at 100%. All 27 native tests and the 11 separately
labelled gameplay campaigns passed. The complete 26-test ASan/UBSan core suite
passed, followed by focused sanitizer checks of the final generation/spawn changes.
The six coverage-gate self-tests and strict OpenSpec validation also passed.

All thirteen packaged Chromium/WebGL suites passed. The new
`test_habitat_browser.py` tours seven conditions, both vault levels and bent/horseshoe
plans; checks real screenshot pixels and animation; opens the local dossier/map;
walks through a generated space; checks bounded split-screen sprites; enters the
shelter with weather still rendered in the other view; and exercises mobile-sized
rendering/rotation. The main browser regression checks native/WASM fingerprints.
Existing combat, equipment, story, matrix, input and persistence regressions remain
in the publishing workflow. Final atmosphere and main-browser checks were repeated
after layout/renderer refinements; final spawn refinements also passed the focused
atmosphere suite, unit/coverage checks and gameplay campaigns.

Linux, Windows x86-64 cross-compilation and Emscripten builds succeeded without
compiler diagnostics. Native Mesa smoke checks cover both vault levels, steam,
runoff, split-screen, matrix, shelter and skiff cabin views. This is automated
Linux/software-GL and Chromium execution, with touch emulation; physical Windows
GPUs/controllers, Safari and actual mobile performance remain unverified.
See [LIVING_SHIP.md](LIVING_SHIP.md) and [captures](docs/screenshots/README.md).

```sh
ctest --test-dir build -R 'living_ship' --output-on-failure
python3 tests/test_habitat_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
```

**Controls and traversal (generator 9).** The foundation milestone recorded 25 native CTests: 14 unit,
10 gameplay and one SDL platform test. `platform_input` uses SDL virtual gamepads
and actual axis/button events to verify controller ownership, disconnect/reconnect,
player-count changes, preferences, safe bounds and atomic persistence. It runs in
the Linux and Windows CI jobs; it does not certify physical controllers.

`vertical_traversal` checks shared stair/ramp surfaces, collision and rays, step
limits, crouch/headroom, phase and pending-deck barriers, regional column alignment,
landing/terminal clearance, lifts, co-op, rigs and safe transit save snapshots. A
24-seed shaft sweep peaks at 277 of the 512 available boxes; flight storage is
bounded at 16 per chunk. Twenty immutable generator-8 geometry fingerprints and
a real format-11 save protect older ships. `traversal_journeys` separately walks
both complete 64-metre routes up/down through normal gameplay actions, reloads on
a landing and crosses a negative-deck seam. Campaign runs do not contribute to the
unit coverage report.

`test_foundation_browser.py` exercises labelled HTML settings, native F1 controls,
IndexedDB reload, import/new-ship independence, independent cached split targets,
virtual browser gamepads, the DOM blur boundary, held-touch rotation/cancellation,
tiny viewports, real keyboard stair/ramp movement, separate split-screen lift
positions, saving/reloading during transit and mobile lift buttons. Headless
Chromium retains focus on background tabs, so the blur check dispatches the
browser event explicitly and observes the production WASM adapter; physical tab
switching remains on the manual checklist. Other browser interactions use normal
DOM, keyboard, mouse and CDP touch input, with read-only runtime diagnostics.

All twelve packaged Chromium/WebGL suites passed, including the new controls/
traversal suite and the existing story, matrix, combat, equipment and persistence
regressions. Captures of the new routes are in [docs/screenshots](docs/screenshots/README.md).

The new core source `traversal.c` is included in the same >=95% per-file gate.
Foundation unit-only core coverage was **98.41% (6448/6552 lines)**, with every measured
source above 95% and `traversal.c` at 100%. All 25 native tests, ten separately
labelled gameplay tests and 24 ASan/UBSan core tests passed; the final traversal
refinements also passed focused sanitizer checks. Linux, Windows x86-64 and
Emscripten compilation succeeded without compiler diagnostics. Native Mesa smoke
checks include stairs, ramps, lifts, cabin, hall, shelter and matrix split views.
The [device checklist](FOUNDATIONS.md#physical-device-checklist-not-completed-in-cloud)
explicitly leaves physical Windows GPU, controllers, Safari and mobile devices
unverified.

```sh
ctest --test-dir build -R 'platform_input|vertical_traversal|traversal_journeys' --output-on-failure
python3 tests/test_foundation_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy \
  ./build/starship_crawl --seed 713 --scene stairs --split --smoke 30 --no-save
```

**Arrival exterior.** `space_exterior` adds unit checks for sky/hull visibility,
the shared skiff collision and weapon-ray geometry, cabin save/resume, co-op,
generator-7/8 interior and population parity across eight seeds, and a real older
phased save. `arrival_journey` uses movement actions to enter the craft, encounter
its walls/ceiling, visit the bay, return, reload and exit with a second occupant.
Both are part of the 22 native CTests (13 unit and 9 gameplay). Executable
`exterior.c` lines are included in the same per-source 95% coverage gate.

The eleventh packaged browser suite, `test_exterior_browser.py`, repeats the
return route using keyboard input, saves in the cabin, exercises separate
split-screen views and enters/leaves using real mobile touch events. Screenshot
pixel checks verify stars in exposed sky, opaque hull, lamps with the flashlight
off, and cabin lighting. Captures use the `exterior-` prefix in
`build-web/test-artifacts/`. Native inspection scenes are `arrival`, `hull`,
`space`, `skiff` and `cabin`; existing shelter/matrix/indoor smoke checks verify
that each view restores its own fog, projection and lighting.

```sh
ctest --test-dir build -R 'space_exterior|arrival_journey' --output-on-failure
python3 tests/test_exterior_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy \
  ./build/starship_crawl --seed 713 --scene cabin --split --smoke 20 --no-save
```

Arrival verification on 2026-11-05 passed all 22 native CTests, all 22
ASan/UBSan CTests, the nine separately labelled gameplay tests, six coverage-gate
self-tests, all eleven packaged Chromium/WebGL suites, and strict current/change/
archived OpenSpec validation. Unit-only core line coverage was **98.30%**
(6212/6320 lines), with every executable source above 95%; `exterior.c` reached
100% line and branch coverage. Native Mesa captured the five exterior views plus
hall, shelter and matrix split-screen views. Linux, Emscripten and Windows
x86-64 builds completed without compiler diagnostics. Mobile checks used Chromium
touch emulation; Windows compilation and software rendering do not establish
physical-device graphics, controller or mobile-GPU performance.

**Specification workflow.** Node 22.12+ is needed only for development tooling. OpenSpec 1.16.0 is pinned in package-lock.json, with six generated Codex workflows under `.agents/skills/`. The C game has no Node runtime dependency.

```sh
npm ci --ignore-scripts
npm run spec:validate
npm run spec:list
npm exec -- openspec new change descriptive-change-name
npm exec -- openspec status --change descriptive-change-name
```

The baseline specs describe generation, combat, progression, persistence, expedition goals, input/presentation, and quality gates. Use `npm exec -- openspec ...` when following the generated skills so the repository's pinned CLI is selected. `openspec/config.yaml` supplies project context and validation rules. Specification checks run noninteractively in CI with telemetry disabled.

**Unit tests.** These compile and run without SDL or a graphics device:

```sh
cmake -S . -B build-core -DSC_BUILD_GAME=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build-core --parallel 4
ctest --test-dir build-core -L unit --output-on-failure
```

`equipment` checks personal and mech beams, piercing and wall blocking, energy refusal, owned rig deployment, low ceilings, blocked exits and ejection, speed/braking, plating and medical repair, surface flare attachments, non-damaging utility shots, upgrades/caps, expiry/replacement, format-5 migration and format-6 occupied-rig/light persistence. `navigation_psionics` checks map/generator parity across region boundaries and distant coordinates, 30 body variants, all visible weapon volleys and piercing, cloak targeting versus incoming damage, energy refusal, flight collision, charged jumps, nova pulses, and v4/v5 persistence. `shelter` checks doorway placement, furnishing collisions, recovery and cancellation, hostile/projectile/pickup isolation, co-op, format-3 expedition retention, and format-4 sanctuary persistence. `core` checks deterministic generation, reciprocal portals, clear floors/connectors, fair bounded streaming, distant split-screen coordinates, physics, weapon effects, all enemy species, progression, loot, missions, and save compatibility. `edge_cases` adds invalid API inputs, useful-only supplies, duplicate rewards, melee/explosive attacks, healing, swept projectile impacts, jump/ceiling/lift boundaries, allocation failures, failed flush/close/replacement, malformed checksummed save fields, and transactional rollback.

Fault tests compile a separate `sc_core_fault` library from the same core sources with forced allocator/I/O wrappers. The production core has no test-only switches, injected allocator, or failure API. Fault injection and assertion helpers are excluded from the production binary, not from measured core coverage.

**Coverage.** Install the pinned reporters, then run the gate with GCC and its matching `gcov` available:

```sh
python3 -m pip install -r tests/requirements.txt
python3 scripts/check-coverage.py
# Optional separate build directory:
python3 scripts/check-coverage.py --build-dir build-coverage-check
```

The command configures `SC_COVERAGE=ON`, resets that build's counters, runs only unit-labelled CTest suites, and generates `build-coverage/coverage/index.html`, `coverage.json`, `summary.json`, and Cobertura `coverage.xml`. It fails below **95% line coverage overall or in any measured source file**. Gate self-tests also verify rejection of empty reports, missing sources, low individual files, and newly added uncovered core files:

```sh
python3 -m unittest discover -s tests -p 'test_quality_tools.py'
```

The measured scope is **all executable `sc_core` code** (`generation.c`, `history.c`, `interior.c`, `world.c`, `game.c`, `progression.c`, `story.c`, `expedition.c`, `death.c`, `save.c`, `shelter.c`, `navigation.c`, `psionics.c`, `equipment.c`, and any future core sources), plus shared `math3d.h`. `content.c` currently consists of static definitions with no executable lines; catalog tests exercise its definitions. The unit metric excludes third-party SDL and the SDL/OpenGL/browser adapters (`main.c`, `render.c`, `gl_api.c`, JavaScript, and shaders), which are validated by integration tests below. It is not a whole-application or shader coverage claim. There are no line exclusion markers or suppressed uncovered core paths. The fault variant's hits merge with the ordinary core by source location.

The current unit-only result and per-file counts are generated in
`build-coverage/coverage/summary.txt` and `summary.json`. The enforced threshold is
line coverage; branch/function coverage are reported separately. CI retains the
reports as `unit-coverage`. Campaign and render/browser execution stay out of the
unit number. The phased-expedition results are recorded below.

**Automated gameplay.** `equipment_journey` walks from the space arrival onto the bay floor, unfolds and fires the mech, drives/brakes the wagon, attaches a flare, and saves/reloads its light through the real simulation.  `shelter_journey` walks from space into the ship, summons the sanctuary, navigates around furniture to cook, rest, and receive treatment, saves/reloads inside, and returns to the same entry anchor. It uses movement/actions rather than writing positions or clearing enemies. The retained legacy `solo_campaign` CTest runs seeds 713, 1, and 42 from the space arrival through all six objectives. Its player driver navigates connected routes, uses gravity and crouching, aims and fires the real weapon system, spends earned points, rides lifts, defeats the Warden, and extracts. It checks victory, kills, finite defensive resources, and streaming budgets. Each run has a bounded simulation duration and a CTest timeout.

```sh
ctest --test-dir build-core -L gameplay --output-on-failure
# Direct execution prints per-system and campaign results:
./build-core/sc_campaign_tests
```

This is a reproducible navigation/combat regression, with an automated aiming driver. It does not establish human difficulty balance. The core tests separately cover co-op ownership, checkpoint death, new expeditions, and persistence.

**Sanitizers.** Native GCC/Clang address and undefined-behavior checks run all component and campaign tests:

```sh
cmake -S . -B build-sanitize -DSC_BUILD_GAME=OFF -DSC_SANITIZE=ON -DCMAKE_BUILD_TYPE=Debug '-DCMAKE_C_FLAGS_DEBUG=-O1 -g'
cmake --build build-sanitize --parallel 4
ctest --test-dir build-sanitize --output-on-failure --output-junit sanitizer-tests.xml
```

The sanitizer build uses modest optimization to keep the full simulation campaigns
within their test timeouts while retaining debug symbols, assertions, frame
pointers and both sanitizers. Coverage remains a separate unoptimized build with
the same 95% gate. CI pins its quality runner to Ubuntu 24.04 and publishes failed
test names/output as annotations; the `quality-test-diagnostics` artifact retains
JUnit reports and sanitizer test logs.

**Actual browser game.** Build with Emscripten, package the exact Pages layout, and exercise keyboard/mouse, optional split-screen, eleven interiors, first-mission rewards, journal upgrades, weapon switching, save/reload/import, native/WASM parity, flashlight controls, mobile multitouch, and resizing:

```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-web --parallel 4
python3 -m playwright install --with-deps chromium
python3 scripts/package-web.py --output build-pages-preview/starship_crawl
python3 tests/test_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
python3 tests/test_exterior_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
python3 tests/test_shelter_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
python3 tests/test_features_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
```

Set `SC_INPUT_RELEASE_DELAY_MS=350` when running `test_shelter_browser.py` to stress delayed automation key releases. The driver releases movement through the browser keyboard listener on arrival, then verifies the character stays stopped while delivery to the Python runner is delayed.

The features browser suite captures all six body families (five variants each), HUD/full maps, map pan/deck/recenter, visible firing that breaks veil, flight/ascent/landing, enhanced jumping, nova, format-6 persistence, shelter packing, and touch powers/map open/close. It checks that covered touch controls cannot activate while the map is open.

The shelter browser suite also walks between all facilities using keyboard controls, verifies holstered weapons, reloads a current-format IndexedDB save inside, returns to the exact ship anchor, and exercises mobile summon/entry/exit. It captures the exterior door, interior, split-screen, bed, kitchen, medical station, and mobile view.

Build `sc_tests` first, or set `SC_CORE_TEST` to its path. `SC_BROWSER` selects an installed Chromium; otherwise the suite uses full Chromium headless mode. It checks camera preservation during boarding and actual relative aiming, and records state plus a screenshot on failure. CI retains captures as `browser-captures`. The game runs through the compiled WASM and real WebGL renderer; tests do not replace its gameplay with JavaScript mocks.

**Native graphics.** The Linux job renders real OpenGL frames with Mesa. Windows CI compiles the game and executes all core/campaign tests; physical Windows graphics, mobile GPUs, Safari, and controllers still require device checks.

```sh
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy \
  ./build/starship_crawl --seed 713 --scene bridge --split --smoke 90 --no-save
```

In the managed cloud workspace, source `/workspace/.starship-tools/activate.sh` before build commands. If the locally installed reporter is outside the default Python environment, use `PYTHONPATH=/workspace/.starship-tools/qa/python` for the coverage command. CI installs the pinned Python requirements normally.

The packaged equipment browser suite uses real keyboard, pointer-lock mouse and touch/menu input to unfold/board rigs, fire all three mech guns and the three personal beams, save/reload while mounted, drive/brake/dismount, earn flare upgrades at the entry mission, attach light to the deck and restore it after reload. CI retains cockpit, parked-rig, wagon, beam, flare and touch HUD captures. Run:

```sh
python3 tests/test_equipment_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy ./build/starship_crawl --seed 713 --scene mech --split --smoke 30 --no-save
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy ./build/starship_crawl --seed 713 --scene wagon --smoke 30 --no-save
```

These are automated Chromium/Mesa checks; physical gamepad, mobile GPU and Windows graphics behavior still require device testing.

The browser CI job uses `ubuntu-24.04` so the pinned Playwright dependencies do
not change when GitHub migrates `ubuntu-latest`. Installation failures expose
their error tail as a job annotation before any game tests run.

The generated-interiors unit suite samples 640 plans for ship/profile diversity,
rare-hall frequency and source-connected incident trails. A 24-seed geometry
sweep covers all nine furnishing roles, three floor plans, all incidents and
four landmark types. It walks central passages and both side-space entrances
with mech clearance, checks floors and cache positions, compares load orders,
and enforces existing geometry/light/dressing budgets. Original generator-3
plan and geometry fixtures and a real format-6 save verify compatibility;
cache tests verify streaming and save/reload cannot duplicate a discovery.

The fifth browser suite captures all layouts, incident states and landmark
interiors; it uses keyboard movement to collect side-room caches, save/reload,
and leave through the other entrance. It also checks landmark maps, importing
an old generator-3 save, opting into a new ship with fresh characters, and
mobile landmark rendering. Browser movement releases keys inside animation
frames to avoid automation latency changing the route. Long walks use bounded
waypoints with the same position assertions so slow software rendering cannot
exhaust one deadline merely because a route is long. Run:

```sh
python3 tests/test_interiors_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy ./build/starship_crawl --seed 713 --scene landmark3 --split --smoke 60 --no-save
```

These tests establish correctness and bounded work; they do not establish a
sustained frame-rate target on physical mobile hardware.


**Procedural narrative.** The `narrative` unit suite covers the original three
motivations and four new motivations across 24 seeds each, all three outcomes,
both investigation orders, stable reservations, actual-entry discovery versus
streaming/map previews, contested and remote-choice refusal, prerequisites,
exactly-once co-op rewards, repeat extraction and malformed payload rollback.
Forty additional histories cover every cause/profile/arc, matching origin rooms,
controller and survivor notices, physical-route clearance, bounded geometry,
dossier fact gating, continued discovery after decisions and persistent purges.
Original format-7 decision saves round-trip without changing their story or
geometry. Both `story.c` and `history.c` measure 100% unit line coverage.

The `generated_expeditions` gameplay test retains all nine original
motivation/outcome combinations on generator 4 / narrative 1. The new
`history_expeditions` walks all **21** new combinations: three motivations × two
arc shapes × three outcomes, plus three containment methods. Every run starts
in space, navigates actual doors and lifts, fights enemies, spends earned points,
saves/resumes mid-arc, resolves its target and extracts. It uses normal actions
without teleporting or deleting enemies. The legacy six-stage campaign still
runs seeds 713, 1 and 42 on its original generator 4.

The sixth browser suite checks four briefings; walks from the bay to a real
command-origin room; reads unknown and discovered dossiers; changes map leads;
imports gameplay-produced old/new decision checkpoints through the save UI;
confirms direct and containment conclusions; and checks keyboard and mobile
locked/prepared choices, dossier pages and IndexedDB persistence. Captures
include origin interiors, dossiers, decisions and the selected map lead.
Decision checkpoints come from `./build/sc_campaign_tests --history-fixtures`;
original format-7 fixtures remain immutable. See `tests/fixtures/README.md`.
The shared fixture-import driver observes each import's completion message and
waits for simulation ticks before sending interaction keys; an earlier save's
unpaused state cannot satisfy the wait for a new asynchronous import.

```sh
ctest --test-dir build -R 'narrative|generated_expeditions|history_expeditions' --output-on-failure
python3 tests/test_story_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy \
  ./build/starship_crawl --seed 22 --scene origin --split --smoke 30 --no-save
```

Generator-3 and generator-4 plan/geometry goldens, all eight browser suites,
native OpenGL and ASan/UBSan remain required. Windows cross-compilation and
emulated touch do not establish physical Windows, controller or mobile-GPU behavior.

**Reactive expeditions and aftermath.** There are nine unit and six gameplay
CTest suites. The new `reactive_aftermath` unit suite exercises 16 seeds with
all three primary/council choices and multiple follow-on chapters. It checks
observation-driven events, both operation orders, local power/recovery, ward
boundaries and enemy targeting, turret shutdowns, faction gates and rewards,
all five continuing arcs, the original HARROW Warden gate, exactly-once co-op
rewards, retained careers, the bounded event ledger and chapter limit. Valid
checksummed malformed format-9 payloads must fail transactionally; legacy
formats and narratives retain their original behavior.

The `continuing_expeditions` gameplay suite completes 18 arcs across six seeds,
covering all five follow-on kinds. It walks, fights, rides lifts, resolves
councils and operations, extracts and resumes on the same ship through normal
actions. The navigation driver caches pure door plans for route finding;
planning does not advance story state. No player teleportation or enemy
deletion substitutes for completing these journeys. Unit tests cover the
larger outcome matrix; the gameplay runs cover representative full journeys.

The seventh browser suite imports eight checkpoints produced by that gameplay
driver. It checks actual powered rooms, ward entry/exit and map markers,
controller shutdown, keyboard and touch council choices, assisted conclusions,
F7 debriefs and event browsing, character careers, repeatable bay commissions,
deterministic next destinations, and IndexedDB/export/import persistence.
Captures include the council, powered rooms, debriefs, follow-on map and mobile
controls. Regenerate fixtures only deliberately; see their provenance in
`tests/fixtures/README.md`.

```sh
ctest --test-dir build -R 'reactive_aftermath|continuing_expeditions' --output-on-failure
python3 tests/test_aftermath_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
```

**Enemy deaths and machines.** `enemy_deaths` tests all 32 species, the five naturally generated robots, immutable generator-5 encounter fingerprints, generator-5/6 architecture parity, death/reward idempotence, warning periods, real bulkhead cover, blast chains, acid cadence, mine arming/trigger/dodge/expiry, co-op and sanctuary/ward isolation, pool saturation, detached streaming, save/reset behavior and attacks credited to a departed co-op profile. The continuing-expedition driver additionally records actual defeats of all five machines and all three hazard kinds while walking and fighting normally.

The eighth browser suite fires actual beam weapons, checks animated corpse lifetime and pause behavior, captures acid and mechanical wrecks, triggers and retreats from a mine through movement inputs, and performs touch combat. The existing anatomy suite now captures all six families and 30 structural variants. These are Chromium/Mesa checks, not physical mobile-GPU testing.

```sh
ctest --test-dir build -R 'enemy_deaths|continuing_expeditions' --output-on-failure
python3 tests/test_deaths_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy ./build/starship_crawl --seed 713 --scene form5 --split --smoke 120 --no-save
```


**Ship matrix.** All four hacking milestones are covered by two unit suites and
three played journeys. `ship_matrix` samples 512 seeded/versioned regional
networks, verifies real room mapping, connected reciprocal links, collision,
credentials, routing/force access, every program and upgrade, all six services,
robot controls, stationary ranged hits, movement dodges, all three conclusions, shared payments, faction/event history,
safe/exposed bodies, separate co-op networks, reserved links and active saves.
`matrix_faults` exercises index growth/collisions, duplicate records, allocation
failure, malformed checksummed format-10 payloads, transactional rollback and
the immutable pre-matrix format-9 migration fixture. Every executable matrix
core source exceeds the same 95% line threshold; no source is excluded.

`matrix_journeys` starts seeds 713, 42 and 1 in space and uses ordinary movement,
collision, jumping, aiming, pulses and interactions to link at the bay, restore
power, fight security, find a key, solve routing, prepare an optional conclusion,
save while choosing, resolve each of the three outcomes, and disconnect to the
same body pose. Its only navigation information is the public generated graph.
It never teleports avatars, clears security or grants resources. Gameplay
execution is excluded from unit coverage. Controller fixtures for browser tests
come from this played route:

```sh
./build/sc_matrix_journey
mkdir -p build-web/test-artifacts/matrix-checkpoints
./build/sc_matrix_journey --checkpoints build-web/test-artifacts/matrix-checkpoints
SC_BROWSER=/usr/lib/chromium/chromium python3 tests/test_matrix_browser.py --site-dir build-pages-matrix-tested --url-path /starship_crawl/
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy ./build/starship_crawl --seed 713 --scene matrix --split --smoke 60 --no-save
```

The ninth packaged browser suite walks to the actual terminal, links, restores
physical power, checks reward idempotence, blocks input/discovery through the
schematic, casts Veil and fires a visible pulse, equips Echo, renders mixed
matrix/physical split-screen, resumes a saved avatar and disconnects to its body.
It imports the played controller checkpoints, confirms all three outcomes via
keyboard and checks IndexedDB reload. It also imports a real old format-9 save
and uses touch/menu controls to enter the shelter workstation, run a service,
cast, inspect/close the map and return safely. Captures include the terminal,
cathedral network, pulse, programs, schematic, split-screen, all controllers and
touch view. `SC_MATRIX_JOURNEY` can select a different journey executable.

The current checks comprise **18 native CTests (11 unit, 7 gameplay)** and **nine
packaged browser suites**. Linux/Mesa rendering and Chromium software WebGL are
automated execution; Windows cross-compilation and emulated touch do not claim
physical Windows graphics, mobile GPU, Safari or controller-device testing.

Set `SC_CPU_THROTTLE=4` when running the matrix browser suite to exercise the same controls with Chromium CPU throttling. CI runs that suite first and annotates its error output before the retained browser captures, so a failure is diagnosable without private log access.


**Phased expeditions (generator 7 / save 11).** There are twelve unit suites and
eight gameplay suites. `phased_expeditions` covers fresh/inactive-partner reset,
owned and equipped capabilities, all reward families and receipts, refitting,
modules, gate collision and lift access, seeded traits, all guardian attack
families, retreat/co-op/reload behavior, sixteen generated stories, immutable
format-10 compatibility and checksummed malformed appendices with rollback.
The earlier mechanics suites explicitly use the retained generator-6 sandbox;
pure generator tests and the new progression suite exercise generator 7.

`phase_journeys` walks seed 713 via starboard and seed 42 via port from the space
arrival through the elite, four phase guardians, a witness, the deep target and
extraction. It uses ordinary movement, real aiming/projectiles, earned upgrades,
lifts, interactions and reward choices, with no teleportation, damage calls,
equipment grants or enemy deletion. Both runs save/resume midship and verify
fresh characters on the next ship. The carbine remains usable throughout.

```sh
ctest --test-dir build -R 'phased_expeditions|phase_journeys' --output-on-failure
./build/sc_phase_journey --fixtures tests/fixtures
python3 tests/test_phases_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy ./build/starship_crawl --seed 713 --scene guardian --split --smoke 60 --no-save
```

The tenth packaged browser suite starts a normal fresh game, checks locked
capabilities, cycles routes with F10/U, imports gameplay-produced checkpoints via
the file UI, claims a reward, refits near safe staging, saves/reloads, restarts an
interrupted guardian attempt, checks phase maps and completion, starts fresh from
an old save, and claims/persists a reward using emulated mobile menu input, including readable landscape and portrait journals.
Historical `--scene` views remain generator-6 inspection sandboxes so the nine
existing browser suites retain their original fixtures and equipment regressions.
The `guardian` preview uses generator 7; normal games never use the sandbox rules.

Automated campaigns do not establish human difficulty, and emulated touch,
Windows cross-compilation and Mesa rendering do not replace physical device tests.

The final phased-expedition unit report covers **6,187 / 6,295 executable lines (98.28%)**, with every core source above 95%. The lowest source is `matrix.c` at 95.74%. Reports include the new campaign/generator/combat sources without exclusions.

The final verification passed all 20 native CTests and all 20 ASan/UBSan/LeakSanitizer
CTests (GCC, RelWithDebInfo), all ten packaged browser suites, native Mesa smoke
checks and a Windows cross-build. The new browser suite additionally verifies
post-finale optional route guidance. Browser capture is owned by the web shell
so SDL and HTML cannot issue competing pointer-lock requests when journals close.
No physical Windows, controller, mobile-GPU or Safari testing is claimed.
