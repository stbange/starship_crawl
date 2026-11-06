New ships use generator 10. Press **N**, then **Enter**, to begin one with fresh characters. Existing generator 3–9 expeditions keep their layouts, encounters and progress; save format remains 11.

The expanded milestone 3 links architecture, environmental evidence and inhabitants. It adds eight large footprints alongside the ordinary rooms: processional vaults, long galleries, turbine canyons, bent arcades, branching concourses, horseshoe galleries, reservoir halls and hanging gardens. Footprints use three to six occupied tiles, with some spanning 276 metres horizontally. Bent and branching forms contain real neighboring rooms in their missing tiles.

Some macro spaces span two decks: a 64-metre lower vault opens into a 58-metre upper volume. Upper galleries have cross routes, perimeter walks and railings around openings, while the full space reaches 122 metres. Faint structural strips reveal repeated columns and overhead ribs. Banquet tables, conservatory canopies, suspended machinery and reliquaries provide large focal points. Normal service stairs, ramps, lifts and mission terminals remain available.

Seven environmental conditions are derived from room function, connected damage and the ship's directional traits: still air, venting steam, colored runoff, spores, cryogenic mist, ember rain and luminous condensate. You can encounter steam plumes, shallow rippling water, cyan/rust/blue/amber/violet precipitation, floating spores, strange glowing particles and low fog. Banners, fungal silhouettes, papers, glyphs and skeletal remains add pixel sprite detail. Water and weather are atmospheric effects; they do not introduce swimming or invisible damage.

The local population fits the conditions. Six weighted ecologies favor feral remnants, autonomous machines, nest colonies, cold-adapted hunters, phase scavengers or runoff scavengers. Each mixes silhouettes and abilities while respecting difficulty and tunnel clearance. Machines favor industrial/orderly areas, cold hunters gather near cryogenic damage, and nests spread through fungal growth. Large rooms hold more inhabitants, within the existing population cap. Guardian encounters retain their phase rules.

Eight local tale families leave physical evidence: an abandoned evacuation queue, an interrupted feast, an improvised triage ward, failed containment, unfinished repairs, a surviving shrine, a crew barricade and overgrown cultivation. **F6** opens the ship dossier, including the current room's local account. Accounts reconstruct from the seed and describe the evidence and conditions; they do not replace the expedition's main missions or award repeated quest rewards. Narrow conduits and the safe loading bay have appropriate observations instead of claiming furniture that cannot fit.

Both cameras render their own atmosphere in optional split-screen. Sprite work is capped at 1,536 quads per view and 32 anchors per chunk; chunk generation and GPU uploads retain the two-per-frame limit. Fog/effects are kept out of the shelter, matrix and space. These bounds are resource limits, not a promised frame rate on every GPU.

For a quick visual tour, use the normal executable's inspection scenes (or add the equivalent query to the web URL):

| Preview | Native argument | Browser query |
| --- | --- | --- |
| Lower vault | `--scene volume1` | `?seed=713&scene=volume1` |
| Upper gallery | `--scene volume2` | `?seed=713&scene=volume2` |
| Eight footprint forms | `--scene shape1` through `shape8` | `?seed=713&scene=shape4` |
| Seven conditions | `--scene biome0` through `biome6` | `?seed=713&scene=biome1` |

These find real generated spaces for the seed, open progression for inspection, and disable saving. Ordinary expeditions retain guardian gates. Use **F** to compare the flashlight with the room's weak lighting, **M** to inspect its footprint, and **F6** to read its evidence.

Automated coverage includes old saves, deterministic footprints/conditions, paired-deck alignment, ownership holes, supported spawns, portal routes, upper railings, load-order parity, actual movement across both levels, save/resume, and native/WASM parity. Packaged Chromium exercises all seven conditions, both vault levels, bent/branching maps, local accounts, sprite budgets, split-screen and touch-sized views. See [TESTING.md](TESTING.md) and [captures](docs/screenshots/README.md). Physical Windows graphics, Safari and mobile-GPU performance still require device testing.
