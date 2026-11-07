#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encounter_driver.h"
int main(int argc, char **argv) {
    bool prepare = argc == 2 && !strcmp(argv[1], "--prepare");
    Game *g = calloc(1, sizeof(*g));
    if (!g)
        return 1;
    game_init(g, 713);
    bool ok = true;
    for (unsigned outcome = 1; outcome <= 3 && ok; outcome++) {
        unsigned nation = outcome - 1;
        ok = game_load(g, SC_FIXTURE_DIR "/courts-expedition.sav") && g->world.generator == 13;
        Cell court = politics_site(g->world.seed, nation, COURT_AUDIENCE);
        ok = ok && route(g, court, false, false);
        if (!ok)
            break;
        game_interact(g, 0);
        ok = politics_choose(g, 0, outcome);
        Cell conduit = politics_site(g->world.seed, nation, COURT_CONDUIT);
        Cell finale = politics_site(g->world.seed, nation, COURT_FINALE);
        ok = ok && route(g, conduit, true, false) && g->politics.countries[nation].stage == 2 &&
             route(g, finale, false, false);
        if (prepare && ok) {
            char path[512];
            snprintf(path, sizeof(path), "%s/courts-finale-%u.sav", SC_FIXTURE_DIR, outcome);
            ok = game_save(g, path);
        }
        ok = ok && route(g, finale, true, false) && g->politics.countries[nation].stage == 3;
        unsigned credits = g->players[0].credits;
        game_interact(g, 0);
        ok = ok && g->players[0].credits == credits;
        ok = ok && game_save(g, "courts-journey.sav") && game_load(g, "courts-journey.sav") &&
             g->politics.countries[nation].stage == 3;
        printf("COURTS outcome=%u country=%s stage=%u relation=%s sequence=%u %s\n", outcome,
               politics_name(g->world.seed, nation), g->politics.countries[nation].stage,
               politics_relation_name(politics_relation(g, nation, (nation + 1) % 3)),
               g->director.sequence, ok ? "PASS" : "FAIL");
        fflush(stdout);
    }
    remove("courts-journey.sav");
    game_destroy(g);
    free(g);
    return ok ? 0 : 1;
}
