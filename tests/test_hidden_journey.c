#define _POSIX_C_SOURCE 200809L
#include "game.h"
#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

/* Review-only input controller. No position, damage, inventory, enemy, world
   record or progression writes. Menus use their production actions; the sole
   direct presentation write closes a journal, as in the existing journeys.
   Navigation reads the same generated plans as the HUD. This is not human play. */
/* Link wrappers are confined to this review executable. The shipped core has no
   director-disable switch or profiling hook. The paired run changes only this call. */
static bool without_director;
static double timings[6][300000];
static unsigned timing_count[6];
static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}
void __real_hidden_director_step(Game *g, double dt);
void __real_factions_step(Game *g, double dt);
void __real_factions_forces_step(Game *g, double dt);
void __real_factions_tactics(Game *g, double dt);
void __real_game_step(Game *g, const Actions *actions, double dt);
static void measured(unsigned kind, double elapsed) {
    if (timing_count[kind] < 300000)
        timings[kind][timing_count[kind]++] = elapsed * 1e6;
}
void __wrap_hidden_director_step(Game *g, double dt) {
    double start = now();
    if (!without_director)
        __real_hidden_director_step(g, dt);
    measured(2, now() - start);
}
void __wrap_factions_step(Game *g, double dt) {
    unsigned tick = g->factions.tick;
    double start = now();
    __real_factions_step(g, dt);
    double elapsed = now() - start;
    measured(0, elapsed);
    if (g->factions.tick != tick)
        measured(3, elapsed);
}
void __wrap_factions_forces_step(Game *g, double dt) {
    double start = now();
    __real_factions_forces_step(g, dt);
    measured(1, now() - start);
}
void __wrap_factions_tactics(Game *g, double dt) {
    double start = now();
    __real_factions_tactics(g, dt);
    measured(4, now() - start);
}
void __wrap_game_step(Game *g, const Actions *actions, double dt) {
    double start = now();
    __real_game_step(g, actions, dt);
    measured(5, now() - start);
}
static int compare_time(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
static void performance(void) {
    const char *names[] = {"strategy",        "forces",   "director", "strategy_active_tick",
                           "faction_tactics", "game_step"};
    for (unsigned i = 0; i < 6; i++) {
        unsigned n = timing_count[i];
        if (!n)
            continue;
        qsort(timings[i], n, sizeof(double), compare_time);
        printf("{\"event\":\"performance\",\"part\":\"%s\",\"samples\":%u,\"p50_us\":%.3f,\"p95_"
               "us\":%.3f,\"p99_us\":%.3f,\"max_us\":%.3f}\n",
               names[i], n, timings[i][n / 2], timings[i][n * 95 / 100], timings[i][n * 99 / 100],
               timings[i][n - 1]);
    }
}
static const unsigned seeds[] = {713, 42, 1, 3, 4, 14, 22, 109, 314, 2026, 9001, 8675309};
static const char *routes[] = {"inward",           "east",           "west",
                               "up-tour",          "down-tour",      "east-inward",
                               "west-inward",      "up-inward",      "down-inward",
                               "east-down-inward", "west-up-inward", "zigzag-inward"};
static const int directions[][3] = {{0, 0, 1},  {1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                    {0, -1, 0}, {1, 0, 1},  {-1, 0, 1}, {0, 1, 1},
                                    {0, -1, 1}, {1, -1, 1}, {-1, 1, 1}, {0, 0, 1}};
static const char *profiles[] = {"offense", "survival", "toolbox"};
static const double reactions[] = {.30, .55, .40}, turns[] = {2.8, 1.8, 2.3},
                    errors[] = {.025, .04, .035};
typedef struct {
    double health_loss, shield_loss, armor_loss, upgrade_restore, level_restore;
    double min_health, min_health_ratio, min_shield, min_energy, max_stall;
    unsigned damage_frames, recoveries, upgrades, saves, save_failures, skipped;
    unsigned close_frames, combat_frames, empty_energy_frames, guardian_frames;
    unsigned shelter_frames, tool_frames, director_claims, local_claims, frontier_claims;
    unsigned weapons[SC_WEAPONS], species[SC_ENEMY_TYPES], max_layer;
} Metrics;
static bool save_at(Game *g, const char *directory, const char *tag, Metrics *m) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.sav", directory, tag);
    m->saves++;
    bool ok = game_save(g, path);
    if (!ok)
        m->save_failures++;
    printf("{\"event\":\"save\",\"t\":%.3f,\"tag\":\"%s\",\"ok\":%s}\n", g->time, tag,
           ok ? "true" : "false");
    return ok;
}
static void select_weapon(Game *g, unsigned wanted) {
    for (unsigned n = 0; n < SC_WEAPONS && g->players[0].weapon != wanted; n++)
        game_weapon(g, 0, 1);
}
static double defense(const Player *p) {
    return p->health + p->armor + p->shield;
}
static void upgrade(Game *g, unsigned profile, Metrics *m) {
    Player *p = &g->players[0];
    static const unsigned order[3][SC_ATTRIBUTES] = {{0, 1, 2, 3, 4, 8, 7, 5, 6, 9},
                                                     {1, 4, 0, 3, 7, 8, 2, 5, 6, 9},
                                                     {0, 8, 1, 5, 4, 2, 3, 7, 6, 9}};
    /* Offensive profile spends immediately; others spend in quiet rooms at
       thirty-second intervals. All keep earning, purchasing and paying costs. */
    for (unsigned n = 0; n < 120; n++) {
        unsigned attr = 10;
        for (unsigned j = 0; j < SC_ATTRIBUTES; j++) {
            unsigned a = order[profile][j];
            unsigned cap = (j < 2 && p->level < 10) ? 6 : 12;
            if (*player_attribute(p, (int)a) < cap) {
                attr = a;
                break;
            }
        }
        double before = defense(p);
        if (attr == 10 || !game_upgrade(g, 0, (int)attr))
            break;
        m->upgrades++;
        m->upgrade_restore += fmax(0, defense(p) - before);
        printf("{\"event\":\"upgrade\",\"t\":%.3f,\"attribute\":%u,\"value\":%u,"
               "\"defense_increase\":%.2f}\n",
               g->time, attr, *player_attribute(p, attr), fmax(0, defense(p) - before));
    }
}
static Cell campaign_goal(Game *g, unsigned run) {
    Cell goal = {0};
    unsigned phase = campaign_phase(&g->campaign);
    bool briefing = (g->story.done & 1) != 0;
    if (briefing && !(g->campaign.defeated & (1u << 8)))
        return campaign_site(8);
    if (briefing && phase < 4 && (phase == 0 || (g->story.assigned & (1u << game_story_target(g)))))
        return campaign_site(phase * 2 + run % 2);
    while (g->campaign.tracked != SC_GUARDIANS)
        campaign_track(g);
    game_objective(g, &goal);
    return goal;
}
static Cell exploration_goal(unsigned run, unsigned step) {
    const int *d = directions[run];
    Cell c = {d[0] * (int)step, d[1] * (int)(step / 2), 2 + d[2] * (int)step};
    if (run == 3 || run == 4) {
        c.x = step % 2 ? 2 : -2; /* Walk across each deck; don't just ride the lift. */
        c.z = 2 + (int)(step % 3);
    }
    if (run == 11)
        c.x = (step / 4) % 2 ? -4 : 4;
    return c;
}
static bool optional_goal(Game *g, unsigned sequence, Cell *goal, int *kind) {
    Vec3 feet = position_absolute(g->players[0].pos);
    double best = 180;
    bool found = false;
    for (int i = 0; i < g->world.count; i++) {
        Room *r = &g->world.rooms[i];
        if (!r->ready || r->habitat.court || campaign_guardian(r->cell) >= 0 ||
            r->cell.y != world_cell(feet).y || r->habitat.volume ||
            (g->world.open_layer && campaign_layer(r->cell) > g->world.open_layer))
            continue;
        int type = 0;
        if (r->habitat.pace == PACE_FRONTIER &&
            !(world_local_state(&g->world, r) & FRONTIER_CLAIMED))
            type = 1;
        if (sequence % 3 == 2 && local_available(&g->world, r))
            type = 2;
        double distance = vlen(vsub(world_center(r->cell), feet));
        if (type && distance < best) {
            best = distance;
            *goal = r->cell;
            *kind = type;
            found = true;
        }
    }
    return found;
}
static void prepare_tools(Game *g, unsigned challenge) {
    Player *p = &g->players[0];
    if (g->shelter.deployed)
        game_summon(g, 0);
    /* Deployment is an ordinary action; no turning snaps or location changes. */
    game_summon(g, 0);
    if (!g->shelter.deployed || !game_shelter_interact(g, 0) || !p->sheltered)
        return;
    unsigned power = challenge == CH_AERIE ? PSI_FLIGHT : PSI_NOVA;
    for (unsigned i = 0; i < 5 && !campaign_power(g, 0, power); i++)
        campaign_refit(g, 0, 2, 1);
    unsigned rig = challenge == CH_CIRCUIT ? WAGON : MECH;
    for (unsigned i = 0; i < 3 && !campaign_rig(g, 0, rig); i++)
        campaign_refit(g, 0, 4, 1);
    /* Leave through the doorway at which we just entered. */
    game_shelter_interact(g, 0);
    if (!p->sheltered)
        game_summon(g, 0);
}
static int journey(unsigned run, const char *directory, unsigned duration) {
    Game *g = calloc(1, sizeof(*g));
    if (!g)
        return 2;
    unsigned seed = seeds[run], profile = run % 3, nation = (run / 3) % 3;
    game_init(g, seed);
    Session session;
    session_init(&session, g);
    Metrics m = {.min_health = g->players[0].health,
                 .min_health_ratio = 1,
                 .min_shield = g->players[0].shield,
                 .min_energy = g->players[0].energy};
    printf("{\"event\":\"start\",\"seed\":%u,\"run\":%u,\"route\":\"%s\","
           "\"profile\":\"%s\",\"reaction\":%.2f,\"turn\":%.2f,\"aim_error\":%.3f,"
           "\"ship\":%u,\"history\":%u,\"motivation\":%u,\"seconds\":%u}\n",
           seed, run, routes[run], profiles[profile], reactions[profile], turns[profile],
           errors[profile], world_profile(seed), world_history(seed).cause,
           story_motivation_version(seed, g->story.version), duration);
    unsigned previous_defeated = 0, previous_story = 0, previous_politics = 0;
    unsigned stage = 4, mode = 0, option_sequence = 0, previous_deaths = 0;
    unsigned aim_target = 0, recovery_ticks = 0, optional_kind = 0;
    unsigned director_sequence = 0, director_kind = 0, fight_guardian = SC_GUARDIANS;
    unsigned captured_forms = 0, last_event = 0, captured_evidence = 0;
    unsigned pressure_frames[4] = {0}, max_forces = 0, max_actors = 0;
    uint32_t sequence_ids[SC_SEQUENCES] = {0};
    unsigned sequence_beats[SC_SEQUENCES] = {0};
    Cell previous_cell = {9999, 9999, 9999}, previous_owner = previous_cell;
    Cell optional = {0}, nav_goal = previous_cell, fight_cell = previous_cell;
    bool detour = false, returned = false, tools_prepared = false, resumed = false;
    bool shelter_done = false, resting = false;
    double reaction = 0, stalled = 0, optional_at = 0, next_optional = 0, rest_until = 0;
    double nav_until = 0, stage_at = 0, fight_at = 0, fight_damage = 0;
    double last_upgrade = -30, last_weapon = -10, last_use = -1;
    Vec3 progress = position_absolute(g->players[0].pos);
    Navigation nav = {0};
    unsigned frame;
    for (frame = 0; frame < duration * 60 && !g->world.failed; frame++) {
        Player *p = &g->players[0];
        Vec3 feet = position_absolute(p->pos);
        if (!world_refresh(&g->world, &feet, 1))
            break;
        /* Session owns the sole streaming budget for this fixed tick. */
        Cell cell = world_cell(feet);
        Room *room = world_find(&g->world, cell);
        if (!cell_equal(cell, previous_cell)) {
            RoomSpec s = world_describe_version(seed, g->world.generator, cell);
            printf("{\"event\":\"cell\",\"t\":%.3f,\"cell\":[%d,%d,%d],\"owner\":[%d,%d,%d],"
                   "\"layer\":%u,\"theme\":%u,\"district\":%u,\"form\":%u,\"volume\":%u,"
                   "\"weather\":%u,\"ecology\":%u,\"pace\":%u,\"challenge\":%u,"
                   "\"reveal\":%u,\"country\":%u,\"court\":%u,\"height\":%.1f}\n",
                   g->time, cell.x, cell.y, cell.z, s.cell.x, s.cell.y, s.cell.z,
                   campaign_layer(cell), s.theme, s.district, s.habitat.form, s.habitat.volume,
                   s.habitat.atmosphere, s.habitat.ecology, s.habitat.pace, s.habitat.challenge,
                   s.habitat.reveal, politics_owner(g, cell), s.habitat.court, s.height);
            previous_cell = cell;
            nav_until = 0;
            if (campaign_layer(cell) > m.max_layer)
                m.max_layer = campaign_layer(cell);
            if (!cell_equal(s.cell, previous_owner)) {
                for (int i = 0; room && i < room->enemy_count; i++)
                    if (room->enemies[i].alive)
                        m.species[room->enemies[i].kind]++;
                previous_owner = s.cell;
            }
            if (s.habitat.form && !(captured_forms & (1u << s.habitat.form)) && !p->lift_active &&
                room && room->ready) {
                char tag[40];
                snprintf(tag, sizeof(tag), "form-%u", s.habitat.form);
                save_at(g, directory, tag, &m);
                captured_forms |= 1u << s.habitat.form;
            }
        }
        if (g->campaign.defeated != previous_defeated) {
            printf("{\"event\":\"guardian_victory\",\"t\":%.3f,\"defeated\":%u,\"rank\":%u}\n",
                   g->time, g->campaign.defeated, p->level);
            campaign_journal(g, 0);
            unsigned choice = profile == 2 && campaign_phase(&g->campaign) % 2 ? 1 : profile + 1;
            if (!campaign_choose(g, 0, choice)) {
                fprintf(stderr, "Reward refused at %.3f\n", g->time);
                break;
            }
            PLAYER_VIEW(g, p).journal = false;
            previous_defeated = g->campaign.defeated;
            if (campaign_phase(&g->campaign) == 4)
                save_at(g, directory, "unbound", &m);
        }
        if (g->story.done != previous_story) {
            printf("{\"event\":\"story\",\"t\":%.3f,\"done\":%u,\"rank\":%u}\n", g->time,
                   g->story.done, p->level);
            previous_story = g->story.done;
        }
        if (!mode && (g->story.done & (1u << STORY_EXTRACTION))) {
            mode = 2;
            stage_at = g->time;
            next_optional = g->time + 30;
            save_at(g, directory, "extraction", &m);
        }
        unsigned political_stage = g->politics.countries[nation].stage;
        if (political_stage != previous_politics) {
            printf("{\"event\":\"politics\",\"t\":%.3f,\"nation\":%u,\"stage\":%u,\"pledge\":%u}\n",
                   g->time, nation, political_stage, g->politics.countries[nation].pledge);
            previous_politics = political_stage;
            save_at(g, directory, political_stage == 3 ? "settlement" : "commission", &m);
        }
        if (mode == 1 && returned) {
            mode = 2;
            stage_at = g->time;
            next_optional = g->time + 30;
            if (g->politics.tracked)
                politics_track(g, 0);
        }
        if (g->director.sequence != director_sequence) {
            director_sequence = g->director.sequence;
            printf("{\"event\":\"dispatch\",\"t\":%.3f,\"sequence\":%u,\"kind\":%u,"
                   "\"site\":[%d,%d,%d],\"health\":%.2f,\"energy\":%.2f}\n",
                   g->time, director_sequence, g->director.kind, g->director.site.x,
                   g->director.site.y, g->director.site.z, p->health, p->energy);
        }
        if (director_kind && !g->director.kind)
            printf("{\"event\":\"dispatch_end\",\"t\":%.3f,\"kind\":%u}\n", g->time, director_kind);
        director_kind = g->director.kind;
        int guardian = -1;
        for (unsigned i = 0; i < SC_GUARDIANS; i++)
            if (g->campaign.attempts[i].active)
                guardian = (int)i;
        bool arena = room && room->frontier_active;
        bool fighting = guardian >= 0 || arena;
        if (fighting && fight_guardian == SC_GUARDIANS && fight_cell.x == 9999) {
            fight_guardian = guardian >= 0 ? (unsigned)guardian : SC_GUARDIANS;
            fight_cell = room ? room->cell : cell;
            fight_at = g->time;
            fight_damage = m.health_loss + m.shield_loss + m.armor_loss;
            printf("{\"event\":\"fight_start\",\"t\":%.3f,\"guardian\":%d,\"challenge\":%u,"
                   "\"court\":%u,\"weapon\":%u,\"rank\":%u,\"cell\":[%d,%d,%d]}\n",
                   g->time, guardian, room ? room->habitat.challenge : 0,
                   room ? room->habitat.court : 0, p->weapon, p->level, fight_cell.x, fight_cell.y,
                   fight_cell.z);
            save_at(g, directory,
                    mode == 1   ? "court-fight"
                    : mode == 2 ? "optional-fight"
                                : "guardian-fight",
                    &m);
        } else if (!fighting && fight_cell.x != 9999) {
            printf("{\"event\":\"fight_end\",\"t\":%.3f,\"seconds\":%.3f,\"damage\":%.2f,"
                   "\"guardian\":%u,\"cell\":[%d,%d,%d]}\n",
                   g->time, g->time - fight_at,
                   m.health_loss + m.shield_loss + m.armor_loss - fight_damage, fight_guardian,
                   fight_cell.x, fight_cell.y, fight_cell.z);
            fight_guardian = SC_GUARDIANS;
            fight_cell = (Cell){9999, 9999, 9999};
        }
        Cell goal = mode == 0   ? campaign_goal(g, run)
                    : mode == 1 ? politics_site(seed, nation,
                                                political_stage == 1   ? COURT_CONDUIT
                                                : political_stage == 2 ? COURT_FINALE
                                                                       : COURT_AUDIENCE)
                                : exploration_goal(run, stage);
        if (mode == 2 && !detour && g->time >= next_optional && !fighting) {
            int kind = 0;
            detour = optional_goal(g, option_sequence++, &optional, &kind);
            optional_kind = (unsigned)kind;
            optional_at = g->time;
            next_optional = g->time + (profile == 2 ? 100 : 160);
            tools_prepared = false;
            if (detour)
                printf("{\"event\":\"detour\",\"t\":%.3f,\"kind\":%u,\"goal\":[%d,%d,%d]}\n",
                       g->time, optional_kind, optional.x, optional.y, optional.z);
        }
        if (detour) {
            goal = optional;
            Room *o = world_find(&g->world, optional);
            unsigned state = o ? world_local_state(&g->world, o) : 0;
            if ((optional_kind == 1 && (state & FRONTIER_CLAIMED)) ||
                (optional_kind == 2 && (state & LOCAL_RESOLVED))) {
                m.frontier_claims += optional_kind == 1;
                m.local_claims += optional_kind == 2;
                printf("{\"event\":\"detour_claim\",\"t\":%.3f,\"seconds\":%.3f,\"kind\":%u,"
                       "\"challenge\":%u,\"contract\":%u}\n",
                       g->time, g->time - optional_at, optional_kind, o ? o->habitat.challenge : 0,
                       o ? o->habitat.contract : 0);
                detour = false;
                goal = exploration_goal(run, stage);
                stage_at = g->time;
            } else if (g->time - optional_at > 180) {
                printf("{\"event\":\"abandon_detour\",\"t\":%.3f,\"kind\":%u}\n", g->time,
                       optional_kind);
                m.skipped++;
                detour = false;
                goal = exploration_goal(run, stage);
                next_optional = g->time + 160;
            }
        }
        if (!cell_equal(goal, nav_goal) || g->time >= nav_until) {
            if (!game_navigation_to(g, 0, goal, &nav))
                memset(&nav, 0, sizeof(nav));
            nav_goal = goal;
            nav_until = g->time + .35;
        }
        Vec3 waypoint = nav.point;
        Room *target_room = world_find(&g->world, goal);
        bool here = target_room && room == target_room && room->ready;
        if (here) {
            if (mode == 1 || (detour && optional_kind == 1))
                waypoint = encounter_console(room);
            else if (detour && optional_kind == 2)
                waypoint = local_console(room);
            else if (mode == 2)
                waypoint = vadd(world_center(goal), v3(0, .02, 0));
        }
        Actions actions[2] = {0};
        if (guardian >= 0) {
            Vec3 center = world_center(campaign_site((unsigned)guardian));
            waypoint = vadd(center, v3(sin(g->time * .65) * 10, 0, cos(g->time * .65) * 10));
            m.guardian_frames++;
        } else if (arena) {
            Vec3 center = world_center(room->cell);
            waypoint = vadd(center, v3(sin(g->time * .6) * 5, 0, 2 + cos(g->time * .6) * 3));
            if (room->habitat.challenge == CH_CIRCUIT && room->circuit != 3)
                waypoint = frontier_pylon(room, room->circuit & 1 ? 1 : 0);
            if (profile == 2) {
                if (room->habitat.challenge == CH_AERIE && campaign_power(g, 0, PSI_FLIGHT)) {
                    if (p->flight <= 0 && p->energy >= 20)
                        game_psionic(g, 0, PSI_FLIGHT);
                    actions[0].jump = feet.y < center.y + 9;
                    actions[0].crouch = feet.y > center.y + 10;
                }
                if (room->habitat.challenge == CH_COLONY && p->nova <= 0 && p->energy >= 35)
                    game_psionic(g, 0, PSI_NOVA);
            }
        }
        if (profile == 2 && here && optional_kind == 1 && detour && !tools_prepared && !arena) {
            prepare_tools(g, room->habitat.challenge);
            tools_prepared = true;
        }
        if (!fighting && p->riding) {
            RigKind rig = (RigKind)p->riding;
            game_rig_interact(g, 0);
            if (!p->riding)
                game_rig_toggle(g, 0, rig);
        }
        if (arena && profile == 2 && !p->riding && frame % 120 == 0 &&
            (room->habitat.challenge == CH_SIEGE || room->habitat.challenge == CH_CIRCUIT)) {
            RigKind rig = room->habitat.challenge == CH_SIEGE ? MECH : WAGON;
            if (campaign_rig(g, 0, rig)) {
                game_rig_toggle(g, 0, rig);
                game_rig_interact(g, 0);
            }
        }
        if (!fighting && p->flight > 0)
            game_psionic(g, 0, PSI_FLIGHT);
        Vec3 delta = vsub(waypoint, feet);
        delta.y = 0;
        double distance = vlen(delta);
        if (!fighting && !p->lift_active && g->time - last_use > .5) {
            bool dispatch_here =
                g->director.kind && room && cell_equal(g->director.site, room->cell);
            if (dispatch_here && vlen(vsub(feet, director_point(&g->director))) < 2.5) {
                unsigned before = g->director.kind;
                game_interact(g, 0);
                if (!g->director.kind) {
                    m.director_claims++;
                    printf("{\"event\":\"dispatch_claim\",\"t\":%.3f,\"kind\":%u}\n", g->time,
                           before);
                }
                last_use = g->time;
            } else if (here && distance < 1.2) {
                if (mode == 2 && !detour) {
                    stage++;
                    stage_at = g->time;
                    nav_until = 0;
                } else {
                    game_interact(g, 0);
                    if (PLAYER_INTERACTION(g, p).decision_open) {
                        printf("{\"event\":\"decision\",\"t\":%.3f,\"scene\":%u,\"choice\":1}\n",
                               g->time, PLAYER_INTERACTION(g, p).decision_scene);
                        game_story_choose(g, 0, 1);
                    }
                    if (mode == 1) {
                        if (political_stage == 0 && g->politics.audience[0])
                            politics_choose(g, 0, profile + 1);
                        if (political_stage == 3 && room->habitat.court == COURT_AUDIENCE) {
                            returned = true;
                            save_at(g, directory, "changed-court", &m);
                        }
                    }
                    PLAYER_VIEW(g, p).journal = false;
                    last_use = g->time;
                }
            }
        }
        /* Nearby dispatches are acknowledged, but never dragged across rooms. */
        if (!fighting && !detour && g->director.kind && room &&
            cell_equal(g->director.site, room->cell) &&
            vlen(vsub(feet, director_point(&g->director))) < 18) {
            waypoint = director_point(&g->director);
            delta = vsub(waypoint, feet);
            delta.y = 0;
            distance = vlen(delta);
        }
        if (!fighting && nav.kind == NAV_LIFT && distance < 1) {
            actions[0].lift_up = !p->lift_latched && cell.y < goal.y;
            actions[0].lift_down = !p->lift_latched && cell.y > goal.y;
        }
        Vec3 aim = vnorm(delta);
        unsigned target = 0;
        double nearest = 65;
        for (int r = 0; r < g->world.count; r++) {
            Room *rr = &g->world.rooms[r];
            for (int e = 0; e < rr->enemy_count; e++) {
                Enemy *enemy = &rr->enemies[e];
                int guard = campaign_guardian(rr->cell);
                if (!enemy->alive || enemy->allegiance || encounter_passive(rr, e) ||
                    !politics_threat(g, enemy) ||
                    (guard >= 0 && !g->campaign.attempts[guard].active))
                    continue;
                Vec3 to = vsub(enemy_center(enemy), player_eye(p));
                double d = vlen(to);
                if (d < nearest &&
                    world_raycast(&g->world, player_eye(p), vnorm(to), d) >= d - .1) {
                    nearest = d;
                    aim = vnorm(to);
                    target = world_hash(seed, rr->cell, enemy->id + 1);
                }
            }
        }
        if (profile == 0 || (!target && !fighting && g->time - last_upgrade >= 30)) {
            upgrade(g, profile, &m);
            last_upgrade = g->time;
        }
        if (g->time - last_weapon >= 10) {
            unsigned weapon = 0;
            unsigned second = g->campaign.profiles[0].guns[1];
            if (profile == 1 && second != SC_FLARE_CANNON && (second != 1 || nearest < 28))
                weapon = second;
            if (profile == 2) {
                if (second != SC_FLARE_CANNON && (second != 1 || nearest < 28))
                    weapon = second;
                static const unsigned preference[] = {17, 15, 10, 16, 8, 3, 7, 2};
                for (unsigned i = 0; i < sizeof(preference) / sizeof(*preference); i++)
                    if (campaign_weapon(g, 0, preference[i])) {
                        weapon = preference[i];
                        break;
                    }
            }
            select_weapon(g, weapon);
            last_weapon = g->time;
        }
        if (target != aim_target) {
            aim_target = target;
            reaction = reactions[profile];
        }
        reaction = fmax(0, reaction - 1.0 / 60);
        double yaw = atan2(aim.x, aim.z) - p->yaw;
        yaw = atan2(sin(yaw), cos(yaw));
        double pitch = asin(clampd(aim.y, -1, 1)) - p->pitch;
        if (target) {
            yaw += errors[profile] * sin(g->time * 3.1 + target % 17);
            pitch += errors[profile] * cos(g->time * 2.7);
        }
        double limit = turns[profile] / 60;
        game_look(p, clampd(yaw, -limit, limit), clampd(pitch, -limit, limit));
        actions[0].fire = target && reaction <= 0 && fabs(yaw) < .16;
        Vec3 move = vnorm(delta);
        if (distance > .65) {
            actions[0].forward = (float)vdot(move, v3(sin(p->yaw), 0, cos(p->yaw)));
            actions[0].strafe = (float)vdot(move, v3(cos(p->yaw), 0, -sin(p->yaw)));
        }
        actions[0].sprint = mode > 0 || profile == 0;
        actions[0].crouch |=
            world_describe_version(seed, g->world.generator, cell).height < 2 ||
            (distance > 1 &&
             world_describe_version(seed, g->world.generator, world_cell(waypoint)).height < 2);
        /* One ordinary safe rest per expedition, after court business. Recovery
           is measured separately from field combat. No interior teleport. */
        if (mode == 2 && !shelter_done && !target && !fighting && !detour && !p->lift_active &&
            room && room->height > 4) {
            if (g->shelter.deployed)
                game_summon(g, 0);
            game_summon(g, 0);
            if (g->shelter.deployed && game_shelter_interact(g, 0) && p->sheltered) {
                resting = shelter_done = true;
                rest_until = g->time + 12;
                printf("{\"event\":\"shelter_enter\",\"t\":%.3f}\n", g->time);
                save_at(g, directory, "shelter", &m);
            }
        }
        if (resting) {
            memset(&actions[0], 0, sizeof(actions[0]));
            if (g->time >= rest_until) {
                game_shelter_interact(g, 0);
                if (!p->sheltered) {
                    resting = false;
                    game_summon(g, 0);
                    printf("{\"event\":\"shelter_exit\",\"t\":%.3f}\n", g->time);
                }
            }
        }
        if (vlen(vsub(feet, progress)) > 1 || p->lift_active || distance < .65 || resting) {
            progress = feet;
            stalled = 0;
        } else
            stalled += 1.0 / 60;
        m.max_stall = fmax(m.max_stall, stalled);
        if (stalled >= 5) {
            m.recoveries++;
            recovery_ticks = 120;
            stalled = 0;
            nav_until = 0;
            printf("{\"event\":\"recovery\",\"t\":%.3f,\"pos\":[%.2f,%.2f,%.2f],"
                   "\"goal\":[%d,%d,%d],\"nav\":%u}\n",
                   g->time, feet.x, feet.y, feet.z, goal.x, goal.y, goal.z, nav.kind);
        }
        if (recovery_ticks && !resting) {
            recovery_ticks--;
            actions[0].strafe = m.recoveries % 2 ? 1 : -1;
            actions[0].forward = .3f;
            actions[0].jump = true;
            actions[0].crouch = false;
            nav_until = 0;
        }
        if (mode == 2 && !detour && g->time - stage_at > 180) {
            printf("{\"event\":\"abandon_waypoint\",\"t\":%.3f,\"stage\":%u}\n", g->time, stage);
            stage++;
            stage_at = g->time;
            m.skipped++;
        }
        float hp = p->health, sh = p->shield, ar = p->armor;
        unsigned rank = p->level, shots = g->shots;
        session_sync(&session);
        for (unsigned i = 0; i < g->player_count; i++)
            if (!session_input(&session, game_player_id(g, (int)i), &actions[i]))
                abort();
        session_tick(&session);
        for (unsigned s = 0; s < SC_SEQUENCES; s++) {
            const HiddenSequence *q = &g->rhythm.sequences[s];
            if (q->identity != sequence_ids[s] || (unsigned)q->beat != sequence_beats[s]) {
                printf("{\"event\":\"sequence\",\"t\":%.3f,\"slot\":%u,\"id\":%u,\"kind\":%u,"
                       "\"beat\":%u,\"site\":[%d,%d,%d],\"reserved\":%s}\n",
                       g->time, s, q->identity, (unsigned)q->kind, (unsigned)q->beat, q->site.x,
                       q->site.y, q->site.z, q->reserved ? "true" : "false");
                sequence_ids[s] = q->identity;
                sequence_beats[s] = (unsigned)q->beat;
            }
        }
        pressure_frames[g->factions.director.pace[0]]++;
        unsigned force_count = 0, actor_count = 0;
        for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
            FactionForce *f = &g->factions.forces[i];
            if (!f->id)
                continue;
            force_count++;
            if (f->physical)
                actor_count += factions_strength(f);
            unsigned cue = f->physical && f->cargo                    ? 1
                           : f->physical && f->order == ORDER_RETREAT ? 2
                                                                      : 0;
            if (cue && !(captured_evidence & (1u << cue))) {
                Vec3 delta = position_delta(f->members[0].pos, p->pos);
                if (vlen(delta) < 60) {
                    save_at(g, directory, cue == 1 ? "convoy-observed" : "retreat-observed", &m);
                    captured_evidence |= 1u << cue;
                }
            }
        }
        max_forces = force_count > max_forces ? force_count : max_forces;
        max_actors = actor_count > max_actors ? actor_count : max_actors;
        if (g->factions.event_count != last_event) {
            unsigned first = g->factions.event_count > SC_FACTION_EVENTS
                                 ? g->factions.event_count - SC_FACTION_EVENTS
                                 : 0;
            if (first < last_event)
                first = last_event;
            for (unsigned i = first; i < g->factions.event_count; i++) {
                FactionEvent *e = &g->factions.events[i % SC_FACTION_EVENTS];
                printf("{\"event\":\"simulation\",\"t\":%.3f,\"tick\":%u,\"kind\":%u,\"country\":%"
                       "u,\"force\":%u,\"site\":[%d,%d,%d],\"detail\":%u}\n",
                       g->time, e->tick, e->kind, e->country, e->force, e->site.x, e->site.y,
                       e->site.z, e->detail);
                if (e->kind == FE_CAPTURE && !(captured_evidence & 8)) {
                    save_at(g, directory, "occupation", &m);
                    captured_evidence |= 8;
                }
            }
            last_event = g->factions.event_count;
        }
        m.health_loss += fmaxf(0, hp - p->health);
        m.shield_loss += fmaxf(0, sh - p->shield);
        m.armor_loss += fmaxf(0, ar - p->armor);
        m.level_restore += p->level != rank ? fmax(0, defense(p) - hp - sh - ar) : 0;
        m.damage_frames += p->health < hp || p->shield < sh || p->armor < ar;
        m.min_health = fmin(m.min_health, p->health);
        m.min_health_ratio = fmin(m.min_health_ratio, p->health / player_max_health(p));
        m.min_shield = fmin(m.min_shield, p->shield);
        m.min_energy = fmin(m.min_energy, p->energy);
        m.combat_frames += target != 0;
        m.close_frames += nearest < 12;
        m.empty_energy_frames += p->energy < sc_weapons[p->weapon].cost;
        m.shelter_frames += p->sheltered;
        m.tool_frames += p->riding || p->flight > 0 || p->nova > 0;
        m.weapons[p->weapon] += g->shots - shots;
        if (g->respawns != previous_deaths) {
            printf("{\"event\":\"respawn\",\"t\":%.3f,\"count\":%u}\n", g->time, g->respawns);
            previous_deaths = g->respawns;
            nav_until = 0;
        }
        if (frame % 1800 == 0 || frame == duration * 60 - 1) {
            printf("{\"event\":\"sample\",\"t\":%.3f,\"cell\":[%d,%d,%d],\"mode\":%u,"
                   "\"stage\":%u,\"kills\":%u,\"deaths\":%u,\"rank\":%u,\"phase\":%u,"
                   "\"health\":%.2f,\"shield\":%.2f,\"armor\":%.2f,\"energy\":%.2f,"
                   "\"health_loss\":%.2f,\"shield_loss\":%.2f,\"armor_loss\":%.2f,"
                   "\"shots\":%u,\"hits\":%u,\"weapon\":%u,\"credits\":%u,\"points\":%u}\n",
                   g->time, cell.x, cell.y, cell.z, mode, stage, p->kills, g->respawns, p->level,
                   campaign_phase(&g->campaign), p->health, p->shield, p->armor, p->energy,
                   m.health_loss, m.shield_loss, m.armor_loss, g->shots, g->hits, p->weapon,
                   p->credits, p->points);
            fflush(stdout);
        }
        if ((frame + 1) % 54000 == 0 || frame == duration * 60 - 1) {
            char tag[80];
            snprintf(tag, sizeof(tag), "minute-%02u", (frame + 1) / 3600);
            save_at(g, directory, tag, &m);
        }
        /* A real round trip at thirty minutes; snapshots are loaded unchanged. */
        if (!resumed && g->time >= 1800 && !fighting && !p->lift_active && !p->riding &&
            !p->sheltered) {
            char path[512];
            snprintf(path, sizeof(path), "%s/resume.sav", directory);
            bool ok = save_at(g, directory, "resume", &m) && game_load(g, path);
            printf("{\"event\":\"resume\",\"t\":%.3f,\"ok\":%s}\n", g->time, ok ? "true" : "false");
            if (!ok) {
                m.save_failures++;
                break;
            }
            resumed = true;
            nav_until = 0;
        }
        if (!isfinite(p->health) || !isfinite(p->energy) || g->world.last_work > SC_STREAM_BUDGET)
            break;
    }
    Player *p = &g->players[0];
    save_at(g, directory, "final", &m);
    printf("{\"event\":\"end\",\"t\":%.3f,\"frames\":%u,\"complete\":%s,\"world_failed\":%s,"
           "\"kills\":%u,\"deaths\":%u,\"rank\":%u,\"phase\":%u,\"mode\":%u,\"max_layer\":%u,"
           "\"health_loss\":%.2f,\"shield_loss\":%.2f,\"armor_loss\":%.2f,"
           "\"upgrade_defense_increase\":%.2f,\"level_defense_increase\":%.2f,\"upgrades\":%u,"
           "\"min_health\":%.2f,\"min_health_ratio\":%.4f,\"min_shield\":%.2f,\"min_energy\":%.2f,"
           "\"damage_frames\":%u,\"recoveries\":%u,\"max_stall\":%.3f,\"skipped\":%u,"
           "\"combat_seconds\":%.2f,\"close_seconds\":%.2f,\"energy_starved_seconds\":%.2f,"
           "\"guardian_seconds\":%.2f,\"shelter_seconds\":%.2f,\"tool_seconds\":%.2f,"
           "\"dispatches\":%u,\"dispatch_claims\":%u,\"local_claims\":%u,\"frontier_claims\":%u,"
           "\"saves\":%u,\"save_failures\":%u,\"resumed\":%s,\"weapons\":[",
           g->time, frame, frame == duration * 60 ? "true" : "false",
           g->world.failed ? "true" : "false", p->kills, g->respawns, p->level,
           campaign_phase(&g->campaign), mode, m.max_layer, m.health_loss, m.shield_loss,
           m.armor_loss, m.upgrade_restore, m.level_restore, m.upgrades, m.min_health,
           m.min_health_ratio, m.min_shield, m.min_energy, m.damage_frames, m.recoveries,
           m.max_stall, m.skipped, m.combat_frames / 60.0, m.close_frames / 60.0,
           m.empty_energy_frames / 60.0, m.guardian_frames / 60.0, m.shelter_frames / 60.0,
           m.tool_frames / 60.0, director_sequence, m.director_claims, m.local_claims,
           m.frontier_claims, m.saves, m.save_failures, resumed ? "true" : "false");
    for (unsigned i = 0; i < SC_WEAPONS; i++)
        printf("%s%u", i ? "," : "", m.weapons[i]);
    printf("],\"species\":[");
    for (unsigned i = 0; i < SC_ENEMY_TYPES; i++)
        printf("%s%u", i ? "," : "", m.species[i]);
    printf("],\"relics\":[");
    for (unsigned i = 0; i < RELICS; i++)
        printf("%s%u", i ? "," : "", g->frontier.ranks[0][i]);
    printf("]}\n");
    printf("{\"event\":\"simulation_summary\",\"disabled\":%s,\"ticks\":%u,\"launches\":%u,"
           "\"arrivals\":%u,\"losses\":%u,\"captures\":%u,\"retreats\":%u,\"interventions\":%u,"
           "\"rejections\":%u,\"regions\":%zu,\"max_forces\":%u,\"max_actors\":%u,\"pacing_"
           "frames\":[%u,%u,%u,%u],\"valid\":%s}\n",
           without_director ? "true" : "false", g->factions.tick, g->factions.launched,
           g->factions.arrived, g->factions.casualties, g->factions.captures, g->factions.retreats,
           g->factions.director.accepted, g->factions.director.rejected, g->factions.count,
           max_forces, max_actors, pressure_frames[0], pressure_frames[1], pressure_frames[2],
           pressure_frames[3], factions_valid(&g->factions, 14) ? "true" : "false");
    performance();
    printf("{\"event\":\"sequence_summary\",\"started\":%u,\"completed\":%u,\"cancelled\":%u,"
           "\"reserved\":%u,\"budget\":%u,\"valid\":%s}\n",
           g->rhythm.next_id, g->rhythm.completed, g->rhythm.cancelled, rhythm_reserved(&g->rhythm),
           g->factions.director.budget,
           rhythm_valid(&g->rhythm, g->factions.director.budget) ? "true" : "false");
    bool ok = frame == duration * 60 && !g->world.failed && !m.save_failures &&
              factions_valid(&g->factions, 14);
    game_destroy(g);
    free(g);
    return ok ? 0 : 1;
}
int main(int argc, char **argv) {
    if (argc < 3 || argc > 5)
        return 2;
    unsigned run = (unsigned)strtoul(argv[1], NULL, 10);
    without_director = argc == 5 && !strcmp(argv[4], "--no-director");
    mkdir(argv[2], 0755);
    unsigned duration = argc >= 4 ? (unsigned)strtoul(argv[3], NULL, 10) : 3600;
    if (run >= 12 || !duration || duration > 7200)
        return 2;
    return journey(run, argv[2], duration);
}
