#include "game.h"
#include "legacy_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BIT(n) (UINT32_C(1) << (n))
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static Game *create(unsigned seed) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    legacy_game_init(g, seed);
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void valid(Game *g) {
    if (!story_valid(&g->story, g->world.seed, g->world.generator, g->mission)) {
        fprintf(stderr,
                "invalid: seed=%u stage=%u chapter=%u arc=%u choice=%u/%u assigned=%x seen=%x "
                "done=%x tracked=%u support=%u,%u remain=%u last=%u,%u,%u,%u,%u "
                "total=%u,%u,%u,%u,%u events=%u\n",
                g->world.seed, g->mission, g->story.chapter, g->story.arc, g->story.choice,
                g->story.chapter_choice, g->story.assigned, g->story.seen, g->story.done,
                g->story.tracked, g->story.support[0], g->story.support[1], g->story.remaining,
                g->story.last[0], g->story.last[1], g->story.last[2], g->story.last[3],
                g->story.last[4], g->story.totals[0], g->story.totals[1], g->story.totals[2],
                g->story.totals[3], g->story.totals[4], g->story.event_count);
        CHECK(false);
    }
}
static void pump(Game *g, Cell c) {
    Vec3 at = game_terminal(c);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending)
        world_stream(&g->world, SC_STREAM_BUDGET);
    CHECK(world_ready(&g->world, c));
}
static void arrive(Game *g, Cell c) {
    g->players[0].pos = position_from(game_terminal(c));
    g->players[0].velocity = v3(0, 0, 0);
    pump(g, c);
    for (int r = 0; r < g->world.count; r++)
        for (int i = 0; i < g->world.rooms[r].enemy_count; i++)
            g->world.rooms[r].enemies[i].alive = false;
}
static void use(Game *g, unsigned scene) {
    CHECK(game_story_available(g, scene));
    arrive(g, g->story.sites[scene]);
    game_story_step(g);
    CHECK(game_story_near(g, 0) == scene);
    game_interact(g, 0);
    valid(g);
}
static void reload(Game *g) {
    Story before = g->story;
    unsigned career[SC_OUTCOMES];
    memcpy(career, g->players[0].career, sizeof(career));
    CHECK(game_save(g, "sc-saga-unit.sav"));
    CHECK(game_load(g, "sc-saga-unit.sav"));
    CHECK(!memcmp(&before, &g->story, sizeof(before)) &&
          !memcmp(career, g->players[0].career, sizeof(career)));
    valid(g);
    remove("sc-saga-unit.sav");
}
static void inspect(Game *g) {
    char text[400];
    for (unsigned scene = 0; scene <= SC_STORY_SCENES; scene++) {
        game_story_entry(g, scene, text, sizeof(text));
        CHECK(*text);
        CHECK(*game_story_title(g, scene));
    }
    for (int line = 0; line < 11; line++) {
        game_story_debrief(g, 0, line, text, sizeof(text));
        CHECK(*text);
        game_story_line(g, line, text, sizeof(text));
        CHECK(*text);
    }
    CHECK(!*expedition_option(g, STORY_COUNCIL, 0) && !*expedition_option(g, STORY_RESOLUTION, 4));
    CHECK(!*game_story_choice_for(g, -1, 0));
    game_story_debrief(g, -1, 7, text, sizeof(text));
    CHECK(*text);
    game_story_event_read(g, -1);
    game_story_event_read(g, 2);
    for (int n = 0; n < 35; n++)
        game_story_event_read(g, 0);
}
static void investigate(Game *g) {
    use(g, STORY_BRIEFING);
    unsigned lead = STORY_REFUGE + story_motivation(g->world.seed) % 3;
    use(g, lead);
    CHECK((g->story.assigned &
           (BIT(STORY_POWER) | BIT(STORY_BEACON) | BIT(STORY_CIPHER) | BIT(STORY_COUNCIL))) ==
          (BIT(STORY_POWER) | BIT(STORY_BEACON) | BIT(STORY_CIPHER) | BIT(STORY_COUNCIL)));
}
static void test_arcs(void) {
    unsigned arc_mask = 0, motives = 0;
    for (unsigned seed = 1; seed <= 16; seed++)
        for (unsigned choice = 1; choice <= 3; choice++) {
            Game *g = create(seed);
            valid(g);
            inspect(g);
            motives |= BIT(story_motivation(seed));
            Story fresh = g->story;
            MapView map;
            CHECK(game_map_build(g, 0, (Cell){200, 2, 300}, 4, &map));
            world_stream(&g->world, 2);
            CHECK(!memcmp(&fresh, &g->story, sizeof(fresh)));
            investigate(g);
            reload(g);
            use(g, STORY_COUNCIL);
            CHECK(g->players[0].decision_scene == STORY_COUNCIL);
            CHECK(!game_story_choice_ready(g, 0));
            CHECK(!game_story_choose(g, 0, 3));
            CHECK(strstr(game_story_choice(g, 1), "EVACUATION"));
            g->players[0].decision_open = g->players[0].journal = false;
            if (choice == 1) {
                use(g, STORY_BEACON);
                use(g, STORY_POWER);
            } else {
                use(g, STORY_POWER);
                use(g, STORY_BEACON);
            }
            CHECK(game_story_safe(g, game_terminal(g->story.sites[STORY_BEACON])));
            CHECK(game_story_safe(g, game_terminal(g->story.sites[STORY_COUNCIL])));
            CHECK(!game_story_safe(
                g, vadd(game_terminal(g->story.sites[STORY_COUNCIL]), v3(30, 0, 0))));
            use(g, STORY_CIPHER);
            CHECK(game_story_available(g, STORY_OVERRIDE));
            use(g, STORY_OVERRIDE);
            use(g, STORY_COUNCIL);
            CHECK(game_story_choice_ready(g, 0));
            game_join(g);
            g->players[1].pos = g->players[0].pos;
            game_interact(g, 1);
            CHECK(strstr(game_story_choice_for(g, 1, 3), "BROKER"));
            CHECK(game_story_choose(g, 1, choice));
            CHECK(!game_story_choose(g, 0, choice));
            CHECK(g->story.council == choice);
            valid(g);
            g->player_count = 1;
            if (choice == 2)
                use(g, game_story_operation(g));
            unsigned goal = game_story_target(g);
            if (choice == 3) {
                CHECK(game_story_prepared(g));
                CHECK(!(g->story.done & BIT(game_story_operation(g))));
            }
            use(g, goal);
            CHECK(game_story_choose(g, 0, choice));
            valid(g);
            inspect(g);
            reload(g);
            use(g, STORY_EXTRACTION);
            CHECK(g->story.totals[4] == 1 && g->players[0].career[4] == 1);
            CHECK(g->players[0].journal_page == 3 && game_story_available(g, STORY_CONTINUE));
            unsigned money = g->players[0].credits, victories = g->players[0].victories;
            expedition_record(g, SC_STORY_SCENES, 0);
            CHECK(g->players[0].credits == money);
            reload(g);
            for (unsigned chapter = 1; chapter <= 3; chapter++) {
                use(g, STORY_CONTINUE);
                CHECK(g->story.chapter == chapter);
                arc_mask |= BIT(g->story.arc);
                valid(g);
                inspect(g);
                reload(g);
                use(g, STORY_CONTACT);
                CHECK(game_story_available(g, STORY_RELAY) &&
                      game_story_available(g, STORY_RESOLUTION));
                Cell fixed = g->story.sites[STORY_RESOLUTION];
                if (g->story.arc == HARROW_ARC) {
                    arrive(g, fixed);
                    Room *heart = world_find(&g->world, fixed);
                    CHECK(heart);
                    heart->enemies[0].kind = 26;
                    heart->enemies[0].alive = true;
                    heart->enemies[0].pos = position_from(vadd(game_terminal(fixed), v3(30, 0, 0)));
                    g->players[0].decision_open = false;
                    game_interact(g, 0);
                    CHECK(!g->players[0].decision_open && strstr(g->message, "WARDEN"));
                    world_kill(&g->world, heart, 0);
                }
                use(g, STORY_RESOLUTION);
                CHECK(!game_story_choose(g, 0, 3));
                g->players[0].decision_open = g->players[0].journal = false;
                use(g, STORY_RELAY);
                CHECK(cell_equal(fixed, g->story.sites[STORY_RESOLUTION]));
                CHECK(game_story_safe(g, game_terminal(g->story.sites[STORY_RELAY])));
                reload(g);
                use(g, STORY_RESOLUTION);
                CHECK(game_story_choice_ready(g, 0));
                if (g->story.arc == QUARANTINE_ARC && choice == 1) {
                    Room *r = world_find(&g->world, fixed);
                    CHECK(r);
                    r->enemies[0].alive = true;
                    r->enemies[0].pos = position_from(vadd(game_terminal(fixed), v3(30, 0, 0)));
                }
                CHECK(game_story_choose(g, 0, choice));
                valid(g);
                inspect(g);
                use(g, STORY_RETURN);
                CHECK(g->story.totals[4] == chapter + 1 && g->players[0].career[4] == chapter + 1);
                CHECK(g->players[0].victories == victories + chapter &&
                      g->players[0].credits > money);
                CHECK(game_story_available(g, STORY_CONTINUE));
                valid(g);
                reload(g);
            }
            unsigned career[SC_OUTCOMES];
            memcpy(career, g->players[0].career, sizeof(career));
            game_new_ship(g, seed + 2000);
            CHECK(memcmp(career, g->players[0].career, sizeof(career)) != 0);
            for (int i = 0; i < SC_OUTCOMES; i++)
                CHECK(g->players[0].career[i] == 0);
            valid(g);
            destroy(g);
        }
    CHECK(arc_mask == (1u << STORY_ARCS) - 1 && motives == 15);
}
static void test_physical_effects(void) {
    Game *g = create(22);
    CHECK(!game_story_safe(g, v3(0, 0, -20)));
    use(g, STORY_BRIEFING);
    arrive(g, (Cell){1, 0, 0});
    game_story_step(g);
    use(g, STORY_POWER);
    use(g, STORY_BEACON);
    use(g, STORY_REFUGE + story_motivation(g->world.seed) % 3);
    CHECK((world_systems(&g->world, g->story.sites[STORY_COUNCIL]) & 3) == 3);
    use(g, STORY_CIPHER);
    use(g, STORY_OVERRIDE);
    Cell beacon = g->story.sites[STORY_BEACON], control = g->story.sites[STORY_OVERRIDE];
    arrive(g, beacon);
    Player *p = &g->players[0];
    p->health = 50;
    p->shield = 5;
    p->energy = 5;
    p->hurt = 0;
    game_story_recovery(g, p, 1);
    CHECK(p->health == 55 && p->shield == 13 && p->energy == 15);
    game_hurt(g, 0, 50, CHILL | DRAIN);
    CHECK(p->health == 55 && p->shield == 13 && p->energy == 15 && p->slow == 0);
    p->hurt = 1;
    game_story_recovery(g, p, 1);
    CHECK(p->health == 55);
    p->hurt = 0;
    p->sheltered = true;
    game_story_recovery(g, p, 1);
    CHECK(p->health == 55);
    p->sheltered = false;
    Enemy enemy = {
        .alive = true, .kind = 16, .health = 100, .pos = position_from(game_terminal(control))};
    CHECK(game_story_inert(g, &enemy));
    enemy.kind = 0;
    CHECK(!game_story_inert(g, &enemy));
    arrive(g, control);
    Room *r = world_find(&g->world, control);
    CHECK(r);
    r->enemy_count = 1;
    r->enemies[0] = enemy;
    r->enemies[0].kind = 16;
    unsigned shots = g->enemy_shots;
    Actions actions[2] = {{0}, {0}};
    for (int i = 0; i < 90; i++)
        game_step(g, actions, 1. / 60);
    CHECK(g->enemy_shots == shots);
    arrive(g, beacon);
    r = world_find(&g->world, beacon);
    r->enemy_count = 1;
    r->enemies[0] = enemy;
    r->enemies[0].pos = position_from(vadd(game_terminal(beacon), v3(2, 0, 0)));
    float health = p->health;
    for (int i = 0; i < 90; i++)
        game_step(g, actions, 1. / 60);
    CHECK(p->health >= health && g->enemy_shots == shots);
    uint32_t flags = world_systems(&g->world, beacon);
    CHECK((flags & (SYSTEM_POWER | SYSTEM_BEACON)) == (SYSTEM_POWER | SYSTEM_BEACON));
    pump(g, (Cell){100, 0, 100});
    CHECK(world_systems(&g->world, beacon) == flags);
    pump(g, beacon);
    CHECK(world_systems(&g->world, beacon) == flags);
    arrive(g, beacon);
    reload(g);
    pump(g, beacon);
    CHECK(game_story_safe(g, game_terminal(beacon)));
    MapView map;
    CHECK(game_map_build(g, 0, beacon, 1, &map));
    CHECK((map.systems[4] & (SYSTEM_POWER | SYSTEM_BEACON)) == (SYSTEM_POWER | SYSTEM_BEACON));
    CHECK(!world_set_systems(&g->world, beacon, 16));
    CHECK(!world_set_systems(&g->world, beacon, SYSTEM_POWER | SYSTEM_BLACKOUT));
    CHECK(!world_set_systems(&g->world, (Cell){0, 0, -1}, 0));
    CHECK(world_set_systems(&g->world, beacon, SYSTEM_BLACKOUT | SYSTEM_BEACON));
    CHECK(!game_story_safe(g, game_terminal(beacon)));
    p->pos = position_from(game_terminal(beacon));
    p->health = 55;
    game_story_recovery(g, p, 1);
    CHECK(p->health == 55);
    p->pos = position_from(vadd(game_terminal(beacon), v3(30, 0, 0)));
    game_hurt(g, 0, 10, 0);
    CHECK(p->hurt > 0);
    CHECK(world_systems(&g->world, (Cell){0, 0, -1}) == 0);
    destroy(g);
}
static void reject(Game *g, Story bad) {
    Story before = g->story;
    RoomRecord *records = g->world.records;
    unsigned credits = g->players[0].credits;
    g->story = bad;
    CHECK(game_save(g, "sc-saga-bad.sav"));
    g->story = before;
    CHECK(!story_valid(&bad, g->world.seed, g->world.generator, g->mission));
    CHECK(!game_load(g, "sc-saga-bad.sav"));
    CHECK(!memcmp(&g->story, &before, sizeof(before)) && g->world.records == records &&
          g->players[0].credits == credits);
    remove("sc-saga-bad.sav");
}
#define BAD(...)                                                                                   \
    do {                                                                                           \
        Story bad = g->story;                                                                      \
        __VA_ARGS__;                                                                               \
        reject(g, bad);                                                                            \
    } while (0)
static void test_validation(void) {
    Game *g = create(22);
    BAD(bad.support[0] = 101);
    BAD(bad.remaining = 1);
    BAD(bad.support[0] = 49);
    BAD(bad.council = 1);
    BAD(bad.done |= BIT(STORY_OVERRIDE));
    BAD(bad.assigned |= BIT(STORY_OVERRIDE));
    BAD(bad.assigned |= BIT(STORY_CONTINUE));
    BAD(bad.chapter = 1);
    BAD(bad.arc = CHART_ARC);
    BAD(bad.chapter_choice = 1);
    BAD(bad.done |= BIT(STORY_COUNCIL); bad.council = 3);
    CHECK(!expedition_choose(g, -1, STORY_COUNCIL, 1));
    CHECK(!expedition_choose(g, 0, STORY_POWER, 1));
    CHECK(!expedition_available(&g->story, STORY_BRIEFING));
    investigate(g);
    BAD(bad.totals[4] = 1);
    BAD(bad.remaining++);
    BAD(bad.last[1] = 4);
    BAD(bad.totals[1] = 3004);
    BAD(bad.remaining--; bad.last[0] = 1);
    BAD(bad.last[4] = 1);
    BAD(bad.events[bad.event_count].scene = 1);
    BAD(bad.events[0].scene = SC_STORY_SCENES);
    BAD(bad.events[0].chapter = 1);
    BAD(bad.events[0].choice = 4);
    BAD(bad.events[0].arc = STORY_ARCS);
    BAD(bad.events[0].site.z = -1);
    BAD(bad.events[0].site.x = 1);
    BAD(bad.events[0].scene = STORY_POWER);
    BAD(bad.events[0].scene = STORY_POWER; bad.events[0].site = (Cell){1, 0, 0};
        bad.events[0].choice = 1);
    BAD(bad.events[0].outcome[0] = world_history(g->world.seed).population + 1);
    BAD(bad.events[1] = bad.events[0]);
    use(g, STORY_POWER);
    use(g, STORY_CIPHER);
    use(g, STORY_COUNCIL);
    CHECK(game_story_choose(g, 0, 3));
    use(g, game_story_target(g));
    CHECK(game_story_choose(g, 0, 3));
    use(g, STORY_EXTRACTION);
    use(g, STORY_CONTINUE);
    BAD(bad.assigned &= ~BIT(STORY_CONTACT));
    BAD(bad.done |= BIT(STORY_CONTACT));
    BAD(bad.done &= ~BIT(STORY_CONTINUE));
    use(g, STORY_CONTACT);
    BAD(bad.chapter_choice = 1);
    BAD(bad.chapter_choice = 3; bad.done |= BIT(STORY_RESOLUTION));
    BAD(bad.assigned |= BIT(STORY_RETURN));
    BAD(bad.sites[STORY_CONTACT].x = 8);
    BAD(bad.arc = HARROW_ARC; bad.sites[STORY_RESOLUTION] = (Cell){1, 0, 0});
    BAD(bad.events[1].chapter = 1; bad.events[0].chapter = 1; bad.events[2].chapter = 0);
    use(g, STORY_RELAY);
    use(g, STORY_RESOLUTION);
    CHECK(game_story_choose(g, 0, 3));
    use(g, STORY_RETURN);
    for (unsigned i = 0; i < 7; i++) {
        use(g, STORY_CONTINUE);
        use(g, STORY_CONTACT);
        use(g, STORY_RELAY);
        use(g, STORY_RESOLUTION);
        CHECK(game_story_choose(g, 0, 3));
        use(g, STORY_RETURN);
    }
    CHECK(g->story.event_count == SC_STORY_EVENTS);
    reload(g);
    Story cap = g->story;
    cap.chapter = 1000;
    cap.totals[4] = 1001;
    cap.tracked = STORY_RETURN;
    unsigned n = 0;
    for (int z = 4000; z < 4008 && n < 3; z++)
        for (int x = 0; x < 8 && n < 3; x++) {
            RoomSpec r = world_describe(g->world.seed, (Cell){x, 0, z});
            if (r.height < 3 || !cell_equal(r.cell, (Cell){x, 0, z}))
                continue;
            cap.sites[STORY_CONTACT + n++] = r.cell;
        }
    CHECK(n == 3);
    if (cap.arc == HARROW_ARC)
        cap.sites[STORY_RESOLUTION] = (Cell){2, 1, 2};
    CHECK(story_valid(&cap, g->world.seed, g->world.generator, 6));
    CHECK(!expedition_available(&cap, STORY_CONTINUE));
    g->players[0].career[0] = 1000000001;
    CHECK(game_save(g, "sc-saga-bad.sav"));
    g->players[0].career[0] = 0;
    CHECK(!game_load(g, "sc-saga-bad.sav"));
    remove("sc-saga-bad.sav");
    CHECK(g->world.record_count);
    unsigned saved = g->world.records[0].systems;
    g->world.records[0].systems = 16;
    CHECK(game_save(g, "sc-saga-bad.sav"));
    g->world.records[0].systems = saved;
    CHECK(!game_load(g, "sc-saga-bad.sav"));
    remove("sc-saga-bad.sav");
    g->story = (Story){.version = 2, .assigned = 1};
    char text[400];
    game_story_debrief(g, 0, 0, text, sizeof(text));
    CHECK(strstr(text, "ARCHIVAL"));
    BAD(bad.support[0] = 1);
    destroy(g);
}
int main(void) {
    test_arcs();
    test_physical_effects();
    test_validation();
    puts("PASS reactive aftermath: councils, power, wards, controllers, all chapter "
         "kinds/outcomes, Warden gate, careers, events and persistence");
    return 0;
}
