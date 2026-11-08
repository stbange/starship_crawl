# Quest 3 WebXR build

Starship Crawl has an opt-in **standalone Quest Browser** target. The headset
runs the C game through WebAssembly and renders through WebGL 2 and WebXR.
There is no store submission, APK, PC streaming requirement or native OpenXR
dependency. The ordinary Linux, Windows and web applications keep their
existing entry points, controls and storage.

After a successful main-branch Pages deployment, open
**https://stbange.github.io/starship_crawl/quest/** in **Meta Quest Browser**.
Hold both Touch controllers, face forward in your seated or standing position,
and choose **Enter VR**. Accept the browser's immersive-session prompt. The
regular web game remains at https://stbange.github.io/starship_crawl/.

This first VR target is single-player. It uses stick travel and bounded tracked
leaning; it does not implement teleport locomotion, free room-scale walking,
hand tracking or motion-based melee. An imported split-screen save opens with
one player under the existing save-loading rules.

## Controls

| Control | In the expedition | In a floating menu |
| --- | --- | --- |
| Headset | Look and lean, including while paused | Look around the anchored panel |
| Left stick | Move relative to head direction | — |
| Left stick click | Sprint while held | — |
| Right stick left/right | 45-degree snap turn; release before turning again | — |
| Right stick up/down | — | Select an item; release between steps |
| Right trigger | Fire along the right controller's aim ray | Confirm selected item |
| Right grip | Interact, investigate, board/dismount, use a shelter station | — |
| A | Jump/rise; request a lift up | Confirm selected item; resume after tracking recovery |
| B | Hold to crouch/descend; request a lift down | Return to main menu |
| X | Next weapon; hold left grip for previous weapon | — |
| Y | Open menu | Resume expedition |
| Left trigger | Toggle left-controller flashlight | — |
| Left grip | Show wrist survival display and objective map | — |
| Right stick click | Recenter current seated/standing position | Recenter panel and tracking |

The menu provides story decisions and records, guardian rewards, loadout refits,
character upgrades, weapons, psionics, shelter/mech/wagon deployment, flare
upgrades, matrix programs/services, maps, saving and exit. **Next text page**
reveals the rest of long accounts. All actions use ordinary progression and
location requirements. A new ship requires a separate confirmation in the menu
and starts a new character. Matrix pulses, projectile weapons and beams all aim
from the controller; aiming a gun does not rotate the view.

Physical crouching lowers the tracked view and the collision stance; B is
available for seated play. Recenter after changing between seated and standing.
Stay near the calibrated position and use the stick to travel. Leaning farther
than 1.5 metres horizontally, excessive height changes or pushing the head
through blocking geometry obscures the world. Discontinuous transitions and
snap turns receive a short fade. The movement vignette can be disabled in the
headset menu or launch page. Render scale applies to the next session.

Removing a controller, losing head tracking, a reference-space reset or hiding
the session pauses gameplay and clears held inputs. When tracking returns,
release all buttons/sticks and press A to resume. A graphics-context failure
requires the launch page's **Reload and recover saved expedition** button.

## Saves

Quest uses `/quest-persist` in IDBFS; ordinary web continues using `/persist`.
Comfort preferences use `starship-crawl.quest.preferences.v1` in local storage.
The two games can run on the same origin with different expeditions. Quest
autosaves and saves on normal VR exit. Export a backup from the launch page
before clearing browser data or replacing an expedition.

The launch page imports and exports the normal `.sav` format. Import explicitly
replaces only the Quest expedition, retains earned capabilities according to
the existing compatibility rules, and rejects malformed/unsupported files
transactionally. Generator 14, simulation 1, narrative 3, matrix 1 and save 15
are unchanged. Older supported saves keep their existing rules and migrations.

## Build and distribution

Activate Emscripten (CI uses 4.0.23), then:

```sh
emcmake cmake -S . -B build-quest -DSC_BUILD_QUEST=ON -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-quest --target starship_crawl_quest --parallel 4
python3 scripts/package-quest.py --output build-pages/quest
```

Artifacts are `starship_crawl_quest.html`, `.js` and `.wasm`. The packager produces
`quest/index.html` and refuses to overwrite an ordinary web root. Serve the
directory over HTTPS; remote plain HTTP and opening the files directly do not
provide a usable immersive WebXR origin. GitHub Actions retains a downloadable
**quest-webxr** artifact and publishes the tested page under `/quest/` after all
quality, native, ordinary-web and Quest checks pass.

`SC_BUILD_QUEST` defaults to **OFF** and is accepted only for an Emscripten game
build. The `SC_WEBXR` definition belongs to `sc_quest_core` and its application;
it never propagates to `sc_core`, the ordinary application or the shared SDL
platform library. Existing native and web build commands require no change.

Headless checks do not require SDL or a headset:

```sh
cmake -S . -B build-quest-unit -DSC_BUILD_GAME=OFF -DSC_QUEST_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-quest-unit --parallel 4
ctest --test-dir build-quest-unit -L quest --output-on-failure
python3 scripts/check-coverage.py --quest --build-dir build-quest-coverage
```

See [TESTING.md](TESTING.md) for the packaged device-emulation test and
[the Quest 3 checklist](docs/playtests/QUEST3_CHECKLIST.md) for physical validation.

## Current validation limits

The automated device supplies standard WebXR poses, controllers and session
events outside the production application. Tests execute the real WASM game
and render distinct eye images into a real WebGL framebuffer. Native tests
cover tracking geometry, invalid poses, cooldown/energy rules, controller rays
through walls and unchanged ordinary-control saves.

**A physical Quest 3 has not been tested in this environment.** The application
requests 72 Hz only if the headset runtime advertises that rate; this is a
request, not a measured performance claim. Sustained frame rate, readable text,
comfort, controller ergonomics, audio and thermal behavior need the hardware
checklist. Existing field-message prose can still mention desktop key names;
the controller menus provide the corresponding actions. Audio retains the
existing mix and is not spatialized binaural VR audio.
