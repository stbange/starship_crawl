# The Ship Matrix — proposed hacking layer

This is a design proposal. Terminal login and matrix gameplay are not implemented yet.

The ship has two overlapping worlds: its steel corridors and a damaged network
that still believes it is maintaining a living vessel. Enter a terminal and
explore that network in first person. Cathedral aisles become circuit channels,
crew districts become memory stacks, and the reactor becomes a distant, pulsing
sun. The same retro 3D style gains black glass, luminous symbols, broken data
bridges and the silhouettes of creatures trapped in obsolete security routines.

## Two spaces, one ship

Each terminal is a fixed entry point into its physical sector's network. Network
nodes represent actual rooms, controllers, relays, archives and sealed systems.
Connections derive from the seeded ship plan, with useful alternate routes across
physical barriers. Machinery sectors have dense power-routing networks; luxury
decks contain guest identities and surveillance; command sectors have security
gates; research sectors hold quarantined data and unstable AI fragments.

Only a nearby part of the network is generated and loaded. Returning through the
same terminal finds the same layout and committed changes. An unexplored node
shows its function, not every secret inside it. Browsing the schematic never
advances a mission: you must enter, investigate or operate a node.

Your body stays at the terminal. Field dives should be short, with a clear warning
and quick disconnect when the body is threatened. Powered refuge terminals and
a shelter workstation support longer, safe sessions. In optional split-screen,
one player can guard the terminal while the other dives; both can also dive or
continue independently. Every objective must remain achievable alone.

## The first playable loop

1. Find a terminal and read its visible services: power, security, archive or transit.
2. Enter the matrix as a small luminous avatar, with a distinct HUD for integrity,
   bandwidth, trace and the route back to the terminal.
3. Explore a compact network of rooms and bridges. Locate an access key, route
   power around a damaged connection, or open a timed path through a firewall.
4. Face security programs and corrupted routines. Use programs to distract,
   interrupt or bypass them; direct combat remains a valid approach.
5. Commit a useful action at its controller node, then disconnect and see its
   consequence in the physical ship.

A typical dive takes two to five minutes. Deeper optional branches offer better
rewards and more revelations. The main mission should not require a long hacking
detour every time you arrive at a new room.

## Programs and pressure

Start with two equipped programs and a basic defensive pulse. Suggested programs:

| Program | Use |
| --- | --- |
| Veil | Slip past a patrol until you operate a secured node or attack |
| Echo | Send a decoy along a selected connection |
| Interrupt | Stop a security attack or briefly freeze a firewall gate |
| Bridge | Reconnect a broken data crossing for a limited time |
| Fork | Operate two nearby switches during one timing window |
| Siphon | Recover bandwidth from a defeated security routine |

Bandwidth limits repeated powerful actions. Trace rises when you force a gate,
fight loudly or take protected data. Show its next threshold before it happens:
first additional patrols, then locked connections, then a clearly announced
counterintrusion. Exploration alone should not be a relentless countdown.

Failure ejects the avatar, briefly locks that terminal and costs temporary
bandwidth or uncommitted data. It retains the character, equipment, confirmed
story discoveries and completed physical work. A player can always disconnect;
there is no requirement to replay an entire mission after a failed dive.

Existing Reactor, Precision and Recovery attributes can influence bandwidth,
interrupt timing and integrity recovery. Found programs and upgrades provide
build variety without requiring a second full character progression grind.

## Consequences that make hacking worthwhile

| Network action | Physical result |
| --- | --- |
| Reconnect a microgrid | Restore local lights and recovery |
| Authenticate an evacuation beacon | Activate its ward after power is restored |
| Override a security controller | Silence its local turrets |
| Reboot a robot | Temporarily stop it; a later program could briefly recruit it |
| Reassign maintenance traffic | Open a service route or send a temporary distraction |
| Recover a witness checksum | Establish a story fact or reveal a known lead |
| Copy an archive partition | Recover knowledge without destroying its original |

These actions should use the same ship-system state and mission prerequisites
as physical operations. Completing an operation by hacking must not award it
again at its physical console. Important final decisions still take place at
the mission target; hacking supplies evidence and alternative preparation.

The factions give hacking a purpose: Refuge wants evacuation systems and life
support restored; Keepers wants archives and living controllers preserved.
Overriding a node can solve an immediate problem while damaging trust or
redirecting a later commission. Changes appear in the existing event history
and debrief. ORISON can first speak through corrupted network nodes, with its
actual identity revealed only through evidence.

## A concrete example

A sealed research district blocks the route to a stranded refuge. Its terminal
reveals an evacuation controller behind a corrupted firewall. Inside the matrix,
the player can recover a maintenance credential from a side branch, defeat the
firewall's guardian, or spend bandwidth to force access and increase trace.
At the controller, they choose whether to cut security power or preserve the
research archive while rerouting it. Back in the ship, the appropriate turrets
go quiet, the service route becomes usable, and the refuge can receive power.
Later transmissions acknowledge which systems and records survived.

## Proposed implementation milestones

1. **Terminal and return path.** One compact seeded network per terminal, a
   separate player space, matrix HUD, safe login/disconnect and optional
   split-screen isolation. Commit only a local power operation initially.
2. **Hacking encounters.** Security routines, bandwidth/trace, three programs,
   an access-key branch and a short routing puzzle. Keep a solo bypass available.
3. **Persistent consequences.** Security and beacon controllers, robot reboot,
   saved node states, exactly-once rewards, story evidence and faction responses.
4. **Sector variety.** Different network architecture by ship layer, rare archive
   vaults, program upgrades, corrupted refuges and a generated ORISON storyline.

Keep physical and matrix coordinates separate and associate network actions with
stable physical owner cells and system IDs. Reuse the C renderer, collision and
streaming budgets, but give matrix generation its own version and persistence
validation. Saves must identify each player's space and return anchor; a failed
import must leave both spaces unchanged. Before expanding the network, test
login/return, body danger, disconnect during combat, co-op isolation, old saves,
streaming determinism and every physical consequence through real controls.
