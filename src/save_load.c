#include "save_internal.h"
bool game_load(Game *g, const char *path) {
    if (SC_PLAYERS != SC_LEGACY_PLAYERS)
        return false;
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    Stream s = {f, UINT32_C(2166136261), true};
    uint32_t magic = sc_save_number(&s, 0, true), version = sc_save_number(&s, 0, true),
             seed = sc_save_number(&s, 0, true);
    uint32_t generator = version >= 2 ? sc_save_number(&s, 0, true) : 1;
    if (magic != UINT32_C(0x5343524c) || (version < 1 || version > 16) ||
        (version == 2 && generator != 2) || (version < 11 && generator > 6) ||
        (version < 12 && generator > 10) || (version < 13 && generator > 11) ||
        (version < 14 && generator > 12) || (version < 15 && generator > 13) ||
        (version < 16 && generator > 14) ||
        (version >= 3 && (generator < SC_OLDEST_GENERATOR || generator > SC_GENERATOR_VERSION)))
        s.ok = false;
    Player players[SC_LEGACY_PLAYERS];
    memset(players, 0, sizeof(players));
    for (int i = 0; i < SC_LEGACY_PLAYERS; i++)
        sc_save_profile(&s, &players[i], true, version);
    uint32_t mission = 0, kills = 0, loot = 0, contracts = 0;
    if (version >= 3) {
        mission = sc_save_number(&s, 0, true);
        kills = sc_save_number(&s, 0, true);
        loot = sc_save_number(&s, 0, true);
        contracts = sc_save_number(&s, 0, true);
        if (mission > 6 || contracts > 7 || kills > 1000000000 || loot > 1000000000)
            s.ok = false;
    }
    uint32_t count = sc_save_number(&s, 0, true);
    if (count > 1000000)
        s.ok = false;
    RoomRecord *records = NULL;
    if (s.ok && count) {
        records = calloc(count, sizeof(*records));
        if (!records)
            s.ok = false;
    }
    for (uint32_t i = 0; s.ok && i < count; i++) {
        RoomRecord *r = &records[i];
        r->cell.x = (int32_t)sc_save_number(&s, 0, true);
        r->cell.y = (int32_t)sc_save_number(&s, 0, true);
        r->cell.z = (int32_t)sc_save_number(&s, 0, true);
        r->killed = sc_save_number(&s, 0, true);
        r->collected = version >= 3 ? sc_save_number(&s, 0, true) : 0;
        if (r->collected >= 32)
            s.ok = false;
        if (r->cell.x < -10000000 || r->cell.x > 10000000 || r->cell.y < -10000000 ||
            r->cell.y > 10000000 || r->cell.z < 0 || r->cell.z > 10000000 ||
            r->killed >= (UINT32_C(1) << SC_MAX_ENEMIES))
            s.ok = false;
    }
    Shelter shelter = {0};
    if (version >= 4)
        sc_save_sanctuary(&s, &shelter, players, true);
    if (version >= 5)
        sc_save_powers(&s, players, true);
    Rig rigs[SC_LEGACY_PLAYERS][2] = {0};
    Flare flares[SC_MAX_FLARES] = {0};
    if (version >= 6)
        sc_save_equipment(&s, players, rigs, flares, true);
    else
        for (int p = 0; p < SC_LEGACY_PLAYERS; p++) {
            players[p].weapons |= SC_NEW_WEAPONS;
            for (int k = 0; k < 2; k++)
                rigs[p][k] = (Rig){players[p].pos,
                                   game_rig_max_armor(&players[p], (RigKind)(k + 1)), false, 0};
        }
    Story story = {0};
    if (version >= 7) {
        sc_save_narrative(&s, &story, true, version);
        if (version >= 9)
            sc_save_aftermath(&s, &story, players, records, count, true, version);
        if (version < 10)
            for (unsigned i = 0; i < story.event_count && i < SC_STORY_EVENTS; i++)
                if (story.events[i].scene >= SC_STORY_SCENES)
                    s.ok = false;
        if (!story_valid(&story, seed, generator, mission))
            s.ok = false;
    }
    MatrixState *matrix = calloc(1, sizeof(*matrix));
    if (!matrix)
        s.ok = false;
    else {
        matrix_init(matrix);
        if (version >= 10)
            sc_save_matrix_stream(&s, matrix, true);
    }
    Campaign campaign = {0};
    if (version >= 11)
        sc_save_campaign_stream(&s, &campaign, true);
    for (uint32_t i = 0; s.ok && version >= 12 && i < count; i++) {
        RoomRecord *r = &records[i];
        r->local = sc_save_number(&s, 0, true);
        uint32_t claims = (r->local >> 4) & 3, eligible = (r->local >> 2) & 3;
        if (r->local >= 64 || (r->local && generator < 11) || (claims & ~eligible) ||
            (claims && !(r->local & FRONTIER_CLEARED)) ||
            ((r->local & FRONTIER_CLEARED) && (r->killed & 15) != 15))
            s.ok = false;
    }
    FrontierProgress frontier = {0};
    if (version >= 13)
        for (int p = 0; p < SC_LEGACY_PLAYERS; p++)
            for (int r = 0; r < RELICS; r++)
                frontier.ranks[p][r] = sc_save_number(&s, 0, true);
    if (!frontier_valid(&frontier, generator))
        s.ok = false;
    Politics politics = {0};
    Director director = {0};
    if (version >= 14)
        sc_save_politics_stream(&s, &politics, &director, true);
    if (!politics_valid(&politics, generator) || !director_valid(&director, generator))
        s.ok = false;
    Factions *factions = calloc(1, sizeof(*factions));
    if (!factions)
        s.ok = false;
    else {
        if (version >= 15)
            sc_save_factions_stream(&s, factions, true);
        if (!factions_valid(factions, generator))
            s.ok = false;
    }
    if (version >= 16)
        for (int p = 0; p < SC_LEGACY_PLAYERS; p++) {
            players[p].intro = sc_save_number(&s, 0, true);
            players[p].lessons = sc_save_number(&s, 0, true);
            if (!onboarding_valid(players[p].intro, players[p].lessons, generator,
                                  campaign.defeated) ||
                (matrix && matrix->divers[p].active && !(players[p].intro & INTRO_INTERFACE)))
                s.ok = false;
        }
    uint32_t expected = s.hash, actual = sc_save_number(&s, 0, true);
    if (actual != expected || fgetc(f) != EOF)
        s.ok = false;
    fclose(f);
    if (!s.ok) {
        if (factions) {
            factions_destroy(factions);
            free(factions);
        }
        if (matrix) {
            matrix_destroy(matrix);
            free(matrix);
        }
        free(records);
        return false;
    }
    if (version < 3) {
        /* Incompatible geometry must not reinterpret old doors, kills or positions.
           Keep both profiles and visibly begin a new expedition instead. */
        free(records);
        records = NULL;
        count = 0;
        seed = world_hash(seed, (Cell){0, 0, 0}, UINT32_C(0x4d494752));
        story_init(&story);
        for (int i = 0; i < SC_LEGACY_PLAYERS; i++) {
            players[i].pos = position_from(v3(i * 1.4, 4, -34));
            players[i].checkpoint = position_from(v3(0, .02, 12));
            players[i].health = player_max_health(&players[i]);
            players[i].yaw = 0;
            players[i].pitch = -.04;
        }
    }
    World *replacement = malloc(sizeof(*replacement));
    if (!replacement) {
        factions_destroy(factions);
        free(factions);
        matrix_destroy(matrix);
        free(matrix);
        free(records);
        return false;
    }
    world_init(replacement, seed);
    replacement->generator = version >= 3 ? generator : 6;
    replacement->records = records;
    replacement->record_count = count;
    replacement->record_cap = count;
    Vec3 start = position_absolute(players[0].pos);
    if (!campaign_valid(&campaign, players, replacement) ||
        !factions_world_valid(factions, replacement,
                              campaign_phase(&campaign) < SC_PHASES ? campaign_phase(&campaign) + 1
                                                                    : 0) ||
        !politics_world_valid(&politics, &director, replacement) || !matrix_index(matrix) ||
        !matrix_rehydrate(matrix, seed, replacement->generator) ||
        !matrix_valid(matrix, seed, replacement->generator, players, &shelter) ||
        !world_index_records(replacement) || !world_refresh(replacement, &start, 1)) {
        factions_destroy(factions);
        free(factions);
        matrix_destroy(matrix);
        free(matrix);
        world_destroy(replacement);
        free(replacement);
        return false;
    }
    SavedExpedition checkpoint = {
        .world = replacement,
        .matrix = matrix,
        .factions = factions,
        .players = players,
        .rigs = rigs,
        .flares = flares,
        .shelter = shelter,
        .story = story,
        .campaign = campaign,
        .frontier = frontier,
        .politics = politics,
        .director = director,
        .mission = mission,
        .kills = kills,
        .loot = loot,
        .contracts = contracts,
        .version = version,
    };
    sc_save_restore(g, &checkpoint);
    return true;
}
