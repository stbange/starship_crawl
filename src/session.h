#ifndef SC_SESSION_H
#define SC_SESSION_H
#include "game.h"
#define SC_TICK_SECONDS (1.0 / 60.0)
#define SC_SIM_STREAM_BUDGET 2u
#define SC_MAX_CATCHUP_TICKS 6u
/* The host supplies elapsed time and held inputs; no SDL, GL, or wall clock is used. */
typedef struct {
    Game *game;
    uint64_t tick;
    double accumulator;
    uint32_t epoch;
    Actions inputs[SC_PLAYERS];
    PlayerId roster[SC_PLAYERS];
    ConnectionId owners[SC_PLAYERS];
    ViewBindings views;
    unsigned local_view_limit;
} Session;
void session_init(Session *session, Game *game);
void session_sync(Session *session);
/* Zero for a server, one for a single local view, two for optional split-screen. */
bool session_set_local_views(Session *session, unsigned count);
/* Axes are bounded to +/-2 to preserve simultaneous keyboard + pad input.
   Movement normalizes the combined world-space wish before applying speed. */
bool session_input(Session *session, PlayerId player, const Actions *input);
bool session_command(Session *session, const GameCommand *command);
void session_tick(Session *session);
unsigned session_advance(Session *session, double elapsed, bool paused);
/* Connection zero denotes a local/offline participant. Ownership is explicit. */
bool session_assign(Session *session, PlayerId player, ConnectionId connection);
bool session_owned(const Session *session, PlayerId player, ConnectionId connection);
#endif
