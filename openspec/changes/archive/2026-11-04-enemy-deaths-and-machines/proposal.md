## Why
Enemies currently disappear on defeat. Deaths should communicate impact, distinguish living creatures from machinery, and occasionally create an avoidable tactical hazard. The ship also needs recognisable robots.

## What Changes
- Animate all enemy deaths with bounded, detached visual snapshots and debris.
- Add clearly signalled acid residue, delayed explosions and dropped proximity mines to selected species.
- Add five robotic species and a sixth body family with five mechanical silhouettes.
- Use generator 6 for the expanded encounter roster; preserve generator 3–5 encounters, architecture and saves. Keep format 9 and narrative 3. Transient death visuals and hazards clear on load/new ship like projectiles.
- Document a proposed terminal-accessible ship matrix with physical-world consequences; do not implement hacking in this change.

## Capabilities
### Modified Capabilities
- `combat`: death presentation, hazards and robotic encounters.
- `persistence`: expanded generator compatibility and transient-state reset.

## Impact
Core combat, generation, rendering, inspection scenes, documentation and automated tests. Require strict specs, >=95% unit line coverage overall/per core file, actual combat journeys, native rendering, sanitizers and packaged WebGL tests before publishing.
