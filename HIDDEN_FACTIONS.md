# Hidden factions and the living ship

New ships use generator **14**, faction simulation **1**, narrative **3**, matrix
**1** and save format **15**. Alien powers act while you explore: columns travel
between supply sites, rivals fight, defeated troops retreat, and stations change
hands. You can follow them, avoid them or interfere. There is no political quest
to accept. **M**, the minimap and the ordinary story journal still guide your
expedition; **F6** opens the discovered ship dossier.

Existing generator-13 expeditions retain their courts and visible dispatches,
including their saved decisions and controls. [The legacy guide](POLITICS.md)
describes those ships. Starting a new ship resets both characters and the world.

## Reading the inhabitants

Three governments occupy seeded districts, separated by unclaimed territory.
Infestation also exists independently of them. Markings have different shapes,
as well as colours, so groups remain distinguishable in dark rooms:

| Government | Visible clues | Behaviour |
| --- | --- | --- |
| Divided Crown | Three upright crown prongs, mechanical guards | Competing priorities favour different decks on successive deployments |
| Molting Court | Arched shell markings, carried supply blocks | Biomass raids alternate with periods favouring protected supply processions |
| Parliament of Echoes | Floating diamond markings, signal apparatus | Power objectives and faster coordinated deployment; silenced relays delay orders |

Peaceful patrols leave you and story terminals alone. A nonlethal stray hit
produces a warning call, two raised signal bars and a short local memory. Repeated or lethal attacks
provoke local defence; surviving witnesses can report the attack after five
seconds. Repeated reports spread hostility. Disengagement allows it to decay.
Your split-screen partner has separate incident history. Hacking a relay can
interrupt a report that has not been transmitted; it does not erase earlier ones.

Troops carry supplies, prepare glowing attacks, and signal a retreat with a
diagonal marking. Travel pulses, warning calls and attack sounds supplement the
shapes. Cues pan with your view in solo play; in split-screen they favour the
speaker channel matching the affected viewport. Mobile formations currently contain three members. Multiple formations
can share a campaign, with arrivals deferred when a room is full.

## Causes and consequences

Power, biomass and fabrication sites support recruitment and convoys. Forces
consume supplies when deployed; cargo is taken from a stockpile and can be
delivered, returned or lost. Shortages, losses, government priorities and current
relationships choose the next objective. War, truce, tribute, symbiosis and
succession evolve on a slower clock. These decisions continue without the director.

Near either player, troops use doors, stairs and eligible lifts and fight through
the ordinary collision/projectile systems. Distant travel and battles use a
coarser simulation. A formation has one roster throughout: changing rooms,
leaving the cache or reloading cannot create another copy or revive a casualty.
Occupied sites can acquire damaged supplies, discarded shells, broken
emplacements, guards, changed lights, growth and spores. These decorations leave
crossings, story terminals and lift approaches clear.

Destroying an escort removes its cargo. Capturing a site changes the supply
network. Existing matrix power and silence operations affect its equipment and
orders. The effects are saved; returning can reveal an occupied or abandoned
station without a message explaining the entire war.

NPC-only casualties give no player kills, XP or equipment. Meaningful player
participation can earn ordinary XP and finite battlefield salvage. Mobile patrols
do not grant guardian equipment. Weapons, rigs and powers retain the existing
challenge/participation requirements and once-only claims. Required story sites,
phase guardians and their approaches, extraction and the shelter are protected.

## Pacing behind the scenes

The director follows each outside player's pressure, defences, energy, nearby
threats, sustained combat and exploration separately. Recent observed body families,
formation orders and deliberately avoided groups inform repetition memory. It shares one budget and remembers recent
interventions. It can fund a scout, place an abandoned field kit or optional nest,
substitute an unseen encounter with a compatible ecology, alter an upcoming
atmospheric effect, redirect a patrol, delay reinforcements,
omit unseen optional infestation, defer a hazard or order a physical withdrawal.
It can also leave events alone. High pressure starts a recovery window; it does
not increase the statistics of an enemy already fighting you.

Changes are proposed first and checked again before committing. New pressure can
cancel a pending unseen attack without spending its budget. Both cameras,
nearby bodies and projectiles, known rooms, existing combat, goals, casualties and
claims constrain them. A visible squad must walk away rather than vanish.
The system has no player-facing countdown, seal, director menu or faction map.

Strategy advances once every two seconds of active play. Pause, suspended play
and shelter occupied by every player stop it. An exposed body in the matrix
keeps the physical world active; when nobody explores physically, the director
stops introducing encounters. Loading does not simulate time spent away.

## Scope and persistence

This is a bounded simulation around the players, with retained travelling
campaigns, rather than a simulation of every room of an infinite ship. It caps
active formations at 32, members at three per formation, room actors at 20 and
recent diagnostic events at 128. Strategy considers at most fourteen local
resource sites per update. Sparse occupation, casualties and claims remain after
cache eviction; their save cost grows with explored changes.

Format 15 saves the simulation clock, deterministic decision state, resources,
relationships, incidents, rosters, travel, occupation, director memory and pending
interventions. Loads validate them before replacing the current expedition.
Generator-13 and earlier ships retain their original rules and compatible save
path. Physical room proxies and transient animation effects are reconstructed.

See the [implementation and playtest review](docs/reviews/2026-11-08-hidden-simulation/README.md)
for measured limits, retained failures, automated journeys and current validation.
The [human checklist](docs/playtests/HIDDEN_FACTIONS_CHECKLIST.md) covers the
readability and feel that automated simulations cannot establish.
