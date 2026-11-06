#include "interior.h"
#include <string.h>

#define SLAB .35

bool world_service_column(uint32_t seed, Cell cell) {
    Cell region = world_region(cell);
    region.y = 0;
    if (region.x == 0 && region.z == 0)
        return cell.x == 0 && cell.z == 0;
    uint32_t h = world_hash(seed, region, 0x51af7u);
    return cell.x == region.x * 4 + (int)(h % 4) && cell.z == region.z * 4 + (int)((h >> 8) % 4);
}
double flight_top(const Flight *f, double z) {
    double t = clampd((z - f->min.z) / (f->max.z - f->min.z), 0, 1);
    if (f->reverse)
        t = 1 - t;
    if (f->stairs)
        t = fmin(1, (floor(t * 32) + 1) / 32);
    return f->min.y + (f->max.y - f->min.y) * t;
}
static bool footprint(const Flight *f, Vec3 p, double radius, double *low, double *high) {
    double dx = p.x - clampd(p.x, f->min.x, f->max.x);
    if (fabs(dx) >= radius)
        return false;
    double reach = sqrt(radius * radius - dx * dx);
    double a = fmax(f->min.z, p.z - reach), b = fmin(f->max.z, p.z + reach);
    if (a >= b - 1e-7)
        return false;
    /* The open side of a tread boundary belongs to the lower tread. */
    double first = flight_top(f, a + 1e-7), last = flight_top(f, b - 1e-7);
    *low = fmin(first, last);
    *high = fmax(first, last);
    return true;
}
bool flight_overlap(const Flight *f, Vec3 p, double radius, double height) {
    double lo, hi;
    return footprint(f, p, radius, &lo, &hi) && p.y < hi - 1e-6 && p.y + height > lo - SLAB + 1e-6;
}
/* Clip a ray against n.p <= bound. This also handles parallel/on-plane rays. */
static bool clip(Vec3 o, Vec3 d, Vec3 n, double bound, double *lo, double *hi) {
    double at = bound - vdot(n, o), speed = vdot(n, d);
    if (fabs(speed) < 1e-10)
        return at >= 0;
    double t = at / speed;
    if (speed > 0)
        *hi = fmin(*hi, t);
    else
        *lo = fmax(*lo, t);
    return *hi >= *lo;
}
double flight_ray(const Flight *f, Vec3 o, Vec3 d, double best) {
    int count = f->stairs ? 32 : 1;
    for (int i = 0; i < count; i++) {
        double za = f->min.z + (f->max.z - f->min.z) * i / count;
        double zb = f->min.z + (f->max.z - f->min.z) * (i + 1) / count;
        double slope = f->stairs ? 0 : (f->max.y - f->min.y) / (f->max.z - f->min.z);
        if (f->reverse)
            slope = -slope;
        double offset = flight_top(f, (za + zb) * .5) - slope * (za + zb) * .5;
        double lo = 0, hi = best;
        if (clip(o, d, v3(-1, 0, 0), -f->min.x, &lo, &hi) &&
            clip(o, d, v3(1, 0, 0), f->max.x, &lo, &hi) &&
            clip(o, d, v3(0, 0, -1), -za, &lo, &hi) && clip(o, d, v3(0, 0, 1), zb, &lo, &hi) &&
            clip(o, d, v3(0, 1, -slope), offset, &lo, &hi) &&
            clip(o, d, v3(0, -1, slope), SLAB - offset, &lo, &hi))
            best = fmin(best, lo);
    }
    return best;
}
static void piece(Builder *b, Box box, double x0, double x1, double z0, double z1) {
    box.min.x = x0;
    box.max.x = x1;
    box.min.z = z0;
    box.max.z = z1;
    world_box(b, vadd(b->origin, vmul(vadd(box.min, box.max), .5)), vsub(box.max, box.min),
              box.color, box.glow, box.solid);
}
static void carve(Builder *b, double x0, double x1, double z0, double z1) {
    Box before[SC_CHUNK_BOXES];
    size_t count = b->chunk->box_count;
    memcpy(before, b->chunk->boxes, count * sizeof(Box));
    b->chunk->box_count = 0;
    for (size_t i = 0; i < count; i++) {
        Box box = before[i];
        double a = fmax(box.min.x, x0), c = fmin(box.max.x, x1);
        double d = fmax(box.min.z, z0), e = fmin(box.max.z, z1);
        if (a >= c || d >= e) {
            piece(b, box, box.min.x, box.max.x, box.min.z, box.max.z);
            continue;
        }
        piece(b, box, box.min.x, a, box.min.z, box.max.z);
        piece(b, box, c, box.max.x, box.min.z, box.max.z);
        piece(b, box, a, c, box.min.z, d);
        piece(b, box, a, c, e, box.max.z);
    }
    for (int i = 0; i < b->chunk->dressing_count;)
        if (b->chunk->dressing[i].pos.x > x0 && b->chunk->dressing[i].pos.x < x1 &&
            b->chunk->dressing[i].pos.z > z0 && b->chunk->dressing[i].pos.z < z1)
            b->chunk->dressing[i] = b->chunk->dressing[--b->chunk->dressing_count];
        else
            i++;
}
static void part(Builder *b, Vec3 at, Vec3 size, Color color, float glow, bool solid) {
    world_box(b, vadd(b->origin, at), size, color, glow, solid);
}
void traversal_build(Builder *b) {
    bool service = world_service_column(b->world->seed, b->chunk->cell);
    Color steel = rgb(.14f, .20f, .22f), amber = rgb(.65f, .32f, .08f), cyan = rgb(.1f, .5f, .65f);
    carve(b, service ? -14 : -3, service ? 14 : 3, service ? -34 : -13, -6);
    /* A walkable field-grate; only the powered cradle passes through it. */
    part(b, v3(0, -.2, service ? -20 : -9.5), v3(6, .4, service ? 28 : 7), steel, 0, true);
    for (int side = -1; side <= 1; side += 2) {
        part(b, v3(side * 2.6, 32, -10), v3(.15, 64, .15), cyan, .4f, false);
        part(b, v3(side * 2.4, .04, -10), v3(.1, .06, 4.8), cyan, 2, false);
    }
    for (int k = 0; k < 8; k++) {
        part(b, v3(0, k * 8 + 4, -12.9), v3(4, .12, .1), cyan, 1.2f, false);
        part(b, v3(0, k * 8 + 4, -6.1), v3(4, .12, .1), cyan, 1.2f, false);
        if (!service)
            continue;
        for (int side = -1; side <= 1; side += 2) {
            double x = side * (k % 2 ? 10 : 6);
            Flight f = {v3(x - 2, k * 8, -28), v3(x + 2, (k + 1) * 8, -12),
                        side > 0 ? amber : steel, k % 2 != 0, side > 0};
            b->chunk->flights[b->chunk->flight_count++] = f;
            double z = k % 2 ? -9 : -31;
            part(b, v3(side * (k ? 8 : 7), k * 8 - .2, z), v3(k ? 8 : 10, .4, 6), steel, 0, true);
            part(b, v3(side * 8, k * 8 + .03, z), v3(7, .04, .2), side > 0 ? amber : cyan, 1.5f,
                 false);
            if (k > 0)
                part(b, v3(side * 3.8, k * 8 + .7, z), v3(.25, 1.4, 6), steel, 0, true);
        }
    }
    if (service)
        for (int side = -1; side <= 1; side += 2) {
            /* A full deck-level apron connects the lift and the room's cross routes. */
            part(b, v3(side * 8.5, -.2, -9), v3(11, .4, 6), steel, 0, true);
            part(b, v3(side * 13, 32, -23), v3(2, 64, 22), steel, 0, true);
            part(b, v3(side * 13, 33.7, -9), v3(2, 60.6, 6), steel, 0, true);
            part(b, v3(side * 3.5, 32, -20), v3(.5, 64, 16), steel, 0, true);
            part(b, v3(side * 8, 32, -33.8), v3(8, 64, .4), steel, 0, true);
            part(b, v3(side * 8, 33.7, -6.2), v3(8, 60.6, .4), steel, 0, true);
        }
}
/* Highest walkable support within the small step/snap interval. */
static double support(const World *w, Vec3 p, double radius, double below, double above) {
    double best = p.y - below - 1;
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *c = &w->chunks[i];
        if (!c->active)
            continue;
        Vec3 origin = world_center(c->cell), q = vsub(p, origin);
        if (fabs(q.x) > 48 + radius || fabs(q.z) > 48 + radius || q.y < -above ||
            q.y > SC_DECK + below)
            continue;
        for (size_t j = 0; j < c->box_count; j++) {
            const Box *box = &c->boxes[j];
            double dx = q.x - clampd(q.x, box->min.x, box->max.x);
            double dz = q.z - clampd(q.z, box->min.z, box->max.z);
            double y = box->max.y + origin.y;
            if (box->solid && dx * dx + dz * dz < radius * radius - 1e-7 && y >= p.y - below &&
                y <= p.y + above)
                best = fmax(best, y);
        }
        for (int j = 0; j < c->flight_count; j++) {
            double lo, hi;
            if (footprint(&c->flights[j], q, radius, &lo, &hi)) {
                double y = hi + origin.y;
                if (y >= p.y - below && y <= p.y + above)
                    best = fmax(best, y);
            }
        }
    }
    return best;
}
void world_move(const World *w, Position *p, Vec3 delta, double radius, double height, bool walking,
                bool *grounded) {
    int steps = (int)clampd(ceil(vlen(delta) / .16), 1, 64);
    delta = vmul(delta, 1.0 / steps);
    bool follow = walking && w->generator >= 9 && delta.y <= 0 &&
                  world_overlap(w, vadd(position_absolute(*p), v3(0, -.06, 0)), radius, height);
    for (int i = 0; i < steps; i++)
        for (int axis = 0; axis < 3; axis++) {
            Position q = *p;
            position_move(
                &q, v3(axis == 0 ? delta.x : 0, axis == 2 ? delta.y : 0, axis == 1 ? delta.z : 0));
            if (follow && axis != 2) {
                Vec3 at = position_absolute(q);
                double y = support(w, at, radius, .32, .30);
                if (y >= at.y - .32) {
                    at.y = y + .00001;
                    if (!world_overlap(w, at, radius, height)) {
                        q = position_from(at);
                        if (grounded)
                            *grounded = true;
                    }
                }
            }
            if (!world_overlap(w, position_absolute(q), radius, height))
                *p = q;
            else if (axis == 2 && delta.y < 0 && grounded)
                *grounded = true;
        }
}
