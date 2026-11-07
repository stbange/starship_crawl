#include "game.h"
#include "legacy_game.h"
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
static Game *fresh(unsigned seed) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, seed);
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void settle(Game *g) {
    Vec3 positions[2] = {position_absolute(g->players[0].pos),
                         position_absolute(g->players[1].pos)};
    CHECK(world_refresh(&g->world, positions, g->player_count));
    for (int i = 0; g->world.pending && i < 256; i++)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed && !g->world.pending);
}
static Room *stage(Game *g, unsigned id) {
    g->players[0].pos = position_from(game_terminal(campaign_site(id)));
    if (g->player_count == 2)
        g->players[1].pos = g->players[0].pos;
    settle(g);
    Room *r = world_find(&g->world, campaign_site(id));
    CHECK(r && r->ready && r->enemy_count == 1);
    CHECK(world_overlap(&g->world, position_absolute(g->players[0].pos), .34, 1.78) ==
          (g->world.open_layer && campaign_layer(campaign_site(id)) > g->world.open_layer));
    return r;
}
static void win(Game *g, unsigned id) {
    Room *r = stage(g, id);
    game_interact(g, 0);
    CHECK(g->campaign.attempts[id].active);
    game_damage_enemy(g, r, 0, 0, 1000000, 0);
    CHECK(!r->enemies[0].alive && (g->campaign.defeated & (1u << id)));
    CHECK(campaign_valid(&g->campaign, g->players, &g->world));
}
static void fresh_characters(void) {
    Game *g = fresh(713);
    CHECK(g->world.generator == SC_GENERATOR_VERSION && g->campaign.version == 1 &&
          g->world.open_layer == 1);
    for (int i = 0; i < 2; i++) {
        CHECK(g->players[i].weapons == 3 && g->players[i].level == 1);
        CHECK(!g->campaign.profiles[i].powers && !g->campaign.profiles[i].rigs);
    }
    CHECK(!campaign_weapon(g, -1, 0) && !campaign_weapon(g, 2, 0) && !campaign_weapon(g, 0, 20));
    CHECK(!campaign_power(g, -1, 0) && !campaign_power(g, 2, 0) && !campaign_power(g, 0, 4));
    CHECK(!campaign_rig(g, -1, 1) && !campaign_rig(g, 2, 1) && !campaign_rig(g, 0, 0) &&
          !campaign_rig(g, 0, 3));
    CHECK(campaign_weapon(g, 0, 0) && campaign_weapon(g, 0, 1) && !campaign_weapon(g, 0, 16));
    float energy = g->players[0].energy;
    for (int i = 0; i < 4; i++)
        game_psionic(g, 0, (Psionic)i);
    game_rig_toggle(g, 0, MECH);
    game_rig_toggle(g, 0, WAGON);
    CHECK(g->players[0].energy == energy && !g->players[0].cloaked && !g->rigs[0][0].deployed);
    g->players[0].points = 20;
    for (int i = 0; i < 12; i++)
        CHECK(game_upgrade(g, 0, 0));
    CHECK(!game_upgrade(g, 0, 0) && g->players[0].power == 12);
    game_join(g);
    for (int i = 0; i < 2; i++) {
        g->players[i].level = 30;
        g->players[i].credits = 500;
        g->players[i].flare_strength = 4;
        g->players[i].mods[3] = 8;
        g->campaign.profiles[i].powers = 15;
        g->campaign.profiles[i].rigs = 3;
        g->matrix.profiles[i].programs = 63;
        g->matrix.profiles[i].data = 10;
        g->matrix.profiles[i].upgrades[0] = 3;
    }
    game_leave(g);
    game_new_ship(g, 713);
    CHECK(g->player_count == 1 && g->world.seed == 713 && !g->world.record_count);
    for (int i = 0; i < 2; i++) {
        CHECK(g->players[i].level == 1 && g->players[i].weapons == 3 && !g->players[i].credits);
        CHECK(!g->players[i].flare_strength && !g->players[i].mods[3] && !g->players[i].power);
        CHECK(!g->campaign.profiles[i].rigs && !g->campaign.profiles[i].powers);
        CHECK(g->matrix.profiles[i].programs == 7 && !g->matrix.profiles[i].data &&
              !g->matrix.profiles[i].upgrades[0]);
    }
    game_join(g);
    game_new_ship(g, 42);
    CHECK(g->player_count == 2);
    CHECK(campaign_valid(&g->campaign, g->players, &g->world));
    g->players[0].pos = position_from(v3(0, .02, 12));
    settle(g);
    g->players[0].level = 4;
    g->players[0].health = 0;
    Actions a[2] = {0};
    game_step(g, a, 1.0 / 60);
    CHECK(g->players[0].level == 4 && g->respawns == 1);
    destroy(g);
}
static void geography(void) {
    CHECK(campaign_layer((Cell){0}) == 1 && campaign_layer((Cell){-4, -1, 3}) == 1);
    CHECK(campaign_layer((Cell){4, 0, 0}) == 2 && campaign_layer((Cell){-5, 0, 0}) == 2);
    CHECK(campaign_layer((Cell){0, 2, 0}) == 2 && campaign_layer((Cell){0, -2, 0}) == 2);
    CHECK(campaign_layer((Cell){0, 0, 4}) == 2 && campaign_layer((Cell){0, 0, -4}) == 1);
    unsigned families = 0, guns = 0, powers = 0, rigs = 0, modules = 0;
    for (unsigned seed = 0; seed < 64; seed++) {
        ShipTraits t = campaign_traits(seed, (Cell){12, 6, 12});
        ShipTraits opposite = campaign_traits(seed, (Cell){-12, -6, 12});
        CHECK(abs(t.heat) == 3 && abs(t.order) == 3 && t.alien == 3);
        CHECK(t.heat == -opposite.heat && t.order == -opposite.order);
        for (unsigned id = 0; id < SC_GUARDIANS; id++) {
            Cell c = campaign_site(id);
            CHECK(campaign_guardian(c) == (int)id);
            CHECK(campaign_layer(c) == (id == 8 ? 1 : id / 2 + 1));
            RoomSpec r = world_describe(seed, c);
            CHECK(cell_equal(r.cell, c) && r.height >= 8);
            CHECK(id == 8 || (r.span_x == 2 && r.span_z == 2));
            families |= 1u << campaign_family(seed, id);
            CHECK(strlen(campaign_guardian_name(seed, id)) > 5);
            for (unsigned i = 0; i < 3; i++) {
                CampaignReward rw = campaign_reward(seed, id, i);
                if (rw.kind == REWARD_GUN)
                    guns |= 1u << rw.item;
                if (rw.kind == REWARD_POWER)
                    powers |= 1u << rw.item;
                if (rw.kind == REWARD_RIG)
                    rigs |= 1u << (rw.item - 1);
                if (rw.kind == REWARD_MODULE)
                    modules |= 1u << (rw.item - 1);
            }
        }
        RegionPlan a, b;
        world_plan(seed, (Cell){-3, 3, 2}, &a);
        world_plan(seed, (Cell){8, -5, 9}, &b);
        world_plan(seed, (Cell){-3, 3, 2}, &b);
        CHECK(!memcmp(&a, &b, sizeof(a)));
    }
    CHECK(families == 31 && guns == ((1u << SC_WEAPONS) - 4) && powers == 15 && rigs == 3 &&
          modules == 15);
    CHECK(campaign_guardian((Cell){100, 0, 0}) == -1);
    CHECK(strlen(campaign_phase_name(99)) && strlen(campaign_module_name(99)));
    Game *g = fresh(42);
    stage(g, 0);
    CHECK(campaign_gate_ray(&g->world, v3(0, 10, 10), v3(1, 0, 0), 1000) == 336);
    CHECK(campaign_gate_ray(&g->world, v3(0, 10, 10), v3(-1, 0, 0), 1000) == 432);
    CHECK(campaign_gate_ray(&g->world, v3(0, 10, 10), v3(0, 0, 1), 1000) == 374);
    CHECK(campaign_gate_ray(&g->world, v3(0, 10, 10), v3(0, 0, -1), 1000) == 1000);
    CHECK(campaign_gate_ray(&g->world, v3(0, 10, 10), v3(0, 1, 0), 1000) == 118);
    CHECK(campaign_gate_ray(&g->world, v3(0, 10, 10), v3(0, -1, 0), 1000) == 74);
    CHECK(campaign_gate_ray(&g->world, v3(500, 10, 10), v3(1, 0, 0), 1000) == 1000);
    CHECK(campaign_gate_ray(&g->world, v3(500, 10, 10), v3(-1, 0, 0), 1000) == 164);
    CHECK(campaign_gate_ray(&g->world, v3(0, 200, 10), v3(0, -1, 0), 1000) == 72);
    CHECK(campaign_gate_ray(&g->world, v3(0, 10, 500), v3(0, 0, -1), 1000) == 116);
    g->world.open_layer = 0;
    CHECK(campaign_gate_ray(&g->world, v3(0, 10, 10), v3(1, 0, 0), 1000) == 1000);
    destroy(g);
}
static void rewards_and_loadouts(void) {
    unsigned awarded = 0;
    for (unsigned seed = 1; seed <= 18; seed++) {
        Game *g = fresh(seed);
        game_join(g);
        win(g, 8);
        CHECK(g->campaign.eligible[8] == 3 && campaign_phase(&g->campaign) == 0);
        for (int p = 0; p < 2; p++) {
            campaign_journal(g, p);
            unsigned choice = (seed + (unsigned)p) % 3 + 1;
            CampaignReward rw = campaign_reward(seed, 8, choice - 1);
            awarded |= 1u << rw.kind;
            CHECK(g->campaign.choice[p] == 9 && campaign_choose(g, p, choice));
            CHECK(!campaign_choose(g, p, choice));
        }
        win(g, 0);
        CHECK(campaign_phase(&g->campaign) == 1 && g->world.open_layer == 2);
        campaign_journal(g, 0);
        CHECK(campaign_choose(g, 0, 3));
        awarded |= 1u << REWARD_MODULE;
        CHECK(g->campaign.profiles[0].module);
        /* A saved inactive partner still has a distinct, unclaimed reward. */
        game_leave(g);
        CHECK(game_save(g, "phase-unit.sav") && game_load(g, "phase-unit.sav"));
        game_join(g);
        campaign_journal(g, 1);
        CHECK(campaign_choose(g, 1, 1));
        CHECK(campaign_valid(&g->campaign, g->players, &g->world));
        win(g, 1);
        campaign_journal(g, 0);
        CampaignReward rw = campaign_reward(seed, 1, 0);
        g->players[0].weapons |= 1u << rw.item;
        unsigned points = g->players[0].points, credits = g->players[0].credits;
        char name[140];
        campaign_reward_name(g, 0, 1, 0, name, sizeof(name));
        CHECK(strstr(name, "OWNED"));
        CHECK(campaign_choose(g, 0, 1));
        CHECK(g->players[0].points == points + 2 && g->players[0].credits == credits + 100);
        CHECK(campaign_phase(&g->campaign) == 1);
        destroy(g);
    }
    CHECK(awarded == 15);
    Game *g = fresh(7);
    stage(g, 8);
    CHECK(!campaign_safe_refit(g, -1) && !campaign_safe_refit(g, 2));
    CHECK(!campaign_refit(g, 0, 0, 1));
    g->players[0].sheltered = true;
    CHECK(campaign_safe_refit(g, 0));
    CampaignProfile *p = &g->campaign.profiles[0];
    p->powers = 15;
    p->rigs = 3;
    p->modules = 15;
    g->players[0].weapons = (1u << SC_WEAPONS) - 1;
    for (unsigned slot = 0; slot < 6; slot++)
        for (int change = 0; change < 24; change++)
            CHECK(campaign_refit(g, 0, slot, change % 2 ? 1 : -1));
    CHECK(!campaign_refit(g, -1, 0, 1) && !campaign_refit(g, 0, 6, 1));
    g->players[0].riding = MECH;
    CHECK(!campaign_safe_refit(g, 0));
    g->players[0].riding = 0;
    g->matrix.divers[0].active = true;
    CHECK(!campaign_safe_refit(g, 0));
    g->matrix.divers[0].active = false;
    g->players[0].health = 0;
    CHECK(!campaign_safe_refit(g, 0));
    g->players[0].health = 100;
    p->guns[0] = 0;
    p->guns[1] = 1;
    g->players[0].weapons = 3;
    CHECK(campaign_refit(g, 0, 0, 1)); /* The current item is a valid wrap-around choice. */
    g->players[0].weapons = 0;
    CHECK(!campaign_refit(g, 0, 0, 1));
    g->players[0].weapons = 3;
    CHECK(!campaign_choose(g, -1, 1) && !campaign_choose(g, 0, 0) && !campaign_choose(g, 0, 4));
    campaign_journal(g, -1);
    campaign_journal(g, 2);
    g->players[0].sheltered = false;
    position_move(&g->players[0].pos, v3(10, 0, 0));
    CHECK(!campaign_interact(g, 0));
    destroy(g);
}
static void fights_and_retry(void) {
    unsigned families = 0;
    for (unsigned seed = 0; seed < 40 && families != 31; seed++) {
        unsigned family = campaign_family(seed, 8);
        if (families & (1u << family))
            continue;
        families |= 1u << family;
        Game *g = fresh(seed);
        Room *r = stage(g, 8);
        float health = r->enemies[0].health;
        game_damage_enemy(g, r, 0, 0, 100, 0);
        CHECK(r->enemies[0].health == health);
        CHECK(!campaign_engaged(g, -1));
        game_interact(g, 0);
        CHECK(campaign_engaged(g, 0));
        CHECK(campaign_interact(g, 0));
        g->players[0].cloaked = true;
        CHECK(campaign_enemy_step(g, r, 0, .05));
        g->players[0].cloaked = false;
        CHECK(world_set_systems(&g->world, r->cell, SYSTEM_POWER | SYSTEM_BEACON));
        float shield = g->players[0].shield;
        game_hurt(g, 0, 5, 0);
        CHECK(g->players[0].shield < shield);
        Actions a[2] = {0};
        for (int i = 0; i < 700; i++)
            game_step(g, a, 1.0 / 60);
        CHECK(g->enemy_shots > 0 && g->projectile_hits > 0);
        g->campaign.attempts[8].opening = 1;
        CHECK(campaign_damage(g, r, 0, 0, 10) == 16);
        g->campaign.attempts[8].opening = 0;
        CHECK(campaign_damage(g, r, 0, 0, 10) == 5.5f);
        g->players[0].sheltered = true;
        campaign_step(g, .05);
        CHECK(!g->campaign.attempts[8].active && r->enemies[0].health == health);
        for (int i = 0; i < SC_MAX_PROJECTILES; i++)
            CHECK(!g->projectiles[i].active || g->projectiles[i].guardian != 9);
        CHECK(campaign_damage(g, r, 0, 0, 10) == 0);
        g->players[0].sheltered = false;
        game_interact(g, 0);
        g->players[0].pos.local.x += 1;
        CHECK(game_save(g, "phase-attempt.sav") && game_load(g, "phase-attempt.sav"));
        settle(g);
        CHECK(!g->campaign.attempts[8].active);
        CHECK(vlen(position_delta(g->players[0].pos, g->players[0].checkpoint)) == 0);
        game_join(g);
        game_interact(g, 0);
        CHECK(g->campaign.attempts[8].participants == 3);
        g->players[0].sheltered = true;
        campaign_step(g, .05);
        CHECK(g->campaign.attempts[8].active);
        g->players[1].health = 0;
        campaign_step(g, .05);
        CHECK(!g->campaign.attempts[8].active);
        destroy(g);
    }
    CHECK(families == 31);
}
static void phases_and_persistence(void) {
    Game *g = fresh(713);
    Cell objective;
    CHECK(!campaign_objective(g, &objective));
    for (unsigned id = 0; id < 8; id += 2) {
        win(g, id);
        campaign_journal(g, 0);
        CHECK(campaign_choose(g, 0, id % 3 + 1));
        CHECK(campaign_phase(&g->campaign) == id / 2 + 1);
        CHECK(game_save(g, "phase-unit.sav") && game_load(g, "phase-unit.sav"));
        settle(g);
        CHECK(campaign_phase(&g->campaign) == id / 2 + 1);
    }
    CHECK(!g->world.open_layer && !campaign_objective(g, &objective));
    g->story.done = 1;
    campaign_track(g);
    CHECK(campaign_objective(g, &objective) && cell_equal(objective, campaign_site(1)));
    g->campaign.tracked = 0;
    CHECK(!campaign_objective(g, &objective));
    g->story.done = 0;
    CHECK(campaign_valid(&g->campaign, g->players, &g->world));
    /* Validate corrupted state independently of checksum or serializer behavior. */
    Campaign saved = g->campaign;
#define BAD(field, value)                                                                          \
    do {                                                                                           \
        g->campaign = saved;                                                                       \
        g->campaign.field = (value);                                                               \
        CHECK(!campaign_valid(&g->campaign, g->players, &g->world));                               \
    } while (0)
    BAD(version, 2);
    BAD(tracked, 10);
    BAD(defeated, 512);
    BAD(defeated, 64);
    BAD(eligible[0], 4);
    BAD(eligible[0], 0);
    BAD(eligible[1], 1);
    BAD(attempts[0].active, true);
    BAD(attempts[1].participants, 4);
    BAD(profiles[0].powers, 16);
    BAD(profiles[0].rigs, 4);
    BAD(profiles[0].modules, 16);
    BAD(profiles[0].claimed, 511);
    BAD(profiles[0].guns[0], 20);
    BAD(profiles[0].guns[1], saved.profiles[0].guns[0]);
    BAD(profiles[0].power_slots[0], 5);
    BAD(profiles[0].power_slots[0], 3);
    BAD(profiles[0].rig, 3);
    BAD(profiles[0].module, 5);
    BAD(profiles[1].claimed, 1);
    g->campaign = saved;
    uint32_t killed = g->world.records[0].killed;
    g->world.records[0].killed = 0;
    CHECK(!campaign_valid(&g->campaign, g->players, &g->world));
    g->world.records[0].killed = killed;
    CHECK(!campaign_choose(g, 2, 1));
    campaign_defeat(g, 99);
    campaign_defeat(g, 0);
    destroy(g);
    g = fresh(713);
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v6-format10.sav"));
    CHECK(g->world.generator == 6 && !g->campaign.version &&
          g->players[0].weapons == SC_START_WEAPONS);
    CHECK(campaign_weapon(g, 0, 19) && campaign_power(g, 0, 3) && campaign_rig(g, 0, 2));
    CHECK(campaign_attack_bonus(g, 0) == 1);
    g->players[0].pos = position_from(game_terminal(campaign_site(3)));
    campaign_journal(g, 0);
    CHECK(g->campaign.tracked == 0 && campaign_valid(&g->campaign, g->players, &g->world));
    campaign_track(g);
    campaign_step(g, .05);
    campaign_defeat(g, 0);
    CHECK(!campaign_objective(g, &objective) && !campaign_interact(g, 0));
    CHECK(!campaign_choose(g, 0, 1));
    CHECK(!campaign_refit(g, 0, 0, 1));
    game_new_ship(g, 42);
    CHECK(g->campaign.version == 1 && g->players[0].weapons == 3);
    destroy(g);
    remove("phase-unit.sav");
    remove("phase-attempt.sav");
}
static void guidance_and_builds(void) {
    Game *g = fresh(713);
    Cell goal;
    g->story.done = 1;
    CHECK(campaign_objective(g, &goal) && cell_equal(goal, campaign_site(0)));
    campaign_track(g);
    CHECK(g->campaign.tracked == 1);
    campaign_track(g);
    CHECK(g->campaign.tracked == 8);
    campaign_track(g);
    CHECK(g->campaign.tracked == 9 && !campaign_objective(g, &goal));
    campaign_track(g);
    CHECK(g->campaign.tracked == 0);
    CHECK(game_objective(g, &goal));
    CHECK(strstr(game_mission(g), "GUARDIAN") || strlen(game_mission(g)));
    g->story.done = 0;
    CHECK(!game_flare_upgrade(g, 0, false));
    g->players[0].weapons |= 1u << 19;
    g->players[0].points = 1;
    CHECK(game_flare_upgrade(g, 0, true));
    Room *r = stage(g, 2);
    CHECK(campaign_interact(g, 0) && !g->campaign.attempts[2].active);
    r = stage(g, 0);
    r->ready = false;
    CHECK(campaign_interact(g, 0));
    r->ready = true;
    win(g, 0);
    CHECK(campaign_safe_refit(g, 0));
    CHECK(campaign_interact(g, 0) && g->players[0].journal_page == 5);
    g->story.done = 1;
    CHECK(!campaign_objective(g, &goal));
    g->story.assigned |= 1u << game_story_target(g);
    g->campaign.tracked = 0;
    CHECK(campaign_objective(g, &goal) && cell_equal(goal, campaign_site(2)));
    for (unsigned id = 0; id < 9; id++)
        for (unsigned choice = 0; choice < 3; choice++) {
            char text[160];
            campaign_reward_name(g, 0, id, choice, text, sizeof(text));
            CHECK(strlen(text) > 3);
        }
    Player *p = &g->players[0];
    CampaignProfile *build = &g->campaign.profiles[0];
    build->powers = 15;
    build->power_slots[0] = PSI_VEIL;
    build->power_slots[1] = PSI_FLIGHT;
    CHECK(campaign_power(g, 0, PSI_VEIL) && !campaign_power(g, 0, PSI_NOVA));
    game_psionic(g, 0, PSI_VEIL);
    CHECK(p->cloaked);
    build->module = MOD_HUNTER;
    CHECK(campaign_attack_bonus(g, 0) == 2.5f);
    game_psionic(g, 0, PSI_VEIL);
    CHECK(campaign_attack_bonus(g, 0) == 1);
    game_psionic(g, 0, PSI_FLIGHT);
    build->module = MOD_STORM;
    CHECK(campaign_attack_bonus(g, 0) == 1.6f);
    p->flight = 0;
    p->riding = MECH;
    build->module = MOD_SIEGE;
    CHECK(campaign_attack_bonus(g, 0) == 1.5f);
    p->riding = 0;
    build->module = MOD_SABOTEUR;
    CHECK(campaign_attack_bonus(g, 0) == 1);
    CHECK(world_set_systems(&g->world, campaign_site(0), SYSTEM_SILENCED));
    CHECK(campaign_attack_bonus(g, 0) == 1.6f);
    r = stage(g, 8);
    game_interact(g, 0);
    p->pos = position_from(v3(0, .02, 12));
    settle(g);
    CHECK(!campaign_engaged(g, 0));
    campaign_step(g, .01);
    CHECK(!g->campaign.attempts[8].active);
    r = stage(g, 8);
    game_interact(g, 0);
    /* Burn continues through openings; powered/silenced systems alter volleys. */
    r->enemies[0].burn = 1;
    float health = r->enemies[0].health;
    CHECK(world_set_systems(&g->world, r->cell, SYSTEM_SILENCED));
    g->story.done |= (1u << STORY_POWER) | (1u << STORY_OVERRIDE);
    r->enemies[0].windup = .01;
    g->campaign.attempts[8].aim = player_eye(p);
    unsigned shots = g->enemy_shots;
    CHECK(campaign_enemy_step(g, r, 0, .02));
    CHECK(r->enemies[0].health < health);
    CHECK(g->enemy_shots == shots + 3 && g->campaign.attempts[8].opening > 3);
    g->campaign.attempts[8].opening = 1;
    r->enemies[0].health = .01;
    CHECK(campaign_enemy_step(g, r, 0, .05) && !r->enemies[0].alive);
    Room ordinary = {.cell = {99, 0, 99}};
    CHECK(campaign_damage(g, &ordinary, 0, 0, 17) == 17);
    RegionPlan small = {.count = 1};
    small.rooms[0] = (RoomSpec){.cell = campaign_site(8), .type = CREW, .height = 3};
    campaign_plan(713, &small);
    CHECK(small.rooms[0].height >= 8);
    destroy(g);
    g = fresh(713);
    stage(g, 2);
    CHECK(!campaign_valid(&g->campaign, g->players, &g->world));
    g->players[0].pos = position_from(v3(0, .02, 12));
    settle(g);
    p = &g->players[0];
    p->power = 13;
    CHECK(!campaign_valid(&g->campaign, g->players, &g->world));
    p->power = 0;
    p->cloaked = true;
    CHECK(!campaign_valid(&g->campaign, g->players, &g->world));
    p->cloaked = false;
    destroy(g);
}
static void malformed_appendix(void) {
    Game *g = fresh(713);
    win(g, 8);
    campaign_journal(g, 0);
    CHECK(campaign_choose(g, 0, 1));
    CHECK(game_save(g, "phase-good.sav"));
    FILE *f = fopen("phase-good.sav", "rb");
    CHECK(f);
    CHECK(!fseek(f, 0, SEEK_END));
    long length = ftell(f);
    rewind(f);
    unsigned char *original = malloc((size_t)length), *data = malloc((size_t)length);
    CHECK(original && data);
    CHECK(fread(original, 1, (size_t)length, f) == (size_t)length);
    fclose(f);
    /* Format 12 appends one local-state word per room after the campaign. */
    const unsigned offsets[] = {
        0, 4, 8, 12 + 8 * 12, 12 + 8 * 12 + 4, 120, 124, 128, 132, 136, 140, 144, 148, 152, 156};
    const uint32_t values[] = {2, 0, 10, 4, 2, 16, 4, 16, 511, 20, 5, 0, 5, 3, 5};
    World *before = malloc(sizeof(*before));
    CHECK(before); /* A world snapshot exceeds the default Windows stack. */
    for (unsigned n = 0; n < sizeof(offsets) / sizeof(offsets[0]); n++) {
        memcpy(data, original, (size_t)length);
        size_t at = (size_t)length - 204 - SC_POLITICS_SAVE_BYTES - SC_PLAYERS * RELICS * 4 -
                    g->world.record_count * 4 + offsets[n];
        for (int b = 0; b < 4; b++)
            data[at + b] = (unsigned char)(values[n] >> (b * 8));
        uint32_t hash = 2166136261u;
        for (long i = 0; i < length - 4; i++)
            hash = (hash ^ data[i]) * 16777619u;
        for (int b = 0; b < 4; b++)
            data[length - 4 + b] = (unsigned char)(hash >> (b * 8));
        f = fopen("phase-bad.sav", "wb");
        CHECK(f && fwrite(data, 1, (size_t)length, f) == (size_t)length);
        fclose(f);
        *before = g->world;
        Campaign campaign = g->campaign;
        Player player = g->players[0];
        CHECK(!game_load(g, "phase-bad.sav"));
        CHECK(!memcmp(before, &g->world, sizeof(*before)) &&
              !memcmp(&campaign, &g->campaign, sizeof(campaign)));
        CHECK(!memcmp(&player, &g->players[0], sizeof(player)));
    }
    CHECK(game_load(g, "phase-good.sav") && g->campaign.profiles[0].claimed == (1u << 8));
    free(before);
    free(original);
    free(data);
    destroy(g);
    remove("phase-good.sav");
    remove("phase-bad.sav");
}
static void phase_boundaries(void) {
    Game *g = fresh(713);
    Game *open = fresh(713);
    /* Streaming before a boundary opens must preserve encounter spawn positions. */
    Cell population_cell = {4, 0, 0};
    for (int z = 0; z < 12; z++) {
        RegionPlan plan;
        world_plan(g->world.seed, world_region((Cell){4, 0, z}), &plan);
        for (int i = 0; i < plan.count; i++)
            if (plan.rooms[i].habitat.pace == PACE_SWARM &&
                campaign_guardian(plan.rooms[i].cell) < 0) {
                population_cell = plan.rooms[i].cell;
                goto found_population;
            }
    }
found_population:
    g->players[0].pos = open->players[0].pos = position_from(world_center(population_cell));
    open->world.open_layer = 0;
    settle(g);
    settle(open);
    Room *sealed_room = world_find(&g->world, population_cell);
    Room *open_room = world_find(&open->world, population_cell);
    CHECK(sealed_room && open_room && sealed_room->enemy_count > 0 &&
          sealed_room->enemy_count == open_room->enemy_count);
    for (int i = 0; i < sealed_room->enemy_count; i++)
        CHECK(vlen(position_delta(sealed_room->enemies[i].pos, open_room->enemies[i].pos)) == 0);
    destroy(open);
    Cell border = {3, 0, 0};
    g->players[0].pos = position_from(world_center(border));
    settle(g);
    Vec3 wall = v3(336, 1, 48);
    CHECK(world_overlap(&g->world, wall, .34, 1.78));
    CHECK(world_overlap(&g->world, wall, .7, 3.2));
    CHECK(world_overlap(&g->world, vadd(wall, v3(0, 8, 0)), .34, 1.78));
    g->players[0].pos = position_from(world_lift((Cell){0, 1, 0}));
    settle(g);
    Actions actions[2] = {0};
    actions[0].lift_up = true;
    for (int i = 0; i < 10; i++)
        game_step(g, actions, 1.0 / 60);
    CHECK(world_cell(position_absolute(g->players[0].pos)).y == 1);
    CHECK(strstr(g->message, "SEALED"));
    win(g, 0);
    g->players[0].pos = position_from(world_lift((Cell){0, 1, 0}));
    settle(g);
    for (int i = 0; i < 190; i++) {
        world_stream(&g->world, 2);
        game_step(g, actions, 1.0 / 60);
    }
    CHECK(!g->players[0].lift_active);
    CHECK(world_cell(position_absolute(g->players[0].pos)).y == 2);
    destroy(g);
}
static void use_story(Game *g, unsigned scene) {
    g->players[0].pos = position_from(game_terminal(g->story.sites[scene]));
    settle(g);
    for (int r = 0; r < g->world.count; r++)
        if (campaign_guardian(g->world.rooms[r].cell) < 0)
            for (int e = 0; e < g->world.rooms[r].enemy_count; e++)
                if (g->world.rooms[r].enemies[e].alive)
                    game_damage_enemy(g, &g->world.rooms[r], e, 0, 1000000, 0);
    game_interact(g, 0);
}
static void generated_stories(void) {
    unsigned motivations = 0;
    for (unsigned seed = 1; seed <= 16; seed++) {
        Game *g = fresh(seed);
        motivations |= 1u << story_motivation(seed);
        use_story(g, STORY_BRIEFING);
        CHECK(g->story.done & 1);
        use_story(g, g->story.tracked);
        unsigned target = game_story_target(g), op = game_story_operation(g);
        CHECK(g->story.assigned & (1u << target));
        CHECK(campaign_layer(g->story.sites[target]) == 4 &&
              campaign_guardian(g->story.sites[target]) < 0);
        CHECK(campaign_layer(g->story.sites[op]) == 3);
        use_story(g, target);
        CHECK(!g->players[0].decision_open && strstr(g->message, "SEALED"));
        for (unsigned id = 0; id < 8; id += 2)
            win(g, id);
        use_story(g, op);
        CHECK(g->story.done & (1u << op));
        use_story(g, target);
        CHECK(g->players[0].decision_open);
        CHECK(game_story_choose(g, 0, seed % 3 + 1));
        CHECK(story_valid(&g->story, seed, g->world.generator, g->mission));
        CHECK(game_save(g, "phase-story.sav") && game_load(g, "phase-story.sav"));
        use_story(g, STORY_EXTRACTION);
        CHECK(g->players[0].victories == 1);
        CHECK(game_save(g, "phase-story.sav") && game_load(g, "phase-story.sav"));
        unsigned rank = g->players[0].level, owned = g->players[0].weapons;
        use_story(g, STORY_CONTINUE);
        CHECK(g->story.chapter == 1 && g->players[0].level == rank &&
              g->players[0].weapons == owned);
        destroy(g);
    }
    CHECK(motivations == 15);
    Game *port = fresh(713);
    use_story(port, STORY_BRIEFING);
    win(port, 1);
    use_story(port, port->story.tracked);
    CHECK(port->story.sites[game_story_target(port)].x < 0);
    CHECK(port->story.sites[game_story_operation(port)].x < 0);
    CHECK(story_valid(&port->story, 713, port->world.generator, port->mission));
    destroy(port);
    remove("phase-story.sav");
}
static void matrix_on_phased_ship(void) {
    Game *g = fresh(713);
    g->players[0].pos = position_from(matrix_terminal((Cell){0}));
    settle(g);
    CHECK(matrix_toggle(g, 0));
    CHECK(matrix_valid(&g->matrix, 713, 7, g->players, &g->shelter));
    CHECK(game_save(g, "phase-matrix.sav") && game_load(g, "phase-matrix.sav"));
    CHECK(g->matrix.divers[0].active && g->campaign.version == 1);
    CHECK(matrix_toggle(g, 0) && !g->matrix.divers[0].active);
    CHECK(campaign_valid(&g->campaign, g->players, &g->world));
    destroy(g);
    remove("phase-matrix.sav");
}
int main(void) {
    fresh_characters();
    geography();
    rewards_and_loadouts();
    fights_and_retry();
    phases_and_persistence();
    guidance_and_builds();
    malformed_appendix();
    phase_boundaries();
    generated_stories();
    matrix_on_phased_ship();
    puts("PASS fresh ships, directional plans, guardian rewards, loadouts, combat, retry and "
         "versioned saves");
    return 0;
}
