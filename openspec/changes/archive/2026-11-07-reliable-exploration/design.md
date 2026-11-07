## Approach

Use a bounded topology search from deterministic plans, cached per player and refreshed on movement/goal changes. Guidance distinguishes a nearby connection from the remote objective and never advances discovery or streams rooms. Floor-level approach points use the protected central cross routes; service stair landings get explicit return-to-deck cues.

Lift input latches after successful departure and clears only after release following arrival. Failed requests remain retryable while a destination streams. Show adjacent destination themes and phase locks before departure, then a local exit cue on arrival. Keep camera control with the player throughout.

World accents are renderer-only, depth tested and bounded to nearby loaded spaces. They preserve collision fingerprints and darkness between focal points. Persistent HUD prioritizes health, weapon, minimap and next action; context remains available in I/F6/F10.

Automation uses production controls and deterministic reaction/aim limits. Measure resources every tick and report simulation duration separately from wall time. Retain ideal-aim correctness journeys and add distinct realistic-input profiles; neither substitutes for first-time human or physical-device testing.
