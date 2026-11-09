#include "game.h"
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
    /* These are the retained generator-13 courts and dispatch contracts. */
    world_destroy(&g->world);
    world_init(&g->world, seed);
    g->world.generator = 13;
    campaign_init(g);
    matrix_init(&g->matrix);
    politics_init(g);
    g->world.open_layer = 0;
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static Room *arrive(Game *g, Cell c) {
    Vec3 at = vadd(world_center(c), v3(0, .02, 2));
    g->players[0].pos = position_from(at);
    g->players[0].sheltered = false;
    g->players[0].riding = 0;
    g->players[0].lift_active = false;
    g->matrix.divers[0].active = false;
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed);
    Room *r = world_find(&g->world, c);
    CHECK(r && r->ready);
    return r;
}
static Room *site(Game *g, unsigned n, CourtRole role) {
    Room *r = arrive(g, politics_site(g->world.seed, n, role));
    CHECK(r->habitat.court == role && r->habitat.country == n + 1);
    return r;
}
static void win(Game *g, Room *r) {
    for (int i = 0; i < 4; i++)
        game_damage_enemy(g, r, i, 0, 100000, 0);
    game_encounters_step(g, .02);
    CHECK(world_local_state(&g->world, r) & FRONTIER_CLEARED);
    game_interact(g, 0);
}
static void generation(void) {
    unsigned governments = 0, owners = 0, relations = 0;
    for (unsigned seed = 1; seed <= 48; seed++) {
        Game *g = fresh(seed);
        Politics before = g->politics;
        for (unsigned n = 0; n < 3; n++) {
            governments |= 1u << politics_government(seed, n);
            CHECK(strlen(politics_name(seed, n)) > 4 &&
                  strlen(politics_government_name(politics_government(seed, n))) > 4);
            CHECK(politics_color(seed, n).r > 0);
            for (unsigned role = 1; role <= 3; role++) {
                Cell c = politics_site(seed, n, (CourtRole)role);
                RoomSpec r = world_describe_version(seed, 13, c);
                CHECK(r.habitat.court == role && r.habitat.country == n + 1 && r.height >= 8 &&
                      !r.habitat.volume);
                CHECK(cell_equal(world_region(c), politics_region(seed, n, role)));
                CHECK(role == 1 ? r.habitat.pace == PACE_QUIET : r.habitat.pace == PACE_FRONTIER);
                CHECK(politics_country(seed, c) == (role == 2 ? (n + 1) % 3 + 1 : n + 1));
                RegionPlan p;
                world_plan_version(seed, 12, world_region(c), &p);
                for (int i = 0; i < p.count; i++)
                    CHECK(!p.rooms[i].habitat.court && !p.rooms[i].habitat.country);
            }
            for (unsigned k = 0; k < 3; k++) {
                Relation r = politics_relation(g, n, k);
                relations |= 1u << r;
                CHECK(r == politics_relation(g, k, n) && strlen(politics_relation_name(r)) > 2);
            }
        }
        for (int x = -14; x <= 14; x++)
            for (int y = -16; y < 16; y++)
                for (int z = 0; z < 16; z += 4)
                    owners |= 1u << politics_country(seed, (Cell){x, y, z});
        CHECK(!politics_country(seed, (Cell){0}) && !politics_country(seed, (Cell){0, 0, -1}));
        MapView map;
        CHECK(game_map_build(g, 0, (Cell){0}, 3, &map));
        CHECK(!memcmp(&before, &g->politics, sizeof(before)));
        destroy(g);
    }
    CHECK(governments == 7 && owners == 15 && relations == 15);
    puts("PASS stable countries, nine reachable macro sites, governments, symmetric relations and "
         "pure map queries");
}
static void disputes(void) {
    for (unsigned nation = 0; nation < 3; nation++)
        for (unsigned choice = 1; choice <= 3; choice++) {
            Game *g = fresh(713);
            CHECK(game_load(g, SC_FIXTURE_DIR "/courts-expedition.sav"));
            politics_init(g);
            game_join(g);
            g->world.open_layer = 0;
            CHECK(!politics_choose(g, -1, 1) && !politics_choose(g, 0, 4) &&
                  !politics_interact(g, -1));
            CHECK(!politics_interact(g, 0));
            politics_journal(g, -1);
            politics_read(g, -1);
            politics_track(g, -1);
            site(g, nation, COURT_FINALE);
            game_interact(g, 0);
            CHECK(g->politics.countries[nation].stage == 0);
            Room *r = site(g, nation, COURT_AUDIENCE);
            CHECK(r->enemy_count == 0);
            g->players[1].pos = g->players[0].pos;
            game_interact(g, 0);
            CHECK(g->politics.audience[0] == nation + 1);
            g->players[0].journal = false;
            CHECK(!politics_choose(g, 0, choice));
            game_interact(g, 0);
            politics_read(g, 0);
            CHECK(!politics_choose(g, 0, choice));
            game_interact(g, 0);
            for (int l = 0; l <= 10; l++) {
                char text[256];
                politics_line(g, 0, l, text, sizeof(text));
                CHECK(strlen(text) > 5);
            }
            CHECK(politics_choose(g, 0, choice));
            CHECK(!politics_choose(g, 0, choice));
            CHECK(g->politics.countries[nation].stage == 1 && !g->players[0].journal);
            Cell goal;
            CHECK(game_objective(g, &goal) &&
                  cell_equal(goal, politics_site(713, nation, COURT_CONDUIT)));
            CHECK(strstr(game_mission(g), "CONDUIT"));
            politics_track(g, 0);
            CHECK(!politics_objective(g, &goal));
            politics_track(g, 0);
            r = site(g, nation, COURT_CONDUIT);
            g->players[1].pos = g->players[0].pos;
            game_interact(g, 0);
            CHECK(r->frontier_active);
            win(g, r);
            CHECK(g->politics.countries[nation].stage == 2);
            CHECK(strstr(game_mission(g), "GUARD"));
            r = site(g, nation, COURT_FINALE);
            g->players[1].pos = g->players[0].pos;
            game_interact(g, 0);
            CHECK(r->frontier_active &&
                  r->enemies[0].health >= 300 + sc_enemies[r->enemies[0].kind].health);
            g->players[1].points = 1000;
            g->players[1].credits = 1000000000;
            win(g, r);
            CHECK(g->players[1].points == 1000 && g->players[1].credits == 1000000000);
            CHECK(g->politics.countries[nation].stage == 3 &&
                  g->politics.countries[nation].claimed == 3);
            unsigned credits = g->players[0].credits;
            game_interact(g, 0);
            CHECK(g->players[0].credits == credits);
            game_interact(g, 1); /* Its equipment claim is independent of the shared settlement. */
            Relation expected = choice == 1 ? REL_PEACE : choice == 2 ? REL_TRIBUTE : REL_WAR;
            CHECK(politics_relation(g, nation, (nation + 1) % 3) == expected);
            CHECK(politics_reputation(g, nation + 1, 0) == 3);
            if (choice == 3) {
                CHECK(politics_reputation(g, (nation + 1) % 3 + 1, 1) == -3);
                CHECK(politics_owner(g, politics_site(713, nation, COURT_CONDUIT)) == nation + 1);
            }
            if (choice == 1)
                CHECK((world_systems(&g->world, politics_site(713, nation, COURT_CONDUIT)) & 3) ==
                      3);
            CHECK(game_save(g, "politics-roundtrip.sav"));
            if (!game_load(g, "politics-roundtrip.sav")) {
                fprintf(stderr,
                        "save validity nation=%u choice=%u story=%u campaign=%u politics=%u "
                        "director=%u matrix=%u\n",
                        nation, choice,
                        story_valid(&g->story, g->world.seed, g->world.generator, g->mission),
                        campaign_valid(&g->campaign, g->players, &g->world),
                        politics_valid(&g->politics, g->world.generator),
                        director_valid(&g->director, g->world.generator),
                        matrix_valid(&g->matrix, g->world.seed, g->world.generator, g->players,
                                     &g->shelter));
                CHECK(false);
            }
            CHECK(g->politics.countries[nation].stage == 3 &&
                  g->politics.countries[nation].claimed == 3);
            CHECK(politics_valid(&g->politics, 13) && !politics_valid(&g->politics, 12));
            site(g, nation, COURT_AUDIENCE);
            g->players[0].armor = 0;
            g->players[0].energy = 0;
            g->players[0].hurt = 0;
            politics_step(g, .05);
            CHECK(g->players[0].armor > 0);
            if (choice == 2)
                CHECK(g->players[0].energy > 0);
            politics_journal(g, 0);
            g->politics.reading[0] = nation;
            for (int l = 0; l <= 10; l++) {
                char text[256];
                politics_line(g, 0, l, text, sizeof(text));
                CHECK(strlen(text) > 5);
            }
            CHECK(strstr(game_mission(g), "COURT"));
            game_new_ship(g, 42);
            CHECK(!g->politics.countries[nation].stage && !g->director.sequence);
            destroy(g);
        }
    remove("politics-roundtrip.sav");
    puts("PASS all nine settlements, co-op eligible rewards, ordinary damage, systems, borders, "
         "rulers and saved consequences");
}
static Room *dispatch_room(Game *g, unsigned kind) {
    for (int y = -2; y <= 2; y++)
        for (int z = 0; z < 6; z++)
            for (int x = -4; x <= 4; x++) {
                RegionPlan p;
                world_plan_version(g->world.seed, 13, (Cell){x, y, z}, &p);
                for (int i = 0; i < p.count; i++) {
                    RoomSpec *a = &p.rooms[i];
                    if (a->type == BAY || a->habitat.court || a->height < 8 || a->habitat.volume ||
                        campaign_guardian(a->cell) >= 0)
                        continue;
                    unsigned country = politics_country(g->world.seed, a->cell);
                    if (kind == EVENT_COURIER && !country)
                        continue;
                    if (kind == EVENT_REVEAL && !a->habitat.reveal)
                        continue;
                    if (kind == EVENT_CONFLICT &&
                        (a->habitat.pace != PACE_FRONTIER || (country != 1 && country != 2)))
                        continue;
                    Room *r = arrive(g, a->cell);
                    if (kind != EVENT_CONFLICT)
                        for (int j = 0; j < r->enemy_count; j++)
                            r->enemies[j].alive = false;
                    return r;
                }
            }
    CHECK(false);
    return NULL;
}
static void director(void) {
    Game *g = fresh(713);
    director_step(g, .05);
    CHECK(g->director.cooldown == 30000); /* outside */
    for (unsigned kind = 1; kind < EVENTS; kind++) {
        Room *r = dispatch_room(g, kind);
        director_init(&g->director, 13);
        g->director.cooldown = 0;
        g->director.quiet = 30000;
        for (int i = 0; i < 3; i++)
            g->politics.countries[i] = (CountryState){.met = 1, .stage = 3, .pledge = 1};
        if (kind == EVENT_COURIER)
            g->politics.countries[politics_owner(g, r->cell) - 1].stage = 0;
        g->players[0].health = kind == EVENT_SUPPLIES ? 20 : 100;
        g->players[0].energy = 100;
        if (kind == EVENT_REVEAL)
            g->players[0].credits = 1000000000;
        if (kind == EVENT_CONFLICT)
            for (int i = 0; i < 3; i++)
                g->politics.countries[i].stage = 1;
        if (kind == EVENT_CONFLICT)
            g->director.history[0] = EVENT_REVEAL, g->director.sequence = 1;
        director_step(g, .05);
        CHECK(g->director.kind == kind);
        CHECK(director_valid(&g->director, 13) && !director_valid(&g->director, 12));
        CHECK(strlen(director_name(kind)) > 5 &&
              strlen(director_reason(
                  g->director.reasons[(g->director.sequence - 1) % SC_DIRECTOR_LOG])) > 5);
        Director d = g->director;
        g->players[0].sheltered = true;
        director_step(g, .05);
        CHECK(!memcmp(&d, &g->director, sizeof(d)));
        g->players[0].sheltered = false;
        g->matrix.divers[0].active = true;
        director_step(g, .05);
        CHECK(!memcmp(&d, &g->director, sizeof(d)));
        g->matrix.divers[0].active = false;
        CHECK(!director_interact(g, -1));
        g->players[0].riding = 1;
        CHECK(!director_interact(g, 0));
        g->players[0].riding = 0;
        g->world.pending = 1;
        CHECK(director_interact(g, 0) && g->director.kind == kind);
        g->world.pending = 0;
        game_join(g);
        g->world.open_layer = 0;
        CHECK(director_interact(g, 0) == (kind != EVENT_CONFLICT));
        CHECK(!director_interact(g, 0));
        CHECK(g->director.claimed == 1);
        CHECK(director_interact(g, 1) == (kind != EVENT_CONFLICT));
        CHECK(!g->director.kind && !g->director.remaining && !g->director.claimed);
        if (kind == EVENT_SUPPLIES)
            CHECK(g->players[0].health == 55);
        if (kind == EVENT_REVEAL)
            CHECK(g->players[0].credits == 1000000000);
        d = g->director;
        g->director.cooldown = 0;
        director_step(g, .05);
        CHECK(!g->director.kind); /* Same room/recent kind cannot farm it. */
        g->director = d;
        g->player_count = 1;
    }
    Room *r = dispatch_room(g, EVENT_SUPPLIES);
    g->players[0].health = 20;
    director_init(&g->director, 13);
    g->director.cooldown = 0;
    r->frontier_active = true;
    director_step(g, .05);
    CHECK(!g->director.kind);
    r->frontier_active = false;
    r->enemy_count = 1;
    r->enemies[0] = (Enemy){.alive = true, .country = 1, .pos = g->players[0].pos};
    g->politics.countries[0].offended = 1;
    director_step(g, .05);
    CHECK(!g->director.kind);
    r->enemies[0].alive = false;
    director_step(g, .05);
    CHECK(g->director.kind == EVENT_SUPPLIES);
    g->director.remaining = 1;
    director_step(g, .05);
    CHECK(!g->director.kind);
    g->director.recharge = 29999;
    g->director.budget = 0;
    director_step(g, .05);
    CHECK(g->director.budget == 1);
    Director saved = g->director;
    /* State corruption is rejected independently of a valid file checksum. */
    for (int i = 0; i < 12; i++) {
        Director bad = saved;
        switch (i) {
        case 0:
            bad.version = 2;
            break;
        case 1:
            bad.budget = 4;
            break;
        case 2:
            bad.cooldown = 180001;
            break;
        case 3:
            bad.recharge = 30000;
            break;
        case 4:
            bad.quiet = 120001;
            break;
        case 5:
            bad.kind = EVENTS;
            break;
        case 6:
            bad.remaining = 1;
            break;
        case 7:
            bad.claimed = 4;
            break;
        case 8:
            bad.site.z = -1;
            break;
        case 9:
            bad.seen[0] = 2;
            break;
        case 10:
            bad.history[0] = EVENTS;
            break;
        default:
            bad.reasons[0] = DIRECTOR_REASONS;
            break;
        }
        CHECK(!director_valid(&bad, 13));
    }
    director_init(&g->director, 12);
    CHECK(director_valid(&g->director, 12));
    director_step(g, .05);
    CHECK(!director_interact(g, 0));
    destroy(g);
    puts("PASS physical director dispatches, both players, recovery budget, repeat limits, expiry "
         "and frozen clocks");
}
static void patrols(void) {
    Game *g = fresh(99);
    Room *r = NULL;
    Enemy *e = NULL;
    for (int x = 1; x < 12 && !e; x++) {
        r = arrive(g, (Cell){x, 2, 8});
        for (int i = 0; i < r->enemy_count; i++)
            if (r->enemies[i].country) {
                e = &r->enemies[i];
                break;
            }
    }
    CHECK(e);
    unsigned country = e->country;
    for (int i = 0; i < r->enemy_count; i++)
        if (&r->enemies[i] != e)
            r->enemies[i].alive = false;
    g->players[0].pos = e->pos;
    position_move(&g->players[0].pos, v3(0, 0, 3));
    CHECK(politics_enemy_step(g, r, e, .05));
    CHECK(politics_reputation(g, country, 0) >= 0);
    Enemy *victim = &r->enemies[e == r->enemies ? 1 : 0];
    *victim = (Enemy){.alive = true, .health = 10, .pos = e->pos};
    position_move(&victim->pos, v3(0, 0, 1));
    e->cooldown = 0;
    CHECK(politics_enemy_step(g, r, e, .05));
    CHECK(!victim->alive);
    CHECK(g->deaths[0].active);
    CHECK(g->players[0].kills == 0);
    politics_attack(g, e, 0);
    politics_attack(g, e, 0);
    CHECK(politics_reputation(g, country, 0) == -3);
    CHECK(!politics_enemy_step(g, r, e, .05));
    CHECK(politics_reputation(g, country, 1) >= 0);
    e->allegiance = 2;
    CHECK(!politics_enemy_step(g, r, e, .05));
    e->allegiance = 0;
    g->players[0].sheltered = true;
    CHECK(politics_enemy_step(g, r, e, .05));
    e->country = 0;
    CHECK(!politics_enemy_step(g, r, e, .05));
    CHECK(!politics_reputation(g, 0, 0) && !politics_reputation(g, 5, 0) &&
          !politics_reputation(g, 1, -1));
    destroy(g);
    puts("PASS neutral patrols, local defense, individual standing and matrix control precedence");
}
static void saves(void) {
    Game *g = fresh(713);
    CHECK(game_load(g, SC_FIXTURE_DIR "/frontier-expedition.sav"));
    CHECK(g->world.generator == 12 && !g->politics.version && !g->director.version);
    CHECK(!politics_owner(g, (Cell){2, 1, 8}) && !politics_interact(g, 0));
    politics_step(g, .05);
    politics_track(g, 0);
    char text[256];
    politics_line(g, 0, 9, text, sizeof(text));
    CHECK(strstr(text, "LEGACY"));
    CHECK(game_load(g, SC_FIXTURE_DIR "/courts-expedition.sav"));
    politics_init(g);
    Room *dispatch = dispatch_room(g, EVENT_SUPPLIES);
    for (int i = 0; i < dispatch->enemy_count; i++)
        world_kill(&g->world, dispatch, i);
    g->players[0].health = 20;
    g->director.cooldown = 0;
    director_step(g, .05);
    CHECK(g->director.kind == EVENT_SUPPLIES);
    Director pending = g->director;
    CHECK(game_save(g, "politics-pending.sav"));
    CHECK(game_load(g, "politics-pending.sav"));
    CHECK(!memcmp(&pending, &g->director, sizeof(pending)));
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed);
    CHECK(director_interact(g, 0));
    CHECK(!g->director.kind && g->players[0].health == 55);
    remove("politics-pending.sav");
    game_new_ship(g, 713);
    CHECK(g->factions.version && !g->politics.version);
    world_destroy(&g->world);
    world_init(&g->world, 713);
    g->world.generator = 13;
    campaign_init(g);
    matrix_init(&g->matrix);
    politics_init(g);
    Politics forged = g->politics;
    forged.countries[0] = (CountryState){.met = 1, .pledge = 1, .stage = 2};
    CHECK(politics_valid(&forged, 13) && !politics_world_valid(&forged, &g->director, &g->world));
    Director bad_site = g->director;
    bad_site.kind = EVENT_SUPPLIES;
    bad_site.remaining = 1000;
    CHECK(!politics_world_valid(&g->politics, &bad_site, &g->world));
    CHECK(game_save(g, "politics-good.sav"));
    FILE *f = fopen("politics-good.sav", "rb");
    CHECK(f);
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    unsigned char *original = malloc((size_t)length), *wire = malloc((size_t)length);
    CHECK(original && wire);
    CHECK(fread(original, 1, (size_t)length, f) == (size_t)length);
    fclose(f);
    size_t start = (size_t)length - 4 - SC_POLITICS_SAVE_BYTES;
    CHECK(original[start] == 1);
    const unsigned words[] = {0,  1,  2,  3,  4,  5,  6,  17, 18, 19,
                              20, 21, 22, 23, 24, 25, 28, 32, 37, 38};
    for (unsigned n = 0; n < sizeof(words) / sizeof(words[0]); n++) {
        memcpy(wire, original, (size_t)length);
        memset(wire + start + words[n] * 4, 255, 4);
        uint32_t hash = 2166136261u;
        for (long k = 0; k < length - 4; k++)
            hash = (hash ^ wire[k]) * 16777619u;
        for (int k = 0; k < 4; k++)
            wire[length - 4 + k] = (unsigned char)(hash >> (8 * k));
        f = fopen("politics-bad.sav", "wb");
        CHECK(f && fwrite(wire, 1, (size_t)length, f) == (size_t)length);
        fclose(f);
        RoomRecord *records = g->world.records;
        Politics before = g->politics;
        Director d = g->director;
        CHECK(!game_load(g, "politics-bad.sav"));
        CHECK(records == g->world.records && !memcmp(&before, &g->politics, sizeof(before)) &&
              !memcmp(&d, &g->director, sizeof(d)));
    }
    free(wire);
    free(original);
    CHECK(game_load(g, "politics-good.sav"));
    Politics p = g->politics;
    p.countries[0].stage = 1;
    CHECK(!politics_valid(&p, 13));
    p = g->politics;
    p.countries[0].claimed = 1;
    CHECK(!politics_valid(&p, 13));
    p = (Politics){0};
    CHECK(politics_valid(&p, 12));
    p.countries[0].offended = 1;
    CHECK(!politics_valid(&p, 12));
    remove("politics-good.sav");
    remove("politics-bad.sav");
    destroy(g);
    puts("PASS format-13 migration, format-14 corruption rollback and political invariants");
}
static void peaceful_objectives(void) {
    Game *g = fresh(713);
    Room *r = arrive(g, (Cell){0});
    g->story.sites[STORY_REFUGE] = r->cell;
    r->enemy_count = 1;
    Enemy *e = &r->enemies[0];
    *e = (Enemy){.alive = true,
                 .country = 1,
                 .pos = position_from(vadd(game_terminal(r->cell), v3(1, 0, 0)))};
    CHECK(story_clear(g, STORY_REFUGE));
    politics_attack(g, e, 0);
    CHECK(!story_clear(g, STORY_REFUGE));
    e->allegiance = 2;
    CHECK(story_clear(g, STORY_REFUGE));
    e->allegiance = 0;
    game_join(g);
    g->players[0].sheltered = true;
    CHECK(story_clear(g, STORY_REFUGE));
    politics_attack(g, e, 1);
    CHECK(!story_clear(g, STORY_REFUGE));
    g->matrix.divers[1].active = true;
    CHECK(story_clear(g, STORY_REFUGE));
    destroy(g);
    puts("PASS neutral patrols leave story objectives accessible; hostile and hacked states agree");
}
int main(void) {
    generation();
    disputes();
    director();
    patrols();
    peaceful_objectives();
    saves();
    return 0;
}
