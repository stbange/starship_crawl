#include "game.h"
#include "legacy_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c);                                \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
#define BIT(n) (1u << (n))
static unsigned narrative_version = 2;
static Game *create(unsigned seed) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    legacy_game_init(g, seed);
    if (narrative_version == 1) {
        world_destroy(&g->world);
        world_init(&g->world, seed);
        g->world.generator = 4;
        g->story = (Story){.version = 1, .assigned = 1};
    }
    if (narrative_version == 2) {
        world_destroy(&g->world);
        world_init(&g->world, seed);
        g->world.generator = 5;
        g->story = (Story){.version = 2, .assigned = 1};
    }
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void arrive(Game *g, Cell cell) {
    Player *p = &g->players[0];
    p->pos = position_from(game_terminal(cell));
    p->velocity = v3(0, 0, 0);
    Vec3 at = position_absolute(p->pos);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending)
        world_stream(&g->world, SC_STREAM_BUDGET);
    CHECK(world_ready(&g->world, cell));
    for (int r = 0; r < g->world.count; r++)
        for (int e = 0; e < g->world.rooms[r].enemy_count; e++)
            g->world.rooms[r].enemies[e].alive = false;
}
static void valid(Game *g) {
    CHECK(story_valid(&g->story, g->world.seed, g->world.generator, g->mission));
}
static void roundtrip(Game *g) {
    Story before = g->story;
    unsigned credits = g->players[0].credits, victories = g->players[0].victories;
    CHECK(game_save(g, "sc-story-roundtrip.sav"));
    CHECK(game_load(g, "sc-story-roundtrip.sav"));
    CHECK(memcmp(&before, &g->story, sizeof(before)) == 0);
    CHECK(g->players[0].credits == credits && g->players[0].victories == victories);
    valid(g);
    remove("sc-story-roundtrip.sav");
}
static void use(Game *g, unsigned scene) {
    CHECK(game_story_available(g, scene));
    arrive(g, g->story.sites[scene]);
    game_story_step(g);
    CHECK(game_story_near(g, 0) == scene);
    game_interact(g, 0);
    valid(g);
}
static void inspect_text(Game *g) {
    char text[320];
    for (unsigned i = 0; i <= SC_STORY_SCENES; i++) {
        game_story_entry(g, i, text, sizeof(text));
        CHECK(*text && strlen(text) < sizeof(text));
    }
    for (int line = 0; line <= 8; line++) {
        game_story_line(g, line, text, sizeof(text));
        CHECK(*text);
        game_story_dossier(g, line, text, sizeof(text));
        CHECK(*text);
    }
    for (unsigned i = 0; i <= SC_STORY_SCENES; i++)
        CHECK(*game_story_title(g, i));
    CHECK(!*game_story_choice(g, 0) && !*game_story_choice(g, 4));
    for (unsigned choice = 1; choice <= 3; choice++)
        CHECK(*game_story_choice(g, choice));
}
static void test_arcs(void) {
    unsigned motivations = 0, clues = 0;
    for (unsigned seed = 1; seed <= 24; seed++) {
        for (unsigned choice = 1; choice <= 3; choice++) {
            Game *g = create(seed);
            StoryMotivation m = story_motivation_version(seed, g->story.version);
            motivations |= BIT(m);
            valid(g);
            inspect_text(g);
            CHECK(*story_ship(seed) && *story_commission(seed));
            game_story_read(g, -1);
            game_story_read(g, 2);
            game_story_read(g, 0); /* No observations yet. */
            CHECK(g->players[0].story_reading == 0);
            CHECK(!game_story_available(g, 99));
            CHECK(game_story_near(g, -1) == SC_STORY_SCENES);
            CHECK(game_story_near(g, 2) == SC_STORY_SCENES);
            CHECK(!game_story_choose(g, -1, 1));
            CHECK(!game_story_choose(g, 2, 1));
            CHECK(!game_story_choose(g, 0, 0));
            CHECK(!game_story_choose(g, 0, 4));
            unsigned money = g->players[0].credits;
            game_interact(g, 0);
            CHECK(g->players[0].credits == money && g->mission == 0);
            roundtrip(g);
            use(g, STORY_BRIEFING);
            CHECK(g->mission == 1 && g->players[0].credits == 75);
            unsigned initial = STORY_REFUGE + m % 3, alternate = STORY_REFUGE + (m + 1) % 3;
            CHECK(g->story.tracked == initial);
            Cell stable = g->story.sites[initial];
            game_interact(g, 0); /* Repeat the completed bay interaction. */
            CHECK(g->players[0].credits == 75);
            /* Room entry reveals an alternate lead, independently of terminal use. */
            arrive(g, (Cell){1, 0, 0});
            Story before = g->story;
            MapView map;
            for (int i = 0; i < 4; i++) {
                CHECK(game_map_build(g, 0, (Cell){100 + i, i, 200}, 4, &map));
                world_stream(&g->world, 2);
            }
            CHECK(memcmp(&before, &g->story, sizeof(before)) == 0);
            g->players[0].sheltered = true;
            game_story_step(g);
            CHECK(memcmp(&before, &g->story, sizeof(before)) == 0);
            CHECK(game_story_near(g, 0) == SC_STORY_SCENES);
            g->players[0].sheltered = false;
            game_story_step(g);
            CHECK(g->story.assigned & BIT(alternate));
            CHECK(cell_equal(stable, g->story.sites[initial]));
            clues |= g->story.seen & 62u;
            game_story_step(g); /* Observe any newly reserved site already within view. */
            Story discovered = g->story;
            game_story_step(g);
            CHECK(memcmp(&discovered, &g->story, sizeof(discovered)) == 0);
            game_story_track(g);
            CHECK(g->story.tracked == alternate);
            CHECK(game_map_build(g, 0, (Cell){0}, 2, &map));
            CHECK(cell_equal(map.objective, g->story.sites[alternate]));
            roundtrip(g);
            /* Both investigation orders work; first discovered target stays reserved. */
            unsigned first = choice & 1 ? alternate : initial;
            use(g, first);
            unsigned goal = game_story_target(g);
            unsigned op = game_story_operation(g);
            CHECK(game_story_available(g, goal) && game_story_available(g, op));
            Cell fixed_goal = g->story.sites[goal], fixed_op = g->story.sites[op];
            use(g, first == initial ? alternate : initial);
            CHECK(cell_equal(fixed_goal, g->story.sites[goal]) &&
                  cell_equal(fixed_op, g->story.sites[op]));
            roundtrip(g);
            use(g, goal);
            CHECK(g->players[0].decision_open);
            CHECK(!game_story_choose(g, 0, 3));
            CHECK(g->story.choice == 0 && strstr(g->message, "REQUIRED"));
            /* Even an open decision cannot be committed remotely. */
            g->players[0].pos = position_from(game_terminal((Cell){0}));
            CHECK(!game_story_choose(g, 0, 1));
            if (choice == 3) {
                g->players[0].decision_open = g->players[0].journal = false;
                use(g, op);
                roundtrip(g);
            }
            use(g, goal);
            game_join(g);
            g->players[1].pos = g->players[0].pos;
            game_interact(g, 1);
            unsigned p0 = g->players[0].credits, p1 = g->players[1].credits;
            CHECK(game_story_choose(g, 0, choice));
            CHECK(g->players[0].credits == p0 + 75 && g->players[1].credits == p1 + 75);
            CHECK(!game_story_choose(g, 1, choice));
            CHECK(g->story.choice == choice && g->story.tracked == STORY_EXTRACTION);
            CHECK(!game_story_available(g, op));
            Story decided = g->story;
            game_story_step(g);
            CHECK(memcmp(&decided, &g->story, sizeof(decided)) == 0);
            valid(g);
            roundtrip(g);
            use(g, STORY_EXTRACTION);
            CHECK(g->mission == 6 && g->players[0].victories == 1);
            CHECK(!game_objective(g, &stable));
            money = g->players[0].credits;
            game_story_track(g);
            game_interact(g, 0);
            CHECK(g->players[0].credits == money && g->players[0].victories == 1);
            inspect_text(g);
            for (int n = 0; n < 20; n++) {
                game_story_read(g, 0);
                CHECK(g->story.seen & BIT(g->players[0].story_reading));
            }
            roundtrip(g);
            game_new_ship(g, seed + 987);
            CHECK(g->mission == 0 && g->story.version == 3 && g->story.seen == 0);
            CHECK(g->players[0].credits == 0 && g->players[0].victories == 0);
            valid(g);
            destroy(g);
        }
    }
    CHECK(motivations == (narrative_version == 1 ? 7u : 15u) && clues);
}
static void test_discovery_and_contest(void) {
    Game *g = create(713);
    game_story_step(g); /* Space arrival cannot discover a room. */
    CHECK(g->story.seen == 0);
    use(g, STORY_BRIEFING);
    unsigned observed = 0;
    for (int y = -3; y <= 3 && observed != 62; y++)
        for (int x = -3; x <= 3 && observed != 62; x++)
            for (int z = 0; z <= 3 && observed != 62; z++) {
                arrive(g, (Cell){x, y, z});
                game_story_step(g);
                observed |= g->story.seen & 62;
                valid(g);
            }
    for (int y = -12; y <= 12 && observed != 62; y++)
        for (int x = 0; x < 12 && observed != 62; x++) {
            RoomSpec lab = world_describe_version(713, g->world.generator, (Cell){x, y, 8});
            if (lab.district == RESEARCH && lab.incident != FROST && lab.height >= 3) {
                arrive(g, lab.cell);
                game_story_step(g);
                observed |= g->story.seen & 62;
            }
        }
    CHECK(observed == 62);
    inspect_text(g);
    roundtrip(g);
    unsigned lead = STORY_REFUGE + story_motivation_version(713, g->story.version) % 3;
    arrive(g, g->story.sites[lead]);
    Room *r = world_find(&g->world, g->story.sites[lead]);
    CHECK(r);
    r->enemy_count = 1;
    r->enemies[0] = (Enemy){.alive = true, .pos = g->players[0].pos, .health = 50, .kind = 0};
    unsigned money = g->players[0].credits;
    game_interact(g, 0);
    CHECK(g->players[0].credits == money && strstr(g->message, "CONTESTED"));
    r->enemies[0].alive = false;
    game_interact(g, 0);
    unsigned goal = game_story_target(g);
    use(g, goal);
    r = world_find(&g->world, g->story.sites[goal]);
    r->enemy_count = 1;
    r->enemies[0] = (Enemy){.alive = true, .pos = g->players[0].pos, .health = 50, .kind = 0};
    CHECK(!game_story_choose(g, 0, 1));
    r->enemies[0].alive = false;
    CHECK(game_story_choose(g, 0, 1));
    valid(g);
    destroy(g);
}
static void rejected(Game *g, Story bad, unsigned mission) {
    CHECK(!story_valid(&bad, g->world.seed, g->world.generator, mission));
    Story before = g->story;
    unsigned stage = g->mission;
    g->story = bad;
    g->mission = mission;
    CHECK(game_save(g, "sc-story-bad.sav"));
    g->story = before;
    g->mission = stage;
    CHECK(!game_load(g, "sc-story-bad.sav"));
    CHECK(memcmp(&before, &g->story, sizeof(before)) == 0 && g->mission == stage);
    remove("sc-story-bad.sav");
}
static void test_validation_and_legacy(void) {
    Game *g = create(713);
    Story good = g->story, bad;
    bad = good;
    bad.version = 4;
    rejected(g, bad, 0);
    bad = good;
    bad.assigned |= BIT(30);
    rejected(g, bad, 0);
    bad = good;
    bad.assigned = 0;
    rejected(g, bad, 0);
    bad = good;
    bad.seen = BIT(STORY_DRIVE);
    rejected(g, bad, 0);
    bad = good;
    bad.done = 1;
    rejected(g, bad, 0);
    bad = good;
    bad.choice = 4;
    rejected(g, bad, 0);
    bad = good;
    bad.tracked = SC_STORY_SCENES;
    rejected(g, bad, 0);
    bad = good;
    bad.tracked = 9;
    rejected(g, bad, 0);
    bad = good;
    bad.sites[0].x = 1;
    rejected(g, bad, 0);
    bad = good;
    bad.sites[5].x = 1;
    rejected(g, bad, 0);
    bad = good;
    bad.assigned |= BIT(STORY_REFUGE);
    rejected(g, bad, 0);
    CHECK(!story_valid(&good, 713, 3, 0));
    use(g, STORY_BRIEFING);
    use(g, STORY_REFUGE + story_motivation_version(713, g->story.version) % 3);
    good = g->story;
    unsigned goal = game_story_target(g);
    unsigned op = game_story_operation(g);
    bad = good;
    bad.choice = 1;
    rejected(g, bad, g->mission);
    bad = good;
    bad.assigned &= ~BIT(op);
    rejected(g, bad, g->mission);
    bad = good;
    bad.assigned |= BIT(STORY_EXTRACTION);
    rejected(g, bad, g->mission);
    bad = good;
    bad.sites[goal] = (Cell){10000001, 0, 0};
    rejected(g, bad, g->mission);
    bad = good;
    bad.sites[goal] = (Cell){0, -4000001, 0};
    rejected(g, bad, g->mission);
    bad = good;
    bad.sites[goal] = (Cell){0, 0, -1};
    rejected(g, bad, g->mission);
    bad = good;
    bad.sites[goal] = (Cell){0, 0, 0};
    rejected(g, bad, g->mission);
    bad = good;
    bad.sites[goal] = good.sites[op];
    rejected(g, bad, g->mission);
    bad = good;
    bad.sites[goal] = world_describe(713, (Cell){8, 0, 8}).cell;
    rejected(g, bad, g->mission);
    arrive(g, (Cell){1, 0, 0});
    game_story_step(g);
    good = g->story;
    unsigned found = 1;
    while (!(good.seen & BIT(found)))
        found++;
    bad = good;
    bad.seen &= ~BIT(found);
    rejected(g, bad, g->mission);
    bad = good;
    bad.done |= BIT(found);
    rejected(g, bad, g->mission);
    bad = good;
    bad.sites[found] = (Cell){0, 1, 0};
    rejected(g, bad, g->mission);
    use(g, goal);
    CHECK(game_story_choose(g, 0, 2));
    good = g->story;
    bad = good;
    bad.choice = 3;
    rejected(g, bad, g->mission);
    use(g, STORY_EXTRACTION);
    good = g->story;
    bad = good;
    bad.tracked = 0;
    rejected(g, bad, g->mission);
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v4-format6.sav"));
    CHECK(g->world.generator == 4 && g->mission == 2 && g->story.version == 0);
    CHECK(g->players[0].level == 7 && g->players[0].credits == 123);
    valid(g);
    roundtrip(g);
    game_story_track(g);
    game_story_step(g);
    CHECK(game_story_near(g, 0) == SC_STORY_SCENES && !game_story_available(g, 0));
    bad = g->story;
    bad.seen = 1;
    rejected(g, bad, g->mission);
    game_new_ship(g, 713);
    CHECK(g->story.version == 3 && g->players[0].level == 1);
    valid(g);
    destroy(g);
}
static void test_history_discovery(void) {
    unsigned causes = 0, arcs = 0, profiles = 0, roles = 0;
    for (unsigned seed = 1; seed <= 40; seed++) {
        Game *g = create(seed);
        ShipHistory h = world_history(seed);
        CHECK(h.cause == world_history(seed).cause &&
              cell_equal(h.origin, world_history(seed).origin));
        causes |= BIT(h.cause);
        arcs |= BIT(h.arc);
        profiles |= BIT(world_profile(seed));
        RoomSpec origin = world_describe(seed, h.origin);
        CHECK(origin.history_role == HISTORY_ORIGIN && origin.height >= 3);
        CHECK(origin.theme == world_history_theme(h.cause));
        CHECK(origin.incident == world_history_incident(h.cause) && origin.intensity == 3);
        CHECK(cell_equal(origin.cell, h.origin));
        char text[320];
        for (unsigned role = 0; role < 4; role++) {
            world_history_notice(seed, role, text, sizeof(text));
            CHECK(*text && strlen(text) < sizeof(text));
            CHECK(role == 0 || *world_history_role(role));
        }
        game_story_dossier(g, 2, text, sizeof(text));
        CHECK(strstr(text, "UNKNOWN"));
        arrive(g, h.origin);
        game_story_step(g);
        CHECK(!(g->story.seen & BIT(STORY_ORIGIN_RECORD))); /* No commission accepted. */
        use(g, STORY_BRIEFING);
        unsigned money = g->players[0].credits, stage = g->mission;
        for (int y = -1; y <= 1; y++) {
            RegionPlan plan;
            world_plan_version(seed, g->world.generator, (Cell){0, y, 0}, &plan);
            for (int i = 0; i < plan.count; i++) {
                unsigned role = plan.rooms[i].history_role;
                if (!role)
                    continue;
                roles |= BIT(role);
                arrive(g, plan.rooms[i].cell);
                Room *r = world_find(&g->world, plan.rooms[i].cell);
                CHECK(r && r->history_role == role);
                Vec3 center = world_center(r->cell);
                for (int d = -6; d <= 6; d++)
                    CHECK(!world_overlap(&g->world, vadd(center, v3(d, .02, 0)), .7, 3.2));
                const Chunk *chunk = world_chunk(&g->world, r->cell);
                CHECK(chunk && chunk->box_count <= SC_CHUNK_BOXES &&
                      chunk->light_count == SC_CHUNK_LIGHTS);
                game_story_step(g);
                unsigned record = STORY_ORIGIN_RECORD + role - 1;
                CHECK(g->story.seen & BIT(record));
                Cell fixed = g->story.sites[record];
                game_story_step(g);
                CHECK(cell_equal(fixed, g->story.sites[record]));
                game_story_entry(g, record, text, sizeof(text));
                char notice[320];
                world_history_notice(seed, role, notice, sizeof(notice));
                CHECK(strcmp(text, notice) == 0);
                valid(g);
            }
        }
        CHECK(g->players[0].credits == money && g->mission == stage);
        CHECK(g->story.seen & BIT(STORY_ORIGIN_RECORD));
        game_story_dossier(g, 2, text, sizeof(text));
        CHECK(strcmp(text, world_history_cause(h.cause)) == 0);
        game_story_dossier(g, 3, text, sizeof(text));
        CHECK(strstr(text, "CATASTROPHE ORIGIN"));
        roundtrip(g);
        Story bad = g->story;
        bad.sites[STORY_ORIGIN_RECORD] = (Cell){1, 0, 0};
        rejected(g, bad, g->mission);
        bad = g->story;
        bad.seen &= ~BIT(STORY_ORIGIN_RECORD);
        rejected(g, bad, g->mission);
        if (story_motivation(seed) == STORY_CONTAINMENT) {
            use(g, STORY_REFUGE);
            CHECK(cell_equal(g->story.sites[STORY_SOURCE], h.origin));
            bad = g->story;
            bad.sites[STORY_SOURCE] = (Cell){1, 0, 0};
            rejected(g, bad, g->mission);
            use(g, STORY_SOURCE);
            Room *r = world_find(&g->world, h.origin);
            CHECK(r && r->enemy_count > 0);
            unsigned count = r->enemy_count;
            /* A distant remaining creature is purged along with the source. */
            r->enemies[count - 1].alive = true;
            r->enemies[count - 1].pos = position_from(vadd(game_terminal(h.origin), v3(25, 0, 0)));
            CHECK(game_story_choose(g, 0, 1));
            CHECK(r->record != SIZE_MAX);
            CHECK(g->world.records[r->record].killed == (BIT(count) - 1));
            roundtrip(g);
            while (g->world.pending)
                world_stream(&g->world, SC_STREAM_BUDGET);
            r = world_find(&g->world, h.origin);
            CHECK(r && r->enemy_count == (int)count);
            for (unsigned i = 0; i < count; i++)
                CHECK(!r->enemies[i].alive);
        }
        destroy(g);
    }
    CHECK(causes == 15 && arcs == 3 && profiles == 15 && roles == 14);
    Game *after = create(22);
    use(after, STORY_BRIEFING);
    use(after, STORY_REFUGE + story_motivation(22));
    use(after, game_story_target(after));
    CHECK(game_story_choose(after, 0, 1));
    unsigned money = after->players[0].credits, stage = after->mission;
    CHECK(!(after->story.seen & BIT(STORY_ORIGIN_RECORD)));
    arrive(after, world_history(22).origin);
    game_story_step(after);
    CHECK(after->story.seen & BIT(STORY_ORIGIN_RECORD));
    CHECK(after->players[0].credits == money && after->mission == stage &&
          after->story.choice == 1);
    valid(after);
    roundtrip(after);
    destroy(after);
    const char *fixtures[] = {"story-0-choice1.sav", "story-0-choice3.sav", "story-1-choice3.sav",
                              "story-2-choice3.sav"};
    for (unsigned i = 0; i < 4; i++) {
        Game *g = create(999);
        char path[256];
        snprintf(path, sizeof(path), "%s/%s", SC_FIXTURE_DIR, fixtures[i]);
        CHECK(game_load(g, path));
        CHECK(g->world.generator == 4 && g->story.version == 1);
        CHECK(story_motivation_version(g->world.seed, 1) == (i < 2    ? STORY_RESCUE
                                                             : i == 2 ? STORY_KNOWLEDGE
                                                                      : STORY_SALVAGE));
        CHECK(game_story_prepared(g) == (i > 0));
        valid(g);
        roundtrip(g);
        inspect_text(g);
        destroy(g);
    }
}
int main(void) {
    narrative_version = 1;
    test_arcs();
    narrative_version = 2;
    test_arcs();
    test_discovery_and_contest();
    test_validation_and_legacy();
    test_history_discovery();
    puts("PASS narrative: motivations, all outcomes, discoveries, stable reservations, co-op, "
         "persistence and validation");
    return 0;
}
