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

Loading must retain the expedition, positions, checkpoint, defeated enemy, and
both profiles. It starts solo with flashlights on and no in-flight projectiles.
