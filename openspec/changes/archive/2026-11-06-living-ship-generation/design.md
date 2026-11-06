## Context
The existing 4×4 region graph and bounded chunk cache must remain deterministic and compatible with phases, mission sites, stairs, rigs and split-screen.

## Decisions
Use a footprint bit mask alongside bounding spans; holes belong to other rooms and are excluded from ownership/stream completion. Reserve macro geometry before filling single-cell rooms. Keep tutorial, catastrophe-origin and guardian reservations. Paired decks share a deterministic macro footprint; the lower floor opens vertically into an upper gallery with protected cross routes. Logical per-deck room records remain independent.

Derive environmental conditions and local tales from room function, connected incidents, directional traits and history. Furniture, sprite anchors and weighted tier-appropriate inhabitants share that profile. Preserve guardian encounters. Surface water is shallow cosmetic runoff; rain/steam are non-damaging and avoid inventing swimming rules.

Procedural pixel sprites require no external assets: depth-tested transparent quads, bounded seeds and visible budget, per-view sorting, no simulation allocations. Decorations are anchored to surfaces. Fog and effects reset independently for each camera and are absent in the skiff, shelter and matrix.

Local accounts appear in the existing dossier for the current room. These accounts are deterministic observations, not new quest state or rewards; no save format bump is needed. Generator 10 adds fields to semantic fingerprints without altering older hashes. Freeze real generator-9 save, geometry and population fixtures before editing.

## Risks and Validation
Nonrectangular holes and upper gallery voids require ownership, support, streaming and physical-route tests. Bounded sprite work and alpha sorting require real WebGL/native captures. Verify native/WASM fingerprints, older saves, separated co-op, phases and all browser regressions. Preserve >=95% unit-only line coverage overall and per executable core file. Physical GPU/mobile checks remain explicitly unverified.

Limit macro footprints to six occupied tiles. At most four regions intersect a 3×3 neighborhood, so a room adds at most five cells beyond the touched tile. The two-player desired union is bounded by (9 + 4×5) × 3 decks × 2 players = 174, below the 192-resident limit. Seven-tile footprints could exceed that bound at adverse region corners.
