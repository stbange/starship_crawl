/* Read-only evidence from the ordinary browser save action. */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    if (argc != 4)
        return 2;
    unsigned country = (unsigned)atoi(argv[2]);
    unsigned player = (unsigned)atoi(argv[3]);
    if (country < 1 || country > SC_COUNTRIES || player >= SC_PLAYERS)
        return 2;
    Game *g = calloc(1, sizeof(*g));
    if (!g)
        return 2;
    game_init(g, 713);
    if (!game_load(g, argv[1])) {
        game_destroy(g);
        free(g);
        return 1;
    }
    const FactionIncident *incident = &g->factions.incidents[player][country - 1];
    printf("{\"strikes\":%u,\"reports\":%u,\"grace\":%.4f,\"local_time\":%.4f,\"partner_strikes\":%u}\n",
           incident->strikes, incident->reports, incident->grace, incident->local_time,
           g->factions.incidents[1 - player][country - 1].strikes);
    game_destroy(g);
    free(g);
}
