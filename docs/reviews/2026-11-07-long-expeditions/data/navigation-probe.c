#include "game.h"
#include <stdio.h>
#include <stdlib.h>

/* Independent normal-input escape trials from one unchanged earned snapshot. */
int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    for (unsigned trial = 0; trial < 8; trial++) {
        Game *g = calloc(1, sizeof(*g));
        if (!g)
            return 2;
        game_init(g, 1);
        if (!game_load(g, argv[1]))
            return 3;
        while (g->world.pending && !g->world.failed)
            world_stream(&g->world, SC_STREAM_BUDGET);
        Player *p = &g->players[0];
        Vec3 initial = position_absolute(p->pos);
        bool blocked = world_overlap(&g->world, initial, .34, p->crouched ? 1 : 1.78);
        Vec3 direction = v3(sin(trial * SC_PI / 4), 0, cos(trial * SC_PI / 4));
        Actions actions[2] = {0};
        actions[0].forward = (float)vdot(direction, v3(sin(p->yaw), 0, cos(p->yaw)));
        actions[0].strafe = (float)vdot(direction, v3(cos(p->yaw), 0, -sin(p->yaw)));
        for (unsigned tick = 0; tick < 240; tick++)
            game_step(g, actions, 1.0 / 60);
        Vec3 end = position_absolute(p->pos);
        printf("{\"trial\":%u,\"seconds\":4,\"initial_overlap\":%s,\"from\":[%.3f,%.3f,%.3f],"
               "\"to\":[%.3f,%.3f,%.3f],\"displacement\":%.3f,\"deaths\":%u}\n",
               trial, blocked ? "true" : "false", initial.x, initial.y, initial.z, end.x, end.y,
               end.z, vlen(vsub(end, initial)), g->respawns);
        game_destroy(g);
        free(g);
    }
    return 0;
}
