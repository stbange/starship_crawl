Starship Crawl is a playable first-person prototype in C17, SDL3, and OpenGL / WebGL 2. Enter an enormous gothic ship from space, explore generated decks, fight geometric enemies, and grow a persistent character.

**Play begins with one player.** Split-screen is optional: press F2, or use the browser menu, to explicitly add a second player. Loading a save always starts in solo mode and retains the second character's progression.

The prototype includes:

- A space approach and safe level-0 loading bay with left, right, inward, upper, and lower routes.
- Seeded room streaming across multiple decks: cathedral halls, reactors, crew quarters, crouching passages, and gilded atriums with lighter gravity.
- Movement, collision, jumping, crouching, sprinting, and grav lifts that activate checkpoints.
- Three geometric enemy roles, a pulse carbine, hit feedback, procedural weapon audio, experience, and damage/health/fire-rate upgrades.
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

SDL is linked statically. A Windows x86-64 executable has also been cross-compiled with MinGW-w64; it imports only standard Windows system DLLs. The CI configuration builds both Windows and Linux. Actual Windows runtime and physical controller testing remain separate checks from compilation.

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
| Jump / rise outside | Space | A / south button |
| Crouch / descend outside | Left Ctrl | B / east button |
| Sprint | Left Shift | Left shoulder |
| Grav lift up / down | E / Q while on the cyan pad | X / Y |
| Damage / health / rate upgrade | 1 / 2 / 3 | D-pad left / up / right |
| Change armor color | C | Browser menu or keyboard |
| Pause / resume | Esc, Tab, Enter | Start |
| Optional second player | F2 | Browser menu or keyboard |
| Save / load | F5 / F9 | Browser menu or keyboard |
| New ship | N, then Enter to confirm; Esc cancels | Browser menu or keyboard |
| Debug display | F3 | Keyboard |

With one gamepad and split-screen enabled, keyboard/mouse controls player one and the gamepad controls player two. With two gamepads, each player has one. Controller disconnects pause the game. Mobile uses a left movement pad, right-side drag to look, and on-screen fire/jump/crouch/lift buttons; upgrades and saves are available in the browser menu.

Grav lifts currently transfer instantly between decks and establish a checkpoint. Death retains experience, skills, and equipment, restores health, and returns that player to the latest checkpoint. New expeditions replace the ship while retaining both characters. Re-entering a generated room preserves defeated enemies; partially injured enemies reset when their room unloads.

**Saves** are versioned, checksummed, and validated before replacing the current session. Native writes replace a temporary file; the default location comes from SDL's application preference directory (`Stbange/StarshipCrawl`). `--save-dir DIRECTORY` selects another location. Browser saves are local to the site's origin and browser profile; the HUD confirms completion after IndexedDB synchronization. Export a save to move it to another browser or device. A successful disk write alone is not reported as completed browser persistence.

Autosaves occur every 20 seconds of active play. Save explicitly before closing a tab; asynchronous browser writes cannot be guaranteed during tab shutdown. Imported saves with an unsupported version or corrupt payload leave the running expedition unchanged.

**Development commands**

```sh
# Reproducible expedition, without changing your normal save.
./build/starship_crawl --seed 713 --no-save

# Inspect a specific room type.
./build/starship_crawl --seed 713 --scene hall --no-save
# Other scenes: bay, engine, crew, tunnel, atrium.

# Run core checks without SDL or a graphics device.
cmake -S . -B build-core -DSC_BUILD_GAME=OFF -DSC_SANITIZE=ON
cmake --build build-core --parallel 4
ctest --test-dir build-core --output-on-failure

# Render actual native frames using a headless Mesa driver.
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy \
  ./build/starship_crawl --seed 713 --smoke 10 --capture build/arrival.ppm

# Exercise the built browser application with Chromium.
python3 -m pip install playwright
python3 -m playwright install chromium
python3 tests/test_browser.py
```

Use `SC_BROWSER` to select an existing Chromium executable. The browser test starts and closes its own local HTTP server. Screenshots are written under `build-web/test-artifacts/`. Web query parameters `?seed=713&scene=hall` provide the same inspection workflow. Scene/seed overrides begin a fresh expedition; use them in a separate browser profile when keeping an existing expedition matters.

The core suite exercises 24 generation seeds, shared portals, load-order independence, active streaming bounds for separated players, persistent kills, gravity, crouch clearance, lifts, respawns, raycast combat, upgrades, solo/co-op transitions, and save failure handling. The browser suite exercises actual WASM and WebGL rendering, keyboard movement, firing, browser save/reload, split-screen toggling, rejected imports, all five interior types, simultaneous touch movement/fire, and resizing. Real mobile GPU performance, Safari compatibility, physical gamepads, and Windows runtime behavior still need device testing.

**In the managed cloud workspace**, `scripts/setup-cloud.sh` prepares local build tools and graphics headers without requiring a system installation. It uses signed Debian package metadata and the allowed Debian/Python/GitHub download hosts. It is specific to the Debian 13 cloud image. After setup:

```sh
. /workspace/.starship-tools/activate.sh
cmake --build build --parallel 4
cmake --build build-web --parallel 4
ctest --test-dir build --output-on-failure
SC_BROWSER=/usr/lib/chromium/chromium python3 tests/test_browser.py
```

The engine modules are `world.c` (deterministic cells, geometry, collision, records), `game.c` (players, combat, progression), `save.c` (portable persistence), `render.c` (OpenGL rendering and HUD), and `main.c` (SDL lifecycle and input). The core has no SDL dependency. Simulation coordinates use doubles; rendered positions are relative to the streamed world origin. Meshes rebuild when the active region set changes, with one simulation shared by optional player cameras.

This is the first playable foundation. Generation currently assembles six room families on a connected cell grid. District-level composition, halls spanning several generation cells, authored landmarks, richer enemy navigation, traps, riddles, full equipment inventories, animated lifts, and comprehensive appearance customization remain in [the development plan](DEVELOPMENT_PLAN.md). Generation and mesh rebuilding currently run synchronously; streaming and lighting need further profiling before claiming a mobile performance target.
