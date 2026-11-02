#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* A versioned little-endian stream, independent of C struct layout and platform. */
typedef struct {
    FILE *file;
    uint32_t hash;
    bool ok;
} Stream;
static void bytes(Stream *s, void *data, size_t n, bool read) {
    if (!s->ok)
        return;
    size_t done = read ? fread(data, 1, n, s->file) : fwrite(data, 1, n, s->file);
    if (done != n) {
        s->ok = false;
        return;
    }
    const unsigned char *b = data;
    for (size_t i = 0; i < n; i++) {
        s->hash ^= b[i];
        s->hash *= UINT32_C(16777619);
    }
}
static uint32_t number(Stream *s, uint32_t n, bool read) {
    unsigned char b[4];
    for (int i = 0; i < 4; i++)
        b[i] = (unsigned char)(n >> (i * 8));
    bytes(s, b, 4, read);
    n = 0;
    for (int i = 0; i < 4; i++)
        n |= (uint32_t)b[i] << (i * 8);
    return n;
}
static double real(Stream *s, double d, bool read) {
    uint64_t bits = 0;
    memcpy(&bits, &d, 8);
    uint32_t lo = number(s, (uint32_t)bits, read), hi = number(s, (uint32_t)(bits >> 32), read);
    bits = (uint64_t)lo | ((uint64_t)hi << 32);
    memcpy(&d, &bits, 8);
    return d;
}
static Vec3 vector(Stream *s, Vec3 v, bool read) {
    v.x = real(s, v.x, read);
    v.y = real(s, v.y, read);
    v.z = real(s, v.z, read);
    return v;
}
static bool valid_vector(Vec3 v) {
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z) && fabs(v.x) < 9e8 && fabs(v.y) < 3e8 &&
           fabs(v.z) < 9e8;
}
static Position location(Stream *s, Position p, bool read, bool legacy) {
    if (legacy) {
        Vec3 v = vector(s, position_absolute(p), read);
        if (!valid_vector(v)) {
            s->ok = false;
            return (Position){0};
        }
        return position_from(v);
    }
    p.sector.x = (int32_t)number(s, (uint32_t)p.sector.x, read);
    p.sector.y = (int32_t)number(s, (uint32_t)p.sector.y, read);
    p.sector.z = (int32_t)number(s, (uint32_t)p.sector.z, read);
    p.local = vector(s, p.local, read);
    if (!valid_vector(p.local) || p.local.x < -48 || p.local.x >= 48 || p.local.y < 0 ||
        p.local.y >= SC_DECK || p.local.z < -48 || p.local.z >= 48 ||
        !valid_vector(position_absolute(p)))
        s->ok = false;
    return p;
}
static void profile(Stream *s, Player *p, bool read, bool legacy) {
    p->pos = location(s, p->pos, read, legacy);
    p->checkpoint = location(s, p->checkpoint, read, legacy);
    p->yaw = real(s, p->yaw, read);
    p->pitch = real(s, p->pitch, read);
    p->level = number(s, p->level, read);
    p->xp = number(s, p->xp, read);
    p->kills = number(s, p->kills, read);
    p->points = number(s, p->points, read);
    p->power = number(s, p->power, read);
    p->vitality = number(s, p->vitality, read);
    p->haste = number(s, p->haste, read);
    p->tint = number(s, p->tint, read);
    p->health = (float)real(s, p->health, read);
    if (read) {
        if (!valid_vector(position_absolute(p->pos)) ||
            !valid_vector(position_absolute(p->checkpoint)) || !isfinite(p->yaw) ||
            !isfinite(p->pitch) || p->level < 1 || p->level > 1000 || p->xp > 100000 ||
            p->points > 1000 || p->power > 50 || p->vitality > 50 || p->haste > 50 ||
            !isfinite(p->health) || p->health <= 0 || p->health > player_max_health(p) ||
            fabs(p->pitch) > 1.5 || fabs(p->yaw) > SC_PI + 1e-6)
            s->ok = false;
        p->crouched = true;
        p->flashlight = true;
    }
}
bool game_save(const Game *g, const char *path) {
    if (!path || g->world.record_count > 1000000)
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
    number(&s, UINT32_C(0x5343524c), false);
    number(&s, 2, false);
    number(&s, g->world.seed, false);
    number(&s, g->world.generator, false);
    /* A saved session always opens in solo; the other profile remains available. */
    for (int i = 0; i < SC_PLAYERS; i++) {
        Player p = g->players[i];
        profile(&s, &p, false, false);
    }
    number(&s, (uint32_t)g->world.record_count, false);
    for (size_t i = 0; i < g->world.record_count; i++) {
        const RoomRecord *r = &g->world.records[i];
        number(&s, (uint32_t)r->cell.x, false);
        number(&s, (uint32_t)r->cell.y, false);
        number(&s, (uint32_t)r->cell.z, false);
        number(&s, r->killed, false);
    }
    uint32_t hash = s.hash;
    number(&s, hash, false);
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
bool game_load(Game *g, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    Stream s = {f, UINT32_C(2166136261), true};
    uint32_t magic = number(&s, 0, true), version = number(&s, 0, true), seed = number(&s, 0, true);
    uint32_t generator = version == 2 ? number(&s, 0, true) : 1;
    if (magic != UINT32_C(0x5343524c) || (version != 1 && version != 2) ||
        (version == 2 && generator != SC_GENERATOR_VERSION))
        s.ok = false;
    Player players[SC_PLAYERS];
    memset(players, 0, sizeof(players));
    for (int i = 0; i < SC_PLAYERS; i++)
        profile(&s, &players[i], true, version == 1);
    uint32_t count = number(&s, 0, true);
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
        r->cell.x = (int32_t)number(&s, 0, true);
        r->cell.y = (int32_t)number(&s, 0, true);
        r->cell.z = (int32_t)number(&s, 0, true);
        r->killed = number(&s, 0, true);
        if (r->cell.x < -10000000 || r->cell.x > 10000000 || r->cell.y < -10000000 ||
            r->cell.y > 10000000 || r->cell.z < 0 || r->cell.z > 10000000 ||
            r->killed >= (UINT32_C(1) << SC_MAX_ENEMIES))
            s.ok = false;
    }
    uint32_t expected = s.hash, actual = number(&s, 0, true);
    if (actual != expected || fgetc(f) != EOF)
        s.ok = false;
    fclose(f);
    if (!s.ok) {
        free(records);
        return false;
    }
    if (version == 1) {
        /* Incompatible geometry must not reinterpret old doors, kills or positions.
           Keep both profiles and visibly begin a new expedition instead. */
        free(records);
        records = NULL;
        count = 0;
        seed = world_hash(seed, (Cell){0, 0, 0}, UINT32_C(0x4d494752));
        for (int i = 0; i < SC_PLAYERS; i++) {
            players[i].pos = position_from(v3(i * 1.4, 4, -34));
            players[i].checkpoint = position_from(v3(0, .02, 12));
            players[i].health = player_max_health(&players[i]);
            players[i].yaw = 0;
            players[i].pitch = -.04;
        }
    }
    World *replacement = malloc(sizeof(*replacement));
    if (!replacement) {
        free(records);
        return false;
    }
    world_init(replacement, seed);
    replacement->records = records;
    replacement->record_count = count;
    replacement->record_cap = count;
    Vec3 start = position_absolute(players[0].pos);
    if (!world_index_records(replacement) || !world_refresh(replacement, &start, 1)) {
        world_destroy(replacement);
        free(replacement);
        return false;
    }
    world_destroy(&g->world);
    g->world = *replacement;
    free(replacement);
    memcpy(g->players, players, sizeof(players));
    g->player_count = 1;
    memset(g->projectiles, 0, sizeof(g->projectiles));
    g->dirty = version == 1;
    game_message(g, version == 1 ? "SHIP GENERATOR UPDATED / CHARACTERS RETAINED / NEW EXPEDITION"
                                 : "EXPEDITION RESUMED / WELCOME BACK");
    return true;
}
