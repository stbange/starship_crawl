## Why
The approved hacking design needs a playable second layer connected to actual ship systems, with persistent consequences and safe transitions between player spaces.

## What Changes
- Implement all four HACKING.md milestones: terminal entry/return, virtual encounters and programs, durable physical consequences, and sector-specific networks with generated controller stories.
- Keep bodies in physical space while avatars use separate seeded, bounded networks. Support independent split-screen sessions, shelter access, body-danger ejection and save/resume.
- Add six programs, two equipped slots, bandwidth, trace, security routines, credentials, routing puzzles, optional caches and rare vaults.
- Connect hacking to power, beacon wards, turret shutdown, robot reboot/recruitment, distractions, story evidence and faction support with exactly-once rewards.
- Add format 10 and independent matrix generator 1. Keep ship generator 6/narrative 3 and preserve formats 1–9.

## Capabilities
### New Capabilities
- `matrix`: network exploration, encounters, programs, persistent operations and controller stories.
### Modified Capabilities
- `persistence`: matrix profiles, spaces, records and active links.
- `shelter`: a safe network workstation.

## Impact
Core simulation, player-space isolation, saves, story events, rendering, journals and keyboard/controller/touch controls. Require >=95% per-file/overall unit line coverage, normal-control journeys, corrupt-save rollback, legacy fixtures, native/Windows builds, sanitizers and packaged browser checks before Pages publication.
