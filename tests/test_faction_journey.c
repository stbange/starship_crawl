#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef SC_FIXTURE_DIR
#define SC_FIXTURE_DIR "tests/fixtures"
#endif
/* Gameplay-earned starting checkpoint. Only look, movement, fire and save/import
   actions mutate the expedition below. No actor, resource or character setters. */
static bool save_failed;
static bool snapshot(Game *g, const char *folder, const char *name) {
    if (!folder)
        return true;
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.sav", folder, name);
    bool ok = game_save(g, path);
    save_failed |= !ok;
    return ok;
}
int main(int argc, char **argv) {
    Game *g = calloc(1, sizeof(*g));
    if (!g)
        return 2;
    game_init(g, 713);
    const char *path = argc > 1 ? argv[1] : SC_FIXTURE_DIR "/hidden-convoy.sav";
    const char *folder = argc > 2 && *argv[2] ? argv[2] : NULL;
    bool convoy = argc > 3 && !strcmp(argv[3], "--convoy");
    if (!game_load(g, path)) {
        fprintf(stderr, "Cannot import earned convoy checkpoint: %s\n", path);
        return 2;
    }
    uint32_t id = 0;
    double nearest = 1e9;
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        FactionForce *a = &g->factions.forces[i];
        if (!a->id || (convoy ? !a->cargo : a->order != ORDER_RAID) || !factions_strength(a) ||
            cell_equal(world_region(a->home), world_region(a->goal)))
            continue;
        double distance = vlen(position_delta(a->members[0].pos, g->players[0].pos));
        if (distance < nearest) {
            nearest = distance;
            id = a->id;
        }
    }
    if (!id) {
        fprintf(stderr, "Checkpoint has no cross-sector raid\n");
        return 2;
    }
    printf("FOLLOW force=%u distance=%.0f\n", id, nearest);
    fflush(stdout);
    unsigned phase = 0, dead = 0, observed = 0, steps = 0, casualties = g->factions.casualties,
             initial_dead = 0;
    const FactionForce *selected = factions_force(g, id);
    for (unsigned m = 0; m < selected->count; m++)
        if (!selected->members[m].alive)
            initial_dead |= 1u << m;
    Cell last = {INT32_MAX, 0, 0}, interruption = {0}, away = {0}, changed = {0};
    bool have_changed = false;
    Vec3 previous = position_absolute(g->players[0].pos);
    double stalled = 0, nav_until = 0;
    Navigation nav = {0};
    for (unsigned frame = 0; frame < 90000; frame++) {
        Player *p = &g->players[0];
        Vec3 feet = position_absolute(p->pos);
        if (!world_refresh(&g->world, &feet, 1))
            break;
        world_stream(&g->world, SC_STREAM_BUDGET);
        FactionForce *force = factions_force(g, id);
        unsigned member = 0;
        if (force)
            while (member + 1 < force->count && !force->members[member].alive)
                member++;
        Cell cell = world_cell(feet), goal = phase >= 4   ? changed
                                             : phase == 3 ? interruption
                                                          : away;
        Vec3 leader = force ? position_absolute(force->members[member].pos) : feet;
        if (phase < 2) {
            if (!force || !factions_strength(force)) {
                fprintf(stderr, "Raid ended before interception\n");
                break;
            }
            goal = world_cell(leader);
            if (!cell_equal(goal, last)) {
                steps++;
                last = goal;
            }
            Vec3 ray = vsub(enemy_center(&force->members[member]), player_eye(p));
            if (vlen(vsub(leader, feet)) < (convoy ? 18 : 35) &&
                world_raycast(&g->world, player_eye(p), vnorm(ray), vlen(ray)) >= vlen(ray) - .1)
                observed++;
            if (convoy && observed >= 60) {
                bool valid = snapshot(g, folder, "convoy-close") && !save_failed;
                printf("CONVOY force=%u cargo=%u distance=%.2f observed=%u t=%.2f\n", id,
                       force->cargo, vlen(ray), observed, g->time);
                game_destroy(g);
                free(g);
                return valid ? 0 : 1;
            }
            if (phase == 0 && observed >= 600) {
                phase = 1;
                initial_dead = 0;
                for (unsigned m = 0; m < force->count; m++)
                    if (!force->members[m].alive)
                        initial_dead |= 1u << m;
                snapshot(g, folder, "followed-raid");
                printf("OBSERVED force=%u traversed_cells=%u order=%u t=%.2f\n", id, steps,
                       force->order, g->time);
                fflush(stdout);
            }
        }
        if (phase == 1 && force) {
            unsigned casualties_mask = 0;
            for (unsigned m = 0; m < force->count; m++)
                if (!force->members[m].alive)
                    casualties_mask |= 1u << m;
            if (casualties_mask != initial_dead && force->participation[0]) {
                dead = casualties_mask;
                phase = 2;
                interruption = cell;
                away = cell;
                away.x += 3;
                away.z += 2;
                snapshot(g, folder, "interrupted-raid");
                printf("INTERRUPTED force=%u dead_mask=%u t=%.2f\n", id, dead, g->time);
                fflush(stdout);
            }
        }
        if (phase == 2 && vlen(vsub(feet, world_center(interruption))) > 240) {
            phase = 3;
            nav_until = 0;
            snapshot(g, folder, "left-battle");
        }
        if (phase == 3 && cell_equal(cell, interruption) &&
            vlen(vsub(feet, game_terminal(interruption))) < 4) {
            if (force)
                for (unsigned m = 0; m < force->count; m++)
                    if ((dead & (1u << m)) && force->members[m].alive) {
                        fprintf(stderr, "A killed force member returned\n");
                        game_destroy(g);
                        free(g);
                        return 1;
                    }
            snapshot(g, folder, "revisited-battle");
            printf("REVISITED retained_casualties=%u t=%.2f\n", g->factions.casualties - casualties,
                   g->time);
            fflush(stdout);
            double best = 1e9;
            for (size_t i = 0; i < g->factions.count; i++) {
                const FactionRegion *r = &g->factions.regions[i];
                double distance = vlen(vsub(world_center(r->site), feet));
                if ((r->flags & FS_CAPTURED) && distance < best) {
                    best = distance;
                    changed = r->site;
                    have_changed = true;
                }
            }
            phase = have_changed ? 4 : 5;
            if (!have_changed)
                changed = (Cell){0};
            nav_until = 0;
        }
        if (phase >= 4 && cell_equal(cell, changed) &&
            vlen(vsub(feet, game_terminal(changed))) < 4) {
            if (phase == 4) {
                snapshot(g, folder, "occupied-station");
                changed = (Cell){0};
                phase = 5;
                nav_until = 0;
            } else {
                bool valid = factions_valid(&g->factions, 14) &&
                             g->factions.casualties > casualties && !save_failed &&
                             snapshot(g, folder, "returned-bay");
                printf("RETURNED observed_frames=%u force_cells=%u changed_station=%u valid=%u "
                       "t=%.2f\n",
                       observed, steps, have_changed, valid, g->time);
                game_destroy(g);
                free(g);
                return valid ? 0 : 1;
            }
        }
        if (g->time >= nav_until || !cell_equal(goal, nav.goal) || !cell_equal(cell, nav.from)) {
            game_navigation_to(g, 0, goal, &nav);
            nav_until = g->time + .35;
        }
        Vec3 waypoint = nav.point;
        if (phase < 2 && cell_equal(cell, goal)) {
            waypoint = leader;
            if (vlen(vsub(waypoint, feet)) < 12)
                waypoint = feet;
        }
        Vec3 delta = vsub(waypoint, feet);
        delta.y = 0;
        Vec3 aim = vnorm(delta);
        Enemy *target = NULL;
        double best = 40;
        for (int r = 0; r < g->world.count; r++)
            for (int e = 0; e < g->world.rooms[r].enemy_count; e++) {
                Room *room = &g->world.rooms[r];
                Enemy *enemy = &room->enemies[e];
                if (!enemy->alive || enemy->allegiance || encounter_passive(room, e) ||
                    campaign_guardian(room->cell) >= 0 ||
                    (!(phase == 1 && enemy->force == id) && !politics_threat(g, enemy)))
                    continue;
                Vec3 ray = vsub(enemy_center(enemy), player_eye(p));
                double distance = vlen(ray);
                if (distance < best && world_raycast(&g->world, player_eye(p), vnorm(ray),
                                                     distance) >= distance - .1) {
                    target = enemy;
                    best = distance;
                    aim = vnorm(ray);
                }
            }
        double yaw = atan2(aim.x, aim.z) - p->yaw;
        yaw = atan2(sin(yaw), cos(yaw));
        double pitch = atan2(aim.y, hypot(aim.x, aim.z)) - p->pitch;
        game_look(p, clampd(yaw, -.05, .05), clampd(pitch, -.05, .05));
        Actions input[2] = {0};
        input[0].fire = target && fabs(yaw) < .12;
        if (vlen(delta) > .7) {
            Vec3 direction = vnorm(delta);
            input[0].forward = (float)vdot(direction, v3(sin(p->yaw), 0, cos(p->yaw)));
            input[0].strafe = (float)vdot(direction, v3(cos(p->yaw), 0, -sin(p->yaw)));
        }
        input[0].sprint = true;
        input[0].crouch =
            world_describe_version(g->world.seed, 14, world_cell(waypoint)).height < 2;
        if (nav.kind == NAV_LIFT && vlen(delta) < 1) {
            input[0].lift_up = !p->lift_latched && goal.y > cell.y;
            input[0].lift_down = !p->lift_latched && goal.y < cell.y;
        }
        if (vlen(vsub(feet, previous)) > 1 || vlen(delta) < .7 || p->lift_active) {
            stalled = 0;
            previous = feet;
        } else
            stalled += 1.0 / 60;
        if (stalled > 5) {
            input[0].strafe = sin(g->time) > 0 ? 1 : -1;
            input[0].jump = true;
            nav_until = 0;
        }
        if (frame % 3600 == 0) {
            printf("PROGRESS phase=%u cell=%d,%d,%d goal=%d,%d,%d nav=%u t=%.2f\n", phase, cell.x,
                   cell.y, cell.z, goal.x, goal.y, goal.z, nav.kind, g->time);
            fflush(stdout);
        }
        game_step(g, input, 1.0 / 60);
        if (g->world.failed)
            break;
    }
    fprintf(stderr, "Raid journey incomplete phase=%u at %.2f\n", phase, g->time);
    snapshot(g, folder, "failure");
    game_destroy(g);
    free(g);
    return 1;
}
