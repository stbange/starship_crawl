# Design

## Context
Growth and nest dressings already have deterministic positions, normals, scale and hashes. Their static roots live in chunk VBOs. Gazer anatomy currently has short segmented appendages. See proposal.md for scope.

## Goals / Non-Goals
Add organic motion and readable hooked silhouettes within the existing low-resolution renderer. Environmental tentacles are visual dressing; attacks, grappling, destruction and new collision are outside this change.

## Decisions
- Build connected low-sided tapered rings along a curved, time-driven spine, with small sucker rings and emissive tips. This keeps geometry lightweight and avoids new asset dependencies.
- Keep static root mats; append dynamic tentacles in the opaque world pass from the existing seeds and surface normals. This preserves all generator/save fingerprints and avoids rebuilding chunk VBOs each frame.
- Select at most 32 nearest visible growth anchors, up to three arms each, with lower segment/detail counts at distance. Use game time for pause and shared co-op consistency.
- Reuse the shape for gazer tendrils in their existing local anatomy and death pose. Environmental and enemy animations retain separate size and timing.
- Expose read-only per-view draw counts and add a saving-disabled inspection view of actual seeded growth for browser verification.

## Risks / Trade-offs
- Decorative limbs can overlap nearby props → short bounded reach from existing off-route roots; depth testing preserves occlusion and no collision is added.
- Dynamic geometry costs CPU work → fixed nearby anchor budget and distance detail reduction; test split-screen and emulated mobile views.

## Migration Plan
Ship with the ordinary main/Pages workflow. No migration or new ship is required. Reverting renderer changes restores the old appearance without touching saves.
