#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static Cell next_cell(uint32_t seed, Cell from, Cell target) {
    int parent[16], queue[16], head = 0, tail = 0;
    for (int i = 0; i < 16; i++)
        parent[i] = -1;
    int start = from.z * 4 + from.x, end = target.z * 4 + target.x;
    if (start < 0 || start >= 16 || end < 0 || end >= 16)
        return from;
    queue[tail++] = start;
    parent[start] = start;
    while (head < tail && parent[end] < 0) {
        int id = queue[head++];
        Cell c = {id % 4, from.y, id / 4};
        for (int d = 0; d < 4; d++) {
            Cell n = world_neighbor(c, (Direction)d);
            if (n.x < 0 || n.x > 3 || n.z < 0 || n.z > 3 || !world_door(seed, c, (Direction)d))
                continue;
            int ni = n.z * 4 + n.x;
            if (parent[ni] >= 0)
                continue;
            parent[ni] = id;
            queue[tail++] = ni;
        }
    }
    if (parent[end] < 0)
        return from;
    int id = end;
    while (parent[id] != start && id != start)
        id = parent[id];
    return (Cell){id % 4, from.y, id / 4};
}
static int campaign(uint32_t seed) {
    Game *g = malloc(sizeof(*g));
    if (!g)
        return 2;
    game_init(g, seed);
    Vec3 waypoint = {0};
    bool have = false;
    unsigned lastmission = 0, lastrespawn = 0;
    int frame;
    for (frame = 0; frame < 120000 && g->mission < 6 && !g->world.failed; frame++) {
        Player *p = &g->players[0];
        Vec3 feet = position_absolute(p->pos);
        world_refresh(&g->world, &feet, 1);
        world_stream(&g->world, SC_STREAM_BUDGET);
        Cell target;
        game_objective(g, &target);
        Cell cell = world_cell(feet);
        if (g->mission != lastmission || g->respawns != lastrespawn)
            have = false;
        lastmission = g->mission;
        lastrespawn = g->respawns;
        while (p->points)
            game_upgrade(g, 0, p->power < 10 ? 0 : 1);
        if (p->weapon != 0)
            game_weapon(g, 0, 1);
        Actions a[2] = {{0}, {0}};
        if (!have) {
            if (cell.y != target.y)
                waypoint = world_lift(cell);
            else if (cell.x == target.x && cell.z == target.z)
                waypoint = game_terminal(target);
            else
                waypoint = world_center(next_cell(g->world.seed, cell, target));
            have = true;
        }
        Vec3 diff = vsub(waypoint, feet);
        diff.y = 0;
        double distance = vlen(diff);
        if (distance < .65) {
            have = false;
            if (cell.y != target.y) {
                a[0].lift_up = cell.y < target.y;
                a[0].lift_down = cell.y > target.y;
            } else if (cell.x == target.x && cell.z == target.z)
                game_interact(g, 0);
        }
        Vec3 aim = vnorm(diff);
        aim.y = 0;
        double near = 65;
        for (int r = 0; r < g->world.count; r++)
            for (int e = 0; e < g->world.rooms[r].enemy_count; e++) {
                Enemy *enemy = &g->world.rooms[r].enemies[e];
                if (!enemy->alive)
                    continue;
                Vec3 delta = vsub(enemy_center(enemy), player_eye(p));
                double range = vlen(delta);
                if (range < near &&
                    world_raycast(&g->world, player_eye(p), vnorm(delta), range) >= range - .1) {
                    near = range;
                    aim = vnorm(delta);
                    a[0].fire = true;
                }
            }
        game_look(p, atan2(aim.x, aim.z) - p->yaw, asin(clampd(aim.y, -1, 1)) - p->pitch);
        Vec3 move = vnorm(diff), forward = v3(sin(p->yaw), 0, cos(p->yaw)),
             right = v3(cos(p->yaw), 0, -sin(p->yaw));
        if (distance > .65) {
            a[0].forward = (float)vdot(move, forward);
            a[0].strafe = (float)vdot(move, right);
        }
        a[0].crouch =
            world_describe(g->world.seed, cell).height < 2 ||
            (distance > 1 && world_describe(g->world.seed, world_cell(waypoint)).height < 2);
        game_step(g, a, 1.0 / 60);
        if (g->world.last_work > SC_STREAM_BUDGET || g->world.chunk_count > SC_MAX_CHUNKS ||
            !isfinite(p->health) || !isfinite(p->shield) || !isfinite(p->energy) || p->shield < 0 ||
            p->shield > player_max_shield(p) || p->energy < 0 || p->energy > player_max_energy(p)) {
            fprintf(stderr, "Campaign budget/resource invariant failed for seed %u at frame %d\n",
                    seed, frame);
            g->world.failed = true;
        }
        if (g->mission != lastmission) {
            printf("System %u restored at %.1fs / %u kills / %u respawns\n", g->mission, g->time,
                   p->kills, g->respawns);
            fflush(stdout);
        }
    }
    printf(
        "Campaign seed %u: stage %u, %.1fs, %u kills, rank %u, %u respawns, %d frames, failed=%d\n",
        seed, g->mission, g->time, g->players[0].kills, g->players[0].level, g->respawns, frame,
        g->world.failed);
    int result = g->mission == 6 && g->players[0].victories == 1 && g->players[0].kills >= 1 &&
                         !g->world.failed
                     ? 0
                     : 1;
    game_destroy(g);
    free(g);
    return result;
}

int main(void) {
    const uint32_t seeds[] = {713, 1, 42};
    for (unsigned i = 0; i < sizeof(seeds) / sizeof(seeds[0]); i++)
        if (campaign(seeds[i]))
            return 1;
    return 0;
}
