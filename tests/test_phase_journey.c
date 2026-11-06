#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A player driver: no position writes, damage calls, equipment grants or enemy
   removal. Plans only read the generator; all progress uses ordinary controls. */
static Cell next_cell(Game *g, Cell from, Cell goal) {
    enum { WIDTH = 32, DEPTH = 20, N = WIDTH * DEPTH };
    int parent[N], queue[N], head = 0, tail = 0;
    static uint32_t seed, generator;
    static int deck = -1000;
    static uint8_t doors[N];
    if (seed != g->world.seed || generator != g->world.generator || deck != from.y) {
        for (int rz = 0; rz < DEPTH / 4; rz++)
            for (int rx = -4; rx < 4; rx++) {
                RegionPlan plan;
                world_plan_version(g->world.seed, g->world.generator, (Cell){rx, from.y, rz},
                                   &plan);
                for (int z = 0; z < 4; z++)
                    for (int x = 0; x < 4; x++)
                        doors[(rz * 4 + z) * WIDTH + rx * 4 + x + 16] = plan.doors[z * 4 + x];
            }
        seed = g->world.seed;
        generator = g->world.generator;
        deck = from.y;
    }
    for (int i = 0; i < N; i++)
        parent[i] = -1;
    int start = from.z * WIDTH + from.x + 16, end = goal.z * WIDTH + goal.x + 16;
    if (start < 0 || start >= N || end < 0 || end >= N)
        return from;
    queue[tail++] = start;
    parent[start] = start;
    while (head < tail && parent[end] < 0) {
        int at = queue[head++];
        Cell c = {at % WIDTH - 16, from.y, at / WIDTH};
        for (int d = 0; d < 4; d++) {
            Cell n = world_neighbor(c, (Direction)d);
            if (n.x < -16 || n.x >= 16 || n.z < 0 || n.z >= DEPTH || !(doors[at] & (1u << d)) ||
                (g->world.open_layer && campaign_layer(n) > g->world.open_layer))
                continue;
            int ni = n.z * WIDTH + n.x + 16;
            if (parent[ni] >= 0)
                continue;
            parent[ni] = at;
            queue[tail++] = ni;
        }
    }
    if (parent[end] < 0)
        return from;
    int at = end;
    while (at != start && parent[at] != start)
        at = parent[at];
    return (Cell){at % WIDTH - 16, from.y, at / WIDTH};
}
static bool capture(Game *g, const char *directory, const char *name) {
    if (!directory)
        return true;
    char path[512];
    snprintf(path, sizeof(path), "%s/phase-%s.sav", directory, name);
    return game_save(g, path);
}
static int journey(unsigned seed, bool port, const char *fixtures) {
    Game *g = malloc(sizeof(*g));
    if (!g)
        return 1;
    game_init(g, seed);
    Vec3 waypoint = {0};
    bool have = false, resumed = false, pending = false, attempt = false;
    Cell previous = {999, 999, 999};
    unsigned previous_respawns = 0, previous_defeated = 0;
    unsigned frames;
    for (frames = 0;
         frames < 360000 && !(g->story.done & (1u << STORY_EXTRACTION)) && !g->world.failed;
         frames++) {
        Player *p = &g->players[0];
        Vec3 stream_position = position_absolute(p->pos);
        world_refresh(&g->world, &stream_position, 1);
        world_stream(&g->world, SC_STREAM_BUDGET);
        Cell goal = {0};
        unsigned phase = campaign_phase(&g->campaign);
        bool briefing = (g->story.done & 1) != 0;
        if (briefing && !(g->campaign.defeated & (1u << 8)))
            goal = campaign_site(8);
        else if (briefing && phase < 4 &&
                 (phase == 0 || (g->story.assigned & (1u << game_story_target(g)))))
            goal = campaign_site(phase * 2 + port);
        else {
            while (g->campaign.tracked != SC_GUARDIANS)
                campaign_track(g);
            game_objective(g, &goal);
        }
        if (!cell_equal(previous, goal) || previous_respawns != g->respawns)
            have = false;
        previous = goal;
        previous_respawns = g->respawns;
        if (g->campaign.defeated != previous_defeated) {
            printf("Seed %u: phase %u, guardians %x, rank %u, %u kills, %u respawns at %.1fs\n",
                   seed, phase, g->campaign.defeated, p->level, p->kills, g->respawns, g->time);
            fflush(stdout);
            previous_defeated = g->campaign.defeated;
            campaign_journal(g, 0);
            if (!pending) {
                if (!capture(g, fixtures, "reward"))
                    return 1;
                pending = true;
            }
            if (!campaign_choose(g, 0, phase % 3 + 1))
                return 1;
            p->journal = false; /* Closing the player's journal; no simulation mutation. */
            if (phase == 2 && !resumed) {
                if (!game_save(g, "phase-journey.sav") || !game_load(g, "phase-journey.sav")) {
                    fprintf(stderr,
                            "Resume refused: phase=%d story=%d matrix=%d scene=%u mission=%u\n",
                            campaign_valid(&g->campaign, g->players, &g->world),
                            story_valid(&g->story, seed, g->world.generator, g->mission),
                            matrix_valid(&g->matrix, seed, 7, g->players, &g->shelter),
                            g->story.tracked, g->mission);
                    return 1;
                }
                remove("phase-journey.sav");
                resumed = true;
                have = false;
                if (!capture(g, fixtures, "midship"))
                    return 1;
            }
            if (phase == 4 && !capture(g, fixtures, "unbound"))
                return 1;
        }
        while (p->points || p->credits >= 60 + p->power * 25) {
            int attr = p->power < 10 ? 0 : p->vitality < 8 ? 1 : 3;
            if (*player_attribute(p, attr) >= 12 || !game_upgrade(g, 0, attr))
                break;
        }
        /* The basic gun remains equipped even when the second slot earns loot. */
        for (int i = 0; p->weapon != 0 && i < SC_WEAPONS; i++)
            game_weapon(g, 0, 1);
        Vec3 feet = position_absolute(p->pos);
        Cell cell = world_cell(feet);
        Actions actions[2] = {0};
        int active = -1;
        for (unsigned i = 0; i < SC_GUARDIANS; i++)
            if (g->campaign.attempts[i].active)
                active = (int)i;
        if (active >= 0) {
            if (!attempt) {
                if (!capture(g, fixtures, "attempt"))
                    return 1;
                attempt = true;
            }
            /* Keep moving across the locked firing line in the central clear lane. */
            Vec3 center = world_center(campaign_site((unsigned)active));
            waypoint = vadd(center, v3(sin(g->time * .65) * 10, 0, cos(g->time * .65) * 10));
            have = false;
        } else if (!have) {
            if (cell.y != goal.y)
                waypoint = world_lift(cell);
            else if (cell.x == goal.x && cell.z == goal.z)
                waypoint = game_terminal(goal);
            else
                waypoint = world_center(next_cell(g, cell, goal));
            have = true;
        }
        Vec3 diff = vsub(waypoint, feet);
        diff.y = 0;
        double distance = vlen(diff);
        if (active < 0 && distance < .65) {
            have = false;
            if (cell.y != goal.y) {
                actions[0].lift_up = cell.y < goal.y;
                actions[0].lift_down = cell.y > goal.y;
            } else if (cell.x == goal.x && cell.z == goal.z) {
                game_interact(g, 0);
                if (p->decision_open) {
                    if (!capture(g, fixtures, "decision"))
                        return 1;
                    if (!game_story_choose(g, 0, 1))
                        return 1;
                }
            }
        }
        Vec3 aim = vnorm(diff);
        double nearest = 70;
        for (int r = 0; r < g->world.count; r++)
            for (int e = 0; e < g->world.rooms[r].enemy_count; e++) {
                Room *room = &g->world.rooms[r];
                Enemy *enemy = &room->enemies[e];
                int guardian = campaign_guardian(room->cell);
                if (!enemy->alive || (guardian >= 0 && !g->campaign.attempts[guardian].active))
                    continue;
                Vec3 delta = vsub(enemy_center(enemy), player_eye(p));
                double range = vlen(delta);
                if (range < nearest &&
                    world_raycast(&g->world, player_eye(p), vnorm(delta), range) >= range - .1) {
                    nearest = range;
                    aim = vnorm(delta);
                    actions[0].fire = true;
                }
            }
        game_look(p, atan2(aim.x, aim.z) - p->yaw, asin(clampd(aim.y, -1, 1)) - p->pitch);
        Vec3 move = vnorm(diff), forward = v3(sin(p->yaw), 0, cos(p->yaw)),
             right = v3(cos(p->yaw), 0, -sin(p->yaw));
        if (distance > .65) {
            actions[0].forward = (float)vdot(move, forward);
            actions[0].strafe = (float)vdot(move, right);
        }
        actions[0].crouch =
            world_describe_version(seed, g->world.generator, cell).height < 2 ||
            (distance > 1 &&
             world_describe_version(seed, g->world.generator, world_cell(waypoint)).height < 2);
        game_step(g, actions, 1.0 / 60);
        if (g->world.last_work > SC_STREAM_BUDGET || !isfinite(p->health) || !isfinite(p->energy) ||
            g->respawns > 12)
            break;
        if (frames % 24000 == 0) {
            printf("Tick %u: %.1f %.1f %.1f -> [%d,%d,%d], %s\n", frames, feet.x, feet.y, feet.z,
                   goal.x, goal.y, goal.z, g->message);
            fflush(stdout);
        }
    }
    bool ok = (g->story.done & (1u << STORY_EXTRACTION)) && campaign_phase(&g->campaign) == 4 &&
              resumed && g->players[0].kills > 8 &&
              campaign_valid(&g->campaign, g->players, &g->world);
    printf("Phase journey %u %s: %s, %.1fs, %u kills, %u deaths, %u frames\n", seed,
           port ? "port" : "starboard", ok ? "PASS" : "FAIL", g->time, g->players[0].kills,
           g->respawns, frames);
    if (ok) {
        unsigned rank = g->players[0].level, claimed = g->campaign.profiles[0].claimed;
        if (!game_save(g, "phase-journey.sav") || !game_load(g, "phase-journey.sav") ||
            g->players[0].level != rank || g->campaign.profiles[0].claimed != claimed)
            ok = false;
        remove("phase-journey.sav");
        game_new_ship(g, seed);
        ok = ok && g->players[0].level == 1 && g->players[0].weapons == 3 && !g->campaign.defeated;
    }
    game_destroy(g);
    free(g);
    return ok ? 0 : 1;
}
int main(int argc, char **argv) {
    const char *fixtures = argc == 3 && !strcmp(argv[1], "--fixtures") ? argv[2] : NULL;
    if (journey(713, false, fixtures))
        return 1;
    return fixtures ? 0 : journey(42, true, NULL);
}
