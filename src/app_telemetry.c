#include "app_internal.h"

EMSCRIPTEN_KEEPALIVE uint32_t sc_web_fingerprint(uint32_t seed, int x, int y, int z) {
    if (x < -2000000 || x > 2000000 || y < -1000000 || y > 1000000 || z < 0 || z > 2000000)
        return 0;
    return world_fingerprint(seed, (Cell){x, y, z});
}
EMSCRIPTEN_KEEPALIVE const char *sc_web_state(void) {
    static char result[32768];
    const Player *p = &app.game.players[0];
    Vec3 feet = position_absolute(p->pos);
    unsigned projectiles = 0, climbers = 0, bodies = 0;
    for (int i = 0; i < SC_MAX_PROJECTILES; i++)
        projectiles += app.game.projectiles[i].active;
    for (int i = 0; i < app.game.world.count; i++)
        for (int j = 0; j < app.game.world.rooms[i].enemy_count; j++)
            climbers += app.game.world.rooms[i].enemies[j].climbing;
    for (int r = 0; r < app.game.world.count; r++)
        for (int j = 0; j < app.game.world.rooms[r].enemy_count; j++) {
            const Enemy *e = &app.game.world.rooms[r].enemies[j];
            if (e->alive && cell_equal(world_cell(position_absolute(e->pos)), world_cell(feet))) {
                EnemyBody body = sc_enemy_bodies[e->kind];
                bodies |= 1u << (body.form * 5 + body.variant);
            }
        }
    unsigned corpses = 0, death_forms = 0, hazards = 0, hazard_kinds = 0, triggered = 0, spent = 0;
    float death_age = 0;
    for (int i = 0; i < SC_MAX_DEATHS; i++)
        if (app.game.deaths[i].active) {
            corpses++;
            death_forms |= 1u << sc_enemy_bodies[app.game.deaths[i].body.kind].form;
            death_age = fmaxf(death_age, app.game.deaths[i].age);
        }
    for (int i = 0; i < SC_MAX_DEATH_HAZARDS; i++)
        if (app.game.death_hazards[i].active) {
            hazards++;
            hazard_kinds |= 1u << app.game.death_hazards[i].kind;
            triggered += app.game.death_hazards[i].triggered;
            spent += app.game.death_hazards[i].spent;
        }
    unsigned flares = 0, beams = 0;
    float flare_life = 0, flare_power = 0;
    for (int i = 0; i < SC_MAX_FLARES; i++)
        if (app.game.flares[i].active) {
            flares++;
            flare_life = fmaxf(flare_life, app.game.flares[i].life);
            flare_power = fmaxf(flare_power, app.game.flares[i].power);
        }
    for (int i = 0; i < SC_PLAYERS; i++)
        beams += app.game.beams[i].life > 0;
    float hull = p->riding ? app.game.rigs[0][p->riding - 1].armor : 0;
    Room *room = world_find(&app.game.world, world_cell(feet));
    Cell objective = {0};
    bool has_goal = game_objective(&app.game, &objective);
    ShipHistory past = world_history(app.game.world.seed);
    snprintf(
        result, sizeof(result),
        "{\"seed\":%u,\"players\":%d,\"paused\":%s,\"frames\":%d,\"rooms\":%d,"
        "\"x\":%.5f,\"y\":%.5f,\"z\":%.5f,\"rank\":%u,\"kills\":%u,\"shots\":%u,\"failed\":%s,"
        "\"generator\":%u,\"form\":%u,\"volume\":%u,\"atmosphere\":%u,\"ecology\":%u,\"tale\":%u,"
        "\"sprites\":%u,\"sprites2\":%u,\"tentacles\":%u,\"tentacles2\":%u,"
        "\"room_height\":%.1f,\"chunks\":%d,\"pending\":%d,"
        "\"generated\":%u,\"work\":%u,\"uploads\":%u,"
        "\"span\":%d,\"ready\":%s,\"fingerprint\":%u,\"flashlight\":%s,\"projectiles\":%u,\"enemy_"
        "shots\":%u,\"projectile_hits\":%u,\"climbers\":%u,\"mission\":%u,\"weapon\":%u,"
        "\"weapons\":%u,\"shield\":%.2f,\"armor\":%.2f,\"energy\":%.2f,\"credits\":%u,\"pickups\":%"
        "u,\"contracts\":%u,\"theme\":%u,\"logs\":%u,\"yaw\":%.4f,\"pitch\":%.4f,\"time\":%.3f,"
        "\"sheltered\":%s,\"shelter_deployed\":%s,\"sx\":%.3f,\"sz\":%.3f,\"resting\":%.2f,"
        "\"nourished\":%.2f,\"health\":%.2f,\"facility\":%d,"
        "\"map\":%s,\"map_x\":%d,\"map_y\":%d,\"map_z\":%d,\"veil\":%s,\"flight\":%.2f,\"leap\":%."
        "2f,\"nova\":%.2f,\"power\":%u,\"bodies\":%u,\"key_w\":%u,\"riding\":%u,\"rig_weapon\":%u,"
        "\"hull\":%.2f,\"drive_speed\":%.2f,\"mech_deployed\":%s,\"wagon_deployed\":%s,\"beams\":%"
        "u,\"flares\":%u,\"flare_lifetime\":%u,\"flare_strength\":%u,\"flare_life\":%.2f,\"flare_"
        "power\":%.2f,\"points\":%u,\"profile\":%u,\"layout\":%u,\"incident\":%u,\"intensity\":%u,"
        "\"landmark\":%u,\"room_depth\":%.1f,\"cache_x\":%.3f,\"cache_z\":%.3f,"
        "\"story_version\":%u,\"motivation\":%u,\"story_assigned\":%u,\"story_seen\":%u,"
        "\"story_done\":%u,\"story_choice\":%u,\"story_tracked\":%u,\"story_reading\":%u,"
        "\"journal\":%s,\"journal_page\":%u,\"decision\":%s,\"has_goal\":%s,"
        "\"goal_x\":%d,\"goal_y\":%d,\"goal_z\":%d,\"victories\":%u,"
        "\"choice1\":\"%s\",\"choice2\":\"%s\",\"choice3\":\"%s\","
        "\"prepared\":%s,\"history_cause\":%u,\"history_arc\":%u,\"history_role\":%u,"
        "\"origin_x\":%d,\"origin_y\":%d,\"origin_z\":%d,"
        "\"council\":%u,\"support0\":%u,\"support1\":%u,\"chapter\":%u,\"arc_kind\":%u,\"chapter_"
        "choice\":%u,"
        "\"remaining\":%u,\"events\":%u,\"systems\":%u,\"safe\":%s,\"event_reading\":%u,"
        "\"last0\":%u,\"last1\":%u,\"last2\":%u,\"last3\":%u,"
        "\"total0\":%u,\"total1\":%u,\"total2\":%u,\"total3\":%u,\"total4\":%u,"
        "\"career0\":%u,\"career1\":%u,\"career2\":%u,\"career3\":%u,\"career4\":%u,"
        "\"corpses\":%u,\"death_forms\":%u,\"death_age\":%.3f,\"hazards\":%u,"
        "\"hazard_kinds\":%u,\"hazard_triggered\":%u,\"hazard_spent\":%u,\"respawns\":%u}",
        app.game.world.seed, app.game.player_count, app.paused ? "true" : "false", app.frame,
        app.game.world.count, feet.x, feet.y, feet.z, p->level, p->kills, app.game.shots,
        app.failed ? "true" : "false", app.game.world.generator, room ? room->habitat.form : 0,
        room ? room->habitat.volume : 0, room ? room->habitat.atmosphere : 0,
        room ? room->habitat.ecology : 0, room ? room->habitat.tale : 0,
        app.renderer.sprite_drawn[0], app.renderer.sprite_drawn[1], app.renderer.tentacle_drawn[0],
        app.renderer.tentacle_drawn[1], room ? room->height : 0, app.game.world.chunk_count,
        app.game.world.pending, app.game.world.generated, app.game.world.last_work,
        app.renderer.last_uploads, room ? room->span_x * room->span_z : 0,
        room && room->ready ? "true" : "false",
        world_fingerprint_version(app.game.world.seed, app.game.world.generator,
                                  world_region(world_cell(feet))),
        p->flashlight ? "true" : "false", projectiles, app.game.enemy_shots,
        app.game.projectile_hits, climbers, app.game.mission, p->weapon, p->weapons, p->shield,
        p->armor, p->energy, p->credits, p->pickups, app.game.contracts, room ? room->theme : 0,
        p->logs, p->yaw, p->pitch, app.game.time, p->sheltered ? "true" : "false",
        app.game.shelter.deployed ? "true" : "false", p->shelter_pos.x, p->shelter_pos.z,
        p->resting, p->nourished, p->health, (int)game_shelter_near(&app.game, 0),
        PLAYER_VIEW(&app.game, p).map_open ? "true" : "false",
        PLAYER_VIEW(&app.game, p).map_center.x, PLAYER_VIEW(&app.game, p).map_center.y,
        PLAYER_VIEW(&app.game, p).map_center.z, p->cloaked ? "true" : "false", p->flight, p->leap,
        p->nova, p->selected_power, bodies, (unsigned)SDL_GetKeyboardState(NULL)[SDL_SCANCODE_W],
        p->riding, p->rig_weapon, hull, p->drive_speed,
        app.game.rigs[0][0].deployed ? "true" : "false",
        app.game.rigs[0][1].deployed ? "true" : "false", beams, flares, p->flare_lifetime,
        p->flare_strength, flare_life, flare_power, p->points, world_profile(app.game.world.seed),
        room ? room->layout : 0, room ? room->incident : 0, room ? room->intensity : 0,
        room ? room->landmark : 0, room ? room->depth : 0,
        room && room->ready ? position_absolute(room->pickups[3].pos).x : 0,
        room && room->ready ? position_absolute(room->pickups[3].pos).z : 0, app.game.story.version,
        story_motivation_version(app.game.world.seed, app.game.story.version),
        app.game.story.assigned, app.game.story.seen, app.game.story.done, app.game.story.choice,
        app.game.story.tracked, PLAYER_VIEW(&app.game, p).story_reading,
        PLAYER_VIEW(&app.game, p).journal ? "true" : "false",
        PLAYER_VIEW(&app.game, p).journal_page,
        (PLAYER_INTERACTION(&app.game, p).decision_open || app.game.matrix.divers[0].choice ||
         app.game.politics.audience[0])
            ? "true"
            : "false",
        has_goal ? "true" : "false", objective.x, objective.y, objective.z, p->victories,
        game_story_choice(&app.game, 1), game_story_choice(&app.game, 2),
        game_story_choice(&app.game, 3), game_story_choice_ready(&app.game, 0) ? "true" : "false",
        past.cause, past.arc, room ? room->history_role : 0, past.origin.x, past.origin.y,
        past.origin.z, app.game.story.council, app.game.story.support[0], app.game.story.support[1],
        app.game.story.chapter, app.game.story.arc, app.game.story.chapter_choice,
        app.game.story.remaining, app.game.story.event_count,
        world_systems(&app.game.world, world_cell(feet)),
        game_story_safe(&app.game, feet) ? "true" : "false",
        PLAYER_VIEW(&app.game, p).event_reading, app.game.story.last[0], app.game.story.last[1],
        app.game.story.last[2], app.game.story.last[3], app.game.story.totals[0],
        app.game.story.totals[1], app.game.story.totals[2], app.game.story.totals[3],
        app.game.story.totals[4], p->career[0], p->career[1], p->career[2], p->career[3],
        p->career[4], corpses, death_forms, death_age, hazards, hazard_kinds, triggered, spent,
        app.game.respawns);
    size_t used = strlen(result);
    const MatrixDiver *d = &app.game.matrix.divers[0];
    const MatrixProfile *mp = &app.game.matrix.profiles[0];
    const MatrixNetwork *mw = &app.game.matrix.networks[d->active ? d->network : 0];
    const MatrixRecord *mr = mw->active ? matrix_find(&app.game.matrix, mw->key) : NULL;
    Vec3 goal = {0};
    matrix_goal(&app.game, 0, &goal);
    unsigned guards = 0, shots = 0;
    for (int i = 0; i < SC_MATRIX_GUARDS; i++)
        guards += mw->guards[i].active;
    for (int i = 0; i < SC_MATRIX_SHOTS; i++)
        shots += mw->shots[i].active;
    used--;
    int n =
        snprintf(result + used, sizeof(result) - used,
                 ",\"matrix\":%s,\"matrix_near\":%s,\"mx\":%.4f,\"my\":%.4f,\"mz\":%.4f,"
                 "\"integrity\":%.2f,\"bandwidth\":%.2f,\"trace\":%.2f,\"matrix_map\":%s,"
                 "\"matrix_choice\":%s,"
                 "\"programs\":%u,\"slot0\":%u,\"slot1\":%u,\"matrix_selected\":%u,\"matrix_"
                 "data\":%u,\"matrix_service\":%u,"
                 "\"matrix_keys\":%u,\"matrix_flags\":%u,\"matrix_outcome\":%u,\"matrix_seen\":"
                 "%u,\"matrix_killed\":%u,"
                 "\"matrix_entry\":%u,\"matrix_core\":%u,\"matrix_credential\":%u,\"matrix_"
                 "vault\":%u,\"matrix_rare\":%s,"
                 "\"matrix_theme\":%u,\"matrix_guards\":%u,\"matrix_shots\":%u,\"matrix_"
                 "switch0\":%.2f,\"matrix_switch1\":%.2f,"
                 "\"matrix_goal_x\":%.2f,\"matrix_goal_z\":%.2f,\"matrix_veil\":%.2f,\"matrix_"
                 "lock\":%.2f,\"matrix_nodes\":[",
                 d->active ? "true" : "false", matrix_near(&app.game, 0) ? "true" : "false",
                 d->pos.x, d->pos.y, d->pos.z, d->integrity, d->bandwidth, d->trace,
                 DIVER_VIEW(&app.game, d).matrix_map ? "true" : "false",
                 d->choice ? "true" : "false", mp->programs, mp->slots[0], mp->slots[1],
                 d->selected, mp->data, d->service, mr ? mr->keys : 0, mr ? mr->flags : 0,
                 mr ? mr->outcome : 0, mr ? mr->seen : 0, mr ? mr->killed : 0, mw->entry, mw->core,
                 mw->credential, mw->vault, mw->rare ? "true" : "false", mw->theme, guards, shots,
                 mw->switches[0], mw->switches[1], goal.x, goal.z, d->veil, d->lockout);
    if (n > 0 && (size_t)n < sizeof(result) - used)
        used += (size_t)n;
    bool first = true;
    for (unsigned i = 0; i < 16; i++)
        if (mw->nodes[i].valid) {
            const MatrixNode *node = &mw->nodes[i];
            n = snprintf(result + used, sizeof(result) - used,
                         "%s{\"id\":%u,\"x\":%.2f,\"z\":%.2f,\"links\":%u,\"room_x\":%d,\"room_y\":"
                         "%d,\"room_z\":%d}",
                         first ? "" : ",", i, node->at.x, node->at.z, node->links, node->room.x,
                         node->room.y, node->room.z);
            if (n > 0 && (size_t)n < sizeof(result) - used)
                used += (size_t)n;
            first = false;
        }
    const Campaign *campaign = &app.game.campaign;
    const CampaignProfile *build = &campaign->profiles[0];
    unsigned selected = campaign->tracked < SC_GUARDIANS ? campaign->tracked : 0;
    Room *guardian_room = world_find(&app.game.world, campaign_site(selected));
    char offers[3][110];
    for (unsigned i = 0; i < 3; i++)
        campaign_reward_name(&app.game, 0, campaign->choice[0] ? campaign->choice[0] - 1 : selected,
                             i, offers[i], sizeof(offers[i]));
    snprintf(result + used, sizeof(result) - used,
             "],\"campaign\":%u,\"phase\":%u,\"defeated\":%u,\"guardian\":%u,"
             "\"guardian_active\":%s,\"guardian_health\":%.2f,\"guardian_stage\":%u,"
             "\"guardian_broken\":%u,\"reward_pending\":%u,"
             "\"powers_owned\":%u,\"rigs_owned\":%u,\"modules_owned\":%u,\"claimed\":%u,"
             "\"gun_slots\":[%u,%u],\"power_slots\":[%u,%u],\"rig_slot\":%u,\"module\":%u,"
             "\"build_slot\":%u,\"offer1\":\"%s\",\"offer2\":\"%s\",\"offer3\":\"%s\"}",
             campaign->version, campaign_phase(campaign), campaign->defeated, campaign->tracked,
             campaign->attempts[selected].active ? "true" : "false",
             guardian_room && guardian_room->ready ? guardian_room->enemies[0].health : 0,
             campaign->attempts[selected].stage, campaign->attempts[selected].broken,
             campaign->choice[0], build->powers, build->rigs, build->modules, build->claimed,
             build->guns[0], build->guns[1], build->power_slots[0], build->power_slots[1],
             build->rig, build->module, app.game.views[0].loadout_slot, offers[0], offers[1],
             offers[2]);
    used = strlen(result) - 1;
    snprintf(result + used, sizeof(result) - used,
             ",\"capabilities\":%u,\"intro\":%u,\"lesson\":%u,\"lessons\":%u,\"guidance\":%u,"
             "\"interface_ready\":%s}",
             onboarding_capabilities(&app.game, 0), p->intro, PLAYER_VIEW(&app.game, p).lesson,
             p->lessons, app.renderer.preferences.view[0].guidance,
             onboarding_interface_ready(&app.game, 0) ? "true" : "false");
    used = strlen(result) - 1;
    used += (size_t)snprintf(result + used, sizeof(result) - used, ",\"actions\":[");
    bool first_action = true;
    for (int action = 1; action <= 241; action++)
        if (onboarding_action(&app.game, 0, action)) {
            used += (size_t)snprintf(result + used, sizeof(result) - used, "%s%d",
                                     first_action ? "" : ",", action);
            first_action = false;
        }
    snprintf(result + used, sizeof(result) - used, "]}");
    used = strlen(result) - 1;
    const Navigation *nav = &p->navigation;
    snprintf(result + used, sizeof(result) - used,
             ",\"nav_kind\":%u,\"nav_next\":[%d,%d,%d],\"nav_point\":[%.3f,%.3f,%.3f],"
             "\"nav_searched\":%u,\"lift_latched\":%s,\"arrival\":%.2f,\"nav2_kind\":%u}",
             nav->kind, nav->next.x, nav->next.y, nav->next.z, nav->point.x, nav->point.y,
             nav->point.z, nav->searched, p->lift_latched ? "true" : "false", p->arrival_time,
             app.game.players[1].navigation.kind);
    used = strlen(result) - 1;
    Room *local = world_find(&app.game.world, world_cell(feet));
    unsigned living = 0, behaviors = 0, motions = 0;
    if (local)
        for (int i = 0; i < local->enemy_count; i++) {
            const Enemy *e = &local->enemies[i];
            living += e->alive;
            if (e->alive) {
                behaviors |= 1u << e->behavior;
                motions |= 1u << e->motion;
            }
        }
    char offer[120] = "";
    if (local && local->habitat.pace == PACE_FRONTIER)
        encounter_reward_name(&app.game, 0, local, offer, sizeof(offer));
    snprintf(result + used, sizeof(result) - used,
             ",\"pace\":%u,\"local_state\":%u,\"frontier_active\":%s,\"frontier_living\":%u,"
             "\"frontier_core\":%.2f,\"frontier_attacker\":%u,\"frontier_disrupt\":%.3f,"
             "\"alien_behaviors\":%u,\"alien_motions\":%u,\"frontier_offer\":\"%s\","
             "\"drill_time\":%.2f,\"drill_complete\":%s}",
             local ? local->habitat.pace : 0, world_local_state(&app.game.world, local),
             local && local->frontier_active ? "true" : "false", living,
             local && local->enemy_count ? local->enemies[0].health : 0,
             local ? local->frontier_attacker : 0, local ? local->frontier_disrupt : 0, behaviors,
             motions, offer, app.game.drills[0].time,
             app.game.drills[0].complete ? "true" : "false");
    used = strlen(result) - 1;
    Vec3 console = local ? encounter_console(local) : v3(0, 0, 0);
    snprintf(result + used, sizeof(result) - used,
             ",\"encounter_console\":[%.3f,%.3f,%.3f],\"regional_enemies\":[", console.x, console.y,
             console.z);
    if (local && local->habitat.pace == PACE_FRONTIER)
        for (int i = 0; i < 4; i++) {
            const Enemy *e = &local->enemies[i];
            Vec3 at = enemy_center(e);
            used = strlen(result);
            snprintf(result + used, sizeof(result) - used,
                     "%s{\"at\":[%.3f,%.3f,%.3f],\"alive\":%s,\"motion\":%u,\"health\":%.2f}",
                     i ? "," : "", at.x, at.y, at.z, e->alive ? "true" : "false", e->motion,
                     e->health);
        }
    used = strlen(result);
    snprintf(result + used, sizeof(result) - used, "]}");
    used = strlen(result) - 1;
    snprintf(result + used, sizeof(result) - used,
             ",\"challenge\":%u,\"reveal\":%u,\"contract\":%u,\"circuit\":%u,\"hazard_time\":%.3f,"
             "\"relics\":[%u,%u,%u,%u,%u],\"specialists\":[",
             local ? local->habitat.challenge : 0, local ? local->habitat.reveal : 0,
             local ? local->habitat.contract : 0, local ? local->circuit : 0,
             local ? local->hazard_time : 0, app.game.frontier.ranks[0][0],
             app.game.frontier.ranks[0][1], app.game.frontier.ranks[0][2],
             app.game.frontier.ranks[0][3], app.game.frontier.ranks[0][4]);
    if (local)
        for (int i = 0; i < local->enemy_count; i++) {
            const Enemy *e = &local->enemies[i];
            Vec3 at = enemy_center(e);
            used = strlen(result);
            snprintf(result + used, sizeof(result) - used,
                     "%s{\"kind\":%u,\"behavior\":%u,\"motion\":%u,\"alive\":%s,\"observed\":%s,"
                     "\"molted\":%s,\"unfold\":%.3f,\"sound\":%.3f,\"at\":[%.3f,%.3f,%.3f]}",
                     i ? "," : "", e->kind, e->behavior, e->motion, e->alive ? "true" : "false",
                     e->observed ? "true" : "false", e->molted ? "true" : "false", e->unfold,
                     e->warning_sound, at.x, at.y, at.z);
        }
    used = strlen(result);
    snprintf(result + used, sizeof(result) - used, "]}");
    used = strlen(result) - 1;
    Politics *pol = &app.game.politics;
    Director *dir = &app.game.director;
    unsigned pn = app.game.views[0].country_reading % SC_COUNTRIES;
    snprintf(result + used, sizeof(result) - used,
             ",\"politics\":%u,\"country\":%u,\"court\":%u,\"court_reading\":%u,\"audience\":%u,"
             "\"pledge\":%u,\"court_stage\":%u,\"political_goal\":%u,\"director_kind\":%u,"
             "\"director_sequence\":%u,\"director_cooldown\":%u,\"director_remaining\":%u}",
             pol->version, politics_owner(&app.game, world_cell(feet)),
             local ? local->habitat.court : 0, pn, pol->audience[0], pol->countries[pn].pledge,
             pol->countries[pn].stage, pol->tracked, dir->kind, dir->sequence, dir->cooldown,
             dir->remaining);
    used = strlen(result) - 1;
    snprintf(result + used, sizeof(result) - used,
             ",\"court_stages\":[%u,%u,%u],\"court_pledges\":[%u,%u,%u],\"court_claims\":[%u,%u,%u]"
             ",\"director_reason\":%u}",
             pol->countries[0].stage, pol->countries[1].stage, pol->countries[2].stage,
             pol->countries[0].pledge, pol->countries[1].pledge, pol->countries[2].pledge,
             pol->countries[0].claimed, pol->countries[1].claimed, pol->countries[2].claimed,
             dir->reasons[(dir->sequence + SC_DIRECTOR_LOG - 1) % SC_DIRECTOR_LOG]);
    Factions *f = &app.game.factions;
    used = strlen(result) - 1;
    snprintf(result + used, sizeof(result) - used,
             ",\"simulation\":%u,\"strategy_tick\":%u,\"forces_launched\":%u,\"forces_arrived\":%u,"
             "\"force_losses\":%u,\"captures\":%u,\"retreats\":%u,\"interventions\":%u,"
             "\"rejections\":%u,"
             "\"pressure\":[%.3f,%.3f],\"pacing\":[%u,%u],\"regions\":%zu,\"forces\":[",
             f->version, f->tick, f->launched, f->arrived, f->casualties, f->captures, f->retreats,
             f->director.accepted, f->director.rejected, f->director.pressure[0],
             f->director.pressure[1], f->director.pace[0], f->director.pace[1], f->count);
    unsigned printed = 0;
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        const FactionForce *force = &f->forces[i];
        if (!force->id)
            continue;
        unsigned member = 0;
        while (member + 1 < force->count && !force->members[member].alive)
            member++;
        Vec3 point = enemy_center(&force->members[member]);
        used = strlen(result);
        snprintf(
            result + used, sizeof(result) - used,
            "%s{\"id\":%u,\"country\":%u,\"order\":%u,\"strength\":%u,\"physical\":%s,\"cargo\":%u,"
            "\"at\":[%.2f,%.2f,%.2f],\"goal\":[%d,%d,%d]}",
            printed++ ? "," : "", force->id, force->country, force->order, factions_strength(force),
            force->physical ? "true" : "false", force->cargo, point.x, point.y, point.z,
            force->goal.x, force->goal.y, force->goal.z);
    }
    used = strlen(result);
    snprintf(result + used, sizeof(result) - used, "]}");
    used = strlen(result) - 1;
    unsigned relays = 0;
    for (unsigned i = 0; i < 3; i++)
        relays += app.game.builds[0].relays[i].life > 0;
    snprintf(result + used, sizeof(result) - used,
             ",\"rules\":%u,\"invested\":%u,\"relays\":%u,\"echo_life\":%.3f,\"mech_charge\":%.2f,"
             "\"branch_preview\":%u}",
             p->rules, player_investment(p), relays, app.game.builds[0].echo_life,
             app.game.builds[0].charge, app.game.views[0].branch_preview);
    used = strlen(result) - 1;
    const Room *district_room = world_find(&app.game.world, world_cell(position_absolute(p->pos)));
    snprintf(result + used, sizeof(result) - used,
             ",\"district_family\":%u,\"district_condition\":%u,\"district_role\":%u,"
             "\"district_power\":%s,\"district_warning\":%.3f,\"district_arc\":%.3f,\"district_"
             "facts\":%u}",
             district_room ? district_room->journey.family : 0,
             district_room ? district_room->journey.condition : 0,
             district_room ? district_room->journey.role : 0,
             district_room && district_room->district_powered ? "true" : "false",
             district_room ? district_room->district_warning : 0,
             district_room ? district_room->district_arc : 0,
             district_room ? world_district_state(&app.game.world, district_room->journey.anchor)
                           : 0);
    used = strlen(result) - 1;
    int near_person = chronicle_near(&app.game, 0);
    snprintf(result + used, sizeof(result) - used,
             ",\"person\":%d,\"person_fate\":%u,\"person_knowledge\":%u,\"story_decision\":%s}",
             near_person,
             near_person >= 0 ? (unsigned)app.game.chronicle.people[near_person].fate : 0,
             near_person >= 0 ? app.game.chronicle.people[near_person].knowledge : 0,
             app.game.interactions[0].decision_open ? "true" : "false");
    used = strlen(result) - 1;
    snprintf(result + used, sizeof(result) - used,
             ",\"meal\":%u,\"meal_seconds\":%.2f,\"home_projects\":%u,\"home_research\":%u,"
             "\"keepsakes\":%u,\"flare_prepared\":%s}",
             (unsigned)app.game.home.personal[0].meal, app.game.home.personal[0].remaining,
             app.game.home.projects, app.game.home.researched, app.game.home.keepsake_count,
             app.game.home.personal[0].flare_charge ? "true" : "false");
    used = strlen(result) - 1;
    snprintf(
        result + used, sizeof(result) - used,
        ",\"rhythm_facts\":%u,\"rhythm_started\":%u,\"rhythm_completed\":%u,\"rhythm_quiet\":%.2f}",
        district_room ? world_rhythm_state(&app.game.world, district_room->cell) : 0,
        app.game.rhythm.next_id, app.game.rhythm.completed, app.game.rhythm.quiet);
    return result;
}
