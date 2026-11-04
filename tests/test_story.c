#include "game.h"
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
static Game *create(unsigned seed) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    game_init(g, seed);
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
    }
    CHECK(!*game_story_choice(g, 0) && !*game_story_choice(g, 4));
    for (unsigned choice = 1; choice <= 3; choice++)
        CHECK(*game_story_choice(g, choice));
}
static void test_arcs(void) {
    unsigned motivations = 0, clues = 0;
    for (unsigned seed = 1; seed <= 12; seed++) {
        for (unsigned choice = 1; choice <= 3; choice++) {
            Game *g = create(seed);
            StoryMotivation m = story_motivation(seed);
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
            unsigned initial = STORY_REFUGE + m, alternate = STORY_REFUGE + (m + 1) % 3;
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
            unsigned goal = STORY_SURVIVORS + m;
            unsigned ops[] = {STORY_EVAC, STORY_MIRROR, STORY_REACTOR}, op = ops[m];
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
            CHECK(g->mission == 0 && g->story.version == 1 && g->story.seen == 0);
            CHECK(g->players[0].credits == money && g->players[0].victories == 1);
            valid(g);
            destroy(g);
        }
    }
    CHECK(motivations == 7 && clues);
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
            RoomSpec lab = world_describe(713, (Cell){x, y, 8});
            if (lab.district == RESEARCH && lab.incident != FROST && lab.height >= 3) {
                arrive(g, lab.cell);
                game_story_step(g);
                observed |= g->story.seen & 62;
            }
        }
    CHECK(observed == 62);
    inspect_text(g);
    roundtrip(g);
    unsigned lead = STORY_REFUGE + story_motivation(713);
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
    unsigned goal = STORY_SURVIVORS + story_motivation(713);
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
    bad.version = 2;
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
    bad.tracked = 16;
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
    use(g, STORY_REFUGE + story_motivation(713));
    good = g->story;
    unsigned goal = STORY_SURVIVORS + story_motivation(713);
    const unsigned ops[] = {STORY_EVAC, STORY_MIRROR, STORY_REACTOR};
    unsigned op = ops[story_motivation(713)];
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
    CHECK(g->story.version == 1 && g->players[0].level == 7);
    valid(g);
    destroy(g);
}
int main(void) {
    test_arcs();
    test_discovery_and_contest();
    test_validation_and_legacy();
    puts("PASS narrative: motivations, all outcomes, discoveries, stable reservations, co-op, "
         "persistence and validation");
    return 0;
}
