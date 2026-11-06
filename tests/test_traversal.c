#include "game.h"
#include "geometry_fingerprint.h"
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
static void settle(Game *g) {
    Vec3 at[2] = {position_absolute(g->players[0].pos), position_absolute(g->players[1].pos)};
    CHECK(world_refresh(&g->world, at, g->player_count));
    while (g->world.pending && !g->world.failed)
        CHECK(world_stream(&g->world, SC_STREAM_BUDGET) <= SC_STREAM_BUDGET);
    CHECK(!g->world.failed);
}
static Game *fresh(void) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    g->players[0].pos = position_from(v3(0, .02, 12));
    settle(g);
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void tick(Game *g, Actions a, int frames) {
    Actions actions[2] = {a, {0}};
    for (int n = 0; n < frames; n++) {
        world_stream(&g->world, SC_STREAM_BUDGET);
        game_step(g, actions, 1.0 / 60);
        CHECK(!g->world.failed);
    }
}
static void fixtures(void) {
    Game *g = fresh();
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v8-format11.sav"));
    CHECK(g->world.generator == 8 && g->players[0].xp == 11 && g->players[0].credits == 23);
    settle(g);
    CHECK(!world_overlap(&g->world, position_absolute(g->players[0].pos), .34, 1.78));
    CHECK(!g->players[0].lift_active);
    FILE *file = fopen(SC_FIXTURE_DIR "/generator-v8-geometry.txt", "r");
    CHECK(file);
    unsigned seed, hash, count = 0;
    Cell cell;
    while (fscanf(file, "%u %d %d %d %u", &seed, &cell.x, &cell.y, &cell.z, &hash) == 5) {
        world_destroy(&g->world);
        world_init(&g->world, seed);
        g->world.generator = 8;
        g->players[0].pos = position_from(world_lift(cell));
        settle(g);
        const Chunk *c = world_chunk(&g->world, cell);
        CHECK(c && !c->flight_count && c->box_count <= SC_LEGACY_BOXES);
        CHECK(geometry_fingerprint(c) == hash);
        count++;
    }
    fclose(file);
    CHECK(count == 20);
    /* Original ships still transfer immediately. */
    g->players[0].pos = position_from(world_lift((Cell){0, 0, 0}));
    settle(g);
    tick(g, (Actions){.lift_up = true}, 1);
    CHECK(!g->players[0].lift_active && g->players[0].pos.sector.y == 1);
    destroy(g);
    puts("PASS immutable generator-8 save, 20 geometry fixtures and legacy lift behavior");
}
static void surfaces(void) {
    for (int reverse = 0; reverse < 2; reverse++)
        for (int stairs = 0; stairs < 2; stairs++) {
            Flight f = {v3(4, 0, -28), v3(8, 8, -12), {1, 1, 1}, reverse != 0, stairs != 0};
            for (int i = 0; i < 32; i++) {
                double z = -28 + (i + .5) * .5, y = flight_top(&f, z);
                CHECK(flight_overlap(&f, v3(6, y - .1, z), .01, .2));
                CHECK(!flight_overlap(&f, v3(6, y + .02, z), .01, 1.78));
                CHECK(!flight_overlap(&f, v3(6, y - 3, z), .01, 1));
                CHECK(fabs(flight_ray(&f, v3(6, y + 2, z), v3(0, -1, 0), 5) - 2) < 1e-6);
                CHECK(fabs(flight_ray(&f, v3(6, y - 2, z), v3(0, 1, 0), 5) - 1.65) < 1e-6);
                CHECK(flight_ray(&f, v3(6, y + 2, z), v3(0, -1, 0), 1) == 1);
                CHECK(flight_ray(&f, v3(6, y - .1, z), v3(1, 0, 0), 4) == 0);
            }
            CHECK(!flight_overlap(&f, v3(9, 4, -20), .34, 1.78));
            CHECK(!flight_overlap(&f, v3(6, 4, -10), .34, 1.78));
            CHECK(flight_overlap(&f, v3(3.9, 4, -20), .34, 1.78));
            CHECK(flight_ray(&f, v3(9, 2, -20), v3(0, -1, 0), 20) == 20);
            CHECK(flight_ray(&f, v3(6, 20, -10), v3(0, -1, 0), 30) == 30);
            CHECK(flight_ray(&f, v3(6, 20, -20), v3(1, 0, 0), 30) == 30);
        }
    puts("PASS tread/slab top, underside, side, parallel, starting-inside and limited rays");
}
static void geometry(void) {
    Game *g = fresh();
    size_t maximum = 0;
    for (uint32_t seed = 1; seed <= 24; seed++) {
        for (int rx = -2; rx <= 2; rx++)
            for (int rz = 0; rz < 3; rz++) {
                int columns = 0;
                for (int x = 0; x < 4; x++)
                    for (int z = 0; z < 4; z++) {
                        Cell c = {rx * 4 + x, 0, rz * 4 + z};
                        bool service = world_service_column(seed, c);
                        columns += service;
                        c.y = -3;
                        CHECK(world_service_column(seed, c) == service);
                    }
                CHECK(columns == 1);
            }
        world_destroy(&g->world);
        world_init(&g->world, seed);
        settle(g);
        for (int i = 0; i < SC_MAX_CHUNKS; i++) {
            const Chunk *c = &g->world.chunks[i];
            if (!c->active)
                continue;
            CHECK(c->flight_count == (world_service_column(seed, c->cell) ? 16 : 0));
            CHECK(c->box_count <= SC_CHUNK_BOXES);
            if (c->box_count > maximum)
                maximum = c->box_count;
            if (!world_ready(&g->world, c->cell))
                continue;
            CHECK(!world_overlap(&g->world, world_lift(c->cell), .8, 2.8));
            CHECK(!world_overlap(&g->world, game_terminal(c->cell), .34, 1));
            if (c->flight_count) {
                Vec3 origin = world_center(c->cell);
                Vec3 feet = vadd(origin, v3(6, 4.6, -20));
                CHECK(!world_overlap(&g->world, feet, .34, 1.78));
                CHECK(world_raycast(&g->world, feet, v3(0, -1, 0), 10) < 1);
                /* Deck apron remains connected to the room cross route. */
                for (int x = -14; x <= 14; x++)
                    CHECK(!world_overlap(&g->world, vadd(origin, v3(x, .02, -8)), .34, 1.78));
            }
        }
    }
    destroy(g);
    printf("PASS aligned region columns, landing/terminal clearance and rays; max %zu boxes\n",
           maximum);
}
static void walk_to(Game *g, double x, double z) {
    for (int i = 0; i < 700; i++) {
        Vec3 p = position_absolute(g->players[0].pos), d = v3(x - p.x, 0, z - p.z);
        double distance = vlen(d);
        if (distance < .1)
            return;
        g->players[0].yaw = atan2(d.x, d.z);
        tick(g, (Actions){.forward = (float)fmin(1, distance / .125)}, 1);
        CHECK(!world_overlap(&g->world, position_absolute(g->players[0].pos), .34, 1.78));
    }
    Vec3 p = position_absolute(g->players[0].pos);
    fprintf(stderr, "Walk blocked at %.4f %.4f %.4f toward %.2f %.2f\n", p.x, p.y, p.z, x, z);
    CHECK(false);
}
static void journeys(void) {
    for (int side = -1; side <= 1; side += 2) {
        Game *g = fresh();
        walk_to(g, 0, 17);
        walk_to(g, side * 6, 17);
        for (int k = 0; k < 8; k++) {
            walk_to(g, side * (k % 2 ? 10 : 6), k % 2 ? 17 : 39);
            CHECK(fabs(position_absolute(g->players[0].pos).y - (k + 1) * 8) < .06);
            if (k < 7)
                walk_to(g, side * (k % 2 ? 6 : 10), k % 2 ? 17 : 39);
        }
        CHECK(g->players[0].pos.sector.y == 1 && !g->players[0].lift_active);
        CHECK(game_save(g, "traversal-landing.sav"));
        CHECK(game_load(g, "traversal-landing.sav"));
        settle(g);
        for (int k = 7; k >= 0; k--) {
            walk_to(g, side * (k % 2 ? 10 : 6), k % 2 ? 39 : 17);
            CHECK(fabs(position_absolute(g->players[0].pos).y - k * 8) < .06);
            if (k > 0)
                walk_to(g, side * (k % 2 ? 6 : 10), k % 2 ? 39 : 17);
        }
        walk_to(g, 0, 17);
        walk_to(g, 0, 38);
        CHECK(g->players[0].pos.sector.y == 0 && g->respawns == 0);
        /* The negative deck seam uses the same aligned continuous architecture. */
        g->players[0].pos = position_from(v3(side * 6, -64 + .02, 17));
        settle(g);
        for (int k = 0; k < 8; k++) {
            walk_to(g, side * (k % 2 ? 10 : 6), k % 2 ? 17 : 39);
            if (k < 7)
                walk_to(g, side * (k % 2 ? 6 : 10), k % 2 ? 17 : 39);
        }
        CHECK(fabs(position_absolute(g->players[0].pos).y) < .06);
        destroy(g);
    }
    remove("traversal-landing.sav");
    puts(
        "PASS real normal-gravity actions: stairs and ramps up/down 64m, reload and negative seam");
}
static void lifts(void) {
    Game *g = fresh(), *copy = fresh();
    Player *p = &g->players[0];
    p->pos = position_from(world_lift((Cell){0, 0, 0}));
    Position checkpoint = p->checkpoint;
    g->rigs[1][0] =
        (Rig){.pos = position_from(world_lift((Cell){0, 1, 0})), .armor = 300, .deployed = true};
    tick(g, (Actions){.lift_up = true}, 1);
    CHECK(!p->lift_active && strstr(g->message, "OBSTRUCTED"));
    g->rigs[1][0].deployed = false;
    tick(g, (Actions){.lift_up = true}, 1);
    CHECK(p->lift_active && p->lift_progress == 0);
    tick(g, (Actions){.forward = 1, .jump = true, .fire = true, .lift_down = true}, 60);
    CHECK(p->lift_active && position_absolute(p->pos).y > 10 && position_absolute(p->pos).y < 50);
    CHECK(vlen(position_delta(p->checkpoint, checkpoint)) == 0 && g->shots == 0);
    game_look(p, .4, .1);
    CHECK(p->yaw == .4);
    Position underway = p->pos;
    CHECK(!campaign_refit(g, 0, 0, 1));
    game_summon(g, 0);
    game_interact(g, 0);
    game_rig_toggle(g, 0, MECH);
    CHECK(!game_rig_interact(g, 0));
    game_psionic(g, 0, PSI_FLIGHT);
    CHECK(!matrix_toggle(g, 0));
    CHECK(!p->sheltered && !g->shelter.deployed && !p->riding && !p->flight);
    CHECK(game_save(g, "traversal-ride.sav"));
    CHECK(game_load(copy, "traversal-ride.sav"));
    CHECK(!copy->players[0].lift_active &&
          vlen(position_delta(copy->players[0].pos, p->lift_from)) < .001);
    CHECK(vlen(position_delta(p->pos, underway)) == 0 && p->lift_active);
    CHECK(!game_save(g, "missing/traversal.sav"));
    CHECK(!game_load(g, "missing/traversal.sav"));
    CHECK(p->lift_active);
    game_join(g);
    CHECK(!g->players[1].lift_active && fabs(position_absolute(g->players[1].pos).y) < .03);
    tick(g, (Actions){0}, 125);
    CHECK(!p->lift_active && p->pos.sector.y == 1 && p->checkpoint.sector.y == 1);
    CHECK(g->players[1].pos.sector.y == 0);
    tick(g, (Actions){.lift_down = true}, 250);
    CHECK(!p->lift_active && p->pos.sector.y == 0);
    /* Independent rides: joining/leaving cancels only the departing participant. */
    g->players[1].pos = position_from(world_lift((Cell){0, 0, 0}));
    g->players[1].lift_cooldown = 0;
    Actions actions[2] = {{.lift_up = true}, {.lift_down = true}};
    p->lift_cooldown = 0;
    game_step(g, actions, 1.0 / 60);
    CHECK(p->lift_active && g->players[1].lift_active);
    tick(g, (Actions){0}, 50);
    game_leave(g);
    CHECK(p->lift_active && !g->players[1].lift_active && g->players[1].pos.sector.y == 0);
    game_lift_cancel(g, -1);
    game_lift_cancel(g, SC_PLAYERS);
    game_lift_cancel(g, 0);
    game_lift_cancel(g, 0);
    CHECK(!p->lift_active && p->pos.sector.y == 0);
    /* A boarded rig is snapshotted with its rider, without mutating live transit. */
    g->campaign.profiles[0].rigs |= 1u << (MECH - 1);
    g->campaign.profiles[0].rig = MECH;
    p->riding = MECH;
    g->rigs[0][0] = (Rig){.pos = p->pos, .armor = 300, .deployed = true};
    p->lift_cooldown = 0;
    tick(g, (Actions){.lift_up = true}, 50);
    CHECK(p->lift_active);
    CHECK(game_save(g, "traversal-ride.sav"));
    CHECK(game_load(copy, "traversal-ride.sav"));
    CHECK(copy->players[0].riding == MECH && !copy->players[0].lift_active);
    CHECK(vlen(position_delta(copy->rigs[0][0].pos, copy->players[0].pos)) < .001);
    p->health = -100;
    tick(g, (Actions){0}, 1);
    CHECK(!p->lift_active && p->health > 0 && g->respawns == 1);
    CHECK(g->rigs[0][0].pos.sector.y == 0);
    p->pos = position_from(world_lift((Cell){0, 1, 0}));
    p->lift_cooldown = 0;
    settle(g);
    tick(g, (Actions){.lift_up = true}, 1);
    CHECK(!p->lift_active && strstr(g->message, "SEALED"));
    /* A newly requested frontier never launches until the destination is ready. */
    p->pos = position_from(world_lift((Cell){0, 0, 0}));
    world_destroy(&g->world);
    world_init(&g->world, 713);
    Vec3 at = position_absolute(p->pos);
    world_refresh(&g->world, &at, 1);
    while (!world_ready(&g->world, (Cell){0, 0, 0}))
        world_stream(&g->world, 1);
    game_step(g, actions, 1.0 / 60);
    CHECK(!p->lift_active && strstr(g->message, "SURVEYING"));
    settle(g);
    tick(g, (Actions){.lift_up = true}, 1);
    CHECK(p->lift_active);
    g->world.open_layer = 1;
    p->lift_to = position_from(world_lift((Cell){0, 2, 0}));
    tick(g, (Actions){0}, 1);
    CHECK(!p->lift_active && p->pos.sector.y == 0);
    p->lift_cooldown = 0;
    tick(g, (Actions){.lift_up = true}, 1);
    CHECK(p->lift_active);
    game_new_ship(g, 22);
    CHECK(!g->players[0].lift_active && !g->players[1].lift_active);
    remove("traversal-ride.sav");
    destroy(copy);
    destroy(g);
    puts("PASS animated rides, checkpoints, input guards, save/failure safety, rigs, co-op, death "
         "and barriers");
}
static void frontier(void) {
    Game *g = fresh();
    g->players[0].pos = position_from(world_lift((Cell){0, 1, 0}));
    settle(g);
    Room *next = world_find(&g->world, (Cell){0, 2, 0});
    CHECK(next && next->ready);
    Position start = position_from(v3(10, 127.75001, 21.1)), p = start;
    CHECK(!world_overlap(&g->world, position_absolute(p), .34, 1.78));
    bool grounded = false;
    world_move(&g->world, &p, v3(0, -.03, -2), .34, 1.78, true, &grounded);
    CHECK(position_absolute(p).y < 128 && p.sector.y == 1);
    CHECK(!world_overlap(&g->world, position_absolute(p), .34, 1.78));
    g->world.open_layer = 0;
    next->ready = false;
    p = start;
    world_move(&g->world, &p, v3(0, -.03, -2), .34, 1.78, true, &grounded);
    CHECK(position_absolute(p).y < 128 && p.sector.y == 1);
    next->ready = true;
    world_move(&g->world, &p, v3(0, -.03, -2), .34, 1.78, true, &grounded);
    CHECK(p.sector.y == 2 && position_absolute(p).y >= 128);
    destroy(g);
    puts("PASS real stair support stops at phase barriers and pending decks, then resumes safely");
}
static void movement_limits(void) {
    Game *g = fresh();
    const Chunk *original = world_chunk(&g->world, (Cell){0, 0, 0});
    Chunk *c = &g->world.chunks[original - g->world.chunks];
    c->flight_count = 0;
    c->box_count = 3;
    c->boxes[0] = (Box){v3(-20, -.5, -20), v3(20, 0, 20), {0}, 0, true};
    c->boxes[1] = (Box){v3(1, 0, -2), v3(3, .25, 2), {0}, 0, true};
    c->boxes[2] = (Box){v3(4, 0, -2), v3(6, 2, 2), {0}, 0, true};
    Position p = position_from(v3(0, .02, 48));
    bool grounded = false;
    world_move(&g->world, &p, v3(2, -.03, 0), .34, 1.78, true, &grounded);
    CHECK(position_absolute(p).x > 1.9 && position_absolute(p).y > .249 && grounded);
    world_move(&g->world, &p, v3(4, -.03, 0), .34, 1.78, true, &grounded);
    CHECK(position_absolute(p).x < 3.67);
    world_move(&g->world, &p, v3(-3, -.03, 0), .34, 1.78, true, &grounded);
    CHECK(position_absolute(p).y < .03);
    /* A low beam prevents standing ascent but permits a crouched approach. */
    c->boxes[2] = (Box){v3(.5, 1.9, -2), v3(3.5, 2.2, 2), {0}, 0, true};
    p = position_from(v3(0, .02, 48));
    world_move(&g->world, &p, v3(2, -.03, 0), .34, 1.78, true, &grounded);
    CHECK(position_absolute(p).x < .67);
    world_move(&g->world, &p, v3(2, -.03, 0), .34, 1, true, &grounded);
    CHECK(position_absolute(p).x > 1.9 && position_absolute(p).y > .249);
    p = position_from(v3(0, .02, 48));
    world_move(&g->world, &p, v3(2, 0, 0), .34, 1, false, NULL);
    CHECK(position_absolute(p).x < .67);
    g->players[0].pos = position_from(v3(2, .25, 48));
    g->players[0].crouched = true;
    tick(g, (Actions){0}, 1);
    CHECK(g->players[0].crouched);
    world_move(&g->world, &p, v3(0, 0, 0), .34, 1, true, NULL);
    /* Test automatic support directly; full journeys are a separate gameplay test. */
    c->box_count = 1;
    c->flight_count = 1;
    for (int stairs = 0; stairs < 2; stairs++) {
        c->flights[0] = (Flight){v3(8, 0, -10), v3(12, 4, 0), {0}, false, stairs != 0};
        p = position_from(v3(10, .02, 37.5));
        world_move(&g->world, &p, v3(0, -.02, 2), .34, 1.78, true, &grounded);
        CHECK(position_absolute(p).y > .6 &&
              !world_overlap(&g->world, position_absolute(p), .34, 1.78));
        world_move(&g->world, &p, v3(0, -.02, -2), .34, 1.78, true, &grounded);
        CHECK(position_absolute(p).y < .03);
    }
    destroy(g);
    puts("PASS step limit, descent snap, head clearance, crouch and flight without free stepping");
}
int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--journey")) {
        journeys();
        return 0;
    }
    fixtures();
    surfaces();
    geometry();
    movement_limits();
    frontier();
    lifts();
    return 0;
}
