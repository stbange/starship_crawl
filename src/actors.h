#ifndef SC_ACTORS_H
#define SC_ACTORS_H
#include "world.h"
struct Game;
/* A spawn identity survives array compaction, but not eviction or a new expedition.
   Mobile members instead use their persistent force identity. */
typedef struct {
    uint32_t epoch, force;
    uint64_t residency;
    Cell room;
    uint8_t member;
} ActorId;
ActorId game_actor_id(const struct Game *game, const Room *room, const Enemy *enemy);
bool game_actor_same(ActorId a, ActorId b);
Enemy *game_actor(struct Game *game, ActorId id);
#endif
