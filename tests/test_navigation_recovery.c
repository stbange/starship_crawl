#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static void tick(Game *g, Actions a) {
    Actions actions[2] = {a, {0}};
    world_stream(&g->world, 2);
    game_step(g, actions, 1.0 / 60);
    CHECK(!g->world.failed);
}
static void walk(Game *g, Vec3 target) {
    Player *p = g->players;
    for (int i = 0; i < 1800; i++) {
        Vec3 delta = vsub(target, position_absolute(p->pos));
        delta.y = 0;
        if (vlen(delta) < .3)
            return;
        game_look(p, atan2(delta.x, delta.z) - p->yaw, -p->pitch);
        tick(g, (Actions){.forward = (float)fmin(1, vlen(delta) * 8)});
    }
    Vec3 at = position_absolute(p->pos);
    fprintf(stderr, "Blocked %.2f %.2f %.2f -> %.2f %.2f %.2f\n", at.x, at.y, at.z, target.x,
            target.y, target.z);
    CHECK(false);
}
int main(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    game_init(g, 3);
    CHECK(game_load(g, SC_REVIEW_DIR "/3/phase-minute-24.sav"));
    Player *p = g->players;
    Vec3 start = position_absolute(p->pos);
    CHECK(fabs(start.x + 66.09) < .02 && fabs(start.y + 186.30) < .02);
    Vec3 positions[1] = {start};
    CHECK(world_refresh(&g->world, positions, 1));
    while (g->world.pending)
        world_stream(&g->world, 2);
    CHECK(world_overlap(&g->world, vadd(start, v3(0, -.06, 0)), .34, 1.78));
    game_look(p, -SC_PI / 2 - p->yaw, -p->pitch);
    for (int i = 0; i < 300; i++)
        tick(g, (Actions){.forward = 1});
    CHECK(vlen(position_delta(p->pos, position_from(start))) < .1);
    /* Walk off the connector roof, let gravity settle on the next supported
       deck, then use real passages. No coordinate writes, grants or damage. */
    for (int i = 0; i < 60; i++)
        tick(g, (Actions){.strafe = 1, .forward = .2f, .jump = true});
    for (int i = 0; i < 180; i++)
        tick(g, (Actions){0});
    Vec3 landed = position_absolute(p->pos);
    printf("Seed 3 roof %.2f %.2f %.2f -> gravity descent %.2f %.2f %.2f\n", start.x, start.y,
           start.z, landed.x, landed.y, landed.z);
    CHECK(vlen(vsub(landed, start)) > 5 && g->respawns == 0);
    Cell floor = world_cell(landed);
    Vec3 center = world_center(floor);
    walk(g, vadd(center, v3(landed.x - center.x, .02, 0)));
    walk(g, center);
    walk(g, world_lift(floor));
    for (int i = 0; i < 180; i++)
        tick(g, (Actions){0});
    CHECK(fabs(position_absolute(p->pos).y - world_lift(floor).y) < .1);
    /* A deliberate ride returns to the original deck, below the roof header. */
    while (floor.y < -3) {
        tick(g, (Actions){0});
        for (int i = 0; i < 250; i++)
            tick(g, (Actions){.lift_up = true});
        CHECK(!p->lift_active);
        floor = world_cell(position_absolute(p->pos));
    }
    walk(g, world_center(floor));
    walk(g, v3(-65, -192, 48));
    walk(g, v3(-100, -192, 48));
    CHECK(g->respawns == 0 && fabs(position_absolute(p->pos).y + 192) < .1);
    CHECK(game_save(g, "navigation-recovered.sav") && game_load(g, "navigation-recovered.sav"));
    remove("navigation-recovered.sav");
    puts("PASS archived seed-3 roof stall reproduced, ordinary recovery and doorway crossed both "
         "ways at floor level");
    game_destroy(g);
    free(g);
    return 0;
}
