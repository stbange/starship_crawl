#include "game.h"
#include "actors.h"
ActorId game_actor_id(const Game *g, const Room *r, const Enemy *e) {
    return (ActorId){g->epoch, e->force, r->residency, r->cell, e->force ? e->member : e->id};
}
bool game_actor_same(ActorId a, ActorId b) {
    return a.epoch && a.epoch == b.epoch && a.force == b.force && a.member == b.member &&
           (a.force || (a.residency == b.residency && cell_equal(a.room, b.room)));
}
Enemy *game_actor(Game *g, ActorId id) {
    if (!id.epoch || id.epoch != g->epoch)
        return NULL;
    for (int r = 0; r < g->world.count; r++) {
        Room *room = &g->world.rooms[r];
        if (!id.force && (room->residency != id.residency || !cell_equal(room->cell, id.room)))
            continue;
        for (int i = 0; i < room->enemy_count; i++) {
            Enemy *e = &room->enemies[i];
            if (e->alive && e->force == id.force && (id.force ? e->member : e->id) == id.member)
                return e;
        }
    }
    return NULL;
}
