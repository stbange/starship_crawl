#include "game.h"
#include "legacy_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int sc_fail_allocation;
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
    legacy_game_init(g, 713);
    g->players[0].pos = position_from(matrix_terminal((Cell){0}));
    Vec3 at = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending)
        world_stream(&g->world, 2);
    return g;
}
static void indices(void) {
    MatrixState *m = calloc(1, sizeof(*m));
    CHECK(m);
    matrix_init(m);
    CHECK(matrix_index(m));
    CHECK(!matrix_record(m, (Cell){0}, false));
    sc_fail_allocation = 0;
    CHECK(!matrix_record(m, (Cell){0}, true));
    sc_fail_allocation = -1;
    sc_fail_allocation = 1;
    CHECK(!matrix_record(m, (Cell){0}, true));
    sc_fail_allocation = -1;
    for (int i = 0; i < 300; i++)
        CHECK(matrix_record(m, (Cell){4 * i, 0, 0}, true));
    CHECK(m->count == 300 && matrix_index(m));
    for (int i = 0; i < 300; i++)
        CHECK(matrix_record(m, (Cell){4 * i, 0, 0}, false));
    CHECK(matrix_record(m, (Cell){0}, true) && m->count == 300);
    sc_fail_allocation = 0;
    CHECK(!matrix_index(m));
    sc_fail_allocation = -1;
    Cell old = m->records[1].key;
    m->records[1].key = m->records[0].key;
    CHECK(!matrix_index(m));
    m->records[1].key = old;
    size_t count = m->count;
    m->count = SC_MATRIX_RECORDS;
    CHECK(!matrix_record(m, (Cell){-4, 0, 0}, true));
    m->count = count;
    m->networks[0].active = true;
    m->networks[0].key = (Cell){-4, 0, 0};
    CHECK(!matrix_rehydrate(m, 713, 6));
    matrix_destroy(m);
    free(m);
}
static void validation(void) {
    Game *g = fresh();
    CHECK(matrix_toggle(g, 0));
    MatrixState *m = &g->matrix;
    MatrixDiver *d = &m->divers[0];
    MatrixProfile *p = &m->profiles[0];
    MatrixRecord *r = &m->records[0];
    MatrixNetwork *w = &m->networks[0];
#define VALID() matrix_valid(m, g->world.seed, g->world.generator, g->players, &g->shelter)
#define BAD_D(field, value)                                                                        \
    do {                                                                                           \
        MatrixDiver keep = *d;                                                                     \
        d->field = value;                                                                          \
        CHECK(!VALID());                                                                           \
        *d = keep;                                                                                 \
    } while (0)
#define BAD_P(field, value)                                                                        \
    do {                                                                                           \
        MatrixProfile keep = *p;                                                                   \
        p->field = value;                                                                          \
        CHECK(!VALID());                                                                           \
        *p = keep;                                                                                 \
    } while (0)
#define BAD_R(field, value)                                                                        \
    do {                                                                                           \
        MatrixRecord keep = *r;                                                                    \
        r->field = value;                                                                          \
        CHECK(!VALID());                                                                           \
        *r = keep;                                                                                 \
    } while (0)
    CHECK(VALID());
    m->version = 2;
    CHECK(!VALID());
    m->version = 1;
    BAD_R(key, ((Cell){1, 0, 0}));
    BAD_R(flags, 4);
    BAD_R(outcome, 4);
    BAD_R(alert, 4);
    BAD_R(seen, 0x10000);
    BAD_R(keys, 1u << w->entry);
    BAD_R(loot, 1u << w->core);
    BAD_R(killed, 1u << 31);
    BAD_R(flags, MX_VAULT);
    BAD_R(outcome, 1);
    BAD_R(done[0], 1u << w->core);
    BAD_P(programs, 0);
    BAD_P(slots[0], 9);
    BAD_P(upgrades[0], 4);
    BAD_P(upgrades[5], 1);
    BAD_P(data, 1000000001);
    BAD_D(selected, 2);
    BAD_D(service, 6);
    BAD_D(unbanked, 100000);
    BAD_D(integrity, -1);
    BAD_D(bandwidth, NAN);
    BAD_D(trace, 101);
    BAD_D(body_yaw, 4);
    BAD_D(body_pitch, 2);
    BAD_D(pos.x, NAN);
    BAD_D(pos.y, -13);
    BAD_D(velocity.x, 31);
    BAD_D(echo.z, NAN);
    BAD_D(network, 2);
    BAD_D(key, ((Cell){4, 0, 0}));
    BAD_D(terminal, ((Cell){1, 0, 1}));
    Position body = g->players[0].pos;
    g->players[0].pos = position_from(v3(20, 0, 20));
    CHECK(!VALID());
    g->players[0].pos = body;
    g->players[0].sheltered = true;
    CHECK(!VALID());
    g->shelter.deployed = true;
    g->players[0].shelter_pos = shelter_station(SHELTER_TERMINAL);
    CHECK(VALID());
    g->players[0].shelter_pos = v3(0, 0, 0);
    CHECK(!VALID());
    g->players[0].sheltered = false;
    g->shelter.deployed = false;
    w->gate = 16;
    CHECK(!VALID());
    w->gate = 0;
    w->record = 2;
    CHECK(!VALID());
    w->record = 0;
    m->networks[1] = *w;
    CHECK(!VALID());
    memset(&m->networks[1], 0, sizeof(m->networks[1]));
    m->controls[0] = (MatrixControl){.life = 31};
    CHECK(!VALID());
    m->controls[0] = (MatrixControl){.life = 1};
    CHECK(!VALID());
    m->controls[0] = (MatrixControl){.life = 1, .kind = 1, .room = {1, 0, 1}};
    CHECK(!VALID());
    m->controls[0] = (MatrixControl){.life = 1, .kind = 1, .room = {0}};
    CHECK(VALID());
    m->controls[1] = m->controls[0];
    CHECK(!VALID());
    memset(m->controls, 0, sizeof(m->controls));
    CHECK(VALID());
    game_destroy(g);
    free(g);
}
static unsigned char *read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    CHECK(f);
    CHECK(fseek(f, 0, SEEK_END) == 0);
    *size = (size_t)ftell(f);
    rewind(f);
    unsigned char *b = malloc(*size);
    CHECK(b && fread(b, 1, *size, f) == *size);
    fclose(f);
    return b;
}
static void word(unsigned char *b, size_t at, uint32_t n) {
    for (int i = 0; i < 4; i++)
        b[at + i] = (unsigned char)(n >> (i * 8));
}
static void rewrite(unsigned char *b, size_t size) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size - 4; i++) {
        hash ^= b[i];
        hash *= 16777619u;
    }
    word(b, size - 4, hash);
    FILE *f = fopen("matrix-corrupt.sav", "wb");
    CHECK(f && fwrite(b, 1, size, f) == size);
    fclose(f);
}
static void saves_and_faults(void) {
    Game *g = fresh();
    CHECK(matrix_toggle(g, 0));
    CHECK(game_save(g, "matrix-fault.sav"));
    size_t size;
    unsigned char *original = read_file("matrix-fault.sav", &size), *bytes = malloc(size);
    CHECK(bytes);
    size_t appendix = size - 4 - 200 - 1584 - 64 * g->matrix.count;
    CHECK(original[4] == 11 && original[appendix] == 1);
    size_t offsets[] = {0, 4, 8, 20, 52, 68};
    uint32_t values[] = {2, SC_MATRIX_RECORDS + 1, 1, 0x10000, 8, 9};
    for (unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++) {
        memcpy(bytes, original, size);
        word(bytes, appendix + offsets[i], values[i]);
        rewrite(bytes, size);
        MatrixRecord *records = g->matrix.records;
        Position body = g->players[0].pos;
        CHECK(!game_load(g, "matrix-corrupt.sav"));
        CHECK(g->matrix.records == records && g->matrix.divers[0].active &&
              vlen(position_delta(body, g->players[0].pos)) == 0);
    }
    int failures = 0, successes = 0;
    for (int fail = 0; fail < 48; fail++) {
        MatrixRecord *records = g->matrix.records;
        sc_fail_allocation = fail;
        bool loaded = game_load(g, "matrix-fault.sav");
        sc_fail_allocation = -1;
        if (!loaded) {
            failures++;
            CHECK(g->matrix.records == records && g->matrix.divers[0].active);
        } else
            successes++;
    }
    printf("Matrix load fault sites: %d failures, %d successes\n", failures, successes);
    CHECK(failures >= 5);
    CHECK(game_load(g, "matrix-fault.sav"));
    CHECK(successes < 48);
    // Allocation failures at login and at a physical system operation do not charge resources.
    while (g->world.pending)
        world_stream(&g->world, 2);
    matrix_disconnect(g, 0, false);
    matrix_destroy(&g->matrix);
    matrix_init(&g->matrix);
    sc_fail_allocation = 0;
    CHECK(!matrix_toggle(g, 0));
    sc_fail_allocation = -1;
    CHECK(matrix_toggle(g, 0));
    MatrixDiver *d = &g->matrix.divers[0];
    d->pos = g->matrix.networks[d->network].nodes[0].at;
    d->service = MX_POWER;
    float bandwidth = d->bandwidth;
    sc_fail_allocation = 0;
    CHECK(!matrix_service(g, 0));
    sc_fail_allocation = -1;
    CHECK(d->bandwidth == bandwidth);
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v6-format9.sav"));
    CHECK(g->world.generator == 6 && g->story.version == 3 && g->matrix.count == 0 &&
          g->matrix.profiles[0].programs == 7);
    CHECK(game_save(g, "matrix-fault.sav") && game_load(g, "matrix-fault.sav"));
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v3.sav"));
    unsigned generator = g->world.generator, mission = g->mission;
    CHECK(g->story.version == 0);
    g->players[0].pos = position_from(matrix_terminal((Cell){0}));
    Vec3 at = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending)
        world_stream(&g->world, 2);
    CHECK(matrix_toggle(g, 0));
    g->matrix.divers[0].pos = g->matrix.networks[0].nodes[0].at;
    CHECK(matrix_service(g, 0));
    CHECK(game_save(g, "matrix-fault.sav") && game_load(g, "matrix-fault.sav"));
    CHECK(g->world.generator == generator && g->mission == mission && g->story.version == 0);
    CHECK(g->matrix.divers[0].active && (world_systems(&g->world, (Cell){0}) & SYSTEM_POWER));
    free(original);
    free(bytes);
    game_destroy(g);
    free(g);
    remove("matrix-fault.sav");
    remove("matrix-corrupt.sav");
}
int main(void) {
    indices();
    validation();
    saves_and_faults();
    puts("PASS matrix validation, legacy migration and transactional allocation/I/O faults");
    return 0;
}
