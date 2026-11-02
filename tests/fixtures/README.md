`generator-v1.sav` was written by the unmodified version-1 save writer from commit
3433429. It contains seed 123456, a defeated encounter, and two characters:

- Player 1: rank 7, 31 XP, 81 kills, 4 points, damage 3, vitality 2, tint 2.
- Player 2: rank 4, haste 2.

Migration must preserve these profiles, discard incompatible expedition geometry
and encounter records, announce a new expedition, and resume in solo mode.
