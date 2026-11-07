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
static void settle(Game *g) {
    Vec3 at[2] = {position_absolute(g->players[0].pos), position_absolute(g->players[1].pos)};
    CHECK(world_refresh(&g->world, at, g->player_count));
    while (g->world.pending)
        CHECK(world_stream(&g->world, 2) <= 2);
    CHECK(!g->world.failed);
}
static void tick(Game *g, Actions action, int frames) {
    Actions actions[2] = {action, {0}};
    for (int i = 0; i < frames; i++) {
        world_stream(&g->world, 2);
        game_step(g, actions, 1.0 / 60);
    }
}
static void routes(Game *g) {
    g->world.open_layer = 0;
    Navigation nav;
    CHECK(!game_navigation_to(g, -1, (Cell){0}, &nav));
    CHECK(!game_navigation_to(g, 2, (Cell){0}, &nav));
    CHECK(game_navigation_to(g, 0, (Cell){0}, &nav) && nav.kind == NAV_BOARD);
    Player *p = g->players;
    p->sheltered = true;
    CHECK(!game_navigation_to(g, 0, (Cell){0}, &nav));
    p->sheltered = false;
    g->matrix.divers[0].active = true;
    CHECK(!game_navigation_to(g, 0, (Cell){0}, &nav));
    g->matrix.divers[0].active = false;
    unsigned doors = 0, turns = 0;
    for (unsigned seed = 1; seed <= 12; seed++) {
        g->world.seed = seed;
        for (int d = 0; d < 4; d++)
            for (int offset = -5; offset <= 5; offset++) {
                Cell from = {offset, offset % 3, 2}, goal = world_neighbor(from, (Direction)d);
                goal.x += d == EAST ? 6 : d == WEST ? -6 : 0;
                goal.z += d == NORTH ? 6 : 0;
                p->pos = position_from(vadd(world_center(from), v3(8, .02, 8)));
                unsigned generated = g->world.generated;
                uint32_t assigned = g->story.assigned;
                CHECK(game_navigation_to(g, 0, goal, &nav));
                CHECK(nav.searched <= 256 && nav.kind == NAV_DOOR);
                CHECK(g->world.generated == generated && g->story.assigned == assigned);
                Direction step = nav.next.x < from.x   ? WEST
                                 : nav.next.x > from.x ? EAST
                                 : nav.next.z < from.z ? SOUTH
                                                       : NORTH;
                CHECK(world_door_version(seed, g->world.generator, from, step));
                CHECK(
                    world_door_version(seed, g->world.generator, nav.next, (Direction)(step ^ 1)));
                turns += step != d;
                CHECK(fabs(nav.point.x - world_center(from).x) < .1);
                p->pos = position_from(vadd(world_center(from), v3(0, .02, 0)));
                CHECK(game_navigation_to(g, 0, goal, &nav));
                CHECK(vlen(vsub(nav.point, world_center(from))) > 49);
                doors++;
            }
    }
    CHECK(doors > 500 && turns > 0);
    g->world.seed = 713;
    Cell from = {0, 0, 0};
    p->pos = position_from(world_lift(from));
    CHECK(game_navigation_to(g, 0, from, &nav) && nav.kind == NAV_GOAL);
    CHECK(game_navigation_to(g, 0, (Cell){0, 3, 0}, &nav) && nav.kind == NAV_LIFT &&
          nav.next.y == 1);
    CHECK(game_navigation_to(g, 0, (Cell){0, -3, 0}, &nav) && nav.kind == NAV_LIFT &&
          nav.next.y == -1);
    g->world.open_layer = 1;
    p->pos = position_from(world_lift((Cell){0, 1, 0}));
    CHECK(game_navigation_to(g, 0, (Cell){0, 3, 0}, &nav) && nav.kind == NAV_BLOCKED);
    p->pos = position_from(world_center((Cell){3, 0, 0}));
    CHECK(game_navigation_to(g, 0, (Cell){900, 0, 0}, &nav) && nav.kind == NAV_BLOCKED);
    g->world.open_layer = 0;
    p->pos = position_from(world_center((Cell){3000000, -3000000, 3000000}));
    CHECK(game_navigation_to(g, 0, (Cell){9000000, -3000000, 9000000}, &nav));
    CHECK(nav.searched <= 256 && nav.kind == NAV_DOOR);
    for (int side = -1; side <= 1; side += 2)
        for (int up = 0; up <= 1; up++) {
            p->pos = position_from(v3(side * 6, 4, 28));
            CHECK(game_navigation_to(g, 0, (Cell){0, up, 0}, &nav) && nav.kind == NAV_LANDING);
            CHECK(nav.point.z == (up ? 39 : 17));
            p->pos = position_from(v3(side * 6, 8.02, 39));
            CHECK(game_navigation_to(g, 0, (Cell){0, up, 0}, &nav) && nav.kind == NAV_LANDING);
        }
    p->pos = position_from(world_lift((Cell){0}));
    game_navigation_step(g, .4);
    CHECK(p->navigation.kind);
    Navigation saved = p->navigation;
    game_navigation_step(g, .01);
    CHECK(memcmp(&saved, &p->navigation, sizeof(saved)) == 0);
    game_join(g);
    g->players[1].pos = position_from(world_center((Cell){2, 1, 2}));
    game_navigation_step(g, .4);
    CHECK(!cell_equal(p->navigation.from, g->players[1].navigation.from));
    p->sheltered = true;
    game_navigation_step(g, .01);
    CHECK(p->navigation.kind == NAV_NONE && g->players[1].navigation.kind);
    p->sheltered = false;
    game_leave(g);
    puts("PASS reciprocal bounded guidance, detours, floors, phase limits, distant coordinates and "
         "independent views");
}
static void lift(Game *g) {
    game_new_ship(g, 713);
    Player *p = g->players;
    p->pos = position_from(world_lift((Cell){0}));
    settle(g);
    tick(g, (Actions){.lift_up = true}, 600);
    CHECK(!p->lift_active && p->lift_latched && p->pos.sector.y == 1);
    CHECK(p->checkpoint.sector.y == 1);
    tick(g, (Actions){.lift_down = true}, 120);
    CHECK(p->pos.sector.y == 1); /* Changing the held direction is not release. */
    tick(g, (Actions){0}, 1);
    CHECK(!p->lift_latched);
    double yaw = p->yaw, pitch = p->pitch;
    tick(g, (Actions){.lift_down = true}, 190);
    CHECK(!p->lift_active && p->pos.sector.y == 0 && p->arrival_time > 5);
    CHECK(p->yaw == yaw && p->pitch == pitch);
    CHECK(game_save(g, "guidance.sav") && game_load(g, "guidance.sav"));
    CHECK(g->world.generator == 10 && !p->lift_latched && !p->navigation.kind);
    remove("guidance.sav");
    puts("PASS held lift input cannot chain rides; release, return, camera and save compatibility");
}
static void landings(Game *g) {
    game_new_ship(g, 713);
    Player *p = g->players;
    for (int deck = -1; deck <= 1; deck++)
        for (int side = -1; side <= 1; side += 2) {
            Cell cell = {0, deck, 0};
            Vec3 start = vadd(world_center(cell), v3(side * 10, .02, -31));
            p->pos = position_from(start);
            settle(g);
            Navigation nav;
            CHECK(game_navigation_to(g, 0, (Cell){0, deck + 1, 0}, &nav));
            CHECK(nav.kind == NAV_LANDING && nav.point.x == 0 && nav.point.z == 17);
            /* The guide crosses the deck landing before turning into the shaft.
               Its straight approach must remain supported and clear of rails. */
            for (int i = 0; i <= 40; i++) {
                Vec3 at = vadd(start, vmul(vsub(nav.point, start), i / 40.0));
                CHECK(!world_overlap(&g->world, at, .34, 1.78));
                CHECK(world_overlap(&g->world, vadd(at, v3(0, -.06, 0)), .34, 1.78));
            }
            p->pos = position_from(nav.point);
            CHECK(game_navigation_to(g, 0, (Cell){0, deck + 1, 0}, &nav));
            CHECK(nav.kind == NAV_LIFT || nav.kind == NAV_BLOCKED);
        }
    puts("PASS upper stair/ramp exit guidance follows supported landings around railings on both "
         "sides and negative decks");
}
int main(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    routes(g);
    lift(g);
    landings(g);
    game_destroy(g);
    free(g);
    return 0;
}
