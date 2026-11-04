#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define make_dir(p) _mkdir(p)
#define remove_dir(p) _rmdir(p)
#else
#include <sys/stat.h>
#include <unistd.h>
#define make_dir(p) mkdir(p, 0700)
#define remove_dir(p) rmdir(p)
#endif
extern int sc_fail_allocation, sc_fail_flush, sc_fail_close;
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        checks++;                                                                                  \
        if (!(x)) {                                                                                \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                           \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static void settle(Game *g) {
    Vec3 pos = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &pos, 1));
    for (int i = 0; g->world.pending && i < SC_MAX_CHUNKS; i++)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed && !g->world.pending);
}
static Game *quiet(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    g->players[0].pos = position_from(vadd(world_center((Cell){1, 0, 0}), v3(0, .02, -24)));
    settle(g);
    for (int r = 0; r < g->world.count; r++) {
        g->world.rooms[r].enemy_count = 0;
        g->world.rooms[r].pickup_count = 0;
    }
    return g;
}
static void destroy(Game *g) {
    sc_fail_allocation = -1;
    sc_fail_flush = sc_fail_close = 0;
    game_destroy(g);
    free(g);
}
static void tick(Game *g, Actions a, int count) {
    Actions all[2] = {a, {0}};
    for (int i = 0; i < count; i++)
        game_step(g, all, 1.0 / 60);
}
static Enemy *enemy(Game *g, int kind, Vec3 position) {
    Room *r = world_find(&g->world, (Cell){1, 0, 0});
    CHECK(r && r->enemy_count < SC_MAX_ENEMIES);
    int id = r->enemy_count++;
    r->enemies[id] = (Enemy){.pos = position_from(position),
                             .health = 50,
                             .normal = {0, 1, 0},
                             .heading = {0, 0, -1},
                             .alive = true,
                             .kind = (uint8_t)kind,
                             .id = (uint8_t)id};
    return &r->enemies[id];
}
static void test_controls_and_resources(void) {
    Game *g = quiet();
    Player *p = &g->players[0];
    CHECK(!game_upgrade(g, -1, 0) && !game_upgrade(g, 0, -1) && !game_upgrade(g, 0, SC_ATTRIBUTES));
    p->power = 50;
    p->points = 1;
    CHECK(!game_upgrade(g, 0, 0) && p->points == 1);
    p->power = 0;
    CHECK(player_attribute(p, -1) == NULL && player_attribute(p, SC_ATTRIBUTES) == NULL);
    game_weapon(g, -1, 1);
    game_weapon(g, SC_PLAYERS, 1);
    CHECK(p->weapon == 0);
    game_weapon(g, 0, -1);
    CHECK(p->weapon == SC_FLARE_CANNON);
    float hp = p->health;
    game_hurt(g, -1, 10, 0);
    game_hurt(g, 2, 10, 0);
    game_hurt(g, 0, NAN, 0);
    game_hurt(g, 0, 0, 0);
    CHECK(p->health == hp);
    game_join(g);
    Position joined = g->players[1].pos;
    game_join(g);
    CHECK(g->player_count == 2 && vlen(position_delta(joined, g->players[1].pos)) == 0);
    game_leave(g);
    double time = g->time;
    Actions none[2] = {{0}, {0}};
    game_step(g, none, 0);
    game_step(g, none, -1);
    game_step(g, none, .1);
    game_step(g, none, NAN);
    CHECK(g->time == time);
    p->weapon = 1;
    p->energy = 0;
    p->cooldown = 0;
    tick(g, (Actions){.fire = true}, 1);
    CHECK(g->shots == 0 && strstr(g->message, "ENERGY"));
    p->weapon = 4;
    p->energy = player_max_energy(p);
    p->cooldown = 0;
    for (int i = 0; i < SC_MAX_PROJECTILES; i++)
        g->projectiles[i] = (Projectile){.pos = p->pos, .life = 2, .active = true, .owner = 0};
    tick(g, (Actions){.fire = true}, 1);
    CHECK(g->shots == 0 && p->energy == player_max_energy(p));
    memset(g->projectiles, 0, sizeof(g->projectiles));
    /* Supply effects, useful-only consumption, and duplicate weapon salvage. */
    Room *r = world_find(&g->world, (Cell){1, 0, 0});
    r->pickup_count = 1;
    for (int kind = 0; kind <= PICK_SALVAGE; kind++) {
        p->health = 10;
        p->shield = 1;
        p->armor = 1;
        p->energy = 1;
        p->hurt = 10;
        r->pickups[0] = (Pickup){.pos = p->pos, .kind = (uint8_t)kind, .item = 0, .active = true};
        unsigned credits = p->credits, items = p->pickups;
        game_progression_step(g, .01);
        CHECK(!r->pickups[0].active && p->pickups == items + 1);
        if (kind == PICK_MEDKIT)
            CHECK(p->health == 65);
        if (kind == PICK_SHIELD)
            CHECK(p->shield == player_max_shield(p));
        if (kind == PICK_ARMOR)
            CHECK(p->armor == player_max_armor(p));
        if (kind == PICK_ENERGY)
            CHECK(p->energy == player_max_energy(p));
        if (kind == PICK_WEAPON)
            CHECK(p->credits == credits + 30);
        if (kind == PICK_SALVAGE)
            CHECK(p->credits == credits + 25);
    }
    p->health = player_max_health(p);
    r->pickups[0] = (Pickup){.pos = p->pos, .kind = PICK_MEDKIT, .active = true};
    game_progression_step(g, .01);
    CHECK(r->pickups[0].active);
    g->story = (Story){0};
    g->mission = 6;
    game_interact(g, 0);
    CHECK(strstr(g->message, "BEACON LIT"));
    g->mission = 0;
    game_interact(g, 0);
    CHECK(g->mission == 0 && strstr(g->message, "FOLLOW"));
    g->mission = 1;
    p->pos = position_from(game_terminal((Cell){1, 0, 0}));
    Enemy *e = enemy(g, 0, vadd(position_absolute(p->pos), v3(0, 0, 2)));
    game_interact(g, 0);
    CHECK(g->mission == 1 && strstr(g->message, "LOCKED"));
    e->alive = false;
    game_interact(g, 0);
    CHECK(g->mission == 2);
    destroy(g);
}
static void test_combat_and_movement(void) {
    Game *g = quiet();
    Player *p = &g->players[0];
    Vec3 at = position_absolute(p->pos);
    p->shield = 0;
    p->armor = 0;
    p->hurt = 10;
    g->projectiles[0] = (Projectile){.pos = position_from(vadd(at, v3(.4, .87, 0))),
                                     .velocity = {0, 0, 20},
                                     .life = 1,
                                     .damage = 12,
                                     .active = true,
                                     .owner = -1};
    tick(g, (Actions){0}, 1);
    CHECK(g->projectile_hits == 1 && p->health == 88);
    g->projectiles[0] = (Projectile){.pos = position_from(vadd(at, v3(0, 1.4, 0))),
                                     .velocity = {0, 0, 20},
                                     .life = 1,
                                     .damage = 12,
                                     .active = true,
                                     .owner = -1};
    tick(g, (Actions){0}, 1);
    CHECK(g->projectile_hits == 2 && p->health == 76);
    Enemy *e = enemy(g, 1, vadd(at, v3(0, 0, 5)));
    e->cooldown = 100;
    g->projectiles[0] = (Projectile){.pos = position_from(vadd(enemy_center(e), v3(0, 0, -2))),
                                     .velocity = {0, 0, 100},
                                     .life = 1,
                                     .damage = 12,
                                     .active = true,
                                     .owner = 0};
    tick(g, (Actions){0}, 3);
    CHECK(e->health == 38);
    e->alive = false;
    g->projectiles[0] = (Projectile){.pos = position_from(vadd(at, v3(0, 1, 0))),
                                     .velocity = {0, 0, 100},
                                     .life = 1,
                                     .damage = 12,
                                     .active = true,
                                     .owner = 0};
    tick(g, (Actions){0}, 1);
    CHECK(g->projectiles[0].active);
    destroy(g);
    g = quiet();
    p = &g->players[0];
    at = position_absolute(p->pos);
    Enemy *healer = enemy(g, 10, vadd(at, v3(0, 0, 10)));
    healer->special = 0;
    Enemy *ally = enemy(g, 2, vadd(at, v3(2, 0, 11)));
    ally->health = 10;
    tick(g, (Actions){0}, 1);
    CHECK(ally->health == 28 && healer->special > 2);
    destroy(g);
    g = quiet();
    p = &g->players[0];
    at = position_absolute(p->pos);
    p->shield = 0;
    p->armor = 0;
    p->hurt = 10;
    e = enemy(g, 13, vadd(at, v3(0, 0, 2)));
    tick(g, (Actions){0}, 1);
    CHECK(!e->alive && p->health < 100 && p->kills == 1);
    destroy(g);
    g = quiet();
    p = &g->players[0];
    at = position_absolute(p->pos);
    p->shield = 0;
    p->armor = 0;
    p->hurt = 10;
    e = enemy(g, 2, vadd(at, v3(0, 0, 1.9)));
    tick(g, (Actions){0}, 1);
    CHECK(p->health == 87 && e->cooldown > 0);
    destroy(g);
    g = quiet();
    p = &g->players[0];
    at = position_absolute(p->pos);
    e = enemy(g, 1, vadd(at, v3(0, 0, 5)));
    e->cooldown = 10;
    Position before = e->pos;
    tick(g, (Actions){0}, 6);
    CHECK(position_delta(e->pos, before).z > 0);
    destroy(g);
    g = quiet();
    p = &g->players[0];
    at = position_absolute(p->pos);
    p->grounded = true;
    tick(g, (Actions){.jump = true, .forward = 1, .strafe = 1}, 1);
    CHECK(p->pos.local.y > .1 && vlen(p->velocity) > 7.5);
    /* Upward collision in a low conduit cancels vertical velocity. */
    p->pos = position_from(vadd(world_center((Cell){0, -1, 0}), v3(0, .54, 0)));
    settle(g);
    p->velocity = v3(0, 10, 0);
    p->grounded = false;
    tick(g, (Actions){.crouch = true}, 1);
    CHECK(p->velocity.y == 0);
    destroy(g);
    g = malloc(sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    g->players[0].pos = position_from(v3(0, .02, 12));
    before = g->players[0].pos;
    tick(g, (Actions){.forward = 1}, 1);
    CHECK(vlen(position_delta(g->players[0].pos, before)) == 0);
    settle(g);
    p = &g->players[0];
    p->pos = position_from(world_lift((Cell){0, 0, 0}));
    Room *upper = world_find(&g->world, (Cell){0, 1, 0});
    CHECK(upper);
    upper->ready = false;
    tick(g, (Actions){.lift_up = true}, 1);
    CHECK(world_cell(position_absolute(p->pos)).y == 0 && strstr(g->message, "CHARGING"));
    upper->ready = true;
    g->world.failed = true;
    double time = g->time;
    tick(g, (Actions){0}, 1);
    CHECK(g->time > time);
    destroy(g);
}
static void test_allocation_and_cache_failures(void) {
    for (int nth = 0; nth < 2; nth++) {
        World *w = malloc(sizeof(*w));
        CHECK(w);
        world_init(w, 713);
        Vec3 pos = v3(0, 0, 20);
        CHECK(world_refresh(w, &pos, 1));
        sc_fail_allocation = nth;
        world_stream(w, 1);
        sc_fail_allocation = -1;
        CHECK(w->failed);
        world_destroy(w);
        free(w);
    }
    World *w = malloc(sizeof(*w));
    CHECK(w);
    world_init(w, 713);
    CHECK(world_stream(w, 2) == 0 && !world_refresh(w, NULL, 0) && !world_refresh(w, NULL, 3));
    Room r = {.cell = {1, 0, 0}, .enemy_count = 1, .record = SIZE_MAX};
    world_kill(w, &r, -1);
    world_kill(w, &r, 1);
    CHECK(w->record_count == 0);
    sc_fail_allocation = 0;
    world_kill(w, &r, 0);
    CHECK(w->failed && r.record == SIZE_MAX);
    world_destroy(w);
    world_init(w, 713);
    CHECK(world_index_records(w));
    sc_fail_allocation = 0;
    world_kill(w, &r, 0);
    CHECK(w->failed && r.record == SIZE_MAX);
    world_destroy(w);
    world_init(w, 713);
    for (int i = 0; i < 300; i++) {
        r.cell = (Cell){i, 0, 1};
        r.record = SIZE_MAX;
        world_kill(w, &r, 0);
        CHECK(!w->failed);
    }
    CHECK(w->record_count == 300 && w->index_cap >= 1024);
    for (int i = 0; i < 300; i++) {
        r.cell = (Cell){i, 0, 1};
        r.record = SIZE_MAX;
        world_kill(w, &r, 0);
        CHECK(w->record_count == 300 && cell_equal(w->records[r.record].cell, r.cell));
    }
    w->records[1].cell = w->records[0].cell;
    CHECK(!world_index_records(w));
    world_destroy(w);
    world_init(w, 713);
    for (int i = 0; i < 127; i++) {
        r.cell = (Cell){i, 0, 1};
        r.record = SIZE_MAX;
        world_kill(w, &r, 0);
    }
    r.cell = (Cell){500, 0, 1};
    r.record = SIZE_MAX;
    sc_fail_allocation = 0;
    world_kill(w, &r, 0);
    CHECK(w->failed);
    world_destroy(w);
    world_init(w, 713);
    Vec3 pos = v3(0, 0, 20);
    CHECK(world_refresh(w, &pos, 1));
    w->count = SC_MAX_ROOMS;
    for (int i = 0; i < SC_MAX_ROOMS; i++)
        w->rooms[i].cell = (Cell){1000 + i, 0, 1000};
    world_stream(w, 1);
    CHECK(w->failed);
    world_destroy(w);
    free(w);
    Game *g = quiet();
    Room *room = world_find(&g->world, (Cell){1, 0, 0});
    room->pickup_count = 1;
    room->pickups[0] = (Pickup){.pos = g->players[0].pos, .kind = PICK_SALVAGE, .active = true};
    world_collect(&g->world, room, -1);
    world_collect(&g->world, room, 1);
    CHECK(room->pickups[0].active);
    sc_fail_allocation = 0;
    game_progression_step(g, .01);
    CHECK(g->world.failed && room->pickups[0].active && g->players[0].pickups == 0);
    destroy(g);
    CHECK(strcmp(world_room_name((RoomType)-1), "UNKNOWN") == 0);
    CHECK(strcmp(world_district_name((District)DISTRICT_TYPES), "UNSURVEYED") == 0);
    World *empty = calloc(1, sizeof(*empty));
    CHECK(empty);
    CHECK(world_overlap(empty, v3(43, 2, -2), .34, 1.78));
    free(empty);
}
static unsigned char *read_file(const char *path, size_t *length) {
    FILE *f = fopen(path, "rb");
    CHECK(f);
    CHECK(fseek(f, 0, SEEK_END) == 0);
    long size = ftell(f);
    CHECK(size > 4);
    *length = (size_t)size;
    rewind(f);
    unsigned char *data = malloc(*length);
    CHECK(data);
    CHECK(fread(data, 1, *length, f) == *length);
    fclose(f);
    return data;
}
static void put32(unsigned char *p, uint32_t value) {
    for (int i = 0; i < 4; i++)
        p[i] = (unsigned char)(value >> (8 * i));
}
static void put_real(unsigned char *p, double value) {
    uint64_t bits;
    memcpy(&bits, &value, 8);
    for (int i = 0; i < 8; i++)
        p[i] = (unsigned char)(bits >> (8 * i));
}
static void write_payload(const char *path, unsigned char *data, size_t len) {
    uint32_t sum = 2166136261u;
    for (size_t i = 0; i < len - 4; i++) {
        sum ^= data[i];
        sum *= 16777619u;
    }
    put32(data + len - 4, sum);
    FILE *f = fopen(path, "wb");
    CHECK(f);
    CHECK(fwrite(data, 1, len, f) == len);
    CHECK(fclose(f) == 0);
}
static bool rejected_unchanged(Game *g, const char *path) {
    Game *before = malloc(sizeof(*before));
    CHECK(before);
    memcpy(before, g, sizeof(*g));
    size_t bytes = g->world.record_count * sizeof(RoomRecord);
    RoomRecord *records = bytes ? malloc(bytes) : NULL;
    if (bytes) {
        CHECK(records);
        memcpy(records, g->world.records, bytes);
    }
    bool loaded = game_load(g, path);
    CHECK(!loaded && memcmp(before, g, sizeof(*g)) == 0);
    if (bytes)
        CHECK(memcmp(records, g->world.records, bytes) == 0);
    free(records);
    free(before);
    return true;
}
static void test_save_failure_safety(void) {
    const char *valid = "sc-edge-valid.sav", *bad = "sc-edge-invalid.sav";
    Game *g = quiet();
    g->players[0].level = 7;
    Room *room = world_find(&g->world, (Cell){1, 0, 0});
    room->enemy_count = 1;
    world_kill(&g->world, room, 0);
    CHECK(game_save(g, valid));
    CHECK(!game_save(g, NULL));
    CHECK(!game_load(g, "sc-missing-file.sav"));
    size_t count = g->world.record_count;
    g->world.record_count = 1000001;
    CHECK(!game_save(g, bad));
    g->world.record_count = count;
    sc_fail_allocation = 0;
    CHECK(!game_save(g, valid));
    CHECK(game_load(g, valid));
    sc_fail_flush = 1;
    CHECK(!game_save(g, valid));
    CHECK(game_load(g, valid));
    sc_fail_close = 1;
    CHECK(!game_save(g, valid));
    CHECK(game_load(g, valid));
    CHECK(make_dir("sc-edge-directory") == 0);
    CHECK(!game_save(g, "sc-edge-directory"));
    CHECK(remove_dir("sc-edge-directory") == 0);
    for (int nth = 0; nth < 3; nth++) {
        sc_fail_allocation = nth;
        CHECK(rejected_unchanged(g, valid));
        sc_fail_allocation = -1;
        CHECK(g->world.seed == 713 && g->players[0].level == 7);
    }
    CHECK(game_load(g, valid));
    size_t length;
    unsigned char *base = read_file(valid, &length), *data = malloc(length);
    CHECK(data);
    /* Mutate format-3 wire fields and recompute the checksum so validation,
       rather than only checksum rejection, must preserve the active session. */
    struct {
        size_t offset;
        uint32_t value;
    } invalid_ints[] = {
        {4, 99},         {12, 99},          {104, 0},       {144, 51},
        {172, 0},        {176, SC_WEAPONS}, {184, 4096},    {424, 7},
        {436, 8},        {440, 1000001},    {444 + 16, 32}, {444 + 12, 1u << SC_MAX_ENEMIES},
        {444, 10000001}, {464, 2},          {512, 2},       {512, 1},
        {572, 2},        {632, 2},          {668, 2},       {704, 3},
        {708, 3},        {712, 6},          {716, 6},       {764, 2},
        {704, 1},        {960, 33}};
    for (size_t i = 0; i < sizeof(invalid_ints) / sizeof(invalid_ints[0]); i++) {
        memcpy(data, base, length);
        CHECK(invalid_ints[i].offset + 4 <= length - 4);
        put32(data + invalid_ints[i].offset, invalid_ints[i].value);
        write_payload(bad, data, length);
        CHECK(rejected_unchanged(g, bad));
        CHECK(g->world.seed == 713 && g->players[0].level == 7);
    }
    struct {
        size_t offset;
        double value;
    } invalid_reals[] = {{28, NAN},  {28, 48},   {88, NAN}, {136, -1},  {196, NAN}, {204, 100000},
                         {212, -1},  {504, NAN}, {504, 4},  {516, NAN}, {516, 9},   {524, -.1},
                         {532, 8},   {540, NAN}, {540, 4},  {548, NAN}, {548, 2},   {556, NAN},
                         {556, -1},  {556, 7},   {556, 1},  {564, NAN}, {564, -1},  {564, 121},
                         {636, NAN}, {636, -1},  {636, 19}, {644, NAN}, {644, -1},  {644, 46},
                         {652, NAN}, {652, -1},  {652, 9},  {660, NAN}, {660, -1},  {660, .6},
                         {660, .25}, {756, NAN}, {756, -1}, {756, 601}, {768, NAN}, {768, 4}};
    for (size_t i = 0; i < sizeof(invalid_reals) / sizeof(invalid_reals[0]); i++) {
        memcpy(data, base, length);
        put_real(data + invalid_reals[i].offset, invalid_reals[i].value);
        write_payload(bad, data, length);
        CHECK(rejected_unchanged(g, bad));
        CHECK(g->world.seed == 713);
    }
    /* A truncated checksum and a duplicate record are both transactional. */
    memcpy(data, base, length);
    write_payload(bad, data, length - 1);
    CHECK(rejected_unchanged(g, bad));
    unsigned char *duplicate = malloc(length + 20);
    CHECK(duplicate);
    memcpy(duplicate, base, 464);
    memcpy(duplicate + 464, base + 444, 20);
    memcpy(duplicate + 484, base + 464, length - 464);
    put32(duplicate + 440, 2);
    write_payload(bad, duplicate, length + 20);
    CHECK(rejected_unchanged(g, bad));
    free(duplicate);
    free(base);
    free(data);
    /* Validate each attached-light field with a fresh correct checksum. */
    settle(g);
    game_attach_flare(g, 0, v3(96, 0, 24), v3(0, -1, 0));
    CHECK(g->flares[0].active && game_save(g, valid));
    base = read_file(valid, &length);
    data = malloc(length);
    CHECK(data);
    struct {
        size_t offset;
        double value;
    } bad_lights[] = {{1000, NAN}, {1008, 0},  {1024, NAN}, {1024, 0}, {1024, 106}, {1032, NAN},
                      {1032, 11},  {1032, 23}, {1040, NAN}, {1040, 2}, {1040, 7}};
    for (size_t i = 0; i < sizeof(bad_lights) / sizeof(bad_lights[0]); i++) {
        memcpy(data, base, length);
        put_real(data + bad_lights[i].offset, bad_lights[i].value);
        write_payload(bad, data, length);
        CHECK(rejected_unchanged(g, bad));
    }
    memcpy(data, base, length);
    put32(data + 1048, 2);
    write_payload(bad, data, length);
    CHECK(rejected_unchanged(g, bad));
    free(data);
    free(base);
    base = read_file(SC_FIXTURE_DIR "/generator-v1.sav", &length);
    put_real(base + 12, NAN);
    write_payload(bad, base, length);
    CHECK(rejected_unchanged(g, bad) && g->players[0].level == 7);
    free(base);
    remove(valid);
    remove(bad);
    destroy(g);
}
int main(void) {
    test_controls_and_resources();
    test_combat_and_movement();
    test_allocation_and_cache_failures();
    test_save_failure_safety();
    printf("PASS %u edge-case assertions: resource rules, combat, invalid input, allocation/I/O "
           "failures, save validation and rollback\n",
           checks);
    return 0;
}
