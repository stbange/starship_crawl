#ifndef SC_INTERIOR_H
#define SC_INTERIOR_H
#include "world.h"

/* Shared bounded geometry writer; all coordinates supplied here are absolute. */
typedef struct {
    World *world;
    Chunk *chunk;
    Vec3 origin;
} Builder;
void world_box(Builder *builder, Vec3 center, Vec3 size, Color color, float glow, bool solid);
void traversal_build(Builder *builder);
void interior_build(Builder *builder, const Room *room);
void interior_conduit(Builder *builder, const Room *room);
Vec3 interior_cache(const Room *room, int pickup);
#endif
