#include "save_internal.h"
void sc_save_restore(Game *g, const SavedExpedition *checkpoint) {
    matrix_destroy(&g->matrix);
    g->matrix = *checkpoint->matrix;
    free(checkpoint->matrix);
    world_destroy(&g->world);
    g->world = *checkpoint->world;
    free(checkpoint->world);
    memcpy(g->players, checkpoint->players, sizeof(g->players));
    memset(g->interactions, 0, sizeof(g->interactions));
    game_views_reset(g);
    for (int i = 0; i < SC_LEGACY_PLAYERS; i++)
        if (g->matrix.divers[i].active && g->matrix.divers[i].choice) {
            g->views[i].journal = true;
            g->views[i].journal_page = 4;
        }
    g->shelter = checkpoint->shelter;
    memcpy(g->rigs, checkpoint->rigs, sizeof(g->rigs));
    memcpy(g->flares, checkpoint->flares, sizeof(g->flares));
    memset(g->beams, 0, sizeof(g->beams));
    memset(g->builds, 0, sizeof(g->builds));
    g->player_count = 1;
    game_identity_reset(g);
    memset(g->projectiles, 0, sizeof(g->projectiles));
    game_deaths_reset(g);
    g->mission = checkpoint->mission;
    g->mission_kills = checkpoint->kills;
    g->mission_loot = checkpoint->loot;
    g->contracts = checkpoint->contracts;
    g->story = checkpoint->story;
    g->chronicle = checkpoint->chronicle;
    g->campaign = checkpoint->campaign;
    g->frontier = checkpoint->frontier;
    g->politics = checkpoint->politics;
    g->director = checkpoint->director;
    factions_destroy(&g->factions);
    g->factions = *checkpoint->factions;
    free(checkpoint->factions);
    memset(g->drills, 0, sizeof(g->drills));
    campaign_resume(g);
    Vec3 resumed = position_absolute(g->players[0].pos);
    world_refresh(&g->world, &resumed, 1);
    g->dirty = checkpoint->version < 6;
    game_message(g, checkpoint->version < 3
                        ? "SHIP GENERATOR UPDATED / CHARACTERS RETAINED / NEW EXPEDITION"
                        : "EXPEDITION RESUMED / WELCOME BACK");
}
