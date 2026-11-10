**New ships earn the shared shelter from their first elite or guardian victory.**
The controls below appear when it is available. Earlier saves retain their shelter
access; see [staged onboarding](ONBOARDING.md).

The Fold is a portable refuge: a compact, freestanding doorway opens into an 18 by 16 metre sanctuary with warm steady lamps, a star window, dining furniture, a bed, medical equipment, and a stocked kitchen.

Press **R** on clear, supported ship flooring, then **G** near the doorway. On a controller use **Back / Select** to summon and **D-pad down** to use. Touch players have **FOLD** and **USE** buttons; summoning is also available in the browser menu. Space, cramped passages, obstructions, and unfinished rooms require moving to a wider, surveyed floor. Summoning is free.

Inside, approach a facility and press **G / USE**:

| Facility | Effect |
| --- | --- |
| Bed, ahead left | Six seconds of rest restores health, shield, and energy. Movement, jumping, or firing input wakes you. |
| Medical station, ahead right | Restores health and repairs armor immediately. |
| Kitchen, on the left | Cooks stocked stargrain stew, heals 30% of maximum health, and adds 3 health per second to normal recovery for 120 seconds. |
| Network workstation, near the entrance on the right | After recovering a matrix interface, C or G enters the local ship matrix while your body remains safe. Disconnect returns to the same sanctuary position; see [HACKING.md](HACKING.md). |
| Dining place | Optional 30-salvage meals: electrical resistance, cold resistance or energy recovery for five minutes outside. |
| Locker-side project bench | Shared refit bench, flare calibrator and observed-condition research; priced choices before purchase. |
| Interior doorway, behind the arrival point | Returns you to the ship at your own entry position and restores your camera orientation. |

The pantry is stocked; food does not require ingredients and there is no hunger penalty. Recovery stays within your upgraded capacities. Weapons are holstered inside. Enemies and their projectiles remain in the ship, and sanctuary occupants cannot collect outside caches. The network workstation can remotely complete mapped preparation and system operations through the matrix; final physical story decisions stay at their targets.

The sanctuary is shared in optional local co-op. One player can recover while the other keeps fighting. Each viewport shows its own interior or ship view, and each character retains an individual return anchor. The doorway cannot relocate while either active player is inside or a saved second-player shelter link is reserved. Once everyone leaves, press R to pack it away, then R again on clear flooring to deploy it elsewhere. Starting a new expedition packs it away.

You can save inside and resume there safely. Format 10 stores the doorway, interior state, return orientation, rest, and food benefit. Compatible format-3, format-4 and format-5 saves retain the same ship, missions, characters, and changed-room records and any existing sanctuary; format 3 starts with no sanctuary. Saving upgrades them to format 10, including matrix state when linked. Generator 3 is unchanged. Existing version-1/version-2 character migration remains supported.

For inspection, `--scene shelter --no-save` opens the sanctuary without writing a save. Add `--split` to see a second character outside the door. Automated unit, campaign, browser, and native render checks are described in [TESTING.md](TESTING.md).

The medical station also repairs both of your fold rigs, whether parked or packed. Dismount before deploying the shelter. See [EQUIPMENT.md](EQUIPMENT.md).

Generator-16 expeditions add optional preparation and six outcome keepsakes; free recovery remains unchanged. See [current preparation rules](COHERENT_EXPEDITIONS.md#e5--useful-shelter-preparation). These additions use checkpoint format 17.
