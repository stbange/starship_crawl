#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Earned checkpoint, normal movement/look/fire/use only. No position, inventory,
   health or enemy writes. Read-only plans provide the same route as the HUD. */
static bool refit(Game *g, unsigned kind) {
    if (g->shelter.deployed)
        game_summon(g, 0);
    for (unsigned i = 0; i < 8 && !g->shelter.deployed; i++) {
        game_look(&g->players[0], SC_PI / 4, 0);
        game_summon(g, 0);
    }
    if (!g->shelter.deployed || !game_shelter_interact(g, 0) || !g->players[0].sheltered)
        return false;
    unsigned wanted = kind == CH_AERIE ? PSI_FLIGHT : PSI_NOVA;
    if (kind == CH_AERIE || kind == CH_COLONY) {
        for (unsigned i = 0; i < 5 && !campaign_power(g, 0, wanted); i++)
            campaign_refit(g, 0, 2, 1);
        if (!campaign_power(g, 0, wanted))
            return false;
    }
    if (kind == CH_SIEGE || kind == CH_CIRCUIT) {
        wanted = kind == CH_SIEGE ? MECH : WAGON;
        for (unsigned i = 0; i < 3 && !campaign_rig(g, 0, wanted); i++)
            campaign_refit(g, 0, 4, 1);
        if (!campaign_rig(g, 0, wanted))
            return false;
    }
    if (!game_shelter_interact(g, 0) || g->players[0].sheltered)
        return false;
    game_summon(g, 0);
    return true;
}
#include "encounter_driver.h"
static bool nearest(Game *g, unsigned kind, Cell *goal) {
    Cell from = world_cell(position_absolute(g->players[0].pos));
    double best = 1e9;
    Cell base = world_region(from);
    for (int z = base.z - 5; z <= base.z + 5; z++)
        for (int x = base.x - 5; x <= base.x + 5; x++) {
            if (z < 0)
                continue;
            RegionPlan plan;
            world_plan_version(g->world.seed, g->world.generator, (Cell){x, from.y, z}, &plan);
            for (int n = 0; n < plan.count; n++) {
                RoomSpec *r = &plan.rooms[n];
                if (r->habitat.pace != PACE_FRONTIER || r->habitat.challenge != kind ||
                    r->habitat.volume || r->height < 12 || r->width < 70 ||
                    campaign_layer(r->cell) < 5)
                    continue;
                bool done = false;
                for (size_t i = 0; i < g->world.record_count; i++)
                    if (cell_equal(g->world.records[i].cell, r->cell) &&
                        (g->world.records[i].local & FRONTIER_CLEARED))
                        done = true;
                if (done)
                    continue;
                double score = abs(r->cell.x - from.x) + abs(r->cell.z - from.z);
                if (score < best) {
                    best = score;
                    *goal = r->cell;
                }
            }
        }
    return best < 1e9;
}
int main(int argc, char **argv) {
    bool prepare = argc == 2 && !strcmp(argv[1], "--prepare");
    Game *g = calloc(1, sizeof(*g));
    if (!g)
        return 1;
    game_init(g, 713);
    bool ok = game_load(g, SC_FIXTURE_DIR "/frontier-expedition.sav") && g->world.generator == 12;
    const unsigned order[] = {CH_AERIE, CH_COLONY, CH_SIEGE, CH_CIRCUIT, CH_CIPHER};
    for (unsigned n = 0; n < 5 && ok; n++) {
        unsigned kind = order[n];
        Cell first = {0}, second = {0};
        ok = nearest(g, kind, &first);
        printf("APPROACH kind=%u first=%d,%d,%d\n", kind, first.x, first.y, first.z);
        fflush(stdout);
        ok = ok && route(g, first, true, false) && refit(g, kind) && nearest(g, kind, &second) &&
             route(g, second, false, false);
        if (!ok)
            break;
        /* Match both loadouts to the always-owned carbine. Reward auto-equip is
           otherwise a confounder when comparing the newly earned capability. */
        for (unsigned i = 0; i < SC_WEAPONS && g->players[0].weapon; i++)
            game_weapon(g, 0, 1);
        /* The second arena is identical for both runs; only normal tool use changes. */
        ok = game_save(g, "frontier-comparison.sav");
        if (prepare) {
            char path[512];
            snprintf(path, sizeof(path), "%s/frontier-%u.sav", SC_FIXTURE_DIR, kind);
            ok = ok && game_save(g, path);
        }
        for (unsigned tool = 0; tool < 2 && ok; tool++) {
            ok = game_load(g, "frontier-comparison.sav") && route(g, second, true, tool != 0);
            if (!ok)
                fprintf(stderr, "comparison failed kind=%u tool=%u: %s\n", kind, tool, g->message);
        }
    }
    remove("frontier-comparison.sav");
    game_destroy(g);
    free(g);
    puts(ok ? "PASS five earned-tool encounter comparisons" : "FAIL frontier journeys");
    return ok ? 0 : 1;
}
