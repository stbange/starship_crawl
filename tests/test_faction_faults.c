#include "faction_fixture.h"
extern int sc_fail_allocation;
static void validation(void) {
    Game *g = fixture(713);
    Factions *f = &g->factions;
    Cell site = ordinary(g, (Cell){8, 0, 8});
    CHECK(factions_record(g, site));
    CHECK(factions_launch(g, 1, ORDER_RAID, site, site));
    CHECK(factions_valid(f, 14));
#define BAD(field, value)                                                                          \
    do {                                                                                           \
        Factions copy = *f;                                                                        \
        f->field = (value);                                                                        \
        CHECK(!factions_valid(f, 14));                                                             \
        *f = copy;                                                                                 \
    } while (0)
    BAD(version, 4);
    BAD(count, SC_FACTION_REGIONS + 1);
    BAD(regions, NULL);
    BAD(remainder, NAN);
    BAD(next_id, 0);
    BAD(relations[0][1], 99);
    BAD(nations[0].supplies[0], 1000001);
    BAD(incidents[0][0].reports, 4);
    BAD(incidents[0][0].local_time, NAN);
    BAD(forces[1], f->forces[0]);
    BAD(forces[0].id, f->next_id);
    BAD(forces[0].country, 99);
    BAD(forces[0].route_time, NAN);
    BAD(forces[0].order, 99);
    BAD(forces[0].members[0].force, 99);
    BAD(forces[0].members[0].health, -1);
    BAD(forces[0].members[0].heading.x, INFINITY);
    BAD(forces[0].route_time, 1);
    BAD(forces[1].count, 1);
    BAD(forces[0].siege, 4);
    BAD(director.budget, 4);
    BAD(director.pressure[0], NAN);
    BAD(director.pace[1], 9);
    BAD(director.history[4], 99);
    BAD(director.proposal.phase, 5);
    BAD(director.proposal.force, f->next_id);
    BAD(events[0].kind, 99);
    BAD(events[0].force, f->next_id);
    FactionRegion region = f->regions[0];
    f->regions[0].region.x = INT32_MIN;
    CHECK(!factions_valid(f, 14));
    f->regions[0] = region;
    f->regions[0].edit = EDIT_STASH;
    f->regions[0].edit_site = (Cell){999, 0, 999};
    CHECK(!factions_valid(f, 14));
    f->regions[0] = region;
    CHECK(!factions_valid(f, 13));
#undef BAD
    finish(g);
}
static void allocation(void) {
    Game *g = fixture(42);
    Cell site = ordinary(g, (Cell){8, 0, 8});
    sc_fail_allocation = 0;
    CHECK(!factions_record(g, site));
    sc_fail_allocation = -1;
    CHECK(!g->factions.count && !g->factions.event_count);
    CHECK(factions_record(g, site));
    CHECK(factions_record(g, (Cell){20, 0, 8}));
    CHECK(factions_launch(g, 1, ORDER_PATROL, site, site));
    CHECK(game_save(g, "faction-transaction.sav"));
    bool loaded = false;
    unsigned failures = 0;
    for (int fail = 0; fail < 40; fail++) {
        unsigned seed = g->world.seed, tick = g->factions.tick;
        FactionRegion *regions = g->factions.regions;
        sc_fail_allocation = fail;
        bool ok = game_load(g, "faction-transaction.sav");
        sc_fail_allocation = -1;
        if (ok) {
            loaded = true;
            break;
        }
        failures++;
        CHECK(g->world.seed == seed && g->factions.tick == tick && g->factions.regions == regions);
        CHECK(factions_valid(&g->factions, 14));
    }
    CHECK(loaded && failures >= 4);
    /* The serialized appendix can be rejected even with a recomputed checksum. */
    FILE *file = fopen("faction-transaction.sav", "rb");
    CHECK(file);
    CHECK(!fseek(file, 0, SEEK_END));
    size_t length = (size_t)ftell(file);
    rewind(file);
    unsigned char *bytes = malloc(length), *copy = malloc(length);
    CHECK(bytes && copy);
    CHECK(fread(bytes, 1, length, file) == length);
    fclose(file);
    memcpy(copy, bytes, length);
    size_t start =
        length - 4 - 16 - SC_FACTIONS_SAVE_BYTES - g->factions.count * SC_FACTION_REGION_BYTES;
    const size_t offsets[] = {start, start + 52, start + 56 + 24,
                              start + 500 + g->factions.count * SC_FACTION_REGION_BYTES,
                              start + 500 + g->factions.count * SC_FACTION_REGION_BYTES + 4};
    for (unsigned n = 0; n < sizeof(offsets) / sizeof(offsets[0]); n++) {
        memcpy(bytes, copy, length);
        for (int j = 0; j < 4; j++)
            bytes[offsets[n] + j] = 255;
        uint32_t hash = 2166136261u;
        for (size_t i = 0; i < length - 4; i++)
            hash = (hash ^ bytes[i]) * 16777619u;
        for (int i = 0; i < 4; i++)
            bytes[length - 4 + i] = (unsigned char)(hash >> (i * 8));
        file = fopen("faction-corrupt.sav", "wb");
        CHECK(file);
        CHECK(fwrite(bytes, 1, length, file) == length);
        fclose(file);
        CHECK(!game_load(g, "faction-corrupt.sav"));
        CHECK(factions_valid(&g->factions, 14));
    }
    free(bytes);
    free(copy);
    remove("faction-transaction.sav");
    remove("faction-corrupt.sav");
    finish(g);
}
int main(void) {
    validation();
    allocation();
    puts("PASS malformed references, portable appendix and transactional allocation failures");
}
