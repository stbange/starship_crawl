#include "game.h"
#include "habitat.h"
#include "legacy_game.h"
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
static void settle(World *w) {
    int frames = 0;
    while (w->pending && frames++ < SC_MAX_CHUNKS) {
        unsigned done = world_stream(w, SC_STREAM_BUDGET);
        CHECK(done <= SC_STREAM_BUDGET);
        CHECK(done > 0);
        CHECK(!w->failed);
    }
    CHECK(w->pending == 0);
}
static void settle_game(Game *g) {
    Vec3 positions[] = {position_absolute(g->players[0].pos), position_absolute(g->players[1].pos)};
    CHECK(world_refresh(&g->world, positions, g->player_count));
    settle(&g->world);
}
static void step(Game *g, Actions a, int ticks) {
    Actions all[2] = {a, {0}};
    for (int i = 0; i < ticks; i++) {
        Vec3 positions[] = {position_absolute(g->players[0].pos),
                            position_absolute(g->players[1].pos)};
        CHECK(world_refresh(&g->world, positions, g->player_count));
        world_stream(&g->world, SC_STREAM_BUDGET);
        game_step(g, all, 1.0 / 60);
    }
}
static void test_topology(void) {
    unsigned districts = 0;
    int loops = 0, closed = 0;
    for (uint32_t seed = 1; seed <= 128; seed++)
        for (int region = 0; region < 3; region++) {
            Cell key = region == 0   ? (Cell){0, 0, 0}
                       : region == 1 ? (Cell){-3, -2, 1}
                                     : (Cell){2, 3, 2};
            RegionPlan p;
            world_plan(seed, key, &p);
            districts |= 1u << p.district;
            int footprint[16] = {0}, large = 0, edges = 0;
            for (int i = 0; i < p.count; i++) {
                RoomSpec *r = &p.rooms[i];
                CHECK(r->district == p.district ||
                      (r->history_role == HISTORY_ORIGIN &&
                       r->district ==
                           sc_themes[world_history_theme(world_history(seed).cause)].sector));
                if (r->span_x > 1 || r->span_z > 1) {
                    large++;
                    CHECK((r->width >= 180 || r->depth >= 180) && r->height >= 58);
                }
                for (int z = 0; z < r->span_z; z++)
                    for (int x = 0; x < r->span_x; x++) {
                        if (!habitat_owns(r->cell, r->span_x, r->span_z, r->habitat.mask,
                                          (Cell){r->cell.x + x, r->cell.y, r->cell.z + z}))
                            continue;
                        int index = (r->cell.z - p.origin.z + z) * 4 + r->cell.x - p.origin.x + x;
                        CHECK(index >= 0 && index < 16);
                        CHECK(p.owner[index] == i);
                        footprint[index]++;
                    }
            }
            CHECK(large == 1);
            for (int i = 0; i < 16; i++) {
                CHECK(footprint[i] == 1);
                Cell c = {p.origin.x + i % 4, p.origin.y, p.origin.z + i / 4};
                RoomSpec r = world_describe(seed, c);
                CHECK(cell_equal(r.cell, p.rooms[p.owner[i]].cell));
                for (int d = 0; d < 4; d++) {
                    Cell n = world_neighbor(c, (Direction)d);
                    bool open = (p.doors[i] & (1 << d)) != 0;
                    CHECK(open == world_door(seed, c, (Direction)d));
                    if (n.z >= 0) {
                        CHECK(open == world_door(seed, n, (Direction)(d ^ 1)));
                        Connector a = world_connector(seed, c, (Direction)d),
                                  b = world_connector(seed, n, (Direction)(d ^ 1));
                        CHECK(a.open == b.open && a.width == b.width &&
                              a.clearance == b.clearance && a.crouch == b.crouch);
                        if (open) {
                            CHECK(vlen(vsub(vadd(world_center(c), a.local_center),
                                            vadd(world_center(n), b.local_center))) == 0);
                            CHECK(a.clearance >= 1 && a.width >= .8);
                        }
                    } else
                        CHECK(!open);
                    if (d == EAST && i % 4 < 3) {
                        if (open)
                            edges++;
                        else
                            closed++;
                    }
                    if (d == NORTH && i / 4 < 3) {
                        if (open)
                            edges++;
                        else
                            closed++;
                    }
                }
            }
            if (edges > 15)
                loops++;
            /* Every cell and every room is reachable inside its region. Together
               with paired face portals and lifts this proves global reachability. */
            bool visited[16] = {true};
            int queue[16] = {0}, head = 0, tail = 1;
            while (head < tail) {
                int i = queue[head++];
                for (int d = 0; d < 4; d++) {
                    int x = i % 4 + (d == WEST   ? -1
                                     : d == EAST ? 1
                                                 : 0),
                        z = i / 4 + (d == SOUTH   ? -1
                                     : d == NORTH ? 1
                                                  : 0);
                    if (x < 0 || x >= 4 || z < 0 || z >= 4 || !(p.doors[i] & (1 << d)))
                        continue;
                    int j = z * 4 + x;
                    if (!visited[j]) {
                        visited[j] = true;
                        queue[tail++] = j;
                    }
                }
            }
            CHECK(tail == 16);
            CHECK(world_fingerprint(seed, key) != world_fingerprint(seed + 1, key));
        }
    CHECK(districts == ((1u << DISTRICT_TYPES) - 1) && loops > 100 && closed > 100);
    CHECK(world_difficulty((Cell){0, 0, 0}) == 0);
    CHECK(world_difficulty((Cell){0, 1, 0}) > 0);
    CHECK(world_difficulty((Cell){10, 0, 0}) > world_difficulty((Cell){1, 0, 0}));
    puts("PASS 384 region layouts: footprint reservation, district variety, connected backbone, "
         "loops, reciprocal seams");
}
static void test_generation(void) {
    for (uint32_t seed = 1; seed <= 12; seed++) {
        World *a = malloc(sizeof(*a)), *b = malloc(sizeof(*b));
        CHECK(a && b);
        world_init(a, seed);
        world_init(b, seed);
        Vec3 start = world_center((Cell){0, 0, 1}), far = world_center((Cell){8, 2, 8});
        start.y += .02;
        CHECK(world_refresh(a, &start, 1));
        settle(a);
        CHECK(world_refresh(b, &far, 1));
        settle(b);
        CHECK(world_refresh(b, &start, 1));
        settle(b);
        for (int i = 0; i < a->count; i++) {
            Room *r = &a->rooms[i], *other = world_find(b, r->cell);
            CHECK(other && other->ready && r->ready);
            CHECK(other->hash == r->hash && other->type == r->type && other->width == r->width);
            for (int j = 0; j < r->enemy_count; j++) {
                CHECK(!world_overlap(a, position_absolute(r->enemies[j].pos), .42, 1.25));
                CHECK(vlen(position_delta(r->enemies[j].pos, other->enemies[j].pos)) < 1e-9);
            }
        }
        for (int i = 0; i < SC_MAX_CHUNKS; i++) {
            const Chunk *c = &a->chunks[i];
            if (!c->active)
                continue;
            const Chunk *other = world_chunk(b, c->cell);
            CHECK(other && other->box_count == c->box_count);
            for (size_t j = 0; j < c->box_count; j++) {
                CHECK(vlen(vsub(c->boxes[j].min, other->boxes[j].min)) == 0);
                CHECK(vlen(vsub(c->boxes[j].max, other->boxes[j].max)) == 0);
            }
            Vec3 center = world_center(c->cell);
            CHECK(!world_overlap(a, world_lift(c->cell), .34, 1));
            for (int d = 0; d < 4; d++) {
                if (!world_door(seed, c->cell, (Direction)d))
                    continue;
                /* Real collision and floor checks across the entire connector,
                   including the shared seam, not just abstract graph edges. */
                bool neighbor = world_ready(a, world_neighbor(c->cell, (Direction)d));
                for (int k = 0; k <= 24; k++) {
                    double at = k * 2.0;
                    Vec3 p = vadd(center, v3(d == WEST   ? -at
                                             : d == EAST ? at
                                                         : 0,
                                             .02,
                                             d == SOUTH   ? -at
                                             : d == NORTH ? at
                                                          : 0));
                    if (k == 24 && !neighbor)
                        continue;
                    CHECK(!world_overlap(a, p, .3, 1));
                    CHECK(world_raycast(a, p, v3(0, -1, 0), 2) < .03);
                }
            }
        }
        CHECK(a->record_count == 0); /* Immutable exploration creates no save records. */
        world_destroy(a);
        world_destroy(b);
        free(a);
        free(b);
    }
    puts("PASS geometry and population independent of load order; walkable connectors and floors "
         "(12 seeds)");
}
static void test_streaming(void) {
    World *w = malloc(sizeof(*w));
    CHECK(w);
    world_init(w, 51);
    Vec3 origin = world_center((Cell){1, 0, 1});
    origin.y += .02;
    CHECK(world_refresh(w, &origin, 1));
    CHECK(w->chunk_count == 0 && w->pending > 0);
    CHECK(world_overlap(w, origin, .34, 1.78));
    CHECK(world_stream(w, 1) == 1);
    CHECK(!world_ready(w, (Cell){1, 0, 1})); /* Hall publication is atomic. */
    settle(w);
    Room *r = world_find(w, (Cell){1, 0, 1});
    CHECK(r && r->ready && r->enemy_count > 0);
    world_kill(w, r, 0);
    CHECK(w->record_count == 1);
    /* Repeated seam crossing should reuse the retained neighborhood. */
    Vec3 seam = world_center((Cell){2, 0, 1});
    CHECK(world_refresh(w, &seam, 1));
    settle(w);
    CHECK(world_refresh(w, &origin, 1));
    settle(w);
    uint32_t generated = w->generated;
    for (int i = 0; i < 20; i++) {
        CHECK(world_refresh(w, i % 2 ? &seam : &origin, 1));
        settle(w);
        CHECK(w->generated == generated);
    }
    for (int i = 1; i <= 100; i++) {
        Vec3 positions[] = {world_center((Cell){i * 3, 0, i * 3}),
                            world_center((Cell){-i * 3, 2, i * 3})};
        CHECK(world_refresh(w, positions, 2));
        world_stream(w, 2);
        CHECK(world_chunk(w, world_describe(w->seed, world_cell(positions[0])).cell) != NULL);
        CHECK(world_chunk(w, world_describe(w->seed, world_cell(positions[1])).cell) != NULL);
        settle(w);
        CHECK(w->chunk_count <= SC_MAX_CHUNKS && w->count <= SC_MAX_ROOMS);
        CHECK(w->box_count <= SC_MAX_CHUNKS * SC_CHUNK_BOXES);
        CHECK(world_ready(w, world_cell(positions[0])) && world_ready(w, world_cell(positions[1])));
        CHECK(w->record_count == 1);
        for (int a = 0; a < w->count; a++)
            for (int b = a + 1; b < w->count; b++)
                CHECK(!cell_equal(w->rooms[a].cell, w->rooms[b].cell));
    }
    CHECK(world_refresh(w, &origin, 1));
    settle(w);
    r = world_find(w, (Cell){1, 0, 1});
    CHECK(r && !r->enemies[0].alive);
    Vec3 same[] = {origin, origin};
    CHECK(world_refresh(w, same, 2));
    settle(w);
    int count = w->chunk_count;
    CHECK(world_refresh(w, same, 1));
    settle(w);
    CHECK(count == w->chunk_count);
    Vec3 extreme[] = {world_center((Cell){8000000, 0, 5000000}),
                      world_center((Cell){-8000000, 2, 5000000})};
    CHECK(world_refresh(w, extreme, 1));
    settle(w);
    CHECK(world_refresh(w, extreme, 2));
    settle(w);
    CHECK(world_ready(w, world_cell(extreme[0])) && world_ready(w, world_cell(extreme[1])));
    world_destroy(w);
    free(w);
    puts("PASS bounded fair streaming, atomic hall publication, shared co-op chunks, hysteresis, "
         "100 separated-player moves, persistent kills");
}
static void test_movement(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g != NULL);
    unsigned seed = 17;
    while (!world_door_version(seed, 6, (Cell){0, -1, 0}, EAST))
        seed++;
    legacy_game_init(g, seed);
    settle_game(g);
    for (int r = 0; r < g->world.count; r++)
        g->world.rooms[r].enemy_count = 0;
    CHECK(g->player_count == 1);
    step(g, (Actions){.forward = 1}, 700);
    CHECK(position_absolute(g->players[0].pos).z > 55);
    CHECK(position_absolute(g->players[0].pos).y < .2);
    CHECK(g->players[0].grounded);
    Player *p = &g->players[0];
    p->pos = position_from(v3(10, -SC_DECK + .02, 48));
    settle_game(g);
    for (int r = 0; r < g->world.count; r++)
        g->world.rooms[r].enemy_count = 0;
    p->velocity = v3(0, 0, 0);
    p->yaw = -SC_PI * .5;
    step(g, (Actions){.forward = 1}, 80);
    CHECK(position_absolute(p->pos).x > 3.5);
    step(g, (Actions){.forward = 1, .crouch = true}, 140);
    CHECK(position_absolute(p->pos).x < 3.3);
    CHECK(p->crouched);
    step(g, (Actions){0}, 2);
    CHECK(p->crouched);
    CHECK(!world_overlap(&g->world, position_absolute(p->pos), .34, 1));
    game_join(g);
    CHECK(g->players[1].crouched);
    CHECK(!world_overlap(&g->world, position_absolute(g->players[1].pos), .34, 1));
    game_leave(g);
    p->pos = position_from(world_lift((Cell){0, 0, 0}));
    settle_game(g);
    for (int r = 0; r < g->world.count; r++)
        g->world.rooms[r].enemy_count = 0;
    p->velocity = v3(0, 0, 0);
    step(g, (Actions){.lift_up = true}, 1);
    CHECK(position_absolute(p->pos).y > 31);
    CHECK(position_absolute(p->checkpoint).y > 31);
    p->health = 0;
    step(g, (Actions){0}, 1);
    CHECK(g->respawns == 1);
    CHECK(p->health == player_max_health(p));
    CHECK(vlen(position_delta(p->pos, p->checkpoint)) < .1);
    game_destroy(g);
    free(g);
    puts("PASS space approach, gravity, crouch clearance, lift, checkpoint respawn");
}
static void test_combat(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g != NULL);
    legacy_game_init(g, 81);
    settle_game(g);
    Player *p = &g->players[0];
    p->pos = position_from(v3(0, .02, 20));
    p->yaw = 0;
    p->pitch = -.21;
    p->xp = 70;
    Room *bay = world_find(&g->world, (Cell){0, 0, 0});
    CHECK(bay != NULL);
    bay->enemy_count = 1;
    bay->enemies[0] =
        (Enemy){.pos = position_from(v3(0, .02, 24)), .health = 10, .kind = 0, .alive = true};
    step(g, (Actions){.fire = true}, 1);
    CHECK(g->shots == 1 && g->hits == 0);
    step(g, (Actions){0}, 8);
    CHECK(g->hits == 1);
    CHECK(p->kills == 1);
    CHECK(!bay->enemies[0].alive);
    CHECK(p->level == 2 && p->points == 1);
    CHECK(game_upgrade(g, 0, 1));
    CHECK(p->vitality == 1 && p->health == 125);
    CHECK(!game_upgrade(g, 0, 0));
    CHECK(world_raycast(&g->world, v3(0, 2, 20), v3(1, 0, 0), 100) < 42);
    Position position = p->pos;
    game_join(g);
    CHECK(g->player_count == 2);
    game_look(&g->players[1], 1, .5);
    CHECK(g->players[0].yaw == 0);
    CHECK(vlen(position_delta(position, p->pos)) < 1e-6);
    game_leave(g);
    CHECK(g->player_count == 1);

    game_new_ship(g, 999);
    CHECK(g->world.seed == 999);
    CHECK(p->level == 1 && p->weapons == 1 && g->campaign.version == 1);
    CHECK(position_absolute(p->pos).z < 0);
    game_destroy(g);
    free(g);
    puts("PASS swept projectile combat, upgrades, optional co-op, new expedition progression");
}
static void test_save(void) {
    const char *path = "sc-test-expedition.sav";
    Game *g = malloc(sizeof(*g));
    CHECK(g != NULL);
    legacy_game_init(g, 123456);
    settle_game(g);
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
    settle_game(g);
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
    /* This file was written by the unmodified, pre-infestation v2 game. */
    g->projectiles[0].active = true;
    g->players[0].flashlight = false;
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v2.sav"));
    settle_game(g);
    CHECK(g->world.generator == 6 && g->dirty);
    CHECK(g->players[0].level == 7 && g->players[0].kills == 81 && g->players[0].power == 3);
    CHECK(g->players[1].level == 4 && g->players[1].haste == 2);
    CHECK(position_absolute(g->players[0].pos).z == -34);
    CHECK(g->world.record_count == 0 && g->players[0].weapons == SC_START_WEAPONS);
    CHECK(g->players[0].flashlight && !g->projectiles[0].active);
    game_destroy(g);
    free(g);
    puts("PASS save round-trip, v2 character migration, solo default, both profiles, "
         "corruption and failure safety");
}
static void test_migration_and_precision(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g != NULL);
    legacy_game_init(g, 77);
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v1.sav"));
    CHECK(g->world.generator == 6 && g->world.seed != 123456);
    CHECK(g->player_count == 1 && g->world.record_count == 0 && g->dirty);
    Player *p = &g->players[0];
    CHECK(p->level == 7 && p->xp == 31 && p->kills == 81 && p->points == 4);
    CHECK(p->power == 3 && p->vitality == 2 && p->tint == 2 && p->health == 150);
    CHECK(g->players[1].level == 4 && g->players[1].haste == 2);
    CHECK(position_absolute(p->pos).z == -34);
    CHECK(strstr(g->message, "NEW EXPEDITION") != NULL);
    const char *path = "sc-test-migration.sav";
    CHECK(game_save(g, path));
    uint32_t seed = g->world.seed;
    CHECK(game_load(g, path));
    CHECK(g->world.seed == seed && !g->dirty);
    g->world.generator = 999;
    CHECK(game_save(g, path));
    g->world.generator = 6;
    CHECK(!game_load(g, path));
    CHECK(g->world.seed == seed && p->level == 7);
    remove(path);
    /* Physics coordinates normalize locally; far-apart cameras never share a
       single floating origin. Exercise seams hundreds of millions of metres out. */
    Position near = position_from(v3(2, .02, 50)),
             far = position_from(v3(288000002, 128000.02, 288000050));
    for (int i = 0; i < 10000; i++) {
        position_move(&near, v3(.125, .001, .0625));
        position_move(&far, v3(.125, .001, .0625));
    }
    CHECK(vlen(vsub(position_delta(far, near), v3(288000000, 128000, 288000000))) < 1e-7);
    p->pos = position_from(vadd(world_center((Cell){3000000, 2000, 3000000}), v3(0, .02, 0)));
    p->checkpoint = p->pos;
    p->velocity = v3(0, 0, 0);
    p->crouched = true;
    g->players[1].pos = position_from(v3(0, 4, -34));
    g->player_count = 2;
    settle_game(g);
    CHECK(world_ready(&g->world, world_cell(position_absolute(p->pos))));
    CHECK(world_ready(&g->world, (Cell){0, 0, 0}));
    CHECK(!world_overlap(&g->world, position_absolute(p->pos), .34, 1));
    Position before = p->pos;
    step(g, (Actions){.forward = 1, .crouch = true}, 10);
    CHECK(position_delta(p->pos, before).z > .5);
    CHECK(game_save(g, path));
    CHECK(game_load(g, path));
    settle_game(g);
    CHECK(g->player_count == 1 && position_absolute(p->pos).x > 287999000);
    CHECK(world_ready(&g->world, world_cell(position_absolute(p->pos))));
    remove(path);
    game_destroy(g);
    free(g);
    puts("PASS v1 character migration, generator compatibility, sector-local physics, distant "
         "co-op collision and save/resume");
}
static Room *quiet_encounter(Game *g) {
    legacy_game_init(g, 713);
    g->players[0].pos = position_from(vadd(world_center((Cell){1, 0, 0}), v3(0, .02, -24)));
    g->players[0].hurt = 10;
    g->players[0].shield = 0;
    g->players[0].armor = 0;
    settle_game(g);
    for (int r = 0; r < g->world.count; r++)
        g->world.rooms[r].enemy_count = 0;
    return world_find(&g->world, (Cell){1, 0, 0});
}
static Enemy *place_enemy(Room *room, int kind, Vec3 local) {
    room->enemy_count = 1;
    room->enemies[0] = (Enemy){.pos = position_from(vadd(world_center(room->cell), local)),
                               .health = 100,
                               .normal = {0, 1, 0},
                               .heading = {0, 0, -1},
                               .kind = (uint8_t)kind,
                               .alive = true};
    return &room->enemies[0];
}
static void test_infestation(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    Room *room = quiet_encounter(g);
    Enemy *e = place_enemy(room, 1, v3(0, .02, -8));
    CHECK(g->players[0].flashlight);
    game_flashlight(g, 0);
    CHECK(!g->players[0].flashlight);
    game_join(g);
    CHECK(g->players[1].flashlight);
    game_flashlight(g, 1);
    CHECK(!g->players[1].flashlight);
    game_leave(g);
    step(g, (Actions){0}, 1);
    CHECK(e->windup > 0 && g->enemy_shots == 0 && g->players[0].health == 100);
    step(g, (Actions){.strafe = 1}, 120);
    step(g, (Actions){0}, 45);
    CHECK(g->enemy_shots == 1 && g->projectile_hits == 0 && g->players[0].health == 100);
    game_destroy(g);
    room = quiet_encounter(g);
    e = place_enemy(room, 1, v3(0, .02, -8));
    step(g, (Actions){0}, 60);
    CHECK(g->enemy_shots == 1 && g->projectile_hits == 0 && g->players[0].health == 100);
    step(g, (Actions){0}, 90);
    CHECK(g->projectile_hits == 1 && g->players[0].health < 100);
    game_destroy(g);
    room = quiet_encounter(g);
    Vec3 eye = player_eye(&g->players[0]);
    g->projectiles[0] =
        (Projectile){position_from(vadd(eye, v3(0, 0, -5))), {0, 0, 200}, 2, 12, true, -1};
    Actions none[2] = {{0}, {0}};
    game_step(g, none, .05);
    CHECK(g->projectile_hits == 1 && g->players[0].health == 88 && !g->projectiles[0].active);
    /* A wall intercepts a fast shot before it can reach the player. */
    g->players[0].health = 100;
    g->projectiles[0] =
        (Projectile){position_from(vadd(world_center(room->cell), v3(room->width * .5 + 4, 1, -8))),
                     {-200, 0, 0},
                     2,
                     12,
                     true,
                     -1};
    game_step(g, none, .05);
    CHECK(!g->projectiles[0].active && g->players[0].health == 100);
    game_join(g);
    g->players[1].hurt = 10;
    g->players[1].shield = 0;
    g->players[1].armor = 0;
    g->players[1].pos = g->players[0].pos;
    position_move(&g->players[1].pos, v3(0, 0, -2));
    g->projectiles[0] =
        (Projectile){position_from(vadd(eye, v3(0, 0, -5))), {0, 0, 200}, 2, 12, true, -1};
    game_step(g, none, .05);
    CHECK(g->players[0].health == 100 && g->players[1].health == 88);
    g->projectiles[0].active = true;
    game_new_ship(g, 17);
    CHECK(!g->projectiles[0].active);
    game_destroy(g);
    room = quiet_encounter(g);
    e = place_enemy(room, 0, v3(-(room->width * .5 - 1.1), .02, -20));
    step(g, (Actions){0}, 20);
    CHECK(e->climbing && e->normal.x > .5 && position_absolute(e->pos).y > .8);
    CHECK(!world_overlap(&g->world, position_absolute(e->pos), .42, 1.25));
    for (int frame = 0; frame < 180 && e->climbing; frame++)
        step(g, (Actions){0}, 1);
    CHECK(!e->climbing && position_absolute(e->pos).y >= -.001);
    game_destroy(g);
    room = quiet_encounter(g);
    e = place_enemy(room, 0, v3(0, .02, -8));
    Position before = e->pos;
    step(g, (Actions){0}, 30);
    CHECK(vlen(position_delta(e->pos, before)) > 2);
    for (int c = 0; c < SC_MAX_CHUNKS; c++)
        if (g->world.chunks[c].active) {
            const Chunk *chunk = &g->world.chunks[c];
            CHECK(chunk->light_count == SC_CHUNK_LIGHTS);
            CHECK(chunk->dressing_count > 0 && chunk->dressing_count <= SC_CHUNK_DRESSING);
        }
    bool variation = false;
    float previous = world_lamp_power(1, 0);
    for (int i = 0; i < 200; i++) {
        CHECK(world_lamp_power(0, i * .1) == 0);
        float power = world_lamp_power(1, i * .1);
        CHECK(power >= 0 && power <= 1);
        if (fabsf(power - previous) > .1f)
            variation = true;
    }
    CHECK(variation);
    game_destroy(g);
    free(g);
    puts("PASS charged dodgeable shots, swept player/wall collision, co-op projectile ownership, "
         "wall climbing, fast crawlers, flashlight controls, bounded dressing and broken lamps");
}
static void test_expansion(void) {
    uint32_t themes = 0;
    uint64_t enemies = 0;
    for (unsigned seed = 1; seed <= 24; seed++)
        for (int y = -4; y <= 9; y++) {
            RegionPlan p;
            world_plan(seed, (Cell){2, y, 2}, &p);
            for (int i = 0; i < p.count; i++) {
                CHECK(p.rooms[i].theme < SC_THEMES);
                themes |= 1u << p.rooms[i].theme;
            }
        }
    CHECK((themes | 1u) == ((1u << SC_THEMES) - 1));
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    Room *room = quiet_encounter(g);
    Player *p = &g->players[0];
    p->shield = 50;
    p->armor = 40;
    game_hurt(g, 0, 30, 0);
    CHECK(p->health == 100 && p->shield == 20 && p->armor == 40);
    game_hurt(g, 0, 40, CHILL | DRAIN);
    CHECK(p->shield == 0 && p->armor < 40 && p->health < 100 && p->slow > 0 && p->energy == 60);
    float health = p->health, shield = p->shield;
    step(g, (Actions){0}, 180);
    CHECK(p->shield > shield && p->health > health);
    for (int a = 0; a < SC_ATTRIBUTES; a++) {
        p->points = 1;
        unsigned old = *player_attribute(p, a);
        CHECK(game_upgrade(g, 0, a));
        CHECK(*player_attribute(p, a) == old + 1);
    }
    p->points = 0;
    p->credits = 1000;
    CHECK(game_upgrade(g, 0, 9));
    CHECK(p->credits < 1000);
    p->weapons = 1;
    game_weapon(g, 0, 1);
    CHECK(p->weapon == 0);
    p->weapons = 7;
    game_weapon(g, 0, 1);
    CHECK(p->weapon == 1);
    game_destroy(g);
    for (int weapon = 0; weapon < SC_GENERATED_WEAPONS; weapon++) {
        room = quiet_encounter(g);
        p = &g->players[0];
        p->weapon = weapon;
        p->weapons = (1u << SC_WEAPONS) - 1;
        p->energy = 100;
        p->pitch = atan2(.65 - 1.62, 16);
        Enemy *e = place_enemy(room, 1, v3(0, .02, -8));
        e->health = 500;
        e->cooldown = 100;
        step(g, (Actions){.fire = true}, 1);
        CHECK(g->shots == 1);
        step(g, (Actions){0}, 75);
        CHECK(e->health < 500 || !e->alive);
        CHECK(p->energy >= 0 && p->energy <= player_max_energy(p));
        game_destroy(g);
    }
    /* All species execute under collision, plus specific ability regressions. */
    for (int kind = 0; kind < SC_ENEMY_TYPES; kind++) {
        room = quiet_encounter(g);
        Enemy *e = place_enemy(room, kind, v3(0, .02, -8));
        e->health = 30;
        e->special = 0;
        step(g, (Actions){0}, 90);
        CHECK(isfinite(position_absolute(e->pos).x) && isfinite(e->health));
        enemies |= UINT64_C(1) << kind;
        if (sc_enemies[kind].flags & BROOD)
            CHECK(room->enemy_count > 1);
        if (sc_enemies[kind].flags & REGENERATE)
            CHECK(e->health > 30);
        if (sc_enemies[kind].flags & HOVER)
            CHECK(position_absolute(e->pos).y > .8);
        if (sc_enemies[kind].flags & TURRET)
            CHECK(fabs(position_absolute(e->pos).z - (world_center(room->cell).z - 8)) < .01);
        game_destroy(g);
    }
    CHECK(enemies == ((UINT64_C(1) << SC_ENEMY_TYPES) - 1));
    room = quiet_encounter(g);
    p = &g->players[0];
    Pickup *loot = &room->pickups[3];
    p->pos = loot->pos;
    uint32_t mask = p->weapons;
    step(g, (Actions){0}, 1);
    CHECK(!loot->active && (p->weapons & mask) == mask && p->pickups > 0);
    unsigned gun = p->weapon;
    size_t records = g->world.record_count;
    step(g, (Actions){0}, 1);
    CHECK(g->world.record_count == records && p->weapon == gun);
    /* Shared loot has a single owner, independent of the co-op profile. */
    game_join(g);
    g->players[1].pos = p->pos;
    unsigned second = g->players[1].pickups;
    step(g, (Actions){0}, 1);
    CHECK(g->players[1].pickups == second);
    game_leave(g);
    CHECK(game_save(g, "sc-cache-check.sav"));
    CHECK(game_load(g, "sc-cache-check.sav"));
    settle_game(g);
    room = world_find(&g->world, (Cell){1, 0, 0});
    CHECK(room && !room->pickups[3].active);
    remove("sc-cache-check.sav");
    /* Keep the original campaign regression alongside generated-story tests. */
    g->story = (Story){0};
    for (int stage = 0; stage < 6; stage++) {
        Cell target;
        CHECK(game_objective(g, &target));
        p->pos = position_from(game_terminal(target));
        settle_game(g);
        for (int r = 0; r < g->world.count; r++)
            g->world.rooms[r].enemy_count = 0;
        unsigned before = g->mission;
        game_interact(g, 0);
        CHECK(g->mission == before + 1);
    }
    /* A living Warden blocks the heart even when lured away from its console. */
    unsigned victories = p->victories;
    g->mission = 4;
    Cell heartcell = {2, 1, 2};
    p->pos = position_from(game_terminal(heartcell));
    settle_game(g);
    for (int r = 0; r < g->world.count; r++)
        g->world.rooms[r].enemy_count = 0;
    Room *heart = world_find(&g->world, heartcell);
    CHECK(heart);
    Enemy *warden = place_enemy(heart, 26, v3(0, .02, 0));
    warden->pos = p->pos;
    position_move(&warden->pos, v3(0, 0, 4));
    warden->health = 1;
    game_interact(g, 0);
    CHECK(g->mission == 4);
    p->weapon = 0;
    p->yaw = 0;
    p->pitch = atan2(.65 - 1.62, 4);
    p->cooldown = 0;
    step(g, (Actions){.fire = true}, 1);
    step(g, (Actions){0}, 30);
    CHECK(!warden->alive);
    game_interact(g, 0);
    CHECK(g->mission == 5 && (p->weapons & (1u << 15)));
    g->mission = 6;
    p->victories = victories;
    Cell target;
    CHECK(!game_objective(g, &target) && p->victories == 1);
    g->mission_kills = 12;
    g->mission_loot = 8;
    game_progression_step(g, .01);
    CHECK(g->contracts == 7);
    unsigned points = p->points;
    game_progression_step(g, .01);
    CHECK(p->points == points);
    p->mods[0] = 4;
    p->mods[1] = 3;
    p->weapons = (1u << SC_WEAPONS) - 1;
    p->weapon = 15;
    p->logs = 4095;
    p->armor = player_max_armor(p);
    p->shield = player_max_shield(p);
    p->energy = player_max_energy(p);
    CHECK(game_save(g, "sc-expansion.sav"));
    game_new_ship(g, 88);
    CHECK(game_load(g, "sc-expansion.sav"));
    settle_game(g);
    CHECK(g->mission == 6 && g->contracts == 7 && p->weapon == 15 && p->mods[0] == 4 &&
          p->mods[1] == 3 && p->logs == 4095 && p->victories == 1);
    game_new_ship(g, 89);
    CHECK(g->mission == 0 && g->contracts == 0 && p->weapon == 0 && p->logs == 0 &&
          p->victories == 0);
    remove("sc-expansion.sav");
    game_destroy(g);
    free(g);
    puts("PASS 28 themes, 27 species, 16 functional weapons, 10 attributes, shield/armor, shared "
         "persistent loot, six-stage story, contracts, v3 saves");
}
static void fingerprints(void) {
    uint32_t seeds[] = {1, 713, UINT32_MAX};
    Cell regions[] = {
        {0, 0, 0}, {-2, -1, 0}, {2, 3, 1}, {500000, 2000, 500000}, {-500000, -2000, 500000}};
    printf("[");
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 5; j++) {
            Cell c = regions[j];
            printf("%s{\"seed\":%u,\"region\":[%d,%d,%d],\"hash\":%u}", i || j ? "," : "", seeds[i],
                   c.x, c.y, c.z, world_fingerprint(seeds[i], c));
        }
    puts("]");
}
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--fingerprints")) {
        fingerprints();
        return 0;
    }
    test_topology();
    test_generation();
    test_streaming();
    test_movement();
    test_combat();
    test_save();
    test_migration_and_precision();
    test_infestation();
    test_expansion();
    printf("PASS %d assertions\n", checks);
    return 0;
}
