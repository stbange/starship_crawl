# Design

## Context

Space movement already uses free flight for negative Z. The hull has a fixed bay aperture and a bounded camera-relative mesh; the current stars are finite nearby boxes and use interior fog. Static world collision and rendering are separate, so the craft needs a shared geometry source.

## Goals / Non-Goals

**Goals:** Preserve deterministic interiors and old saves, use bounded procedural geometry, maintain camera-relative precision, and make the cabin physically traversable on all input paths.

**Non-Goals:** Flying the craft, new services or collectibles, music changes, or importing models/textures.

## Decisions

- Generator 8 selects the skiff and a starting position farther from the bay. Interior generation and enemy placement retain version-7 salts. Reusing generator 7 would introduce collision inside existing saved positions; a new exterior version avoids relocation and keeps old runs intact without a new payload.
- One small immutable box/fixture catalog feeds the craft's renderer and collision/raycast paths. Zero-gravity movement continues inside its cabin; adding artificial gravity or a teleport would complicate the simple return route.
- A deterministic camera-centered star sphere is drawn as an unlit background before world geometry. Hull ray/aperture visibility suppresses stars behind the effectively endless plane, including beyond the ordinary geometry far clip. Shelter and matrix views retain their own presentation.
- Exterior views use vacuum-appropriate fog and a longer far plane. Sparse emissive hull lamps and dark ribs repeat on a stable world grid, with fixed iteration bounds. Nearby cabin/bay lights use the existing twelve-light selection budget.
- Inspection scenes and browser screenshots cover hull, outward sky, the skiff and its cabin. Gameplay coverage walks the complete arrival/skiff/bay return route and round-trips saves, including optional co-op.

## Risks / Trade-offs

- Far-clip depth precision → Extend it only outside; retain the indoor projection and test close cabin surfaces.
- Stars leaking through the hull → Mask by the actual bay aperture and validate sky/hull screenshots, including rotated and translated views.
- New collision altering old runs → Gate the catalog by generator 8; retain immutable generator-7 saves and compare interior plans/populations.
- Extra exterior work → Bound stars, hull cells and geometry; preserve chunk streaming/upload limits and test mobile/split-screen.

## Migration Plan

Keep format 11 and narrative 3. New runs use generator 8; existing saves stay on their recorded generator. Documentation explains that a new ship enables the skiff. Publishing remains the existing tested Pages workflow when requested.
