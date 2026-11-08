#include "faction_fixture.h"
static void step(Game *g, unsigned frames) {
    for (unsigned i = 0; i < frames; i++)
        hidden_director_step(g, .05);
}
static void interventions(void) {
    for (unsigned kind = EDIT_SCOUT; kind < DIRECTOR_EDITS; kind++) {
        Game *g = fixture(713);
        Cell start = ordinary(g, (Cell){0, 0, 2}), site = ordinary(g, (Cell){8, 0, 8});
        load_area(g, start);
        empty_actors(g);
        FactionRegion *r = factions_record(g, site);
        CHECK(r);
        r->owner = 1;
        uint32_t force = 0;
        if (kind == EDIT_ROUTE || kind == EDIT_DELAY || kind == EDIT_WITHDRAW) {
            CHECK(factions_launch(g, 1, ORDER_PATROL, site, site));
            force = g->factions.next_id - 1;
        }
        CHECK(hidden_director_propose(g, kind, 0, site, force));
        CHECK(!hidden_director_propose(g, kind, 0, site, force));
        CHECK(g->factions.director.budget == 3 && g->factions.director.proposal.phase == 1);
        CHECK(factions_valid(&g->factions, 14));
        g->players[0].pos = g->players[0].checkpoint;
        CHECK(game_save(g, "director-resume.sav"));
        CHECK(game_load(g, "director-resume.sav"));
        g->world.open_layer = 0;
        load_area(g, start);
        empty_actors(g);
        step(g, 62);
        CHECK(g->factions.director.accepted == 1 && g->factions.director.budget == 2);
        CHECK(g->factions.director.proposal.phase == 3);
        step(g, 22);
        CHECK(!g->factions.director.proposal.phase);
        CHECK(g->factions.director.accepted == 1 && factions_valid(&g->factions, 14));
        r = (FactionRegion *)factions_at(&g->factions, site);
        CHECK(r);
        if (kind == EDIT_STASH || kind == EDIT_NEST || kind == EDIT_CANCEL ||
            kind == EDIT_SUBSTITUTE || kind == EDIT_WEATHER) {
            CHECK(r->edit == kind);
            load_area(g, site);
            Room *room = world_find(&g->world, site);
            CHECK(room);
            if (kind == EDIT_SUBSTITUTE) {
                room->habitat.ecology = MECHANICAL;
                room->enemy_count = 1;
                room->enemies[0] = (Enemy){.alive = true,
                                           .kind = 0,
                                           .health = 60,
                                           .pos = position_from(world_center(site))};
            }
            factions_environment(g);
            CHECK(room->faction_edit == kind);
            unsigned after = (unsigned)room->enemy_count;
            factions_environment(g);
            CHECK((unsigned)room->enemy_count == after);
            if (kind == EDIT_NEST)
                CHECK(after == 16);
            if (kind == EDIT_CANCEL)
                for (int e = 0; e < room->enemy_count; e++)
                    CHECK(!room->enemies[e].alive || room->enemies[e].country);
            if (kind == EDIT_WEATHER)
                CHECK(room->habitat.atmosphere == VOID_GLOW);
            if (kind == EDIT_SUBSTITUTE) {
                CHECK(after == 1 && room->enemies[0].kind >= 27 && room->enemies[0].kind <= 29);
                CHECK(room->enemies[0].health == sc_enemies[room->enemies[0].kind].health * 2);
                world_kill(&g->world, room, 0);
                factions_environment(g);
                CHECK(!room->enemies[0].alive);
            }
            if (kind == EDIT_STASH) {
                g->players[0].pos = position_from(vadd(world_center(site), v3(0, .02, 3)));
                unsigned credits = g->players[0].credits;
                factions_environment(g);
                CHECK(g->players[0].credits == credits + 2 && r->edit_claims);
                factions_environment(g);
                CHECK(g->players[0].credits == credits + 2);
            }
        }
        remove("director-resume.sav");
        finish(g);
    }
}
static void safety_and_pressure(void) {
    Game *g = fixture(42);
    Cell start = ordinary(g, (Cell){0, 0, 2}), site = ordinary(g, (Cell){8, 0, 8});
    load_area(g, start);
    empty_actors(g);
    CHECK(!hidden_director_propose(g, 0, 0, site, 0));
    CHECK(!hidden_director_propose(g, EDIT_NEST, 0, start, 0));
    CHECK(!hidden_director_propose(g, EDIT_NEST, 0, (Cell){0}, 0));
    CHECK(!hidden_director_propose(g, EDIT_ROUTE, 0, site, 999));
    g->matrix.divers[0].active = true;
    CHECK(!hidden_director_propose(g, EDIT_NEST, 0, site, 0));
    step(g, 100);
    CHECK(!g->factions.director.accepted && !g->factions.director.sequence);
    g->matrix.divers[0].active = false;
    CHECK(hidden_director_propose(g, EDIT_NEST, 0, site, 0));
    /* A separated partner sees the reservation before commit: it must cancel without cost. */
    game_join(g);
    g->players[1].pos = position_from(vadd(world_center(site), v3(0, .02, 0)));
    step(g, 62);
    CHECK(g->factions.director.rejected == 1 && g->factions.director.budget == 3);
    CHECK(!factions_at(&g->factions, site)->edit);
    step(g, 22);
    g->players[1].sheltered = true;
    FactionRegion *r = (FactionRegion *)factions_at(&g->factions, site);
    r->seen = 65535;
    CHECK(!hidden_director_propose(g, EDIT_NEST, 0, site, 0));
    r->seen = 0;
    for (unsigned i = 0; i < 4; i++)
        g->factions.director.history[i] = EDIT_NEST;
    CHECK(!hidden_director_propose(g, EDIT_NEST, 0, site, 0));
    g->factions.director.budget = 0;
    CHECK(!hidden_director_propose(g, EDIT_STASH, 0, site, 0));
    g->players[0].health = 10;
    step(g, 80);
    CHECK(g->factions.director.pace[0] == PACE_RECOVERY && g->factions.director.recovery[0] == 45);
    g->players[0].health = player_max_health(&g->players[0]);
    step(g, 100);
    CHECK(g->factions.director.pace[0] == PACE_RECOVERY);
    g->players[0].sheltered = true;
    float recovery = g->factions.director.recovery[0];
    step(g, 100);
    CHECK(g->factions.director.recovery[0] == recovery);
    g->players[0].sheltered = false;
    step(g, 1100);
    CHECK(g->factions.director.budget + g->factions.director.accepted > 0);
    CHECK(factions_valid(&g->factions, 14));
    finish(g);
}
static void experience_and_cancellation(void) {
    Game *g = fixture(713);
    Cell start = ordinary(g, (Cell){10, 0, 10}), site = ordinary(g, (Cell){20, 0, 20});
    load_area(g, start);
    empty_actors(g);
    g->players[0].pos = position_from(vadd(world_center(start), v3(0, .02, 2)));
    Room *room = world_find(&g->world, start);
    room->enemy_count = 2;
    for (unsigned e = 0; e < 2; e++)
        room->enemies[e] =
            (Enemy){.alive = true,
                    .kind = 3,
                    .health = 30,
                    .pos = position_from(vadd(world_center(start), v3(e * 2, .02, 8)))};
    game_join(g);
    g->players[1].pos = position_from(world_center((Cell){40, 0, 40}));
    for (unsigned i = 1; i <= 7; i++) {
        g->factions.tick = i * 5;
        g->factions.remainder = 0;
        g->factions.director.cooldown = 100;
        hidden_director_step(g, .05);
    }
    CHECK(g->factions.director.pace[0] == PACE_RECOVERY && g->factions.director.recovery[0] > 40);
    CHECK(!g->factions.director.pressure[1] && !g->factions.director.recovery[1]);
    CHECK(g->players[0].health == player_max_health(&g->players[0]));
    empty_actors(g);
    g->factions.director.recovery[0] = 0;
    CHECK(hidden_director_propose(g, EDIT_NEST, 0, site, 0));
    g->players[0].health = 10;
    unsigned budget = g->factions.director.budget;
    step(g, 62);
    CHECK(g->factions.director.rejected == 1 && g->factions.director.budget == budget);
    CHECK(!factions_at(&g->factions, site)->edit);
    g->players[0].health = player_max_health(&g->players[0]);
    step(g, 22);
    g->factions.director.recovery[0] = 0;
    g->factions.director.pressure[0] = 0;
    g->factions.director.quiet[0] = 80;
    g->factions.director.cooldown = 100;
    g->factions.tick = 36;
    hidden_director_step(g, .05);
    CHECK(g->factions.director.pace[0] == PACE_DISCOVERY);
    /* Empty-room travel preserves quiet time; map/cache queries do not create memories. */
    unsigned count = g->factions.event_count;
    MapView map;
    CHECK(game_map_build(g, 0, (Cell){60, 2, 60}, 3, &map));
    CHECK(g->factions.event_count == count);
    g->players[0].pos = position_from(world_center((Cell){start.x + 2, start.y, start.z}));
    hidden_director_step(g, .05);
    CHECK(g->factions.director.quiet[0] >= 80);
    CHECK(game_save(g, "experience.sav") && game_load(g, "experience.sav"));
    CHECK(g->factions.director.quiet[0] >= 80 && g->factions.event_count > count);
    remove("experience.sav");
    finish(g);
}
static void avoid_and_repeat(void) {
    Game *g = fixture(713);
    Cell start = ordinary(g, (Cell){10, 0, 10}), site = ordinary(g, (Cell){20, 0, 20});
    load_area(g, start);
    empty_actors(g);
    CHECK(factions_launch(g, 1, ORDER_PATROL, site, site));
    FactionForce *a = factions_force(g, 1);
    g->players[0].pos = position_from(vadd(world_center(start), v3(0, .02, 2)));
    a->members[0].pos = position_from(vadd(world_center(start), v3(0, .02, 8)));
    Room *room = world_find(&g->world, start);
    room->enemies[0] = a->members[0];
    room->enemy_count = 1;
    hidden_director_step(g, .05);
    g->players[0].pos = position_from(world_center((Cell){14, 0, 14}));
    hidden_director_step(g, .05);
    CHECK(!hidden_director_propose(g, EDIT_ROUTE, 0, site, 1));
    factions_record(g, site)->owner = 1;
    CHECK(!hidden_director_propose(g, EDIT_SCOUT, 0, site, 0));
    /* Three observed matching formations suppress another pressure encounter. */
    for (unsigned i = 0; i < 3; i++)
        factions_log(g, FE_EXPERIENCE, 1, 1, start,
                     EXPERIENCE_CONTENT | (HUSK << 4) | (ORDER_RAID << 8));
    g->factions.director.cooldown = 0;
    hidden_director_step(g, .05);
    CHECK(!g->factions.director.proposal.phase || g->factions.director.proposal.kind == EDIT_STASH);
    /* The same intervention may return after a genuinely quiet two-minute interval. */
    memset(&g->factions.director.proposal, 0, sizeof(g->factions.director.proposal));
    for (unsigned i = 0; i < 4; i++)
        g->factions.director.history[i] = EDIT_WEATHER;
    g->factions.tick = 61;
    CHECK(hidden_director_propose(g, EDIT_WEATHER, 0, site, 0));
    CHECK(factions_valid(&g->factions, 14));
    finish(g);
}
static void guardian_footprint(void) {
    Game *g = fixture(2026);
    Cell alias = {-10, 0, 2};
    RoomSpec room = world_describe_version(g->world.seed, 14, alias);
    CHECK(campaign_guardian(alias) < 0 && campaign_guardian(room.cell) == 5);
    CHECK(factions_safe(g, alias) && factions_required(g, alias));
    load_area(g, ordinary(g, (Cell){2, 0, 4}));
    CHECK(!hidden_director_propose(g, EDIT_STASH, 0, alias, 0));
    CHECK(!factions_launch(g, 1, ORDER_RAID, alias, (Cell){8, 0, 8}));
    CHECK(!factions_at(&g->factions, alias));
    finish(g);
}
int main(void) {
    interventions();
    safety_and_pressure();
    experience_and_cancellation();
    avoid_and_repeat();
    guardian_footprint();
    puts("PASS hidden intervention lifecycle, real content, shared cameras, recovery and claims");
}
