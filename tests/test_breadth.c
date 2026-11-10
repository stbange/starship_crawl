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
static Game *fresh(unsigned seed, Cell cell) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, seed);
    Vec3 at = vadd(world_center(cell), v3(0, .02, 12));
    g->players[0].pos = position_from(at);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed);
    return g;
}
static void done(Game *g) {
    game_destroy(g);
    free(g);
}
static void equip(Game *g, BuildBranch branch) {
    g->campaign.profiles[0].modules |= 1u << (branch - 1);
    g->campaign.profiles[0].module = branch;
    g->builds[0] = (BuildEffects){0};
}
static Room *targets(Game *g) {
    for (int i = 0; i < g->world.count; i++)
        g->world.rooms[i].enemy_count = 0;
    Room *r = world_find(&g->world, (Cell){0});
    CHECK(r);
    r->enemy_count = 12;
    for (int i = 0; i < r->enemy_count; i++)
        r->enemies[i] = (Enemy){.alive = true,
                                .kind = 1,
                                .health = 500,
                                .id = (uint8_t)i,
                                .pos = position_from(v3((i % 3 - 1) * .2, .02, 15 + i * .1)),
                                .normal = {0, 1, 0}};
    return r;
}
static void conditions(void) {
    for (unsigned condition = 2; condition <= 3; condition++) {
        Game *g = fresh(condition == 2 ? 42 : 1, campaign_site(0));
        Game *copy = fresh(713, (Cell){0});
        Room *r = world_find(&g->world, campaign_site(0));
        CHECK(r);
        CHECK(r->journey.condition == condition);
        Vec3 center = world_center(r->cell), wet = vadd(center, v3(-10, .02, 20));
        CHECK(condition_surface(r, wet));
        CHECK(!condition_surface(r, vadd(center, v3(0, .02, 20))));
        CHECK(!condition_surface(r, vadd(center, v3(-10, 3, 20))));
        g->players[0].pos = position_from(wet);
        g->players[0].shield = g->players[0].armor = 0;
        r->district_powered = true;
        float offset = (float)((r->journey.identity >> 8) % 3) * 4;
        g->condition_clock = fmodf(24 - offset, 24);
        CHECK(!condition_live(g, r));
        float hp = g->players[0].health;
        condition_step(g, .25);
        CHECK(g->players[0].health == hp);
        g->condition_clock = fmodf(29 - offset, 24);
        CHECK(condition_live(g, r));
        condition_step(g, .25);
        CHECK(g->players[0].health < hp);
        CHECK(condition == 2 ? g->players[0].slow > 0 : g->players[0].energy < 100);
        g->players[0].pos = position_from(vadd(center, v3(0, .02, 20)));
        hp = g->players[0].health;
        condition_step(g, .25);
        CHECK(g->players[0].health == hp);
        float clock = g->condition_clock;
        g->players[0].sheltered = true;
        condition_step(g, 10);
        CHECK(g->condition_clock == clock);
        g->players[0].sheltered = false;
        g->matrix.divers[0].active = true;
        condition_step(g, .1);
        CHECK(g->condition_clock == clock);
        g->matrix.divers[0].active = false;
        CHECK(game_save(g, "breadth.sav") && game_load(copy, "breadth.sav"));
        CHECK(copy->condition_clock == clock);
        unsigned char *bytes;
        size_t size;
        CHECK(game_snapshot_encode(g, &bytes, &size) && game_snapshot_decode(copy, bytes, size));
        free(bytes);
        CHECK(copy->condition_clock == clock);
        g->condition_clock = 24;
        CHECK(!game_snapshot_encode(g, &bytes, &size));
        CHECK(game_save(g, "breadth-bad.sav") && !game_load(copy, "breadth-bad.sav"));
        g->condition_clock = clock;
        CHECK(world_set_systems(&g->world, r->journey.anchor, SYSTEM_SILENCED));
        CHECK(!condition_live(g, r));
        CHECK(world_set_systems(&g->world, r->journey.anchor, SYSTEM_POWER));
        CHECK(world_set_district_state(&g->world, r->journey.anchor, DISTRICT_CONDUCTOR));
        CHECK(!condition_live(g, r));
        CHECK(world_set_district_state(&g->world, r->journey.anchor, 0));
        /* A regular district uses the same surface rule for its resident creatures. */
        Cell original = r->cell;
        r->cell = (Cell){20, 0, 20};
        r->enemy_count = 4;
        for (int i = 0; i < 4; i++)
            r->enemies[i] =
                (Enemy){.alive = true,
                        .kind = 1,
                        .health = 1,
                        .pos = position_from(vadd(world_center(r->cell), v3(-10, .02, 20)))};
        r->enemies[1].force = 1;
        r->enemies[2].kind = 27;
        r->enemies[3].health = 1000;
        condition_step(g, .25);
        if (condition == 2)
            CHECK(r->enemies[0].slow > 0 && !r->enemies[1].slow);
        else
            CHECK(r->enemies[0].health == 3 && r->enemies[1].health == 1 &&
                  r->enemies[2].health == 1 && r->enemies[3].health == 1000);
        r->enemies[0].burn = 1;
        hp = r->enemies[0].health;
        condition_step(g, .25);
        CHECK(condition == 2 || r->enemies[0].health == hp);
        r->cell = original;
        g->world.generator = 15;
        clock = g->condition_clock;
        condition_step(g, .1);
        CHECK(g->condition_clock == clock && !condition_live(g, r));
        done(copy);
        done(g);
    }
    remove("breadth.sav");
    remove("breadth-bad.sav");
}
static void branches(void) {
    Game *g = fresh(713, (Cell){0}), *copy = fresh(713, (Cell){0});
    Player *p = &g->players[0];
    Room *r = targets(g);
    /* Contact pulse: stable target, bounded charge, piercing lane and finite energy. */
    equip(g, BUILD_FOCUS);
    CHECK(!game_build_active(g, 0, BUILD_FOCUS));
    campaign_grant(g, 0, (CampaignReward){REWARD_GUN, 16});
    CHECK(game_build_active(g, 0, BUILD_FOCUS));
    p->weapon = 16;
    p->energy = 0;
    for (int i = 0; i < 30; i++) {
        g->time += .1;
        game_build_contact(g, r, 0, 0, 10);
    }
    CHECK(g->builds[0].focus == 1.2f && r->enemies[0].health == 500);
    p->energy = 50;
    g->time += .1;
    game_build_contact(g, r, 0, 0, 10);
    CHECK(p->energy == 38 && !g->builds[0].focus && g->builds[0].relays[0].life > 0);
    unsigned hit = 0;
    for (int i = 0; i < 12; i++)
        hit += r->enemies[i].health < 500;
    CHECK(hit == 3);
    g->time += .1;
    game_build_contact(g, r, 0, 0, 10);
    CHECK(g->builds[0].focus > 0);
    g->time += .1;
    game_build_contact(g, r, 1, 0, 10);
    CHECK(!g->builds[0].focus);
    g->time += 2;
    game_build_branches_step(g, .1);
    CHECK(!g->builds[0].focus);
    /* First hit after cloak marks once; its defeat emits one capped burst. */
    equip(g, BUILD_AMBUSH);
    CHECK(!game_build_active(g, 0, BUILD_AMBUSH));
    campaign_grant(g, 0, (CampaignReward){REWARD_POWER, PSI_VEIL});
    p->cloaked = true;
    WeaponDef shot = sc_weapons[0];
    game_build_shot(g, 0, &shot);
    CHECK((shot.flags & W_MARK) && g->builds[0].mark_armed);
    game_build_hit(g, r, 0, -1, shot.flags);
    game_build_hit(g, r, 0, 0, W_BUILD | W_MARK);
    CHECK(g->builds[0].mark_armed);
    game_damage_enemy(g, r, 0, 0, 1, shot.flags);
    CHECK(g->builds[0].mark_life == 6 && !g->builds[0].mark_armed);
    ActorId marked = g->builds[0].mark_target;
    game_damage_enemy(g, r, 1, 0, 1, shot.flags);
    CHECK(game_actor_same(marked, g->builds[0].mark_target));
    unsigned char *data;
    size_t size;
    CHECK(game_snapshot_encode(g, &data, &size) && game_snapshot_decode(copy, data, size));
    free(data);
    CHECK(game_actor_same(copy->builds[0].mark_target,
                          game_actor_id(copy, world_find(&copy->world, r->cell),
                                        &world_find(&copy->world, r->cell)->enemies[0])));
    CHECK(copy->builds[0].mark_life == 6);
    game_build_kill(g, r, 0, -1);
    game_damage_enemy(g, r, 0, 0, 10000, 0);
    CHECK(!g->builds[0].mark_life && g->builds[0].pulse_life > 0);
    CHECK(p->kills == 1); /* Neighbors survive; no synthetic kills from the marker. */
    r = targets(g);
    r->enemies[0].allegiance = 1;
    r->enemies[1].alive = false;
    game_build_wave(g, 0, player_eye(p), 8, 10, false);
    CHECK(r->enemies[0].health == 500 && r->enemies[10].health == 500);
    p->sheltered = true;
    float hp = r->enemies[2].health;
    game_build_wave(g, 0, player_eye(p), 8, 10, false);
    CHECK(r->enemies[2].health == hp);
    p->sheltered = false;
    game_build_wave(g, -1, player_eye(p), 8, 10, false);
    /* Brace only mitigates eligible frontal attacks after a stationary commitment. */
    equip(g, BUILD_BRACE);
    CHECK(!game_build_active(g, 0, BUILD_BRACE));
    campaign_grant(g, 0, (CampaignReward){REWARD_RIG, MECH});
    p->riding = MECH;
    p->grounded = true;
    p->velocity = v3(0, 0, 0);
    p->yaw = 0;
    for (int i = 0; i < 4; i++)
        game_build_branches_step(g, .25);
    CHECK(g->builds[0].brace_time == 1);
    /* A powered refuge prevents charge farming. A real combat location permits it. */
    CHECK(world_set_systems(&g->world, (Cell){0}, SYSTEM_POWER | SYSTEM_BEACON));
    p->pos = position_from(game_terminal((Cell){0}));
    Vec3 front = vadd(player_eye(p), v3(0, 0, 5));
    game_hostile_hit_from(g, 0, 100, 0, front);
    CHECK(!g->builds[0].brace_charge);
    CHECK(world_set_systems(&g->world, (Cell){0}, 0));
    p->pos = position_from(v3(0, .02, 60));
    p->shield = 0;
    g->rigs[0][0].armor = 500;
    front = vadd(player_eye(p), v3(0, 0, 5));
    game_hostile_hit_from(g, 0, 100, 0, front);
    CHECK(g->builds[0].brace_charge == 50);
    game_hostile_hit_from(g, 0, 10, 0, vadd(player_eye(p), v3(0, 0, -5)));
    CHECK(g->builds[0].brace_charge == 50);
    game_hostile_hit_from(g, 0, 500, 0, front);
    CHECK(g->builds[0].brace_charge == 100);
    game_hostile_hit_from(g, -1, 10, 0, front);
    game_hostile_hit_from(g, 0, NAN, 0, front);
    shot = sc_mech_weapons[0];
    game_build_shot(g, 0, &shot);
    CHECK(!g->builds[0].brace_charge);
    p->velocity.x = 1;
    game_build_branches_step(g, .25);
    CHECK(!g->builds[0].brace_time);
    /* Flight field and committed leap are exclusive and consume no invented unlocks. */
    p->riding = 0;
    p->pos = position_from(v3(0, .02, 12));
    r = targets(g);
    equip(g, BUILD_FIELD);
    CHECK(!game_build_active(g, 0, BUILD_FIELD));
    campaign_grant(g, 0, (CampaignReward){REWARD_POWER, PSI_FLIGHT});
    p->flight = 10;
    p->energy = 10;
    game_build_branches_step(g, .25);
    CHECK(p->energy == 8 && g->builds[0].pulse_life > 0);
    p->energy = 1;
    game_build_branches_step(g, .25);
    CHECK(!p->flight && p->energy == 1);
    equip(g, BUILD_LANDING);
    campaign_grant(g, 0, (CampaignReward){REWARD_POWER, PSI_LEAP});
    p->grounded = false;
    g->builds[0].landing_armed = true;
    hp = r->enemies[0].health;
    game_build_branches_step(g, .25);
    CHECK(r->enemies[0].health == hp);
    p->grounded = true;
    game_build_branches_step(g, .25);
    CHECK(!g->builds[0].landing_armed && r->enemies[0].health < hp);
    hp = r->enemies[0].health;
    game_build_branches_step(g, .25);
    CHECK(r->enemies[0].health == hp);
    g->builds[0].landing_armed = true;
    p->flight = 1;
    game_build_branches_step(g, .1);
    CHECK(!g->builds[0].landing_armed);
    p->flight = 0;
    p->grounded = true;
    p->leap = 10;
    p->velocity = v3(0, 0, 0);
    Actions input[SC_PLAYERS] = {0};
    input[0].jump = true;
    game_step(g, input, SC_TICK_SECONDS);
    CHECK(g->builds[0].landing_armed && !p->grounded);
    CHECK(game_save(g, "branch-live.sav") && game_load(copy, "branch-live.sav"));
    CHECK(!copy->builds[0].landing_armed && !copy->builds[0].pulse_life);
    g->campaign.version = 1;
    game_build_branches_step(g, .1);
    remove("branch-live.sav");
    done(copy);
    done(g);
}
static void inventory(void) {
    unsigned matrix[5] = {0}, roles = 0, motives = 0, counts = 0;
    for (unsigned seed = 1; seed <= 48; seed++) {
        Game *g = fresh(seed, (Cell){0});
        CHECK(g->chronicle.count == 2 || g->chronicle.count == 3);
        counts |= 1u << g->chronicle.count;
        motives |= 1u << story_motivation_version(seed, g->story.version);
        for (unsigned i = 0; i < g->chronicle.count; i++) {
            RecurringPerson *p = &g->chronicle.people[i];
            roles |= 1u << p->role;
            RoomSpec spec = world_describe_version(seed, 16, p->site);
            CHECK(spec.journey.role & JOURNEY_REFUGE);
            CHECK(chronicle_valid(&g->chronicle, seed, 16));
            Vec3 meeting = chronicle_position(p);
            CHECK(world_refresh(&g->world, &meeting, 1));
            while (g->world.pending && !g->world.failed)
                world_stream(&g->world, 2);
            CHECK(!g->world.failed && world_spawn_clear(&g->world, meeting, .55, 2.2));
            CHECK(world_spawn_supported(&g->world, meeting, 2.2));
            for (unsigned j = 0; j < i; j++) {
                RecurringPerson other = g->chronicle.people[j];
                other.site = p->site;
                CHECK(vlen(vsub(chronicle_position(p), chronicle_position(&other))) > 3);
            }
        }
        for (int axis = 0; axis < 3; axis++)
            for (int sign = -1; sign <= 1; sign += 2) {
                Cell key = {axis == 0 ? sign * 4 : 0, axis == 1 ? sign * 4 : 0,
                            axis == 2 ? 8 + sign * 4 : 8};
                RegionPlan plan;
                world_plan_version(seed, 16, key, &plan);
                if (plan.journey.family) {
                    CHECK(district_compatible(plan.journey.family, plan.journey.condition));
                    CHECK(plan.journey.condition == district_condition(seed));
                    matrix[plan.journey.family] |= 1u << plan.journey.condition;
                }
            }
        done(g);
    }
    CHECK(roles == 126 && motives == 15 && counts == 12);
    CHECK(matrix[1] == 10 && matrix[2] == 6 && matrix[3] == 14 && matrix[4] == 12);
    CHECK(!district_compatible(0, 1) && !district_compatible(5, 2) && !district_compatible(1, 4));
    puts("inventory: four districts, three conditions, six recurring roles, four motivations");
}
int main(void) {
    conditions();
    branches();
    inventory();
    puts("PASS coherent breadth rules, counterplay and persistent boundaries");
}
