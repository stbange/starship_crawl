#ifndef SC_FACTION_FIXTURE_H
#define SC_FACTION_FIXTURE_H
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static Game *fixture(unsigned seed) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, seed);
    g->world.open_layer = 0;
    /* Unit fixture only: a consistent unbound campaign for isolated deep-world tests. */
    for (unsigned i = 0; i < 8; i += 2) {
        Cell cell = campaign_site(i);
        CHECK(world_set_systems(&g->world, cell, 0));
        for (size_t r = 0; r < g->world.record_count; r++)
            if (cell_equal(g->world.records[r].cell, cell))
                g->world.records[r].killed |= 1;
        g->campaign.defeated |= 1u << i;
        g->campaign.eligible[i] = 1;
    }
    for (unsigned c = 0; c < 3; c++)
        for (unsigned r = 0; r < 3; r++)
            g->factions.nations[c].supplies[r] = 200;
    return g;
}
static void finish(Game *g) {
    game_destroy(g);
    free(g);
}
static Cell ordinary(Game *g, Cell start) {
    for (int z = 0; z < 12; z++)
        for (int x = 0; x < 12; x++) {
            Cell c = {start.x + x, start.y, start.z + z};
            RoomSpec r = world_describe_version(g->world.seed, g->world.generator, c);
            if (cell_equal(c, r.cell) && !factions_safe(g, c))
                return c;
        }
    CHECK(false);
    return start;
}
static void load_area(Game *g, Cell cell) {
    factions_forces_sync(g, true);
    g->players[0].pos = position_from(vadd(world_center(cell), v3(0, .02, -30)));
    g->players[0].yaw = SC_PI;
    Vec3 at[2] = {position_absolute(g->players[0].pos), position_absolute(g->players[1].pos)};
    CHECK(world_refresh(&g->world, at, g->player_count));
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed);
}
static void empty_actors(Game *g) {
    for (int i = 0; i < g->world.count; i++)
        g->world.rooms[i].enemy_count = 0;
}
static void force_frame(Game *g, double dt) {
    factions_forces_sync(g, true);
    factions_forces_step(g, dt);
    factions_tactics(g, dt);
    factions_forces_sync(g, false);
}
static unsigned actors(const Game *g) {
    unsigned n = 0;
    for (int r = 0; r < g->world.count; r++)
        for (int e = 0; e < g->world.rooms[r].enemy_count; e++)
            n += g->world.rooms[r].enemies[e].alive && g->world.rooms[r].enemies[e].force != 0;
    return n;
}
#endif
