#include "game.h"
#include "exterior.h"
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
    return g;
}
static void discard(Game *g) {
    game_destroy(g);
    free(g);
}
static void settle(World *w, Vec3 at) {
    CHECK(world_refresh(w, &at, 1));
    for (int i = 0; w->pending && i < 256; i++)
        world_stream(w, SC_STREAM_BUDGET);
    CHECK(!w->failed && !w->pending);
}
static void sky_visibility(void) {
    Vec3 outside = v3(0, 6, -84), inside = v3(0, 6, 12);
    const Vec3 sky[] = {{0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
    for (size_t i = 0; i < sizeof(sky) / sizeof(sky[0]); i++)
        CHECK(exterior_sky_visible(outside, sky[i]));
    CHECK(exterior_sky_visible(outside, v3(0, 0, 1)));
    CHECK(!exterior_sky_visible(outside, vnorm(v3(1, 0, 1))));
    CHECK(!exterior_sky_visible(outside, vnorm(v3(0, 1, 1))));
    CHECK(!exterior_sky_visible(outside, vnorm(v3(0, -1, 1))));
    CHECK(!exterior_sky_visible(outside, v3(1, 0, 1e-12)));
    CHECK(exterior_sky_visible(inside, v3(0, 0, -1)));
    CHECK(!exterior_sky_visible(inside, v3(0, 0, 1)));
    CHECK(!exterior_sky_visible(inside, v3(1, 0, 0)));
    CHECK(!exterior_sky_visible(v3(42, 6, 12), v3(0, 0, -1)));
    CHECK(!exterior_sky_visible(v3(0, 28, 12), v3(0, 0, -1)));
    CHECK(!exterior_sky_visible(v3(0, 0, 12), v3(0, 0, -1)));
    CHECK(!exterior_sky_visible(v3(10000000, 10000000, -20), v3(0, 0, 1)));
    CHECK(exterior_sky_visible(v3(10000000, 10000000, -20), v3(0, 0, -1)));
    puts("PASS surrounding sky, bay aperture, grazing angles and distant hull occlusion");
}
static void cabin_geometry(void) {
    Game *g = fresh();
    World *w = &g->world;
    CHECK(w->generator == 8);
    CHECK(!world_overlap(w, sc_arrival_spawn, .34, 1.78));
    CHECK(!exterior_cabin(w, sc_arrival_spawn));
    CHECK(exterior_cabin(w, v3(0, 4, -98)));
    const Vec3 absent[] = {{5, 4, -98}, {0, 2, -98}, {0, 9, -98}, {0, 4, -107}, {0, 4, -87}};
    for (size_t i = 0; i < sizeof(absent) / sizeof(absent[0]); i++)
        CHECK(!exterior_cabin(w, absent[i]));
    for (double z = -105; z <= -84; z += .25) {
        CHECK(!world_overlap(w, v3(0, 4, z), .34, 1.78));
        CHECK(!world_overlap(w, v3(1.4, 4, z), .34, 1.78));
    }
    for (size_t i = 0; i < sc_skiff_box_count; i++) {
        const Box *b = &sc_skiff_boxes[i];
        CHECK(b->min.x < b->max.x && b->min.y < b->max.y && b->min.z < b->max.z);
        if (b->solid) {
            Vec3 center = vmul(vadd(b->min, b->max), .5);
            CHECK(world_overlap(w, center, .03, .04));
        }
    }
    Vec3 eye = v3(0, 5.62, -98);
    CHECK(fabs(world_raycast(w, eye, v3(1, 0, 0), 50) - 4) < 1e-6);
    CHECK(fabs(world_raycast(w, eye, v3(-1, 0, 0), 50) - 4) < 1e-6);
    CHECK(fabs(world_raycast(w, eye, v3(0, 0, -1), 50) - 7.5) < 1e-6);
    CHECK(fabs(world_raycast(w, eye, v3(0, 1, 0), 50) - 2.68) < 1e-6);
    CHECK(fabs(world_raycast(w, eye, v3(0, -1, 0), 50) - 2.02) < 1e-6);
    CHECK(world_raycast(w, eye, v3(0, 0, 1), 50) == 50);
    CHECK(world_raycast(w, v3(100, 5, -100), v3(0, 0, -1), 50) == 50);
    CHECK(world_raycast(w, eye, v3(1, 0, 0), 2) == 2);
    w->generator = 7;
    CHECK(!world_overlap(w, v3(4.2, 4, -98), .34, 1.78));
    CHECK(world_raycast(w, eye, v3(1, 0, 0), 50) == 50);
    CHECK(!exterior_cabin(w, eye));
    discard(g);
    puts("PASS shared solid shell, open hatch, furnishings, weapon rays and legacy empty space");
}
static void interior_parity(void) {
    World *a = malloc(sizeof(*a)), *b = malloc(sizeof(*b));
    CHECK(a && b);
    for (unsigned seed = 1; seed <= 8; seed++) {
        world_init(a, seed);
        world_init(b, seed);
        a->generator = 7;
        for (int n = 0; n < 3; n++) {
            Cell c = n == 0 ? (Cell){0, 0, 0} : n == 1 ? (Cell){-8, 1, 8} : (Cell){8, -2, 12};
            CHECK(world_fingerprint_version(seed, 7, world_region(c)) ==
                  world_fingerprint_version(seed, 8, world_region(c)));
            Vec3 p = vadd(world_center(c), v3(0, .02, 0));
            settle(a, p);
            settle(b, p);
            CHECK(a->count == b->count && a->box_count == b->box_count);
            for (int i = 0; i < a->count; i++) {
                const Room *left = &a->rooms[i], *right = world_find(b, left->cell);
                CHECK(right && left->enemy_count == right->enemy_count);
                for (int e = 0; e < left->enemy_count; e++) {
                    CHECK(left->enemies[e].kind == right->enemies[e].kind);
                    CHECK(vlen(position_delta(left->enemies[e].pos, right->enemies[e].pos)) < 1e-9);
                }
            }
            for (int i = 0; i < SC_MAX_CHUNKS; i++)
                if (a->chunks[i].active) {
                    const Chunk *left = &a->chunks[i], *right = world_chunk(b, left->cell);
                    CHECK(right && left->box_count == right->box_count);
                    /* C leaves struct padding unspecified. Compare geometry and
                       materials, not compiler-dependent object representations. */
                    for (size_t j = 0; j < left->box_count; j++) {
                        const Box *x = &left->boxes[j], *y = &right->boxes[j];
                        CHECK(x->min.x == y->min.x && x->min.y == y->min.y && x->min.z == y->min.z);
                        CHECK(x->max.x == y->max.x && x->max.y == y->max.y && x->max.z == y->max.z);
                        CHECK(x->color.r == y->color.r && x->color.g == y->color.g &&
                              x->color.b == y->color.b);
                        CHECK(x->glow == y->glow && x->solid == y->solid);
                    }
                }
        }
        world_destroy(a);
        world_destroy(b);
    }
    free(a);
    free(b);
    puts("PASS generator-7/8 interior, enemy and collision parity across eight seeds");
}
static void persistence(void) {
    Game *g = fresh(), *copy = fresh();
    g->players[0].pos = position_from(v3(0, 4, -98));
    game_join(g);
    CHECK(!world_overlap(&g->world, position_absolute(g->players[1].pos), .34, 1.78));
    CHECK(game_save(g, "exterior-test.sav") && game_load(copy, "exterior-test.sav"));
    CHECK(copy->world.generator == 8 && copy->player_count == 1);
    CHECK(exterior_cabin(&copy->world, position_absolute(copy->players[0].pos)));
    CHECK(exterior_cabin(&copy->world, position_absolute(copy->players[1].pos)));
    game_join(copy);
    CHECK(!world_overlap(&copy->world, position_absolute(copy->players[1].pos), .34, 1.78));
    g->world.generator = 7;
    g->players[0].pos = position_from(v3(4.2, 4, -98));
    CHECK(game_save(g, "exterior-test.sav") && game_load(copy, "exterior-test.sav"));
    CHECK(copy->world.generator == 7);
    CHECK(position_absolute(copy->players[0].pos).x == 4.2);
    CHECK(!world_overlap(&copy->world, position_absolute(copy->players[0].pos), .34, 1.78));
    CHECK(game_load(copy, SC_FIXTURE_DIR "/phase-midship.sav"));
    CHECK(copy->world.generator == 7 && campaign_phase(&copy->campaign) == 2);
    game_new_ship(copy, 713);
    CHECK(copy->world.generator == 8 &&
          !vlen(position_delta(copy->players[0].pos, position_from(sc_arrival_spawn))));
    remove("exterior-test.sav");
    discard(copy);
    discard(g);
    puts("PASS cabin save/resume, co-op and immutable phased generator-7 save");
}
static void tick(Game *g, Actions action) {
    Vec3 at[2] = {position_absolute(g->players[0].pos), position_absolute(g->players[1].pos)};
    CHECK(world_refresh(&g->world, at, g->player_count));
    world_stream(&g->world, SC_STREAM_BUDGET);
    Actions actions[2] = {action, {0}};
    game_step(g, actions, 1.0 / 60);
    CHECK(!g->world.failed && g->world.last_work <= 2 && g->world.chunk_count <= SC_MAX_CHUNKS);
}
static void travel(Game *g, int axis, double target, Actions action) {
    for (int i = 0; i < 1500; i++) {
        Vec3 p = position_absolute(g->players[0].pos);
        double at = axis == 0 ? p.x : axis == 1 ? p.y : p.z;
        if (fabs(at - target) < .24)
            return;
        tick(g, action);
    }
    CHECK(!"arrival route exceeded its movement budget");
}
static void journey(void) {
    Game *g = fresh();
    game_look(&g->players[0], 0, .04);
    travel(g, 2, -98, (Actions){.forward = -1});
    CHECK(exterior_cabin(&g->world, position_absolute(g->players[0].pos)));
    unsigned weapons = g->players[0].weapons, credits = g->players[0].credits;
    game_interact(g, 0);
    CHECK(g->players[0].weapons == weapons && g->players[0].credits == credits &&
          !g->players[0].sheltered);
    for (int i = 0; i < 100; i++)
        tick(g, (Actions){.strafe = 1});
    CHECK(position_absolute(g->players[0].pos).x < 3.7);
    travel(g, 0, 0, (Actions){.strafe = -1});
    for (int i = 0; i < 100; i++)
        tick(g, (Actions){.vertical = 1});
    CHECK(position_absolute(g->players[0].pos).y < 6.53);
    travel(g, 1, 4, (Actions){.vertical = -1});
    travel(g, 2, 12, (Actions){.forward = 1});
    for (int i = 0; i < 90; i++)
        tick(g, (Actions){0});
    CHECK(g->players[0].grounded && position_absolute(g->players[0].pos).y < .1);
    travel(g, 2, -84, (Actions){.forward = -1});
    travel(g, 1, 4, (Actions){.vertical = 1});
    travel(g, 2, -98, (Actions){.forward = -1});
    CHECK(exterior_cabin(&g->world, position_absolute(g->players[0].pos)));
    CHECK(game_save(g, "exterior-journey.sav") && game_load(g, "exterior-journey.sav"));
    game_join(g);
    CHECK(exterior_cabin(&g->world, position_absolute(g->players[1].pos)));
    travel(g, 2, -84, (Actions){.forward = 1});
    CHECK(!exterior_cabin(&g->world, position_absolute(g->players[0].pos)));
    CHECK(exterior_cabin(&g->world, position_absolute(g->players[1].pos)));
    CHECK(g->players[0].health == 100 && g->players[0].weapons == weapons &&
          g->players[0].credits == credits);
    remove("exterior-journey.sav");
    discard(g);
    puts("PASS controls-only skiff/cabin/bay/return journey, wall and ceiling blocking, "
         "save/resume and co-op");
}
int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--journey"))
        journey();
    else {
        sky_visibility();
        cabin_geometry();
        interior_parity();
        persistence();
    }
    return 0;
}
