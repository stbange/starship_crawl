# Session architecture (R1–R7)

The game still ships as solo play with optional two-view local co-op. The refactor
adds a headless session boundary and a tested four-character core configuration;
it does not add an Internet transport or change expedition generation.

| Refactor | Ownership and interface |
| --- | --- |
| R1: commands | `commands.h/.c` defines semantic commands, argument bounds, player validation and device labels. SDL keyboard/controller, touch and Quest adapters call the same dispatcher. `client_actions.h` names the existing browser menu numbers without changing them. |
| R2: state | `Player` holds character/body state. `PlayerView` holds journals, maps, hints, local selections and HUD messages. Pending story decisions live in `InteractionState`. `presentation.h` provides sequenced, addressed events and party broadcasts. |
| R3: simulation | `session.h` owns the fixed 60 Hz accumulator and held input. Simulation streaming has a two-chunk **tick** budget; renderer uploads have a separate two-chunk **frame** budget. `actors.h` provides stable spawn/force handles used by piercing projectile and beam hit histories. |
| R4: identity | `PlayerId` combines expedition epoch, slot and join generation. `ConnectionId` identifies ownership independently. `ViewBindings` maps two local screens to player identities. `SC_PARTY_CAPACITY` configures bounded core storage; `sc_party_core` and its tests use four. |
| R5: modules | The application has lifecycle, action, input, audio, telemetry, inspection and Quest modules. Rendering has separate mesh, world, actor, text, HUD and scene modules. Shared implementation interfaces are private to `app_internal.h` and `render_internal.h`. |
| R6: persistence | Checkpoint streams, profiles, story, matrix, campaign/politics, factions, loading/validation and final restoration are separate modules. `snapshot.h` provides a separate portable live-state codec. |
| R7: verification | `cmake/CoreSources.cmake` is the canonical core inventory. `sc_configure_core` applies consistent warnings, instrumentation and sanitizer settings to normal, Quest, fault and four-player variants. Coverage discovers required sources from that inventory. |

## Input and simulation contract

A host initializes a `Game`, then a `Session` pointing to it. It submits held
`Actions` with `session_input`, discrete `GameCommand` values with
`session_command`, and elapsed time with `session_advance`. A replay or headless
host can call `session_tick` directly. A command addresses a current `PlayerId`,
not an SDL device number, screen index or connection number.

`session_set_local_views` independently selects zero views for a headless server,
one for a single-view client, or two for split-screen. It does not change the party.
`game_bind_view` can bind a local screen to any active player. The offline default
retains automatic solo/two-player display selection up to the two-view limit.

Commands reject stale/inactive players, unknown kinds, invalid arguments and
non-finite look values. Domain systems retain their resource, location and reward
checks. A successfully dispatched context-sensitive request can still do nothing
when its physical preconditions are absent. Commands are synchronous: frontends
apply them at their frame boundary, before advancing the next tick. A replay must
record their tick and order, not their original display-frame time. The architecture
test replays the same command/input trace at 30, 60 and 120 Hz and compares the
complete portable simulation state.

Held axes allow +/-2 to preserve simultaneous keyboard plus controller input;
physical movement normalizes the combined direction before applying speed. Quest
rays receive additional finite, length and offset checks. The session accepts at
most six catch-up ticks per advance. Pause clears held input and accumulated time;
paused previews can populate geometry without advancing gameplay. GPU upload
speed never determines the authoritative stream budget.

Changing expeditions or restoring state invalidates old local identities. Leaving
and rejoining invalidates that slot's generation; session synchronization clears
its held input and connection assignment. `session_owned` checks the generation
as well as the connection. A future transport must use this ownership check before
submitting a remote command; the offline API is not an authentication service.

Actor handles use a room residency serial and spawn identity, or the persistent
force/member identity for mobile aliens. Array compaction cannot retarget a handle.
Eviction invalidates static spawns; expedition replacement invalidates all old
handles. Beam/projectile hit histories use these handles, including across mobile
force room changes.

## Local presentation

`Game` remains an aggregate for the current executable, but its `views` and
`presentation` storage are distinct from its character and world data. Maps,
loadout selection, dossier selection, device guidance preferences and HUD text do
not travel in live snapshots. Pending choices that affect gameplay do travel.
Device preferences remain in the existing two-view preferences file.

Messages have a bounded 32-event sequence with an optional recipient. Consumers
maintain their own cursor; a slow consumer resumes at the oldest retained event.
The old `Game.message` is retained as a diagnostic last-message value. Player HUDs
use their own addressed text. The renderer resolves the current player through
`ViewBindings` and allocates framebuffers per local screen.

There are still intentional offline policies in the client: opening a journal
pauses the local session, and local interaction helpers open their corresponding
view. Online pause/menu policy is a later gameplay decision, not a hidden change
made by this refactor.

## Checkpoints and live snapshots

Existing `.sav` formats 1–16 retain their two-profile layout, explicit little-endian
fields, migrations, checksums and transactional replacement. Generators 3–15 and
their fingerprints remain supported. The historical matrix map flag is read and
validated but is treated as local UI state. Ordinary checkpoints still resume solo
and reconstruct/reset the same transient combat state as before.

The four-player headless configuration explicitly rejects the two-profile
checkpoint API rather than silently omitting characters. Expanded sessions use the
live snapshot API; a future durable multiplayer save format needs its own migration
and ownership design. The existing application targets continue to use the default
two-profile core.

`game_snapshot_encode` returns an allocated byte buffer that the caller frees.
`game_snapshot_decode` validates and constructs a replacement before changing the
current game. It retains streamed geometry, active enemies and animation timers,
projectiles and piercing history, flares, deaths, rigs, guardian attempts, matrix
networks/guards/shots, faction activity and progression. Index tables are rebuilt.
Local view preferences survive import; ephemeral view state is reset. Prior local
handles are invalidated, and internal projectile handles are rebound to the new
local epoch.

The live wire format has a separate magic/version, schema fingerprint and party
capacity. It uses explicit little-endian integers and IEEE binary32/binary64 values;
no pointers, compiler padding or raw C structures are written. Field order, bounds
and exclusions are declared in `snapshot_schema.c` and
`scripts/snapshot-fields.json`. `generate-snapshot-schema.py --check` fails when
reachable C fields change without a codec review. Regeneration updates the schema
fingerprint, so incompatible layouts are rejected rather than reinterpreted.

The codec bounds the packet to 32 MiB and each dynamic record table to 65,536 entries.
It rejects truncation, extra bytes, checksum/schema/capacity mismatches, non-finite
values, invalid scalar/index bounds, inconsistent references and allocation failure.
The checksum detects corruption; transport authentication belongs to the future
network protocol. A full live snapshot is a restoration/replay contract, **not** a
packet intended to be sent every simulation tick.

## What Internet multiplayer still requires

1. A headless server executable with connection lifecycle, ownership assignment,
   ordered/tick-stamped input queues and a versioned command protocol. Choose native
   and browser-compatible transports; browsers cannot use an arbitrary raw UDP socket.
2. An authoritative replication layer: seed/generator negotiation, interest regions,
   initial baselines, compact state deltas and reliable gameplay events. Geometry
   may be regenerated client-side, with the server owning mutable world state.
3. Client interpolation plus movement prediction/reconciliation. The frame-schedule
   replay tests do not claim bit-identical floating-point simulation across native,
   Emscripten and different CPUs; full cross-platform lockstep is not assumed.
4. Co-op policies for menus, pausing, late joining, disconnects, respawns, shelter,
   matrix dives, objectives and rewards. A bounded four-player core is now tested;
   it is not yet an online party experience.
5. Durable multiplayer saves, reconnect snapshots, protocol authentication, rate and
   resource limits, deployment/hosting and tests under latency, loss, duplication,
   reordering and hostile input. Host migration and matchmaking remain separate choices.

The application/core and ordinary/Quest build boundaries keep those additions from
requiring SDL or OpenGL in the server, or making VR a dependency of other targets.
