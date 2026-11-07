#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Earned checkpoint, normal movement/look/fire/use only. No position, inventory,
   health or enemy writes. Read-only plans provide the same route as the HUD. */
static bool route(Game *g, Cell goal, bool fight, unsigned profile) {
    const double dt = 1.0 / 60;
    unsigned start_kills = g->players[0].kills, deaths = g->respawns, shots = g->shots;
    unsigned weapon = g->players[0].weapon;
    double elapsed = 0, damage = 0, energy = 100, stalled = 0, reaction = 0;
    unsigned last_target = 0, interrupts = 0, recoveries = 0;
    Vec3 progress = position_absolute(g->players[0].pos);
    for (unsigned frame = 0; frame < 180000; frame++) {
        Player *p = &g->players[0];
        Vec3 feet = position_absolute(p->pos);
        if (!world_refresh(&g->world, &feet, 1))
            return false;
        world_stream(&g->world, SC_STREAM_BUDGET);
        Room *room = world_find(&g->world, goal);
        bool here = room && room->ready && world_find(&g->world, world_cell(feet)) == room;
        if (here && fight && (world_local_state(&g->world, room) & FRONTIER_CLEARED)) {
            if (vlen(vsub(feet, encounter_console(room))) < 2) {
                game_interact(g, 0);
                printf("REGIONAL profile=%u gun=%u cell=%d,%d,%d seconds=%.2f kills=%u deaths=%u "
                       "shots=%u damage=%.2f min_energy=%.2f disrupted_frames=%u recoveries=%u\n",
                       profile, weapon, goal.x, goal.y, goal.z, elapsed, p->kills - start_kills,
                       g->respawns - deaths, g->shots - shots, damage, energy, interrupts,
                       recoveries);
                return (world_local_state(&g->world, room) & FRONTIER_CLAIMED) != 0;
            }
        }
        Navigation nav = {0};
        Vec3 waypoint;
        if (here)
            waypoint = encounter_console(room);
        else if (game_navigation_to(g, 0, goal, &nav) && nav.kind != NAV_BLOCKED)
            waypoint = nav.point;
        else {
            fprintf(stderr, "route blocked %.2f %.2f %.2f toward %d %d %d\n", feet.x, feet.y,
                    feet.z, goal.x, goal.y, goal.z);
            return false;
        }
        Actions actions[2] = {0};
        double range = vlen(vsub(feet, waypoint));
        if (here && range < 2 && !room->frontier_active &&
            !(world_local_state(&g->world, room) & FRONTIER_CLEARED)) {
            if (!fight)
                return true;
            game_interact(g, 0);
        }
        if (here && room->frontier_active) {
            Vec3 center = world_center(goal);
            waypoint = vadd(center, v3(sin(g->time * .6) * 5, 0, 2 + cos(g->time * .6) * 3));
            interrupts += room->frontier_disrupt > 0;
        }
        Vec3 delta = vsub(waypoint, feet);
        delta.y = 0;
        double distance = vlen(delta);
        if (!here && nav.kind == NAV_LIFT && distance < 1) {
            actions[0].lift_up = !p->lift_latched && goal.y > world_cell(feet).y;
            actions[0].lift_down = !p->lift_latched && goal.y < world_cell(feet).y;
        }
        Vec3 aim = vnorm(delta);
        double best = 60;
        unsigned target = 0;
        for (int r = 0; r < g->world.count; r++)
            for (int e = 0; e < g->world.rooms[r].enemy_count; e++) {
                Room *rr = &g->world.rooms[r];
                Enemy *enemy = &rr->enemies[e];
                if (!enemy->alive || encounter_passive(rr, e) || campaign_guardian(rr->cell) >= 0)
                    continue;
                Vec3 to = vsub(enemy_center(enemy), player_eye(p));
                double d = vlen(to);
                if (d < best && world_raycast(&g->world, player_eye(p), vnorm(to), d) >= d - .1) {
                    best = d;
                    aim = vnorm(to);
                    target = world_hash(g->world.seed, rr->cell, enemy->id + 1);
                }
            }
        if (target != last_target) {
            last_target = target;
            reaction = profile == 2 ? .55 : .3;
        }
        reaction = fmax(0, reaction - dt);
        double error = profile == 2 ? .04 : .025;
        double yaw = atan2(aim.x, aim.z) - p->yaw;
        yaw = atan2(sin(yaw), cos(yaw)) + (target ? error * sin(g->time * 3.1) : 0);
        double pitch =
            asin(clampd(aim.y, -1, 1)) - p->pitch + (target ? error * cos(g->time * 2.7) : 0);
        double limit = (profile == 2 ? 1.8 : 2.8) * dt;
        game_look(p, clampd(yaw, -limit, limit), clampd(pitch, -limit, limit));
        actions[0].fire = target && reaction <= 0 && fabs(yaw) < .16;
        if (distance > .55) {
            Vec3 move = vnorm(delta);
            actions[0].forward = (float)vdot(move, v3(sin(p->yaw), 0, cos(p->yaw)));
            actions[0].strafe = (float)vdot(move, v3(cos(p->yaw), 0, -sin(p->yaw)));
        }
        actions[0].crouch =
            world_describe_version(g->world.seed, g->world.generator, world_cell(feet)).height <
                2 ||
            world_describe_version(g->world.seed, g->world.generator, world_cell(waypoint)).height <
                2;
        if (vlen(vsub(feet, progress)) > 1 || p->lift_active || distance < .55) {
            progress = feet;
            stalled = 0;
        } else
            stalled += dt;
        if (stalled > 5) {
            actions[0].strafe = frame % 240 < 120 ? 1 : -1;
            actions[0].forward = .4f;
            actions[0].jump = true;
            recoveries++;
        }
        float health = p->health, shield = p->shield, armor = p->armor;
        game_step(g, actions, dt);
        elapsed += dt;
        damage += fmaxf(0, health - p->health) + fmaxf(0, shield - p->shield) +
                  fmaxf(0, armor - p->armor);
        energy = fmin(energy, p->energy);
        if (g->world.failed || g->respawns > deaths + 3)
            return false;
    }
    fprintf(stderr, "Timed out at %.1f %.1f %.1f: %s\n", position_absolute(g->players[0].pos).x,
            position_absolute(g->players[0].pos).y, position_absolute(g->players[0].pos).z,
            g->message);
    return false;
}
int main(int argc, char **argv) {
    Game *g = calloc(1, sizeof(*g));
    if (!g)
        return 1;
    game_init(g, 713);
    bool prepare = argc == 2 && !strcmp(argv[1], "--prepare");
    const Cell first = {13, -3, 5}, second = {12, -3, 8};
    bool ok = game_load(g, SC_FIXTURE_DIR "/regional-expedition.sav");
    if (prepare) {
        ok = ok && route(g, first, false, 1) && game_save(g, SC_FIXTURE_DIR "/regional-relay.sav");
        ok =
            ok && route(g, first, true, 1) && campaign_owns(g, 0, (CampaignReward){REWARD_GUN, 17});
        ok = ok && route(g, second, false, 1) &&
             game_save(g, SC_FIXTURE_DIR "/regional-comparison.sav");
    } else {
        ok = ok && game_load(g, SC_FIXTURE_DIR "/regional-relay.sav") && route(g, first, true, 1) &&
             campaign_owns(g, 0, (CampaignReward){REWARD_GUN, 17});
        for (unsigned profile = 1; profile <= 2 && ok; profile++)
            for (unsigned gun = 0; gun < 2 && ok; gun++) {
                ok = game_load(g, SC_FIXTURE_DIR "/regional-comparison.sav");
                unsigned selected = gun ? 17 : 0;
                for (unsigned i = 0; i < SC_WEAPONS && g->players[0].weapon != selected; i++)
                    game_weapon(g, 0, 1);
                ok = ok && g->players[0].weapon == selected && route(g, second, true, profile);
            }
    }
    game_destroy(g);
    free(g);
    puts(ok ? "PASS regional earned-tool journeys" : "FAIL regional earned-tool journeys");
    return ok ? 0 : 1;
}
