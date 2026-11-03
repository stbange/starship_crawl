## Decisions

Navigation is a bounded schematic of procedural floor plans, not another chunk stream. Build at most nine-by-nine tiles from a small region-plan cache. Show north-up rooms, doors, actor facing, objective and shelter. Clamp off-map objectives to an edge marker and indicate deck differences; M opens a larger pannable map with deck browsing. Map UI is transient and does not change expedition records.

Use five procedural body families (husks, gazers, void rays, worms, constructs), five named structural variants each, and a stable species-to-body table. Keep 27 behavior identities, saved kill bits and generator version unchanged. Provide inspection scenes for reviewing every variant.

All weapon volleys reserve projectile capacity before charging. Projectiles have distinct finite speeds/ranges and swept nearest-wall/body collision, retain bounded hit history for piercing, and apply chain/status/blast effects on impact. Never damage instantly at the moment of firing.

Psionics use normal rechargeable energy: veil persists until a successful weapon shot or offensive aura; flight lasts 18 seconds with real collision and vertical controls; long-jump charge lasts 45 seconds; nova pulses around the player for eight seconds. Cloaking removes AI acquisition, not incoming projectile collision. Sanctuary occupants cannot activate powers or damage the exterior. Map and power controls are available on keyboard, controller and browser/touch UI.

Append format-5 power state after the format-4 shelter section. Validate booleans, finite timers and bounds transactionally. Formats 3 and 4 retain ship/profiles/records/missions; older migrations retain their existing policy.

## Validation

Unit-test map parity/markers/bounds, shelter toggling, body catalog coverage, all weapon effects and capacity/piercing, power activation/cost/expiry/collision/targeting/sanctuary safety, and v4/v5 migration. Add gameplay and actual-browser checks and body captures; run coverage, sanitizers, native/WebGL/Windows and strict specs before Pages publication.
