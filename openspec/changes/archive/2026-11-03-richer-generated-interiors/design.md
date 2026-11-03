## Context

The engine streams 96-metre chunks on 64-metre decks. One four-chunk footprint per
region provides large halls. Most furniture is currently repeated, and infestation
is scattered independently of the connectivity graph.

## Goals / Non-Goals

Provide collision-aware side rooms, functional furnishings and large silhouettes;
make run identity and incident trails deterministic. Preserve routes, saved ships,
mobile budgets and native/WASM parity. Multi-deck navigation and interactive ship
systems are outside this change.

## Decisions

- Keep a versioned generator-3 path, including historical hashes and geometry.
  New expeditions use 4. Save 6 already stores generator identity and cache masks.
- Choose a seed-based ship profile to bias sectors and layouts. Compose suites,
  staggered work bays or open arcades with sector prop kits and secondary roles.
- Reserve central crosses and walkable side-space entrances before decoration.
  Fixtures and small debris are cosmetic; furniture and partitions collide.
- Spread a single regional incident over actual open room connections. Its graph
  distance determines intensity, lighting, surface details and infestation.
- Place occasional banquet, conservatory, suspended-engine or reliquary landmarks
  in existing large footprints. Move existing weapon/record rewards off the main
  route without changing their stable collection identities.
- Keep 256 boxes, six lights, 32 dressing groups and two generated chunks per frame.

## Risks / Trade-offs

Detailed rooms can create blocked entrances, inaccessible rewards or exhausted
geometry budgets. Broad deterministic traversal tests check all new variants and
landmarks. Legacy fingerprint and save fixtures prevent accidental regeneration.
Architectural elements stay axis aligned; vertical grandeur uses overhead geometry
inside existing 58-metre halls instead of new movement systems.

## Migration Plan

Load format-3 through format-6 generator-3 expeditions with their old geometry.
Create generator-4 expeditions only on new game/new ship. Version-1/2 migration
keeps profiles and starts the current generator as before. No destructive reset.

## Open Questions

None blocking this milestone.
