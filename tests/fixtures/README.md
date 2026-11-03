`generator-v1.sav` was written by the unmodified version-1 save writer from commit
3433429. It contains seed 123456, a defeated encounter, and two characters:

- Player 1: rank 7, 31 XP, 81 kills, 4 points, damage 3, vitality 2, tint 2.
- Player 2: rank 4, haste 2.

Migration must preserve these profiles, discard incompatible expedition geometry
and encounter records, announce a new expedition, and resume in solo mode.

`generator-v2.sav` was written by the unmodified version-2 core from commit
038c64b, before the infestation update. It contains seed 713, a defeated enemy
(ID 1) in room `(1, 0, 0)`, and two characters:

- Player 1: rank 7, damage 3, 81 kills, position `(96, 0.02, 23)`, checkpoint at that room's lift.
- Player 2: rank 4, haste 2.

Generator 3 must retain both character profiles and announce a fresh expedition,
discarding the incompatible old ship, positions, checkpoint, and encounter records.
The new profile receives starter weapons, armor, and a shield.

`generator-v3.sav` was written with the unmodified format-3 writer from commit
79014a0. It contains seed 713, mission stage 2, a defeated encounter (ID 0) in
room `(1, 0, 0)`, and player 1 at rank 7 with 81 kills and 123 salvage at `(96, 0.02, 24)`.
Format 4 must retain this expedition, character, and record without changing the
seed or mission. The sanctuary starts undeployed.

`generator-v4.sav` was written by commit 5e1e9f6's format-4 writer. It holds
seed 713, mission stage 2, rank 7, and a deployed shelter with player 1 inside.
Format 5 retains the expedition and sanctuary, initializing psionic effects inactive.

`generator-v5.sav` was written with commit cd4d0a5's unmodified format-5 core,
starting from the v4 fixture and activating player 1's veil and ten seconds of
flight. It retains seed 713, mission 2, rank 7 and the occupied deployed shelter.
Format 6 retains those states and records, grants the four new personal guns, and
initializes both fold rigs packed with full plating and flare upgrades at zero.

`generator-v6.sav` was written by the unmodified format-6 core from commit
e3f5013, loading the v5 fixture first. It retains the occupied shelter, powers,
rank 7, mission 2 and generator 3. Generator 4 must keep this expedition on the
old generator, including save/reload, until the player requests a new ship.

`generator-v3-fingerprints.txt` contains 12 region fingerprints captured from
e3f5013 before generator changes. `generator-v3-geometry.txt` contains 14 chunk
geometry/fixture/dressing fingerprints generated from that commit's source.
The quantized field hashing is in `geometry_fingerprint.h`; it never hashes
struct padding. Tests compare the supported generator-3 path against these
pre-change references, including the bay, introduction rooms and all nine decks.
