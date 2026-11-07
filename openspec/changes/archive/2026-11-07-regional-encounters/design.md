## Design

Keep the six existing anatomical families and 32 species. Generator 11 assigns
three motion/decision archetypes to compatible bodies; shared transient states
drive both simulation and animation. Committed attacks retain their target,
respect geometry and have visible warning/recovery windows. Co-op uses the same
world state and either player can draw attention.

Optional encounters reserve four existing enemy slots: a vulnerable support and
three different attackers. Interaction wakes them; exits remain open. A living
support gates staggered attack commitments and visible shielding. Beam pressure
can interrupt its links; any weapon can destroy it. Its death disrupts the pack.
The coordinator owns its four-slot population and capped health budget, so generic
brood spawning, healing and regeneration do not run on that support. No infinite
health scaling or mandatory equipment gate is added.

Use bounded per-room transient coordination. Persist only completion, earned
eligibility/claims and resolved local actions in an appended format-12 section.
Defeated identities use existing kill records. Reload discards unfinished attack
commitments but preserves kills and earned rewards, avoiding repeat loot.

Seeded room-function weather variants and pacing are generator-versioned. Local
actions use existing physical evidence and persistent ship systems; resolved
scenes visibly light up, calm venting or disable machines. The main story route
remains available. Reward drills occur in cleared arenas and introduce the real
earned ability without granting extra equipment or changing the main objective.

All new world effects remain bounded and isolated from matrix/shelter/exterior
views. No forced camera movement, invisible attacks or fake interface effects.
