#include "game.h"
#include "habitat.h"
#include "geometry_fingerprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static Game *fresh(void) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    g->world.generator = 11; /* Preserve the original linked-encounter regression. */
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void settle(Game *g, Cell cell) {
    g->players[0].pos = position_from(world_lift(cell));
    Vec3 at = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed);
}
static Room *find(Game *g, unsigned pace, int ecology, int tale) {
    for (int y = -3; y <= 3; y++)
        for (int z = 1; z <= 5; z++)
            for (int x = -5; x <= 5; x++) {
                RegionPlan plan;
                world_plan_version(g->world.seed, g->world.generator, (Cell){x, y, z}, &plan);
                for (int i = 0; i < plan.count; i++) {
                    RoomSpec *r = &plan.rooms[i];
                    if (r->habitat.pace != pace || r->height < 5 || r->width < 40 ||
                        r->habitat.volume ||
                        (ecology >= 0 && r->habitat.ecology != (unsigned)ecology) ||
                        (tale >= 0 && r->habitat.tale != (unsigned)tale))
                        continue;
                    settle(g, r->cell);
                    Room *room = world_find(&g->world, r->cell);
                    CHECK(room && room->ready);
                    return room;
                }
            }
    CHECK(false);
    return NULL;
}
static void unbound(Game *g) {
    const unsigned ids[] = {8, 0, 2, 4, 6};
    for (unsigned i = 0; i < 5; i++) {
        Cell cell = campaign_site(ids[i]);
        settle(g, cell);
        g->players[0].pos = position_from(game_terminal(cell));
        CHECK(campaign_interact(g, 0));
        Room *r = world_find(&g->world, cell);
        CHECK(r && r->enemy_count);
        game_damage_enemy(g, r, 0, 0, 1000000, 0);
        CHECK(g->campaign.defeated & (1u << ids[i]));
    }
    CHECK(campaign_phase(&g->campaign) == 4);
}
static void legacy(void) {
    Game *g = fresh();
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v10-format11.sav"));
    CHECK(g->world.generator == 10 && g->players[0].xp == 17 && g->players[0].credits == 29);
    CHECK(game_save(g, "regional-old.sav") && game_load(g, "regional-old.sav"));
    CHECK(g->world.generator == 10 && g->players[0].xp == 17);
    remove("regional-old.sav");
    FILE *f = fopen(SC_FIXTURE_DIR "/generator-v10-geometry.txt", "r");
    CHECK(f);
    unsigned seed, expected, geometry, population, sprites, count = 0;
    Cell c;
    while (fscanf(f, "%u %d %d %d %u %u %u %u", &seed, &c.x, &c.y, &c.z, &expected, &geometry,
                  &population, &sprites) == 8) {
        world_destroy(&g->world);
        world_init(&g->world, seed);
        g->world.generator = 10;
        settle(g, c);
        const Chunk *chunk = world_chunk(&g->world, c);
        Room *r = world_find(&g->world, c);
        CHECK(chunk && r);
        CHECK(world_fingerprint_version(seed, 10, world_region(c)) == expected);
        CHECK(geometry_fingerprint(chunk) == geometry);
        unsigned p = 2166136261u, s = 2166136261u;
        for (int i = 0; i < r->enemy_count; i++) {
            CHECK(!r->enemies[i].behavior);
            p = fingerprint_word(p, r->enemies[i].kind);
            p = fingerprint_vector(p, position_absolute(r->enemies[i].pos));
        }
        for (int i = 0; i < chunk->sprite_count; i++) {
            s = fingerprint_word(s, chunk->sprites[i].kind);
            s = fingerprint_word(s, chunk->sprites[i].atmosphere);
            s = fingerprint_vector(s, chunk->sprites[i].pos);
        }
        CHECK(p == population && s == sprites && !r->habitat.pace);
        CHECK(!world_mark_local(&g->world, r, LOCAL_RESOLVED));
        count++;
    }
    fclose(f);
    CHECK(count == 20);
    destroy(g);
    puts("PASS immutable generator-10 plans, geometry, sprites, populations and save migration");
}
static void plans(void) {
    unsigned climates = 0, hot = 0, cold = 0, hot_ecology = 0, cold_ecology = 0, paces = 0;
    for (unsigned seed = 1; seed <= 32; seed++)
        for (int y = -5; y <= 5; y++)
            for (int x = -8; x <= 8; x += 4) {
                RegionPlan p;
                world_plan(seed, (Cell){x, y, 2}, &p);
                for (int i = 0; i < p.count; i++) {
                    RoomSpec *r = &p.rooms[i];
                    ShipTraits t = campaign_traits(seed, r->cell);
                    climates |= 1u << r->habitat.atmosphere;
                    paces |= 1u << r->habitat.pace;
                    if (t.heat >= 2) {
                        hot |= 1u << r->habitat.atmosphere;
                        hot_ecology |= 1u << r->habitat.ecology;
                    }
                    if (t.heat <= -2) {
                        cold |= 1u << r->habitat.atmosphere;
                        cold_ecology |= 1u << r->habitat.ecology;
                    }
                    CHECK(r->habitat.atmosphere < ATMOSPHERES && r->habitat.ecology < ECOLOGIES);
                    if (r->habitat.pace == PACE_FRONTIER && !r->habitat.court)
                        CHECK(campaign_layer(r->cell) >= 2 && !r->history_role);
                }
            }
    CHECK((climates & 0x3f80) == 0x3f80 && paces == 15);
    CHECK((hot & (1u << BOILING_AIR)) && (hot & (1u << ASH_AIR)) && (hot & (1u << HEAT_HAZE)));
    CHECK((cold & (1u << FROZEN_RUNOFF)) && (cold & (1u << ICE_SHARDS)) &&
          (cold & (1u << COLD_CLEAR)));
    CHECK(hot_ecology == 63 && cold_ecology == 63);
    Game *g = fresh();
    Room *r = find(g, PACE_QUIET, -1, -1);
    CHECK(r->enemy_count == 0);
    r = find(g, PACE_TRACE, -1, -1);
    CHECK(r->enemy_count == 3);
    r = find(g, PACE_FRONTIER, -1, -1);
    CHECK(r->enemy_count == 4);
    for (int i = 0; i < 4; i++)
        CHECK(habitat_spawn(&g->world, r, position_absolute(r->enemies[i].pos)));
    CHECK(r->enemies[0].behavior == ALIEN_CONDUCTOR && r->enemies[1].behavior == ALIEN_PILGRIM &&
          r->enemies[2].behavior == ALIEN_GAZER && r->enemies[3].behavior == ALIEN_SEAM);
    unsigned squads = 0;
    for (int y = -4; y <= 4; y++)
        for (int x = -4; x < 4; x++) {
            RegionPlan plan;
            world_plan_version(713, 11, (Cell){x, y, 2}, &plan);
            for (int i = 0; i < plan.count; i++)
                if (plan.rooms[i].habitat.pace == PACE_FRONTIER) {
                    settle(g, plan.rooms[i].cell);
                    r = world_find(&g->world, plan.rooms[i].cell);
                    CHECK(r && r->enemy_count == 4);
                    for (int e = 0; e < 4; e++)
                        CHECK(habitat_spawn(&g->world, r, position_absolute(r->enemies[e].pos)));
                    CHECK(world_spawn_supported(&g->world, encounter_console(r), 1.78));
                    squads++;
                }
        }
    CHECK(squads > 40);
    destroy(g);
    puts("PASS hot/cold variety, compatible ecologies, pacing, supported mixed-role squads");
}
static void local_actions(void) {
    Game *g = fresh();
    unbound(g);
    CHECK(!game_encounter_interact(g, -1) && !game_encounter_interact(g, 2));
    CHECK(!game_encounter_interact(g, 0));
    CHECK(!local_available(&g->world, NULL));
    CHECK(!world_mark_local(&g->world, NULL, 1));
    for (unsigned tale = 0; tale < LOCAL_TALES; tale++) {
        Room *r = find(g, PACE_QUIET, -1, (int)tale);
        Cell cell = r->cell;
        CHECK(strstr(local_action(tale), " "));
        CHECK(!world_mark_local(&g->world, r, 64));
        g->players[0].pos = position_from(local_console(r));
        g->players[0].health = 20;
        g->players[0].armor = 0;
        unsigned credits = g->players[0].credits;
        CHECK(game_encounter_interact(g, 0));
        CHECK((world_local_state(&g->world, r) & LOCAL_RESOLVED) && !local_available(&g->world, r));
        CHECK(world_systems(&g->world, cell) & SYSTEM_POWER);
        CHECK(g->players[0].credits == credits + 35);
        CHECK(!game_encounter_interact(g, 0) && g->players[0].credits == credits + 35);
        if (tale == MUTINY)
            CHECK(world_systems(&g->world, cell) & SYSTEM_SILENCED);
        if (tale == LAST_FEAST || tale == CULTIVATION)
            CHECK(g->players[0].nourished >= 90);
        if (tale == TRIAGE)
            CHECK(g->players[0].health == player_max_health(&g->players[0]));
        CHECK(game_save(g, "regional-local.sav") && game_load(g, "regional-local.sav"));
        settle(g, cell);
        r = world_find(&g->world, cell);
        CHECK(world_local_state(&g->world, r) & LOCAL_RESOLVED);
    }
    Room *r = find(g, PACE_TRACE, -1, -1);
    g->players[0].pos = position_from(local_console(r));
    r->enemies[0].pos = g->players[0].pos;
    CHECK(game_encounter_interact(g, 0) && !(world_local_state(&g->world, r) & LOCAL_RESOLVED));
    g->players[0].sheltered = true;
    CHECK(!game_encounter_interact(g, 0));
    g->players[0].sheltered = false;
    g->matrix.divers[0].active = true;
    CHECK(!game_encounter_interact(g, 0));
    g->matrix.divers[0].active = false;
    g->players[0].lift_active = true;
    CHECK(!game_encounter_interact(g, 0));
    g->players[0].lift_active = false;
    remove("regional-local.sav");
    destroy(g);
    puts("PASS all eight local actions, one-time rewards and durable environmental consequences");
}
static void challenge(void) {
    Game *g = fresh();
    unbound(g);
    Room *r = find(g, PACE_FRONTIER, MECHANICAL, -1);
    Cell cell = r->cell;
    g->world.open_layer = 0;
    g->players[0].pos = position_from(encounter_console(r));
    CHECK(!strcmp(encounter_name(r), "CHOIR RELAY"));
    CHECK(encounter_passive(r, 0) && !encounter_passive(r, 4));
    float hp = r->enemies[1].health;
    game_damage_enemy(g, r, 1, 0, 1000, 0);
    CHECK(r->enemies[1].health == hp);
    CHECK(!encounter_linked(&g->world, r, 1));
    game_join(g);
    CHECK(game_encounter_interact(g, 0) && r->frontier_active);
    CHECK((world_local_state(&g->world, r) & 12) == 12);
    r->frontier_timer = .5f;
    CHECK(game_encounter_interact(g, 0) && r->frontier_timer == .5f);
    for (int i = 0; i < 100; i++)
        game_encounters_step(g, .02);
    CHECK(r->frontier_attacker > 0 && r->frontier_attacker < 4);
    CHECK(encounter_attack(r, r->frontier_attacker));
    CHECK(!encounter_linked(&g->world, r, 0) && !encounter_linked(&g->world, r, 4));
    CHECK(encounter_linked(&g->world, r, 1));
    CHECK(encounter_damage(g, r, 1, 10, 0) < 10);
    CHECK(encounter_damage(g, r, 0, 60, W_BEAM) == 60 && r->frontier_disrupt > 1.9);
    CHECK(!encounter_linked(&g->world, r, 1) && !encounter_attack(r, 1));
    CHECK(encounter_damage(g, r, 1, 10, 0) == 10);
    game_damage_enemy(g, r, 0, 0, 10000, 0);
    game_encounters_step(g, .02);
    CHECK(!r->enemies[0].alive && r->frontier_disrupt > 1);
    for (int i = 0; i < 100; i++)
        game_encounters_step(g, .02);
    CHECK(encounter_attack(r, 1) && encounter_attack(r, 2));
    for (int i = 1; i < 4; i++)
        game_damage_enemy(g, r, i, 0, 10000, 0);
    game_encounters_step(g, .02);
    CHECK(world_local_state(&g->world, r) & FRONTIER_CLEARED);
    unsigned kills = g->players[0].kills;
    CHECK(!r->frontier_active);
    CHECK(game_save(g, "regional-win.sav") && game_load(g, "regional-win.sav"));
    settle(g, cell);
    r = world_find(&g->world, cell);
    g->players[0].pos = position_from(encounter_console(r));
    CampaignReward reward = encounter_reward(g, 0, r);
    char text[160];
    encounter_reward_name(g, 0, r, text, sizeof(text));
    CHECK(*text);
    CHECK(game_encounter_interact(g, 0) && campaign_owns(g, 0, reward));
    CHECK(world_local_state(&g->world, r) & FRONTIER_CLAIMED);
    CHECK(g->drills[0].time > 0 && *reward_drill_hint(reward));
    unsigned credits = g->players[0].credits;
    CHECK(game_encounter_interact(g, 0) && g->players[0].credits == credits &&
          g->players[0].kills == kills);
    game_join(g);
    CHECK(game_encounter_interact(g, 1));
    CHECK(world_local_state(&g->world, r) & (FRONTIER_CLAIMED << 1));
    CHECK(game_save(g, "regional-win.sav") && game_load(g, "regional-win.sav"));
    settle(g, cell);
    r = world_find(&g->world, cell);
    CHECK((world_local_state(&g->world, r) & 48) == 48);
    for (int i = 0; i < 4; i++)
        CHECK(!r->enemies[i].alive);
    r = find(g, PACE_FRONTIER, BROOD_NEST, -1);
    CHECK(!strcmp(encounter_name(r), "NEST CONDUCTOR"));
    g->players[0].pos = position_from(encounter_console(r));
    CHECK(game_encounter_interact(g, 0));
    /* The conductor owns its health budget; generic nest regeneration must not clamp it. */
    float core_health = r->enemies[0].health;
    Actions idle[2] = {0};
    game_step(g, idle, .02);
    CHECK(r->enemies[0].health == core_health);
    g->players[0].sheltered = true;
    game_encounters_step(g, .02);
    CHECK(!r->frontier_active);
    g->players[0].sheltered = false;
    CHECK(game_encounter_interact(g, 0));
    for (int i = 0; i < 7; i++) {
        CampaignReward a = encounter_reward(g, 0, r);
        campaign_grant(g, 0, a);
    }
    encounter_reward_name(g, 0, r, text, sizeof(text));
    CHECK(strstr(text, "+2 POINTS"));
    r = find(g, PACE_FRONTIER, MECHANICAL, -1);
    for (int i = 0; i < 7; i++) {
        CampaignReward a = encounter_reward(g, 0, r);
        campaign_grant(g, 0, a);
    }
    encounter_reward_name(g, 0, r, text, sizeof(text));
    CHECK(strstr(text, "+2 POINTS"));
    r = find(g, PACE_FRONTIER, BROOD_NEST, -1);
    g->players[0].pos = position_from(encounter_console(r));
    CHECK(game_encounter_interact(g, 0));
    for (int i = 0; i < 4; i++)
        game_damage_enemy(g, r, i, 0, 10000, 0);
    game_encounters_step(g, .02);
    CHECK(world_local_state(&g->world, r) & FRONTIER_CLEARED);
    g->drills[0].time = 0;
    credits = g->players[0].credits;
    unsigned points = g->players[0].points;
    CHECK(game_encounter_interact(g, 0) && g->players[0].credits == credits + 100 &&
          g->players[0].points == points + 2 && g->drills[0].time == 0);
    remove("regional-win.sav");
    destroy(g);
    puts("PASS optional activation, staggered links, beam disruption, core collapse, co-op durable "
         "claims");
}
static void aliens(void) {
    Game *g = fresh();
    Room *r = find(g, PACE_QUIET, -1, -1);
    g->world.open_layer = 0;
    Vec3 center = world_center(r->cell);
    Player *p = &g->players[0];
    r->enemy_count = 1;
    Enemy *e = &r->enemies[0];
    for (unsigned type = ALIEN_PILGRIM; type <= ALIEN_SEAM; type++) {
        *e = (Enemy){.pos = position_from(vadd(center, v3(0, .02, 9))),
                     .kind = type == ALIEN_PILGRIM ? 4
                             : type == ALIEN_GAZER ? 1
                                                   : 0,
                     .behavior = (uint8_t)type,
                     .alive = true,
                     .health = 50,
                     .normal = {0, 1, 0},
                     .body_heading = {0, 0, 1}};
        p->pos = position_from(vadd(center, v3(0, .02, 1)));
        p->health = 100;
        p->shield = 0;
        p->hurt = 0;
        for (int i = 0; i < 200 && e->motion != MOTION_WARN; i++)
            CHECK(game_alien_step(g, r, 0, 0, true, 1.0 / 60));
        CHECK(e->motion == MOTION_WARN && e->windup >= .8f);
        Vec3 locked = e->aim;
        unsigned shots = g->enemy_shots;
        position_move(&p->pos, v3(6, 0, 0));
        for (int i = 0; i < 66; i++)
            game_alien_step(g, r, 0, 0, true, 1.0 / 60);
        CHECK(vlen(vsub(e->aim, locked)) < .0001 && p->health == 100);
        if (type == ALIEN_GAZER)
            CHECK(g->enemy_shots > shots);
        for (int i = 0; i < 180; i++)
            game_alien_step(g, r, 0, 0, false, 1.0 / 60);
        CHECK(e->motion != MOTION_COMMIT);
        e->motion = MOTION_WARN;
        e->motion_time = .8f;
        e->flash = .1f;
        game_alien_step(g, r, 0, 0, true, 1.0 / 60);
        CHECK(e->motion == MOTION_RECOVER);
        e->flash = 0;
        e->motion = MOTION_APPROACH;
        e->cooldown = 2;
        e->slow = 1;
        p->pos = position_from(vadd(center, v3(0, .02, 40)));
        for (int i = 0; i < 80; i++)
            game_alien_step(g, r, 0, 0, true, 1.0 / 60);
        CHECK(e->stride > 0 && isfinite(e->travel_speed));
    }
    /* Locate an actual wall with supported feet; verify planted limbs ray-hit it. */
    bool climbed = false;
    for (int c = 0; c < SC_MAX_CHUNKS && !climbed; c++) {
        Chunk *chunk = &g->world.chunks[c];
        if (!chunk->active || chunk->cell.y != r->cell.y)
            continue;
        for (size_t i = 0; i < chunk->box_count && !climbed; i++) {
            Box *box = &chunk->boxes[i];
            if (!box->solid || box->max.y - box->min.y < 5 || box->min.y > .1)
                continue;
            Vec3 at = vadd(world_center(chunk->cell),
                           v3(box->min.x - .8, .02, (box->min.z + box->max.z) * .5));
            if (world_overlap(&g->world, at, .42, 1.25) ||
                !world_spawn_supported(&g->world, at, 1.25))
                continue;
            *e = (Enemy){.pos = position_from(at),
                         .kind = 0,
                         .behavior = ALIEN_SEAM,
                         .motion = MOTION_APPROACH,
                         .alive = true,
                         .health = 50,
                         .normal = {0, 1, 0},
                         .body_heading = {0, 0, 1}};
            p->pos = position_from(vadd(at, v3(0, 0, 20)));
            g->time = .3;
            for (int frame = 0; frame < 20; frame++)
                game_alien_step(g, r, 0, 0, false, 1.0 / 60);
            if (e->climbing) {
                CHECK(position_absolute(e->pos).y > at.y);
                climbed = true;
            }
        }
    }
    CHECK(climbed);
    unsigned contacts = 0;
    for (int i = 0; i < 4; i++)
        contacts += vlen(e->contacts[i]) > .1;
    CHECK(contacts > 0);
    e->behavior = ALIEN_NONE;
    CHECK(!game_alien_step(g, r, 0, 0, true, .02));
    destroy(g);
    puts("PASS locked attack lines, evasion, interruption, locomotion and real-wall planted limbs");
}
static void drills(void) {
    Game *g = fresh();
    settle(g, (Cell){0, 0, 0});
    Player *p = &g->players[0];
    p->pos = position_from(v3(0, .02, 24));
    Cell site = {0, 0, 0};
    game_join(g);
    CampaignReward gun = {REWARD_GUN, 16};
    campaign_grant(g, 0, gun);
    game_reward_drill(g, 0, gun, site);
    /* Another player's fire must not complete this player's drill. */
    Actions a[2] = {{0}, {.fire = true}};
    for (int i = 0; i < 20; i++)
        game_step(g, a, 1.0 / 60);
    CHECK(!g->drills[0].complete && g->players[1].shots_fired > 0);
    a[0].fire = true;
    a[1].fire = false;
    for (int i = 0; i < 20; i++)
        game_step(g, a, 1.0 / 60);
    CHECK(g->drills[0].complete && g->drills[0].time <= 5);
    for (unsigned power = 0; power < SC_PSIONICS; power++) {
        CampaignReward reward = {REWARD_POWER, (uint8_t)power};
        campaign_grant(g, 0, reward);
        game_reward_drill(g, 0, reward, site);
        CHECK(strstr(reward_drill_hint(reward), "TRY"));
        p->energy = player_max_energy(p);
        game_psionic(g, 0, (Psionic)power);
        game_encounters_step(g, .02);
        CHECK(g->drills[0].complete);
        game_psionic(g, 0, (Psionic)power);
    }
    for (unsigned rig = MECH; rig <= WAGON; rig++) {
        CampaignReward reward = {REWARD_RIG, (uint8_t)rig};
        campaign_grant(g, 0, reward);
        game_reward_drill(g, 0, reward, site);
        CHECK(strstr(reward_drill_hint(reward), "ENTER"));
        game_rig_toggle(g, 0, (RigKind)rig);
        CHECK(game_rig_interact(g, 0));
        game_encounters_step(g, .02);
        CHECK(g->drills[0].complete);
        CHECK(game_rig_interact(g, 0));
        game_rig_toggle(g, 0, (RigKind)rig);
    }
    CampaignReward module = {REWARD_MODULE, MOD_STORM};
    campaign_grant(g, 0, module);
    game_reward_drill(g, 0, module, site);
    CHECK(strstr(reward_drill_hint(module), "SPECIALIZATION"));
    p->cooldown = 0;
    p->energy = 100;
    for (int i = 0; i < 20; i++)
        game_step(g, a, 1.0 / 60);
    CHECK(g->drills[0].complete);
    game_reward_drill(g, 0, gun, site);
    p->sheltered = true;
    game_encounters_step(g, .1);
    CHECK(!g->drills[0].complete);
    p->sheltered = false;
    p->pos = position_from(v3(0, .02, 90));
    game_encounters_step(g, .1);
    CHECK(!g->drills[0].complete);
    game_encounters_step(g, 100);
    CHECK(g->drills[0].time == 0);
    CHECK(strstr(reward_drill_hint((CampaignReward){REWARD_GUN, 5}), "SPLASH"));
    CHECK(strstr(reward_drill_hint((CampaignReward){REWARD_GUN, 0}), "EQUIPPED"));
    destroy(g);
    puts("PASS earned-tool drills require actual per-player fire, powers or rig use");
}
static void commitment_damage(void) {
    Game *g = fresh();
    Room *r = find(g, PACE_FRONTIER, MECHANICAL, -1);
    g->players[0].pos = position_from(encounter_console(r));
    CHECK(game_encounter_interact(g, 0));
    CHECK(game_alien_step(g, r, 0, 0, false, .02));
    CHECK(r->enemies[0].motion == MOTION_APPROACH);
    r->enemies[0].windup = 1;
    game_alien_step(g, r, 0, 0, false, .02);
    CHECK(r->enemies[0].motion == MOTION_WARN);
    Enemy *e = &r->enemies[1];
    e->pos = g->players[0].pos;
    position_move(&e->pos, v3(0, 0, 1));
    e->motion = MOTION_COMMIT;
    e->motion_time = .5;
    e->aim = v3(0, 0, -1);
    e->attack_done = false;
    g->players[0].shield = 0;
    g->players[0].armor = 0;
    g->players[0].hurt = 0;
    float hp = g->players[0].health;
    game_alien_step(g, r, 1, 0, true, .02);
    CHECK(e->attack_done && g->players[0].health < hp);
    hp = g->players[0].health;
    g->players[0].hurt = 0;
    game_alien_step(g, r, 1, 0, true, .02);
    CHECK(g->players[0].health == hp);
    game_join(g);
    game_encounters_step(g, .02);
    CHECK(world_local_state(&g->world, r) & (FRONTIER_ELIGIBLE << 1));
    destroy(g);
    puts("PASS conductor cues, single committed contact and late co-op participation");
}
static void malformed_local(void) {
    Game *g = fresh();
    settle(g, (Cell){0, 0, 0});
    Room *r = world_find(&g->world, (Cell){0, 0, 0});
    CHECK(world_mark_local(&g->world, r, 0));
    CHECK(g->world.record_count == 1);
    CHECK(game_save(g, "regional-wire.sav"));
    FILE *f = fopen("regional-wire.sav", "rb");
    CHECK(f);
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    unsigned char *bytes = malloc((size_t)length);
    CHECK(bytes);
    CHECK(fread(bytes, 1, (size_t)length, f) == (size_t)length);
    fclose(f);
    const unsigned invalid[] = {64, FRONTIER_CLAIMED, FRONTIER_CLEARED,
                                FRONTIER_CLAIMED | FRONTIER_ELIGIBLE};
    for (unsigned n = 0; n < sizeof(invalid) / sizeof(invalid[0]); n++) {
        for (int b = 0; b < 4; b++)
            bytes[length - 8 - SC_PLAYERS * RELICS * 4 + b] =
                (unsigned char)(invalid[n] >> (8 * b));
        unsigned hash = 2166136261u;
        for (long i = 0; i < length - 4; i++)
            hash = (hash ^ bytes[i]) * 16777619u;
        for (int b = 0; b < 4; b++)
            bytes[length - 4 + b] = (unsigned char)(hash >> (8 * b));
        f = fopen("regional-bad.sav", "wb");
        CHECK(f);
        CHECK(fwrite(bytes, 1, (size_t)length, f) == (size_t)length);
        fclose(f);
        g->players[0].credits = 123;
        CHECK(!game_load(g, "regional-bad.sav") && g->players[0].credits == 123);
    }
    free(bytes);
    remove("regional-wire.sav");
    remove("regional-bad.sav");
    game_new_ship(g, 42);
    CHECK(g->world.record_count == 0 && g->drills[0].time == 0);
    destroy(g);
    puts("PASS malformed local claims roll back transactionally and new ships reset progress");
}
int main(void) {
    legacy();
    plans();
    local_actions();
    challenge();
    aliens();
    drills();
    commitment_damage();
    malformed_local();
    return 0;
}
