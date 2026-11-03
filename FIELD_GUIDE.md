The ship now has a deck schematic, five distinct creature anatomies, visible weapon fire and four psionic powers. Generator 3 and all 27 species identities remain stable, so existing ship records and missions retain their meaning.

**Navigation.** The top-left HUD map is north-up and follows your position. Green marks your position and facing, gold marks the mission console, violet marks your deployed shelter, and blue marks an ally or the lift needed for a different deck. Off-map goals stay pinned to the appropriate edge. The goal caption states whether you need to change decks.

Press **M** for the larger map. Arrows pan, **Page Up / Page Down** change the viewed deck, **Home** recenters, and **M / Esc** closes it. A controller uses left-stick click, D-pad panning and shoulder buttons for decks. Map controls are also available in the browser menu. Touch has a **MAP** button and a visible **CLOSE** control. A map blocks that player's movement/fire inputs while exterior simulation and the other player continue. It is a procedural schematic, not a record of explored rooms, and does not generate world chunks.

**Shelter toggle.** **R / Back** deploys a doorway, or packs it away when already deployed and empty. Occupants prevent packing. To relocate, pack it and deploy again on clear flooring. **G / D-pad down / USE** still enters doors and uses recovery stations.

**Enemy body library.** These are structural variants: proportions, appendages, wings, mouths, horns, armor, segments and moving parts change. They are assigned across the existing species; behavior traits such as climbing, healing, charging, brood spawning and ranged fire remain attached to stable species identities.

| Body family | Five variants |
| --- | --- |
| Husk — upright bipeds | Drudge, Cleaver, Bulwark, Horned, Reliquary |
| Gazer — tentacled eye orbs | Oculus, Crown, Lantern, Brood, Siphon |
| Void ray — winged creatures | Dart, Crescent, Manta, Thornwing, Seraph |
| Worm — segmented bodies | Grub, Centipede, Serpent, Maw, Leviathan |
| Construct — angular sentinels | Shard, Obelisk, Gyre, Tripod, Cathedral |

Inspection scenes `--scene form0` through `--scene form4` show five variants of each family safely in the bay, with saving disabled. The Warden remains a larger late-expedition encounter.

**Visible weapons.** Every weapon emits finite projectiles: fast carbine/rail bolts, spreading shotgun/flechette volleys, short-range embers, needles, plasma and rockets. Speeds differ; all shots use swept collision so fast bolts hit the nearest body or wall. Piercing, chain, blast, burn, chill and life-steal remain active on impact. A full projectile pool refuses an entire volley without charging energy. Shots take time to arrive and cannot damage through bulkheads.

**Psionics.** These use the same rechargeable energy as weapons and are available to each character from the start. Press a power key again to release an active effect without another energy cost.

| Power | Key | Activation cost | Effect |
| --- | --- | --- | --- |
| Veil | V | 25 | Hidden from enemy targeting until a successful weapon shot or nova activation. Existing incoming shots can still hit. Activating veil releases an active nova. |
| Flight | B | 20 | 18 seconds of flight. Space rises, Ctrl descends; normal movement and aiming steer. Walls, ceilings and unfinished passages still block you. |
| Long-jump charge | J | 15 | 45 seconds of stronger jumps and faster airborne movement. Space jumps normally; crouching and ceilings still constrain it. |
| Nova aura | H | 35 | Eight seconds of half-second pulses, damaging visible enemies within seven metres. Bulkheads block pulses; allies are safe. |

On a controller, hold **left trigger** and press D-pad **left / up / right / down** for veil / flight / long jumps / nova. Touch uses **POWER** to select an ability and **CAST** to activate it; the browser menu also offers each power directly. The HUD shows selected power, energy and active durations. Powers cannot be activated inside the sanctuary, and an existing aura cannot hit the ship from within it.

Format 5 retains active power state. Compatible format-3 and format-4 saves retain ships, missions, profiles, records and any sanctuary while starting powers inactive. Older character migrations keep their existing new-expedition behavior. Map UI is transient and loads closed. See [TESTING.md](TESTING.md) for unit coverage, gameplay simulations, real browser checks and device-testing limits.
