#ifndef SC_TEST_LEGACY_GAME_H
#define SC_TEST_LEGACY_GAME_H
#include "game.h"
/* Existing mechanics suites retain the generator-6 sandbox as their fixture.
   test_phases.c and test_phase_journey.c exercise the shipped new-game defaults. */
static void legacy_game_init(Game *g, uint32_t seed) {
    game_init(g, seed);
    world_destroy(&g->world);
    world_init(&g->world, seed);
    g->world.generator = 6;
    factions_init(g); /* Legacy fixtures must not retain new-ship simulation flags. */
    g->campaign = (Campaign){0};
    for (int p = 0; p < SC_PLAYERS; p++) {
        g->players[p].rules = 0;
        g->players[p].weapons = SC_START_WEAPONS;
        g->matrix.profiles[p].programs = 7;
        g->matrix.profiles[p].slots[1] = MX_INTERRUPT;
        g->players[p].pos = position_from(v3(0, 4, -34));
    }
    Vec3 at = position_absolute(g->players[0].pos);
    world_refresh(&g->world, &at, 1);
}
#endif
