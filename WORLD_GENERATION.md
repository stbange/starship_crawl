New ships use **generator version 11** with seeded thermal/order directions and four guardian-gated spatial bands; see [Phased expeditions](PHASED_EXPEDITIONS.md). Saved generator-3 through generator-10 expeditions keep their original plans, geometry, enemy identities and collection records. **N**, then **Enter**, starts a new ship with the new generator with fresh character profiles. A seed plus region coordinates determines all static architecture, regardless of load order or platform. Changing the layout, geometry, or population rules incompatibly requires a new generator version and an explicit save migration.

The shared pipeline is implemented in `generation.c`, `habitat.c`, `encounters.c`, `interior.c`, `traversal.c` and `world.c`. Generator 11 adds [regional encounters and consequences](REGIONAL_ENCOUNTERS.md). The original rectangular-room pipeline below remains the compatibility path; generator 10 extends it as described in [LIVING_SHIP.md](LIVING_SHIP.md):

1. Select one of nine sectors for a 4 × 4 chunk region on one deck. A seed selects Pilgrim Ark, Crown Liner, Siege Carrier or Seed Vault, each with a different deck order across habitation, machinery, sanctum, luxury, command, research, logistics, cryonics and botany; one quarter of regions vary from their deck sector. Room themes choose from that sector's catalog. Introductory examples retain their special identities.
2. Reserve one 2 × 2 hall footprint before assigning the remaining cells. Hall geometry is 180 × 180 × 58 metres. A chunk is 96 × 96 metres on a 64-metre deck. Ordinary interiors compose four functional side spaces around protected cross passages. Suites have a front and side entrance, work bays use staggered partial walls, and arcades use columns and overhead frames. Furniture scales to preserve a perimeter route. Single-chunk conduits have 1.55 metres of clearance; large undercrofts retain the full hall envelope. The entrance and its immediate route examples have stable identities for onboarding.
3. Contract occupied footprints into room nodes. Stable Kruskal selection creates a connected regional backbone; additional deterministic edges produce loops. Adjacent regions agree on two face portals by hashing the same face owner and axis. The loading bay also guarantees left, right, and inward routes. Every cell has a grav lift that connects to the next deck, so planar regional connectivity extends vertically.
4. Use the shared seeded history to favor cryonics, research, command or machinery rooms. Reserve a physical catastrophe origin, furnish its damaged apparatus, and add survivor waystations and controller notices. Spread a regional blackout, frost leak, quarantine, fire or bloom from one logical room through open connections. Intensity falls with graph distance; the safe loading bay is excluded. Most regions also contain quiet rooms. Incidents change fixture color and power, cables, scorch marks, frost, quarantine markings, roots and nests. This is environmental history. Narrative-3 missions can restore room power, activate safe beacons and silence controllers through persistent overlays; pressure remains unsimulated.
5. Give roughly one quarter of large halls a rare banquet, survivor conservatory, suspended engine or reliquary interior. Sector identity biases the choice. Chandeliers, a giant tree canopy, a hanging reactor and stacked reliquary arms create large silhouettes within the existing deck envelope.
6. Build only the requested chunks from these plans. Geometry is clipped to chunk bounds, with no internal walls through a landmark. Connector contracts specify a shared location, width, clearance, and crouching requirement. All families accept the same passage standard; the outer hull is closed except at the loading bay.
7. Populate a room only after all chunks in its footprint are complete. Enemy identities, attributes, and candidate positions use independent salts; occupied candidates are retried against completed collision geometry. Defeated enemies use one record per logical room, including halls shared by several chunks.

Random choices use the fixed `uint32_t` hash in `generation.c`, with explicit constants and unsigned wraparound. Inputs include the ship seed, generator version, region or room coordinates, and content category. No platform RNG, pointer identity, generation order, or mutable random stream affects the plan. The native and browser tests compare semantic fingerprints across seeds, negative regions, multiple decks, and distant coordinates.

`world_refresh` requests the union of a one-chunk neighborhood around each active player, expanded to include complete room footprints. It does not generate geometry. The headless session calls `world_stream` once per fixed 60 Hz simulation tick with a maximum of two chunk jobs. Paused previews may also populate geometry without advancing gameplay. GPU uploads have their own two-job display-frame budget. Requests alternate between players and prioritize their immediate rooms. Overlapping requests share the same chunk and encounter population.

Chunks within a two-chunk retention neighborhood remain cached for backtracking. When the cache fills, only rooms outside the required set can be evicted. Eviction removes complete footprints, including their collision and GPU data. The default two-character core retains 192 resident chunks (96 per configured party slot; the four-player headless test retains at most 384), 512 boxes and 16 compact flights per generator-9 chunk, and 20 enemy slots per room. Generators 3–8 retain their 256-box limit. Dormant rooms leave the simulation. A new room remains collision-blocked until its complete footprint is available; visible bulkheads close unfinished passages. Lifts wait for a ready destination. These checks also apply during save loading and checkpoint travel.

The interior builder combines sector furnishings with a secondary service/storage function: bunks and lockers, reactor machinery, chapel pews, banquet seating, command consoles, surgical stations, cargo containers, cryopods and planters, plus theme-specific baths, gaming tables, workshops, turbines, archives and fungi. Solid partitions and furniture affect movement; flat surface details and organic dressing stay cosmetic. The central cross, lifts, mission consoles, six-metre side-space doorways and a perimeter route remain clear. Weapon and record caches move just inside side-room entrances; their existing IDs and consumed bits stay stable. Bay and crawl-space caches retain their familiar locations. Crawl-space lighting and surface marks carry incident trails through narrow routes too.

Each chunk holds at most 32 organic/clutter groups and six light fittings; at least half the fixtures are dead. Both fixtures and the light they cast share a deterministic flicker phase. Base-room cosmetics stop at the historical 256-box budget; generator 9 reserves additional capacity for shaft carving and landings. The current 24-seed interior traversal sweep peaks at 219 boxes per chunk. Generator 3 retains its historical furniture and dressing code path, guarded by original geometry fixtures.

The renderer maintains one reusable GPU buffer per resident chunk and uploads at most two changed chunks per frame. Unchanged meshes are retained. All generation and GL uploads run on the main thread, without workers or cross-origin-isolation requirements. These are work budgets, not a guaranteed number of milliseconds on every device.

Each viewport selects up to 12 nearby point lights, including moving projectiles and muzzle flashes, plus its own directional flashlight. Lighting uses distance attenuation and surface normals, without shadow maps. Static dressing is included in cached chunk meshes. Generators 3–9 limit static drawing to the viewer's deck; generator 10 also draws adjacent decks so vault openings reveal real architecture, inhabitants and effects; animated creatures use fewer limb segments at distance and a 128-creature draw limit per view. A shared pool caps enemy projectiles at 96, with finite lifetimes and collision against walls and players.

Player and enemy positions store integer sectors and local double coordinates. Movement normalizes the local coordinates at sector boundaries. Collision boxes and static meshes are chunk-local; each viewport independently translates nearby geometry relative to its own camera before converting to GPU floats. Split-screen players can therefore travel far apart without sharing a distant floating origin. Objects outside a viewport's draw range are skipped before that conversion.

Only changed encounter and collectible records grow with exploration. A hash index provides bounded average lookup work as the save grows. Unchanged regions are reconstructed from the seed. Enemy injuries and wandering positions reset after their owning room unloads; defeated enemy identities remain defeated. Character progression and checkpoints remain persistent. Save format 6 records the generator version, sector-local positions, defenses, weapons, attributes, mission/contract state, and per-room consumed-cache bits alongside kill bits. Its sanctuary appendix stores the separate interior and return anchors without generating sanctuary cells. Compatible format-3/4/5/6 generator-3 expeditions retain their original ship and records. Format 6 also preserves deployed rigs, plating, flare upgrades and attached lights. Current format-12 saves preserve their generator through version 11, local-action completion and per-player optional-challenge claims, phase/build state, matrix generator 1 and narrative 3, including room power, beacon and controller overlays, factions, chapter events and character careers. Format-8/narrative-2 saves retain their original history. Format-7/narrative-1 saves keep generator 4. Architecture and history metadata reconstruct from the seed. Version-1 and version-2 files retain both character profiles and announce a fresh expedition. Map schematics query bounded region plans without streaming chunks. Mission terminals occupy the protected center cross. The heart room reserves one encounter identity for the Warden; it must be defeated before that terminal can advance the story.

For inspection:

- Press **F3** for the current district map, room dimensions, resident/queued chunks, generation work, and upload counts. Gold marks the current cell; passage marks show connected neighbors.
- Run `./build/starship_crawl --seed 713 --scene hall --no-save` to inspect the four-chunk cathedral, or use `crew`, `engine`, `tunnel`, and `atrium`.
- Run `./build/starship_crawl --seed 713 --scene distant --split --no-save` to put one camera in a distant district and one at the entrance.
- Run `./build/starship_crawl --seed 713 --scene encounter` for a reproducible three-role encounter in the reactor room. This scene disables saving. Press **F** to inspect the room with and without the flashlight.
- Browser equivalents use `?seed=713&scene=hall` or `?seed=713&scene=distant&split`. Scene overrides start a fresh expedition; use a separate browser profile when keeping your normal save matters.
- Use `--scene layout0`, `layout1`, or `layout2` for the three floor plans; `history0` through `history5` for quiet/incident rooms; and `landmark1` through `landmark4` for the four rare halls. Use `origin`, `waystation` or `controller` to inspect historical sites. These inspect real plans found for the selected seed, without forcing metadata.
- The HUD names the ship profile and local incident. **M** opens the map; pink brackets mark landmark halls, separate from the gold mission goal.
- Run `./build/sc_interior_tests` for incident connectivity, deterministic furniture, two-entrance traversal, mech clearance, cache persistence and legacy compatibility.
- Run `./build/sc_tests --fingerprints` for JSON semantic probes, or the regular core suite for topology, physical connector clearance, streaming, persistence, and coordinate checks.

Generator 10 adds masked horizontal footprints and paired-deck architectural volumes. Generator 9 introduced continuous stairs/ramps and animated grav lifts. Richer horde navigation, additional gravity anomalies, traps and puzzles remain later content/gameplay work. Sustained performance on physical mobile hardware and Windows runtime behavior require device testing.

Reactive system flags belong to changed room records and never alter static plans, collision, footprints, portals or original encounter identities. Power overlays affect real point lights and recovery. Beacon wards protect within 18 metres of a powered station. Controller overlays disable local turrets. Records persist after a chapter finishes. Pure map queries expose existing network state without running mission logic.

Generator 6 reuses generator 5 room plans, geometry and history sites. It expands ordinary encounter selection with five robots, preserving Warden ID 26 as a reserved encounter. Generator 3–5 selection and positions remain unchanged. Corpse snapshots and finite death hazards use bounded pools separate from room storage and do not persist in saves. A captured generator-5 enemy fingerprint fixture checks roster and position compatibility.

Generator 8 adds the [boarding skiff and arrival view](ARRIVAL.md). It reuses
generator-7 interior plans, furnishing geometry and encounter salts exactly for
the same seed. The skiff is a bounded static exterior object, outside the streamed
room grid. Only new expeditions receive its collision and the farther starting
position; older saves retain their coordinates and original exterior. Cosmetic
stars and hull lights apply to all generations.


## Generator 9: service columns

Generator 9 keeps the version-7 room plans and overlays deterministic vertical
architecture. Each horizontal four-by-four region chooses one column independently
of deck height; the bay column is guaranteed. Eight switchback stair flights and
eight ramp flights span the full 64 metres. Their compact definitions drive mesh
construction, body support/overlap and projectile rays. Stairs have 32 treads per
flight; ramps are true sloped slabs. Landings, rails, room exits and a level lift
apron keep both vertical and horizontal routes connected. Non-service pads have
compact shafts. Base geometry is carved only for generator 9; older chunks retain
their exact recorded geometry and old lift behavior.

Walking can climb a 0.30-metre step with sufficient head clearance and snap down
0.32 metres. Phase gates and unfinished rooms still use the normal collision rules.
Grav-lift cradles cross walkable field-grates kinematically over three seconds after
the destination is validated. A rider's save snapshot remains on the departure pad.
See [FOUNDATIONS.md](FOUNDATIONS.md) for routes, controls and physical-device checks.


## Generator 10: inhabited volumes

See [LIVING_SHIP.md](LIVING_SHIP.md) for the implemented shapes, local tales, weather, enemy ecology, controls and inspection scenes. New mask fields distinguish owned tiles from holes. Streaming expands only occupied tiles, completeness waits for every owned chunk, and holes have their own neighboring rooms and reciprocal connectors. The map uses the same owner graph.

Paired even/odd decks reserve matching macro footprints before filling ordinary rooms. Lower vaults omit their ceiling; upper galleries retain a cross, perimeter promenade, lift access and railings around true floor openings. Each deck keeps an independent logical record and population. Tutorial examples, guardian arenas and the catastrophe origin retain their reservations.

Profiles use sector function, connected incidents and thermal/order traits. They select atmospheric sprites, furniture evidence and weighted tier-appropriate enemy rosters, without replacing guardians. Large new rooms add inhabitants, reserving two offspring slots within the existing 20-enemy room bound so brood species can still reproduce. Spawn candidates avoid traversal shafts and neighbor seams and require real floor support; generation is independent of phase locks and loading order.

Each chunk stores at most 32 environment sprite anchors. A procedural 64×64 RGBA atlas supplies eleven pixel sprite types. The renderer expands deterministic steam/rain/mote motion into at most 1,536 depth-tested, alpha-sorted quads per viewport. Water uses shallow non-solid surfaces and animated pixel ripples. No fluid simulation, swimming, new environmental damage or saved particle state is implied. Per-view fog/effects reset in space, shelter and matrix. CPU geometry is limited to two chunks per simulation tick; uploads are limited to two chunks per display frame. The default local game retains at most 192 resident chunks shared by both players.

An immutable generator-9 format-11 save and twenty geometry/population fingerprints protect compatibility. New geometry, environment and ecology fields enter generator-10 semantic fingerprints; earlier hashes remain unchanged.
