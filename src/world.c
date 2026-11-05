#include "interior.h"
#include "campaign.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static bool overlap_world(const World *w, Vec3 p, double radius, double height, bool gates);

void world_init(World *w, uint32_t seed) {
    memset(w, 0, sizeof(*w));
    w->seed = seed;
    w->generator = SC_GENERATOR_VERSION;
}
void world_destroy(World *w) {
    for (int i = 0; i < SC_MAX_CHUNKS; i++)
        free(w->chunks[i].boxes);
    free(w->records);
    free(w->record_index);
    memset(w, 0, sizeof(*w));
}
static size_t record_slot(const World *w, Cell c) {
    size_t slot = world_hash(0, c, 77) & (w->index_cap - 1);
    while (w->record_index[slot] && !cell_equal(w->records[w->record_index[slot] - 1].cell, c))
        slot = (slot + 1) & (w->index_cap - 1);
    return slot;
}
bool world_index_records(World *w) {
    size_t cap = 256;
    while (cap < w->record_count * 2 + 2)
        cap *= 2;
    size_t *table = calloc(cap, sizeof(*table));
    if (!table)
        return false;
    free(w->record_index);
    w->record_index = table;
    w->index_cap = cap;
    for (size_t i = 0; i < w->record_count; i++) {
        size_t slot = record_slot(w, w->records[i].cell);
        if (table[slot])
            return false;
        table[slot] = i + 1;
    }
    return true;
}
static size_t record_for(World *w, Cell c, bool create) {
    if (!w->index_cap && !world_index_records(w)) {
        w->failed = true;
        return SIZE_MAX;
    }
    size_t slot = record_slot(w, c);
    if (w->record_index[slot])
        return w->record_index[slot] - 1;
    if (!create)
        return SIZE_MAX;
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
    w->records[i] = (RoomRecord){.cell = c};
    w->record_index[slot] = i + 1;
    if (w->record_count * 2 >= w->index_cap && !world_index_records(w))
        w->failed = true;
    return i;
}
static bool contains(const Room *r, Cell c) {
    return c.y == r->cell.y && c.x >= r->cell.x && c.x < r->cell.x + r->span_x &&
           c.z >= r->cell.z && c.z < r->cell.z + r->span_z;
}
Room *world_find(World *w, Cell c) {
    for (int i = 0; i < w->count; i++)
        if (contains(&w->rooms[i], c))
            return &w->rooms[i];
    return NULL;
}
const Chunk *world_chunk(const World *w, Cell c) {
    for (int i = 0; i < SC_MAX_CHUNKS; i++)
        if (w->chunks[i].active && cell_equal(w->chunks[i].cell, c))
            return &w->chunks[i];
    return NULL;
}
bool world_ready(const World *w, Cell c) {
    for (int i = 0; i < w->count; i++)
        if (contains(&w->rooms[i], c))
            return w->rooms[i].ready;
    return false;
}

void world_box(Builder *b, Vec3 center, Vec3 size, Color color, float glow, bool solid) {
    if (size.x <= 0 || size.y <= 0 || size.z <= 0)
        return;
    center = vsub(center, b->origin);
    Vec3 lo = vsub(center, vmul(size, .5)), hi = vadd(center, vmul(size, .5));
    lo.x = fmax(lo.x, -48);
    hi.x = fmin(hi.x, 48);
    lo.z = fmax(lo.z, -48);
    hi.z = fmin(hi.z, 48);
    if (hi.x - lo.x < .0001 || hi.z - lo.z < .0001)
        return;
    if (b->chunk->box_count == SC_CHUNK_BOXES) {
        b->world->failed = true;
        return;
    }
    b->chunk->boxes[b->chunk->box_count++] = (Box){lo, hi, color, glow, solid};
}
static void local(Builder *b, Vec3 c, double x, double y, double z, double sx, double sy, double sz,
                  Color col, float glow, bool solid) {
    world_box(b, vadd(c, v3(x, y, z)), v3(sx, sy, sz), col, glow, solid);
}
static void corridor(Builder *b, Vec3 c, bool along_x, double from, double to, double clearance,
                     Color light) {
    if (to - from < .001)
        return;
    double mid = (from + to) * .5, len = to - from;
    Color metal = rgb(.15f, .19f, .22f);
    local(b, c, along_x ? mid : 0, -.3, along_x ? 0 : mid, along_x ? len : 7, .6, along_x ? 7 : len,
          metal, 0, true);
    local(b, c, along_x ? mid : 0, clearance + .35, along_x ? 0 : mid, along_x ? len : 8, .7,
          along_x ? 8 : len, metal, 0, true);
    for (int s = -1; s <= 1; s += 2) {
        local(b, c, along_x ? mid : s * 3.8, clearance * .5, along_x ? s * 3.8 : mid,
              along_x ? len : .8, clearance, along_x ? .8 : len, metal, 0, true);
        local(b, c, along_x ? mid : s * 3.32, .15, along_x ? s * 3.32 : mid, along_x ? len : .07,
              .08, along_x ? .07 : len, light, 1.7f, false);
    }
}
static void wall_piece(Builder *b, Vec3 c, bool xplane, double at, double lo, double hi, double y,
                       double height, Color col) {
    if (hi <= lo || height <= 0)
        return;
    local(b, c, xplane ? at : (lo + hi) * .5, y, xplane ? (lo + hi) * .5 : at, xplane ? 1 : hi - lo,
          height, xplane ? hi - lo : 1, col, 0, true);
}
static void wall(Builder *b, Vec3 c, bool xplane, double at, double lo, double hi, double door,
                 double width, double dh, double h, Color col) {
    if (width <= 0) {
        wall_piece(b, c, xplane, at, lo, hi, h * .5, h, col);
        return;
    }
    wall_piece(b, c, xplane, at, lo, door - width * .5, h * .5, h, col);
    wall_piece(b, c, xplane, at, door + width * .5, hi, h * .5, h, col);
    wall_piece(b, c, xplane, at, door - width * .5, door + width * .5, (h + dh) * .5, h - dh, col);
}
float world_lamp_power(uint32_t phase, double time) {
    if (phase % 4 == 0)
        return 0; /* Most fixture clusters include dead fittings. */
    double t = fmod(fmod(time, 4096) + phase * .37, 7 + phase * .13);
    if (t < .3)
        return .06f;
    if (t < .55)
        return .4f;
    if (t < 1.1)
        return .02f;
    return (float)(.75 + .25 * sin(t * 2 + phase));
}
static void dressing(Chunk *chunk, DressingKind kind, Vec3 pos, Vec3 normal, float scale,
                     uint32_t hash) {
    if (chunk->dressing_count < SC_CHUNK_DRESSING)
        chunk->dressing[chunk->dressing_count++] = (Dressing){pos, normal, hash, kind, scale};
}
static void detail(Builder *b, Vec3 p, Vec3 size, Color col, float glow) {
    if (b->chunk->box_count < SC_CHUNK_BOXES)
        world_box(b, vadd(b->origin, p), size, col, glow, false);
}
static void room_dressing(Builder *b, const Room *r) {
    Chunk *chunk = b->chunk;
    Vec3 room = vsub(world_room_center(r), b->origin);
    double xmin = fmax(-45, room.x - r->width * .5 + 1),
           xmax = fmin(45, room.x + r->width * .5 - 1);
    double zmin = fmax(-45, room.z - r->depth * .5 + 1),
           zmax = fmin(45, room.z + r->depth * .5 - 1);
    Color iron = rgb(.15f, .17f, .16f), rust = rgb(.22f, .11f, .065f), bone = rgb(.26f, .25f, .19f);
    /* Existing collision and saved layouts remain intact. Surface dressing is
       attached to walls/furniture or lies flat enough to step over. */
    for (size_t i = 0; i < chunk->box_count; i++)
        if (!chunk->boxes[i].solid && chunk->boxes[i].glow > 0)
            chunk->boxes[i].glow *= .16f;
    for (int i = 0; i < SC_CHUNK_LIGHTS; i++) {
        uint32_t h = world_hash(b->world->seed, chunk->cell, 18000 + (uint32_t)i);
        double sx = i & 1 ? 1 : -1, sz = i & 2 ? 1 : -1;
        Vec3 p;
        if (r->type == CONDUIT)
            p = v3(sx * 1.8, 1.2, (i - 2.5) * 10);
        else if (r->type == CREW)
            p = v3(sx * 7.6, 4.1, sz * (12 + (i / 4) * 15));
        else if (r->type == ENGINE)
            p = v3(sx * (r->span_x == 2 ? 22.8 : r->width * .5 - 13.2), 5, sz * 20);
        else
            p = v3(sx * (r->span_x == 2 ? 28.4 : r->width * .5 - 9.6), 4.8,
                   sz * (r->span_z == 2 ? 30 : r->depth * .5 - 12));
        if (i >= 4 && r->type != CONDUIT)
            p = v3(sx * 3.2, 2.8, sz * (r->span_z == 2 ? 38 : r->depth * .5 - 2));
        uint32_t phase = (h % 32);
        if (i % 2 == 0)
            phase = 0;
        if (r->type == BAY && i == 4) {
            p = v3(0, 5, -34);
            phase = 1;
        }
        Color colors[] = {rgb(.38f, .62f, .68f), rgb(.85f, .35f, .12f), rgb(.30f, .56f, .25f),
                          rgb(.52f, .24f, .46f)};
        Color col = colors[(h >> 5) % 4];
        float radius = r->type == CONDUIT ? 9 : 21;
        chunk->lights[chunk->light_count++] = (PointLight){p, col, radius, .85f, phase};
        detail(b, p, v3(.55, .18, .3), iron, 0);
        detail(b, vadd(p, v3(0, -.11, 0)), v3(.36, .055, .21), col, 4 + (float)phase);
        if (r->height > 5)
            dressing(chunk, CABLE, vadd(p, v3(0, .2, 0)), v3(0, 1, 0),
                     (float)((r->height - p.y - .2) / 3.4), h);
    }
    /* Low roots creep into the occupied aisles; the lift and exits stay clear. */
    for (int side = -1; side <= 1; side += 2)
        for (int end = -1; end <= 1; end += 2) {
            uint32_t h = world_hash(b->world->seed, chunk->cell,
                                    21000 + (uint32_t)(side + 2) * 3 + (uint32_t)(end + 1));
            double x = side * (r->type == CONDUIT ? 2.6 : 5.8), z = end * 18;
            dressing(chunk, GROWTH, v3(x, .07, z), v3(0, 1, 0), r->type == CONDUIT ? .38f : .9f, h);
            if (r->type == CREW) {
                double cabin = (r->depth * .5 - 8) / (2 + (int)((r->hash >> 9) % 2));
                z = end * (8 + cabin * .18);
                dressing(chunk, NEST, v3(side * 7.65, 2.3, z), v3(-side, 0, 0), .75f, h + 5);
                for (int k = 0; k < 4; k++)
                    detail(b, v3(side * 7.65, 3.6 + k * .18, z), v3(.12, .07, 1.2), iron, 0);
            }
        }
    for (int side = 0; side < 2; side++) {
        double edge = room.x + (side ? 1 : -1) * r->width * .5;
        if (fabs(edge) > 48)
            continue;
        double x = side ? xmax : xmin, sign = side ? -1 : 1;
        for (int j = 0; j < 5; j++) {
            uint32_t h = world_hash(b->world->seed, chunk->cell, 19000 + (uint32_t)(side * 11 + j));
            double z = zmin + (zmax - zmin) * (j + .5) / 5;
            if (fabs(z) < 5)
                continue;
            double wall_height = fmin(r->height - 1, 4.0 + (h % 3));
            if (r->type == CONDUIT)
                wall_height = .65;
            Vec3 p = v3(x, wall_height * .5, z);
            /* Recessed service panels, riveted seams and broken vent slats. */
            detail(b, p, v3(.12, wall_height, 3.7), iron, 0);
            for (int slat = 0; slat < 4; slat++) {
                if ((h >> (slat + 3)) % 5 == 0)
                    continue;
                detail(b, vadd(p, v3(sign * .12, -wall_height * .3 + slat * wall_height * .2, 0)),
                       v3(.14, .075, 3.4), rust, 0);
            }
            if (j % 2 == 0) {
                dressing(chunk, GROWTH, v3(x + sign * .18, .13, z), v3(0, 1, 0), 1.3f, h);
                dressing(chunk, NEST, v3(x + sign * .25, fmin(r->height * .6, 4.5), z),
                         v3(sign, 0, 0), 1, h ^ 99);
            }
        }
    }
    for (int i = 0; i < 8; i++) {
        uint32_t h = world_hash(b->world->seed, chunk->cell, 20000 + (uint32_t)i);
        double x = xmin + (xmax - xmin) * (h % 1000) / 1000.0,
               z = zmin + (zmax - zmin) * ((h >> 10) % 1000) / 1000.0;
        if (fabs(x) < 4 && fabs(z + 10) < 5)
            continue;
        dressing(chunk, i % 3 == 0 ? GROWTH : SCRAP, v3(x, .055, z), v3(0, 1, 0),
                 .7f + (h % 5) * .2f, h);
    }
    /* Details reinforce each room's former purpose, beneath the infestation. */
    if (r->type == ENGINE)
        for (int side = -1; side <= 1; side += 2) {
            double rx = side * (r->span_x == 2 ? 27 : r->width * .5 - 9);
            for (int k = 0; k < 4; k++) {
                detail(b, v3(rx + side * (k - 1.5), 11, 0), v3(.34, .34, 49), k % 2 ? rust : iron,
                       0);
                detail(b, v3(rx + side * (k - 1.5), 7, -24), v3(.34, 8, .34), iron, 0);
            }
            dressing(chunk, NEST, v3(rx, 7.5, 16), v3(0, 0, -1), 1.7f,
                     r->hash + (uint32_t)(side + 2));
        }
    if (r->type == CREW)
        for (int side = -1; side <= 1; side += 2)
            for (int z = -1; z <= 1; z += 2) {
                double mid = 8 + (r->depth * .5 - 8) / (2 + (int)((r->hash >> 9) % 2)) * .5;
                detail(b, v3(side * (r->width * .5 - 3.8), 2.24, z * mid), v3(3.8, .2, 1.4), bone,
                       0);
                detail(b, v3(side * 8.3, 4.9, z * 16), v3(.12, .65, 1.1), rust, 0);
                detail(b, v3(side * 8.15, 4.9, z * 16), v3(.08, .23, .7), rgb(.12f, .36f, .25f),
                       .3f);
            }
    if (r->type == NAVE || r->type == GARDEN || r->type == BAY)
        for (int side = -1; side <= 1; side += 2) {
            double x = side * (r->span_x == 2 ? 30 : r->width * .5 - 8),
                   z = r->span_z == 2 ? 30 : r->depth * .5 - 12;
            for (int k = 0; k < 5; k++)
                detail(b, v3(x, 3 + k * 3, -z), v3(3.1, .2, 3.1), k % 2 ? rust : bone, 0);
            dressing(chunk, GROWTH, v3(x, 2, -z - 1.5), v3(0, 0, -1), 2,
                     r->hash + (uint32_t)(side + 3));
            dressing(chunk, CABLE, v3(x - 2, 2.5, -z), v3(0, 1, 0), 2,
                     r->hash + (uint32_t)(side + 7));
        }
}
static void theme_geometry(Builder *b, const Room *r) {
    if (r->theme < 6)
        return;
    Vec3 c = world_room_center(r);
    Color metal = rgb(.19f, .23f, .24f), gold = rgb(.38f, .29f, .13f),
          glass = rgb(.12f, .32f, .33f);
    /* Furniture leaves an eight-metre cross and every lift unobstructed. */
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            double x = sx * (r->width * .5 - 13), z = sz * (r->depth * .5 - 15);
            int theme = r->theme;
            if (theme == 6 || theme == 7 || theme == 9 || theme == 15) {
                local(b, c, x, .7, z, 8, 1.4, 4, gold, 0, true);
                local(b, c, x, 1.5, z + sz * 1.7, 8, 1.2, .5, metal, 0, false);
                local(b, c, x - sx * 6, 1, z, 2, 2, 2, glass, .15f, false);
                if (theme == 7)
                    local(b, c, x, r->height - 6, z, 10, 1, 10, gold, .25f, false);
                if (theme == 9)
                    for (int k = 0; k < 4; k++)
                        local(b, c, x + (k - 1.5) * 1.5, 2, z, 1, 1.7, .7, glass, .35f, false);
            } else if (theme == 8) {
                for (int k = -1; k <= 1; k += 2) {
                    local(b, c, x + k * 4, .4, z, .6, .8, 10, gold, 0, true);
                    local(b, c, x, .4, z + k * 5, 8, .8, .6, gold, 0, true);
                }
                local(b, c, x, .04, z, 7, .06, 9, glass, .35f, false);
            } else if (theme == 10 || theme == 11 || theme == 12 || theme == 13) {
                for (int k = 0; k < 3; k++) {
                    local(b, c, x + (k - 1) * 2.2, theme == 12 ? .7 : 2.5, z, 1.7,
                          theme == 12 ? 1.4 : 5, theme == 13 ? 9 : 5, metal, 0, true);
                    local(b, c, x + (k - 1) * 2.2, 5.1, z, 1.5, .25, 4, theme == 11 ? glass : gold,
                          .3f, false);
                    if (theme == 10)
                        local(b, c, x + (k - 1) * 2.2, 2, z - sz * 2.6, 1.3, 2, .15,
                              rgb(.8f, .18f, .04f), .4f, false);
                    if (theme == 11)
                        for (int j = 0; j < 3; j++)
                            local(b, c, x + (k - 1) * 2.2, 2 + j, z - sz * 2.6, 1.8, .15, .2, glass,
                                  .2f, false);
                    local(b, c, x + (k - 1) * 2.2, 7, z, .7, 3, .7, metal, 0, false);
                }
            } else if (theme == 14 || theme == 16 || theme == 17) {
                local(b, c, x, 1, z, 10, 2, 3, metal, 0, true);
                for (int k = 0; k < 4; k++)
                    local(b, c, x + (k - 1.5) * 2, 2.8, z + .5, 1.5, 1.4, .2, glass, .35f, false);
                if (theme == 17)
                    local(b, c, x, 7, z, 2, 10, 2, gold, 0, true);
            } else if (theme == 18 || theme == 19 || theme == 20 || theme == 26) {
                for (int k = 0; k < 4; k++) {
                    local(b, c, x + (k - 1.5) * 2.3, 1, z, 1.7, 2, 5, metal, 0, true);
                    local(b, c, x + (k - 1.5) * 2.3, 2.8, z, 1.5, 1.5, 4, glass,
                          theme == 26 ? .2f : 0, false);
                    if (theme == 19)
                        local(b, c, x + (k - 1.5) * 2.3, 3, z - sz * 2.6, .15, 5, .3,
                              rgb(.7f, .4f, .1f), .15f, false);
                    local(b, c, x + (k - 1.5) * 2.3, 4, z + 2, 1.7, .3, .4, gold, 0, false);
                }
            } else if (theme == 21 || theme == 27) {
                local(b, c, x, .5, z, 10, 1, 10, metal, 0, true);
                for (int k = 0; k < 6; k++) {
                    local(b, c, x + (k % 3 - 1) * 2.7, 2 + k % 2, z + (k / 3 * 2 - 1) * 2, .6,
                          4 + k % 2 * 2, .6, glass, 0, false);
                    local(b, c, x + (k % 3 - 1) * 2.7, 4 + k % 2, z + (k / 3 * 2 - 1) * 2, 2.5, 1.5,
                          2.5, rgb(.13f, .25f, .10f), 0, false);
                }
            } else if (theme == 22) {
                for (int k = 0; k < 5; k++)
                    local(b, c, x + (k % 2) * 5, 2 + (k / 2) * 4, z, 4, 4, 7, k % 2 ? metal : gold,
                          0, true);
            } else if (theme == 23 || theme == 24 || theme == 25) {
                for (int k = 0; k < 4; k++) {
                    local(b, c, x + (k - 1.5) * 2.5, 3, z, 1.8, 6, 3, metal, 0, true);
                    for (int j = 0; j < 5; j++)
                        local(b, c, x + (k - 1.5) * 2.5, 1 + j, z - sz * 1.6, 1.5, .18, .3,
                              theme == 25 ? gold : glass, .15f, false);
                }
            }
        }
}
static void geometry(World *w, Chunk *chunk, const Room *r) {
    Vec3 c = world_room_center(r), tile = world_center(chunk->cell);
    Builder b = {w, chunk, tile};
    double x = r->width * .5, z = r->depth * .5, h = r->height;
    Color stone = rgb(.17f, .20f, .23f), trim = rgb(.27f, .25f, .22f),
          floor = rgb(.13f, .17f, .19f);
    const Color palettes[DISTRICT_TYPES] = {
        {.22f, .68f, .79f}, {1, .34f, .1f},    {.6f, .43f, .91f},
        {.8f, .6f, .22f},   {.2f, .48f, .85f}, {.3f, .75f, .55f},
        {.8f, .4f, .16f},   {.28f, .7f, .9f},  {.4f, .65f, .16f}};
    Color light = palettes[r->district];
    if (r->type == ENGINE)
        stone = rgb(.22f, .16f, .14f);
    if (r->type == GARDEN) {
        stone = rgb(.20f, .18f, .25f);
        trim = rgb(.38f, .29f, .15f);
    }
    if (r->type == CONDUIT) {
        stone = rgb(.14f, .20f, .17f);
        light = rgb(.40f, .76f, .19f);
    }
    local(&b, c, 0, -.4, 0, r->width, .8, r->depth, floor, 0, true);
    local(&b, c, 0, h + .4, 0, r->width, .8, r->depth, stone, 0, true);
    double tx = tile.x - c.x, tz = tile.z - c.z;
    for (int d = 0; d < 4; d++) {
        bool xp = d <= EAST;
        bool edge = d == WEST    ? chunk->cell.x == r->cell.x
                    : d == EAST  ? chunk->cell.x == r->cell.x + r->span_x - 1
                    : d == SOUTH ? chunk->cell.z == r->cell.z
                                 : chunk->cell.z == r->cell.z + r->span_z - 1;
        if (!edge)
            continue;
        Connector port = world_connector_version(w->seed, w->generator, chunk->cell, (Direction)d);
        bool open = port.open;
        double at = (d == WEST || d == SOUTH) ? -(xp ? x : z) : (xp ? x : z);
        double mid = xp ? tz : tx, half = xp ? z : x;
        bool entry = r->type == BAY && d == SOUTH;
        wall(&b, c, xp, at, fmax(-half, mid - 48), fmin(half, mid + 48), mid,
             entry ? 48 : port.width, entry ? 22 : port.clearance, h, stone);
        if (open) {
            double local_at = at - (xp ? tx : tz);
            corridor(&b, tile, xp, d == WEST || d == SOUTH ? -48 : local_at,
                     d == WEST || d == SOUTH ? local_at : 48, port.clearance, light);
            /* District thresholds frame each passage at a familiar human scale. */
            if (xp)
                for (int s = -1; s <= 1; s += 2)
                    local(&b, tile, local_at, port.clearance * .5, s * 3.6, .25, port.clearance,
                          .16, light, 1, false);
            else
                for (int s = -1; s <= 1; s += 2)
                    local(&b, tile, s * 3.6, port.clearance * .5, local_at, .16, port.clearance,
                          .25, light, 1, false);
        }
    }
    if (r->type == BAY)
        local(&b, c, 0, -.4, -47, 48, .8, 4, floor, 0, true);
    /* Per-chunk detail preserves human-scale references within a shared hall. */
    double px = r->span_x == 2 ? 30 : x - 8, pz = r->span_z == 2 ? 30 : z - 12;
    if (r->type == BAY || (w->generator == 3 && (r->type == NAVE || r->type == GARDEN))) {
        for (int s = -1; s <= 1; s += 2)
            for (int j = -1; j <= 1; j += 2) {
                local(&b, tile, s * px, h * .45, j * pz, 2.8, h * .9, 2.8, trim, 0, true);
                local(&b, tile, s * px, 1, j * pz, 4, 2, 4, stone, 0, true);
                local(&b, tile, s * (px - 1.46), 7, j * pz, .12, 5, .7, light, 2, false);
                for (int k = 0; k < 7; k++)
                    local(&b, tile, s * (px - (k + 1) * 3.2), h - 6 + k * .8, j * pz, 4, 1.1, 1.2,
                          trim, 0, false);
            }
        for (int j = -2; j <= 2; j++)
            local(&b, tile, 0, .025, j * 12, 3, .04, 5, trim, .3f, false);
        if (r->span_x == 2) {
            local(&b, c, 0, h - 12, 0, 4, 18, 4, trim, 0, false);
            local(&b, c, 0, h - 15, 0, 24, 2, 2, light, 1.5f, false);
            local(&b, c, 0, h - 15, 0, 2, 2, 24, light, 1.5f, false);
        }
    }
    if (w->generator == 3 && r->type == ENGINE && r->theme < 6) {
        for (int s = -1; s <= 1; s += 2)
            for (int j = -1; j <= 1; j += 2) {
                double rx = r->span_x == 2 ? 27 : x - 9;
                local(&b, tile, s * rx, 7, j * 20, 8, 14, 8, trim, 0, true);
                for (int k = 0; k < 6; k++)
                    local(&b, tile, s * rx, 2 + k * 2, j * 20, 8.1, .25, 8.1, light, 1.4f, false);
            }
        local(&b, tile, 0, h - 3, 0, 5, 2, 86, trim, 0, false);
        if (r->span_x == 2)
            local(&b, c, 0, h * .5, 0, 8, h - 12, 8, light, .8f, true);
    }
    if (w->generator == 3 && r->type == CREW) {
        int bays = 2 + (int)((r->hash >> 9) % 2);
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                for (int i = 0; i < bays; i++) {
                    double depth = (z - 8) / bays, mid = sz * (8 + depth * (i + .5));
                    /* Separate cabins open into a central cross; exits and lift stay clear. */
                    wall(&b, c, true, sx * 8, mid - depth * .5, mid + depth * .5, mid, 3, 3.2, 6.8,
                         stone);
                    local(&b, c, sx * (x + 8) * .5, 3.4, sz * (8 + depth * i), x - 8, 6.8, .5,
                          stone, 0, true);
                    if (r->theme < 6 || r->theme == 15 || r->theme == 18 || r->theme == 19) {
                        local(&b, c, sx * (x - 4), 1, mid, 5, 2, depth - 3, trim, 0, true);
                        local(&b, c, sx * (x - 4), 2.1, mid, 4, .15, depth - 4, light, .4f, false);
                    }
                    local(&b, c, sx * 8.1, 3.9, mid, .12, .3, 1.5, light, 1.4f, false);
                }
    }
    if (r->type == CONDUIT)
        for (int j = -3; j <= 3; j++) {
            local(&b, c, 0, h - .15, j * 8, r->width, .22, .35, trim, .1f, false);
            local(&b, c, 0, h - .28, j * 8, 1, .05, .12, light, 2, false);
        }
    for (int s = -1; s <= 1; s += 2) {
        local(&b, tile, s * 2, .065, -10, .1, .08, 4, rgb(.1f, .8f, .9f), 2, false);
        local(&b, tile, 0, .065, -10 + s * 2, 4, .08, .1, rgb(.1f, .8f, .9f), 2, false);
    }
    local(&b, tile, 0, .025, -10, 4, .04, 4, rgb(.07f, .16f, .18f), .3f, false);
    if (w->generator == 3 || r->type == BAY || r->height < 3) {
        theme_geometry(&b, r);
        room_dressing(&b, r);
        if (w->generator >= 4 && r->type == CONDUIT)
            interior_conduit(&b, r);
    } else {
        interior_build(&b, r);
    }
}

static bool wanted(const World *w, Cell c) {
    for (int i = 0; i < w->desired_count; i++)
        if (cell_equal(w->desired[i], c))
            return true;
    return false;
}
static int distance(Cell a, Cell b) {
    int x = abs(a.x - b.x), y = abs(a.y - b.y), z = abs(a.z - b.z);
    return x > y ? (x > z ? x : z) : (y > z ? y : z);
}
static bool retained(const World *w, const Room *r) {
    for (int z = 0; z < r->span_z; z++)
        for (int x = 0; x < r->span_x; x++) {
            Cell c = {r->cell.x + x, r->cell.y, r->cell.z + z};
            if (wanted(w, c))
                return true;
            for (int p = 0; p < w->anchor_count; p++)
                if (distance(c, w->anchors[p]) <= 2)
                    return true;
        }
    return false;
}
static void evict(World *w, int room) {
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        Chunk *c = &w->chunks[i];
        if (c->active && cell_equal(c->owner, w->rooms[room].cell)) {
            w->box_count -= c->box_count;
            free(c->boxes);
            memset(c, 0, sizeof(*c));
            w->chunk_count--;
        }
    }
    w->rooms[room] = w->rooms[--w->count];
    w->revision++;
}
bool world_refresh(World *w, const Vec3 *positions, int players) {
    if (players < 1 || players > 2)
        return false;
    bool changed = players != w->anchor_count;
    for (int p = 0; p < players; p++) {
        Cell c = world_cell(positions[p]);
        if (!cell_equal(c, w->anchors[p]))
            changed = true;
        w->anchors[p] = c;
    }
    if (!changed && w->desired_count)
        return !w->failed;
    w->anchor_count = players;
    w->desired_count = 0;
    for (int z = -1; z <= 1; z++)
        for (int y = -1; y <= 1; y++)
            for (int x = -1; x <= 1; x++)
                for (int p = 0; p < players; p++) {
                    Cell c = {w->anchors[p].x + x, w->anchors[p].y + y, w->anchors[p].z + z};
                    if (c.z < 0)
                        continue;
                    RoomSpec r = world_describe_version(w->seed, w->generator, c);
                    for (int dz = 0; dz < r.span_z; dz++)
                        for (int dx = 0; dx < r.span_x; dx++) {
                            Cell part = {r.cell.x + dx, r.cell.y, r.cell.z + dz};
                            if (!wanted(w, part)) {
                                if (w->desired_count == SC_MAX_CHUNKS) {
                                    w->failed = true;
                                    return false;
                                }
                                w->desired[w->desired_count++] = part;
                            }
                        }
                }
    for (int i = w->count - 1; i >= 0; i--)
        if (!retained(w, &w->rooms[i]))
            evict(w, i);
    w->pending = 0;
    for (int i = 0; i < w->desired_count; i++)
        if (!world_chunk(w, w->desired[i]))
            w->pending++;
    return !w->failed;
}
static Room *room_create(World *w, RoomSpec spec) {
    Room *r = world_find(w, spec.cell);
    if (r)
        return r;
    if (w->count == SC_MAX_ROOMS) {
        w->failed = true;
        return NULL;
    }
    r = &w->rooms[w->count++];
    memset(r, 0, sizeof(*r));
    r->cell = spec.cell;
    r->type = spec.type;
    r->district = spec.district;
    r->theme = spec.theme;
    r->layout = spec.layout;
    r->incident = spec.incident;
    r->intensity = spec.intensity;
    r->landmark = spec.landmark;
    r->history_role = spec.history_role;
    r->span_x = spec.span_x;
    r->span_z = spec.span_z;
    r->width = spec.width;
    r->depth = spec.depth;
    r->height = spec.height;
    r->hash = spec.hash;
    r->record = record_for(w, r->cell, false);
    return r;
}
static void populate(World *w, Room *r) {
    r->ready = true;
    r->pickup_count = 5;
    for (int i = 0; i < 5; i++) {
        uint32_t h = world_hash(w->seed, r->cell, 25000 + (uint32_t)i);
        Pickup *p = &r->pickups[i];
        p->id = (uint8_t)i;
        p->kind = (uint8_t)(i == 3 ? PICK_WEAPON : i == 4 ? PICK_LOG : h % 7);
        p->item = (uint8_t)(i == 3 ? 1 + h % (SC_GENERATED_WEAPONS - 1) : h % 12);
        p->pos = position_from(vadd(world_lift(r->cell), v3(i % 2 ? 1.5 : -1.5, .45, 3 + i * 1.6)));
        if (w->generator >= 4 && i >= 3 && r->type != BAY && r->height >= 3)
            p->pos = position_from(interior_cache(r, i));
        p->active = r->record == SIZE_MAX || !(w->records[r->record].collected & (1u << i));
    }

    unsigned difficulty = world_difficulty(r->cell);
    r->enemy_count = r->type == BAY ? 0 : (int)(5 + (difficulty > 12 ? 12 : difficulty));
    for (int i = 0; i < r->enemy_count; i++) {
        uint32_t h = world_hash(w->seed ^ w->generator, r->cell, 1000 + (uint32_t)i * 31);
        Enemy *e = &r->enemies[i];
        e->id = (uint8_t)i;
        e->normal = v3(0, 1, 0);
        e->heading = v3(0, 0, 1);
        unsigned pool = w->generator >= 6 ? SC_ENEMY_TYPES - 1 : 26;
        unsigned choice = h % pool;
        for (unsigned retry = 0; retry < pool; retry++) {
            e->kind = (uint8_t)(choice + (w->generator >= 6 && choice >= 26));
            if (sc_enemies[e->kind].tier <= difficulty)
                break;
            choice = (choice + 1) % pool;
        }
        if (r->height < 3 && sc_enemies[e->kind].size > .8f)
            e->kind = (uint8_t)(h % 2 ? 0 : 3);
        e->health = sc_enemies[e->kind].health + (float)difficulty * 5;
        e->special = 2 + (h % 300) / 100.0f;
        if (i == 0 && contains(r, (Cell){2, 1, 2})) {
            e->kind = 26;
            e->health = sc_enemies[26].health + (float)difficulty * 5;
        }
        e->cooldown = (float)(h % 200) / 100;
        e->alive = r->record == SIZE_MAX || !(w->records[r->record].killed & (UINT32_C(1) << i));
        for (unsigned attempt = 0; attempt < 64; attempt++) {
            uint32_t v =
                world_hash(w->seed ^ w->generator, r->cell, 7000 + (uint32_t)i * 131 + attempt);
            e->pos = position_from(
                vadd(world_room_center(r), v3(((v & 65535) / 65535.0 - .5) * (r->width - 4), .02,
                                              ((v >> 16) / 65535.0 - .5) * (r->depth - 4))));
            if (!overlap_world(w, position_absolute(e->pos), .42, 1.25, false))
                break;
        }
        if (i == 0 && e->kind == 26 && contains(r, (Cell){2, 1, 2}))
            e->pos = position_from(vadd(world_lift((Cell){2, 1, 2}), v3(0, 0, 9)));
        if (overlap_world(w, position_absolute(e->pos), .42, 1.25, false))
            e->pos = position_from(vadd(world_center(r->cell), v3(0, .02, 2 + (i % 4) * 2)));
    }
    if (w->generator >= 7)
        campaign_populate(w, r);
}
unsigned world_stream(World *w, unsigned budget) {
    w->last_work = 0;
    if (w->failed || !w->anchor_count)
        return 0;
    while (w->last_work < budget && w->pending) {
        int viewer = w->next_player % w->anchor_count, selected = -1, best = INT_MAX;
        w->next_player = (viewer + 1) % w->anchor_count;
        RoomSpec focus = world_describe_version(w->seed, w->generator, w->anchors[viewer]);
        for (int i = 0; i < w->desired_count; i++)
            if (!world_chunk(w, w->desired[i])) {
                Cell c = w->desired[i];
                int score = distance(c, w->anchors[viewer]) * 10;
                if (c.y == focus.cell.y && c.x >= focus.cell.x &&
                    c.x < focus.cell.x + focus.span_x && c.z >= focus.cell.z &&
                    c.z < focus.cell.z + focus.span_z)
                    score = 0;
                if (score < best) {
                    best = score;
                    selected = i;
                }
            }
        if (selected < 0) {
            w->pending = 0;
            break;
        }
        if (w->chunk_count == SC_MAX_CHUNKS) {
            int victim = -1, far = -1;
            for (int i = 0; i < w->count; i++) {
                Room *r = &w->rooms[i];
                bool need = false;
                for (int j = 0; j < w->desired_count; j++)
                    if (contains(r, w->desired[j]))
                        need = true;
                int d = distance(r->cell, w->anchors[0]);
                if (w->anchor_count == 2) {
                    int b = distance(r->cell, w->anchors[1]);
                    if (b < d)
                        d = b;
                }
                if (!need && d > far) {
                    victim = i;
                    far = d;
                }
            }
            if (victim < 0) {
                w->failed = true;
                break;
            }
            evict(w, victim);
        }
        Cell cell = w->desired[selected];
        Room *r = room_create(w, world_describe_version(w->seed, w->generator, cell));
        if (!r)
            break;
        Chunk *c = NULL;
        for (int i = 0; i < SC_MAX_CHUNKS; i++)
            if (!w->chunks[i].active) {
                c = &w->chunks[i];
                break;
            }
        if (!c) {
            w->failed = true;
            break;
        }
        c->boxes = calloc(SC_CHUNK_BOXES, sizeof(Box));
        if (!c->boxes) {
            w->failed = true;
            break;
        }
        c->cell = cell;
        c->owner = r->cell;
        c->active = true;
        c->serial = ++w->revision;
        geometry(w, c, r);
        w->box_count += c->box_count;
        w->chunk_count++;
        w->pending--;
        w->generated++;
        w->last_work++;
        bool complete = true;
        for (int z = 0; z < r->span_z; z++)
            for (int x = 0; x < r->span_x; x++)
                if (!world_chunk(w, (Cell){r->cell.x + x, r->cell.y, r->cell.z + z}))
                    complete = false;
        if (complete && !r->ready)
            populate(w, r);
    }
    return w->last_work;
}

static bool overlaps(const Box *b, Vec3 p, double radius, double height) {
    if (!b->solid || p.y + height <= b->min.y + 1e-6 || p.y >= b->max.y - 1e-6)
        return false;
    double x = clampd(p.x, b->min.x, b->max.x), z = clampd(p.z, b->min.z, b->max.z);
    return (p.x - x) * (p.x - x) + (p.z - z) * (p.z - z) < radius * radius - 1e-7;
}
static const Box hull[] = {
    {{-1000000000, -1000000000, -4}, {-42, 1000000000, 0}, {0, 0, 0}, 0, true},
    {{42, -1000000000, -4}, {1000000000, 1000000000, 0}, {0, 0, 0}, 0, true},
    {{-42, 28, -4}, {42, 1000000000, 0}, {0, 0, 0}, 0, true},
    {{-42, -1000000000, -4}, {42, 0, 0}, {0, 0, 0}, 0, true}};
static bool overlap_world(const World *w, Vec3 p, double radius, double height, bool gates) {
    /* Unfinished rooms are solid. A frontier can never be crossed into missing
       floors, even when generation is deliberately throttled to one chunk. */
    if (p.z + radius >= 0)
        for (int z = -1; z <= 1; z += 2)
            for (int x = -1; x <= 1; x += 2) {
                Vec3 sample = vadd(p, v3(x * radius, 0, z * radius));
                if (sample.z >= 0 && (!world_ready(w, world_cell(sample)) ||
                                      (gates && w->open_layer &&
                                       campaign_layer(world_cell(sample)) > w->open_layer)))
                    return true;
            }
    for (size_t i = 0; i < sizeof(hull) / sizeof(hull[0]); i++)
        if (overlaps(&hull[i], p, radius, height))
            return true;
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *c = &w->chunks[i];
        if (!c->active)
            continue;
        Vec3 local_p = vsub(p, world_center(c->cell));
        if (fabs(local_p.x) > 48 + radius || fabs(local_p.z) > 48 + radius ||
            local_p.y + height < -.8 || local_p.y > SC_DECK)
            continue;
        for (size_t j = 0; j < c->box_count; j++)
            if (overlaps(&c->boxes[j], local_p, radius, height))
                return true;
    }
    return false;
}
bool world_overlap(const World *w, Vec3 p, double radius, double height) {
    return overlap_world(w, p, radius, height, true);
}
static double ray_box(const Box *b, Vec3 o, Vec3 d, double best) {
    if (!b->solid)
        return best;
    double lo = 0, hi = best, a[] = {o.x, o.y, o.z}, v[] = {d.x, d.y, d.z};
    double mn[] = {b->min.x, b->min.y, b->min.z}, mx[] = {b->max.x, b->max.y, b->max.z};
    for (int k = 0; k < 3; k++) {
        if (fabs(v[k]) < 1e-10) {
            if (a[k] < mn[k] || a[k] > mx[k])
                return best;
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
    return hi >= lo && lo < best ? lo : best;
}
double world_raycast(const World *w, Vec3 o, Vec3 d, double limit) {
    double best = campaign_gate_ray(w, o, d, limit);
    for (size_t i = 0; i < sizeof(hull) / sizeof(hull[0]); i++)
        best = ray_box(&hull[i], o, d, best);
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *c = &w->chunks[i];
        if (!c->active)
            continue;
        Vec3 local_o = vsub(o, world_center(c->cell));
        Box bounds = {v3(-48, -1, -48), v3(48, SC_DECK, 48), {0, 0, 0}, 0, true};
        if (ray_box(&bounds, local_o, d, best) >= best)
            continue;
        for (size_t j = 0; j < c->box_count; j++)
            best = ray_box(&c->boxes[j], local_o, d, best);
    }
    return best;
}
uint32_t world_systems(const World *w, Cell cell) {
    if (!w->index_cap || !w->record_count || cell.z < 0)
        return 0;
    for (int i = 0; i < w->count; i++)
        if (contains(&w->rooms[i], cell))
            return w->rooms[i].record == SIZE_MAX ? 0 : w->records[w->rooms[i].record].systems;
    Cell owner = world_describe_version(w->seed, w->generator, cell).cell;
    size_t slot = record_slot(w, owner);
    return w->record_index[slot] ? w->records[w->record_index[slot] - 1].systems : 0;
}
bool world_set_systems(World *w, Cell cell, uint32_t systems) {
    if (systems >= 16 || (systems & SYSTEM_POWER && systems & SYSTEM_BLACKOUT) || cell.z < 0)
        return false;
    Cell owner = world_describe_version(w->seed, w->generator, cell).cell;
    size_t index = record_for(w, owner, true);
    if (index == SIZE_MAX || w->failed)
        return false;
    w->records[index].systems = systems;
    for (int i = 0; i < w->count; i++)
        if (cell_equal(w->rooms[i].cell, owner))
            w->rooms[i].record = index;
    return true;
}
void world_kill(World *w, Room *r, int i) {
    if (i < 0 || i >= r->enemy_count)
        return;
    if (r->record == SIZE_MAX)
        r->record = record_for(w, r->cell, true);
    if (r->record == SIZE_MAX)
        return;
    r->enemies[i].alive = false;
    w->records[r->record].killed |= UINT32_C(1) << i;
}

void world_collect(World *w, Room *r, int pickup) {
    if (pickup < 0 || pickup >= r->pickup_count || !r->pickups[pickup].active)
        return;
    r->record = record_for(w, r->cell, true);
    if (r->record == SIZE_MAX)
        return;
    w->records[r->record].collected |= 1u << pickup;
    r->pickups[pickup].active = false;
}
