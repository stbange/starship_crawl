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
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void settle(Game *g, Cell c) {
    g->players[0].pos = position_from(vadd(world_center(c), v3(0, .02, 0)));
    Vec3 at = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed);
}
static Room *find(Game *g, int challenge, int contract, unsigned layer) {
    for (int y = -4; y <= 4; y++)
        for (int z = 1; z < 15; z++)
            for (int x = -14; x <= 14; x++) {
                RegionPlan plan;
                world_plan(g->world.seed, (Cell){x, y, z}, &plan);
                for (int n = 0; n < plan.count; n++) {
                    RoomSpec *r = &plan.rooms[n];
                    if (r->habitat.pace != PACE_FRONTIER || r->habitat.volume || r->height < 10 ||
                        r->width < 70 || (challenge >= 0 && r->habitat.challenge != challenge) ||
                        (contract >= 0 && r->habitat.contract != contract) ||
                        campaign_layer(r->cell) < layer)
                        continue;
                    settle(g, r->cell);
                    return world_find(&g->world, r->cell);
                }
            }
    CHECK(false);
    return NULL;
}
static void freeze(void) {
    Game *g = fresh();
    FILE *f = fopen(SC_FIXTURE_DIR "/generator-v11-geometry.txt", "r");
    CHECK(f);
    unsigned seed, plan, geometry, population, count = 0;
    Cell c;
    while (fscanf(f, "%u %d %d %d %u %u %u", &seed, &c.x, &c.y, &c.z, &plan, &geometry,
                  &population) == 7) {
        world_destroy(&g->world);
        world_init(&g->world, seed);
        g->world.generator = 11;
        settle(g, c);
        CHECK(world_fingerprint_version(seed, 11, world_region(c)) == plan);
        CHECK(geometry_fingerprint(world_chunk(&g->world, c)) == geometry);
        Room *r = world_find(&g->world, c);
        unsigned h = 2166136261u;
        for (int i = 0; i < r->enemy_count; i++) {
            Enemy *e = &r->enemies[i];
            h = fingerprint_word(h, e->kind);
            h = fingerprint_word(h, e->behavior);
            h = fingerprint_vector(h, position_absolute(e->pos));
        }
        CHECK(h == population && !r->habitat.challenge && !r->habitat.reveal &&
              !r->habitat.contract);
        count++;
    }
    fclose(f);
    CHECK(count == 24);
    CHECK(game_load(g, SC_FIXTURE_DIR "/regional-comparison.sav"));
    unsigned credits = g->players[0].credits;
    CHECK(g->world.generator == 11);
    CHECK(game_save(g, "frontier-old.sav") && game_load(g, "frontier-old.sav"));
    CHECK(g->world.generator == 11 && g->players[0].credits == credits && !g->frontier.ranks[0][0]);
    remove("frontier-old.sav");
    destroy(g);
    puts("PASS immutable generator-11 geometry, populations, plans and format-12 migration");
}
static void generation(void) {
    unsigned challenges = 0, reveals = 0, contracts = 0;
    uint64_t species = 0;
    size_t boxes = 0;
    Game *g = fresh();
    for (unsigned seed = 1; seed <= 16; seed++) {
        world_destroy(&g->world);
        world_init(&g->world, seed);
        for (int x = -12; x <= 12; x += 6)
            for (int y = -6; y <= 6; y += 6) {
                RegionPlan p;
                world_plan(seed, (Cell){x, y, 5}, &p);
                for (int i = 0; i < p.count; i++) {
                    RoomSpec *r = &p.rooms[i];
                    CHECK(r->habitat.challenge < CHALLENGES &&
                          r->habitat.reveal <= DISTRICT_TYPES && r->habitat.contract <= RELICS);
                    if (r->habitat.pace == PACE_FRONTIER)
                        challenges |= 1u << r->habitat.challenge;
                    if (r->habitat.reveal)
                        reveals |= 1u << (r->habitat.reveal - 1);
                    if (r->habitat.contract) {
                        contracts |= 1u << (r->habitat.contract - 1);
                        CHECK(campaign_layer(r->cell) >= 5);
                    }
                }
            }
        for (int side = -1; side <= 1; side += 2) {
            settle(g, (Cell){side * 18, (int)(seed % 7) - 3, 20});
            for (int i = 0; i < g->world.count; i++) {
                Room *r = &g->world.rooms[i];
                if (!r->ready)
                    continue;
                for (int n = 0; n < r->enemy_count; n++) {
                    Enemy *e = &r->enemies[n];
                    species |= UINT64_C(1) << e->kind;
                    CHECK(world_spawn_clear(&g->world, position_absolute(e->pos), .42, 1.25));
                    if (!(r->habitat.challenge == CH_AERIE && n == 0) &&
                        !habitat_spawn(&g->world, r, position_absolute(e->pos))) {
                        Vec3 at = position_absolute(e->pos);
                        fprintf(stderr,
                                "spawn seed=%u room=%d,%d,%d kind=%u challenge=%u height=%.2f "
                                "at=%.2f %.2f %.2f\n",
                                seed, r->cell.x, r->cell.y, r->cell.z, e->kind,
                                r->habitat.challenge, r->height, at.x, at.y, at.z);
                        fprintf(
                            stderr,
                            "form%u vol%u mask%x span%u,%u supported=%d owns=%d cell=%d,%d,%d\n",
                            r->habitat.form, r->habitat.volume, r->habitat.mask, r->span_x,
                            r->span_z, world_spawn_supported(&g->world, at, 1.8),
                            habitat_owns(r->cell, r->span_x, r->span_z, r->habitat.mask,
                                         world_cell(at)),
                            world_cell(at).x, world_cell(at).y, world_cell(at).z);
                        CHECK(false);
                    }
                    CHECK(e->health <= sc_enemies[e->kind].health + 80 ||
                          e->behavior == ALIEN_CONDUCTOR);
                    if (e->kind >= 32)
                        CHECK(e->behavior == ALIEN_LISTENER + e->kind - 32 || n == 0);
                }
            }
            for (int i = 0; i < SC_MAX_CHUNKS; i++)
                if (g->world.chunks[i].active) {
                    Chunk *c = &g->world.chunks[i];
                    boxes = (size_t)fmax(boxes, c->box_count);
                    CHECK(c->box_count < SC_CHUNK_BOXES);
                    CHECK(world_spawn_clear(&g->world, world_lift(c->cell), .35, 1.7));
                }
        }
    }
    CHECK(challenges == 63 && reveals == 511 && contracts == 31);
    CHECK((species >> 32) == 1023);
    for (unsigned n = 0; n < CHALLENGES; n++)
        CHECK(strlen(frontier_name(n)) > 5 && strlen(frontier_hint(n)) > 20);
    for (unsigned n = 0; n < RELICS; n++)
        CHECK(strlen(frontier_relic(n)) > 5 && strlen(frontier_effect(n)) > 20 &&
              strlen(frontier_risk(n + 1)) > 10);
    CHECK(strlen(frontier_risk(0)) && strlen(frontier_reveal(99)));
    printf(
        "PASS all six encounters, nine reveals, five contracts, ten species; peak boxes %zu/%d\n",
        boxes, SC_CHUNK_BOXES);
    destroy(g);
}
static void claims(void) {
    Game *g = fresh();
    const unsigned guardians[] = {8, 0, 2, 4, 6};
    for (unsigned n = 0; n < 5; n++) {
        Cell c = campaign_site(guardians[n]);
        settle(g, c);
        g->players[0].pos = position_from(game_terminal(c));
        CHECK(campaign_interact(g, 0));
        Room *r = world_find(&g->world, c);
        game_damage_enemy(g, r, 0, 0, 1000000, 0);
        CHECK(g->campaign.defeated & (1u << guardians[n]));
    }
    CHECK(campaign_phase(&g->campaign) == 4);
    for (unsigned k = 0; k < CHALLENGES; k++) {
        Room *r = find(g, (int)k, -1, 5);
        Cell c = r->cell;
        g->players[0].pos = position_from(encounter_console(r));
        CHECK(game_encounter_interact(g, 0) && r->frontier_active);
        CHECK(world_local_state(&g->world, r) & FRONTIER_ELIGIBLE);
        if (k)
            CHECK(strstr(encounter_name(r), frontier_name(k)));
        char text[160];
        frontier_offer(g, 0, r, text, sizeof(text));
        CHECK(strlen(text) > 10);
        CampaignReward tool = encounter_reward(g, 0, r);
        CHECK(tool.kind <= REWARD_MODULE);
        for (int i = 0; i < 4; i++)
            game_damage_enemy(g, r, i, 0, 100000, 0);
        game_encounters_step(g, .02);
        CHECK(world_local_state(&g->world, r) & FRONTIER_CLEARED);
        CHECK(game_encounter_interact(g, 0));
        CHECK(campaign_owns(g, 0, tool));
        FrontierProgress before = g->frontier;
        unsigned credits = g->players[0].credits;
        CHECK(game_encounter_interact(g, 0));
        CHECK(!memcmp(&before, &g->frontier, sizeof(before)) && credits == g->players[0].credits);
        /* Proper campaign state and ship records may be saved at arbitrary unlocked coordinates. */
        CHECK(game_save(g, "frontier-claim.sav") && game_load(g, "frontier-claim.sav"));
        g->world.open_layer = 0;
        settle(g, c);
        r = world_find(&g->world, c);
        CHECK((world_local_state(&g->world, r) & FRONTIER_CLAIMED) &&
              !memcmp(&before, &g->frontier, sizeof(before)));
    }
    for (unsigned relic = 0; relic < RELICS; relic++) {
        g->frontier.ranks[0][relic] = 0;
        for (unsigned rank = 0; rank < 3; rank++) {
            Room *r = find(g, -1, (int)relic + 1, 5 + rank * 3);
            char text[160];
            frontier_offer(g, 0, r, text, sizeof(text));
            CHECK(strstr(text, "EVOLUTION AVAILABLE"));
            /* Component contract grant is called only after the durable claim in normal play. */
            frontier_claim(g, 0, r);
            CHECK(g->frontier.ranks[0][relic] == rank + 1);
        }
        Room *r = find(g, -1, (int)relic + 1, 5);
        frontier_claim(g, 0, r);
        CHECK(g->frontier.ranks[0][relic] == 3);
        char text[160];
        frontier_offer(g, 0, r, text, sizeof(text));
        CHECK(strstr(text, "COMPLETE"));
    }
    game_join(g);
    g->frontier.ranks[1][RELIC_ALIEN] = 2;
    CHECK(game_save(g, "frontier-claim.sav") && game_load(g, "frontier-claim.sav"));
    CHECK(g->frontier.ranks[1][RELIC_ALIEN] == 2 && g->player_count == 1);
    FILE *f = fopen("frontier-claim.sav", "rb");
    CHECK(f);
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    unsigned char *wire = malloc((size_t)length);
    CHECK(wire && fread(wire, 1, (size_t)length, f) == (size_t)length);
    fclose(f);
    FrontierProgress saved = g->frontier;
    Player saved_player = g->players[0];
    RoomRecord *saved_records = g->world.records;
    wire[length - 4 - SC_POLITICS_SAVE_BYTES - SC_PLAYERS * RELICS * 4] = 4;
    unsigned hash = 2166136261u;
    for (long i = 0; i < length - 4; i++)
        hash = (hash ^ wire[i]) * 16777619u;
    for (int i = 0; i < 4; i++)
        wire[length - 4 + i] = (unsigned char)(hash >> (8 * i));
    f = fopen("frontier-corrupt.sav", "wb");
    CHECK(f && fwrite(wire, 1, (size_t)length, f) == (size_t)length);
    fclose(f);
    free(wire);
    CHECK(!game_load(g, "frontier-corrupt.sav") && !memcmp(&saved, &g->frontier, sizeof(saved)) &&
          !memcmp(&saved_player, &g->players[0], sizeof(saved_player)) &&
          saved_records == g->world.records);
    remove("frontier-corrupt.sav");
    CHECK(frontier_valid(&g->frontier, 12) && !frontier_valid(&g->frontier, 11));
    g->frontier.ranks[0][0] = 4;
    CHECK(!frontier_valid(&g->frontier, 12));
    game_new_ship(g, 42);
    for (int p = 0; p < 2; p++)
        for (int n = 0; n < RELICS; n++)
            CHECK(!g->frontier.ranks[p][n]);
    remove("frontier-claim.sav");
    destroy(g);
    puts("PASS previews, earned tools, once-only claims, both profiles, reload and fresh-ship "
         "reset");
}
static void advantages(void) {
    Game *g = fresh();
    g->world.open_layer = 0;
    Player *p = &g->players[0];
    for (unsigned k = 1; k < CHALLENGES; k++) {
        Room *r = find(g, (int)k, 0, 2);
        p->pos = position_from(encounter_console(r));
        CHECK(game_encounter_interact(g, 0));
        float baseline = frontier_damage(g, r, 1, 0, 100);
        if (k == CH_AERIE)
            position_move(&p->pos, v3(0, 9, 0));
        if (k == CH_CIPHER)
            CHECK(world_set_systems(&g->world, r->cell, SYSTEM_SILENCED));
        if (k == CH_CIRCUIT) {
            p->pos = position_from(frontier_pylon(r, 0));
            frontier_step(g, r, .02);
            CHECK(r->circuit == 1);
            p->pos = position_from(frontier_pylon(r, 1));
            frontier_step(g, r, .02);
            CHECK(r->circuit == 3 && r->frontier_disrupt == 6);
            r->frontier_disrupt = 0;
        }
        float advantage = frontier_damage(g, r, 1, 0, 100);
        CHECK(advantage == 100 && (k == CH_COLONY || k == CH_SIEGE || advantage > baseline));
        CHECK(frontier_damage(g, r, 0, 0, 100) > 0);
        if (k == CH_CIRCUIT) {
            r->circuit = 1;
            r->circuit_time = .01;
            p->pos = position_from(encounter_console(r));
            frontier_step(g, r, .02);
            CHECK(r->circuit == 0);
            p->sheltered = true;
            frontier_step(g, r, .02);
            p->sheltered = false;
        }
    }
    Room *r = find(g, CH_COLONY, 1, 5);
    r->frontier_active = true;
    for (unsigned contract = 1; contract <= RELICS; contract++) {
        r->habitat.contract = (uint8_t)contract;
        r->hazard_time = 0;
        p->pos = position_from(frontier_hazard(r));
        p->health = 100;
        p->shield = 50;
        p->armor = 25;
        p->energy = 100;
        p->slow = 0;
        frontier_step(g, r, 1);
        CHECK(p->health == 100 && p->shield == 50 && !frontier_hazard_live(r));
        frontier_step(g, r, .5);
        CHECK(frontier_hazard_live(r));
        if (contract == RELIC_ORDER + 1)
            CHECK(p->energy < 100);
        else if (contract == RELIC_COLD + 1)
            CHECK(p->slow > 0);
        else
            CHECK(p->shield < 50);
        p->sheltered = true;
        float shield = p->shield;
        frontier_step(g, r, .02);
        CHECK(p->shield == shield);
        p->sheltered = false;
        position_move(&p->pos, v3(0, 6, 0));
        shield = p->shield;
        frontier_step(g, r, .02);
        CHECK(p->shield == shield);
        unsigned stage = r->hazard_stage;
        frontier_step(g, r, 6);
        CHECK(r->hazard_stage == (stage + 1) % 3);
    }
    for (unsigned rank = 1; rank <= 3; rank++) {
        g->frontier.ranks[0][RELIC_HEAT] = g->frontier.ranks[0][RELIC_COLD] = rank;
        WeaponDef blast = sc_weapons[5], beam = sc_weapons[17], gun = sc_weapons[0];
        frontier_shot(g, 0, &blast);
        frontier_shot(g, 0, &beam);
        frontier_shot(g, 0, &gun);
        CHECK((blast.flags & W_BURN) && (beam.flags & W_CHILL) && gun.flags == sc_weapons[0].flags);
        if (rank >= 2)
            CHECK(blast.blast > sc_weapons[5].blast && beam.range > sc_weapons[17].range);
        if (rank >= 3)
            CHECK(blast.cost < sc_weapons[5].cost && beam.pierce > sc_weapons[17].pierce);
    }
    g->frontier.ranks[0][RELIC_ORDER] = 3;
    p->pos = position_from(encounter_console(r));
    p->hurt = 0;
    p->shield = 0;
    CHECK(world_set_systems(&g->world, r->cell, SYSTEM_POWER));
    p->riding = MECH;
    g->rigs[0][0].armor = 0;
    frontier_player_step(g, 0, 1);
    CHECK(p->shield == 5 && g->rigs[0][0].armor == 3);
    p->sheltered = true;
    frontier_player_step(g, 0, 1);
    CHECK(p->shield == 5);
    p->sheltered = false;
    destroy(g);
    puts("PASS flight, circuit and matrix advantages, warned hazards and nonlinear relic effects");
}
static Room *arena(Game *g) {
    g->world.open_layer = 0;
    g->campaign = (Campaign){0};
    g->story = (Story){0};
    settle(g, (Cell){0, 0, 0});
    for (int r = 0; r < g->world.count; r++)
        g->world.rooms[r].enemy_count = 0;
    Room *r = world_find(&g->world, (Cell){0, 0, 0});
    CHECK(r && r->ready);
    r->habitat.pace = PACE_SWARM;
    g->players[0].pos = position_from(v3(0, .02, 12));
    g->players[0].yaw = SC_PI;
    g->players[0].crouched = false;
    return r;
}
static Enemy *put(Room *r, unsigned behavior, Vec3 at) {
    r->enemy_count = 1;
    Enemy *e = &r->enemies[0];
    unsigned kind = 32 + behavior - ALIEN_LISTENER;
    *e = (Enemy){.kind = (uint8_t)kind,
                 .behavior = (uint8_t)behavior,
                 .pos = position_from(at),
                 .home = at,
                 .health = sc_enemies[kind].health,
                 .alive = true,
                 .heading = {0, 0, 1},
                 .body_heading = {0, 0, 1},
                 .normal = {0, 1, 0}};
    return e;
}
static void aliens(void) {
    Game *g = fresh();
    Room *r = arena(g);
    Player *p = &g->players[0];
    for (unsigned kind = ALIEN_LISTENER; kind < ALIEN_BEHAVIORS; kind++) {
        Enemy *e = put(r, kind, v3(0, .02, 6));
        p->yaw = 0;
        p->noise = 2;
        p->health = 100;
        p->shield = 50;
        bool warn = false, commit = false, recover = false;
        CHECK(strlen(alien_hint(kind)) > 15);
        for (int frame = 0; frame < 720; frame++) {
            g->time += 1.0 / 60;
            e->cooldown = fmaxf(0, e->cooldown - 1.0f / 60);
            CHECK(alien_special_step(g, r, 0, 0, true, 1.0 / 60));
            warn |= e->motion == MOTION_WARN;
            commit |= e->motion == MOTION_COMMIT;
            recover |= e->motion == MOTION_RECOVER;
            if (e->motion == MOTION_WARN)
                CHECK(e->warning_sound > 0);
        }
        CHECK(warn && commit && recover && isfinite(position_absolute(e->pos).x));
        if (kind == ALIEN_LISTENER)
            CHECK(r->alarm_time == 20);
        if (kind == ALIEN_REVERSER)
            CHECK(e->stride > 0);
        if (kind == ALIEN_REFLECTION || kind == ALIEN_PRESSURE)
            CHECK(g->enemy_shots > 0);
        Vec3 away = position_delta(e->pos, p->pos);
        p->yaw = remainder(atan2(away.x, away.z) + SC_PI, SC_PI * 2);
        e->motion = MOTION_WARN;
        e->motion_time = .7;
        e->flash = .1;
        alien_special_step(g, r, 0, 0, true, .02);
        CHECK(e->motion == MOTION_RECOVER);
        e->motion = MOTION_WARN;
        e->motion_time = .7;
        e->flash = 0;
        alien_special_step(g, r, 0, 0, false, .02);
        CHECK(e->motion == MOTION_RECOVER);
    }
    Enemy *e = put(r, ALIEN_LISTENER, v3(0, .02, 6));
    p->noise = .06;
    for (int n = 0; n < 240; n++) {
        g->time += .02;
        alien_special_step(g, r, 0, 0, true, .02);
        CHECK(e->motion != MOTION_WARN);
    }
    e = put(r, ALIEN_WATCHED, v3(0, .02, 6));
    p->yaw = SC_PI;
    p->pitch = -.1;
    CHECK(alien_observed(g, e));
    Vec3 before = position_absolute(e->pos);
    for (int n = 0; n < 100; n++)
        alien_special_step(g, r, 0, 0, true, .02);
    CHECK(e->observed && vlen(vsub(before, position_absolute(e->pos))) == 0);
    game_join(g);
    g->players[1].pos = p->pos;
    g->players[1].yaw = SC_PI;
    g->players[1].pitch = p->pitch;
    p->yaw = 0;
    CHECK(alien_observed(g, e));
    g->players[1].sheltered = true;
    CHECK(!alien_observed(g, e));
    g->players[1].sheltered = false;
    g->matrix.divers[1].active = true;
    CHECK(!alien_observed(g, e));
    g->matrix.divers[1].active = false;
    g->players[1].health = 0;
    CHECK(!alien_observed(g, e));
    g->player_count = 1;
    e = put(r, ALIEN_MOLTER, v3(0, .02, 6));
    CHECK(alien_damage(e, 10) < 10);
    e->health = 110;
    float damage = alien_damage(e, 20);
    CHECK(e->molted && damage < 20 && e->motion == MOTION_RECOVER);
    CHECK(alien_damage(e, 10) == 14);
    e->motion = MOTION_APPROACH;
    CHECK(alien_damage(e, 10) == 10);
    e = put(r, ALIEN_COLONY, v3(0, .02, 6));
    e->unfold = 1;
    alien_fragments(&g->world, e);
    CHECK(vlen(vsub(e->fragments[0], e->fragments[2])) > 4);
    unsigned kills = p->kills;
    game_damage_enemy(g, r, 0, 0, 10000, W_BLAST);
    CHECK(!e->alive && p->kills == kills + 1);
    game_damage_enemy(g, r, 0, 0, 10000, W_BLAST);
    CHECK(p->kills == kills + 1);
    e = put(r, ALIEN_BRITTLE, v3(0, .02, 6));
    e->motion = MOTION_WARN;
    CHECK(alien_damage(e, 10) == 15);
    e = put(r, ALIEN_MACHINE, v3(0, .02, 6));
    e->motion = MOTION_RECOVER;
    CHECK(alien_damage(e, 10) == 13.5f);
    e->behavior = ALIEN_NONE;
    CHECK(!alien_special_step(g, r, 0, 0, true, .02));
    CHECK(!strlen(alien_hint(255)));
    destroy(g);
    puts("PASS ten alien commitments, sound, quiet counterplay, co-op gaze, one molt and linked "
         "death");
}
static void real_attacks(void) {
    Game *g = fresh();
    Room *r = arena(g);
    Player *p = &g->players[0];
    Enemy *e = put(r, ALIEN_COLONY, v3(0, .02, 8));
    e->unfold = 1;
    alien_fragments(&g->world, e);
    Vec3 fragment = e->fragments[0];
    p->pos = position_from(v3(fragment.x, .02, 2));
    p->yaw = 0;
    p->pitch = atan2(fragment.y - player_eye(p).y, fragment.z - 2);
    p->weapon = 17;
    p->cooldown = 0;
    Actions a[2] = {{.fire = true}, {0}};
    game_step(g, a, 1.0 / 60);
    CHECK(g->hits == 1 && e->health < sc_enemies[e->kind].health);
    e = put(r, ALIEN_REVERSER, v3(0, .02, 8));
    p->pos = position_from(v3(-4, .02, 6));
    p->cooldown = 0;
    p->yaw = SC_PI / 2;
    p->pitch = atan2(.67 - player_eye(p).y, 4);
    game_step(g, a, 1.0 / 60);
    CHECK(e->health < sc_enemies[e->kind].health);
    e = put(r, ALIEN_COLONY, v3(0, .02, 8));
    p->weapon = 0;
    p->pos = position_from(v3(-2.8, .02, 2));
    p->cooldown = 0;
    e->unfold = 1;
    alien_fragments(&g->world, e);
    p->yaw = 0;
    p->pitch = atan2(.67 - player_eye(p).y, 4.2);
    game_step(g, a, 1.0 / 60);
    a[0].fire = false;
    for (int n = 0; n < 10; n++)
        game_step(g, a, 1.0 / 60);
    CHECK(e->health < sc_enemies[e->kind].health);
    /* The evolved nova's extra reach, chill and per-second refund use the real attack. */
    e = put(r, ALIEN_BRITTLE, v3(0, .02, 10));
    p->pos = position_from(v3(0, .02, 2));
    p->energy = 20;
    p->relic_cooldown = 0;
    game_nova(g, 0);
    CHECK(e->health == sc_enemies[e->kind].health);
    g->frontier.ranks[0][RELIC_CHAOS] = 3;
    game_nova(g, 0);
    CHECK(e->health < sc_enemies[e->kind].health && e->slow > 0 && p->energy == 25);
    game_nova(g, 0);
    CHECK(p->energy == 25);
    g->frontier.ranks[0][RELIC_ALIEN] = 3;
    p->energy = 100;
    game_psionic(g, 0, PSI_FLIGHT);
    CHECK(p->flight > 0 && p->energy == 85);
    p->noise = 0;
    p->slow = 0;
    e->alive = false;
    a[0] = (Actions){.forward = 1};
    game_step(g, a, .02);
    CHECK(p->noise < .2 && vlen(p->velocity) > 7.5);
    p->flight = 0;
    p->crouched = false;
    p->noise = 0;
    g->frontier.ranks[0][RELIC_ALIEN] = 0;
    a[0] = (Actions){.forward = 1, .crouch = true};
    game_step(g, a, .02);
    /* Pressing crouch and movement together must not emit one loud footstep. */
    CHECK(p->crouched && p->noise < .1);
    destroy(g);
    puts("PASS real beam/bullet fragment hits, worm tail, evolved nova and flight inputs");
}
static void surfaces(void) {
    Game *g = fresh();
    Room *r = arena(g);
    Chunk *chunk = (Chunk *)world_chunk(&g->world, (Cell){0, 0, 0});
    Builder b = {&g->world, chunk, world_center(chunk->cell)};
    world_box(&b, v3(2, 3, 20), v3(1, 6, 10), rgb(.2, .2, .2), 0, true);
    world_box(&b, v3(-1, 6, 20), v3(8, 1, 10), rgb(.2, .2, .2), 0, true);
    Enemy *e = &r->enemies[0];
    *e = (Enemy){.kind = 0,
                 .behavior = ALIEN_SEAM,
                 .alive = true,
                 .pos = position_from(v3(.85, .02, 20)),
                 .normal = {0, 1, 0},
                 .heading = {0, 0, 1}};
    r->enemy_count = 1;
    bool wall = false, ceiling = false;
    for (int n = 0; n < 400; n++) {
        g->time += .02;
        Vec3 before = position_absolute(e->pos);
        alien_surface_step(g, r, e, v3(0, 0, .15), .02);
        e->stride += (float)vlen(vsub(position_absolute(e->pos), before));
        wall |= e->climbing && fabs(e->normal.x) > .5;
        ceiling |= e->normal.y < -.5;
        CHECK(!world_overlap(&g->world, position_absolute(e->pos), .42, 1.25));
        if (ceiling)
            break;
    }
    CHECK(wall && ceiling);
    for (int i = 0; i < 4; i++)
        if (vlen(e->contacts[i]) > .1)
            CHECK(fabs(e->contacts[i].y - 5.5) < .01 || fabs(e->contacts[i].x - 1.5) < .01);
    Player *p = &g->players[0];
    p->pos = position_from(v3(-1, .02, 20));
    e->motion = MOTION_APPROACH;
    e->cooldown = 0;
    CHECK(game_alien_step(g, r, 0, 0, true, .02) && e->motion == MOTION_WARN && e->aim.y < 0);
    Vec3 locked = e->aim;
    float health = p->health, shield = p->shield;
    p->pos = position_from(v3(-1, .02, 28)); /* Step sideways after the landing mark locks. */
    for (int n = 0; n < 450 && e->motion != MOTION_RECOVER; n++) {
        g->time += .02;
        game_alien_step(g, r, 0, 0, true, .02);
        CHECK(vlen(vsub(e->aim, locked)) < .001);
    }
    CHECK(e->motion == MOTION_RECOVER && p->health == health && p->shield == shield);
    e->pos = position_from(v3(-20, .02, 20));
    alien_surface_step(g, r, e, v3(0, 0, 1), .02);
    CHECK(!e->climbing);
    destroy(g);
    puts("PASS planted wall-to-ceiling transitions and physical collision");
}
int main(void) {
    freeze();
    generation();
    claims();
    advantages();
    aliens();
    real_attacks();
    surfaces();
    puts("PASS deep frontiers");
    return 0;
}
