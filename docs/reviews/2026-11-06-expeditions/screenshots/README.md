# Checkpoint gallery

These are actual Chromium/WebGL renders imported from the 25-minute diagnostic expeditions. Each image has a JSON sidecar verifying its seed, character rank, coordinates and renderer state. Enemy positions reconstruct on load; these are checkpoint replays, not continuous run recordings. The images retain the game's original dark exposure and HUD.

A few highlights:

- [Seed 713, 5 minutes — Memory Archive](seed-713-minute-05.png): orange architectural planes and the cyan mission/interaction markers in a dark service space.
- [Seed 713, 25 minutes — Crew Sepulchres](seed-713-minute-25.png): a supported stair/lift structure rising through the room, useful evidence for vertical readability.
- [Seed 42, 10 minutes — Cathedral Nave](seed-42-minute-10.png): a dark large-scale interior with colored structural accents.
- [Seed 42, 25 minutes — Turbine Gallery](seed-42-minute-25.png): blue illuminated geometry against the dark ship interior.
- [Seed 3, 5 minutes — First Class Promenade](seed-3-minute-05.png): a luxury-themed district on an expedition with a very different subsequent route.
- [Seed 3, 25 minutes — navigation stall](seed-3-minute-25.png): the camera is pressed close to a surface, making the lack of route feedback apparent.
- [Seed 3, manual-input recovery probe](seed-3-recovery.png): ordinary strafe, jump and forward input move away from the stationary position. See the recorded before/after state in `../data/recovery.json`.

All twelve original checkpoint views are retained, at 5, 10, 20 and 25 minutes for each of seeds 713, 42 and 3. Additional `-upward` images look around the same replayed locations with normal mouse input; they do not teleport the camera or substitute an inspection scene.
