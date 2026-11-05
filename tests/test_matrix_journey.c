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
static void tick(Game *g, Actions a) {
    world_stream(&g->world, 2);
    game_step(g, (Actions[2]){a, {0}}, 1.0 / 60);
}
static MatrixNetwork *net(Game *g) {
    return &g->matrix.networks[g->matrix.divers[0].network];
}
static void aim(Game *g, Vec3 at, bool matrix) {
    Vec3 delta = vsub(at, matrix ? matrix_eye(g, 0) : player_eye(&g->players[0]));
    game_look(&g->players[0], atan2(delta.x, delta.z) - g->players[0].yaw,
              atan2(delta.y, hypot(delta.x, delta.z)) - g->players[0].pitch);
}
static bool fight(Game *g) {
    MatrixNetwork *w = net(g);
    Vec3 eye = matrix_eye(g, 0);
    int target = -1;
    double nearest = 25;
    for (int i = 0; i < SC_MATRIX_GUARDS; i++) {
        MatrixGuard *e = &w->guards[i];
        double dist = vlen(vsub(e->pos, eye));
        if (e->active && dist < nearest &&
            matrix_raycast(w, &g->matrix.records[w->record], eye, vnorm(vsub(e->pos, eye)), dist) >=
                dist - .1) {
            nearest = dist;
            target = i;
        }
    }
    if (target < 0)
        return false;
    aim(g, w->guards[target].pos, true);
    return true;
}
static void walk(Game *g, Vec3 target, bool matrix, bool jump) {
    for (int frame = 0; frame < 3600; frame++) {
        Vec3 pos = matrix ? g->matrix.divers[0].pos : position_absolute(g->players[0].pos),
             delta = vsub(target, pos);
        delta.y = 0;
        if (vlen(delta) < .22)
            return;
        Vec3 wish = vnorm(delta);
        bool firing = matrix && fight(g);
        if (!firing)
            aim(g, vadd(pos, vmul(wish, 10)), matrix);
        Vec3 forward = facing(g->players[0].yaw, 0), right = v3(forward.z, 0, -forward.x);
        Actions a = {.forward = (float)vdot(wish, forward),
                     .strafe = (float)vdot(wish, right),
                     .jump = jump,
                     .fire = firing};
        tick(g, a);
        CHECK(!g->world.failed);
        if (matrix)
            CHECK(g->matrix.divers[0].active);
    }
    fprintf(stderr, "Route failed to %.1f,%.1f at %.2f,%.2f\n", target.x, target.z,
            g->matrix.divers[0].pos.x, g->matrix.divers[0].pos.z);
    CHECK(false);
}
static void settle(Game *g, int frames) {
    for (int i = 0; i < frames; i++) {
        Actions a = {.fire = g->matrix.divers[0].active && fight(g)};
        tick(g, a);
    }
}
static void clear_console(Game *g, unsigned node) {
    for (int i = 0; i < 3600 && matrix_guarded(net(g), net(g)->nodes[node].at); i++) {
        bool fire = fight(g);
        tick(g, (Actions){.fire = fire});
        CHECK(g->matrix.divers[0].active);
    }
    CHECK(!matrix_guarded(net(g), net(g)->nodes[node].at));
    settle(g, 60);
}
static void route(Game *g, unsigned *current, unsigned to) {
    MatrixNetwork *w = net(g);
    int parent[16];
    for (int i = 0; i < 16; i++)
        parent[i] = -1;
    unsigned queue[16], head = 0, tail = 1;
    queue[0] = *current;
    parent[*current] = (int)*current;
    while (head < tail) {
        unsigned a = queue[head++];
        for (unsigned b = 0; b < 16; b++)
            if ((w->nodes[a].links & (1u << b)) && parent[b] < 0) {
                parent[b] = (int)a;
                queue[tail++] = b;
            }
    }
    CHECK(parent[to] >= 0);
    unsigned path[16], count = 0;
    for (unsigned at = to; at != *current; at = (unsigned)parent[at])
        path[count++] = at;
    while (count) {
        unsigned next = path[--count];
        Vec3 a = w->nodes[*current].at, b = w->nodes[next].at;
        /* Geometry publishes horizontal-then-vertical bridges in ascending node order. */
        Vec3 elbow = *current < next ? v3(b.x, 0, a.z) : v3(a.x, 0, b.z);
        walk(g, a, true, false);
        walk(g, elbow, true, true);
        walk(g, b, true, true);
        *current = next;
        clear_console(g, next);
    }
}
static const char *checkpoint_dir;
static void run(unsigned seed, unsigned choice) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, seed);
    for (int i = 0; i < 2400 && position_absolute(g->players[0].pos).z < 12; i++)
        tick(g, (Actions){.forward = 1});
    CHECK(position_absolute(g->players[0].pos).z >= 12);
    settle(g, 90);
    walk(g, game_terminal((Cell){0}), false, false);
    game_interact(g, 0);
    CHECK(g->story.done & 1);
    walk(g, matrix_terminal((Cell){0}), false, false);
    settle(g, 20);
    Position body = g->players[0].pos;
    double yaw = g->players[0].yaw;
    CHECK(matrix_toggle(g, 0));
    MatrixNetwork *w = net(g);
    MatrixRecord *r = &g->matrix.records[w->record];
    unsigned current = w->entry;
    walk(g, w->nodes[current].at, true, false);
    CHECK(matrix_service(g, 0));
    CHECK(world_systems(&g->world, (Cell){0}) & SYSTEM_POWER);
    route(g, &current, w->credential);
    walk(g, vadd(w->nodes[current].at, v3(3, 0, 0)), true, false);
    game_interact(g, 0);
    CHECK(r->keys);
    route(g, &current, w->entry);
    walk(g, vadd(w->nodes[current].at, v3(-3, 0, 0)), true, false);
    game_interact(g, 0);
    CHECK(w->switches[0] > 0);
    route(g, &current, w->switch_b);
    walk(g, vadd(w->nodes[current].at, v3(-3, 0, 0)), true, false);
    game_interact(g, 0);
    CHECK(r->flags & MX_ROUTED);
    if (choice == 3 && w->rare) {
        route(g, &current, w->vault);
        walk(g, vadd(w->nodes[current].at, v3(-3, 0, -3)), true, false);
        game_interact(g, 0);
        CHECK(r->flags & MX_VAULT);
    }
    route(g, &current, w->core);
    walk(g, vadd(w->nodes[current].at, v3(0, 0, -2)), true, false);
    settle(g, 60);
    clear_console(g, current);
    if (choice == 3 && !w->rare) {
        while (g->matrix.divers[0].service != MX_ARCHIVE)
            matrix_cycle_service(g, 0, 1);
        CHECK(matrix_service(g, 0));
    }
    game_interact(g, 0);
    CHECK(g->matrix.divers[0].choice);
    if (checkpoint_dir) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/matrix-choice%u.sav", checkpoint_dir, choice);
        CHECK(game_save(g, path));
    }
    CHECK(game_save(g, "matrix-journey.sav"));
    CHECK(game_load(g, "matrix-journey.sav"));
    CHECK(g->matrix.divers[0].choice && g->players[0].journal && g->players[0].journal_page == 4);
    CHECK(matrix_choose(g, 0, choice));
    unsigned data = g->matrix.profiles[0].data;
    CHECK(matrix_toggle(g, 0));
    CHECK(vlen(position_delta(body, g->players[0].pos)) < .001 &&
          fabs(g->players[0].yaw - yaw) < .001);
    CHECK(game_save(g, "matrix-journey.sav") && game_load(g, "matrix-journey.sav"));
    CHECK(g->matrix.records[0].outcome == choice && g->matrix.profiles[0].data >= data);
    printf("PASS matrix journey seed %u / choice %u / shots %u / integrity %.0f / data %u\n", seed,
           choice, g->shots, g->matrix.divers[0].integrity, g->matrix.profiles[0].data);
    game_destroy(g);
    free(g);
    remove("matrix-journey.sav");
}
int main(int argc, char **argv) {
    if (argc == 3 && !strcmp(argv[1], "--checkpoints"))
        checkpoint_dir = argv[2];
    run(713, 1);
    run(42, 2);
    run(1, 3);
    return 0;
}
