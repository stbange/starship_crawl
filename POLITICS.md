# Courts and the encounter director

This guide describes **legacy generator-13 expeditions**. New generator-14 ships
use [hidden factions and an invisible director](HIDDEN_FACTIONS.md), with travelling
forces and environmental consequences. The courts and controls below remain
available when resuming a generator-13 save.

Existing generator-13 expeditions retain their original geography and populations.
Each of those ships has three major alien countries, each
with an audience chamber, disputed conduit and royal-guard finale. Their names,
rulers, government assignment, initial relationships and physical setting derive
from the seed. Territories span sectors and decks, with wilderness between them.

## Finding a court

Press **F6 twice** (dossier, then courts), or choose **Alien courts** in the browser
menu. **L** reads another country; **U** tracks or untracks its current destination
in gold on both maps and the navigation cue. Selecting an ordinary story or
guardian route clears political tracking. Colored strips mark territory; **C**
marks political sites. Banners and marked patrols identify countries in the world.

At a court's floating seal, press **G / touch USE** for an audience. **1 / 2 / 3**
accept a commission; **Enter** leaves undecided. Gamepads cycle journal pages with
the right shoulder, read with X, track with Y, and choose with D-pad left/up/right.
The browser menu exposes the same choices and controls on touch devices.

| Government | Conflict | Court appearance |
| --- | --- | --- |
| Divided Crown | One monarch's bodies disagree; one has cut the others' heat | Three suspended faces and broken crowns |
| Molting Court | A stolen royal shell carries the missing food tithe | Nested ceremonial carapaces |
| Parliament of Echoes | A forged ancestral vote has cut archive power | Floating memory prisms and signal rings |

Countries can be at peace, at war, in tribute or sharing a nervous system.
Country-to-country relationships and your personal standing are separate. Marked
neutral patrols defend their room and leave peaceful players alone. They also
leave story terminals accessible; hostile patrols can contest nearby terminals. Shooting one
makes that country hostile to that player, with a visible warning. Patrol kills
do not award the player experience.

## A dispute you can finish

All three commissions require securing the conduit, claiming its evidence, then
defeating the royal guard and using its seal. These are optional squad battles;
ordinary weapons always work. Courts introduce flight, area-attack or matrix-aware
encounters according to their government. Exits stay open. The royal guard has
fixed additional protection, independent of the player's performance.

1. **Restore shared supply:** establish peace with the neighboring country and
   restore a powered protective ward at the conduit.
2. **Install a successor:** change the ruler, establish tribute and open a court
   workshop that restores armor and energy.
3. **Seize the conduit:** annex the disputed macro region and make the neighboring
   country's patrols hostile. The new ownership appears on banners and maps.

Completed courts repair armor for players in good standing. Each eligible finale
participant receives two upgrade points and 100 salvage once, in addition to their
ordinary encounter equipment claim. Decisions are shared in split-screen; personal
offenses and equipment claims are independent. All settlements survive travel,
save/resume and death. A new ship resets them with the characters.

Political finales supplement the four existing guardian phases. This release has
one finite dispute per country per ship; repeated commissions do not regenerate
the countries or erase their settlements.

## The director

The director chooses one physical dispatch at a time:

- **Field supply cache:** modest health, shield and energy recovery after pressure.
- **Diplomatic courier:** an invitation to an unresolved court.
- **Architectural survey:** a landmark observation and 25 salvage.
- **Border conflict:** an invitation to an existing optional battle in a country
  involved in war. USE accepts the actual encounter; it does not spawn an ambush.

Cyan dispatch seals appear in the world and map, with a three-minute availability
window. USE claims them after nearby danger is cleared. Each active participant
can claim once. The director considers both players' condition, recent pressure,
new rooms, recent dispatch kind, a three-point budget and a one-minute cooldown.
It cannot rewrite explored rooms, adjust enemy statistics or remove a victory.
Dispatches use the first eligible active player's room; separated co-op players
share one offer, so reaching it can require travel.

Its clock stops while paused or when everyone is in shelter/matrix. Pending seals,
remaining time, budget, last rooms, claims and the last eight reasons survive a
save. Read-only browser diagnostics expose the latest reason and counters. These
rules provide a bounded first event library; new governments, migration, larger
wars and additional event types can extend it after playtesting.

## Compatibility and evidence

Save format 14 appends explicit little-endian political/director state to the
existing payload. Loading validates bounds, completed battle records and sites
before replacing the current game. Formats 1–13 remain readable; generator 3–12
ships do not acquire new countries on resume. Narrative 3 and matrix 1 remain.

Automated journeys use an earned generator-13 expedition checkpoint and normal
movement/look/fire/use inputs. Their late-game character makes some finales short;
these runs establish functional routes and persistence, not human combat balance.
Inspection scenes `court0` through `court2` show the three real seeded courts with
saving disabled. The player guide and tests distinguish those scenes from earned
journey checkpoints. See [TESTING.md](TESTING.md) for the recorded verification.
