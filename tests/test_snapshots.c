#include "snapshot.h"
#include "snapshot_internal.h"
#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef SC_FAULT_TEST
extern int sc_fail_allocation;
#endif
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
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void equal_state(const Game *a, const Game *b) {
    unsigned char *x, *y;
    size_t nx, ny;
    CHECK(game_snapshot_encode(a, &x, &nx));
    CHECK(game_snapshot_encode(b, &y, &ny));
    CHECK(nx == ny && !memcmp(x, y, nx));
    free(x);
    free(y);
}
static void update_hash(unsigned char *data, size_t size) {
    uint32_t h = UINT32_C(2166136261);
    for (size_t i = 0; i + 4 < size; i++)
        h = (h ^ data[i]) * UINT32_C(16777619);
    for (unsigned i = 0; i < 4; i++)
        data[size - 4 + i] = (unsigned char)(h >> (i * 8));
}
static void roundtrip(void) {
    Game *g = fresh(), *copy = fresh();
    CHECK(game_party_resize(g, SC_PLAYERS));
    g->campaign.version = 0;
    g->world.open_layer = 0;
    for (int i = 0; i < SC_PLAYERS; i++) {
        g->players[i].pos = position_from(v3(i * 1.4, .02, 12));
        g->views[i].guide_mode = GUIDE_OFF;
    }
    Session session;
    session_init(&session, g);
    for (int i = 0; i < 40; i++)
        session_tick(&session);
    g->players[0].pos = position_from(matrix_terminal((Cell){0, 0, 0}));
    CHECK(matrix_toggle(g, 0));
    MatrixNetwork *network = &g->matrix.networks[g->matrix.divers[0].network];
    network->guards[0].health = 17.25f;
    network->guards[0].stun = .75f;
    network->shots[0] = (MatrixShot){.pos = {1, 2, 3},
                                     .velocity = {2, 1, 3},
                                     .life = 2,
                                     .damage = 11,
                                     .owner = -1,
                                     .active = true};
    g->projectiles[0] = (Projectile){.pos = g->players[1].pos,
                                     .velocity = {2, 0, 1},
                                     .life = .7f,
                                     .damage = 8,
                                     .owner = 1,
                                     .active = true};
    g->projectiles[0].hit_count = 1;
    g->projectiles[0].hit_actors[0] =
        game_actor_id(g, &g->world.rooms[0], &g->world.rooms[0].enemies[0]);
    g->beams[1] = (Beam){g->players[1].pos, g->players[1].pos, {.2f, .3f, .4f}, .1f};
    g->flares[0] = (Flare){g->players[1].pos, {0, 1, 0}, 20, 8, 1, 1, true};
    g->interactions[1] = (InteractionState){STORY_COUNCIL, true};
    g->players[1].cooldown = .17f;
    g->players[1].velocity = v3(1, 2, 3);
    g->world.rooms[0].enemies[0].windup = .33f;
    g->world.rooms[0].enemies[0].motion_time = .71f;
    g->views[0].journal = true;
    g->views[0].map_open = true;
    copy->views[0].guide_mode = GUIDE_OFF;
    PlayerId stale = game_player_id(copy, 0);
    unsigned char *data;
    size_t size;
    CHECK(game_snapshot_encode(g, &data, &size));
    CHECK(size > 1000 && size < SC_SNAPSHOT_LIMIT);
    CHECK(game_snapshot_decode(copy, data, size));
    CHECK(game_player_slot(copy, stale) == -1);
    CHECK(!copy->views[0].map_open && copy->views[0].guide_mode == GUIDE_OFF);
    CHECK(copy->interactions[1].decision_open);
    CHECK(copy->players[1].cooldown == .17f && copy->projectiles[0].life == .7f);
    CHECK(copy->matrix.networks[copy->matrix.divers[0].network].guards[0].health == 17.25f);
    equal_state(g, copy);
    g->views[0].journal = false;
    g->views[0].map_open = false;
    for (int i = 0; i < SC_PLAYERS; i++)
        copy->views[i].guide_mode = GUIDE_OFF;
    Session a, b;
    session_init(&a, g);
    session_init(&b, copy);
    Actions input = {.strafe = .2f, .fire = true};
    CHECK(session_input(&a, game_player_id(g, 1), &input));
    CHECK(session_input(&b, game_player_id(copy, 1), &input));
    for (int i = 0; i < 5; i++) {
        session_tick(&a);
        session_tick(&b);
    }
    equal_state(g, copy);
    /* Every failed import leaves the destination and its views intact. */
    CHECK(!game_snapshot_decode(copy, NULL, size));
    CHECK(!game_snapshot_decode(copy, data, 15));
    CHECK(!game_snapshot_decode(copy, data, SC_SNAPSHOT_LIMIT + 1));
    for (size_t cut = 16; cut < size; cut = cut * 3 / 2 + 1)
        CHECK(!game_snapshot_decode(copy, data, cut));
    CHECK(!game_snapshot_decode(copy, data, size - 1));
    for (int byte = 0; byte < 16; byte += 4) {
        data[byte] ^= 128;
        update_hash(data, size);
        CHECK(!game_snapshot_decode(copy, data, size));
        data[byte] ^= 128;
        update_hash(data, size);
    }
    unsigned char generator = data[20];
    data[20] = 0;
    update_hash(data, size);
    CHECK(!game_snapshot_decode(copy, data, size));
    data[20] = generator;
    update_hash(data, size);
    data[size - 1] ^= 1;
    CHECK(!game_snapshot_decode(copy, data, size));
    data[size - 1] ^= 1;
    equal_state(g, copy);
    unsigned char *extra = malloc(size + 1);
    CHECK(extra);
    memcpy(extra, data, size);
    extra[size] = 0;
    CHECK(!game_snapshot_decode(copy, extra, size + 1));
    free(extra);
    free(data);
    destroy(g);
    destroy(copy);
}
static void validation(void) {
    Game *g = fresh();
    unsigned char *data;
    size_t size;
    CHECK(game_snapshot_encode(g, &data, &size));
    free(data);
    g->players[0].health = NAN;
    CHECK(!game_snapshot_encode(g, &data, &size));
    g->players[0].health = 100;
    Vec3 position = g->players[0].pos.local;
    g->players[0].pos.local.x = 1e12;
    CHECK(!game_snapshot_encode(g, &data, &size));
    g->players[0].pos.local = position;
    g->factions.forces[0].id = 1;
    CHECK(!sc_snapshot_valid(g));
    g->factions.forces[0].country = 1;
    CHECK(!sc_snapshot_valid(g));
    g->factions.forces[0].count = SC_FORCE_MEMBERS;
    CHECK(sc_snapshot_valid(g));
    g->factions.forces[0] = (FactionForce){0};
    g->player_count = SC_PLAYERS + 1;
    CHECK(!game_snapshot_encode(g, &data, &size));
    g->player_count = 1;
    g->world.record_count = SC_SNAPSHOT_RECORDS + 1;
    CHECK(!game_snapshot_encode(g, &data, &size));
    g->world.record_count = 0;
    g->world.chunk_count = 1;
    CHECK(!game_snapshot_encode(g, &data, &size));
    g->world.chunk_count = 0;
    g->world.chunks[0].box_count = 1;
    CHECK(!game_snapshot_encode(g, &data, &size));
    g->world.chunks[0].box_count = 0;
    Session s;
    session_init(&s, g);
    for (int i = 0; i < 20; i++)
        session_tick(&s);
    CHECK(sc_snapshot_valid(g));
    Room *r = &g->world.rooms[0];
    CHECK(world_mark_local(&g->world, r, LOCAL_RESOLVED));
    size_t record = r->record;
    r->record = g->world.record_count;
    CHECK(!sc_snapshot_valid(g));
    r->record = record;
    uint64_t residency = r->residency;
    r->residency = 0;
    CHECK(!sc_snapshot_valid(g));
    r->residency = residency;
    if (g->world.count > 1) {
        Room prior = g->world.rooms[1];
        g->world.rooms[1] = *r;
        CHECK(!sc_snapshot_valid(g));
        g->world.rooms[1] = prior;
    }
    g->matrix.divers[0].active = true;
    CHECK(!sc_snapshot_valid(g));
    g->matrix.divers[0].active = false;
    g->matrix.networks[0].active = true;
    CHECK(!sc_snapshot_valid(g));
    g->matrix.networks[0].active = false;
    uint32_t killed = g->world.records[0].killed;
    g->world.records[0].killed = UINT32_MAX;
    CHECK(!sc_snapshot_valid(g));
    g->world.records[0].killed = killed;
    CHECK(sc_snapshot_valid(g));
    destroy(g);
}
/* Locate a scalar by two valid exports, then corrupt its wire value and checksum.
   A rejected, checksum-valid packet must not replace the running expedition. */
static void reject_identifier(Game *g, uint32_t *field) {
    unsigned char *a, *b;
    size_t na, nb;
    uint32_t before = *field;
    *field = 1;
    CHECK(game_snapshot_encode(g, &a, &na));
    *field = 2;
    CHECK(game_snapshot_encode(g, &b, &nb));
    CHECK(na == nb);
    size_t offset = 16;
    while (offset < na - 4 && a[offset] == b[offset])
        offset++;
    CHECK(offset + 4 < na - 4);
    memset(a + offset, 255, 4);
    update_hash(a, na);
    CHECK(!game_snapshot_decode(g, a, na));
    unsigned char *after;
    size_t count;
    CHECK(game_snapshot_encode(g, &after, &count));
    CHECK(count == nb && !memcmp(after, b, count));
    free(after);
    free(a);
    free(b);
    *field = before;
}
static void malformed_identifiers(void) {
    Game *g = fresh();
    reject_identifier(g, &g->story.choice);
    reject_identifier(g, &g->story.chapter_choice);
    reject_identifier(g, &g->story.arc);
    reject_identifier(g, &g->politics.countries[0].stage);
    reject_identifier(g, &g->matrix.networks[0].theme);
    FactionRegion *region = factions_record(g, (Cell){0});
    CHECK(region);
    reject_identifier(g, &region->kind);
    destroy(g);
}
#ifdef SC_FAULT_TEST
static void allocation_failures(void) {
    Game *g = fresh(), *target = fresh();
    g->campaign.version = 0;
    g->players[0].pos = position_from(matrix_terminal((Cell){0}));
    Session s;
    session_init(&s, g);
    for (int i = 0; i < 40; i++)
        session_tick(&s);
    CHECK(matrix_toggle(g, 0));
    CHECK(factions_record(g, (Cell){0}));
    CHECK(world_mark_local(&g->world, &g->world.rooms[0], LOCAL_RESOLVED));
    unsigned char *data;
    size_t size;
    sc_fail_allocation = 0;
    CHECK(!game_snapshot_encode(g, &data, &size) && !data && !size);
    sc_fail_allocation = -1;
    CHECK(game_snapshot_encode(g, &data, &size));
    unsigned successes = 0;
    for (int failure = 0; failure < SC_MAX_CHUNKS + 10; failure++) {
        sc_fail_allocation = failure;
        bool ok = game_snapshot_decode(target, data, size);
        sc_fail_allocation = -1;
        if (ok) {
            successes++;
            break;
        }
        CHECK(target->time == 0 && !target->matrix.divers[0].active);
    }
    CHECK(successes == 1);
    equal_state(g, target);
    free(data);
    destroy(g);
    destroy(target);
}
#endif
int main(void) {
    roundtrip();
    validation();
    malformed_identifiers();
#ifdef SC_FAULT_TEST
    allocation_failures();
#endif
    puts("PASS: portable live snapshots");
    return 0;
}
