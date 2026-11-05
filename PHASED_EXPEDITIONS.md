# Phased expeditions

Every **new ship starts two fresh character profiles**, including an inactive
co-op partner. Saving, checkpoint death and additional commissions on the same
ship retain the current build. Replaying the same seed also starts fresh.

Begin with a pulse carbine, breach shotgun, flashlight, shelter, map and basic
matrix access. Advanced weapons, psionics, the mech and wagon come from guardian
rewards. Ten attributes remain upgradeable, with a cap of 12 on new ships.

## Choose a route and earn a build

Accept the bay commission with **G**. **F10** opens the build and guardian journal:
preview three rewards before committing, cycle routes with **U**, and claim one
reward with **1 / 2 / 3** after a victory. The optional Threshold Hunter one deck
above the bay provides an early capability. It does not advance the main phases.

| Phase | Available space | Required challenge |
| --- | --- | --- |
| Foothold | First band around the loading bay | One of two guardian halls |
| Commitment | Next horizontal regions and deck pair | One of the next two halls |
| Transformation | Larger, more strongly themed regions | One of the third pair |
| Reckoning | Deep story target and final pair | One final guardian, then the story conclusion |

Each band expands by four cells horizontally/inward and two decks vertically.
The halls lie on opposite sides of the bay's inward corridor. Either guardian
opens the next boundary for both players; its counterpart remains an optional
source of rewards. Gold barriers, lift indicators and the map mark sealed layers.
Walking, vehicles, flight and lifts share those limits. After the fourth victory,
the ship remains open for endless exploration and further commissions.

Investigate the generated witness lead after establishing a foothold. The main
target is reserved in the fourth band, optional primary preparation in the third.
These follow the established foothold wing; investigating before either guardian
uses a seeded wing. Revealed destinations never move.
The journal can switch guidance back to the story. Completing a target requires
all four phase victories; extraction and further commissions retain their existing
story choices. Restoring power extends guardian openings; silencing security
reduces their volleys. Local matrix services and completed physical power/security
operations provide preparation. Beacons cannot make an active guardian fight
invulnerable, and robot controls cannot recruit a guardian.

## Fight, recover, retry

Use **G** at a guardian's staging console to begin. The Foundry Marshal fires fans,
the Convoy Devourer telegraphs a charge, the Choir of Glass alternates rings and
fans, the Cryogenic Custodian moves laterally and chills, and the Firewall Saint
drains energy. All fire visible avoidable shots and expose a glowing core after
attacking. Basic weapons can defeat every family. Health scales by phase and by
the players present when the encounter begins; ordinary hordes retain bounded
health growth.

If all participants leave the room, die or enter shelter, the attempt resets to
full health. Owned rewards remain. Loading an active fight returns its participants
to staging and resets that fight. A co-op partner can recover while another stays
engaged. Join and enter the arena before starting to participate in its reward;
each eligible character has an independent, saved claim. Duplicate choices clearly
offer two upgrade points and 100 salvage.

## Equipment and specializations

Carry **two personal guns, two powers, one rig and one specialization**. Other
owned items remain in storage. At the bay, inside shelter or within eight metres
of a defeated guardian's console, use F10, **Q / E** to select a slot, and **Z / X**
to change its equipped item. Dismount or disconnect from the matrix before refitting.
Reward claims equip a new capability immediately; they cannot be claimed twice.

| Specialization | Effect |
| --- | --- |
| Mobile Storm | +60% weapon and nova damage while flying |
| Siege Engineer | +50% mech weapon damage |
| Silent Hunter | +150% damage on the shot that breaks veil |
| Systems Saboteur | +60% damage in a room with silenced security |

On a controller, **left trigger + Start** opens the build journal, shoulders
select a slot, left/right refit, X changes the route, and A/Y/B claim choices.
Start closes it. The browser menu provides the equivalent touch controls.

## A different directional identity on each ship

The seed assigns heat/cold and order/chaos to the lateral and vertical axes,
including their polarities. F10 shows the compass; the HUD shows local strength.
Traits grow with displacement. Ordered areas favor suites and machines; chaotic
areas favor workbays and organic swarms; temperature changes dressing and enemy
selection. Ceiling scale increases across deeper bands while narrow conduits and
crew spaces retain their contrast. Existing sectors, landmarks and connected
catastrophe trails remain part of the generator.

New expeditions use **generator 8, narrative 3, matrix generator 1 and save format
11**. Formats 1–10 remain supported. Compatible older ships keep their equipment
and original rules until an explicit new-ship action. Historical `--scene` catalog
views remain generator-6 sandboxes; `--scene guardian` previews the new arena.
Normal play and `--seed` start the new ruleset. Generator 8 adds the [arrival skiff](ARRIVAL.md) while retaining generator-7 room plans, encounters and phase rules. Existing generator-7 saves keep their original exterior and positions.

The broader [design proposal](PROGRESSION_DESIGN.md) also describes possible future
vehicles, artifact variants, stat reassignment and encounter combinations. Those
are playtest directions. This implementation uses the existing mech/wagon and
four explicit specialization modules. Automated campaigns establish completion
and persistence; human playtesting must assess pacing and challenge.
