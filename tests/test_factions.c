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
static Game *fresh(unsigned seed) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, seed);
    return g;
}
static void dispose(Game *g) {
    game_destroy(g);
    free(g);
}
static void enter(Game *g, Cell cell) {
    g->players[0].pos = position_from(vadd(world_center(cell), v3(0, .02, 2)));
    Vec3 at = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed);
}
static void foundations(void) {
    Game *g = fresh(713);
    CHECK(g->world.generator == SC_GENERATOR_VERSION && g->factions.version == 1);
    CHECK(!g->politics.version && !g->director.version && factions_valid(&g->factions, 14));
    CHECK(!factions_active(g));
    politics_journal(g, 0);
    politics_read(g, 0);
    politics_track(g, 0);
    CHECK(!g->views[0].journal && !g->politics.tracked);
    MapView map;
    CHECK(game_map_build(g, 0, (Cell){3, 1, 4}, 3, &map));
    CHECK(g->factions.tick == 0 && g->factions.count == 0);
    CHECK(!factions_record(g, (Cell){0, 0, -1}));
    for (int i = 63; i >= 0; i--)
        CHECK(factions_record(g, (Cell){i * 4 - 128, i % 3 - 1, 8}));
    CHECK(g->factions.count == 64 && g->factions.capacity >= 64);
    CHECK(factions_at(&g->factions, (Cell){-128, -1, 8}));
    CHECK(!factions_at(&g->factions, (Cell){999, 9, 99}));
    CHECK(factions_record(g, (Cell){-128, -1, 8})->owner == factions_owner(g, (Cell){-128, -1, 8}));
    CHECK(factions_valid(&g->factions, 14));
    enter(g, (Cell){1, 0, 0});
    CHECK(factions_active(g));
    for (int i = 0; i < 121; i++)
        factions_step(g, 1.0 / 60);
    CHECK(g->factions.tick == 1);
    g->players[0].sheltered = true;
    factions_step(g, 2);
    CHECK(g->factions.tick == 1 && !factions_active(g));
    game_join(g);
    g->players[1].sheltered = false;
    CHECK(factions_active(g));
    g->matrix.divers[1].active = true;
    CHECK(factions_active(g));
    g->matrix.divers[1].active = false;
    g->players[1].sheltered = true;
    CHECK(!factions_active(g));
    g->players[0].sheltered = false;
    CHECK(factions_observed(g, player_eye(&g->players[0]), 12));
    CHECK(!factions_observed(g, v3(90000, 90000, 90000), 12));
    Enemy e = {.country = 1, .pos = g->players[0].pos};
    CHECK(!factions_hostile(g, &e, 0));
    g->factions.incidents[0][0].local_time = 10;
    g->factions.incidents[0][0].strikes = 2;
    g->factions.incidents[0][0].site = world_cell(position_absolute(e.pos));
    CHECK(factions_hostile(g, &e, 0) && !factions_hostile(g, &e, 1));
    CHECK(factions_relation(g, 1, 2) == REL_WAR && factions_relation(g, 1, 1) == REL_PEACE);
    CHECK(factions_safe(g, (Cell){0}) && factions_safe(g, campaign_site(0)));
    g->factions.incidents[0][0] = (FactionIncident){0};
    g->players[1].sheltered = false;
    g->player_count = 1;
    CHECK(game_save(g, "factions-foundation.sav"));
    unsigned tick = g->factions.tick;
    size_t count = g->factions.count;
    CHECK(game_load(g, "factions-foundation.sav"));
    CHECK(g->factions.tick == tick && g->factions.count == count &&
          factions_valid(&g->factions, 14));
    CHECK(!g->politics.version && !g->director.kind);
    remove("factions-foundation.sav");
    dispose(g);
    puts("PASS hidden state, deterministic strategic records, clocks and portable saves");
}
static void geography(void) {
    unsigned kinds = 0, owners = 0;
    for (unsigned seed = 1; seed <= 12; seed++) {
        Game *g = fresh(seed);
        for (int y = -3; y <= 3; y++)
            for (int x = -3; x <= 3; x++)
                for (int z = 0; z < 3; z++) {
                    RegionPlan a, b, legacy;
                    Cell c = {x, y, z};
                    world_plan_version(seed, 14, c, &a);
                    world_plan_version(seed, 14, c, &b);
                    world_plan_version(seed, 12, c, &legacy);
                    CHECK(!memcmp(&a, &b, sizeof(a)));
                    for (int i = 0; i < a.count; i++) {
                        kinds |= 1u << a.rooms[i].habitat.court;
                        owners |= 1u << a.rooms[i].habitat.country;
                    }
                    for (int i = 0; i < legacy.count; i++)
                        CHECK(!legacy.rooms[i].habitat.country && !legacy.rooms[i].habitat.court);
                }
        CHECK(g->factions.count == 0);
        dispose(g);
    }
    printf("site kinds=%u owners=%u\n", kinds, owners);
    CHECK((kinds & 28) == 28 && (owners & 14) == 14);
    puts("PASS seeded resource sites and legacy generation separation");
}
int main(void) {
    foundations();
    geography();
    return 0;
}
