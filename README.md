Starship Crawl is a playable first-person prototype in C17, SDL3, and OpenGL / WebGL 2. Enter an enormous gothic ship from space, explore its infested decks, fight strange creatures, and grow a persistent character.

**Play begins with one player.** Split-screen is optional: press F2, or use the browser menu, to explicitly add a second player. Loading a save always starts in solo mode and retains the second character's progression.

The prototype includes:

- A space approach and safe level-0 loading bay with left, right, inward, upper, and lower routes.
- Nine themed ship sectors and 28 room themes, including luxury resorts, command decks, research wards, cryonics, cargo, and botanical spaces, with connected routes, loops, and vertical grav lifts.
- Landmark halls spanning four chunks (180 × 180 × 58 metres), cabin clusters inside single chunks, crouching passages, and atriums with lighter gravity.
- Incremental chunk generation and GPU uploads, retained neighborhoods during backtracking, and one shared bounded cache for optional co-op.
- Rusted panels, damaged vents, hanging cables, reactor pipework, cabin furnishings, egg nests, and branching growths in generated rooms.
- Dark district materials, mostly dead light fittings, weak colored pools of flickering light, a player flashlight, ambient drones, and an F3 district map with streaming diagnostics.
- Movement, collision, jumping, crouching, sprinting, and grav lifts that activate checkpoints.
- 27 enemy species built from climbing, leaping, charging, strafing, armor, directional wards, regeneration, healing, brood spawning, cloaking, hovering, turrets, bursts, frost, energy drain, and explosive attacks.
- 16 weapons with spread, piercing, rockets, plasma, burning, freezing, chaining, and life siphoning; regenerating weapon energy keeps combat moving.
- Ten upgradeable attributes, armor durability, a regenerating energy shield, shared collectible caches, salvage currency, and 12 ship records.
- A six-stage story expedition, three side contracts, an end boss, beacon extraction, and persistent victory counts.
- Three armor/weapon colors, forgiving respawns, new ships that retain character progression, and persistent defeated enemies.
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

**GitHub Pages deployment.** The `Build, test, and publish` workflow builds the web game, stages `index.html` with its JavaScript and WASM files, and exercises the complete browser suite under `/starship_crawl/`. Successful runs on `main` publish the tested artifact through the official GitHub Pages actions. Pull requests build and test without deploying. In repository Settings → Pages, set the publishing source to **GitHub Actions** once. The expected project address is `https://stbange.github.io/starship_crawl/`; the Actions deployment result confirms when it is live.

To inspect the exact static site locally:

```sh
python3 scripts/package-web.py --output build-pages-preview/starship_crawl
python3 tests/test_browser.py --site-dir build-pages-preview --url-path /starship_crawl/
```

**Controls**

| Action | Keyboard / mouse | Gamepad |
| --- | --- | --- |
| Move | WASD | Left stick |
| Look | Mouse | Right stick |
| Fire | Hold left mouse | Right trigger |
| Toggle flashlight | F | Right stick click |
| Jump / rise outside | Space | A / south button |
| Crouch / descend outside | Left Ctrl | B / east button |
| Sprint | Left Shift | Left shoulder |
| Grav lift up / down | E / Q while on the cyan pad | X / Y |
| Switch weapon | Z / X or mouse wheel | Right shoulder |
| Use shelter door, recovery station, or mission terminal | G | D-pad down |
| Deploy / pack empty shelter | R | Back / Select |
| Deck map / close map | M; arrows pan, Page Up/Down change deck, Home recenters | Left stick click; D-pad pans, shoulders change deck |
| Veil / flight / long jumps / nova | V / B / J / H | Hold left trigger + D-pad left / up / right / down |
| Character / mission journal | I; Enter closes | Browser menu or keyboard |
| Read next collected record | L while in journal | Browser menu or keyboard |
| Upgrade attribute 1 through 10 | 1 through 9, then 0 | D-pad left / up / right for the first three |
| Change armor color | C | Browser menu or keyboard |
| Pause / resume | Esc, Tab, Enter | Start |
| Optional second player | F2 | Browser menu or keyboard |
| Save / load | F5 / F9 | Browser menu or keyboard |
| New ship | N, then Enter to confirm; Esc cancels | Browser menu or keyboard |
| District map / streaming diagnostics | F3 | Keyboard |

With one gamepad and split-screen enabled, keyboard/mouse controls player one and the gamepad controls player two. With two gamepads, each player has one. Controller disconnects pause the game. Mobile uses a left movement pad, right-side drag to look, and on-screen fire/jump/crouch/lift/lamp buttons; upgrades and saves are available in the browser menu. Each player's flashlight starts on and can be toggled independently, without a battery limit.

Spitters visibly charge for 0.8 seconds before launching a slow glowing shot toward the position they aimed at. Keep moving sideways to evade it. Crawlers climb nearby walls and machinery before dropping back into pursuit. Health regeneration, the safe loading bay, and checkpoint respawns keep early exploration forgiving.

Grav lifts currently transfer instantly between decks and establish a checkpoint. Death retains experience, skills, and equipment, restores health, and returns that player to the latest checkpoint. New expeditions replace the ship while retaining both characters. Re-entering a generated room preserves defeated enemies; partially injured enemies reset when their room unloads.

**The game uses generator 3 and save format 5.** Version-1 and version-2 saves preserve both characters' existing ranks, experience, skills, points, kills, and colors, and visibly start a fresh expedition because the ship layouts and enemy identities changed. New characters and migrated profiles begin with a carbine, shotgun, nail repeater, armor, and a shield. Existing version-3 and version-4 expeditions retain their ship and progress when loaded. Version-5 saves also retain psionic effects, the fold shelter, interior locations, return anchors, and recovery timers, alongside both profiles, weapon ownership, defensive resources, collected records, consumed caches, defeated enemies, missions, contracts, and victory counts. Loading opens solo with the flashlight on and clears in-flight shots. Unknown versions or corrupt data leave the current game unchanged.

Start the story at the gold-marked console beside the loading bay's lift. Press **G** nearby, then follow the target cell and deck in **I**. Use **E/Q** to change decks from a cyan lift pad. Each mission awards upgrade points and salvage; later missions unlock weapons. Read [EXPANSION.md](EXPANSION.md) for the sector guide, combat systems, attribute effects, and mission route.

**Navigation and psionics.** The top-left minimap marks the mission goal, your facing, lifts and the shelter. **M** opens a deck schematic; use arrows to pan, **Page Up/Down** for decks and **Home** to recenter. Goals outside the view stay marked at the edge, and deck differences are labelled. **V** veils you until firing, **B** enables flight, **J** charges long jumps, and **H** emits a damaging nova aura. All use rechargeable energy. Enemy bodies now cover five families and five structural variants each; all 16 weapons fire visible projectiles. See [FIELD_GUIDE.md](FIELD_GUIDE.md) for controls, variants and ability details.

**Fold shelter.** Press **R** on clear ship flooring to summon a small doorway, or press it again to pack an empty shelter. Use **G** nearby to enter an 18 × 16 metre sanctuary. Use **G** beside the bed to rest, the medical station to heal and repair armor, or the kitchen to cook a recovery meal. Return through the interior door to your entry position. Touch players have **FOLD** and **USE** buttons; the browser menu also offers summoning. Enemies stay outside, and the exterior continues for an outside co-op player. The doorway cannot move while occupied. See [SHELTER.md](SHELTER.md).

**Saves** are versioned, checksummed, and validated before replacing the current session. Native writes replace a temporary file; the default location comes from SDL's application preference directory (`Stbange/StarshipCrawl`). `--save-dir DIRECTORY` selects another location. Browser saves are local to the site's origin and browser profile; the HUD confirms completion after IndexedDB synchronization. Export a save to move it to another browser or device. A successful disk write alone is not reported as completed browser persistence.

Autosaves occur every 20 seconds of active play. Save explicitly before closing a tab; asynchronous browser writes cannot be guaranteed during tab shutdown. Imported saves with an unsupported version or corrupt payload leave the running expedition unchanged.

**Specifications and automated quality.** OpenSpec 1.16.0 and Codex workflows are checked in, with validated specifications under `openspec/specs`. Native unit tests measure **98.20% core line coverage** and enforce **95% overall and in every measured source file**. A separate automated campaign plays three seeds through all six objectives and extraction; a shelter journey walks through every recovery facility and returns to the ship. The existing browser suite tests the compiled WASM/WebGL game, input, saves, and mobile controls. Coverage reports and browser captures are retained in CI, and Pages requires all quality, native, and web jobs to pass. See [TESTING.md](TESTING.md) for commands, scope, reports, and the specification workflow.

**Development commands**

```sh
# Reproducible expedition, without changing your normal save.
./build/starship_crawl --seed 713 --no-save

# Inspect a specific room type.
./build/starship_crawl --seed 713 --scene hall --no-save
# Other scenes: bay, engine, crew, tunnel, atrium, distant, encounter.
# New sectors: luxury, bridge, lab, cargo, cryonics, botany.
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

The core suite checks 384 district layouts across 128 seeds for connected routes, disjoint footprints, loops, matching portals and traversal requirements. Collision tests walk connector paths and check floors; streaming tests cover load-order independence, fair scheduling, publication of complete halls, backtracking reuse, 100 moves with separated players, and persistent kills. Combat tests exercise charged shots, dodging, swept player/wall collision, nearest-player hits in co-op, surface climbing, and fast pursuit. Expansion tests cover all 28 themes, 27 species, 16 weapons, defense/attribute effects, shared loot ownership, the full mission chain, contracts, and version-5 persistence. Save tests include real version-1 and pre-infestation version-2 and compatible version-3/version-4 fixtures, generator mismatch rejection, and distant coordinates. Browser tests exercise actual WASM/WebGL, native/WASM generation parity, crossing chunk boundaries inside a hall, distant split-screen cameras, IndexedDB save/reload, migration, wall-climbing and projectile rendering, keyboard/menu/touch flashlight controls, six new sectors, mission interaction and reward spending, journal and weapon switching, version-5 reload, mobile multitouch, and resizing. Real mobile GPU performance, Safari compatibility, physical gamepads, and Windows graphics still need device testing.

**In the managed cloud workspace**, `scripts/setup-cloud.sh` prepares local build tools and graphics headers without requiring a system installation. It uses signed Debian package metadata and the allowed Debian/Python/GitHub download hosts. It is specific to the Debian 13 cloud image. After setup:

```sh
. /workspace/.starship-tools/activate.sh
cmake --build build --parallel 4
cmake --build build-web --parallel 4
ctest --test-dir build --output-on-failure
SC_BROWSER=/usr/lib/chromium/chromium python3 tests/test_browser.py
```

The engine modules are `content.c` (room, enemy, weapon, and lore definitions), `progression.c` (attributes, defenses, loot, contracts, and story), `generation.c` (pure district plans and connector contracts), `world.c` (chunk geometry, streaming, population, collision, and world records), `game.c` (movement, combat, and progression), `save.c` (versioned persistence), `render.c` (chunk meshes, independent camera origins, and HUD), and `main.c` (SDL lifecycle, input, and mixed audio). The core has no SDL dependency. Players and enemies use integer sectors plus local double coordinates. Every static mesh uses chunk-local coordinates and receives a separate camera-relative offset for each viewport.

Milestone 3 is implemented. See [WORLD_GENERATION.md](WORLD_GENERATION.md) for the generation rules, streaming limits, persistence policy, and diagnostics. The current district library uses axis-aligned architecture and one four-chunk landmark per region; more landmark shapes and structures spanning multiple decks belong to the content expansion milestone. Richer enemy navigation, traps, riddles, armor item inventories, animated lifts, and comprehensive appearance customization remain in [the development plan](DEVELOPMENT_PLAN.md). Generation and uploads are capped by work units per frame; a sustained frame-rate target still requires profiling on named physical devices.
