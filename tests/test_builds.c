#include "session.h"
#include "snapshot.h"
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
static void finish(Game *g) {
    game_destroy(g);
    free(g);
}
static void settle(Game *g) {
    Vec3 at = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed);
}
static void branch(Game *g, unsigned item) {
    g->campaign.profiles[0].module = item;
    g->campaign.profiles[0].modules |= 1u << (item - 1);
}
static void rules(void) {
    Game *g = fresh(), *copy = fresh();
    Player *p = &g->players[0];
    CHECK(g->world.generator == 16 && g->campaign.version == 2 && p->rules == 2);
    p->kills = 3;
    p->points = 20;
    p->health = 41;
    p->shield = 12;
    p->armor = 13;
    p->energy = 14;
    for (unsigned i = 0; i < 8; i++)
        CHECK(game_upgrade(g, 0, 1));
    CHECK(p->health == 41 && p->shield == 12 && p->armor == 13 && p->energy == 14);
    CHECK(player_max_health(p) == 250 && player_rank(p, 4) == 4 && player_rank(p, 12) == 8);
    CHECK(player_investment(p) == 8 && game_upgrade_cost(p, 8) == 1000);
    CHECK(player_next_level(p) == 80);
    p->level = 3;
    CHECK(player_next_level(p) == 280);
    campaign_grant(g, 0, (CampaignReward){REWARD_MODULE, BUILD_ECHO});
    CHECK(!game_build_active(g, 0, BUILD_ECHO));
    campaign_grant(g, 0, (CampaignReward){REWARD_POWER, PSI_VEIL});
    CHECK(game_build_active(g, 0, BUILD_ECHO));
    p->pos = position_from(v3(0, .02, 12));
    CHECK(campaign_refit(g, 0, 5, 1) && g->views[0].branch_preview == 1);
    CHECK(g->campaign.profiles[0].module == BUILD_ECHO); /* Preview has no build effects. */
    GameCommand equip = {
        .kind = CMD_CAMPAIGN_BRANCH, .player = game_player_id(g, 0), .argument = 0};
    CHECK(game_command(g, &equip) && !g->campaign.profiles[0].module &&
          !g->views[0].branch_preview);
    CHECK(campaign_refit(g, 0, 5, 1) && g->views[0].branch_preview == BUILD_ECHO + 1);
    CHECK(campaign_refit(g, 0, 5, 1) && g->views[0].branch_preview == 1);
    equip.argument = BUILD_RELAY;
    CHECK(!game_command(g, &equip));
    equip.argument = BUILD_ECHO;
    CHECK(game_command(g, &equip));
    CHECK(game_command(g, &equip)); /* Idempotent and free. */
    equip.argument = 99;
    CHECK(!game_command(g, &equip));
    CHECK(!game_build_equip(g, -1, BUILD_ECHO) && !game_build_equip(g, 0, 99));
    p->riding = MECH;
    CHECK(!game_build_equip(g, 0, BUILD_ECHO));
    p->riding = 0;
    CHECK(!game_build_active(g, -1, BUILD_ECHO) && !game_build_active(g, SC_PLAYERS, BUILD_ECHO));
    CHECK(!game_build_active(g, 0, BUILD_NONE) && !game_build_active(g, 0, (BuildBranch)99));
    CHECK(!game_build_active(g, 0, BUILD_RELAY));
    unsigned points = p->points;
    campaign_grant(g, 0, (CampaignReward){REWARD_MODULE, BUILD_ECHO});
    CHECK(p->points == points + 2 && p->credits == 100);
    CHECK(campaign_valid(&g->campaign, g->players, &g->world));
    CHECK(game_save(g, "builds.sav") && game_load(copy, "builds.sav"));
    CHECK(copy->campaign.profiles[0].module == BUILD_ECHO && copy->players[0].rules == 2);
    CHECK(copy->players[0].health == 41 && copy->players[0].vitality == 8);
    p->level = 200;
    p->xp = 150000;
    CHECK(game_save(g, "builds.sav") && game_load(copy, "builds.sav"));
    CHECK(copy->players[0].xp == 150000);
    p->level = 3;
    p->xp = 0;
    p->health = 300;
    CHECK(!campaign_valid(&g->campaign, g->players, &g->world));
    CHECK(game_save(g, "builds-bad.sav") && !game_load(copy, "builds-bad.sav"));
    p->health = 41;
    for (unsigned invalid = 0; invalid < 3; invalid++) {
        Campaign saved = g->campaign;
        if (!invalid)
            g->campaign.version = 99;
        if (invalid == 1)
            g->campaign.profiles[0].module = 9;
        if (invalid == 2)
            g->campaign.profiles[0].modules |= 256;
        CHECK(game_save(g, "builds-bad.sav"));
        uint32_t seed = copy->world.seed;
        CHECK(!game_load(copy, "builds-bad.sav") && copy->world.seed == seed);
        g->campaign = saved;
    }
    p->rules = 1;
    CHECK(!campaign_valid(&g->campaign, g->players, &g->world));
    p->rules = 2;
    g->world.generator = 15;
    campaign_init(g);
    CHECK(!game_build_equip(g, 0, 0));
    p->points = 1;
    p->health = 10;
    CHECK(game_upgrade(g, 0, 0) && p->health == player_max_health(p));
    CHECK(p->rules == 0 && game_upgrade_cost(p, 1) == 85 && player_next_level(p) == 160);
    CHECK(game_save(g, "builds-old.sav") && game_load(copy, "builds-old.sav"));
    CHECK(copy->world.generator == 15 && copy->campaign.version == 1 && !copy->players[0].rules);
    for (unsigned i = 0; i <= 9; i++) {
        CHECK(strlen(campaign_build_hint(g, i)) && strlen(campaign_build_name(g, i)));
    }
    /* Inactive profiles must not carry a previous ship's branch or effects. */
    g->campaign.profiles[1].modules = 255;
    g->campaign.profiles[1].module = BUILD_FIELD;
    g->builds[1].echo_life = 3;
    game_new_ship(g, 42);
    for (int i = 0; i < SC_PLAYERS; i++) {
        CHECK(g->players[i].rules == 2 && g->players[i].weapons == 1 && !g->players[i].points);
        CHECK(!g->campaign.profiles[i].modules && !g->builds[i].echo_life);
    }
    for (unsigned i = 0; i <= 9; i++) {
        CHECK(strlen(campaign_build_hint(g, i)) && strlen(campaign_build_name(g, i)));
    }
    remove("builds.sav");
    remove("builds-bad.sav");
    remove("builds-old.sav");
    finish(g);
    finish(copy);
}
static void claims(void) {
    Game *g = fresh();
    g->player_count = 2;
    g->campaign.defeated = 1u << 8;
    g->campaign.eligible[8] = 3;
    for (int i = 0; i < 2; i++) {
        g->campaign.choice[i] = 9;
        GameCommand close = {.kind = CMD_CLOSE_INTERACTION, .player = game_player_id(g, i)};
        CHECK(game_command(g, &close));
        CHECK(!g->campaign.profiles[i].claimed); /* Deferred rewards remain unclaimed. */
        g->campaign.choice[i] = 9;
        CHECK(campaign_choose(g, i, 1));
        CampaignProfile claimed = g->campaign.profiles[i];
        unsigned points = g->players[i].points, credits = g->players[i].credits;
        g->campaign.choice[i] = 9;
        CHECK(!campaign_choose(g, i, 2));
        CHECK(!memcmp(&claimed, &g->campaign.profiles[i], sizeof(claimed)));
        CHECK(points == g->players[i].points && credits == g->players[i].credits);
    }
    CHECK(g->campaign.profiles[0].claimed == g->campaign.profiles[1].claimed);
    finish(g);
}
static void effects(void) {
    Game *g = fresh(), *copy = fresh();
    Player *p = &g->players[0];
    p->pos = position_from(v3(0, .02, 12));
    settle(g);
    Room *r = world_find(&g->world, (Cell){0});
    CHECK(r);
    for (int i = 0; i < g->world.count; i++)
        g->world.rooms[i].enemy_count = 0;
    r->enemy_count = 4;
    for (int i = 0; i < 4; i++)
        r->enemies[i] = (Enemy){.alive = true,
                                .kind = 1,
                                .health = 100,
                                .id = (uint8_t)i,
                                .pos = position_from(v3(i - 1, .02, 20)),
                                .normal = {0, 1, 0}};
    p->health = 40;
    p->shield = 10;
    p->armor = 15;
    p->energy = 20;
    p->xp = 79;
    game_damage_enemy(g, r, 0, 0, 200, 0);
    CHECK(p->level == 2 && p->health == 55 && p->shield == 20 && p->armor == 15 && p->energy == 30);
    p->kills = 0;
    g->hits = 0;
    r->enemies[0].alive = true;
    r->enemies[0].health = 100;
    campaign_grant(g, 0, (CampaignReward){REWARD_POWER, PSI_VEIL});
    branch(g, BUILD_ECHO);
    p->cloaked = true;
    WeaponDef shot = sc_weapons[0];
    game_build_shot(g, 0, &shot);
    CHECK(g->builds[0].echo_life == 3 && shot.damage == sc_weapons[0].damage);
    CHECK(game_build_echo(g, &r->enemies[0], .05));
    CHECK(r->enemies[0].pos.local.z < 20 && !p->kills);
    unsigned char *bytes;
    size_t size;
    CHECK(game_snapshot_encode(g, &bytes, &size));
    CHECK(game_snapshot_decode(copy, bytes, size));
    free(bytes);
    CHECK(copy->builds[0].echo_life == 3 && copy->players[0].cloaked);
    Session s;
    session_init(&s, g);
    session_advance(&s, 10, true);
    CHECK(g->builds[0].echo_life == 3);
    p->sheltered = true;
    game_builds_step(g, .5);
    CHECK(g->builds[0].echo_life == 3 && !game_build_echo(g, &r->enemies[0], .05));
    p->sheltered = false;
    game_builds_step(g, 4);
    CHECK(!g->builds[0].echo_life && !game_build_echo(g, &r->enemies[0], .05));
    g->builds[0].echo_life = 3;
    r->enemies[0].allegiance = 2;
    CHECK(!game_build_echo(g, &r->enemies[0], .05));
    r->enemies[0].allegiance = 0;
    r->enemies[0].force = 1;
    CHECK(!game_build_echo(g, &r->enemies[0], .05));
    r->enemies[0].force = 0;
    r->enemies[0].pos = position_from(v3(0, .02, 45));
    CHECK(!game_build_echo(g, &r->enemies[0], .05));
    p->cloaked = false;
    branch(g, BUILD_RELAY);
    CHECK(!game_build_active(g, 0, BUILD_RELAY));
    p->weapons |= (1u << 16) | (1u << 19);
    g->campaign.profiles[0].guns[0] = p->weapon = 16;
    g->campaign.profiles[0].guns[1] = 19;
    CHECK(game_build_active(g, 0, BUILD_RELAY));
    Vec3 from = v3(0, 1, 12), to = v3(0, 1, 16);
    game_build_relay(g, 0, from, to, 40);
    CHECK(!g->builds[0].relays[0].life);
    g->flares[0] =
        (Flare){.active = true, .owner = 1, .pos = position_from(to), .normal = {0, 0, -1}};
    game_build_relay(g, 0, from, to, 40);
    CHECK(!g->builds[0].relays[0].life);
    g->flares[0].owner = 0;
    p->energy = 8;
    game_build_relay(g, 0, from, to, 40);
    CHECK(p->energy == 2 && g->builds[0].relays[0].life > 0 && g->builds[0].relays[1].life > 0);
    CHECK(!g->builds[0].relays[2].life && g->hits == 2);
    p->energy = 100;
    game_build_relay(g, 0, from, to, 40);
    CHECK(g->hits == 5 && p->energy == 91);
    Chunk *chunk = NULL;
    for (int i = 0; i < SC_MAX_CHUNKS; i++)
        if (g->world.chunks[i].active && cell_equal(g->world.chunks[i].cell, (Cell){0}))
            chunk = &g->world.chunks[i];
    CHECK(chunk && chunk->box_count);
    Box saved = chunk->boxes[0];
    chunk->boxes[0] = (Box){.min = vsub(v3(-30, 0, 17), world_center(chunk->cell)),
                            .max = vsub(v3(30, 10, 17.5), world_center(chunk->cell)),
                            .solid = true};
    unsigned hits = g->hits;
    float energy = p->energy;
    game_build_relay(g, 0, from, to, 40);
    CHECK(g->hits == hits && p->energy == energy); /* Wall blocks every outgoing relay. */
    chunk->boxes[0] = saved;
    /* Exercise a real attached floor flare through the ordinary firing path. */
    g->flares[0].active = false;
    game_attach_flare(g, 0, v3(0, .001, 18), v3(0, -1, 0));
    CHECK(g->flares[0].active);
    Vec3 muzzle = vadd(player_eye(p), v3(.16, -.13, 0));
    Vec3 aim = vsub(position_absolute(g->flares[0].pos), muzzle);
    p->yaw = atan2(aim.x, aim.z);
    p->pitch = atan2(aim.y, hypot(aim.x, aim.z));
    p->cooldown = 0;
    g->builds[0] = (BuildEffects){0};
    Actions fire[SC_PLAYERS] = {0};
    fire[0].fire = true;
    game_step(g, fire, SC_TICK_SECONDS);
    CHECK(g->builds[0].relays[0].life > 0 && g->beams[0].life > 0);
    branch(g, BUILD_CAPACITOR);
    CHECK(!game_build_active(g, 0, BUILD_CAPACITOR));
    campaign_grant(g, 0, (CampaignReward){REWARD_RIG, MECH});
    p->riding = MECH;
    p->shield = 0;
    g->rigs[0][0].armor = 500;
    game_hurt(g, 0, 30, 0);
    CHECK(g->builds[0].charge == 0);
    game_hostile_hit(g, 0, 50, 0);
    CHECK(g->builds[0].charge == 50);
    game_hostile_hit(g, 0, 100, 0);
    CHECK(g->builds[0].charge == 120);
    shot = sc_mech_weapons[0];
    p->rig_weapon = 1;
    game_build_shot(g, 0, &shot);
    CHECK(g->builds[0].charge == 120);
    p->rig_weapon = 0;
    game_build_shot(g, 0, &shot);
    CHECK(!g->builds[0].charge && shot.damage == sc_mech_weapons[0].damage + 180);
    p->sheltered = true;
    game_hostile_hit(g, 0, 30, 0);
    CHECK(!g->builds[0].charge);
    p->sheltered = false;
    game_hostile_hit(g, 0, 30, 0);
    p->riding = 0;
    game_builds_step(g, .2);
    CHECK(!g->builds[0].charge);
    for (unsigned i = BUILD_FOCUS; i <= BUILD_FIELD; i++) {
        branch(g, i);
        (void)game_build_active(g, 0, (BuildBranch)i);
    }
    g->campaign.version = 1;
    CHECK(!game_build_active(g, 0, BUILD_RELAY) && !game_build_echo(g, &r->enemies[0], .05));
    finish(g);
    finish(copy);
}
static void recipes(void) {
    unsigned variants = 0;
    for (unsigned recipe = 1; recipe <= 3; recipe++)
        for (unsigned build = 0; build < 3; build++) {
            Game *g = fresh();
            Player *p = &g->players[0];
            /* Earned encounter fixture: open boundaries without changing combat resources. */
            for (unsigned id = 0; id < 8; id += 2) {
                Cell site = campaign_site(id);
                CHECK(world_set_systems(&g->world, site, 0));
                for (size_t k = 0; k < g->world.record_count; k++)
                    if (cell_equal(g->world.records[k].cell, site))
                        g->world.records[k].killed |= 1;
                g->campaign.defeated |= 1u << id;
                g->campaign.eligible[id] = 1;
            }
            campaign_sync(g);
            Cell site = {0};
            bool found = false;
            for (int z = 4; z < 20 && !found; z++)
                for (int x = -8; x < 9 && !found; x++) {
                    RoomSpec spec = world_describe_version(g->world.seed, 16, (Cell){x, 0, z});
                    if (spec.habitat.pace == PACE_FRONTIER && !spec.habitat.challenge &&
                        spec.height >= 5 && cell_equal(spec.cell, (Cell){x, 0, z})) {
                        site = spec.cell;
                        found = true;
                    }
                }
            CHECK(found);
            Vec3 center = world_center(site);
            p->pos = position_from(vadd(center, v3(0, .02, 2)));
            p->checkpoint = p->pos;
            settle(g);
            for (int i = 0; i < g->world.count; i++)
                g->world.rooms[i].enemy_count = 0;
            Room *r = world_find(&g->world, site);
            CHECK(r);
            r->habitat.pace = PACE_FRONTIER;
            r->habitat.ecology = recipe == 3 ? MECHANICAL : FERAL;
            r->habitat.challenge = 0;
            r->hash = (recipe - 1) * 256;
            r->enemy_count = 4;
            for (int i = 0; i < 4; i++)
                r->enemies[i] = (Enemy){
                    .alive = true, .id = (uint8_t)i, .normal = {0, 1, 0}, .heading = {0, 0, -1}};
            encounter_populate(&g->world, r);
            CHECK(r->recipe == recipe && r->enemy_count == 4 &&
                  r->enemies[0].behavior == ALIEN_CONDUCTOR);
            CHECK(strlen(encounter_name(r)) > 12);
            variants |= 1u << r->recipe;
            CHECK(encounter_passive(r, 0));
            p->pos = position_from(encounter_console(r));
            CHECK(game_encounter_interact(g, 0) && r->frontier_active);
            p->pos = position_from(vadd(center, v3(0, .02, 2)));
            p->power = build * 4;
            p->vitality = build * 3;
            p->health = player_max_health(p);
            Session s;
            session_init(&s, g);
            unsigned frame;
            for (frame = 0; frame < 3600; frame++) {
                Enemy *target = NULL;
                for (int i = 0; i < 4; i++)
                    if (r->enemies[i].alive) {
                        target = &r->enemies[i];
                        break;
                    }
                if (!target)
                    break;
                Vec3 to = vsub(enemy_center(target), player_eye(p));
                GameCommand aim = {.kind = CMD_AIM,
                                   .player = game_player_id(g, 0),
                                   .x = atan2(to.x, to.z),
                                   .y = asin(clampd(vnorm(to).y, -1, 1))};
                CHECK(session_command(&s, &aim));
                Actions a = {.fire = true, .strafe = frame % 240 < 120 ? .3f : -.3f};
                CHECK(session_input(&s, game_player_id(g, 0), &a));
                session_tick(&s);
            }
            printf("recipe %u build %u frames %u kills %u deaths %u alive %u%u%u%u hp %.0f %.0f "
                   "%.0f %.0f\n",
                   recipe, build, frame, p->kills, g->respawns, r->enemies[0].alive,
                   r->enemies[1].alive, r->enemies[2].alive, r->enemies[3].alive,
                   r->enemies[0].health, r->enemies[1].health, r->enemies[2].health,
                   r->enemies[3].health);
            CHECK(frame < 3600 && !g->respawns && p->kills >= 4);
            CHECK(r->enemy_count <= SC_MAX_ENEMIES && !r->enemies[0].alive);
            finish(g);
        }
    CHECK(variants == 14);
}
int main(void) {
    rules();
    claims();
    effects();
    recipes();
    puts("PASS coherent growth, legacy saves, branch gates, relays, decoys, charge and snapshots");
}
