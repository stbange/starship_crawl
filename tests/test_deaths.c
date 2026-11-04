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
static Game *fresh(void) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    g->players[0].pos = position_from(v3(0, .02, 12));
    Vec3 p = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &p, 1));
    while (g->world.pending)
        world_stream(&g->world, 2);
    for (int i = 0; i < g->world.count; i++)
        g->world.rooms[i].enemy_count = 0;
    g->players[0].shield = g->players[0].armor = 0;
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static Enemy *put(Game *g, int id, int kind, Vec3 at) {
    Room *room = world_find(&g->world, (Cell){0});
    if (room->enemy_count <= id)
        room->enemy_count = id + 1;
    room->enemies[id] = (Enemy){.pos = position_from(at),
                                .kind = (uint8_t)kind,
                                .id = (uint8_t)id,
                                .health = 10,
                                .alive = true,
                                .heading = {0, 0, 1},
                                .normal = {0, 1, 0}};
    return &room->enemies[id];
}
static void defeat(Game *g, int id) {
    game_damage_enemy(g, world_find(&g->world, (Cell){0}), id, 0, 10000, 0);
}
static void advance(Game *g, int frames) {
    for (int i = 0; i < frames; i++)
        game_deaths_step(g, .05);
}
static int visuals(Game *g) {
    int count = 0;
    for (int i = 0; i < SC_MAX_DEATHS; i++)
        count += g->deaths[i].active;
    return count;
}
static int hazards(Game *g) {
    int count = 0;
    for (int i = 0; i < SC_MAX_DEATH_HAZARDS; i++)
        count += g->death_hazards[i].active;
    return count;
}
static unsigned fingerprint(const Room *r) {
    unsigned h = 2166136261u;
    for (int i = 0; i < r->enemy_count; i++) {
        const Enemy *e = &r->enemies[i];
        Vec3 v = position_absolute(e->pos);
        int values[] = {e->kind, (int)lround(v.x * 1000), (int)lround(v.y * 1000),
                        (int)lround(v.z * 1000), (int)e->health};
        for (int j = 0; j < 5; j++)
            for (int b = 0; b < 4; b++) {
                h ^= ((unsigned)values[j] >> (b * 8)) & 255;
                h *= 16777619u;
            }
    }
    return h;
}
static void test_compatibility(void) {
    FILE *f = fopen(SC_FIXTURE_DIR "/generator-v5-enemies.txt", "r");
    CHECK(f);
    unsigned seed, hash;
    int x, y, z, count, rows = 0;
    while (fscanf(f, "%u%d%d%d%d%u", &seed, &x, &y, &z, &count, &hash) == 6) {
        World *w = calloc(1, sizeof(*w));
        CHECK(w);
        world_init(w, seed);
        w->generator = 5;
        Cell c = {x, y, z};
        Vec3 at = world_center(c);
        CHECK(world_refresh(w, &at, 1));
        while (w->pending)
            world_stream(w, 2);
        Room *r = world_find(w, c);
        CHECK(r && r->enemy_count == count && fingerprint(r) == hash);
        CHECK(world_fingerprint_version(seed, 5, world_region(c)) ==
              world_fingerprint_version(seed, 6, world_region(c)));
        world_destroy(w);
        free(w);
        rows++;
    }
    fclose(f);
    CHECK(rows == 16);
    unsigned machines = 0;
    for (unsigned s = 1; s <= 8; s++) {
        World *w = calloc(1, sizeof(*w));
        CHECK(w);
        world_init(w, s);
        Vec3 at = world_center((Cell){4, 1, 8});
        world_refresh(w, &at, 1);
        while (w->pending)
            world_stream(w, 2);
        for (int r = 0; r < w->count; r++)
            for (int e = 0; e < w->rooms[r].enemy_count; e++) {
                Enemy *enemy = &w->rooms[r].enemies[e];
                if (enemy->kind >= 27)
                    machines |= 1u << (enemy->kind - 27);
                CHECK(enemy->kind < SC_ENEMY_TYPES);
            }
        world_destroy(w);
        free(w);
    }
    CHECK(machines == 31);
}
static void test_each_death(void) {
    for (int kind = 0; kind < SC_ENEMY_TYPES; kind++) {
        Game *g = fresh();
        Enemy *e = put(g, 0, kind, v3(0, .02, 22));
        defeat(g, 0);
        CHECK(!e->alive && g->players[0].kills == 1 && visuals(g) == 1);
        CHECK(g->deaths[0].body.kind == kind && !g->deaths[0].body.alive);
        CHECK(hazards(g) == (sc_death_effects[kind] != DEATH_NONE));
        defeat(g, 0);
        CHECK(g->players[0].kills == 1 && visuals(g) == 1);
        game_deaths_step(g, NAN);
        game_deaths_step(g, 0);
        game_deaths_step(g, .1);
        CHECK(g->deaths[0].age == 0);
        advance(g, 4);
        CHECK(g->deaths[0].age > .19 && g->deaths[0].age < .21);
        advance(g, 60);
        CHECK(!visuals(g) && g->players[0].health == 100);
        advance(g, 200);
        CHECK(!hazards(g));
        Room *r = world_find(&g->world, (Cell){0});
        game_damage_enemy(g, NULL, 0, 0, 1, 0);
        game_damage_enemy(g, r, -1, 0, 1, 0);
        game_damage_enemy(g, r, 1, 0, 1, 0);
        game_damage_enemy(g, r, 0, -1, 1, 0);
        game_damage_enemy(g, r, 0, 2, 1, 0);
        game_damage_enemy(g, r, 0, 0, NAN, 0);
        game_damage_enemy(g, r, 0, 0, 0, 0);
        CHECK(g->players[0].kills == 1);
        destroy(g);
    }
}
static void test_blast_and_chain(void) {
    Game *g = fresh();
    g->players[0].pos = position_from(v3(0, .02, 17));
    put(g, 0, 13, v3(0, .02, 18));
    put(g, 1, 31, v3(1, .02, 18));
    defeat(g, 0);
    advance(g, 17);
    CHECK(g->players[0].health == 100 && g->players[0].kills == 1);
    advance(g, 4);
    CHECK(g->players[0].health < 100 && g->players[0].kills == 2);
    CHECK(g->death_hazards[0].spent && hazards(g) == 2 && !g->death_hazards[1].spent);
    g->players[0].pos = position_from(v3(0, .02, 30));
    float health = g->players[0].health;
    advance(g, 30);
    CHECK(g->players[0].health == health && g->players[0].kills == 2 && !hazards(g));
    destroy(g);
    g = fresh();
    Room *r = world_find(&g->world, (Cell){0});
    g->players[0].pos = position_from(v3(r->width * .5 - 1.8, .02, 15));
    put(g, 0, 13, v3(r->width * .5 + 1.2, .02, 15));
    defeat(g, 0);
    Vec3 delta = vsub(player_eye(&g->players[0]), position_absolute(g->death_hazards[0].pos));
    CHECK(vlen(delta) < g->death_hazards[0].radius);
    CHECK(world_raycast(&g->world, position_absolute(g->death_hazards[0].pos), vnorm(delta),
                        vlen(delta)) < vlen(delta) - .1);
    advance(g, 24);
    CHECK(g->players[0].health == 100);
    destroy(g);
}
static void test_acid_and_mines(void) {
    Game *g = fresh();
    g->players[0].pos = position_from(v3(0, .02, 20));
    put(g, 0, 1, v3(0, .02, 20));
    defeat(g, 0);
    CHECK(fabs(position_absolute(g->death_hazards[0].pos).y - .08) < .02);
    advance(g, 8);
    CHECK(g->players[0].health == 100);
    advance(g, 4);
    CHECK(g->players[0].health == 95);
    advance(g, 11);
    CHECK(g->players[0].health == 90);
    g->players[0].pos = position_from(v3(0, SC_DECK, 20));
    advance(g, 160);
    CHECK(g->players[0].health == 90 && !hazards(g));
    destroy(g);
    g = fresh();
    put(g, 0, 29, v3(0, .02, 17));
    defeat(g, 0);
    advance(g, 30);
    CHECK(!g->death_hazards[0].triggered && g->players[0].health == 100);
    g->players[0].pos = position_from(v3(0, .02, 17));
    advance(g, 1);
    CHECK(g->death_hazards[0].triggered && !g->death_hazards[0].spent);
    advance(g, 8);
    CHECK(g->players[0].health == 100);
    g->players[0].pos = position_from(v3(0, .02, 27));
    advance(g, 8);
    CHECK(g->death_hazards[0].spent && g->players[0].health == 100);
    advance(g, 10);
    CHECK(!hazards(g));
    destroy(g);
    g = fresh();
    put(g, 0, 16, v3(0, .02, 13));
    defeat(g, 0);
    advance(g, 14);
    CHECK(!g->death_hazards[0].triggered);
    advance(g, 24);
    CHECK(g->players[0].health < 100);
    destroy(g);
    g = fresh();
    put(g, 0, 29, v3(0, .02, 24));
    defeat(g, 0);
    advance(g, 200);
    CHECK(!hazards(g) && g->players[0].health == 100);
    destroy(g);
}
static void test_isolation_and_capacity(void) {
    Game *g = fresh();
    game_join(g);
    g->players[0].sheltered = true;
    g->players[1].shield = g->players[1].armor = 0;
    put(g, 0, 13, v3(0, .02, 13));
    defeat(g, 0);
    advance(g, 22);
    CHECK(g->players[0].health == 100 && g->players[1].health < 100);
    game_deaths_reset(g);
    g->players[0].sheltered = false;
    g->players[0].pos = position_from(game_terminal((Cell){0}));
    g->players[1].pos = g->players[0].pos;
    CHECK(world_set_systems(&g->world, (Cell){0}, SYSTEM_POWER | SYSTEM_BEACON));
    CHECK(game_story_safe(g, position_absolute(g->players[0].pos)));
    put(g, 0, 1, game_terminal((Cell){0}));
    defeat(g, 0);
    put(g, 1, 29, game_terminal((Cell){0}));
    defeat(g, 1);
    advance(g, 30);
    CHECK(g->players[0].health == 100 && !g->death_hazards[1].triggered);
    destroy(g);
    g = fresh();
    Enemy *e = put(g, 0, 1, v3(0, .02, 24));
    for (int i = 0; i < SC_MAX_DEATHS * 2; i++)
        game_enemy_death(g, e, 0);
    CHECK(visuals(g) == SC_MAX_DEATHS && hazards(g) == SC_MAX_DEATH_HAZARDS);
    float life = g->death_hazards[0].life;
    for (int i = 0; i < SC_MAX_DEATHS; i++)
        g->deaths[i].age = 0;
    g->deaths[10].age = 1;
    e->kind = 27;
    game_enemy_death(g, e, 0);
    CHECK(g->deaths[10].body.kind == 27 && g->death_hazards[0].life == life);
    e->kind = 13;
    game_enemy_death(g, e, 0);
    CHECK(g->death_hazards[0].kind == DEATH_ACID && hazards(g) == SC_MAX_DEATH_HAZARDS);
    game_deaths_reset(g);
    CHECK(!hazards(g) && !visuals(g));
    destroy(g);
}
static void test_stream_and_save(void) {
    Game *g = fresh();
    Enemy *e = put(g, 0, 0, v3(0, .02, 20));
    defeat(g, 0);
    Position old = e->pos;
    Vec3 far = world_center((Cell){100, 3, 100});
    CHECK(world_refresh(&g->world, &far, 1));
    while (g->world.pending)
        world_stream(&g->world, 2);
    advance(g, 4);
    CHECK(visuals(g) == 1 && vlen(position_delta(g->deaths[0].body.pos, old)) == 0);
    CHECK(game_save(g, "sc-deaths.sav"));
    Enemy explosive = g->deaths[0].body;
    explosive.kind = 13;
    game_enemy_death(g, &explosive, 0);
    CHECK(!game_load(g, "sc-missing-death-save.sav") && visuals(g) == 2 && hazards(g) == 1);
    CHECK(game_load(g, "sc-deaths.sav") && !visuals(g) && !hazards(g));
    CHECK(g->world.generator == 6 && g->world.record_count && (g->world.records[0].killed & 1));
    Vec3 at = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending)
        world_stream(&g->world, 2);
    put(g, 0, 13, v3(0, .02, 20));
    defeat(g, 0);
    CHECK(hazards(g) == 1);
    game_new_ship(g, 41);
    CHECK(!hazards(g) && !visuals(g));
    remove("sc-deaths.sav");
    destroy(g);
}
static void test_departed_owner(void) {
    Game *g = fresh();
    game_join(g);
    game_leave(g);
    Enemy *e = put(g, 0, 0, v3(0, .02, 20));
    e->burn = 3;
    e->health = 1;
    e->last_player = 1;
    Actions actions[SC_PLAYERS] = {0};
    for (int i = 0; i < 20; i++)
        game_step(g, actions, 1.0 / 60);
    CHECK(!e->alive && g->players[1].kills == 1 && g->players[0].kills == 0);
    CHECK(visuals(g) == 1);
    destroy(g);
}
int main(void) {
    test_compatibility();
    test_each_death();
    test_blast_and_chain();
    test_acid_and_mines();
    test_isolation_and_capacity();
    test_stream_and_save();
    test_departed_owner();
    puts("PASS deaths: 32 species, five machines, legacy encounters, warning/dodge/cover, "
         "acid/mines, chains, isolation, bounded pools, streaming and saves");
    return 0;
}
