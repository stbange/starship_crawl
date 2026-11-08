Starship Crawl is a playable first-person prototype in C17, SDL3, and OpenGL / WebGL 2. Enter an enormous gothic ship from space, explore its infested decks, fight strange creatures, and grow a character whose progress persists for that expedition.

**Play begins with one player.** Split-screen is optional: press F2, or use the browser menu, to explicitly add a second player. Loading a save always starts in solo mode and retains the second character's progression.

The prototype includes:

- A star-filled space approach, sparse lights receding along an enormous dark hull, and a small returnable boarding craft behind the player. The safe level-0 loading bay has left, right, inward, upper, and lower routes.
- Nine themed ship sectors and 28 room themes, including luxury resorts, command decks, research wards, cryonics, cargo, and botanical spaces, with connected routes, loops, and vertical grav lifts.
- Four ship identities, three furnished floor-plan families, and connected blackout, frost, quarantine, fire and bloom trails.
- Eight large footprint forms, from long galleries and bent arcades to branching concourses, plus paired-deck vaults up to 122 metres tall. Rare banquet halls, conservatories, suspended engines and reliquaries contrast with narrow crawling passages.
- Fourteen environmental conditions with steam sprites, shallow rippling water, colored rain, spores, strange glows and local fog; decorative banners, fungi, bones, papers and glyphs. Eight local tales connect visible evidence to the F6 dossier, while six enemy ecologies match the surroundings.
- Suites with two entrances, staggered work bays and open arcades; weapon and record caches reward side-room exploration. Pink map markings identify landmark discoveries.
- Incremental chunk generation and GPU uploads, retained neighborhoods during backtracking, and one shared bounded cache for optional co-op.
- Rusted panels, damaged vents, hanging cables, reactor pipework, cabin furnishings, egg nests, and branching growths with animated curling tentacles, suckers and faint luminous tips. Floating gazers carry trailing tentacles too.
- Dark district materials, mostly dead light fittings, weak colored pools of flickering light, a player flashlight, ambient drones, and an F3 district map with streaming diagnostics.
- Movement, collision, jumping, crouching, sprinting, and grav lifts that activate checkpoints.
- 42 enemy species, including robotic machines, listening membranes, gaze-bound scavengers, reversing worms, dividing colonies and molting sentinels. Cold/hot specialists, actual ceiling traversal, locked attacks and audible warnings add regional counterplay.
- Backwards pilgrims, pendulum gazers and wall-gripping seam-runners; optional linked nest/relay fights with earned equipment rewards, guided practice, and quiet/trace/horde room pacing. See [P1 gameplay changes](REGIONAL_ENCOUNTERS.md).
- Five more optional fight patterns reward flight, area damage, mech armor, wagon speed and matrix services. Directional contracts evolve five ship-local relics at layers 5, 8 and 11; nine district focal structures, distant lights and framed approaches strengthen scale. See [Deep frontiers](DEEP_FRONTIERS.md). Introduced in generator 12; older expeditions retain their original worlds.
- Eight local repair, ward, food and medical actions with saved lighting, atmosphere and machine consequences.
- 20 personal weapons, including three true beams and a wall-lighting flare cannon; rechargeable weapon energy keeps combat moving.
- Fold-space mech with heavy plating, cockpit HUD and three built-in weapons, plus a fast deployable wagon.
- Ten upgradeable attributes, armor durability, a regenerating energy shield, shared collectible caches, salvage currency, and 12 ship records.
- Three hidden alien powers with travelling patrols, supply convoys, raids, retreats, occupations and autonomous conflicts. An invisible director adjusts upcoming opportunities and recovery; discover events through encounters and their aftermath. See [Hidden factions](HIDDEN_FACTIONS.md). Generator-13 saves retain [legacy courts](POLITICS.md).
- Generated rescue, knowledge, salvage and containment expeditions, 31 story scenes, direct and dependent targets, optional operations, 21 primary outcomes and five continuing story types.
- Reactive microgrids, powered safe beacons, controller shutdowns, survivor factions and council agreements.
- A first-person ship matrix with separate avatars and bodies, six upgradeable programs in two equipped slots, security routines, routing puzzles, rare vaults, robot allies and seeded controller decisions.
- A debrief, recent event history and persistent rescued/knowledge/technology/containment career totals.
- A ship dossier, history-shaped rooms, catastrophe apparatus, survivor/controller notices, an expedition journal, selectable map leads, beacon extraction and persistent victories; older saves retain their original six-stage campaign.
- Three armor/weapon colors, forgiving respawns, fresh characters on new ships, and persistent defeated enemies.
- Native saves, browser IndexedDB saves, and browser save import/export.
- Low-resolution 3D rendering, procedural panel surfaces, fog, emissive details, and a separately rendered HUD.
- Mouse/keyboard, gamepad bindings, mobile touch controls, and optional two-player rendering.

**Build on Linux.** Install a C compiler, CMake 3.22+, Make or Ninja, and graphics headers. For Debian/Ubuntu:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake libx11-dev libxext-dev libxi-dev \
  libxcursor-dev libxrandr-dev libgl1-mesa-dev libegl1-mesa-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./build/starship_crawl
```

CMake downloads SDL 3.2.30 at a pinned commit and verifies the source archive's SHA-256. An OpenGL 3.3-capable driver is required. The renderer also works with Mesa software rendering for development checks.

**Build on Windows.** Use Visual Studio 2022 with the C++ desktop toolchain (which includes its C17 compiler), CMake, and an OpenGL-capable graphics driver:

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
.\build\Release\starship_crawl.exe
```

SDL is linked statically. A Windows x86-64 executable has also been cross-compiled with MinGW-w64; it imports only standard Windows system DLLs. The CI configuration builds both Windows and Linux. Physical controller and Windows graphics testing remain device checks.

**Build for browsers.** Activate Emscripten first. CI specifies SDK 4.0.23; the cloud prototype has also been compiled and browser-tested with Debian's Emscripten 3.1.69 and LLVM 19.

```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-web --parallel 4
python3 -m http.server --directory build-web 8000
```

Open `starship_crawl.html` through the HTTP server. Keep the HTML, JavaScript, and WASM files together. WebGL 2 is required; the build uses a single browser thread and does not require COOP/COEP headers. A click or tap starts play and unlocks audio. Mobile controls are intended primarily for landscape orientation.

**GitHub Pages deployment.** Work directly on `main`. Every push to `main`, including documentation and configuration changes, starts the `Build, test, and publish` workflow. It builds the web game, stages `index.html` with its JavaScript and WASM files, and exercises the complete browser suite under `/starship_crawl/`. Once the quality, native, and web checks pass, the tested artifact publishes automatically through the official GitHub Pages actions. Pull requests build and test without deploying. In repository Settings → Pages, set the publishing source to **GitHub Actions** once. The expected project address is `https://stbange.github.io/starship_crawl/`; the Actions deployment result confirms when it is live.

For a manual publication, open **Actions → Build, test, and publish → Run workflow**
and select **main**. Other branches can pass every test while their Pages job is
skipped. Merge the desired changes into `main` first; the push also starts
publication automatically. The run title and web job summary state whether that
run will publish. [Approach screenshots](docs/screenshots/README.md) show the
arrival and boarding craft, persistent controls/display settings, continuous stairs/ramps and animated grav lifts. See [controls and traversal](FOUNDATIONS.md).

To inspect the exact static site locally:

```sh
python3 scripts/package-web.py --output build-pages-preview/starship_crawl
python3 tests/test_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
```

**Controls**

| Action | Keyboard / mouse | Gamepad |
| --- | --- | --- |
| Controls and display settings | F1; browser Menu | Pause, then Back/Select |
| Move | WASD | Left stick |
| Look | Mouse | Right stick |
| Fire | Hold left mouse | Right trigger |
| Toggle flashlight | F | Right stick click |
| Jump / rise outside | Space | A / south button |
| Crouch / descend outside | Left Ctrl | B / east button |
| Sprint | Left Shift | Left shoulder |
| Grav lift up / down | E / Q while on the cyan pad | X / Y |
| Switch weapon | Z / X or mouse wheel | Right shoulder |
| Use door, station, terminal; board / dismount rig | G | D-pad down |
| Deploy / pack mech; wagon | T / Y | Left trigger + A / B |
| Select flare cannon | K | Left trigger + X |
| Upgrade flare lifetime / strength | O / P | Browser menu |
| Deploy / pack empty shelter | R | Back / Select |
| Deck map / close map | M; arrows pan, Page Up/Down change deck, Home recenters | Left stick click; D-pad pans, shoulders change deck |
| Veil / flight / long jumps / nova | V / B / J / H | Hold left trigger + D-pad left / up / right / down |
| Expedition journal / character page | I / F4; Enter closes | Left trigger + Back / browser menu |
| Ship dossier / expedition debrief | F6 / F7; L browses past events (F6 twice opens courts on legacy generator-13 ships) | Right shoulder cycles journal pages; X browses events |
| Read next field record / track another lead | L / U | X / Y while in expedition journal |
| Confirm a story decision at its target | 1 / 2 / 3 when the decision is open | D-pad left / up / right |
| Upgrade attribute 1 through 10 | 1 through 9, then 0 | D-pad left / up / right for the first three |
| Link / disconnect matrix; program loadout | C at a terminal; F8 | Back at a terminal; left trigger + right stick |
| Change armor color | Pause, then C; browser menu | Browser menu or paused keyboard |
| Pause / resume | Esc, Tab, Enter | Start |
| Optional second player | F2 | Browser menu or keyboard |
| Build / guardian journal | F10; U route; 1/2/3 reward; Q/E slot; Z/X refit | Left trigger + Start; X route; A/Y/B reward; shoulders slot; left/right refit |
| Save / load | F5 / F9 | Browser menu or keyboard |
| New ship | N, then Enter to confirm; Esc cancels | Browser menu or keyboard |
| District map / streaming diagnostics | F3 | Keyboard |

With one gamepad and split-screen enabled, keyboard/mouse controls player one and the gamepad controls player two. With two gamepads, each player has one. Controller disconnects pause the game. Mobile uses a left movement pad, right-side drag to look, and on-screen fire/jump/crouch/lift/lamp buttons; upgrades and saves are available in the browser menu. Each player's flashlight starts on and can be toggled independently, without a battery limit.

Spitters visibly charge for 0.8 seconds before launching a slow glowing shot toward the position they aimed at. Keep moving sideways to evade it. Crawlers climb nearby walls and machinery before dropping back into pursuit. Health regeneration, the safe loading bay, and checkpoint respawns keep early exploration forgiving.

Current-ship grav lifts travel visibly between decks and establish a checkpoint. Release the travel button after arriving before requesting another ride; holding it cannot skip a landing. Older generator-8-and-earlier ships retain instant lifts. Death retains experience, skills, and equipment, restores health, and returns that player to the latest checkpoint. New ships replace the expedition and start both characters fresh. Re-entering a generated room preserves defeated enemies; partially injured enemies reset when their room unloads.

**New ships use generator 14, hidden simulation 1 and narrative version 3; saves use format 15 (matrix generator 1).** [Hidden factions and the invisible director](HIDDEN_FACTIONS.md) add persistent armies, autonomous supply wars, occupation and recovery pacing. Generator-13 saves retain their [courts and visible dispatches](POLITICS.md) on the format-14 path; all earlier supported expeditions keep their own rules. Generator 11 adds [P1 regional encounters](REGIONAL_ENCOUNTERS.md): varied hot/cold weather, alien attack commitments, optional support squads, tool rewards, pacing and local repairs. Generator-10 ships remain unchanged. Generator 10 adds varied footprints, paired-deck vaults, environmental tales, matching inhabitants and sprite-based atmosphere. Generator-9 saves retain their layouts and populations. See [living ship generation](LIVING_SHIP.md). Generator 9 added aligned stair/ramp shafts and animated grav lifts to the existing room plans. Generator-8 saves keep their original interiors and instant lifts; older generators retain their own geometry. Preferences are stored separately from expeditions. See [controls and vertical exploration](FOUNDATIONS.md). Generator 8 added the boarding craft and preserved generator-7 interior plans and encounters. Existing generator-7 saves keep their original exterior collision and positions. See [Arrival and boarding craft](ARRIVAL.md) for the new approach. Each new ship starts fresh characters; current-ship death, save/resume and continued commissions retain your build. Four guardian phases gate deeper exploration, with earned weapons, powers, rigs and specializations. See [Phased expeditions](PHASED_EXPEDITIONS.md) for routes, rewards and loadouts. Formats 3–6 retain their original architecture, encounters, caches and six-stage campaign. Press **N**, then **Enter**, to begin a new generated story with fresh characters. Formats 1–2 preserve both profiles and visibly begin a fresh ship because their geometry is incompatible. Format-7/narrative-1 expeditions keep their generator-4 geometry and original story. Format-8 expeditions retain narrative version 2. Format 10 adds matrix profiles, network records, active/reserved links and timed controls; format 9 retains room-system changes, faction support, chapter events, outcomes, careers and historical discoveries, assigned sites, tracked lead and final choice alongside character progression, world records, sanctuary, psionics, rigs and flare lights. Loading opens solo with the flashlight on and clears beam traces, in-flight shots, corpse animations and temporary death hazards. Generator-5 expeditions keep their original encounters. Unknown versions or corrupt data leave the current game unchanged.

Start at the gold-marked console beside the loading bay's lift with **G**. Explore for clues, investigate a lead, then decide how to resolve your target. Optional preparation unlocks a third outcome. **F6** opens a ship dossier whose facts are revealed by witnesses and physical notices. **I** shows confirmed facts and unresolved leads; **U** changes map guidance. **G** at the target opens choices, **1/2/3** confirms, and **Enter** leaves undecided. Return to the bay to extract and read the **F7** debrief. **G** at the bay accepts another commission on the same ship; careers remain on this ship and reset with a new ship. Microgrid power, beacon wards, controller shutdowns and the survivor council change the ship and available outcomes. See [NARRATIVE.md](NARRATIVE.md) for stories, consequences and controls, and [EXPANSION.md](EXPANSION.md) for combat, attributes and the legacy mission route.

**Reliable exploration.** Cyan map markers and an AHEAD/LEFT/RIGHT/BEHIND cue guide you toward the next actual doorway or lift; gold marks the final goal. Landing signs distinguish deck exits, stairs and ramps, lift pads preview both destinations, and tall rooms have restrained focal lights. A compact upgrade reminder keeps the scene clear. See [P0 navigation](NAVIGATION.md) and the [human/device checklist](docs/playtests/P0_CHECKLIST.md). Existing saves remain compatible.

**Navigation and psionics.** The top-left minimap marks the mission goal, your facing, lifts and the shelter. **M** opens a deck schematic; use arrows to pan, **Page Up/Down** for decks and **Home** to recenter. Goals outside the view stay marked at the edge, and deck differences are labelled. Earn and equip powers through guardian rewards and **F10**. **V** veils you until firing, **B** enables flight, **J** charges long jumps, and **H** emits a damaging nova aura. All use rechargeable energy. Enemy bodies now cover six families and five structural variants each; the original 16 weapons fire visible projectiles, with three new beam guns and a utility flare cannon. See [FIELD_GUIDE.md](FIELD_GUIDE.md) for controls, variants and ability details.

**Fold equipment.** Earn and equip a rig first. Press **T** for a heavily armored mech with three built-in guns and an amber cockpit HUD, **Y** for a fast wagon, and **G** to board or dismount. **K** selects the flare cannon; **O / P** upgrade burn time and light strength. Flare shots attach to walls and illuminate the ship. Touch players can deploy through the menu and board with USE. See [EQUIPMENT.md](EQUIPMENT.md) for stats, controls, repair, beams and persistence.

**Ship matrix.** Press **C** at a cyan terminal or the shelter workstation. Restore power with **F**, use **G** for keys/switches/controllers, **1/2** for programs, **Q/E** for services, **M** for its schematic and **F8** to equip/upgrade programs. Bodies remain in the ship and exposed-body damage disconnects. See [HACKING.md](HACKING.md) for all four implemented milestones, controls, consequences and limits.

**Fold shelter.** Press **R** on clear ship flooring to summon a small doorway, or press it again to pack an empty shelter. Use **G** nearby to enter an 18 × 16 metre sanctuary. Use **G** beside the bed to rest, the medical station to heal and repair armor, or the kitchen to cook a recovery meal. Return through the interior door to your entry position. Touch players have **FOLD** and **USE** buttons; the browser menu also offers summoning. Enemies stay outside, and the exterior continues for an outside co-op player. The doorway cannot move while occupied. See [SHELTER.md](SHELTER.md).

**Saves** are versioned, checksummed, and validated before replacing the current session. Native writes replace a temporary file; the default location comes from SDL's application preference directory (`Stbange/StarshipCrawl`). `--save-dir DIRECTORY` selects another location. Browser saves are local to the site's origin and browser profile; the HUD confirms completion after IndexedDB synchronization. Export a save to move it to another browser or device. A successful disk write alone is not reported as completed browser persistence.

Autosaves occur every 20 seconds of active play. Save explicitly before closing a tab; asynchronous browser writes cannot be guaranteed during tab shutdown. Imported saves with an unsupported version or corrupt payload leave the running expedition unchanged.

**Specifications and automated quality.** OpenSpec 1.16.0 and Codex workflows are checked in, with validated specifications under `openspec/specs`. Native unit tests measure **at least 95% core line coverage** and enforce **95% overall and in every measured source file**. Automated campaigns finish all nine generated outcomes and the three-seed legacy route; separate journeys exercise the shelter and equipment. Nine browser suites test the compiled WASM/WebGL game, input, saves, and mobile controls. Coverage reports and browser captures are retained in CI, and Pages requires all quality, native, and web jobs to pass. See [TESTING.md](TESTING.md) for commands, scope, reports, and the specification workflow.

**Development commands**

```sh
# Reproducible expedition, without changing your normal save.
./build/starship_crawl --seed 713 --no-save

# Inspect a specific room type.
./build/starship_crawl --seed 713 --scene hall --no-save
# Other scenes: bay, engine, crew, tunnel, atrium, distant, encounter.
# New sectors: luxury, bridge, lab, cargo, cryonics, botany.
# Generated interiors: layout0..2, history0..5, landmark1..4.
./build/starship_crawl --seed 713 --scene landmark3 --no-save
# Sanctuary inspection (saving disabled); add --split for an exterior second view.
./build/starship_crawl --seed 713 --scene shelter --no-save
# Reproducible crawler/spitter/heavy encounter, with saving disabled.
./build/starship_crawl --seed 713 --scene encounter
# Distant co-op diagnostic: player 1 is 288 million metres away; player 2 is at the bay.
./build/starship_crawl --seed 713 --scene distant --split --no-save

# Portable semantic generation records for native/WASM comparison.
./build/sc_tests --fingerprints

# Run core checks without SDL or a graphics device.
cmake -S . -B build-core -DSC_BUILD_GAME=OFF -DSC_SANITIZE=ON
cmake --build build-core --parallel 4
ctest --test-dir build-core --output-on-failure

# Render actual native frames using a headless Mesa driver.
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy \
  ./build/starship_crawl --seed 713 --smoke 10 --capture build/arrival.ppm

# Exercise the built browser application with Chromium.
python3 -m pip install -r tests/requirements.txt
python3 -m playwright install chromium
python3 tests/test_browser.py
```

Build `sc_tests` before running the browser suite; use `SC_CORE_TEST` to select a different native test executable. The suite compares its semantic generation fingerprints against the running WASM build. Use `SC_BROWSER` to select an existing Chromium executable. The browser test starts and closes its own local HTTP server. Screenshots are written under `build-web/test-artifacts/`. Web query parameters `?seed=713&scene=hall` provide the same inspection workflow. Scene/seed overrides begin a fresh expedition; use them in a separate browser profile when keeping an existing expedition matters.

The core suite checks 384 district layouts across 128 seeds for connected routes, disjoint footprints, loops, matching portals and traversal requirements. Collision tests walk connector paths and check floors; streaming tests cover load-order independence, fair scheduling, publication of complete halls, backtracking reuse, 100 moves with separated players, and persistent kills. Combat tests exercise charged shots, dodging, swept player/wall collision, nearest-player hits in co-op, surface climbing, and fast pursuit. Expansion tests cover all 28 themes, 27 species, 20 personal weapons, three mech weapons, rig driving/plating, attached flares, defense/attribute effects, shared loot ownership, the full mission chain, contracts, and version-10 persistence. Interior tests also check 640 plans for connected incident trails, all floor-plan and landmark variants for walking/mech clearance, both entrances, caches and deterministic geometry, plus pre-change generator-3 and generator-4 fingerprints and geometry. Save tests include real version-1 and pre-infestation version-2 and compatible version-3/version-4/version-5/version-6 fixtures, generator mismatch rejection, and distant coordinates. Browser tests exercise actual WASM/WebGL, native/WASM generation parity, crossing chunk boundaries inside a hall, distant split-screen cameras, IndexedDB save/reload, migration, wall-climbing and projectile rendering, keyboard/menu/touch flashlight controls, six new sectors, mission interaction and reward spending, journal and weapon switching, occupied-rig/flare reload, beam rendering, mobile rig controls, mobile multitouch, and resizing. Real mobile GPU performance, Safari compatibility, physical gamepads, and Windows graphics still need device testing.

**In the managed cloud workspace**, `scripts/setup-cloud.sh` prepares local build tools and graphics headers without requiring a system installation. It uses signed Debian package metadata and the allowed Debian/Python/GitHub download hosts. It is specific to the Debian 13 cloud image. After setup:

```sh
. /workspace/.starship-tools/activate.sh
cmake --build build --parallel 4
cmake --build build-web --parallel 4
ctest --test-dir build --output-on-failure
SC_BROWSER=/usr/lib/chromium/chromium python3 tests/test_browser.py
```

The engine modules are `habitat.c` (macro footprints, local conditions, environmental evidence, sprite anchors and enemy ecology), `interior.c` (furnished floor plans, landmark geometry and incident details), `content.c` (room, enemy, weapon, and lore definitions), `progression.c` (attributes, defenses, loot, contracts, and legacy missions), `history.c` (shared seeded causes, controllers and survivor agendas), `story.c` (discoveries, decisions and conclusions), `expedition.c` (reactive operations, factions, continuing arcs, debriefs and careers), `generation.c` (pure district plans and connector contracts), `world.c` (chunk geometry, streaming, population, collision, and world records), `game.c` (movement, combat, and progression), `save.c` (versioned persistence), `render.c` (chunk meshes, independent camera origins, and HUD), and `main.c` (SDL lifecycle, input, and mixed audio). The core has no SDL dependency. Players and enemies use integer sectors plus local double coordinates. Every static mesh uses chunk-local coordinates and receives a separate camera-relative offset for each viewport.

Milestone 3 is implemented. See [WORLD_GENERATION.md](WORLD_GENERATION.md) for the generation rules, streaming limits, persistence policy, and diagnostics. Generator 5 makes the ship history affect room composition, origin apparatus, connected damage and survivor/controller notices. Generator 4 introduced four ship profiles, three floor-plan families, connected incident trails and four rare landmark interiors. Generator 10 expands this with eight macro footprints, paired-deck vaults and galleries, environmental accounts, matching enemy ecologies and bounded atmospheric sprites. See [LIVING_SHIP.md](LIVING_SHIP.md). Richer enemy navigation, traps, riddles, armor item inventories, additional gravity anomalies and comprehensive appearance customization remain in [the development plan](DEVELOPMENT_PLAN.md). Generation and uploads are capped by work units per frame; a sustained frame-rate target still requires profiling on named physical devices.

Enemy defeats now leave animated bodies and debris. Some leave finite acid residue, unstable cores or proximity mines; warning rings and HUD captions show when to retreat. Machines include the Choir Surveyor, Graveyard Loader, Suture Custodian, Coffin Tread and Martyr Mine. New ships use the expanded roster; old ships retain their original encounters. See [FIELD_GUIDE.md](FIELD_GUIDE.md) for effects and [HACKING.md](HACKING.md) for the implemented terminal/matrix layer.
