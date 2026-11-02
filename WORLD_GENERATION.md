The ship uses **generator version 2**. A seed plus region coordinates determines all static architecture, regardless of load order or platform. Changing the layout, geometry, or population rules incompatibly requires a new generator version and an explicit save migration.

The pipeline is implemented in `generation.c` and `world.c`:

1. Select a district for a 4 × 4 chunk region on one deck. Habitat districts favor cabins and service passages; machinery districts favor reactors and conduits; sanctum districts favor naves and atriums. Each family has its own material/light palette and ambient drone.
2. Reserve one 2 × 2 landmark footprint before assigning the remaining cells. Landmark geometry is 180 × 180 × 58 metres. A chunk is 96 × 96 metres on a 64-metre deck. Cabin chunks contain eight or twelve rooms opening onto a central cross passage. Conduits have 1.55 metres of clearance. The entrance and its immediate route examples have stable identities for onboarding.
3. Contract occupied footprints into room nodes. Stable Kruskal selection creates a connected regional backbone; additional deterministic edges produce loops. Adjacent regions agree on two face portals by hashing the same face owner and axis. The loading bay also guarantees left, right, and inward routes. Every cell has a grav lift that connects to the next deck, so planar regional connectivity extends vertically.
4. Build only the requested chunks from these plans. Geometry is clipped to chunk bounds, with no internal walls through a landmark. Connector contracts specify a shared location, width, clearance, and crouching requirement. All initial families accept the same passage standard; the outer hull is closed except at the loading bay.
5. Populate a room only after all chunks in its footprint are complete. Enemy identities, attributes, and candidate positions use independent salts; occupied candidates are retried against completed collision geometry. Defeated enemies use one record per logical room, including halls shared by several chunks.

Random choices use the fixed `uint32_t` hash in `generation.c`, with explicit constants and unsigned wraparound. Inputs include the ship seed, generator version, region or room coordinates, and content category. No platform RNG, pointer identity, generation order, or mutable random stream affects the plan. The native and browser tests compare semantic fingerprints across seeds, negative regions, multiple decks, and distant coordinates.

`world_refresh` requests the union of a one-chunk neighborhood around each active player, expanded to include complete room footprints. It does not generate geometry. The application calls `world_stream` once per displayed frame, including while paused, with a maximum of two chunk jobs. Requests alternate between players and prioritize their immediate rooms. Overlapping requests share the same chunk and encounter population.

Chunks within a two-chunk retention neighborhood remain cached for backtracking. When the cache fills, only rooms outside the required set can be evicted. Eviction removes complete footprints, including their collision and GPU data. The limits are 192 resident chunks, 256 boxes per chunk, and 20 enemy slots per room. Dormant rooms leave the simulation. A new room remains collision-blocked until its complete footprint is available; visible bulkheads close unfinished passages. Lifts wait for a ready destination. These checks also apply during save loading and checkpoint travel.

The renderer maintains one reusable GPU buffer per resident chunk and uploads at most two changed chunks per frame. Unchanged meshes are retained. All generation and GL uploads run on the main thread, without workers or cross-origin-isolation requirements. These are work budgets, not a guaranteed number of milliseconds on every device.

Player and enemy positions store integer sectors and local double coordinates. Movement normalizes the local coordinates at sector boundaries. Collision boxes and static meshes are chunk-local; each viewport independently translates nearby geometry relative to its own camera before converting to GPU floats. Split-screen players can therefore travel far apart without sharing a distant floating origin. Objects outside a viewport's draw range are skipped before that conversion.

Only changed encounter records grow with exploration. A hash index provides bounded average lookup work as the save grows. Unchanged regions are reconstructed from the seed. Enemy injuries and wandering positions reset after their owning room unloads; defeated enemy identities remain defeated. Character progression and checkpoints remain persistent. Save format 2 explicitly records the generator version and sector-local positions. Loading a version-1 save preserves both character profiles and announces a fresh expedition; it does not silently reinterpret the previous ship.

For inspection:

- Press **F3** for the current district map, room dimensions, resident/queued chunks, generation work, and upload counts. Gold marks the current cell; passage marks show connected neighbors.
- Run `./build/starship_crawl --seed 713 --scene hall --no-save` to inspect the four-chunk cathedral, or use `crew`, `engine`, `tunnel`, and `atrium`.
- Run `./build/starship_crawl --seed 713 --scene distant --split --no-save` to put one camera in a distant district and one at the entrance.
- Browser equivalents use `?seed=713&scene=hall` or `?seed=713&scene=distant&split`. Scene overrides start a fresh expedition; use a separate browser profile when keeping your normal save matters.
- Run `./build/sc_tests --fingerprints` for JSON semantic probes, or the regular core suite for topology, physical connector clearance, streaming, persistence, and coordinate checks.

The current library uses rectangular horizontal footprints and instant grav lifts. Multi-deck architectural landmarks, additional footprint shapes, new district families, animated lifts, richer horde navigation, traps, and puzzles remain later content/gameplay work. Sustained performance on physical mobile hardware and Windows runtime behavior require device testing.
