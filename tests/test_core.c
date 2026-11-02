#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks;
#define CHECK(test)                                                                                \
    do {                                                                                           \
        checks++;                                                                                  \
        if (!(test)) {                                                                             \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #test);                        \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static void step(Game *g, Actions a, int ticks) {
    Actions all[2];
    memset(all, 0, sizeof(all));
    all[0] = a;
    for (int i = 0; i < ticks; i++)
        game_step(g, all, 1.0 / 60);
}
static void test_generation(void) {
    for (uint32_t seed = 1; seed <= 24; seed++) {
        World a, b;
        world_init(&a, seed);
        world_init(&b, seed);
        Vec3 start = v3(0, .02, 12), far = v3(192, .02, 240);
        CHECK(world_refresh(&a, &start, 1));
        CHECK(world_refresh(&b, &far, 1));
        CHECK(world_refresh(&b, &start, 1));
        CHECK(a.count == b.count);
        CHECK(a.count <= SC_MAX_ROOMS);
        for (int i = 0; i < a.count; i++) {
            Room *r = world_find(&b, a.rooms[i].cell);
            CHECK(r != NULL);
            CHECK(r->hash == a.rooms[i].hash);
            CHECK(r->type == a.rooms[i].type);
            CHECK(r->width == a.rooms[i].width);
            for (int j = 0; j < r->enemy_count; j++) {
                CHECK(!world_overlap(&a, a.rooms[i].enemies[j].pos, .42, 1.25));
                CHECK(vlen(vsub(r->enemies[j].pos, a.rooms[i].enemies[j].pos)) < 1e-9);
            }
        }
        /* Walkable shared portals must match across both sides of every X/Z seam. */
        for (int i = 0; i < a.count; i++) {
            Vec3 c = world_center(a.rooms[i].cell);
            CHECK(!world_overlap(&a, vadd(c, v3(47.8, .02, 0)), .3, 1));
            CHECK(!world_overlap(&a, vadd(c, v3(0, .02, 47.8)), .3, 1));
        }
        world_destroy(&a);
        world_destroy(&b);
    }
    CHECK(world_difficulty((Cell){0, 0, 0}) == 0);
    CHECK(world_difficulty((Cell){0, 1, 0}) > 0);
    CHECK(world_difficulty((Cell){10, 0, 0}) > world_difficulty((Cell){1, 0, 0}));
    puts("PASS deterministic generation, shared portals, distance scaling (24 seeds)");
}
static void test_streaming(void) {
    World w;
    world_init(&w, 51);
    Vec3 origin = v3(0, .02, 150);
    CHECK(world_refresh(&w, &origin, 1));
    Room *room = world_find(&w, (Cell){0, 0, 1});
    CHECK(room && room->enemy_count > 0);
    world_kill(&w, room, 0);
    for (int i = 0; i < 80; i++) {
        Vec3 positions[2] = {v3(i * 96, .02, i * 96 + 48), v3(-i * 96, 64.02, i * 96 + 48)};
        CHECK(world_refresh(&w, positions, 2));
        CHECK(w.count <= 54);
        CHECK(w.box_count < 20000);
    }
    CHECK(world_refresh(&w, &origin, 1));
    room = world_find(&w, (Cell){0, 0, 1});
    CHECK(room && !room->enemies[0].alive);
    world_destroy(&w);
    puts("PASS bounded active world for separated players and persistent kills");
}
static void test_movement(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g != NULL);
    game_init(g, 17);
    CHECK(g->player_count == 1);
    step(g, (Actions){.forward = 1}, 700);
    CHECK(g->players[0].pos.z > 55);
    CHECK(g->players[0].pos.y < .2);
    CHECK(g->players[0].grounded);
    Player *p = &g->players[0];
    p->pos = v3(10, -31.98, 48);
    p->velocity = v3(0, 0, 0);
    p->yaw = -SC_PI * .5;
    step(g, (Actions){.forward = 1}, 80);
    CHECK(p->pos.x > 3.5);
    step(g, (Actions){.forward = 1, .crouch = true}, 58);
    CHECK(p->pos.x < 3.3);
    CHECK(p->crouched);
    step(g, (Actions){0}, 2);
    CHECK(p->crouched);
    CHECK(!world_overlap(&g->world, p->pos, .34, 1));
    game_join(g);
    CHECK(g->players[1].crouched);
    CHECK(!world_overlap(&g->world, g->players[1].pos, .34, 1));
    game_leave(g);
    p->pos = world_lift((Cell){0, 0, 0});
    p->velocity = v3(0, 0, 0);
    step(g, (Actions){.lift_up = true}, 1);
    CHECK(p->pos.y > 31);
    CHECK(p->checkpoint.y > 31);
    p->health = 0;
    step(g, (Actions){0}, 1);
    CHECK(g->respawns == 1);
    CHECK(p->health == player_max_health(p));
    CHECK(vlen(vsub(p->pos, p->checkpoint)) < .1);
    game_destroy(g);
    free(g);
    puts("PASS space approach, gravity, crouch clearance, lift, checkpoint respawn");
}
static void test_combat(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g != NULL);
    game_init(g, 81);
    Player *p = &g->players[0];
    p->pos = v3(0, .02, 20);
    p->yaw = 0;
    p->pitch = -.21;
    p->xp = 70;
    Room *bay = world_find(&g->world, (Cell){0, 0, 0});
    CHECK(bay != NULL);
    bay->enemy_count = 1;
    bay->enemies[0] = (Enemy){.pos = {0, .02, 24}, .health = 10, .kind = 0, .alive = true};
    step(g, (Actions){.fire = true}, 1);
    CHECK(g->shots == 1);
    CHECK(g->hits == 1);
    CHECK(p->kills == 1);
    CHECK(!bay->enemies[0].alive);
    CHECK(p->level == 2 && p->points == 1);
    CHECK(game_upgrade(g, 0, 1));
    CHECK(p->vitality == 1 && p->health == 125);
    CHECK(!game_upgrade(g, 0, 0));
    CHECK(world_raycast(&g->world, v3(0, 2, 20), v3(1, 0, 0), 100) < 42);
    Vec3 position = p->pos;
    game_join(g);
    CHECK(g->player_count == 2);
    game_look(&g->players[1], 1, .5);
    CHECK(g->players[0].yaw == 0);
    CHECK(vlen(vsub(position, p->pos)) < 1e-6);
    game_leave(g);
    CHECK(g->player_count == 1);
    uint32_t level = p->level;
    game_new_ship(g, 999);
    CHECK(g->world.seed == 999);
    CHECK(p->level == level);
    CHECK(p->pos.z < 0);
    game_destroy(g);
    free(g);
    puts("PASS raycast combat, upgrades, optional co-op, new expedition progression");
}
static void test_save(void) {
    const char *path = "sc-test-expedition.sav";
    Game *g = malloc(sizeof(*g));
    CHECK(g != NULL);
    game_init(g, 123456);
    game_join(g);
    g->players[0].level = 7;
    g->players[0].power = 3;
    g->players[1].level = 4;
    Room *r = world_find(&g->world, (Cell){0, 0, 1});
    CHECK(r != NULL);
    world_kill(&g->world, r, 1);
    CHECK(game_save(g, path));
    game_new_ship(g, 999);
    CHECK(game_load(g, path));
    CHECK(g->world.seed == 123456);
    CHECK(g->player_count == 1);
    CHECK(g->players[0].level == 7);
    CHECK(g->players[1].level == 4);
    r = world_find(&g->world, (Cell){0, 0, 1});
    CHECK(r && !r->enemies[1].alive);
    FILE *f = fopen(path, "ab");
    CHECK(f != NULL);
    fputc('!', f);
    fclose(f);
    CHECK(!game_load(g, path));
    CHECK(g->world.seed == 123456);
    f = fopen(path, "wb");
    CHECK(f != NULL);
    fputs("bad", f);
    fclose(f);
    CHECK(!game_load(g, path));
    CHECK(g->players[0].level == 7);
    CHECK(!game_save(g, "/nonexistent/sc-save-dir/file.sav"));
    remove(path);
    game_destroy(g);
    free(g);
    puts("PASS save round-trip, solo default, both profiles, corruption and failure safety");
}
int main(void) {
    test_generation();
    test_streaming();
    test_movement();
    test_combat();
    test_save();
    printf("PASS %d assertions\n", checks);
    return 0;
}
