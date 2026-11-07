#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Earned checkpoint, normal movement/look/fire/use only. No position, inventory,
   health or enemy writes. Read-only plans provide the same route as the HUD. */
static bool refit(Game *g, unsigned kind) {
    if (g->shelter.deployed)
        game_summon(g, 0);
    for (unsigned i = 0; i < 8 && !g->shelter.deployed; i++) {
        game_look(&g->players[0], SC_PI / 4, 0);
        game_summon(g, 0);
    }
    if (!g->shelter.deployed || !game_shelter_interact(g, 0) || !g->players[0].sheltered)
        return false;
    unsigned wanted = kind == CH_AERIE ? PSI_FLIGHT : PSI_NOVA;
    if (kind == CH_AERIE || kind == CH_COLONY) {
        for (unsigned i = 0; i < 5 && !campaign_power(g, 0, wanted); i++)
            campaign_refit(g, 0, 2, 1);
        if (!campaign_power(g, 0, wanted))
            return false;
    }
    if (kind == CH_SIEGE || kind == CH_CIRCUIT) {
        wanted = kind == CH_SIEGE ? MECH : WAGON;
        for (unsigned i = 0; i < 3 && !campaign_rig(g, 0, wanted); i++)
            campaign_refit(g, 0, 4, 1);
        if (!campaign_rig(g, 0, wanted))
            return false;
    }
    if (!game_shelter_interact(g, 0) || g->players[0].sheltered)
        return false;
    game_summon(g, 0);
    return true;
}
static bool route(Game *g, Cell goal, bool fight, bool use_tool) {
    const double dt = 1.0 / 60;
    unsigned start_kills = g->players[0].kills, deaths = g->respawns, shots = g->shots;
    unsigned weapon = g->players[0].weapon;
    double elapsed = 0, damage = 0, energy = 100, stalled = 0, reaction = 0;
    unsigned last_target = 0, interrupts = 0, recoveries = 0;
    bool rig_attempted = false, hacked = false;
    double circuit_at = 0;
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
            if (p->flight > 0)
                game_psionic(g, 0, PSI_FLIGHT);
            if (p->riding) {
                RigKind rig = p->riding;
                game_rig_interact(g, 0);
                if (!p->riding)
                    game_rig_toggle(g, 0, rig);
                continue;
            }
            if (vlen(vsub(feet, encounter_console(room))) < 2) {
                game_interact(g, 0);
                printf("FRONTIER tool=%u gun=%u cell=%d,%d,%d seconds=%.2f kills=%u deaths=%u "
                       "shots=%u damage=%.2f min_energy=%.2f disrupted_frames=%u recoveries=%u\n",
                       use_tool, weapon, goal.x, goal.y, goal.z, elapsed, p->kills - start_kills,
                       g->respawns - deaths, g->shots - shots, damage, energy, interrupts,
                       recoveries);
                printf("  kind=%u contract=%u circuit_at=%.2f relics=%u,%u,%u,%u,%u\n",
                       room->habitat.challenge, room->habitat.contract, circuit_at,
                       g->frontier.ranks[0][0], g->frontier.ranks[0][1], g->frontier.ranks[0][2],
                       g->frontier.ranks[0][3], g->frontier.ranks[0][4]);
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
        if (here && fight && use_tool && room->habitat.challenge == CH_CIPHER && !hacked) {
            waypoint = matrix_terminal(room->cell);
            if (vlen(vsub(feet, waypoint)) < 2) {
                if (!matrix_toggle(g, 0))
                    return false;
                for (int n = 0; n < 20; n++) {
                    Actions walk[2] = {{.forward = 1}, {0}};
                    game_step(g, walk, dt);
                }
                elapsed += 20 * dt;
                while (g->matrix.divers[0].service != MX_SECURITY)
                    matrix_cycle_service(g, 0, 1);
                if (!matrix_cast(g, 0, 0))
                    return false; /* The earned profile's default Veil. */
                if (!matrix_service(g, 0)) {
                    fprintf(stderr, "matrix service: %s\n", g->message);
                    return false;
                }
                if (!matrix_toggle(g, 0))
                    return false;
                hacked = true;
                continue;
            }
        }
        double range = vlen(vsub(feet, waypoint));
        if (here && range < 2 && !(use_tool && room->habitat.challenge == CH_CIPHER && !hacked) &&
            !room->frontier_active && !(world_local_state(&g->world, room) & FRONTIER_CLEARED)) {
            if (!fight)
                return true;
            game_interact(g, 0);
        }
        if (here && room->frontier_active) {
            Vec3 center = world_center(goal);
            waypoint = vadd(center, v3(sin(g->time * .6) * 5, 0, 2 + cos(g->time * .6) * 3));
            interrupts += room->frontier_disrupt > 0;
            if (room->habitat.challenge == CH_CIRCUIT && room->circuit != 3)
                waypoint = frontier_pylon(room, room->circuit & 1 ? 1 : 0);
            else if (room->habitat.challenge == CH_CIRCUIT && !circuit_at)
                circuit_at = elapsed;
            if (use_tool && room->habitat.challenge == CH_AERIE) {
                if (p->flight <= 0 && p->energy >= 20)
                    game_psionic(g, 0, PSI_FLIGHT);
                actions[0].jump = feet.y < center.y + 9;
                actions[0].crouch = feet.y > center.y + 10;
            }
            if (use_tool && room->habitat.challenge == CH_COLONY) {
                waypoint = vadd(center, v3(sin(g->time * .6) * 3, 0, 11));
                if (p->nova <= 0 && p->energy >= 35)
                    game_psionic(g, 0, PSI_NOVA);
            }
            if (use_tool && !rig_attempted &&
                (room->habitat.challenge == CH_SIEGE || room->habitat.challenge == CH_CIRCUIT)) {
                RigKind rig = room->habitat.challenge == CH_SIEGE ? MECH : WAGON;
                game_rig_toggle(g, 0, rig);
                game_rig_interact(g, 0);
                rig_attempted = p->riding == rig;
            }
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
            reaction = .35;
        }
        reaction = fmax(0, reaction - dt);
        double error = .03;
        double yaw = atan2(aim.x, aim.z) - p->yaw;
        yaw = atan2(sin(yaw), cos(yaw)) + (target ? error * sin(g->time * 3.1) : 0);
        double pitch =
            asin(clampd(aim.y, -1, 1)) - p->pitch + (target ? error * cos(g->time * 2.7) : 0);
        double limit = 2.5 * dt;
        game_look(p, clampd(yaw, -limit, limit), clampd(pitch, -limit, limit));
        actions[0].fire = target && reaction <= 0 && fabs(yaw) < .16;
        if (here && room->frontier_active && room->habitat.challenge == CH_CIRCUIT &&
            room->circuit != 3)
            actions[0].fire = false; /* Complete the same course on foot and in the wagon. */
        if (here && room->frontier_active && use_tool && room->habitat.challenge == CH_COLONY &&
            vlen(vsub(feet, enemy_center(&room->enemies[0]))) > 7)
            actions[0].fire = false;
        if (distance > .55) {
            Vec3 move = vnorm(delta);
            actions[0].forward = (float)vdot(move, v3(sin(p->yaw), 0, cos(p->yaw)));
            actions[0].strafe = (float)vdot(move, v3(cos(p->yaw), 0, -sin(p->yaw)));
        }
        actions[0].sprint = true;
        actions[0].crouch |=
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

static bool nearest(Game *g, unsigned kind, Cell *goal) {
    Cell from = world_cell(position_absolute(g->players[0].pos));
    double best = 1e9;
    Cell base = world_region(from);
    for (int z = base.z - 5; z <= base.z + 5; z++)
        for (int x = base.x - 5; x <= base.x + 5; x++) {
            if (z < 0)
                continue;
            RegionPlan plan;
            world_plan(g->world.seed, (Cell){x, from.y, z}, &plan);
            for (int n = 0; n < plan.count; n++) {
                RoomSpec *r = &plan.rooms[n];
                if (r->habitat.pace != PACE_FRONTIER || r->habitat.challenge != kind ||
                    r->habitat.volume || r->height < 12 || r->width < 70 ||
                    campaign_layer(r->cell) < 5)
                    continue;
                bool done = false;
                for (size_t i = 0; i < g->world.record_count; i++)
                    if (cell_equal(g->world.records[i].cell, r->cell) &&
                        (g->world.records[i].local & FRONTIER_CLEARED))
                        done = true;
                if (done)
                    continue;
                double score = abs(r->cell.x - from.x) + abs(r->cell.z - from.z);
                if (score < best) {
                    best = score;
                    *goal = r->cell;
                }
            }
        }
    return best < 1e9;
}
int main(int argc, char **argv) {
    bool prepare = argc == 2 && !strcmp(argv[1], "--prepare");
    Game *g = calloc(1, sizeof(*g));
    if (!g)
        return 1;
    game_init(g, 713);
    bool ok = game_load(g, SC_FIXTURE_DIR "/frontier-expedition.sav") && g->world.generator == 12;
    const unsigned order[] = {CH_AERIE, CH_COLONY, CH_SIEGE, CH_CIRCUIT, CH_CIPHER};
    for (unsigned n = 0; n < 5 && ok; n++) {
        unsigned kind = order[n];
        Cell first = {0}, second = {0};
        ok = nearest(g, kind, &first);
        printf("APPROACH kind=%u first=%d,%d,%d\n", kind, first.x, first.y, first.z);
        fflush(stdout);
        ok = ok && route(g, first, true, false) && refit(g, kind) && nearest(g, kind, &second) &&
             route(g, second, false, false);
        if (!ok)
            break;
        /* Match both loadouts to the always-owned carbine. Reward auto-equip is
           otherwise a confounder when comparing the newly earned capability. */
        for (unsigned i = 0; i < SC_WEAPONS && g->players[0].weapon; i++)
            game_weapon(g, 0, 1);
        /* The second arena is identical for both runs; only normal tool use changes. */
        ok = game_save(g, "frontier-comparison.sav");
        if (prepare) {
            char path[512];
            snprintf(path, sizeof(path), "%s/frontier-%u.sav", SC_FIXTURE_DIR, kind);
            ok = ok && game_save(g, path);
        }
        for (unsigned tool = 0; tool < 2 && ok; tool++) {
            ok = game_load(g, "frontier-comparison.sav") && route(g, second, true, tool != 0);
            if (!ok)
                fprintf(stderr, "comparison failed kind=%u tool=%u: %s\n", kind, tool, g->message);
        }
    }
    remove("frontier-comparison.sav");
    game_destroy(g);
    free(g);
    puts(ok ? "PASS five earned-tool encounter comparisons" : "FAIL frontier journeys");
    return ok ? 0 : 1;
}
