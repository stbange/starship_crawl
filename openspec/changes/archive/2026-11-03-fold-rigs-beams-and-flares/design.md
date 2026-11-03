## Decisions

Add three beam weapons and a flare cannon after the stable 16 personal weapon IDs. Beam pulses use instantaneous wall-bounded traces, preserve their status/piercing effects and render continuous or pulsing segments rather than moving bullets. Mech weapons use a separate three-item arsenal.

Each player owns one mech and one wagon. T/Y toggle deployment on clear supported floors. G mounts a nearby owned rig or safely exits the current one. Mech collision and eye height are larger than on foot; wagon motion accelerates with a higher top speed. Normal gravity, walls, ceilings and unready frontiers remain authoritative. Mounted pilots use rig plating after personal shields; plating survives packing. A depleted rig ejects its pilot, and the sanctuary medical station repairs parked owned rigs. Psionics require being on foot.

The flare cannon fires finite utility projectiles that ignore enemy bodies and attach to solid world surfaces. Surface normals come from bounded local probes. Upgrades bought with points or salvage improve future flare lifetime and brightness/radius. Keep at most 32 active flares, replacing the nearest-to-expiry slot at capacity. Flare records do not hold chunks resident; rendering selects bounded nearby point lights. Beams and moving shots are transient.

Format 6 appends validated player equipment, four rigs and active flare records after the format-5 fields. Formats 3/4/5 retain profiles, ship, records, missions, shelter and powers. Grant the four new personal weapons on migration. Keep generator-3 random loot selection on its original 16-item pool so old ships do not reinterpret cache identities.

## Validation

Unit-test all beam modes and wall/energy/piercing behavior; placement, occupancy, entry/exit, motion/collision, damage, repair and weapon switching; flare attachment, normal, brightness/lifetime upgrades, expiry/capacity and format-5/6 persistence. Exercise real keyboard/touch in WASM/WebGL, inspect mech/wagon/beam/flare captures, retain campaign regressions, >=95% unit coverage, sanitizers, Windows/native and strict specs before publication.
