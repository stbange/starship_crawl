#include "app_internal.h"

void app_inspect_frame(void) {
    if (app.inspect_district) {
        Game *g = &app.game;
        unsigned scene = (unsigned)app.inspect_district - 1;
        DistrictContract district =
            district_contract(g->world.seed, g->world.generator, campaign_site(0));
        Cell cell = scene == 4 ? district.preparation : district.anchor;
        Room *room = world_find(&g->world, cell);
        if (room && room->ready) {
            Player *p = &g->players[0];
            Vec3 c = world_center(cell);
            p->pos = position_from(vadd(c, scene == 1 || scene == 2 ? v3(-23, 3.02, 20)
                                           : scene == 3             ? v3(-24, .02, -26)
                                                                    : v3(0, .02, -35)));
            p->yaw = scene == 1 ? .9 : 0;
            p->pitch = scene == 0 ? .3 : scene == 1 ? .25 : 0;
            if (scene == 2) {
                world_set_systems(&g->world, cell, SYSTEM_POWER);
                district_step(g, .02);
                Vec3 source = vadd(district_control(cell, DISTRICT_SOURCE_CONTROL), v3(0, .75, 0));
                district_shot(g, 0, source);
                Vec3 to = vsub(source, player_eye(p));
                p->yaw = atan2(to.x, to.z);
                p->pitch = asin(vnorm(to).y);
            } else if (scene == 3) {
                district_victory(g, cell);
            } else if (scene == 4) {
                p->pos = position_from(vadd(district_control(cell, JOURNEY_PUMP), v3(0, 0, -2)));
            }
            app.inspect_district = 0;
            app.inspect_static = true;
            app.paused = true;
            g->message_time = 0;
        }
    }
    if (app.inspect_guardian) {
        unsigned id = (unsigned)(app.inspect_guardian - 1) / 3;
        unsigned state = (unsigned)(app.inspect_guardian - 1) % 3;
        Room *room = world_find(&app.game.world, campaign_site(id));
        if (room && room->ready) {
            Game *g = &app.game;
            Player *p = &g->players[0];
            p->pos = position_from(game_terminal(room->cell));
            campaign_interact(g, 0);
            p->pos = position_from(vadd(world_center(room->cell), v3(-4, .02, -15)));
            Vec3 toward = vsub(enemy_center(&room->enemies[1]), player_eye(p));
            p->yaw = atan2(toward.x, toward.z);
            p->pitch = asin(vnorm(toward).y);
            if (state == 1)
                game_damage_enemy(g, room, 1, 0, 1000000, 0);
            else if (state == 2)
                game_damage_enemy(g, room, 0, 0, 1000000, 0);
            else {
                room->enemies[0].windup = 1;
                g->campaign.attempts[id].aim = player_eye(p);
            }
            g->campaign.tracked = id;
            app.inspect_guardian = 0;
            app.inspect_static = true;
            app.paused = true;
            g->message_time = 0;
        }
    }
    /* Explicit earned-build inspection fixtures; ordinary expeditions never enter this path. */
    if (app.inspect_build && world_ready(&app.game.world, (Cell){0})) {
        Game *g = &app.game;
        Player *p = &g->players[0];
        Room *room = world_find(&g->world, (Cell){0});
        for (int i = 0; i < g->world.count; i++)
            g->world.rooms[i].enemy_count = 0;
        room->enemy_count = 3;
        for (unsigned i = 0; i < 3; i++)
            room->enemies[i] = (Enemy){.kind = 1,
                                       .id = (uint8_t)i,
                                       .health = 200,
                                       .alive = true,
                                       .pos = position_from(v3(i ? 3 : -3, .02, 20 + i * 2)),
                                       .normal = {0, 1, 0},
                                       .heading = {0, 0, -1}};
        p->pos = position_from(v3(0, .02, 12));
        p->kills = 3;
        p->weapons |= 2;
        p->yaw = 0;
        p->pitch = 0;
        campaign_grant(g, 0, (CampaignReward){REWARD_MODULE, 1 + 2 * (app.inspect_build - 1)});
        if (app.inspect_build == 1) {
            campaign_grant(g, 0, (CampaignReward){REWARD_GUN, 16});
            campaign_grant(g, 0, (CampaignReward){REWARD_GUN, 19});
            g->campaign.profiles[0].guns[0] = p->weapon = 16;
            game_attach_flare(g, 0, v3(0, .001, 19), v3(0, -1, 0));
            Vec3 origin = vadd(player_eye(p), v3(.16, -.13, 0));
            Vec3 aim = vsub(position_absolute(g->flares[0].pos), origin);
            p->yaw = atan2(aim.x, aim.z);
            p->pitch = atan2(aim.y, hypot(aim.x, aim.z));
        } else if (app.inspect_build == 2) {
            campaign_grant(g, 0, (CampaignReward){REWARD_POWER, PSI_VEIL});
            game_psionic(g, 0, PSI_VEIL);
        } else {
            campaign_grant(g, 0, (CampaignReward){REWARD_RIG, MECH});
            game_rig_toggle(g, 0, MECH);
            game_rig_interact(g, 0);
            p->shield = 0;
            game_hostile_hit(g, 0, 100, 0);
        }
        Actions input[SC_PLAYERS] = {0};
        input[0].fire = app.inspect_build != 3;
        game_step(g, input, SC_TICK_SECONDS);
        if (app.inspect_build == 2) {
            position_move(&p->pos, v3(2, 0, -3));
            p->yaw = -.35;
        }
        game_message(g, "EARNED BUILD INSPECTION / F10 READ BRANCH REQUIREMENTS");
        app.inspect_build = 0;
        app.inspect_static = true;
        app.paused = true;
    }
    if (app.inspect_tentacles && !app.game.world.pending) {
        Player *p = &app.game.players[0];
        Cell focus = world_cell(position_absolute(p->pos));
        for (int i = 0; i < SC_MAX_CHUNKS && app.inspect_tentacles; i++) {
            const Chunk *c = &app.game.world.chunks[i];
            if (!c->active || !cell_equal(c->cell, focus))
                continue;
            for (int j = 0; j < c->dressing_count; j++) {
                const Dressing *d = &c->dressing[j];
                if (d->kind != GROWTH || d->normal.y < .9 || d->scale < .5)
                    continue;
                Vec3 root = vadd(world_center(c->cell), d->pos);
                double distance = clampd(d->scale * 3.5, 2.5, 6);
                Vec3 feet = vadd(root, v3(d->pos.x > 0 ? -distance * .5 : distance * .5,
                                          .02 - d->pos.y, d->pos.z > 0 ? -distance : distance));
                if (!world_spawn_supported(&app.game.world, feet, 1.8) ||
                    world_overlap(&app.game.world, feet, .35, 1.8))
                    continue;
                p->pos = position_from(feet);
                p->yaw = atan2(root.x - feet.x, root.z - feet.z);
                p->pitch = atan2(d->scale * .8 - 1.6, distance * 1.12);
                app.inspect_tentacles = false;
                if (app.game.player_count == 2) {
                    app.game.players[1].pos = p->pos;
                    app.game.players[1].yaw = p->yaw;
                    app.game.players[1].pitch = p->pitch;
                }
                break;
            }
        }
        app.inspect_tentacles = false;
    }
    if (app.inspect_matrix && matrix_near(&app.game, 0) && matrix_toggle(&app.game, 0))
        app.inspect_matrix = false;
    if (app.inspect_alien && !app.game.world.pending) {
        Room *room =
            world_find(&app.game.world, world_cell(position_absolute(app.game.players[0].pos)));
        if (room && room->ready) {
            for (int n = 0; n < app.game.world.count; n++)
                app.game.world.rooms[n].enemy_count = 0;
            room->enemy_count = 1;
            unsigned kind = 31 + (unsigned)app.inspect_alien;
            Vec3 at = vadd(world_center(room->cell), v3(0, .02, 12));
            room->enemies[0] = (Enemy){.kind = (uint8_t)kind,
                                       .behavior = (uint8_t)(ALIEN_LISTENER + kind - 32),
                                       .pos = position_from(at),
                                       .home = at,
                                       .health = sc_enemies[kind].health,
                                       .normal = {0, 1, 0},
                                       .heading = {0, 0, -1},
                                       .body_heading = {0, 0, -1},
                                       .alive = true};
            alien_fragments(&app.game.world, &room->enemies[0]);
            app.inspect_alien = 0;
        }
    }
    if (app.inspect_form && !app.game.world.pending) {
        Room *room = world_find(&app.game.world, (Cell){0, 0, 0});
        if (room) {
            room->enemy_count = 0;
            for (int kind = 0; kind < SC_LEGACY_ENEMIES; kind++)
                if ((kind < 25 || kind >= 27) &&
                    sc_enemy_bodies[kind].form == app.inspect_form - 1) {
                    int at = room->enemy_count++;
                    Enemy *e = &room->enemies[at];
                    *e = (Enemy){.pos = position_from(v3((at - 2) * 3.2, .02, 22)),
                                 .health = sc_enemies[kind].health,
                                 .kind = (uint8_t)kind,
                                 .id = (uint8_t)at,
                                 .normal = {0, 1, 0},
                                 .heading = {0, 0, -1},
                                 .alive = true};
                }
        }
        app.inspect_form = 0;
    }
    if (app.inspect_rig && world_ready(&app.game.world, (Cell){0, 0, 0})) {
        game_rig_toggle(&app.game, 0, (RigKind)app.inspect_rig);
        game_rig_interact(&app.game, 0);
        app.inspect_rig = 0;
    }
    if (app.inspect_shelter && world_ready(&app.game.world, (Cell){0, 0, 0})) {
        game_summon(&app.game, 0);
        game_interact(&app.game, 0);
        app.inspect_shelter = false;
    }
    if (app.demo_encounter) {
        Room *room = world_find(&app.game.world, (Cell){1, 0, 0});
        if (room && room->ready) {
            room->enemy_count = 3;
            const Vec3 positions[] = {{-8, .02, 5}, {8, .02, -8}, {0, .02, 12}};
            for (int i = 0; i < 3; i++) {
                Enemy *e = &room->enemies[i];
                *e = (Enemy){.pos = position_from(vadd(world_center(room->cell), positions[i])),
                             .health = 100,
                             .kind = (uint8_t)i,
                             .id = (uint8_t)i,
                             .normal = {0, 1, 0},
                             .heading = {0, 0, -1},
                             .alive = true};
            }
            room->enemies[0].pos = position_from(
                vadd(world_center(room->cell), v3(-(room->width * .5 - 1.1), .02, -20)));
            app.demo_encounter = false;
        }
    }
}
void app_inspect_scene(const char *scene, uint32_t seed) {
    if (scene) {
        bool build_scene =
            !strncmp(scene, "build", 5) && scene[5] >= '1' && scene[5] <= '3' && !scene[6];
        bool guardian_scene = !strncmp(scene, "guardian", 8) &&
                              (scene[8] == '0' || scene[8] == '1') &&
                              (!strcmp(scene + 9, "warn") || !strcmp(scene + 9, "break") ||
                               !strcmp(scene + 9, "breach"));
        bool district_scene =
            !strncmp(scene, "foundry", 7) && scene[7] >= '0' && scene[7] <= '4' && !scene[8];
        bool court_scene = !strncmp(scene, "court", 5) || !strncmp(scene, "conduit", 7) ||
                           !strncmp(scene, "crown", 5);
        bool frontier_scene = !strncmp(scene, "front", 5) || !strncmp(scene, "deep", 4) ||
                              !strncmp(scene, "reveal", 6) || !strncmp(scene, "alien", 5);
        bool encounter_scene =
            !strcmp(scene, "nest") || !strcmp(scene, "relay") || !strncmp(scene, "local", 5);
        bool habitat_scene = !strncmp(scene, "biome", 5) || !strncmp(scene, "volume", 6) ||
                             !strncmp(scene, "shape", 5) || !strcmp(scene, "tentacles");
        bool traversal_scene =
            !strcmp(scene, "stairs") || !strcmp(scene, "ramp") || !strcmp(scene, "lift");
        bool exterior_scene = !strcmp(scene, "arrival") || !strcmp(scene, "skiff") ||
                              !strcmp(scene, "cabin") || !strcmp(scene, "space") ||
                              !strcmp(scene, "hull");
        /* Historical inspection scenes are reproducible generator-6 sandboxes.
           The guardian preview and ordinary new games use the phased ruleset. */
        if (strcmp(scene, "guardian") && !guardian_scene && !district_scene && !build_scene &&
            !exterior_scene && !traversal_scene && !habitat_scene && !encounter_scene &&
            !frontier_scene && !court_scene) {
            game_new_ship(&app.game, seed);
            world_destroy(&app.game.world);
            world_init(&app.game.world, seed);
            app.game.world.generator = 6;
            app.game.campaign = (Campaign){0};
            for (int i = 0; i < SC_PLAYERS; i++) {
                app.game.players[i].weapons = SC_START_WEAPONS;
                app.game.players[i].rules = 0;
            }
        }
        if (court_scene)
            app.game.world.generator = 13;
        politics_init(&app.game);
        app.game.message_time = 0;
        Cell c = {0, 0, 0};
        if (court_scene) {
            unsigned role = !strncmp(scene, "court", 5)     ? COURT_AUDIENCE
                            : !strncmp(scene, "conduit", 7) ? COURT_CONDUIT
                                                            : COURT_FINALE;
            unsigned nation = (unsigned)(scene[strlen(scene) - 1] - '0') % SC_COUNTRIES;
            c = politics_site(seed, nation, (CourtRole)role);
            app.game.campaign = (Campaign){0};
            app.game.world.open_layer = 0;
            app.no_save = true;
        }
        if (!strcmp(scene, "guardian")) {
            c = campaign_site(0);
            app.no_save = true;
        }
        if (guardian_scene) {
            unsigned id = (unsigned)(scene[8] - '0');
            c = campaign_site(id);
            app.inspect_guardian = (int)(id * 3 + 1 +
                                         (!strcmp(scene + 9, "break")    ? 1
                                          : !strcmp(scene + 9, "breach") ? 2
                                                                         : 0));
            app.no_save = true;
        }
        if (district_scene) {
            unsigned mode = (unsigned)(scene[7] - '0');
            DistrictContract district =
                district_contract(seed, app.game.world.generator, campaign_site(0));
            c = mode == 4 ? district.preparation : district.anchor;
            app.inspect_district = (int)mode + 1;
            app.no_save = true;
        }
        if (!strcmp(scene, "hall"))
            c.z = 1;
        else if (!strcmp(scene, "engine"))
            c.x = 1;
        else if (!strcmp(scene, "encounter")) {
            c.x = 1;
            app.demo_encounter = true;
            app.no_save = true;
        } else if (!strcmp(scene, "crew"))
            c.x = -1;
        else if (!strcmp(scene, "tunnel"))
            c.y = -1;
        else if (!strcmp(scene, "atrium"))
            c.y = 1;
        else if (!strcmp(scene, "distant"))
            c = (Cell){3000000, 2000, 3000000};
        const char *sector_scenes[] = {"luxury", "bridge", "lab", "cargo", "cryonics", "botany"};
        const District sectors[] = {LUXURY, COMMAND, RESEARCH, LOGISTICS, CRYONICS, BOTANY};
        for (int sector = 0; sector < 6; sector++)
            if (!strcmp(scene, sector_scenes[sector])) {
                bool found = false;
                for (int y = 0; y < 9 && !found; y++)
                    for (int z = 0; z < 12 && !found; z++)
                        for (int x = -8; x < 8 && !found; x++) {
                            RoomSpec spec = world_describe_version(seed, app.game.world.generator,
                                                                   (Cell){x, y, z});
                            if (spec.district == sectors[sector] && spec.theme >= 6 &&
                                spec.span_x == 1) {
                                c = spec.cell;
                                found = true;
                            }
                        }
            }
        int inspect_layout = -1, inspect_incident = -1, inspect_landmark = -1, inspect_role = -1;
        if (!strcmp(scene, "origin"))
            inspect_role = HISTORY_ORIGIN;
        if (!strcmp(scene, "waystation"))
            inspect_role = HISTORY_SURVIVORS;
        if (!strcmp(scene, "controller"))
            inspect_role = HISTORY_CONTROL;
        if (!strncmp(scene, "layout", 6) && scene[6] >= '0' && scene[6] <= '2' && !scene[7])
            inspect_layout = scene[6] - '0';
        if (!strncmp(scene, "history", 7) && scene[7] >= '0' && scene[7] <= '5' && !scene[8])
            inspect_incident = scene[7] - '0';
        if (!strncmp(scene, "landmark", 8) && scene[8] >= '1' && scene[8] <= '4' && !scene[9])
            inspect_landmark = scene[8] - '0';
        bool interior_scene = inspect_layout >= 0 || inspect_incident >= 0 ||
                              inspect_landmark >= 0 || inspect_role >= 0;
        bool found = false;
        if (inspect_role == HISTORY_ORIGIN) {
            c = world_history(seed).origin;
            found = true;
        }
        for (int y = 0; interior_scene && y < 9 && !found; y++)
            for (int z = 0; z < 24 && !found; z++)
                for (int x = -12; x < 12 && !found; x++) {
                    RoomSpec spec =
                        world_describe_version(seed, app.game.world.generator, (Cell){x, y, z});
                    if (spec.type != BAY && spec.height >= 3 &&
                        (inspect_role < 0 || spec.history_role == inspect_role) &&
                        (inspect_layout < 0 ||
                         (spec.layout == inspect_layout && spec.span_x == 1)) &&
                        (inspect_incident < 0 ||
                         (spec.incident == inspect_incident && spec.span_x == 1)) &&
                        (inspect_landmark < 0 || spec.landmark == inspect_landmark)) {
                        c = spec.cell;
                        found = true;
                    }
                }
        app.game.players[0].pos =
            position_from(vadd(world_center(c), v3(0, .02, court_scene ? 0 : -25)));
        if (!strncmp(scene, "form", 4) && scene[4] >= '0' && scene[4] < '0' + SC_BODY_FORMS &&
            !scene[5]) {
            app.game.players[0].pos = position_from(v3(0, .02, 14));
            app.inspect_form = scene[4] - '0' + 1;
            app.no_save = true;
        }
        if (!strcmp(scene, "mech") || !strcmp(scene, "wagon")) {
            app.game.players[0].pos = position_from(v3(0, .02, 12));
            app.inspect_rig = !strcmp(scene, "mech") ? MECH : WAGON;
            app.no_save = true;
        }
        if (!strcmp(scene, "matrix")) {
            app.game.players[0].pos = position_from(matrix_terminal((Cell){0}));
            app.inspect_matrix = true;
            app.no_save = true;
        }
        if (!strcmp(scene, "shelter")) {
            app.game.players[0].pos = position_from(v3(0, .02, 12));
            app.inspect_shelter = true;
            app.no_save = true;
        }
        app.game.players[0].crouched =
            world_describe_version(seed, app.game.world.generator, c).type == CONDUIT;
        app.game.players[0].pitch = .03;
        if (interior_scene) {
            app.game.players[0].pos = position_from(vadd(world_center(c), v3(0, .02, 0)));
            app.game.players[0].yaw = inspect_role >= 0 ? -2.36 : .78;
            app.game.players[0].pitch = inspect_landmark >= 0 ? .26 : .015;
        }
        if (habitat_scene) {
            bool found = false;
            for (int y = -4; y <= 4 && !found; y++)
                for (int rz = 1; rz < 5 && !found; rz++)
                    for (int rx = -4; rx <= 4 && !found; rx++) {
                        RegionPlan plan;
                        world_plan_version(seed, SC_GENERATOR_VERSION, (Cell){rx, y, rz}, &plan);
                        for (int i = 0; i < plan.count && !found; i++) {
                            RoomSpec room = plan.rooms[i];
                            int desired = atoi(scene + (!strncmp(scene, "volume", 6) ? 6 : 5));
                            bool match =
                                !strcmp(scene, "tentacles")
                                    ? (room.incident == BLOOM || room.district == BOTANY) &&
                                          room.height >= 8 && !room.habitat.volume &&
                                          room.span_x == 1 && room.span_z == 1
                                : !strncmp(scene, "biome", 5)  ? room.habitat.atmosphere == desired
                                : !strncmp(scene, "volume", 6) ? room.habitat.volume == desired
                                                               : room.habitat.form == desired;
                            if (match && room.height >= 3) {
                                c = room.cell;
                                found = true;
                            }
                        }
                    }
            Player *p = &app.game.players[0];
            p->pos = position_from(vadd(world_center(c), v3(0, .02, 0)));
            p->checkpoint = position_from(world_lift(c));
            p->yaw = .4;
            p->pitch = .05;
            if (!strncmp(scene, "volume", 6)) {
                if (scene[6] == '2') {
                    p->pos = position_from(vadd(world_center(c), v3(9.2, .02, 24)));
                    p->yaw = SC_PI * .5;
                    p->pitch = -.8;
                } else {
                    p->pos = position_from(vadd(world_center(c), v3(0, .02, 18)));
                    p->yaw = .7;
                    p->pitch = .52;
                }
            }
            app.game.campaign = (Campaign){0};
            app.game.world.open_layer = 0;
            app.no_save = true;
            app.inspect_tentacles = !strcmp(scene, "tentacles");
        }
        if (encounter_scene) {
            app.game.world.generator = 11;
            bool found = false;
            RoomSpec selected_room = {0};
            bool local_scene = !strncmp(scene, "local", 5);
            for (int y = -3; y <= 3 && !found; y++)
                for (int z = 1; z <= 5 && !found; z++)
                    for (int x = -5; x <= 5 && !found; x++) {
                        RegionPlan plan;
                        world_plan_version(seed, app.game.world.generator, (Cell){x, y, z}, &plan);
                        for (int i = 0; i < plan.count && !found; i++) {
                            RoomSpec a = plan.rooms[i];
                            bool match = local_scene
                                             ? (a.habitat.pace == PACE_QUIET ||
                                                a.habitat.pace == PACE_TRACE) &&
                                                   a.habitat.tale == (unsigned)atoi(scene + 5)
                                             : a.habitat.pace == PACE_FRONTIER &&
                                                   (a.habitat.ecology == MECHANICAL) ==
                                                       !strcmp(scene, "relay");
                            if (match && a.height >= 5 && a.width >= 40 && !a.habitat.volume) {
                                selected_room = a;
                                c = a.cell;
                                found = true;
                            }
                        }
                    }
            Player *p = &app.game.players[0];
            p->pos = position_from(
                vadd(world_center(c),
                     local_scene ? v3(selected_room.hash & 1 ? 18 : -18, .02, 2) : v3(0, .02, 0)));
            p->checkpoint = p->pos;
            p->yaw = 0;
            p->pitch = -.03;
            app.game.campaign = (Campaign){0};
            app.game.world.open_layer = 0;
            app.no_save = true;
        }
        if (frontier_scene) {
            world_destroy(&app.game.world);
            world_init(&app.game.world, seed);
            app.game.world.generator = 12;
            bool found = false;
            RoomSpec chosen = {0};
            unsigned desired = (unsigned)atoi(scene + (!strncmp(scene, "reveal", 6) ? 6
                                                       : !strncmp(scene, "deep", 4) ? 4
                                                                                    : 5));
            for (int y = -4; y <= 4 && !found; y++)
                for (int z = 1; z <= 7 && !found; z++)
                    for (int x = -7; x <= 7 && !found; x++) {
                        RegionPlan plan;
                        world_plan_version(seed, 12, (Cell){x, y, z}, &plan);
                        for (int i = 0; i < plan.count && !found; i++) {
                            RoomSpec a = plan.rooms[i];
                            bool match = !strncmp(scene, "reveal", 6)
                                             ? a.habitat.reveal == desired && a.habitat.form
                                         : !strncmp(scene, "alien", 5)
                                             ? a.habitat.pace == PACE_QUIET && a.height >= 10
                                         : !strncmp(scene, "deep", 4)
                                             ? a.habitat.contract == desired + 1
                                             : a.habitat.pace == PACE_FRONTIER &&
                                                   a.habitat.challenge == desired;
                            if (match && a.height >= 10 && a.width >= 50 && !a.habitat.volume) {
                                chosen = a;
                                c = a.cell;
                                found = true;
                            }
                        }
                    }
            Player *p = &app.game.players[0];
            p->pos = position_from(vadd(world_center(c), v3(0, .02, 0)));
            p->checkpoint = p->pos;
            p->yaw = 0;
            p->pitch = -.02;
            if (!strncmp(scene, "reveal", 6)) {
                p->yaw = 0;
                p->pitch = atan2(fmax(10, chosen.height * .60) - 1.62, 26);
            }
            if (!strncmp(scene, "alien", 5) && desired < 10)
                app.inspect_alien = (int)desired + 1;
            app.game.campaign = (Campaign){0};
            app.game.world.open_layer = 0;
            app.no_save = true;
        }
        if (traversal_scene) {
            Player *p = &app.game.players[0];
            p->pos = position_from(!strcmp(scene, "lift")
                                       ? world_lift((Cell){0, 0, 0})
                                       : v3(!strcmp(scene, "stairs") ? 6 : -6, .02, 17));
            p->yaw = 0;
            p->pitch = .25;
            app.no_save = true;
        }
        if (exterior_scene) {
            Player *p = &app.game.players[0];
            p->pos = position_from(sc_arrival_spawn);
            p->pitch = -.04;
            p->crouched = false;
            if (!strcmp(scene, "skiff")) {
                p->pos = position_from(v3(18, 10, -68));
                p->yaw = atan2(-18, -30);
                p->pitch = -.15;
            } else if (!strcmp(scene, "cabin")) {
                p->pos = position_from(v3(0, 4, -97));
                p->yaw = SC_PI;
                p->pitch = -.08;
            } else if (!strcmp(scene, "space")) {
                p->yaw = SC_PI;
                p->pitch = .65;
            } else if (!strcmp(scene, "hull")) {
                p->yaw = .9;
                p->pitch = .75;
            }
            app.no_save = true;
        }
        if (build_scene) {
            app.inspect_build = scene[5] - '0';
            app.game.players[0].pos = position_from(v3(0, .02, 12));
            app.no_save = true;
        }
        politics_init(&app.game);
        if (app.game.world.generator < 15) {
            matrix_destroy(&app.game.matrix);
            matrix_init(&app.game.matrix);
        }
        Vec3 p = position_absolute(app.game.players[0].pos);
        world_refresh(&app.game.world, &p, 1);
    }
}
