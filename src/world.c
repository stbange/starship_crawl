#include "world.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

uint32_t world_hash(uint32_t seed, Cell c, uint32_t salt) {
    uint32_t h = seed ^ salt;
    h ^= (uint32_t)c.x * UINT32_C(0x9e3779b9);
    h ^= (uint32_t)c.y * UINT32_C(0x85ebca6b);
    h ^= (uint32_t)c.z * UINT32_C(0xc2b2ae35);
    h ^= h >> 16;
    h *= UINT32_C(0x7feb352d);
    h ^= h >> 15;
    h *= UINT32_C(0x846ca68b);
    return h ^ (h >> 16);
}
bool cell_equal(Cell a, Cell b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
Cell world_cell(Vec3 p) {
    Cell c = {(int32_t)clampd(floor((p.x + 48) / SC_CELL), -10000000, 10000000),
              (int32_t)clampd(floor((p.y + 0.02) / SC_DECK), -10000000, 10000000),
              (int32_t)clampd(floor(p.z / SC_CELL), 0, 10000000)};
    return c;
}
Vec3 world_center(Cell c) {
    return v3(c.x * SC_CELL, c.y * SC_DECK, c.z * SC_CELL + 48);
}
Vec3 world_lift(Cell c) {
    Vec3 p = world_center(c);
    p.z -= 10;
    p.y += 0.02;
    return p;
}
unsigned world_difficulty(Cell c) {
    double d = sqrt((double)c.x * c.x + (double)c.y * c.y / 9 + (double)c.z * c.z);
    if (c.x == 0 && c.y == 0 && c.z == 0)
        return 0;
    return (unsigned)fmax(1, fmin(d * 1.5, 10000.0));
}
const char *world_room_name(RoomType t) {
    static const char *names[] = {"THE THRESHOLD",   "CATHEDRAL NAVE", "REACTOR CHOIR",
                                  "CREW SEPULCHRES", "THE UNDERCROFT", "GILDED ATRIUM"};
    return t < ROOM_TYPES ? names[t] : "UNKNOWN";
}
void world_init(World *w, uint32_t seed) {
    memset(w, 0, sizeof(*w));
    w->seed = seed;
}
void world_destroy(World *w) {
    free(w->records);
    free(w->boxes);
    memset(w, 0, sizeof(*w));
}

static size_t record_for(World *w, Cell c) {
    for (size_t i = 0; i < w->record_count; i++)
        if (cell_equal(w->records[i].cell, c))
            return i;
    if (w->record_count == w->record_cap) {
        size_t cap = w->record_cap ? w->record_cap * 2 : 128;
        RoomRecord *p = realloc(w->records, cap * sizeof(*p));
        if (!p) {
            w->failed = true;
            return SIZE_MAX;
        }
        w->records = p;
        w->record_cap = cap;
    }
    size_t i = w->record_count++;
    w->records[i] = (RoomRecord){c, 0};
    return i;
}
static void room_init(World *w, Room *r, Cell c) {
    memset(r, 0, sizeof(*r));
    r->cell = c;
    r->hash = world_hash(w->seed, c, 123);
    r->record = record_for(w, c);
    if (r->record == SIZE_MAX)
        return;
    r->type = (RoomType)(1 + r->hash % (ROOM_TYPES - 1));
    if (c.x == 0 && c.y == 0 && c.z == 0)
        r->type = BAY;
    if (c.x == 0 && c.y == 0 && c.z == 1)
        r->type = NAVE;
    if (c.x == -1 && c.y == 0 && c.z == 0)
        r->type = CREW;
    if (c.x == 1 && c.y == 0 && c.z == 0)
        r->type = ENGINE;
    if (c.x == 0 && c.y == 1 && c.z == 0)
        r->type = GARDEN;
    if (c.x == 0 && c.y == -1 && c.z == 0)
        r->type = CONDUIT;
    static const double dims[][3] = {{84, 92, 28}, {82, 84, 29},  {58, 68, 22},
                                     {40, 56, 8},  {7, 68, 1.55}, {72, 74, 24}};
    r->width = dims[r->type][0];
    r->depth = dims[r->type][1];
    r->height = dims[r->type][2];
    if (r->type != BAY && r->type != CONDUIT) {
        r->width -= r->hash % 4 * 2;
        r->depth -= (r->hash >> 4) % 4 * 2;
    }
    unsigned difficulty = world_difficulty(c);
    r->enemy_count = r->type == BAY ? 0 : (int)(5 + (difficulty > 12 ? 12 : difficulty));
    Vec3 center = world_center(c);
    for (int i = 0; i < r->enemy_count; i++) {
        uint32_t h = world_hash(w->seed, c, 1000 + (uint32_t)i * 31);
        Enemy *e = &r->enemies[i];
        e->id = (uint8_t)i;
        e->kind = (uint8_t)(h % 3);
        double x = ((h >> 4) % 1000 / 999.0 - .5) * (r->width - 5);
        double z = ((h >> 15) % 1000 / 999.0 - .5) * (r->depth - 8);
        e->pos = vadd(center, v3(x, .02, z));
        e->health = (float)(e->kind == 2 ? 65 : 26) + (float)difficulty * 4;
        e->alive = (w->records[r->record].killed & (UINT32_C(1) << i)) == 0;
        e->cooldown = (float)(h % 200) / 100.0f;
    }
}
static void box(World *w, Vec3 center, Vec3 size, Color color, float glow, bool solid) {
    if (size.x <= 0 || size.y <= 0 || size.z <= 0)
        return;
    if (w->box_count == w->box_cap) {
        size_t cap = w->box_cap ? w->box_cap * 2 : 2048;
        Box *b = realloc(w->boxes, cap * sizeof(*b));
        if (!b) {
            w->failed = true;
            return;
        }
        w->boxes = b;
        w->box_cap = cap;
    }
    w->boxes[w->box_count++] =
        (Box){vsub(center, vmul(size, .5)), vadd(center, vmul(size, .5)), color, glow, solid};
}
static void local_box(World *w, Vec3 c, double x, double y, double z, double sx, double sy,
                      double sz, Color col, float glow, bool solid) {
    box(w, vadd(c, v3(x, y, z)), v3(sx, sy, sz), col, glow, solid);
}
static void corridor(World *w, Vec3 center, bool along_x, double from, double to) {
    if (to - from < .01)
        return;
    double mid = (from + to) * .5, len = to - from;
    Color metal = rgb(.15f, .19f, .22f), edge = rgb(.12f, .23f, .25f);
    local_box(w, center, along_x ? mid : 0, -.3, along_x ? 0 : mid, along_x ? len : 7, .6,
              along_x ? 7 : len, metal, 0, true);
    local_box(w, center, along_x ? mid : 0, 5, along_x ? 0 : mid, along_x ? len : 8, .7,
              along_x ? 8 : len, metal, 0, true);
    for (int s = -1; s <= 1; s += 2) {
        local_box(w, center, along_x ? mid : s * 3.8, 2.3, along_x ? s * 3.8 : mid,
                  along_x ? len : .8, 5, along_x ? .8 : len, metal, 0, true);
        local_box(w, center, along_x ? mid : s * 3.32, .15, along_x ? s * 3.32 : mid,
                  along_x ? len : .07, .08, along_x ? .07 : len, edge, 1.7f, false);
    }
}
static void wall_with_door(World *w, Vec3 c, bool along_x, double at, double span, double height,
                           double door, double dh, Color color) {
    double side = (span - door) * .5;
    for (int s = -1; s <= 1; s += 2)
        local_box(w, c, along_x ? s * (door + side) * .5 : at, height * .5,
                  along_x ? at : s * (door + side) * .5, along_x ? side : 1, height,
                  along_x ? 1 : side, color, 0, true);
    local_box(w, c, along_x ? 0 : at, (height + dh) * .5, along_x ? at : 0, along_x ? door : 1,
              height - dh, along_x ? 1 : door, color, 0, true);
}
static void room_geometry(World *w, const Room *r) {
    Vec3 c = world_center(r->cell);
    double x = r->width * .5, z = r->depth * .5, h = r->height;
    Color stone = rgb(.17f, .20f, .23f), trim = rgb(.27f, .25f, .22f),
          floor = rgb(.13f, .17f, .19f);
    Color light = rgb(.18f, .75f, .67f);
    if (r->type == ENGINE) {
        stone = rgb(.20f, .16f, .15f);
        light = rgb(1, .34f, .10f);
    }
    if (r->type == GARDEN) {
        stone = rgb(.20f, .18f, .25f);
        trim = rgb(.38f, .29f, .15f);
        light = rgb(.65f, .46f, 1);
    }
    if (r->type == CONDUIT) {
        stone = rgb(.14f, .20f, .17f);
        light = rgb(.40f, .76f, .19f);
    }
    local_box(w, c, 0, -.4, 0, r->width, .8, r->depth, floor, 0, true);
    local_box(w, c, 0, h + .4, 0, r->width, .8, r->depth, stone, 0, true);
    wall_with_door(w, c, false, -x, r->depth, h, 7, fmin(h, 5), stone);
    wall_with_door(w, c, false, x, r->depth, h, 7, fmin(h, 5), stone);
    wall_with_door(w, c, true, z, r->width, h, 7, fmin(h, 5), stone);
    if (r->cell.z > 0 || r->type == BAY)
        wall_with_door(w, c, true, -z, r->width, h, r->type == BAY ? 48 : 7,
                       r->type == BAY ? 22 : fmin(h, 5), stone);
    else
        local_box(w, c, 0, h * .5, -z, r->width, h, 1, stone, 0, true);
    corridor(w, c, true, -48, -x);
    corridor(w, c, true, x, 48);
    corridor(w, c, false, z, 48);
    if (r->cell.z > 0)
        corridor(w, c, false, -48, -z);
    if (r->type == BAY)
        local_box(w, c, 0, -.4, -47, 48, .8, 4, floor, 0, true);
    for (int s = -1; s <= 1; s += 2) {
        local_box(w, c, s * (x - .8), .12, 0, .12, .12, r->depth, light, 1.8f, false);
        if (h > 6)
            local_box(w, c, s * (x - .7), h - 1, 0, .18, .16, r->depth, trim, .4f, false);
    }
    if (r->type == BAY || r->type == NAVE || r->type == GARDEN) {
        for (int s = -1; s <= 1; s += 2)
            for (int j = -1; j <= 1; j++) {
                double px = s * (x - 8), pz = j * (z - 12);
                local_box(w, c, px, h * .45, pz, 2.8, h * .9, 2.8, trim, 0, true);
                local_box(w, c, px, 1, pz, 4, 2, 4, stone, 0, true);
                local_box(w, c, px, h - 3, pz, 4, 2, 4, trim, 0, false);
                local_box(w, c, px - s * 1.46, 7, pz, .12, 5, .7, light, 2, false);
                for (int k = 0; k < 7; k++) {
                    double a = px - s * (k + 1) * 3.2, yy = h - 6 + k * .8;
                    local_box(w, c, a, yy, pz, 4, 1.1, 1.2, trim, 0, false);
                }
            }
        for (int j = -2; j <= 2; j++)
            local_box(w, c, 0, .025, j * 12, 3, .04, 5, trim, .3f, false);
    }
    if (r->type == ENGINE) {
        for (int s = -1; s <= 1; s += 2)
            for (int j = -1; j <= 1; j += 2) {
                local_box(w, c, s * (x - 9), 7, j * 17, 8, 14, 8, trim, 0, true);
                for (int k = 0; k < 6; k++)
                    local_box(w, c, s * (x - 9), 2 + k * 2, j * 17, 8.1, .25, 8.1, light, 1.4f,
                              false);
            }
        local_box(w, c, 0, h - 2, 0, 5, 2, r->depth, trim, 0, false);
    }
    if (r->type == CREW) {
        for (int s = -1; s <= 1; s += 2)
            for (int j = -2; j <= 2; j++) {
                local_box(w, c, s * (x - 4), 1.4, j * 9, 6, 2.8, 5, trim, 0, true);
                local_box(w, c, s * (x - 4), 2.9, j * 9, 5, .18, 3, rgb(.12f, .29f, .33f), .4f,
                          false);
                local_box(w, c, s * (x - .6), 5.5, j * 9, .08, 1.5, 3, light, 1, false);
            }
    }
    if (r->type == CONDUIT)
        for (int j = -3; j <= 3; j++) {
            local_box(w, c, 0, h - .15, j * 8, r->width, .22, .35, trim, .1f, false);
            local_box(w, c, 0, h - .28, j * 8, 1, .05, .12, light, 2, false);
        }
    Vec3 lift = world_lift(r->cell);
    box(w, vadd(lift, v3(0, .005, 0)), v3(4, .04, 4), rgb(.07f, .16f, .18f), .3f, false);
    for (int s = -1; s <= 1; s += 2) {
        box(w, vadd(lift, v3(s * 2, .04, 0)), v3(.1, .08, 4), rgb(.1f, .8f, .9f), 2, false);
        box(w, vadd(lift, v3(0, .04, s * 2)), v3(4, .08, .1), rgb(.1f, .8f, .9f), 2, false);
    }
}
Room *world_find(World *w, Cell c) {
    for (int i = 0; i < w->count; i++)
        if (cell_equal(w->rooms[i].cell, c))
            return &w->rooms[i];
    return NULL;
}
bool world_refresh(World *w, const Vec3 *positions, int players) {
    if (players < 1 || players > 2)
        return false;
    bool changed = players != w->anchor_count;
    Cell anchors[2];
    for (int p = 0; p < players; p++) {
        anchors[p] = world_cell(positions[p]);
        if (!cell_equal(anchors[p], w->anchors[p]))
            changed = true;
    }
    if (!changed && w->count)
        return !w->failed;
    Cell desired[SC_MAX_ROOMS];
    int n = 0;
    for (int p = 0; p < players; p++)
        for (int z = -1; z <= 1; z++)
            for (int y = -1; y <= 1; y++)
                for (int x = -1; x <= 1; x++) {
                    Cell c = {anchors[p].x + x, anchors[p].y + y, anchors[p].z + z};
                    if (c.z < 0)
                        continue;
                    bool found = false;
                    for (int i = 0; i < n; i++)
                        if (cell_equal(c, desired[i])) {
                            found = true;
                            break;
                        }
                    if (!found && n < SC_MAX_ROOMS)
                        desired[n++] = c;
                }
    Room *rooms = calloc((size_t)n, sizeof(*rooms));
    if (!rooms) {
        w->failed = true;
        return false;
    }
    for (int i = 0; i < n; i++) {
        Room *old = world_find(w, desired[i]);
        if (old)
            rooms[i] = *old;
        else
            room_init(w, &rooms[i], desired[i]);
    }
    memcpy(w->rooms, rooms, (size_t)n * sizeof(*rooms));
    free(rooms);
    w->count = n;
    memcpy(w->anchors, anchors, (size_t)players * sizeof(Cell));
    w->anchor_count = players;
    w->box_count = 0;
    Color hull = rgb(.10f, .14f, .18f), trim = rgb(.21f, .25f, .28f);
    box(w, v3(-550, 220, -2), v3(1016, 1200, 4), hull, 0, true);
    box(w, v3(550, 220, -2), v3(1016, 1200, 4), hull, 0, true);
    box(w, v3(0, 430, -2), v3(84, 804, 4), hull, 0, true);
    box(w, v3(0, -400, -2), v3(84, 800, 4), hull, 0, true);
    for (int s = -1; s <= 1; s += 2) {
        box(w, v3(s * 45, 80, -7), v3(6, 240, 14), trim, 0, true);
        for (int k = 1; k < 12; k++) {
            box(w, v3(s * (45 + k * 32), 65, -5), v3(4, 290, 10), trim, 0, false);
            box(w, v3(s * (60 + k * 32), 40 + (k % 3) * 25, -4.2), v3(2, 35, 1),
                rgb(.08f, .22f, .3f), .7f, false);
        }
    }
    box(w, v3(0, 28, -4), v3(84, 3, 8), trim, 0, true);
    box(w, v3(0, 26.2, -8.1), v3(76, .25, .2), rgb(.24f, .78f, .73f), 2, false);
    for (int i = 0; i < w->count; i++)
        room_geometry(w, &w->rooms[i]);
    /* Population follows geometry: retry occupied spawn points with a separate
       deterministic random stream so furniture never traps an enemy at birth. */
    for (int i = 0; i < w->count; i++) {
        Room *room = &w->rooms[i];
        for (int j = 0; j < room->enemy_count; j++) {
            Enemy *e = &room->enemies[j];
            if (!e->alive)
                continue;
            for (uint32_t attempt = 0; attempt < 32 && world_overlap(w, e->pos, .42, 1.25);
                 attempt++) {
                uint32_t h = world_hash(w->seed, room->cell, 7000 + (uint32_t)j * 131 + attempt);
                double x = ((h & 65535) / 65535.0 - .5) * (room->width - 3);
                double z = ((h >> 16) / 65535.0 - .5) * (room->depth - 4);
                e->pos = vadd(world_center(room->cell), v3(x, .02, z));
            }
            if (world_overlap(w, e->pos, .42, 1.25))
                e->pos = vadd(world_center(room->cell), v3(0, .02, 2 + (j % 4) * 2));
        }
    }
    w->revision++;
    return !w->failed;
}
bool world_overlap(const World *w, Vec3 p, double radius, double height) {
    for (size_t i = 0; i < w->box_count; i++) {
        const Box *b = &w->boxes[i];
        if (!b->solid)
            continue;
        if (p.y + height <= b->min.y + 1e-6 || p.y >= b->max.y - 1e-6)
            continue;
        double x = clampd(p.x, b->min.x, b->max.x), z = clampd(p.z, b->min.z, b->max.z);
        if ((p.x - x) * (p.x - x) + (p.z - z) * (p.z - z) < radius * radius - 1e-7)
            return true;
    }
    return false;
}
double world_raycast(const World *w, Vec3 o, Vec3 d, double limit) {
    double best = limit;
    for (size_t i = 0; i < w->box_count; i++) {
        const Box *b = &w->boxes[i];
        if (!b->solid)
            continue;
        double lo = 0, hi = best;
        double a[3] = {o.x, o.y, o.z}, v[3] = {d.x, d.y, d.z};
        double mn[3] = {b->min.x, b->min.y, b->min.z}, mx[3] = {b->max.x, b->max.y, b->max.z};
        for (int k = 0; k < 3; k++) {
            if (fabs(v[k]) < 1e-10) {
                if (a[k] < mn[k] || a[k] > mx[k]) {
                    hi = -1;
                    break;
                }
            } else {
                double t = (mn[k] - a[k]) / v[k], u = (mx[k] - a[k]) / v[k];
                if (t > u) {
                    double swap = t;
                    t = u;
                    u = swap;
                }
                lo = fmax(lo, t);
                hi = fmin(hi, u);
            }
        }
        if (hi >= lo && lo < best)
            best = lo;
    }
    return best;
}
void world_kill(World *w, Room *r, int i) {
    if (i < 0 || i >= r->enemy_count || r->record >= w->record_count)
        return;
    r->enemies[i].alive = false;
    w->records[r->record].killed |= UINT32_C(1) << i;
}
