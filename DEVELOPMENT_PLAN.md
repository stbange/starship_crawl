Starship Crawl is a first-person, procedurally generated space dungeon crawler written in C. The player begins floating in space before the open loading bay of a gothic ship so enormous that its hull resembles an endless wall. Exploration leads through dramatically different spaces, from cramped maintenance tunnels to immense halls. Combat emphasizes slaying hordes, gaining power, and choosing how far into danger to travel.

This document records the agreed direction and the longer-term implementation plan. The README describes the implemented prototype and its current validation status.

The latest [long-expedition design review](docs/reviews/2026-11-07-long-expeditions/README.md)
contains thirteen hour-long automated simulations, earned-checkpoint screenshots,
and a controlled navigation comparison. Its proposed next priorities are growth
and recovery tuning, stronger finales, clearer patrol consequences and rewards
that remain useful. These recommendations are not yet implemented or an approved
new milestone.

## Alien courts and director — 2026-11-07

The four-milestone proposal is implemented in generator 13 / save 14. See
[Courts and director](POLITICS.md) for controls, rules and limits.

1. **Political geography:** three seeded government templates, multi-sector
   catchments and wilderness, rulers, bilateral relationships, banners, court
   projections, marked patrols and political map guidance.
2. **Playable politics:** royal audiences, three competing commissions, conduit
   challenges, per-player offenses, shared co-op decisions and durable rewards.
3. **Bounded encounter director:** physical supply/courier/survey/conflict
   dispatches, both players' pressure, budgets/cooldowns/repetition limits, saved
   clocks and diagnostic reason history. It preserves world geometry and enemy stats.
4. **Succession and escalation:** royal-guard finales, treaties and powered wards,
   successor workshops, local annexations and hostile rival patrols.

This first library has three powers and one dispute per power per ship. The Hungry
Commonwealth, Vacant Throne, broader population movement, additional events and
larger territorial wars remain extensions. Political finales supplement the
existing phase guardians. Human playtesting should compare the three outcomes,
neutral-patrol readability, fight difficulty and whether dispatches improve pacing.

## Gameplay-review follow-up — 2026-11-07

Review items **2, 4, 5 and 6** are implemented for generator 12: five additional
tool-oriented encounter patterns, nine architectural focal structures and scale
cues, directional contracts with five three-rank relic evolutions, eight alien
concepts plus cold/hot specialists, real ceiling traversal and audible warnings.
See [Deep frontiers](DEEP_FRONTIERS.md) for mechanics and validation limits.
The previously completed P1 scope remains documented in
[Regional encounters](REGIONAL_ENCOUNTERS.md).

Overall combat/resource balance and the remaining review items are separate work.
Human playtesting, physical device checks and optional express lift travel remain
open; these automated encounter comparisons do not establish human difficulty.

## P0 exploration improvements — 2026-11-07

Implemented next-connection guidance, readable service landings, deliberate lift
departures and destination previews, contextual room focal accents and a quieter
HUD. The retained seed-3 roof stall now has an ordinary-movement recovery
regression. Two imperfect-input/build profiles run both seeds through extraction,
with per-update resource and navigation observations. See [P0 details](NAVIGATION.md).

The [human/physical checklist](docs/playtests/P0_CHECKLIST.md) supplies repeatable
checkpoints and a results sheet; actual human, Windows GPU/controller and mobile
device sessions remain pending. P1 and the selected follow-up content above are
implemented. Multi-stop express lift routing is a later extension; this milestone makes
adjacent-deck selection deliberate and readable.

## Remaining work reviewed on 2026-11-06

The original six milestones now have a playable foundation. The remaining work
below is distinct from the four completed narrative milestones, four completed
hacking milestones, implemented phase/reward loop, the arrival, and the controls/traversal completion described in [FOUNDATIONS.md](FOUNDATIONS.md).

| Original milestone | Implemented foundation | Still outstanding |
| --- | --- | --- |
| 1. Portable engine, input and views | C17/SDL3/OpenGL, native/web builds, solo/split-screen, touch, persistent per-player controls/quality, stable controller ownership, lifecycle resets and cached per-view targets | Physical Windows graphics, gamepad, Safari and mobile-device validation; tuning from device measurements |
| 2. Arrival and architectural scale | Space approach, lit immense hull, stars, returnable skiff, huge halls/narrow tunnels, gravity, connected stairs/ramps, step/slope movement and animated per-player lifts | Human review of traversal feel and readability on physical devices |
| 3. Deterministic generation and streaming | Connected districts; eight macro footprint forms; paired-deck vaults and supported galleries; seven environmental conditions; eight local tales with physical evidence; six matching enemy ecologies; bounded sprites/streaming and stable revisits | Human review of atmosphere, encounter density and route readability; additional content can build on these systems |
| 4. Combat and progression | Hordes, ranged/heavy/robot enemies, weapons, defenses, pickups, powers, rigs, guardian phases, earned loadouts, checkpoints and forgiving death | Navigation through more complex room routes; human tuning of combat, guardian pacing, rewards, build variety and co-op balance |
| 5. Character profiles and persistence | Both current-ship profiles, three color choices, loadouts, native/browser saves, import/export and migration | Profile/guest selection, richer appearance and visible character customization, equipment/armor item inventories and broader skill configuration |
| 6. World variety and sustained play | Nine sectors, 28 themes, varied footprints and multi-deck landmarks, connected incidents, local tales/ecologies, directional traits, weather/surface effects, physical system effects, per-view quality and generator diagnostics | Additional gravity anomalies, environmental trap/puzzle routes, more contextual systems, long-session profiling and visibility/rendering optimizations |

The expanded milestone 3 is described in [LIVING_SHIP.md](LIVING_SHIP.md). Start a new generator-10 ship to see its architecture and ecology; generator 3–9 saves remain compatible.

The immediate next step is playtesting the current four-phase expedition: time to
the first useful reward, travel versus fighting, guardian retries, route clarity,
and whether different ships produce different builds. More vehicle roles,
artifact variants, optional challenge tiers and deeper post-finale progression
remain later extensions in the progression design. Broad puzzles, elaborate
models and arbitrarily rotated gravity are also later scope.

Every new ship starts a fresh character. Save/resume, death and further
commissions on the same ship preserve that character's build. This supersedes the
original proposal to carry combat progression between ships.

**Expedition progression (implemented on `design/expedition-progression`).**
Every new ship starts fresh characters, including the inactive co-op partner.
Current-ship saves, deaths and further commissions preserve the build. Four spatial
phases, two guardian approaches per phase, an early optional elite, earned equipment,
six loadout slots, four specialization modules and seeded thermal/order directions
now form the progression loop. Generator 7 / narrative 3 / matrix 1 / save 11 retain
compatible older expeditions. See [the playable rules](PHASED_EXPEDITIONS.md) and
[the broader design](PROGRESSION_DESIGN.md). Next: human playtesting of encounter
pacing, route readability, rewards and build balance; then additional vehicles and
artifact variants if they improve those choices. Earlier roadmap entries below
record the sequence that led here.

## Procedural narrative roadmap

The next implementation sequence turns the existing exploration sandbox into a game with a reason to enter each ship and a satisfying, achievable conclusion. Generate causes and consequences first, then derive objectives, environmental evidence and transmissions. Use authored fragments, conditions and effects in C; no runtime language model or external service is needed.

**Narrative milestone 1 — generated expeditions (implemented).** Start each new ship with rescue, knowledge recovery or alien technology salvage. Its seeded identity, original purpose, catastrophe and surviving group provide one consistent hidden history. Ship profiles shape the context: a pilgrim ark's sleepers, a liner's passengers, a carrier's prisoners or a seed vault's custodians. The briefing names the commission, reward and initial lead. Provide sixteen reusable scenes: an arrival briefing, five environmental clues, three investigation sources, three optional operations, three motivation-specific targets and extraction. Actual exploration reveals alternate leads; investigating evidence reveals the target and a related optional mission. Each motivation has three conclusions, one requiring optional preparation. A journal separates goals, confirmed facts, discovered evidence and unresolved leads; the player can select a lead for map guidance. Required information uses short observations; the journal remains available in the shelter. Secure sites and evacuation systems instead of fragile escort NPCs. Keep the original HARROW/ORISON expedition for existing saves.

Acceptance requires complete playable arcs for all three motivations and all conclusions, compatible stable locations, no progression from cache loading or map panning, exactly-once shared rewards, persistent discoveries and decisions, accessible keyboard/controller/touch choices, unchanged old expeditions, and >=95% unit line coverage overall and per core file. Native gameplay simulations and packaged browser tests verify the actual controls, journals and persistence.

**Narrative milestone 2 — history shapes the physical ship (implemented).** Expand history into original purpose, collapse cause, current controlling entities, survivor-group goals, and the true nature of the target and systems depending on it. Bias room composition and connected incident trails from that history: frost leading to ruptured cryonics, organism growth following its power conduits. Generate notices, damage and accounts from the same facts. Add containment motivations (stop a threat reaching inhabited space), quarantine sources, competing shutdown methods and varied arc shapes, including straightforward rescues without a compulsory betrayal. Version architecture changes explicitly; never rewrite observed rooms.

Implemented with four causes and matching controllers, profile-specific agendas, physical origin apparatus, survivor waystations, control notices and an F6 dossier. Rescue/knowledge/salvage have direct and dependent variants; containment has purge, seal and prepared isolation outcomes. Milestone 2 introduced generator 5 / narrative 2 / save 8; original generators and format-7 stories remain supported. Purges persist defeated creatures; milestone 3 now adds physical ship-system overlays and survivor-group decisions. Automated gameplay covers all 21 new variants and nine original ones.

**Narrative milestone 3 — reactive missions and factions (implemented).** Give scene templates prerequisites, room/sector tags, gameplay, revelations and persistent effects. Respond to meaningful discoveries and choices, not background chunk preloading. Reserve compatible destinations ahead of exploration and keep them fixed once revealed. Allow different routes to uncover different evidence about the same cause. Optional missions should restore safe routes, secure beacons, recover evidence, disable controllers or obtain equipment that changes the available endings. Add survivor groups with conflicting but understandable goals and short contextual transmissions; longer records remain optional shelter reading. Persist event history and share one expedition correctly in split-screen. Seed plus actual player events, never cache scheduling, determine the result.

**Narrative milestone 4 — consequences beyond extraction (implemented).** Keep the ship endless while each story has a finite victory/extraction. Reflect rescued people, recovered knowledge or technology, restored systems and contained/released threats in rewards and a debrief. Let players remain aboard, pursue follow-on stories or start a new ship with a fresh character. Reintroduce HARROW/ORISON as one possible newly generated arc as well as a legacy campaign. Expand the template library and combinations only when causal consistency, branch reachability and replay variety are tested.

Milestones 3 and 4 add a prerequisite-driven operation library, persistent room power/beacons/controller shutdowns, two survivor groups with support and assistance, council decisions and contextual transmissions. All actual observations and completed interactions enter a bounded persistent event history. Debriefs record rescued people, knowledge, technology, contained threats and character careers. Players can accept repeatable follow-on rescue, catalogue, containment, fabrication and HARROW/ORISON commissions aboard the same ship, with fixed revealed destinations, their own optional preparation and extraction. HARROW retains its original room-owned Warden gate. Those narrative milestones introduced generator 6 / narrative 3 / save 9; earlier stories and layouts remain intact. Current new ships use generator 8 / narrative 3 / matrix 1 / save 11. See NARRATIVE.md for player behavior and TESTING.md for validation.

An example target experience is a salvage commission aboard *Saint Vesper*. Recent maintenance leads to a refuge under a ruined banquet hall. Evidence reveals that the requested alien drive keeps its life support running; an archive identifies an isolated backup reactor. The player can take the drive immediately, leave it and secure the refuge, or restore the reactor and save the residents while completing the salvage. The environment, optional work, final choice and debrief all refer to the same underlying situation. The scene library generalizes this dependency pattern into other arc shapes so each run can reveal a different situation.

## Original engine and gameplay sequence

The agreed design is:

- First-person movement, with normal gravity through most of the ship and different gravity conditions in selected areas.
- The loading bay is level 0 and offers routes left, right, upward, downward, and farther into the ship. Players can backtrack after entering.
- Every newly generated ship starts fresh characters. Death returns the player to a checkpoint while retaining that ship's character progression.
- Character builds start with a modest kit and earn stronger equipment and abilities through challenges. Appearance customization is currently limited to color choices.
- Linux, Windows, and browsers through Emscripten are target platforms. Mobile browsers should have on-screen controls.
- Multiplayer is local split-screen. Players share one machine and one simulated world.
- Graphics are dark, gloomy, retro, and pixelated. The ship feels infested, with detailed rooms, broken colored lighting, and a flashlight. Enemies have dark, strange silhouettes, tentacles, fast movement, wall climbing, and avoidable glowing shots.
- World generation and architectural variety are central features, with strong differences in room scale.

The initial defaults below resolve details that were not specified. They can be adjusted without changing the overall direction:

- Single-player is the default and the initial gameplay-testing priority. Split-screen is optional and activates only through an explicit join action. Desktop and browser co-op support keyboard and mouse for one player plus a gamepad for the other, or two gamepads. A resumed session opens in solo mode while retaining the second character profile.
- Mobile touch controls initially support one player. Two-player browser support remains part of the engine; two simultaneous touch layouts are a later extension rather than the mobile baseline.
- A new expedition creates a new ship seed. Saving and resuming, character death, or a second player joining do not regenerate the active ship. Starting a new expedition is an explicit action that resets both players' characters.
- The first death policy retains experience, skills, and equipped items, and respawns at the last activated checkpoint. A surviving partner continues playing. Exact respawn timing and temporary penalties are tuning decisions.
- Profile selection and guest slots remain planned. The expedition save currently retains two ship-local character profiles, including the inactive co-op partner.
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

   The Reclaimer expansion adds nine sectors, 28 room themes, 27 enemy species, 16 weapons, ten attributes, armor and shields, persistent caches and lore, three contracts, and a six-stage story with a Warden encounter and beacon extraction. Version-3 saves retain those systems; previous characters migrate to a fresh ship. Broader navigation and equipment item inventories remain future work.

   The earlier atmosphere/combat pass implements procedural infestation and room dressing, broken colored lamps, a per-player flashlight, animated tentacled enemy silhouettes, fast surface-climbing crawlers, and charged, dodgeable projectile attacks. That earlier release preserved generator-2 layouts and saves. Existing weapon feedback, experience, and upgrades remain available. Richer navigation and additional progression depth below remain planned.

   Add a responsive weapon, hit feedback, pickups, experience, and several upgrades with visible effects on combat. Use swarming melee enemies, ranged enemies, and heavy enemies with distinct silhouettes and readable attack cues. Build enemy movement on room connections and local navigation data; use shared navigation work for hordes where practical.

   Difficulty grows with distance from the entrance. Define understandable distance bands and tune enemy health, damage, density, compositions, and rewards together. Keep early regions useful as places where stronger characters can feel powerful. New behaviors and hazards provide additional challenge alongside statistical growth. Maintain encounter and simulation budgets even at large distances.

   Both players participate in one simulation, with per-player damage, inventory, experience, and HUD state. Award participation rewards without allowing duplicate loot claims or duplicate spawns in overlapping active regions. Add checkpoint activation and forgiving respawns. Define a single mixed audio output that remains intelligible when the two camera positions differ.

   Completion means a short solo or cooperative session includes enjoyable battles, meaningful upgrades, a deeper and more dangerous region, and a successful checkpoint respawn that retains progression.

5. **Complete character profiles and expedition persistence.**

   Add character selection, simple appearance choices, equipment, and flexible skill selection. Appearance changes should be visible through first-person equipment or hands and through the other player's character model. Use complete default character profiles so customization never blocks starting a game.

   Keep character records separate from expedition records. Save the seed, generator version, checkpoint state, world changes, player positions, and other state needed to resume an expedition consistently. Regenerate unchanged geometry from the seed. Persist opened doors, collected unique rewards, defeated persistent encounters, and other changes according to explicit gameplay rules; unloading a region must not become a reward duplication mechanism.

   On desktop, use an application save directory and atomic replacement where supported. In browsers, use IndexedDB-backed storage with initialization before loading saves and asynchronous persistence confirmation after writing them. Show save completion only after persistence succeeds, save during play, and do not rely on tab-close callbacks. Include save import/export for backup and moving profiles between devices. A shared file format supports transfer without adding online services.

   Version both saves and generation rules. Do not silently resume an old expedition using incompatible generation rules. Test the joint save of shared world changes and individual progression so a partial save cannot duplicate rewards or erase a character's gains.

   Completion means closing and reopening the game restores the same active ship and both character profiles. Starting a new expedition creates different architecture and fresh characters, as required by the revised progression model. Browser saves survive a page reload, and export/import restores a profile and expedition as documented.

6. **Expand the world and validate sustained play.**

   Luxury, command, research, logistics, cryonics, and botanical sectors with themed furnishings are implemented. Generator 4 adds four ship identities, suites/work bays/arcades, connected environmental incidents, side-space caches and four rare hall interiors. Continue with larger multi-level structures, additional footprint shapes, interactive ship systems and gravity anomalies. Introduce traps and short riddles first in optional reward routes. Main exploration routes remain traversable and the combat pace remains energetic.

   Give each district a recognizable architectural vocabulary and progression of spaces. Use authored landmarks within procedural layouts, deliberate transitions between district types, and room arrangements that create memorable views. Generation quality is measured by interesting routes, coherent spaces, surprise, and scale, as well as validity.

   Add generator inspection tools for seeds, region boundaries, connectors, occupied footprints, navigation, difficulty, and active simulation areas. Batch-test many seeds for matching seams, valid collision clearances, reachable exits, stable generation across platforms, and repeatability regardless of load order. Compare semantic generation records across native and web builds rather than expecting identical rendered pixels.

   Profile long sessions with dense combat, giant halls, two separated players, and mobile browsers. Use per-camera visibility checks, batching, instancing, and room connectivity to control rendering work. Allow separate internal render resolution and view distance settings for mobile and split-screen. Establish frame-time and memory targets against named reference devices once representative scenes exist.

   Completion means the chosen test hardware sustains a documented quality setting, active memory remains bounded during extended travel, save/resume works after long exploration, and both solo and split-screen preserve the intended combat and atmosphere.

The first playable release combines milestones 1–4 with basic character and expedition saves from milestone 5. It includes the space approach, level-0 loading bay, five initial route directions, nine sector families and 28 room themes, giant halls and crouching passages, 27 enemy species, 16 weapons, ten attributes, defenses, collectible caches, and the Reclaimer story, checkpoints, and one- or two-player play. Linux, Windows, desktop web, and mobile touch are exercised throughout development rather than deferred to a final port.

The practical constraints are rendering each split-screen view, bounded browser memory, asynchronous browser saves, and producing coherent structures that span chunk boundaries. The early milestones deliberately exercise these constraints before a large content library depends on the engine. Arbitrarily rotated gravity, four-player split-screen, elaborate character models, and a broad puzzle system can follow the first playable release.

Validation must distinguish compilation, automated execution, and hands-on play. Linux cloud checks can validate compilation, procedural generation, persistence logic, and available headless rendering paths. A Windows build is not proof of Windows runtime behavior, and browser automation is not a replacement for testing touch and gamepad input on actual devices. Keep unrun platform checks explicit.

SDL's official [Emscripten documentation](https://github.com/libsdl-org/SDL/blob/main/docs/README-emscripten.md) documents its browser main loop, OpenGL ES/WebGL integration, gamepad support, audio activation, and persistent storage. Verify the selected stable SDL3 and Emscripten versions against those requirements when implementing the foundation; documentation on the main branch can describe features newer than a pinned release.

**Enemy deaths and machines (implemented).** All species have bounded death presentation and debris. Selected species leave acid, a delayed blast or a proximity mine. Five machines add a sixth body family and an expanded generator-6 roster; generator 3–5 encounters remain stable. Unit, real campaign, native render and browser tests cover warning periods, cover, safe-space isolation, exactly-once defeats, pool budgets and compatibility.

**Ship Matrix (implemented).** All four [hacking milestones](HACKING.md) now provide body/virtual-space separation, safe return and shelter login, independent optional co-op links, seeded regional networks, security combat, six upgradeable programs, credentials and routing, persistent ship systems and robot controls, shared story preparation, rare vaults and generated controller conclusions. Format 10 retains active and reserved links without changing ship generator 6 or narrative 3. Unit and allocation/corruption tests, three normal-control journeys, native rendering and packaged browser input/save/touch tests cover the layer.
