## Decisions
- Defeat records and rewards commit once immediately; corpses cannot attack, block objectives or receive additional rewards. Visual snapshots own their pose and position rather than retaining streamed-room pointers.
- Separate bounded visual and hazard pools so a burst of ordinary deaths cannot cancel existing dangerous effects. Pool exhaustion suppresses additional hazards instead of evicting armed hazards without warning.
- Give explosions a visible countdown, mines an arming delay and triggered countdown, and acid a visible finite lifetime with rate-limited damage. Walls, decks, shelter isolation and powered wards remain authoritative. Explosions can hurt nearby enemies and chain through delayed hazards with normal player attribution.
- Retain all existing species IDs, including Warden 26. Append five machines. Generator 6 selects from the expanded ordinary roster while excluding the Warden; older versions retain exactly their original selection and spawn calculation.
- Keep portable format 9: corpses and hazards are transient combat state, and successfully loading a save or starting a new ship clears them. Failed imports preserve active effects.
- Hacking remains a written design: a seeded network derived from physical ship systems, explored through terminals, with finite sessions and persistent physical consequences.

## Validation
Capture pre-change generator-5 encounter fingerprints. Exercise all body families, every death effect, warning periods, wall/deck and shelter/ward isolation, pool saturation, chain attribution, duplicate-reward refusal, streaming independence, expiry, resets and save compatibility. Verify representative real movement/fire/dodge journeys and desktop/touch browser rendering without adding runtime test controls.
