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
static void settle(Game *g, Vec3 at) {
    g->players[0].pos = position_from(at);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending && !g->world.failed)
        CHECK(world_stream(&g->world, 2) <= 2);
    CHECK(!g->world.failed);
}
static Game *fresh(unsigned seed) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, seed);
    settle(g, game_terminal(campaign_site(0)));
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static Room *vista(Game *g) {
    Room *r = world_find(&g->world, campaign_site(0));
    CHECK(r && r->ready && (r->journey.role & JOURNEY_VISTA));
    return r;
}
static void budgets(const World *w) {
    CHECK(w->chunk_count <= SC_MAX_CHUNKS);
    for (int i = 0; i < w->count; i++)
        CHECK(w->rooms[i].enemy_count <= 20);
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *c = &w->chunks[i];
        CHECK(c->box_count <= SC_CHUNK_BOXES && c->light_count <= 6 && c->sprite_count <= 32 &&
              c->dressing_count <= 32 && c->flight_count <= 16);
    }
}
static void planning(void) {
    for (unsigned seed = 1; seed <= 24; seed++) {
        for (int x = -2; x <= 2; x++) {
            Cell key = {x, (int)(seed % 3) - 1, (int)(seed % 4)};
            RegionPlan a, b, old;
            world_plan_version(seed, 16, key, &a);
            world_plan_version(seed, 16, key, &b);
            CHECK(a.count == b.count && a.journey.identity == b.journey.identity);
            CHECK(cell_equal(a.journey.anchor, b.journey.anchor));
            CHECK(!memcmp(a.owner, b.owner, sizeof(a.owner)) &&
                  !memcmp(a.doors, b.doors, sizeof(a.doors)));
            world_plan_version(seed, 15, key, &old);
            CHECK(!old.journey.family);
            CHECK(!memcmp(a.owner, old.owner, sizeof(a.owner)) &&
                  !memcmp(a.doors, old.doors, sizeof(a.doors)));
            for (int i = 0; i < a.count; i++) {
                RoomSpec *r = &a.rooms[i];
                CHECK(r->journey.role == b.rooms[i].journey.role);
                if (r->journey.role)
                    CHECK(r->height >= 5 && r->width >= 48 && r->depth >= 48);
                for (unsigned d = 0; d < 4; d++) {
                    Cell next = world_neighbor(r->cell, (Direction)d);
                    CHECK(world_door_version(seed, 16, r->cell, (Direction)d) ==
                          world_door_version(seed, 16, next, (Direction)(d ^ 1)));
                }
            }
        }
        Game *g = fresh(seed);
        budgets(&g->world);
        Room *r = vista(g);
        CHECK(r->height >= 32 && r->width >= 80);
        DistrictContract c = district_contract(seed, 16, r->cell);
        CHECK(district_compatible(c.family, c.condition) &&
              c.condition == district_condition(seed) && c.reservations == 7 &&
              cell_equal(c.anchor, r->cell));
        size_t records = g->world.record_count;
        (void)district_contract(seed, 16, (Cell){100, 5, 100});
        CHECK(records == g->world.record_count); /* Surveying is read-only. */
        destroy(g);
    }
    RegionPlan tiny = {.count = 1,
                       .rooms = {{.type = CONDUIT, .height = 2, .width = 12, .depth = 12}}};
    district_plan(1, &tiny);
    CHECK(!tiny.journey.family);
    RegionPlan compact = {
        .count = 1,
        .rooms = {{.cell = {8, 0, 8}, .type = NAVE, .height = 8, .width = 60, .depth = 60}}};
    district_plan(1, &compact);
    CHECK((compact.rooms[0].journey.role & 15) == 15);
    for (unsigned i = 0; i < 6; i++)
        CHECK(strlen(district_name(i)) && strlen(district_condition_name(i)));
    World empty = {0};
    CHECK(!world_record_systems(&empty, (Cell){0}) && !world_district_state(&empty, (Cell){0}));
    CHECK(!empty.records && !empty.index_cap && !empty.failed);
}
static void reverse_loading(void) {
    for (unsigned seed = 1; seed <= 6; seed++) {
        Game *a = fresh(seed), *b = fresh(seed);
        settle(b, world_center((Cell){3, 0, 3}));
        settle(b, game_terminal(campaign_site(0)));
        for (int i = 0; i < SC_MAX_CHUNKS; i++) {
            const Chunk *x = &a->world.chunks[i];
            if (!x->active)
                continue;
            const Chunk *y = world_chunk(&b->world, x->cell);
            CHECK(y && y->box_count == x->box_count && y->sprite_count == x->sprite_count);
            for (size_t j = 0; j < x->box_count; j++)
                CHECK(vlen(vsub(x->boxes[j].min, y->boxes[j].min)) < .00001 &&
                      vlen(vsub(x->boxes[j].max, y->boxes[j].max)) < .00001);
        }
        budgets(&b->world);
        destroy(a);
        destroy(b);
    }
}
static void power_and_water(void) {
    Game *g = fresh(713), *copy = fresh(713);
    Room *r = vista(g);
    DistrictMark mark = r->journey;
    DistrictContract c = district_contract(g->world.seed, 16, r->cell);
    CHECK(!district_powered(&g->world, &mark));
    Room dummy = *r;
    dummy.cell = (Cell){3, 0, 3};
    dummy.frontier_active = false;
    dummy.alarm_time = 0;
    Enemy robot = {.kind = 27};
    CHECK(district_inert(g, &dummy, &robot));
    robot.force = 1;
    CHECK(!district_inert(g, &dummy, &robot));
    robot.force = 0;
    CHECK(!district_interact(g, -1) && !district_interact(g, 2));
    g->players[0].sheltered = true;
    CHECK(!district_interact(g, 0));
    g->players[0].sheltered = false;
    Vec3 source = vadd(district_control(r->cell, DISTRICT_SOURCE_CONTROL), v3(0, .75, 0));
    district_shot(g, 0, source);
    CHECK(!r->district_arc);
    settle(g, district_control(c.preparation, JOURNEY_PUMP));
    CHECK(district_interact(g, 0) && district_interact(g, 0));
    r = vista(g);
    CHECK(district_powered(&g->world, &mark));
    district_step(g, .02);
    CHECK(r->district_warning > 1.9 && r->district_powered);
    dummy.district_powered = true;
    dummy.district_warning = 1;
    CHECK(district_inert(g, &dummy, &robot));
    dummy.district_warning = 0;
    CHECK(!district_inert(g, &dummy, &robot));
    district_step(g, 2.1);
    CHECK(!r->district_warning);
    Vec3 center = world_center(r->cell);
    CHECK(!district_wet(r, vadd(center, v3(0, .02, 20))) &&
          district_wet(r, vadd(center, v3(-8, .02, 20))));
    CHECK(!district_wet(r, vadd(center, v3(-8, 3, 20))) &&
          !district_wet(r, vadd(center, v3(-8, .02, 3))));
    /* Basic carbine shoots the marked conductor from the walkable raised route. */
    settle(g, vadd(center, v3(-23, 3.02, 20)));
    r = vista(g);
    Vec3 to = vsub(source, player_eye(&g->players[0]));
    GameCommand aim = {.kind = CMD_AIM,
                       .player = game_player_id(g, 0),
                       .x = atan2(to.x, to.z),
                       .y = asin(vnorm(to).y)};
    Session s;
    session_init(&s, g);
    CHECK(session_command(&s, &aim));
    Actions input = {.fire = true};
    CHECK(session_input(&s, game_player_id(g, 0), &input));
    for (int i = 0; i < 120 && !r->district_arc; i++)
        session_tick(&s);
    CHECK(r->district_arc > 6.9 && r->district_owner == 1);
    CHECK(world_district_state(&g->world, r->cell) & DISTRICT_CONDUCTOR);
    g->players[0].pos = position_from(vadd(center, v3(-8, .02, 20)));
    g->players[0].shield = 0;
    g->players[0].armor = 0;
    float health = g->players[0].health;
    district_step(g, 1.9);
    CHECK(g->players[0].health == health);
    district_step(g, .2);
    district_step(g, .05);
    CHECK(g->players[0].health < health);
    health = g->players[0].health;
    g->players[0].pos = position_from(vadd(center, v3(8, .02, 20)));
    district_step(g, .05);
    CHECK(g->players[0].health == health); /* Disconnected water. */
    unsigned char *data;
    size_t size;
    CHECK(game_snapshot_encode(g, &data, &size) && game_snapshot_decode(copy, data, size));
    free(data);
    CHECK(fabsf(vista(copy)->district_arc - r->district_arc) < .00001);
    Session paused;
    session_init(&paused, g);
    float clock = r->district_arc;
    session_advance(&paused, 2, true);
    CHECK(r->district_arc == clock);
    g->players[0].sheltered = true;
    district_step(g, .1);
    CHECK(r->district_arc == clock);
    g->players[0].sheltered = false;
    CHECK(game_save(g, "district.sav") && game_load(copy, "district.sav"));
    CHECK(world_district_state(&copy->world, r->cell) & DISTRICT_CONDUCTOR);
    district_step(g, 8);
    CHECK(!r->district_arc);
    district_shot(g, 0, source);
    CHECK(!r->district_arc);
    uint32_t saved = world_district_state(&g->world, r->cell);
    CHECK(!world_set_district_state(&g->world, r->cell, 64) &&
          !world_set_district_state(&g->world, (Cell){0, 0, -1}, 1));
    CHECK(world_set_district_state(&g->world, r->cell, saved));
    size_t index = r->record;
    CHECK(index != SIZE_MAX);
    g->world.records[index].district_state = 64;
    CHECK(!game_snapshot_encode(g, &data, &size));
    CHECK(game_save(g, "district-bad.sav") && !game_load(copy, "district-bad.sav"));
    g->world.records[index].district_state = saved;
    settle(g, world_center((Cell){8, 0, 8}));
    CHECK(!world_find(&g->world, mark.anchor));
    settle(g, game_terminal(mark.anchor));
    r = vista(g);
    CHECK(world_district_state(&g->world, r->cell) == saved);
    CHECK(district_powered(&g->world, &mark));
    g->world.generator = 15;
    CHECK(!world_set_district_state(&g->world, r->cell, 1));
    district_step(g, .1);
    CHECK(!district_interact(g, 0));
    remove("district.sav");
    remove("district-bad.sav");
    destroy(copy);
    destroy(g);
}
static void routes(void) {
    const unsigned seeds[] = {1, 2, 3, 6, 7, 13, 27, 713};
    for (unsigned index = 0; index < 8; index++) {
        unsigned seed = seeds[index];
        Game *g = fresh(seed);
        Room *r = vista(g);
        Vec3 c = world_center(r->cell);
        for (int j = 0; j < g->world.count; j++)
            g->world.rooms[j].enemy_count = 0;
        settle(g, vadd(c, v3(-23, .02, -3)));
        r = vista(g);
        Session s;
        session_init(&s, g);
        Actions input = {.forward = 1};
        CHECK(session_input(&s, game_player_id(g, 0), &input));
        g->players[0].yaw = 0;
        double highest = 0;
        for (int i = 0; i < 900 && position_absolute(g->players[0].pos).z < c.z + 42; i++) {
            session_tick(&s);
            highest = fmax(highest, position_absolute(g->players[0].pos).y - c.y);
        }
        CHECK(position_absolute(g->players[0].pos).z >= c.z + 42 && highest >= 2.9 && !g->respawns);
        CHECK(!g->players[0].flight && !g->players[0].leap);
        g->players[0].pos = position_from(district_control(r->cell, 0));
        CHECK(district_interact(g, 0));
        CHECK(vlen(vsub(position_absolute(g->players[0].pos), district_control(r->cell, 0))) < .01);
        district_victory(g, r->cell);
        CHECK(world_district_state(&g->world, r->cell) & DISTRICT_SHORTCUT);
        CHECK(district_interact(g, 0));
        CHECK(vlen(vsub(position_absolute(g->players[0].pos),
                        district_control(r->cell, DISTRICT_RETURN_CONTROL))) < .01);
        CHECK(district_interact(g, 0));
        for (unsigned rig = 1; rig <= 2; rig++) {
            campaign_grant(g, 0, (CampaignReward){REWARD_RIG, rig});
            g->players[0].riding = (uint8_t)rig;
            g->rigs[0][rig - 1].deployed = true;
            g->rigs[0][rig - 1].pos = g->players[0].pos;
            CHECK(district_interact(g, 0));
            CHECK(vlen(position_delta(g->rigs[0][rig - 1].pos, g->players[0].pos)) < .01);
            g->players[0].riding = 0;
            g->rigs[0][rig - 1].deployed = false;
        }
        budgets(&g->world);
        destroy(g);
    }
}
int main(void) {
    planning();
    reverse_loading();
    power_and_water();
    routes();
    puts("PASS deterministic districts, bounded generation, shared power, conductive water and "
         "basic routes");
}
