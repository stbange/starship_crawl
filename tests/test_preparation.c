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
static Game *fresh(void) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    g->players[0].pos = position_from(v3(0, .02, 12));
    Vec3 at = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed);
    /* Explicit completed-guardian fixture: the ordinary opening still locks the fold. */
    CHECK(!(onboarding_capabilities(g, 0) & OB_SHELTER));
    g->campaign.defeated = 1;
    g->campaign.eligible[0] = 1;
    Room won = {.cell = campaign_site(0), .record = SIZE_MAX, .enemy_count = 1};
    world_kill(&g->world, &won, 0);
    campaign_sync(g);
    game_summon(g, 0);
    CHECK(g->shelter.deployed);
    game_interact(g, 0);
    CHECK(g->players[0].sheltered);
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void station(Game *g, int p, unsigned scene) {
    g->players[p].shelter_pos = preparation_station(scene);
    CHECK(shelter_clear(g->players[p].shelter_pos));
    CHECK(game_command(g, &(GameCommand){.kind = CMD_INTERACT, .player = game_player_id(g, p)}));
    CHECK(g->interactions[p].decision_open && g->interactions[p].decision_scene == scene);
}
static bool choose(Game *g, int p, unsigned choice) {
    return game_command(g, &(GameCommand){.kind = CMD_STORY_CHOOSE,
                                          .player = game_player_id(g, p),
                                          .argument = choice});
}
static void recovery_and_meals(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    CHECK(!preparation_near(g, -1) && !preparation_near(g, 2));
    CHECK(!preparation_interact(g, 0) && !preparation_choose(g, 0, 1));
    for (int use = SHELTER_BED; use <= SHELTER_KITCHEN; use++) {
        p->shelter_pos = vadd(shelter_station((ShelterUse)use), v3(0, .02, 0));
        p->health = 1;
        p->shield = p->energy = 0;
        game_interact(g, 0);
        for (int i = 0; i < 370; i++)
            game_step(g, (Actions[SC_PLAYERS]){{0}}, 1. / 60);
        CHECK(p->health > 1 && !p->credits);
    }
    station(g, 0, PREP_MEALS_SCENE);
    CHECK(!preparation_ready(g, 0) && !choose(g, 0, 1));
    CHECK(!preparation_choose(g, 0, 0) && !preparation_choose(g, 0, 4));
    CHECK(!strlen(preparation_option(g, 0, 4)));
    char text[1024];
    game_story_entry_for(g, 0, PREP_MEALS_SCENE, text, sizeof(text));
    CHECK(strstr(text, "FREE") && strstr(game_story_title(g, PREP_MEALS_SCENE), "MEALS"));
    p->credits = 200;
    for (unsigned meal = 1; meal <= 3; meal++) {
        station(g, 0, PREP_MEALS_SCENE);
        CHECK(preparation_ready(g, 0) && strlen(game_story_choice_for(g, 0, meal)));
        CHECK(choose(g, 0, meal));
        CHECK(g->home.personal[0].meal == (MealKind)meal && g->home.personal[0].remaining == 300);
        station(g, 0, PREP_MEALS_SCENE);
        unsigned before = p->credits;
        CHECK(!choose(g, 0, meal) && p->credits == before);
        preparation_step(g, 20);
        CHECK(g->home.personal[0].remaining == 300);
        game_join(g);
        g->players[1].sheltered = false;
        p->sheltered = false;
        p->energy = 0;
        preparation_step(g, 2);
        CHECK(g->home.personal[0].remaining == 298);
        CHECK(p->energy == (meal == MEAL_CHARGED ? 12 : 0));
        CHECK(preparation_damage(g, 0, 20, HURT_ELECTRIC) == (meal == MEAL_INSULATED ? 10 : 20));
        CHECK(preparation_damage(g, 0, 20, CHILL) == (meal == MEAL_WARM ? 10 : 20));
        CHECK(preparation_damage(g, 0, 20, 0) == 20);
        p->pos = position_from(v3(80, .02, 12)); /* Outside protected bay. */
        p->health = 100;
        p->shield = p->armor = 0;
        game_hurt(g, 0, 20, CHILL);
        CHECK(p->health == (meal == MEAL_WARM ? 90 : 80));
        CHECK(p->slow == (meal == MEAL_WARM ? 1 : 2));
        g->matrix.divers[0].active = true;
        preparation_step(g, 1);
        CHECK(g->home.personal[0].remaining == 297);
        g->matrix.divers[0].active = false;
        p->sheltered = true;
    }
    CHECK(p->credits == 110);
    CHECK(preparation_damage(g, -1, 20, CHILL) == 20 && preparation_damage(g, 2, 20, CHILL) == 20);
    p->sheltered = false;
    preparation_step(g, 400);
    CHECK(!g->home.personal[0].meal && !g->home.personal[0].remaining);
    CHECK(!strcmp(preparation_meal_name(99), "NONE"));
    g->world.generator = 15;
    CHECK(!preparation_near(g, 0) && preparation_damage(g, 0, 20, CHILL) == 20);
    preparation_step(g, 1);
    destroy(g);
}
static void projects(void) {
    Game *g = fresh();
    game_join(g);
    Player *p = &g->players[0];
    p->credits = 3000;
    g->players[1].credits = 3000;
    station(g, 0, PREP_PROJECTS_SCENE);
    station(g, 1, PREP_PROJECTS_SCENE);
    CHECK(strstr(preparation_option(g, 0, 1), "250"));
    CHECK(choose(g, 0, 1) && g->home.projects == PROJECT_REFIT && p->credits == 2750);
    CHECK(!choose(g, 1, 1) && g->players[1].credits == 3000); /* Stale shared quote. */
    CHECK(!choose(g, 1, 1));                                  /* No attributes to redistribute. */
    station(g, 0, PREP_PROJECTS_SCENE);
    for (int i = 0; i < SC_ATTRIBUTES; i++)
        *player_attribute(p, i) = 5;
    unsigned investment = player_investment(p);
    p->points = 1000;
    CHECK(!choose(g, 0, 1));
    p->points = 2;
    p->health = 13;
    p->shield = player_max_shield(p);
    p->energy = player_max_energy(p);
    p->armor = player_max_armor(p);
    g->rigs[0][0].armor = game_rig_max_armor(p, MECH);
    g->rigs[0][1].armor = game_rig_max_armor(p, WAGON);
    unsigned guns = p->weapons;
    CHECK(choose(g, 0, 1));
    CHECK(p->points == investment + 2 && !player_investment(p) && p->health == 13 &&
          p->weapons == guns);
    CHECK(p->shield == player_max_shield(p) && p->energy == player_max_energy(p) &&
          p->armor == player_max_armor(p));
    CHECK(g->rigs[0][0].armor == game_rig_max_armor(p, MECH) &&
          g->rigs[0][1].armor == game_rig_max_armor(p, WAGON));
    station(g, 0, PREP_PROJECTS_SCENE);
    CHECK(strstr(preparation_option(g, 0, 1), "25 SALVAGE") && !choose(g, 0, 1));
    CHECK(strstr(preparation_option(g, 0, 2), "200") && !choose(g, 0, 2));
    campaign_grant(g, 0, (CampaignReward){REWARD_GUN, SC_FLARE_CANNON});
    CHECK(choose(g, 0, 2) && !g->home.personal[0].flare_charge);
    station(g, 0, PREP_PROJECTS_SCENE);
    CHECK(strstr(preparation_option(g, 0, 2), "75") && choose(g, 0, 2));
    station(g, 0, PREP_PROJECTS_SCENE);
    CHECK(!choose(g, 0, 2) && g->home.personal[0].flare_charge);
    p->flare_lifetime = p->flare_strength = 5;
    preparation_flare(g, -1, 0);
    preparation_flare(g, 0, SC_MAX_FLARES);
    preparation_flare(g, 0, 0);
    game_attach_flare(g, 0, v3(0, 4, 12), v3(0, 1, 0));
    CHECK(g->home.personal[0].flare_charge);
    game_attach_flare(g, 0, v3(0, .001, 19), v3(0, -1, 0));
    CHECK(!g->home.personal[0].flare_charge && g->flares[0].active);
    CHECK(g->flares[0].life == 210 && g->flares[0].power == 9.375);
    char text[1024];
    for (unsigned c = 1; c <= 3; c++) {
        g->home.observed |= 1u << c;
        station(g, 0, PREP_PROJECTS_SCENE);
        CHECK(strstr(preparation_option(g, 0, 3), "100") && game_story_choice_ready(g, 0));
        CHECK(choose(g, 0, 3));
    }
    station(g, 0, PREP_PROJECTS_SCENE);
    CHECK(!preparation_ready(g, 0) && !choose(g, 0, 3));
    CHECK(strstr(preparation_option(g, 0, 3), "NO UNRESEARCHED"));
    CHECK(choose(g, 0, 2));
    game_story_entry_for(g, 0, PREP_PROJECTS_SCENE, text, sizeof(text));
    CHECK(strstr(text, "POWER:") && strstr(text, "FREEZE:") && strstr(text, "CONDUITS:") &&
          strstr(text, "FLARE READY"));
    Game *copy = fresh();
    CHECK(game_save(g, "preparation.sav") && game_load(copy, "preparation.sav"));
    CHECK(!memcmp(&g->home, &copy->home, sizeof(Home)) && copy->flares[0].life == 210);
    unsigned char *data;
    size_t size;
    CHECK(game_snapshot_encode(g, &data, &size) && game_snapshot_decode(copy, data, size));
    free(data);
    CHECK(!memcmp(&g->home, &copy->home, sizeof(Home)));
    game_new_ship(copy, 713);
    CHECK(!copy->home.projects && !copy->home.observed && !copy->home.personal[0].flare_charge);
    remove("preparation.sav");
    destroy(copy);
    destroy(g);
}
static void keepsakes(void) {
    Game *g = fresh();
    g->players[0].sheltered = false;
    Room *r = world_find(&g->world, (Cell){0});
    CHECK(r);
    preparation_observe(g, r);
    CHECK(!g->home.observed);
    r->journey = (DistrictMark){
        .anchor = {4, 0, 0}, .identity = 123, .family = 1, .condition = 1, .role = JOURNEY_VISTA};
    g->players[0].pos = position_from(v3(80, .02, 12));
    preparation_observe(g, r);
    CHECK(!g->home.observed);
    g->players[0].pos = position_from(v3(0, .02, 12));
    preparation_observe(g, r);
    CHECK(g->home.observed == 2 && !g->home.keepsake_count);
    for (unsigned i = 0; i < 7; i++) {
        r->journey.anchor.x = (int)i * 4;
        CHECK(world_set_district_state(&g->world, r->journey.anchor,
                                       DISTRICT_SEEN | DISTRICT_RESCUED | DISTRICT_SHORTCUT));
        preparation_observe(g, r);
        preparation_observe(g, r);
    }
    CHECK(g->home.keepsake_count == 6 && preparation_valid(&g->home, &g->world, g->players));
    unsigned count = 0;
    char text[1024];
    CHECK(!preparation_next_record(g, -1, true) && !preparation_next_record(g, 2, true));
    for (bool found = preparation_next_record(g, 0, true); found;
         found = preparation_next_record(g, 0, false)) {
        game_story_entry_for(g, 0, PREP_RECORD_SCENE, text, sizeof(text));
        CHECK(strstr(text, "REFUGE REPAIRED") && strstr(text, "RETURN CONDUIT"));
        count++;
    }
    CHECK(count == 7);
    g->views[0].district_reading = SIZE_MAX;
    game_story_entry_for(g, 0, PREP_RECORD_SCENE, text, sizeof(text));
    CHECK(strstr(text, "NO DISTRICT"));
    g->views[0].story_reading = SC_STORY_SCENES + SC_RECURRING - 1;
    game_story_read(g, 0);
    CHECK(g->views[0].story_reading == PREP_RECORD_SCENE);
    Home good = g->home;
    Home *h = &g->home;
    h->projects = 8;
    CHECK(!preparation_valid(h, &g->world, g->players));
    *h = good;
    h->observed = 16;
    CHECK(!preparation_valid(h, &g->world, g->players));
    *h = good;
    h->researched = 2;
    CHECK(!preparation_valid(h, &g->world, g->players));
    *h = good;
    h->keepsake_count = 7;
    CHECK(!preparation_valid(h, &g->world, g->players));
    *h = good;
    h->personal[0].meal = MEAL_WARM;
    CHECK(!preparation_valid(h, &g->world, g->players));
    *h = good;
    h->personal[0].remaining = NAN;
    CHECK(!preparation_valid(h, &g->world, g->players));
    *h = good;
    h->personal[0].flare_charge = true;
    CHECK(!preparation_valid(h, &g->world, g->players));
    *h = good;
    h->keepsakes[0].family = 0;
    CHECK(!preparation_valid(h, &g->world, g->players));
    *h = good;
    h->keepsakes[1].anchor = h->keepsakes[0].anchor;
    CHECK(!preparation_valid(h, &g->world, g->players));
    *h = good;
    Game *copy = fresh();
    unsigned char *data;
    size_t size;
    CHECK(game_save(g, "preparation.sav") && game_load(copy, "preparation.sav"));
    CHECK(!memcmp(&g->home, &copy->home, sizeof(Home)));
    CHECK(game_snapshot_encode(g, &data, &size) && game_snapshot_decode(copy, data, size));
    free(data);
    h->keepsakes[0].facts |= DISTRICT_LOST;
    CHECK(!game_snapshot_encode(g, &data, &size));
    CHECK(game_save(g, "preparation-bad.sav") && !game_load(copy, "preparation-bad.sav"));
    CHECK(!memcmp(&good, &copy->home, sizeof(Home)));
    remove("preparation.sav");
    remove("preparation-bad.sav");
    destroy(copy);
    destroy(g);
}
int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--fixtures")) {
        for (unsigned n = 0; n < 3; n++) {
            Game *g = fresh();
            if (n) {
                g->players[0].credits = 3000;
                campaign_grant(g, 0, (CampaignReward){REWARD_GUN, SC_FLARE_CANNON});
                g->home.observed = 2;
            }
            g->players[0].shelter_pos =
                preparation_station(n == 2 ? PREP_PROJECTS_SCENE : PREP_MEALS_SCENE);
            g->players[0].yaw = n == 2 ? 2.8 : 0;
            char name[60];
            snprintf(name, sizeof(name), "coherent-home%u.sav", n);
            CHECK(game_save(g, name));
            Game *copy = fresh();
            CHECK(game_load(copy, name));
            destroy(copy);
            destroy(g);
        }
        return 0;
    }
    recovery_and_meals();
    projects();
    keepsakes();
    puts("PASS optional meals, free recovery, shared projects, conserved refits, calibrated flares "
         "and six real keepsakes");
}
