#include "game.h"
PlayerId game_player_id(const Game *g, int slot) {
    return slot >= 0 && slot < g->player_count
               ? (PlayerId){g->epoch, (uint32_t)slot, g->generations[slot]}
               : (PlayerId){0};
}
int game_player_slot(const Game *g, PlayerId id) {
    return id.epoch && id.epoch == g->epoch && id.slot < (unsigned)g->player_count &&
                   id.generation == g->generations[id.slot]
               ? (int)id.slot
               : -1;
}
bool game_bind_view(const Game *g, ViewBindings *views, unsigned view, PlayerId player) {
    if (view >= SC_LOCAL_VIEWS || game_player_slot(g, player) < 0)
        return false;
    views->players[view] = player;
    if (views->count <= view)
        views->count = view + 1;
    return true;
}
int game_view_slot(const Game *g, const ViewBindings *views, unsigned view) {
    return view < views->count && view < SC_LOCAL_VIEWS ? game_player_slot(g, views->players[view])
                                                        : -1;
}
void game_identity_reset(Game *g) {
    if (++g->epoch == 0)
        g->epoch = 1;
}

bool game_party_resize(Game *g, unsigned count) {
    if (!count || count > SC_PLAYERS)
        return false;
    unsigned previous = (unsigned)g->player_count;
    for (unsigned i = count; i < previous; i++) {
        game_lift_cancel(g, (int)i);
        matrix_disconnect(g, (int)i, false);
        g->players[i].riding = 0;
        g->players[i].sheltered = false;
        g->generations[i]++;
    }
    for (unsigned i = previous; i < count; i++) {
        Player *p = &g->players[i];
        if (!g->matrix.divers[i].active) {
            p->pos = g->players[0].lift_active ? g->players[0].lift_from : g->players[0].pos;
            p->checkpoint = g->players[0].checkpoint;
            p->velocity = v3(0, 0, 0);
            p->lift_active = false;
            p->riding = 0;
            p->health = player_max_health(p);
        }
    }
    g->player_count = (int)count;
    return true;
}
