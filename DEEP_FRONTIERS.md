# Deep frontiers

Gameplay-review items **2, 4, 5 and 6** extend P1 with encounters for different
builds, architectural reveals, deeper directional rewards and stranger enemies.
Start a **new ship** for generator 12. Existing ships keep their original geometry
and populations; format 13 reads earlier saves and retains both players' progress.

## Encounters and earned tools

White optional consoles preview the fight, its reward and any regional contract.
Press **G / USE** to awaken the four-member squad. Exits remain open. Ordinary
weapons can win every pattern; the suggested capability offers another approach.
Defeat the squad and return to claim. Dismount before using a console.

| Encounter | Useful approach | First unowned specialist reward |
| --- | --- | --- |
| Hanging Aerie | Fly above the elevated core to remove its ward; ground fire can still break it | Flight |
| Coat Colony | Bring Nova or splash weapons against the clustered, linked organisms | Nova Aura |
| Suppression Battery | Advance between alternating cover; mech plating absorbs the volleys | Fold Mech |
| Courier Circuit | Touch both cyan pylons within 18 seconds to remove the ward and disrupt the squad for six seconds; a wagon shortens the crossing | Fold Wagon |
| Cipher Lock | Link at the room's physical terminal and run **SECURITY** to remove the ward | Saboteur module |

The original linked frontier remains in the mix. A specialist tool already owned
falls back to the existing reward pool. Ownership and equipped loadout differ:
use **F10** to refit at the bay, shelter or a defeated guardian's staging console.
Rewards and relics are claimed once per participating character, including co-op.

## Deeper routes

From **layer 5**, optional fights inherit a heat, cold, order, chaos or alien
contract from the ship's seeded directions. The console names the risk and the
next eligible evolution before activation. Hazard rings warn for 1.4 seconds,
then activate briefly; moving out of the patch or above it avoids the effect.
They rotate through local positions and leave the console and exits available.

Relics evolve through three ranks, requiring victories at layers **5, 8 and 11**.
They belong to this expedition's character. Death and save/resume retain them;
starting a new ship resets them. F10 lists the five ranks in heat/cold/order/
chaos/alien order. Ordinary enemy health scaling keeps its existing cap.

| Contract / relic | Hazard | Rank I → II → III |
| --- | --- | --- |
| Thermal Purge / Cinder Heart | Burning vent patches | Blasts burn → wider blasts → cheaper blast weapons |
| Brittle Harvest / Rime Lens | Slowing frost fields | Beams chill → longer beams → extra piercing |
| Choir Audit / Aegis Coil | Energy-draining ion fields | Powered rooms recharge shields while unhurt → repair occupied mech plating → faster recovery |
| Colony Excision / Molt Seed | Damaging spore patches | Nova chills → larger Nova radius → hitting with Nova restores some energy |
| Mirror Recovery / Rift Sail | Shield-draining void fields | Faster flight → cheaper psionics → quieter movement |

## Architecture and creatures

Major rooms now frame nine sector silhouettes: sleeper spindles, broken turbines,
inverted choirs, fallen chandeliers, command irises, specimen crowns, cargo rib
cages, frozen leviathans and rooted suns. Repeated dim lights make height and
distance legible; landing marks invite a look into the hall. These supplement
existing macro rooms, weather and local stories. Express lift routing remains a
separate optional proposal.

Ten new species bring the roster to **42**:

| Creature | Behavior and counterplay |
| --- | --- |
| Listening Membrane | Hears footsteps and gunfire, then warns and alerts its existing neighbors. Crouch quietly or interrupt it. |
| Gaze-Bound Scavenger | Freezes when either physical player sees it without an obstruction. Looking away releases it; it moves through the world. |
| Reversing Worm | Changes its active mouth, sending a reversal through its body. Both ends can be shot. |
| Colony in a Coat | Splits into three moving, hittable pieces and reforms; they share one health pool, death and reward. |
| Molting Sentinel | Sheds armor once after enough damage, exposes itself during the split, then moves faster. |
| Wrong-Foot Machine | Alternates stillness with abrupt hydraulic motion. Its clicks warn a committed rush; recovery exposes it. |
| Doorframe Ambusher | Waits in a physical side recess, unfolds into a short lunge and retracts. Watch the moving seam. |
| False Reflection | Its distorted pool image betrays a reachable body. Follow the ripple and dodge the glowing shot. |
| Brittle Scavenger | Favors cold regions. Cracks audibly before a short burst; shooting during that warning interrupts it and does extra damage. |
| Pressure-Fed Ambusher | Favors hot regions. Inflates between vent pulses, fires a warned projectile, then deflates into a recovery window. |

Seam-runners can climb real walls onto ceilings and mark their committed drop.
Warning sounds, attack poses and HUD hints describe the same simulation states.
Physical cover, observation by a partner, shelter and matrix-view isolation apply.

## Inspection and validation

Saving-disabled views: `--scene front0`…`front5`, `deep0`…`deep4`,
`reveal1`…`reveal9` and `alien0`…`alien9`; the web equivalent is
`?seed=713&scene=front1`. They are inspection setups, not earned progress.
The `frontier-*.sav` files are separately produced through normal gameplay;
see [fixture provenance](tests/fixtures/README.md).

Unit tests cover compatibility, hazards, tool effects, linked hitboxes,
cooperative observation, claims, saves and ceiling contact. A separate journey
uses earned equipment, shelter refits, ordinary movement/fire and real matrix
Veil/services to compare approaches. Browser tests exercise the shipped WebGL
build and controls. See [TESTING.md](TESTING.md) for measured results.

Automated late-game builds still clear some arenas quickly. These checks establish
working mechanics, not human difficulty: early reward pacing, telegraph readability,
build balance, sound mix and physical mobile/controller performance need playtests.
