#include "faction_fixture.h"
#include "snapshot.h"
static Cell site;
static Game *setup(unsigned kind) {
    Game *g = fixture(713);
    site = ordinary(g, (Cell){8, 0, 8});
    load_area(g, site);
    Room *r = world_find(&g->world, site);
    CHECK(r && r->ready);
    r->enemy_count = kind == SEQUENCE_DISCOVERY ? 0 : 1;
    r->habitat.pace = PACE_QUIET;
    unsigned robot = 0;
    while (robot < SC_ENEMY_TYPES && sc_enemy_bodies[robot].form != ROBOT)
        robot++;
    CHECK(robot < SC_ENEMY_TYPES);
    r->enemies[0] = (Enemy){.alive = true,
                            .id = 0,
                            .kind = kind == SEQUENCE_MACHINE ? robot : 1,
                            .health = 200,
                            .normal = {0, 1, 0},
                            .pos = position_from(vadd(world_center(site), v3(0, .02, 12)))};
    if (kind == SEQUENCE_MACHINE)
        r->journey.family = r->journey.condition = 1;
    g->players[0].pos = position_from(vadd(world_center(site), v3(-100, .02, -80)));
    g->players[0].yaw = SC_PI;
    return g;
}
static void advance(Game *g, float seconds) {
    for (unsigned i = 0; i < (unsigned)(seconds * 4); i++)
        rhythm_step(g, .25);
}
static void approach(Game *g) {
    g->players[0].pos =
        position_from(vadd(rhythm_trace(site, SEQUENCE_DISCOVERY), v3(0, -.06, -2)));
    g->players[0].yaw = 0;
    rhythm_step(g, .02);
    CHECK(g->rhythm.sequences[0].beat == BEAT_APPROACH);
    advance(g, 5.25);
    CHECK(g->rhythm.sequences[0].beat >= BEAT_PRESSURE);
}
static void threats(void) {
    for (unsigned kind = SEQUENCE_HUNTER; kind <= SEQUENCE_MACHINE; kind += 2) {
        Game *g = setup(kind);
        CHECK(!rhythm_start(g, 0, 0, site, 0, 0));
        CHECK(!rhythm_start(g, (SequenceKind)kind, -1, site, 0, 0));
        CHECK(!rhythm_start(g, (SequenceKind)kind, 0, site, 99, 0));
        CHECK(rhythm_start(g, (SequenceKind)kind, 0, site, 0, 0));
        CHECK(rhythm_reserved(&g->rhythm) == 1 && g->factions.director.budget == 2);
        CHECK(!rhythm_start(g, (SequenceKind)kind, 0, site, 0, 0));
        CHECK(rhythm_valid(&g->rhythm, 2));
        CHECK(!hidden_director_propose(g, EDIT_NEST, 0, site, 0));
        advance(g, 15);
        CHECK(g->rhythm.sequences[0].beat == BEAT_TRACE && !world_rhythm_state(&g->world, site));
        g->players[0].sheltered = true;
        Rhythm paused = g->rhythm;
        rhythm_step(g, 1000);
        CHECK(!memcmp(&paused, &g->rhythm, sizeof(paused)));
        CHECK(rhythm_intensity(g, 0) == 0);
        game_join(g);
        g->players[1].sheltered = false;
        g->players[1].pos = position_from(vadd(world_center(site), v3(0, .02, 2)));
        g->players[1].yaw = 0;
        rhythm_step(g, .1);
        CHECK(g->rhythm.sequences[0].witnessed);
        g->players[0].sheltered = false;
        approach(g);
        if (kind == SEQUENCE_MACHINE) {
            CHECK(g->rhythm.sequences[0].reserved);
            CHECK(world_set_systems(&g->world, site, SYSTEM_POWER));
        }
        rhythm_step(g, .25);
        CHECK(g->rhythm.sequences[0].beat == BEAT_FIGHT && !rhythm_reserved(&g->rhythm));
        CHECK(rhythm_intensity(g, 0) > .5);
        Room *r = world_find(&g->world, site);
        CHECK(r->enemies[0].health == 200);
        Game *copy = fixture(713);
        unsigned char *data;
        size_t size;
        CHECK(game_snapshot_encode(g, &data, &size) && game_snapshot_decode(copy, data, size));
        free(data);
        CHECK(!memcmp(&g->rhythm, &copy->rhythm, sizeof(Rhythm)));
        CHECK(game_save(g, "rhythm.sav") && game_load(copy, "rhythm.sav"));
        CHECK(!memcmp(&g->rhythm, &copy->rhythm, sizeof(Rhythm)));
        game_damage_enemy(g, r, 0, 0, 10000, 0);
        rhythm_step(g, .02);
        CHECK(g->rhythm.completed == 1 && g->rhythm.sequences[0].beat == BEAT_QUIET);
        CHECK(rhythm_blocks(g, (Cell){20, 0, 20}, true) &&
              !rhythm_blocks(g, (Cell){20, 0, 20}, false));
        CHECK(rhythm_intensity(g, 0) < 0);
        unsigned killed = g->players[0].kills;
        CHECK(game_save(g, "rhythm.sav") && game_load(copy, "rhythm.sav"));
        advance(copy, 45);
        CHECK(!copy->rhythm.sequences[0].beat && copy->players[0].kills == killed);
        CHECK(world_rhythm_state(&copy->world, site) & (16u << (kind - 1)));
        finish(copy);
        finish(g);
    }
    remove("rhythm.sav");
}
static void cancellation_and_early_death(void) {
    Game *g = setup(SEQUENCE_HUNTER);
    CHECK(rhythm_start(g, SEQUENCE_HUNTER, 0, site, 0, 0));
    Room *room = world_find(&g->world, site);
    game_damage_enemy(g, room, 0, 0, 10000, 0);
    rhythm_step(g, .02);
    CHECK(g->rhythm.sequences[0].beat == BEAT_QUIET && g->rhythm.completed == 1);
    CHECK(!room->enemies[0].alive && g->factions.director.budget == 2);
    advance(g, 46);
    CHECK(!g->rhythm.sequences[0].beat && !room->enemies[0].alive);
    finish(g);
    g = setup(SEQUENCE_HUNTER);
    CHECK(rhythm_start(g, SEQUENCE_HUNTER, 0, site, 0, 0));
    g->players[0].pos = position_from(vadd(world_center(site), v3(0, .02, 2)));
    g->players[0].yaw = 0;
    rhythm_step(g, .1);
    g->factions.director.recovery[0] = 20;
    rhythm_step(g, .02);
    CHECK(g->rhythm.cancelled == 1 && g->factions.director.budget == 3 &&
          !rhythm_reserved(&g->rhythm));
    CHECK(world_rhythm_state(&g->world, site) & 1);
    rhythm_step(g, .02);
    CHECK(g->factions.director.budget == 3);
    finish(g);
    g = setup(SEQUENCE_HUNTER);
    CHECK(rhythm_start(g, SEQUENCE_HUNTER, 0, site, 0, 0));
    g->players[0].pos = position_from(v3(0, .02, 12));
    rhythm_step(g, 1000);
    CHECK(g->rhythm.sequences[0].away == .25);
    advance(g, 31);
    CHECK(g->rhythm.cancelled == 1 && g->factions.director.budget == 3);
    CHECK(!world_rhythm_state(&g->world, site));
    finish(g);
}
static void discovery_and_siege(void) {
    Game *g = setup(SEQUENCE_DISCOVERY);
    CHECK(rhythm_start(g, SEQUENCE_DISCOVERY, 0, site, 0, 0));
    approach(g);
    rhythm_step(g, .25);
    CHECK(!rhythm_reserved(&g->rhythm));
    g->players[0].health = 20;
    g->players[0].energy = 20;
    unsigned salvage = g->players[0].credits;
    factions_environment(g);
    rhythm_step(g, .1);
    CHECK(g->rhythm.completed == 1 && g->players[0].health == 45 &&
          g->players[0].credits == salvage + 2);
    factions_environment(g);
    CHECK(g->players[0].credits == salvage + 2);
    finish(g);
    g = setup(SEQUENCE_SIEGE);
    CHECK(factions_launch(g, 1, ORDER_RAID, site, world_neighbor(site, EAST)));
    FactionForce *f = factions_force(g, g->factions.next_id - 1);
    CHECK(f);
    CHECK(rhythm_start(g, SEQUENCE_SIEGE, 0, site, 0, f->id));
    unsigned kills = g->players[0].kills;
    f->order = ORDER_RETREAT;
    rhythm_step(g, .25);
    CHECK(g->rhythm.completed == 1 && g->players[0].kills == kills);
    finish(g);
}
static void reservations_and_validation(void) {
    Game *g = setup(SEQUENCE_HUNTER);
    Game *copy = fixture(713);
    game_join(g);
    g->players[1].pos = position_from(vadd(world_center(site), v3(0, .02, 2)));
    CHECK(!rhythm_start(g, SEQUENCE_HUNTER, 0, site, 0, 0));
    g->players[1].sheltered = true;
    CHECK(rhythm_start(g, SEQUENCE_HUNTER, 0, site, 0, 0));
    g->players[1].sheltered = false;
    HiddenSequence *s = &g->rhythm.sequences[0];
    g->rhythm.sequences[1] = *s;
    g->factions.director.budget = 1;
    CHECK(!rhythm_valid(&g->rhythm, 1));
    g->rhythm.sequences[1].identity = ++g->rhythm.next_id;
    g->rhythm.sequences[1].site.x += 4;
    g->factions.director.budget = 1;
    CHECK(rhythm_valid(&g->rhythm, 1));
    CHECK(!rhythm_start(g, SEQUENCE_HUNTER, 0, (Cell){20, 0, 20}, 0, 0));
    g->rhythm.sequences[1] = (HiddenSequence){0};
    g->factions.director.budget = 2;
    Rhythm good = g->rhythm;
    s->clock = NAN;
    CHECK(!rhythm_valid(&g->rhythm, 2));
    g->rhythm = good;
    s->beat = 0;
    CHECK(!rhythm_valid(&g->rhythm, 2));
    g->rhythm = good;
    g->rhythm.quiet = 50;
    CHECK(!rhythm_valid(&g->rhythm, 2));
    g->rhythm = good;
    s->identity = 0;
    CHECK(!rhythm_valid(&g->rhythm, 2));
    unsigned char *data;
    size_t size;
    CHECK(!game_snapshot_encode(g, &data, &size));
    CHECK(game_save(g, "rhythm-bad.sav"));
    CHECK(!game_load(copy, "rhythm-bad.sav"));
    g->rhythm = good;
    CHECK(game_save(g, "rhythm.sav") && game_load(copy, "rhythm.sav"));
    CHECK(copy->factions.director.budget == 2 && rhythm_reserved(&copy->rhythm) == 1);
    g->world.generator = 15;
    CHECK(!rhythm_blocks(g, site, true) && rhythm_intensity(g, 0) == 0);
    CHECK(!rhythm_start(g, SEQUENCE_HUNTER, 0, site, 0, 0));
    rhythm_step(g, 1);
    rhythm_select(g, 0);
    CHECK(!world_set_rhythm_state(&g->world, site, 1));
    g->world.generator = 16;
    CHECK(!world_set_rhythm_state(&g->world, site, 256));
    CHECK(!world_set_rhythm_state(&g->world, (Cell){0, 0, -1}, 1));
    game_new_ship(g, 713);
    CHECK(!g->rhythm.next_id && !g->rhythm.completed);
    remove("rhythm.sav");
    remove("rhythm-bad.sav");
    finish(copy);
    finish(g);
}
static void eviction_and_selection(void) {
    Game *g = setup(SEQUENCE_HUNTER);
    Cell origin = site;
    FactionRegion *record = factions_record(g, site);
    CHECK(record);
    unsigned local =
        (unsigned)(site.z - record->region.z * 4) * 4 + (unsigned)(site.x - record->region.x * 4);
    record->seen = 1u << local;
    CHECK(!rhythm_start(g, SEQUENCE_HUNTER, 0, site, 0, 0));
    record->seen = 0;
    CHECK(!rhythm_start(g, SEQUENCE_DISCOVERY, 0, site, 0, 0));
    CHECK(!rhythm_start(g, SEQUENCE_MACHINE, 0, site, 0, 0));
    g->players[0].yaw = .8;
    rhythm_select(g, 0);
    if (!g->rhythm.sequences[0].beat)
        CHECK(rhythm_start(g, SEQUENCE_HUNTER, 0, site, 0, 0));
    HiddenSequence *s = &g->rhythm.sequences[0];
    origin = s->site;
    load_area(g, (Cell){40, 0, 40});
    CHECK(!world_find(&g->world, origin));
    rhythm_step(g, .1); /* An unloaded target is unknown, never assumed dead. */
    Room outcome = {.cell = origin, .record = SIZE_MAX, .enemy_count = 1};
    world_kill(&g->world, &outcome, s->member);
    rhythm_step(g, .1);
    CHECK(s->beat == BEAT_QUIET);
    finish(g);
}
int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--fixtures")) {
        Game *g = setup(SEQUENCE_HUNTER);
        CHECK(world_set_rhythm_state(&g->world, site, 1));
        g->players[0].pos =
            position_from(vadd(rhythm_trace(site, SEQUENCE_HUNTER), v3(0, -.06, -4)));
        g->players[0].yaw = 0;
        g->players[0].pitch = -.25;
        CHECK(game_save(g, "coherent-rhythm.sav"));
        finish(g);
        return 0;
    }
    threats();
    cancellation_and_early_death();
    discovery_and_siege();
    reservations_and_validation();
    eviction_and_selection();
    puts("PASS bounded hidden sequences, actual observations, early victories, refundable "
         "reservations and exact persistence");
}
