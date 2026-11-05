#include "matrix.h"
#include <string.h>

Cell matrix_region(Cell room) {
    Cell region = world_region(room);
    return (Cell){region.x * 4, region.y, region.z * 4};
}
static void box(MatrixNetwork *w, Vec3 at, Vec3 size, Color color, float glow, unsigned tag) {
    if (w->box_count == SC_MATRIX_BOXES)
        return;
    Vec3 half = vmul(size, .5);
    w->boxes[w->box_count++] =
        (MatrixBox){{vsub(at, half), vadd(at, half), color, glow, true}, (uint8_t)tag};
}
static unsigned node_id(Cell room, Cell key) {
    return (unsigned)((room.z - key.z) * 4 + room.x - key.x);
}
static void bridge(MatrixNetwork *w, Vec3 a, Vec3 b, bool gap, bool gate) {
    Vec3 delta = vsub(b, a), mid = vmul(vadd(a, b), .5);
    bool horizontal = fabs(delta.x) > fabs(delta.z);
    double length = horizontal ? fabs(delta.x) : fabs(delta.z);
    if (length < .1)
        return;
    Color path = rgb(.055f, .12f, .16f);
    if (gap && length > 8) {
        Vec3 dir = vnorm(delta);
        for (int side = -1; side <= 1; side += 2) {
            Vec3 p = vadd(mid, vmul(dir, side * (length + 3) * .25));
            p.y = -.2;
            box(w, p,
                v3(horizontal ? (length - 3) * .5 : 2.8, .4, horizontal ? 2.8 : (length - 3) * .5),
                path, .08f, 0);
        }
        mid.y = -.2;
        box(w, mid, v3(horizontal ? 3 : 2.8, .4, horizontal ? 2.8 : 3), rgb(.13f, .4f, .5f), .7f,
            1);
    } else {
        mid.y = -.2;
        box(w, mid, v3(horizontal ? length : 2.8, .4, horizontal ? 2.8 : length), path, .08f, 0);
    }
    if (gate) {
        mid.y = 2;
        box(w, mid, v3(horizontal ? .6 : 4.5, 4, horizontal ? 4.5 : .6), rgb(.5f, .10f, .3f), .8f,
            2);
    }
}
void matrix_generate(uint32_t seed, uint32_t generator, Cell key, MatrixNetwork *w) {
    memset(w, 0, sizeof(*w));
    w->active = true;
    w->key = key;
    RegionPlan plan;
    world_plan_version(seed, generator, world_region(key), &plan);
    w->theme = plan.district;
    const double spacing[DISTRICT_TYPES] = {19, 23, 31, 27, 24, 25, 22, 28, 24};
    double step = spacing[w->theme];
    uint32_t hash = world_hash(seed, key, 0x4d415458u);
    w->motif = hash % 4;
    w->rare = (hash >> 5) % 4 == 0;
    w->entry = 16;
    for (int i = 0; i < plan.count; i++) {
        RoomSpec *r = &plan.rooms[i];
        unsigned id = node_id(r->cell, key);
        MatrixNode *n = &w->nodes[id];
        n->valid = true;
        n->room = r->cell;
        n->at = v3((r->cell.x - key.x) * step, 0, (r->cell.z - key.z) * step);
        n->district = (uint8_t)r->district;
        n->architecture = (uint8_t)r->type;
        n->service = r->district == MACHINERY                             ? MX_POWER
                     : r->district == COMMAND || r->district == LOGISTICS ? MX_SECURITY
                     : r->district == HABITAT || r->district == CRYONICS  ? MX_BEACON
                     : r->district == RESEARCH || r->district == LUXURY   ? MX_ARCHIVE
                     : r->district == BOTANY                              ? MX_ROBOT
                                                                          : MX_TRAFFIC;
        n->cache = (r->hash >> 7) % 3 == 0;
        w->mask |= 1u << id;
        w->count++;
        if (id < w->entry)
            w->entry = id;
    }
    for (int i = 0; i < 16; i++)
        for (int direction = EAST; direction <= NORTH; direction += 2) {
            int other = i + (direction == EAST ? 1 : 4);
            if (other >= 16 || (direction == EAST && i % 4 == 3) ||
                !(plan.doors[i] & (1u << direction)))
                continue;
            unsigned a = node_id(plan.rooms[plan.owner[i]].cell, key);
            unsigned b = node_id(plan.rooms[plan.owner[other]].cell, key);
            if (a != b) {
                w->nodes[a].links |= 1u << b;
                w->nodes[b].links |= 1u << a;
            }
        }
    unsigned distances[16], queue[16], head = 0, tail = 1;
    for (int i = 0; i < 16; i++)
        distances[i] = 100;
    queue[0] = w->entry;
    distances[w->entry] = 0;
    while (head < tail) {
        unsigned a = queue[head++];
        for (unsigned b = 0; b < 16; b++)
            if ((w->nodes[a].links & (1u << b)) && distances[b] == 100) {
                distances[b] = distances[a] + 1;
                queue[tail++] = b;
            }
    }
    w->credential = tail > 1 ? queue[1] : w->entry;
    w->switch_b = w->credential;
    w->core = queue[tail - 1];
    w->vault = tail > 2 ? queue[tail - 2] : w->credential;
    w->nodes[w->entry].service = MX_POWER;
    w->nodes[w->credential].cache = true;
    if (w->rare)
        w->nodes[w->vault].cache = true;
    w->gap_a = w->entry;
    w->gap_b = w->credential;
    for (unsigned i = 0; i < 16; i++) {
        MatrixNode *n = &w->nodes[i];
        if (!n->valid)
            continue;
        double half = n->architecture == NAVE || n->architecture == GARDEN ? 7 : 5;
        box(w, vadd(n->at, v3(0, -.3, 0)), v3(half * 2, .6, half * 2), rgb(.035f, .065f, .09f),
            .04f, 0);
        for (int x = -1; x <= 1; x += 2)
            for (int z = -1; z <= 1; z += 2) {
                double height = w->theme == SANCTUM ? 24 : w->theme == COMMAND ? 12 : 5;
                box(w, vadd(n->at, v3(x * (half - .5), height * .5, z * (half - .5))),
                    v3(.55, height, .55), rgb(.06f, .10f, .15f), .1f, 0);
            }
        for (unsigned j = i + 1; j < 16; j++)
            if (n->links & (1u << j)) {
                Vec3 a = n->at, b = w->nodes[j].at, elbow = v3(b.x, 0, a.z);
                bool gap = i == w->gap_a && j == w->gap_b;
                bool gate = i == w->core || j == w->core;
                bool first = vlen(vsub(a, elbow)) > .1;
                bridge(w, a, elbow, gap && first, gate && first);
                bridge(w, elbow, b, gap && !first, gate && !first);
            }
    }
    for (unsigned i = 0; i < SC_MATRIX_GUARDS; i++) {
        unsigned node = i < 16 ? i : queue[(i - 16) % tail];
        if (!w->nodes[node].valid)
            continue;
        uint32_t h = world_hash(seed, w->nodes[node].room, 0x494345u + i);
        w->guards[i] = (MatrixGuard){.pos = vadd(w->nodes[node].at, v3(2, 1.1, 2)),
                                     .health = i < 16 ? 38 + (h % 3) * 8 : 50,
                                     .cooldown = 1 + (h % 10) * .1f,
                                     .role = (uint8_t)(i >= 22 ? 3 : h % 3),
                                     .active = i < 16 && node != w->entry};
    }
}
bool matrix_box_active(const MatrixNetwork *w, const MatrixRecord *r, unsigned i) {
    unsigned tag = w->boxes[i].tag;
    return tag == 0 || (tag == 1 && (w->bridge > 0 || r->done[MX_TRAFFIC])) ||
           (tag == 2 && w->gate <= 0 && !(r->keys && (r->flags & MX_ROUTED)));
}
double matrix_raycast(const MatrixNetwork *w, const MatrixRecord *r, Vec3 from, Vec3 dir,
                      double limit) {
    for (unsigned i = 0; i < w->box_count; i++) {
        if (!matrix_box_active(w, r, i))
            continue;
        const Box *b = &w->boxes[i].box;
        double lo = 0, hi = limit;
        double p[] = {from.x, from.y, from.z}, d[] = {dir.x, dir.y, dir.z};
        double mn[] = {b->min.x, b->min.y, b->min.z}, mx[] = {b->max.x, b->max.y, b->max.z};
        for (int axis = 0; axis < 3 && lo <= hi; axis++) {
            if (fabs(d[axis]) < 1e-9) {
                if (p[axis] < mn[axis] || p[axis] > mx[axis])
                    hi = -1;
            } else {
                double a = (mn[axis] - p[axis]) / d[axis], z = (mx[axis] - p[axis]) / d[axis];
                lo = fmax(lo, fmin(a, z));
                hi = fmin(hi, fmax(a, z));
            }
        }
        if (lo <= hi)
            limit = lo;
    }
    return limit;
}
bool matrix_overlap(const MatrixNetwork *w, const MatrixRecord *r, Vec3 feet, double height) {
    for (unsigned i = 0; i < w->box_count; i++) {
        if (!matrix_box_active(w, r, i))
            continue;
        const Box *b = &w->boxes[i].box;
        if (feet.x + .32 > b->min.x && feet.x - .32 < b->max.x && feet.z + .32 > b->min.z &&
            feet.z - .32 < b->max.z && feet.y + height > b->min.y + .01 && feet.y < b->max.y - .01)
            return true;
    }
    return false;
}
