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
static int journey(unsigned seed, bool port, const char *fixtures, unsigned profile) {
    Game *g = malloc(sizeof(*g));
    if (!g)
        return 1;
    game_init(g, seed);
    Vec3 waypoint = {0};
    bool have = false, resumed = false, pending = false, attempt = false;
    Cell previous = {999, 999, 999};
    unsigned previous_respawns = 0, previous_defeated = 0;
    uint32_t aim_target = 0;
    double reaction = 0, stalled = 0, max_stall = 0;
    unsigned recovery = 0, recovery_ticks = 0, damage_frames = 0;
    float min_health = 100, min_shield = 50, min_energy = 100;
    double health_loss = 0, shield_loss = 0, armor_loss = 0;
    Vec3 progress_at = position_absolute(g->players[0].pos);
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
        bool introduction = profile == 3 && phase == 1 && !(p->intro & INTRO_GRID);
        if (introduction) {
            goal = (Cell){4, 0, 2};
            if (g->matrix.divers[0].active) {
                game_interact(g, 0);
                if (!(p->intro & INTRO_GRID)) {
                    fprintf(stderr, "Introductory microgrid operation failed\n");
                    return 1;
                }
                have = false;
                continue;
            }
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
            PLAYER_VIEW(g, p).journal =
                false; /* Closing the player's journal; no simulation mutation. */
            if (phase == 2 && !resumed) {
                if (!game_save(g, "phase-journey.sav") || !game_load(g, "phase-journey.sav")) {
                    fprintf(stderr,
                            "Resume refused: phase=%d story=%d matrix=%d scene=%u mission=%u\n",
                            campaign_valid(&g->campaign, g->players, &g->world),
                            story_valid(&g->story, seed, g->world.generator, g->mission),
                            matrix_valid(&g->matrix, seed, SC_GENERATOR_VERSION, g->players,
                                         &g->shelter),
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
            int attr = profile == 2 && p->vitality < 4        ? 1
                       : p->power < (profile == 2 ? 8u : 10u) ? 0
                       : p->vitality < 8                      ? 1
                       : profile == 2                         ? 4
                                                              : 3;
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
            Navigation nav;
            if (profile && game_navigation_to(g, 0, goal, &nav) && nav.kind != NAV_BLOCKED)
                waypoint = nav.point;
            else if (cell.y != goal.y)
                waypoint = world_lift(cell);
            else if (cell.x == goal.x && cell.z == goal.z)
                waypoint = game_terminal(goal);
            else
                waypoint = world_center(next_cell(g, cell, goal));
            have = true;
        }
        if (introduction && cell_equal(cell, goal))
            waypoint = vadd(matrix_terminal(goal), v3(0, 0, -2.5));
        Vec3 diff = vsub(waypoint, feet);
        diff.y = 0;
        double distance = vlen(diff);
        if (active < 0 && distance < .65) {
            have = false;
            if (cell.y != goal.y) {
                actions[0].lift_up = !p->lift_latched && cell.y < goal.y;
                actions[0].lift_down = !p->lift_latched && cell.y > goal.y;
            } else if (cell.x == goal.x && cell.z == goal.z) {
                unsigned intro_before = p->intro;
                game_interact(g, 0);
                if (introduction && intro_before != p->intro && !capture(g, fixtures, "interface"))
                    return 1;
                if (PLAYER_INTERACTION(g, p).decision_open) {
                    if (!capture(g, fixtures, "decision"))
                        return 1;
                    if (!game_story_choose(g, 0, 1))
                        return 1;
                }
            }
        }
        Vec3 aim = vnorm(diff);
        double nearest = 70;
        uint32_t candidate = 0;
        for (int r = 0; r < g->world.count; r++)
            for (int e = 0; e < g->world.rooms[r].enemy_count; e++) {
                Room *room = &g->world.rooms[r];
                Enemy *enemy = &room->enemies[e];
                int guardian = campaign_guardian(room->cell);
                if (!enemy->alive || encounter_passive(room, e) ||
                    (guardian >= 0 && !g->campaign.attempts[guardian].active))
                    continue;
                Vec3 delta = vsub(enemy_center(enemy), player_eye(p));
                double range = vlen(delta);
                if (range < nearest &&
                    world_raycast(&g->world, player_eye(p), vnorm(delta), range) >= range - .1) {
                    nearest = range;
                    aim = vnorm(delta);
                    actions[0].fire = true;
                    candidate = world_hash(seed, room->cell, enemy->id + 1);
                }
            }
        double yaw_delta = atan2(aim.x, aim.z) - p->yaw;
        yaw_delta = atan2(sin(yaw_delta), cos(yaw_delta));
        double pitch_delta = asin(clampd(aim.y, -1, 1)) - p->pitch;
        if (profile) {
            if (candidate != aim_target) {
                aim_target = candidate;
                reaction = profile == 2 ? .55 : .3;
            }
            reaction = fmax(0, reaction - 1.0 / 60);
            if (candidate) {
                double error = profile == 2 ? .04 : .025;
                yaw_delta += error * sin(g->time * 3.1 + candidate % 17);
                pitch_delta += error * cos(g->time * 2.7);
            }
            actions[0].fire &= reaction <= 0 && fabs(yaw_delta) < .16;
            double limit = (profile == 2 ? 1.8 : 2.8) / 60;
            yaw_delta = clampd(yaw_delta, -limit, limit);
            pitch_delta = clampd(pitch_delta, -limit, limit);
        }
        game_look(p, yaw_delta, pitch_delta);
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
        if (profile) {
            if (vlen(vsub(feet, progress_at)) > 1 || p->lift_active || distance < .65) {
                progress_at = feet;
                stalled = 0;
            } else if (distance > .65)
                stalled += 1.0 / 60;
            max_stall = fmax(max_stall, stalled);
            if (stalled >= 5) {
                recovery++;
                recovery_ticks = 120;
                stalled = 0;
                have = false;
            }
            if (recovery_ticks) {
                recovery_ticks--;
                actions[0].strafe = recovery % 2 ? 1 : -1;
                actions[0].forward = .3f;
                actions[0].jump = true;
                actions[0].crouch = false;
                have = false;
            }
        }
        float health = p->health, shield = p->shield, armor = p->armor;
        game_step(g, actions, 1.0 / 60);
        min_health = fminf(min_health, p->health);
        min_shield = fminf(min_shield, p->shield);
        min_energy = fminf(min_energy, p->energy);
        health_loss += fmaxf(0, health - p->health);
        shield_loss += fmaxf(0, shield - p->shield);
        armor_loss += fmaxf(0, armor - p->armor);
        damage_frames += p->health < health || p->shield < shield || p->armor < armor;
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
              (profile != 3 || g->players[0].intro == (INTRO_INTERFACE | INTRO_GRID)) &&
              campaign_valid(&g->campaign, g->players, &g->world);
    printf("Phase journey %u %s: %s, %.1fs, %u kills, %u deaths, %u frames\n", seed,
           port ? "port" : "starboard", ok ? "PASS" : "FAIL", g->time, g->players[0].kills,
           g->respawns, frames);
    if (profile)
        printf("PROFILE {\"seed\":%u,\"profile\":%u,\"passed\":%s,\"simulated_seconds\":%.2f,"
               "\"reaction_seconds\":%.2f,\"turn_rad_s\":%.1f,\"aim_error_rad\":%.3f,"
               "\"kills\":%u,\"deaths\":%u,\"shots\":%u,\"hits\":%u,"
               "\"min_health\":%.2f,\"min_shield\":%.2f,\"min_energy\":%.2f,"
               "\"health_loss\":%.2f,\"shield_loss\":%.2f,\"armor_loss\":%.2f,"
               "\"damage_frames\":%u,\"recoveries\":%u,\"max_stall_seconds\":%.2f}\n",
               seed, profile, ok ? "true" : "false", g->time, profile == 2 ? .55 : .3,
               profile == 2 ? 1.8 : 2.8, profile == 2 ? .04 : .025, g->players[0].kills,
               g->respawns, g->shots, g->hits, min_health, min_shield, min_energy, health_loss,
               shield_loss, armor_loss, damage_frames, recovery, max_stall);
    if (ok) {
        unsigned rank = g->players[0].level, claimed = g->campaign.profiles[0].claimed;
        if (!game_save(g, "phase-journey.sav") || !game_load(g, "phase-journey.sav") ||
            g->players[0].level != rank || g->campaign.profiles[0].claimed != claimed)
            ok = false;
        remove("phase-journey.sav");
        game_new_ship(g, seed);
        ok = ok && g->players[0].level == 1 && g->players[0].weapons == 1 && !g->campaign.defeated;
    }
    game_destroy(g);
    free(g);
    return ok ? 0 : 1;
}
int main(int argc, char **argv) {
    if (argc >= 2 && !strcmp(argv[1], "--onboarding"))
        return journey(713, false, argc == 3 ? argv[2] : NULL, 3);
    if (argc == 3 && !strcmp(argv[1], "--case")) {
        static const struct {
            const char *name;
            unsigned seed;
            bool port;
            unsigned profile;
        } cases[] = {{"starboard", 713, false, 0},
                     {"port", 42, true, 0},
                     {"starboard-responsive", 713, false, 1},
                     {"starboard-cautious", 713, false, 2},
                     {"port-responsive", 42, true, 1},
                     {"port-cautious", 42, true, 2}};
        for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
            if (!strcmp(argv[2], cases[i].name))
                return journey(cases[i].seed, cases[i].port, NULL, cases[i].profile);
        fprintf(stderr, "Unknown expedition case: %s\n", argv[2]);
        return 2;
    }
    if (argc == 2 && !strcmp(argv[1], "--profiles"))
        return journey(713, false, NULL, 1) || journey(713, false, NULL, 2) ||
               journey(42, true, NULL, 1) || journey(42, true, NULL, 2);
    const char *fixtures = argc == 3 && !strcmp(argv[1], "--fixtures") ? argv[2] : NULL;
    if (journey(713, false, fixtures, 0))
        return 1;
    return fixtures ? 0 : journey(42, true, NULL, 0);
}
