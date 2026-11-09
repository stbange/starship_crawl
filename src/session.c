#include "session.h"
#include <string.h>
void session_sync(Session *s) {
    if (s->epoch != s->game->epoch) {
        s->epoch = s->game->epoch;
        s->tick = 0;
        s->accumulator = 0;
        memset(s->inputs, 0, sizeof(s->inputs));
        memset(s->owners, 0, sizeof(s->owners));
        memset(&s->views, 0, sizeof(s->views));
    }
    for (int i = 0; i < SC_PLAYERS; i++) {
        PlayerId id = game_player_id(s->game, i), prior = s->roster[i];
        if (id.epoch != prior.epoch || id.generation != prior.generation) {
            s->roster[i] = id;
            s->inputs[i] = (Actions){0};
            s->owners[i] = (ConnectionId){0};
        }
    }
    unsigned count = (unsigned)s->game->player_count;
    if (count > s->local_view_limit)
        count = s->local_view_limit;
    for (unsigned i = 0; i < count; i++)
        if (game_view_slot(s->game, &s->views, i) < 0)
            game_bind_view(s->game, &s->views, i, game_player_id(s->game, (int)i));
    if (s->views.count > count)
        s->views.count = count;
}
void session_init(Session *s, Game *g) {
    memset(s, 0, sizeof(*s));
    s->game = g;
    s->local_view_limit = SC_LOCAL_VIEWS;
    session_sync(s);
}
bool session_set_local_views(Session *s, unsigned count) {
    if (count > SC_LOCAL_VIEWS)
        return false;
    s->local_view_limit = count;
    session_sync(s);
    return true;
}
bool session_input(Session *s, PlayerId player, const Actions *input) {
    session_sync(s);
    int slot = game_player_slot(s->game, player);
    if (slot < 0 || !isfinite(input->forward) || !isfinite(input->strafe) ||
        !isfinite(input->vertical) || fabs(input->forward) > 2 || fabs(input->strafe) > 2 ||
        fabs(input->vertical) > 2)
        return false;
#ifdef SC_WEBXR
    if (input->aimed && (!isfinite(input->aim_offset.x) || !isfinite(input->aim_offset.y) ||
                         !isfinite(input->aim_offset.z) || !isfinite(input->aim_direction.x) ||
                         !isfinite(input->aim_direction.y) || !isfinite(input->aim_direction.z) ||
                         vlen(input->aim_offset) > 3 || fabs(vlen(input->aim_direction) - 1) > .01))
        return false;
#endif
    s->inputs[slot] = *input;
    return true;
}
bool session_command(Session *s, const GameCommand *command) {
    session_sync(s);
    return game_command(s->game, command);
}
static void stream(Session *s) {
    Vec3 positions[SC_PLAYERS];
    for (int i = 0; i < s->game->player_count; i++)
        positions[i] = position_absolute(s->game->players[i].pos);
    world_refresh(&s->game->world, positions, s->game->player_count);
    world_stream(&s->game->world, SC_SIM_STREAM_BUDGET);
}
void session_tick(Session *s) {
    session_sync(s);
    stream(s);
    game_step(s->game, s->inputs, SC_TICK_SECONDS);
    s->tick++;
}
unsigned session_advance(Session *s, double elapsed, bool paused) {
    session_sync(s);
    if (paused) {
        s->accumulator = 0;
        memset(s->inputs, 0, sizeof(s->inputs));
        stream(s); /* Paused preview may populate geometry, never advance gameplay. */
        return 0;
    }
    if (!isfinite(elapsed) || elapsed < 0)
        return 0;
    s->accumulator += fmin(elapsed, SC_TICK_SECONDS * SC_MAX_CATCHUP_TICKS);
    unsigned ticks = 0;
    while (s->accumulator + 1e-12 >= SC_TICK_SECONDS && ticks < SC_MAX_CATCHUP_TICKS) {
        session_tick(s);
        s->accumulator = fmax(0, s->accumulator - SC_TICK_SECONDS);
        ticks++;
    }
    return ticks;
}
bool session_assign(Session *s, PlayerId player, ConnectionId connection) {
    session_sync(s);
    int slot = game_player_slot(s->game, player);
    if (slot < 0)
        return false;
    s->owners[slot] = connection;
    return true;
}
bool session_owned(const Session *s, PlayerId player, ConnectionId connection) {
    int slot = game_player_slot(s->game, player);
    return s->epoch == s->game->epoch && slot >= 0 && s->roster[slot].epoch == player.epoch &&
           s->roster[slot].generation == player.generation &&
           s->owners[slot].value == connection.value;
}
