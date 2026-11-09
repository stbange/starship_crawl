#include "save_internal.h"
#ifdef _WIN32
#include <windows.h>
#endif
bool game_save(const Game *g, const char *path) {
    if (SC_PLAYERS != SC_LEGACY_PLAYERS)
        return false; /* Expanded parties use the live snapshot format. */
    if (!path || g->world.record_count > 1000000 || g->matrix.count > SC_MATRIX_RECORDS)
        return false;
    size_t len = strlen(path);
    char *tmp = malloc(len + 5);
    if (!tmp)
        return false;
    memcpy(tmp, path, len);
    memcpy(tmp + len, ".tmp", 5);
    FILE *f = fopen(tmp, "wb");
    if (!f) {
        free(tmp);
        return false;
    }
    Stream s = {f, UINT32_C(2166136261), true};
    sc_save_number(&s, UINT32_C(0x5343524c), false);
    sc_save_number(&s, g->world.generator >= 15 ? 16 : g->world.generator >= 14 ? 15 : 14, false);
    sc_save_number(&s, g->world.seed, false);
    sc_save_number(&s, g->world.generator, false);
    /* A saved session always opens in solo; the other profile remains available. */
    for (int i = 0; i < SC_LEGACY_PLAYERS; i++) {
        Player p = g->players[i];
        if (p.lift_active)
            p.pos = p.lift_from;
        sc_save_profile(&s, &p, false, 10);
    }
    sc_save_number(&s, g->mission, false);
    sc_save_number(&s, g->mission_kills, false);
    sc_save_number(&s, g->mission_loot, false);
    sc_save_number(&s, g->contracts, false);
    sc_save_number(&s, (uint32_t)g->world.record_count, false);
    for (size_t i = 0; i < g->world.record_count; i++) {
        const RoomRecord *r = &g->world.records[i];
        sc_save_number(&s, (uint32_t)r->cell.x, false);
        sc_save_number(&s, (uint32_t)r->cell.y, false);
        sc_save_number(&s, (uint32_t)r->cell.z, false);
        sc_save_number(&s, r->killed, false);
        sc_save_number(&s, r->collected, false);
    }
    Shelter shelter = g->shelter;
    Player players[SC_LEGACY_PLAYERS];
    memcpy(players, g->players, sizeof(players));
    for (int i = 0; i < SC_LEGACY_PLAYERS; i++)
        if (players[i].lift_active)
            players[i].pos = players[i].lift_from;
    sc_save_sanctuary(&s, &shelter, players, false);
    sc_save_powers(&s, players, false);
    Rig rigs[SC_LEGACY_PLAYERS][2];
    Flare flares[SC_MAX_FLARES];
    memcpy(rigs, g->rigs, sizeof(rigs));
    for (int i = 0; i < SC_LEGACY_PLAYERS; i++)
        if (players[i].lift_active && players[i].riding)
            rigs[i][players[i].riding - 1].pos = players[i].lift_from;
    memcpy(flares, g->flares, sizeof(flares));
    sc_save_equipment(&s, players, rigs, flares, false);
    Story story = g->story;
    sc_save_narrative(&s, &story, false, 10);
    sc_save_aftermath(&s, &story, players, g->world.records, g->world.record_count, false, 10);
    MatrixState *matrix = malloc(sizeof(*matrix));
    if (!matrix)
        s.ok = false;
    else {
        *matrix = g->matrix;
        sc_save_matrix_stream(&s, matrix, false);
        free(matrix);
    }
    Campaign campaign = g->campaign;
    sc_save_campaign_stream(&s, &campaign, false);
    for (size_t i = 0; i < g->world.record_count; i++)
        sc_save_number(&s, g->world.records[i].local, false);
    for (int p = 0; p < SC_LEGACY_PLAYERS; p++)
        for (int r = 0; r < RELICS; r++)
            sc_save_number(&s, g->frontier.ranks[p][r], false);
    Politics politics = g->world.generator >= 13 ? g->politics : (Politics){0};
    Director director = g->world.generator >= 13 ? g->director : (Director){0};
    sc_save_politics_stream(&s, &politics, &director, false);
    if (g->world.generator >= 14) {
        Factions *factions = malloc(sizeof(*factions));
        if (!factions)
            s.ok = false;
        else {
            *factions = g->factions;
            sc_save_factions_stream(&s, factions, false);
            free(factions);
        }
    }
    if (g->world.generator >= 15)
        for (int p = 0; p < SC_LEGACY_PLAYERS; p++) {
            sc_save_number(&s, g->players[p].intro, false);
            sc_save_number(&s, g->players[p].lessons, false);
        }
    uint32_t hash = s.hash;
    sc_save_number(&s, hash, false);
    if (fflush(f) != 0)
        s.ok = false;
    if (fclose(f) != 0)
        s.ok = false;
    bool ok = false;
    if (s.ok) {
#ifdef _WIN32
        ok = MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        ok = rename(tmp, path) == 0;
#endif
    }
    if (!ok)
        remove(tmp);
    free(tmp);
    return ok;
}
