#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static Cell next_cell(uint32_t seed, unsigned generator, Cell from, Cell target, bool wide) {
    int width = wide ? 12 : 4, height = wide ? 32 : 4, total = width * height;
    static uint32_t planned_seed, planned_generator;
    static int planned_y, planned_width;
    static uint8_t doors[384];
    if (planned_seed != seed || planned_generator != generator || planned_y != from.y ||
        planned_width != width) {
        for (int rz = 0; rz < height / 4; rz++)
            for (int rx = 0; rx < width / 4; rx++) {
                RegionPlan plan;
                world_plan_version(seed, generator, (Cell){rx, from.y, rz}, &plan);
                for (int z = 0; z < 4; z++)
                    for (int x = 0; x < 4; x++)
                        doors[(rz * 4 + z) * width + rx * 4 + x] = plan.doors[z * 4 + x];
            }
        planned_seed = seed;
        planned_generator = generator;
        planned_y = from.y;
        planned_width = width;
    }
    int parent[384], queue[384], head = 0, tail = 0;
    for (int i = 0; i < total; i++)
        parent[i] = -1;
    int start = from.z * width + from.x, end = target.z * width + target.x;
    if (start < 0 || start >= total || end < 0 || end >= total)
        return from;
    queue[tail++] = start;
    parent[start] = start;
    while (head < tail && parent[end] < 0) {
        int id = queue[head++];
        Cell c = {id % width, from.y, id / width};
        for (int d = 0; d < 4; d++) {
            Cell n = world_neighbor(c, (Direction)d);
            if (n.x < 0 || n.x >= width || n.z < 0 || n.z >= height || !(doors[id] & (1u << d)))
                continue;
            int ni = n.z * width + n.x;
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
    return (Cell){id % width, from.y, id / width};
}
static unsigned saga_kinds = 0, machine_kinds = 0, death_effect_kinds = 0;
static int campaign(uint32_t seed, unsigned choice, const char *fixture, unsigned version) {
    Game *g = malloc(sizeof(*g));
    if (!g)
        return 2;
    game_init(g, seed);
    if (version < 2) {
        world_destroy(&g->world);
        world_init(&g->world, seed);
        g->world.generator = 4;
        g->story = (Story){.version = 1, .assigned = 1};
    }
    if (version == 2) {
        world_destroy(&g->world);
        world_init(&g->world, seed);
        g->world.generator = 5;
        g->story = (Story){.version = 2, .assigned = 1};
    }
    if (!choice)
        g->story = (Story){0};
    Vec3 waypoint = {0};
    bool have = false;
    bool fixture_written = false, resumed = false;
    unsigned lastmission = 0, lastrespawn = 0, lasttracked = 0;
    bool saga = version >= 3;
    uint32_t captured = 0;
    int frame;
    for (frame = 0; frame < (saga ? 240000 : 120000) &&
                    !(saga ? (g->story.chapter >= 2 && (g->story.done & (1u << STORY_RETURN)))
                           : g->mission >= 6) &&
                    !g->world.failed;
         frame++) {
        Player *p = &g->players[0];
        if (saga && frame % 6000 == 0) {
            Vec3 at = position_absolute(p->pos);
            printf("Saga tick %d / chapter %u kind %u / scene %u / position %.1f %.1f %.1f / %s\n",
                   frame, g->story.chapter, g->story.arc, g->story.tracked, at.x, at.y, at.z,
                   g->message);
            fflush(stdout);
        }
        Vec3 feet = position_absolute(p->pos);
        world_refresh(&g->world, &feet, 1);
        world_stream(&g->world, SC_STREAM_BUDGET);
        Cell target;
        if (choice == 3) {
            unsigned op = game_story_operation(g);
            if (game_story_available(g, op))
                while (g->story.tracked != op)
                    game_story_track(g);
        }
        if (saga && !g->story.choice) {
            unsigned todo[] = {STORY_POWER, STORY_BEACON, STORY_CIPHER, STORY_OVERRIDE,
                               STORY_COUNCIL};
            for (unsigned i = 0; i < 5; i++)
                if (game_story_available(g, todo[i]) &&
                    (todo[i] != STORY_COUNCIL || ((g->story.done & (1u << STORY_POWER)) &&
                                                  (g->story.done & (1u << STORY_CIPHER))))) {
                    while (g->story.tracked != todo[i])
                        game_story_track(g);
                    break;
                }
        }
        if (saga && g->story.chapter && (choice == 3 || fixture) &&
            game_story_available(g, STORY_RELAY))
            while (g->story.tracked != STORY_RELAY)
                game_story_track(g);
        game_objective(g, &target);
        Cell cell = world_cell(feet);
        if (g->mission != lastmission || g->respawns != lastrespawn ||
            (saga && lasttracked != g->story.tracked))
            have = false;
        lastmission = g->mission;
        lastrespawn = g->respawns;
        lasttracked = g->story.tracked;
        if (saga && g->story.chapter)
            saga_kinds |= 1u << g->story.arc;
        while (p->points) {
            int upgrade = p->power < 10 ? 0 : 1;
            if (*player_attribute(p, upgrade) >= 50)
                for (int i = 0; i < SC_ATTRIBUTES; i++)
                    if (*player_attribute(p, i) < *player_attribute(p, upgrade))
                        upgrade = i;
            if (*player_attribute(p, upgrade) >= 50 || !game_upgrade(g, 0, upgrade))
                break;
        }
        if (p->weapon != 0)
            game_weapon(g, 0, 1);
        Actions a[2] = {{0}, {0}};
        if (!have) {
            if (cell.y != target.y)
                waypoint = world_lift(cell);
            else if (cell.x == target.x && cell.z == target.z)
                waypoint = game_terminal(target);
            else
                waypoint =
                    world_center(next_cell(g->world.seed, g->world.generator, cell, target, saga));
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
            } else if (cell.x == target.x && cell.z == target.z) {
                if (saga && fixture) {
                    unsigned near = game_story_near(g, 0);
                    unsigned flag = near == STORY_POWER      ? 32
                                    : near == STORY_BEACON   ? 64
                                    : near == STORY_OVERRIDE ? 128
                                                             : 0;
                    if (flag && !(captured & flag)) {
                        const char *name = near == STORY_POWER ? "tests/fixtures/saga-power.sav"
                                           : near == STORY_BEACON
                                               ? "tests/fixtures/saga-beacon.sav"
                                               : "tests/fixtures/saga-controller.sav";
                        if (!game_save(g, name))
                            return 1;
                        captured |= flag;
                    }
                }
                game_interact(g, 0);
                if (choice && p->decision_open) {
                    if (fixture && !saga && !fixture_written) {
                        if (!game_save(g, fixture))
                            return 1;
                        fixture_written = true;
                    }
                    if (saga && fixture) {
                        unsigned scene = g->players[0].decision_scene;
                        unsigned bit = scene == STORY_COUNCIL      ? 1
                                       : scene == STORY_RESOLUTION ? 4
                                                                   : 2;
                        if (!(captured & bit)) {
                            const char *name = scene == STORY_COUNCIL      ? "saga-council.sav"
                                               : scene == STORY_RESOLUTION ? "saga-follow.sav"
                                                                           : "saga-main.sav";
                            char path[160];
                            snprintf(path, sizeof(path), "tests/fixtures/%s", name);
                            if (!game_save(g, path))
                                return 1;
                            captured |= bit;
                        }
                    }
                    game_story_choose(g, 0,
                                      saga && p->decision_scene == STORY_COUNCIL ? 3 : choice);
                }
            }
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
        a[0].crouch = world_describe_version(g->world.seed, g->world.generator, cell).height < 2 ||
                      (distance > 1 && world_describe_version(g->world.seed, g->world.generator,
                                                              world_cell(waypoint))
                                               .height < 2);
        game_step(g, a, 1.0 / 60);
        if (version == 3) {
            for (int i = 0; i < SC_MAX_DEATHS; i++)
                if (g->deaths[i].active && g->deaths[i].body.kind >= 27)
                    machine_kinds |= 1u << (g->deaths[i].body.kind - 27);
            for (int i = 0; i < SC_MAX_DEATH_HAZARDS; i++)
                if (g->death_hazards[i].active)
                    death_effect_kinds |= 1u << g->death_hazards[i].kind;
        }
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
        if (saga && fixture) {
            bool bay = g->story.done & (1u << STORY_EXTRACTION);
            unsigned flag = g->story.chapter ? 16 : 8;
            if (bay && !(captured & flag) &&
                (!g->story.chapter || (g->story.done & (1u << STORY_RETURN)))) {
                if (!game_save(g, g->story.chapter ? "tests/fixtures/saga-extracted.sav"
                                                   : "tests/fixtures/saga-continuation.sav"))
                    return 1;
                captured |= flag;
            }
        }
        if (choice && g->mission == 2 && !resumed) {
            Story before = g->story;
            if (!game_save(g, "sc-story-journey.sav") || !game_load(g, "sc-story-journey.sav") ||
                memcmp(&before, &g->story, sizeof(before)))
                return 1;
            remove("sc-story-journey.sav");
            resumed = true;
            have = false;
        }
    }
    printf(
        "Campaign seed %u: stage %u, %.1fs, %u kills, rank %u, %u respawns, %d frames, failed=%d\n",
        seed, g->mission, g->time, g->players[0].kills, g->players[0].level, g->respawns, frame,
        g->world.failed);
    int result = g->mission == 6 && g->players[0].victories == (saga ? 3u : 1u) &&
                         g->players[0].kills >= 1 && (!choice || g->story.choice == choice) &&
                         (!saga || (g->story.chapter == 2 && g->story.totals[4] == 3 &&
                                    g->players[0].career[4] == 3)) &&
                         !g->world.failed
                     ? 0
                     : 1;
    if (result) {
        Vec3 p = position_absolute(g->players[0].pos);
        Cell target = {0};
        game_objective(g, &target);
        fprintf(stderr,
                "Stuck at %.2f %.2f %.2f, waypoint %.2f %.2f %.2f, target %d %d %d, scene %u\n",
                p.x, p.y, p.z, waypoint.x, waypoint.y, waypoint.z, target.x, target.y, target.z,
                g->story.tracked);
        fprintf(stderr, "Message %s / decision %d\n", g->message, g->players[0].decision_open);
        for (int r = 0; r < g->world.count; r++)
            for (int e = 0; e < g->world.rooms[r].enemy_count; e++) {
                Enemy *enemy = &g->world.rooms[r].enemies[e];
                if (enemy->alive && vlen(vsub(enemy_center(enemy), game_terminal(target))) < 12) {
                    Vec3 at = enemy_center(enemy);
                    fprintf(stderr, "Contesting enemy %u at %.2f %.2f %.2f\n", enemy->kind, at.x,
                            at.y, at.z);
                }
            }
    }
    game_destroy(g);
    free(g);
    return result;
}

int main(int argc, char **argv) {
    if (argc > 1 && (strcmp(argv[1], "--saga") == 0 || strcmp(argv[1], "--saga-fixtures") == 0)) {
        bool fixtures = strcmp(argv[1], "--saga-fixtures") == 0;
        const unsigned seeds[] = {1, 3, 5, 14, 22, 8};
        for (unsigned i = 0; i < 6; i++) {
            printf("Reactive saga seed %u / three extracted arcs\n", seeds[i]);
            if (campaign(seeds[i], i % 3 + 1, fixtures && i == 0 ? "saga" : NULL, 3))
                return 1;
        }
        printf("Follow-on kinds exercised: %x\n", saga_kinds);
        printf("Machine defeats: %x / death hazards: %x\n", machine_kinds, death_effect_kinds);
        return saga_kinds == (1u << STORY_ARCS) - 1 && machine_kinds == 31 &&
                       death_effect_kinds == 14
                   ? 0
                   : 1;
    }
    if (argc > 1 &&
        (strcmp(argv[1], "--history") == 0 || strcmp(argv[1], "--history-fixtures") == 0)) {
        bool fixtures = strcmp(argv[1], "--history-fixtures") == 0;
        for (unsigned motivation = 0; motivation < 4; motivation++)
            for (unsigned arc = 0; arc < (motivation == STORY_CONTAINMENT ? 1u : 2u); arc++) {
                unsigned seed = 1;
                while ((unsigned)story_motivation(seed) != motivation ||
                       world_history(seed).arc != (HistoryArc)arc)
                    seed++;
                for (unsigned choice = 1; choice <= 3; choice++) {
                    char path[120];
                    snprintf(path, sizeof(path), "tests/fixtures/history-%u-arc%u-choice%u.sav",
                             motivation, arc, choice);
                    printf("History expedition: motivation %u, arc %u, choice %u, seed %u\n",
                           motivation, arc, choice, seed);
                    if (campaign(seed, choice,
                                 fixtures && arc == 0 &&
                                         (choice == 3 ||
                                          (choice == 1 && (motivation == STORY_RESCUE ||
                                                           motivation == STORY_CONTAINMENT)))
                                     ? path
                                     : NULL,
                                 2))
                        return 1;
                }
            }
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--story") == 0) {
        for (unsigned motivation = 0; motivation < 3; motivation++) {
            unsigned seed = 1;
            while ((unsigned)story_motivation_version(seed, 1) != motivation)
                seed++;
            for (unsigned choice = 1; choice <= 3; choice++) {
                printf("Legacy generated expedition: motivation %u, choice %u, seed %u\n",
                       motivation, choice, seed);
                if (campaign(seed, choice, NULL, 1))
                    return 1;
            }
        }
        return 0;
    }
    const uint32_t seeds[] = {713, 1, 42};
    for (unsigned i = 0; i < sizeof(seeds) / sizeof(seeds[0]); i++)
        if (campaign(seeds[i], 0, NULL, 0))
            return 1;
    return 0;
}
