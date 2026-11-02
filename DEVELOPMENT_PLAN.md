Starship Crawl is a first-person, procedurally generated space dungeon crawler written in C. The player begins floating in space before the open loading bay of a gothic ship so enormous that its hull resembles an endless wall. Exploration leads through dramatically different spaces, from cramped maintenance tunnels to immense halls. Combat emphasizes slaying hordes, gaining power, and choosing how far into danger to travel.

This document records the agreed direction and the longer-term implementation plan. The README describes the implemented prototype and its current validation status.

The agreed design is:

- First-person movement, with normal gravity through most of the ship and different gravity conditions in selected areas.
- The loading bay is level 0 and offers routes left, right, upward, downward, and farther into the ship. Players can backtrack after entering.
- Characters persist between newly generated ships. Death returns the player to a checkpoint while retaining character progression.
- Character builds start with equipment and freely chosen skills, accompanied by simple appearance customization.
- Linux, Windows, and browsers through Emscripten are target platforms. Mobile browsers should have on-screen controls.
- Multiplayer is local split-screen. Players share one machine and one simulated world.
- Graphics are dark, gloomy, retro, and pixelated. Early enemies use basic geometry and distinct colors.
- World generation and architectural variety are central features, with strong differences in room scale.

The initial defaults below resolve details that were not specified. They can be adjusted without changing the overall direction:

- Single-player is the default and the initial gameplay-testing priority. Split-screen is optional and activates only through an explicit join action. Desktop and browser co-op support keyboard and mouse for one player plus a gamepad for the other, or two gamepads. A resumed session opens in solo mode while retaining the second character profile.
- Mobile touch controls initially support one player. Two-player browser support remains part of the engine; two simultaneous touch layouts are a later extension rather than the mobile baseline.
- A new expedition creates a new ship seed. Saving and resuming, character death, or a second player joining do not regenerate the active ship. Starting a new expedition is an explicit action that preserves both players' characters.
- The first death policy retains experience, skills, and equipped items, and respawns at the last activated checkpoint. A surviving partner continues playing. Exact respawn timing and temporary penalties are tuning decisions.
- Each local player can select a persistent character profile. Guests can play without overwriting an existing profile. The expedition save records the shared ship and participating character identities.
- Ship difficulty is based on three-dimensional spatial distance from the entrance, with a safe starting region. Additional players affect encounter budgets in their shared encounter area, not the geography or base difficulty of the ship.

The technology foundation is C17, SDL3, CMake, and OpenGL. Linux and Windows use an OpenGL 3.3 core context; Emscripten uses OpenGL ES 3 through WebGL 2. The renderer uses the feature set shared by these targets, with small platform-specific shader headers and context setup. WebGL 2 is required; an unsupported browser receives a clear capability message.

Use a small engine tailored to this game. SDL handles platform integration, events, gamepads, touch, timing, and audio. Focused C libraries can provide math, desktop GL loading, image decoding, and later glTF loading. Pin stable dependency and toolchain versions when implementing the build, check licenses, and keep dependencies small enough for browser delivery. No complete game framework is required.

The renderer, collision, streaming, procedural generation, entity simulation, combat, and saves belong to the project. Keep them in separate modules with explicit ownership. Gameplay consumes player actions rather than raw keyboard, controller, or touch events. Simulation runs once per fixed timestep; rendering runs once for each active player camera. A monolithic game source file would make these responsibilities difficult to maintain.

The implementation sequence is:

1. **Establish portable builds, input, and multiple views.**

   Create the CMake project and pinned dependency setup. Build and launch the same C application on Linux and through Emscripten, and establish a Windows build check. Use SDL's application callbacks or an equivalent nonblocking browser-compatible loop. Bound fixed-step catch-up after focus loss or suspended browser tabs.

   Render a simple 3D scene, add mouse and gamepad camera control, and render two independent viewports from one world state. Give each viewport its own camera, aspect ratio, resolution setting, and HUD. Add a basic mobile touch layout: movement stick on the left, look area on the right, and buttons for firing, interacting, jumping, and crouching. Track multiple fingers independently. Test gamepad disconnects, touch cancellation, resizing, and orientation changes.

   Establish browser startup from a user gesture, audio activation, desktop pointer capture, focus handling, and a small persistent-save round trip before substantial gameplay code depends on them. The standard web build should run without threading or cross-origin-isolation requirements. Generation work will therefore support incremental execution within a frame budget.

   Completion means a controllable 3D scene works in the native and browser builds, two players can look in different directions without controlling each other, and mobile movement and looking can operate simultaneously. Windows compilation and actual Windows runtime validation are reported separately.

2. **Build the arrival and prove architectural scale.**

   Construct the enormous exterior hull and open loading bay. The hull extends laterally and vertically beyond visibility; ship interiors extend inward. Start the player outside, floating toward the entrance. Transition into normal gravity inside the bay. Define gravity as a property of a region so later low-gravity and zero-gravity spaces fit the movement system.

   Build a compact authored layout containing all five loading-bay route directions, a crew corridor, an engine chamber, a giant hall, and a crouching maintenance passage. Use stairs, ramps, shafts, and elevators to make vertical routes usable under normal gravity. Begin with reliable player collision against simple environment geometry, including floors, walls, doors, slopes, and crouch clearance. Prevent standing up into a low ceiling.

   Establish the retro presentation using a low-resolution world render target, nearest-neighbor upscaling, restrained dithering, distance fog, strong silhouettes, and localized lighting. Draw readable interface elements at a separate resolution. Communicate scale with consistent doors, railings, pillars, windows, machinery, sightlines, and spatial sound. Darkness must still permit navigation and enemy recognition.

   Completion means players can enter the ship, traverse every branch, backtrack, and feel the transition between an immense hall and a restrictive tunnel. Both player cameras must behave correctly when one player is outside and the other is inside.

3. **Generate and stream a coherent ship.**

   Implemented in generator version 2: three district families, reserved four-chunk landmarks, cabin clusters, a connected backbone with loops and matching portal contracts, fair bounded streaming with hysteresis, chunk-local meshes, independent camera origins, and persistent encounter changes. Version-1 characters migrate to a fresh expedition. The core and browser suites validate these behaviors; physical device performance targets remain part of milestone 6. See [WORLD_GENERATION.md](WORLD_GENERATION.md) for current rules and limits.

   Separate generation into district selection, structural layout, room and passage construction, and population. Initial district families are crew quarters, machinery, and great halls. Expand to luxury areas, storage, religious spaces, and unusual landmarks after the basic system produces convincing combinations.

   Districts determine architectural rules, room distributions, lighting, materials, sounds, and encounter possibilities. Layout generation creates a reachable backbone with branches, loops, and vertical connectors. Room definitions describe dimensions, occupied space, connector position and size, traversal requirements, and permitted neighbors. Traps, rewards, and decoration are placed only after structural validity is established.

   Streaming chunks are storage and rendering units, not fixed-size rooms. A large hall may occupy many chunks, while one chunk may contain several small rooms. Larger generation regions own multi-chunk structures and reserve their footprints. Shared boundaries use deterministic connector decisions; generating neighboring chunks in a different order must not produce mismatched doors or overlapping structures.

   Use a fixed-width, explicitly specified random generator and hashes of the ship seed, generator version, region coordinates, and content category. Avoid platform-dependent randomness and generation order dependencies. Store large-scale positions as integer region coordinates with local positions for physics and rendering. Use camera-relative coordinates for each viewport so rendering precision remains stable even when players travel far apart.

   Load and retain the union of the areas needed by both players, with hysteresis to prevent repeated load/unload churn. Schedule incremental generation fairly around both cameras and avoid exposing unfinished geometry to traversal. Share overlapping chunks. Keep rendering and GPU uploads on the main thread. Dormant regions leave the active simulation while their persistent changes remain available.

   Completion means exploration continues beyond the starting area with bounded active memory and simulation work. Revisiting reconstructs the same architecture. Two players can explore different regions without their surroundings unloading or generating twice. Persistent save storage may grow with explored changes even though active memory is bounded.

4. **Make combat and progression enjoyable.**

   Add a responsive weapon, hit feedback, pickups, experience, and several upgrades with visible effects on combat. Start with swarming melee enemies, ranged enemies, and heavy enemies represented by different basic shapes and colors. Build enemy movement on room connections and local navigation data; use shared navigation work for hordes where practical.

   Difficulty grows with distance from the entrance. Define understandable distance bands and tune enemy health, damage, density, compositions, and rewards together. Keep early regions useful as places where stronger characters can feel powerful. New behaviors and hazards provide additional challenge alongside statistical growth. Maintain encounter and simulation budgets even at large distances.

   Both players participate in one simulation, with per-player damage, inventory, experience, and HUD state. Award participation rewards without allowing duplicate loot claims or duplicate spawns in overlapping active regions. Add checkpoint activation and forgiving respawns. Define a single mixed audio output that remains intelligible when the two camera positions differ.

   Completion means a short solo or cooperative session includes enjoyable battles, meaningful upgrades, a deeper and more dangerous region, and a successful checkpoint respawn that retains progression.

5. **Complete character profiles and expedition persistence.**

   Add character selection, simple appearance choices, equipment, and flexible skill selection. Appearance changes should be visible through first-person equipment or hands and through the other player's character model. Use complete default character profiles so customization never blocks starting a game.

   Keep character records separate from expedition records. Save the seed, generator version, checkpoint state, world changes, player positions, and other state needed to resume an expedition consistently. Regenerate unchanged geometry from the seed. Persist opened doors, collected unique rewards, defeated persistent encounters, and other changes according to explicit gameplay rules; unloading a region must not become a reward duplication mechanism.

   On desktop, use an application save directory and atomic replacement where supported. In browsers, use IndexedDB-backed storage with initialization before loading saves and asynchronous persistence confirmation after writing them. Show save completion only after persistence succeeds, save during play, and do not rely on tab-close callbacks. Include save import/export for backup and moving profiles between devices. A shared file format supports transfer without adding online services.

   Version both saves and generation rules. Do not silently resume an old expedition using incompatible generation rules. Test the joint save of shared world changes and individual progression so a partial save cannot duplicate rewards or erase a character's gains.

   Completion means closing and reopening the game restores the same active ship and both character profiles. Starting a new expedition creates different architecture while retaining progression. Browser saves survive a page reload, and export/import restores a profile and expedition as documented.

6. **Expand the world and validate sustained play.**

   Add luxury districts, distinctive landmarks, larger multi-level structures, more module variants, rare spaces, and gravity anomalies. Introduce traps and short riddles first in optional reward routes. Main exploration routes remain traversable and the combat pace remains energetic.

   Give each district a recognizable architectural vocabulary and progression of spaces. Use authored landmarks within procedural layouts, deliberate transitions between district types, and room arrangements that create memorable views. Generation quality is measured by interesting routes, coherent spaces, surprise, and scale, as well as validity.

   Add generator inspection tools for seeds, region boundaries, connectors, occupied footprints, navigation, difficulty, and active simulation areas. Batch-test many seeds for matching seams, valid collision clearances, reachable exits, stable generation across platforms, and repeatability regardless of load order. Compare semantic generation records across native and web builds rather than expecting identical rendered pixels.

   Profile long sessions with dense combat, giant halls, two separated players, and mobile browsers. Use per-camera visibility checks, batching, instancing, and room connectivity to control rendering work. Allow separate internal render resolution and view distance settings for mobile and split-screen. Establish frame-time and memory targets against named reference devices once representative scenes exist.

   Completion means the chosen test hardware sustains a documented quality setting, active memory remains bounded during extended travel, save/resume works after long exploration, and both solo and split-screen preserve the intended combat and atmosphere.

The first playable release combines milestones 1–4 with basic character and expedition saves from milestone 5. It includes the space approach, level-0 loading bay, five initial route directions, three generated district families, a giant hall and a crouching passage, three enemy roles, one satisfying weapon, a small upgrade set, checkpoints, and one- or two-player play. Linux, Windows, desktop web, and mobile touch are exercised throughout development rather than deferred to a final port.

The practical constraints are rendering each split-screen view, bounded browser memory, asynchronous browser saves, and producing coherent structures that span chunk boundaries. The early milestones deliberately exercise these constraints before a large content library depends on the engine. Arbitrarily rotated gravity, four-player split-screen, elaborate character models, and a broad puzzle system can follow the first playable release.

Validation must distinguish compilation, automated execution, and hands-on play. Linux cloud checks can validate compilation, procedural generation, persistence logic, and available headless rendering paths. A Windows build is not proof of Windows runtime behavior, and browser automation is not a replacement for testing touch and gamepad input on actual devices. Keep unrun platform checks explicit.

SDL's official [Emscripten documentation](https://github.com/libsdl-org/SDL/blob/main/docs/README-emscripten.md) documents its browser main loop, OpenGL ES/WebGL integration, gamepad support, audio activation, and persistent storage. Verify the selected stable SDL3 and Emscripten versions against those requirements when implementing the foundation; documentation on the main branch can describe features newer than a pinned release.
