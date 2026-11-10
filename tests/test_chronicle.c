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
static void settle(Game *g, Vec3 at) {
    g->players[0].pos = position_from(at);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending && !g->world.failed)
        CHECK(world_stream(&g->world, 2) <= 2);
    CHECK(!g->world.failed);
}
static void meet(Game *g, unsigned i) {
    settle(g, chronicle_position(&g->chronicle.people[i]));
    GameCommand command = {.kind = CMD_INTERACT, .player = game_player_id(g, 0)};
    CHECK(game_command(g, &command));
    CHECK(g->interactions[0].decision_open &&
          g->interactions[0].decision_scene == SC_STORY_SCENES + i);
}
static bool choose(Game *g, unsigned choice) {
    GameCommand command = {
        .kind = CMD_STORY_CHOOSE, .player = game_player_id(g, 0), .argument = choice};
    return game_command(g, &command);
}
static void people(void) {
    for (unsigned seed = 1; seed <= 16; seed++) {
        Game *g = fresh(seed), *copy = fresh(seed);
        CHECK(chronicle_valid(&g->chronicle, seed, 16));
        CHECK(!memcmp(&g->chronicle, &copy->chronicle, sizeof(Chronicle)));
        CHECK(chronicle_near(g, -1) < 0 && chronicle_near(g, 2) < 0);
        CHECK(!chronicle_interact(g, 0) && !chronicle_choose(g, 0, 0));
        char text[512];
        chronicle_entry(g, 5, text, sizeof(text));
        CHECK(strstr(text, "NO MEETING"));
        meet(g, 0);
        CHECK(g->chronicle.people[0].meetings == 1);
        CHECK(
            strstr(game_story_title(g, SC_STORY_SCENES), chronicle_name(&g->chronicle.people[0])));
        CHECK(strlen(chronicle_role(0)) && strlen(chronicle_role(7)));
        CHECK(!chronicle_ready(g, 0) && !game_story_choice_ready(g, 0));
        CHECK(strstr(game_story_choice_for(g, 0, 2), "LOSES POWER"));
        CHECK(strstr(game_story_choice_for(g, 0, 3), "60 SALVAGE"));
        CHECK(!choose(g, 3) && !g->players[0].credits);
        CHECK(choose(g, 1)); /* Free information does not resolve the optional need. */
        CHECK(!(g->chronicle.people[0].knowledge & PERSON_HELPED));
        meet(g, 0);
        g->players[0].credits = 100;
        CHECK(game_story_choice_ready(g, 0) && choose(g, 3));
        CHECK(g->players[0].credits == 40 && g->chronicle.people[0].knowledge & PERSON_HELPED);
        CHECK(world_systems(&g->world, g->chronicle.people[0].site) & SYSTEM_POWER);
        CHECK(factions_at(&g->factions, g->chronicle.people[0].site)->systems & SYSTEM_POWER);
        meet(g, 0);
        CHECK(choose(g, 2) && g->players[0].credits == 40);
        game_story_entry(g, SC_STORY_SCENES, text, sizeof(text));
        CHECK(strstr(text, "REMEMBER"));
        g->views[0].story_reading = SC_STORY_SCENES - 1;
        game_story_read(g, 0);
        CHECK(g->views[0].story_reading == SC_STORY_SCENES);
        meet(g, 1);
        game_join(g);
        g->players[1].pos = g->players[0].pos;
        CHECK(chronicle_interact(g, 1));
        CHECK(choose(g, 2));
        CHECK(g->chronicle.people[1].fate == PERSON_LEFT && g->players[0].credits == 130);
        CHECK(factions_at(&g->factions, g->chronicle.people[1].site)->systems & SYSTEM_BLACKOUT);
        CHECK(!g->interactions[1].decision_open && !chronicle_choose(g, 1, 2));
        meet(g, 1);
        CHECK(choose(g, 2) && g->players[0].credits == 130);
        game_story_entry(g, SC_STORY_SCENES + 1, text, sizeof(text));
        CHECK(strstr(text, "EVACUATION"));
        CHECK(game_save(g, "chronicle.sav") && game_load(copy, "chronicle.sav"));
        CHECK(!memcmp(&g->chronicle, &copy->chronicle, sizeof(Chronicle)));
        unsigned char *data;
        size_t size;
        CHECK(game_snapshot_encode(g, &data, &size) && game_snapshot_decode(copy, data, size));
        free(data);
        CHECK(!memcmp(&g->chronicle, &copy->chronicle, sizeof(Chronicle)));
        g->chronicle.people[0].identity ^= 2;
        CHECK(!game_snapshot_encode(g, &data, &size));
        CHECK(game_save(g, "chronicle-bad.sav") && !game_load(copy, "chronicle-bad.sav"));
        game_new_ship(g, seed);
        CHECK(!g->chronicle.people[0].knowledge && !g->chronicle.people[1].knowledge);
        destroy(g);
        destroy(copy);
    }
    remove("chronicle.sav");
    remove("chronicle-bad.sav");
}
static void travel_and_loss(void) {
    Game *g = fresh(713);
    meet(g, 0);
    CHECK(world_set_systems(&g->world, g->chronicle.people[0].site, SYSTEM_POWER));
    CHECK(strstr(chronicle_option(g, 0, 3), "FREE") && choose(g, 3));
    CHECK(!g->players[0].credits);
    RecurringPerson *p = &g->chronicle.people[0];
    Cell origin = p->site;
    g->campaign.defeated = 1;
    g->world.open_layer = 2; /* Earned phase fixture. */
    settle(g, v3(0, 4, -84));
    chronicle_step(g, .02);
    CHECK(p->fate == PERSON_TRAVELLING && cell_equal(p->previous, origin));
    float time = p->travel;
    g->players[0].sheltered = true;
    chronicle_step(g, .1);
    CHECK(time == p->travel);
    game_join(g);
    g->players[1].sheltered = false;
    chronicle_step(g, .1);
    CHECK(p->travel < time);
    g->players[0].sheltered = false;
    char text[512];
    chronicle_entry(g, 0, text, sizeof(text));
    CHECK(strstr(text, "SERVICE PASSAGES"));
    for (int i = 0; i < 100 && p->fate == PERSON_TRAVELLING; i++)
        chronicle_step(g, 30);
    CHECK(p->fate == PERSON_SETTLED && !cell_equal(p->site, origin) && p->meetings == 2);
    uint32_t identity = p->identity;
    meet(g, 0);
    CHECK(p->identity == identity && choose(g, 1));
    RecurringPerson *other = &g->chronicle.people[1];
    FactionRegion *r = factions_record(g, other->site);
    CHECK(r);
    r->flags |= FS_CAPTURED;
    r->casualties = 3;
    settle(g, v3(0, 4, -84));
    chronicle_step(g, .02);
    CHECK(other->fate == PERSON_LOST);
    meet(g, 1);
    CHECK(choose(g, 1));
    chronicle_entry(g, 1, text, sizeof(text));
    CHECK(strstr(text, "RECORDER SURVIVED"));
    CHECK(game_story_target(g) < SC_STORY_SCENES);
    g->world.generator = 15;
    chronicle_init(g);
    CHECK(!g->chronicle.version);
    CHECK(chronicle_valid(&g->chronicle, 713, 15));
    chronicle_step(g, 1);
    CHECK(!chronicle_interact(g, 0));
    destroy(g);
}
static ContinuingCampaign *active_chain(Game *g) {
    for (unsigned i = 0; i < SC_CONTINUING_CAMPAIGNS; i++)
        if (g->chronicle.campaigns[i].identity)
            return &g->chronicle.campaigns[i];
    return NULL;
}
static void chains(void) {
    for (unsigned pattern = 0; pattern < 2; pattern++)
        for (unsigned intervene = 0; intervene < 2; intervene++) {
            Game *g = fresh(713);
            g->campaign.defeated = 0x55;
            for (unsigned id = 0; id < 8; id += 2) {
                g->campaign.eligible[id] = 1;
                Room won = {.cell = campaign_site(id), .record = SIZE_MAX, .enemy_count = 1};
                world_kill(&g->world, &won, 0);
            }
            campaign_sync(g); /* Completed-phase fixture makes distant checkpoint travel valid. */
            Cell source = {8, 0, 8}, goal = {12, 0, 8};
            FactionRegion *origin = factions_record(g, source),
                          *destination = factions_record(g, goal);
            CHECK(origin && destination);
            origin = (FactionRegion *)factions_at(&g->factions, source);
            origin->owner = 1;
            destination->owner = pattern ? 1 : 2;
            destination->kind = SITE_FABRICATOR;
            destination->flags |= FS_RAIDED;
            for (int r = 0; r < 3; r++)
                g->factions.nations[0].supplies[r] = 100;
            CHECK(chronicle_order(g, 1, pattern ? ORDER_REINFORCE : ORDER_RAID, source, goal));
            ContinuingCampaign *c = active_chain(g);
            CHECK(c && c->stage == CHAIN_SUPPLY);
            CHECK(!chronicle_order(g, 1, ORDER_RAID, source, goal));
            FactionForce *a = factions_force(g, c->force);
            CHECK(a && a->cargo == 12);
            uint32_t force = a->id, xp = g->players[0].xp, kills = g->players[0].kills;
            CHECK(g->factions.nations[0].supplies[2] <= 94);
            if (intervene) {
                /* Real damage dispatch against the established formation, no XP from NPC war. */
                Room room = {.cell = source, .enemy_count = 3};
                memcpy(room.enemies, a->members, sizeof(a->members));
                factions_damage(g, &room, 0, 10000, 0);
                factions_damage(g, &room, 1, 10000, 0);
                chronicle_campaigns(g, .02);
                CHECK(c->stage == CHAIN_RETREATED && a->order == ORDER_RETREAT);
                CHECK(!a->members[0].alive && !a->members[1].alive);
                CHECK(factions_at(&g->factions, c->staging)->flags & FS_RAIDED);
            } else {
                for (int tick = 0; tick < 1000 && c->stage == CHAIN_SUPPLY; tick++) {
                    factions_forces_tick(g);
                    chronicle_campaigns(g, .02);
                }
                CHECK(c->stage == (pattern ? CHAIN_FACTORY : CHAIN_SIEGE));
                CHECK(a->id == force && !a->cargo);
                if (pattern) {
                    CHECK(factions_at(&g->factions, goal)->flags & FS_REPAIRED);
                    chronicle_campaigns(g, 30);
                    CHECK(c->stage == CHAIN_REINFORCE && c->force != force &&
                          a->order == ORDER_RETREAT);
                }
                for (int tick = 0; tick < 2000 && c->stage < CHAIN_OCCUPIED; tick++) {
                    factions_forces_tick(g);
                    chronicle_campaigns(g, .02);
                }
                CHECK(c->stage == CHAIN_OCCUPIED);
                CHECK(factions_at(&g->factions, goal)->owner == 1);
            }
            CHECK(g->players[0].xp == xp && g->players[0].kills == kills);
            CHECK(chronicle_valid(&g->chronicle, 713, 16));
            unsigned char *data;
            size_t size;
            Game *copy = fresh(713);
            CHECK(game_snapshot_encode(g, &data, &size) && game_snapshot_decode(copy, data, size));
            free(data);
            CHECK(!memcmp(&g->chronicle, &copy->chronicle, sizeof(Chronicle)));
            CHECK(game_save(g, "chain.sav") && game_load(copy, "chain.sav"));
            CHECK(!memcmp(&g->chronicle, &copy->chronicle, sizeof(Chronicle)));
            remove("chain.sav");
            printf("CAMPAIGN pattern=%u intervene=%u stage=%u force=%u casualties=%u owner=%u\n",
                   pattern, intervene, c->stage, c->force, g->factions.casualties,
                   factions_at(&g->factions, goal)->owner);
            destroy(copy);
            destroy(g);
        }
}
static void edge_cases(void) {
    Game *g = fresh(713);
    Chronicle original = g->chronicle;
    g->chronicle.people[1].role = g->chronicle.people[0].role;
    CHECK(!chronicle_valid(&g->chronicle, 713, 16));
    g->chronicle = original;
    CHECK(!*chronicle_option(g, 0, 1));
    meet(g, 0);
    CHECK(!*chronicle_option(g, 0, 4));
    CHECK(*chronicle_option(g, 0, 1));
    g->chronicle.people[0].knowledge |= PERSON_HELPED;
    CHECK(strstr(chronicle_option(g, 0, 2), "UNCHANGED"));
    CHECK(!game_story_choose(g, -1, 1));
    Cell source = {8, 0, 8}, goal = {12, 0, 8};
    CHECK(!chronicle_order(g, 0, ORDER_RAID, source, goal));
    CHECK(!chronicle_order(g, 1, ORDER_PATROL, source, goal));
    g->chronicle.next_id = UINT32_MAX;
    CHECK(!chronicle_order(g, 1, ORDER_RAID, source, goal));
    g->chronicle.next_id = 1;
    CHECK(!chronicle_order(g, 1, ORDER_RAID, source, goal)); /* Sealed spatial band. */
    g->world.open_layer = 0;
    settle(g, v3(0, 4, -84));
    for (unsigned r = 0; r < 3; r++)
        g->factions.nations[0].supplies[r] = 0;
    CHECK(!chronicle_order(g, 1, ORDER_RAID, source, goal));
    CHECK(!g->chronicle.campaigns[0].identity && g->factions.next_id == 1);
    for (unsigned r = 0; r < 3; r++)
        g->factions.nations[0].supplies[r] = 100;
    FactionRegion *r = factions_record(g, goal);
    CHECK(r);
    r->owner = 1;
    r->kind = SITE_FABRICATOR;
    CHECK(chronicle_order(g, 1, ORDER_REINFORCE, source, goal));
    ContinuingCampaign *c = active_chain(g);
    for (int tick = 0; tick < 1000 && c->stage == CHAIN_SUPPLY; tick++)
        factions_forces_tick(g);
    CHECK(c->stage == CHAIN_FACTORY);
    g->players[0].sheltered = true;
    chronicle_campaigns(g, 30);
    CHECK(c->clock == 30);
    g->players[0].sheltered = false;
    for (unsigned resource = 0; resource < 3; resource++)
        g->factions.nations[0].supplies[resource] = 0;
    chronicle_campaigns(g, 30);
    CHECK(c->stage == CHAIN_FACTORY && !c->clock);
    r = (FactionRegion *)factions_at(&g->factions, goal);
    r->systems |= SYSTEM_BLACKOUT;
    chronicle_campaigns(g, .02);
    CHECK(c->stage == CHAIN_RETREATED);
    CHECK(chronicle_valid(&g->chronicle, 713, 16));
    c->country = 4;
    CHECK(!chronicle_valid(&g->chronicle, 713, 16));
    c->country = 1;
    g->chronicle.campaigns[0] = *c;
    CHECK(!chronicle_valid(&g->chronicle, 713, 16));
    g->world.generator = 15;
    CHECK(!chronicle_arrival(g, 1) && !chronicle_order(g, 1, ORDER_RAID, source, goal));
    chronicle_campaigns(g, 30);
    destroy(g);
}
int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--fixtures")) {
        for (unsigned i = 0; i < 2; i++) {
            Game *g = fresh(713);
            settle(g, vadd(chronicle_position(&g->chronicle.people[i]), v3(0, 0, -2)));
            g->players[0].yaw = g->players[0].pitch = 0;
            char file[80];
            snprintf(file, sizeof(file), "coherent-person%u.sav", i);
            CHECK(game_save(g, file));
            destroy(g);
        }
        return 0;
    }
    people();
    travel_and_loss();
    chains();
    edge_cases();
    puts("PASS recurring identities, known choices, routed campaigns and causal paired outcomes");
}
