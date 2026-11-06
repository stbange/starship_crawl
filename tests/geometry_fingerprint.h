#ifndef SC_TEST_GEOMETRY_FINGERPRINT_H
#define SC_TEST_GEOMETRY_FINGERPRINT_H
#include "world.h"
/* Quantized semantic fields, never padding or platform-specific object bytes. */
static uint32_t fingerprint_word(uint32_t h, int64_t n) {
    uint32_t value = (uint32_t)n;
    for (int i = 0; i < 4; i++) {
        h ^= (value >> (i * 8)) & 255;
        h *= UINT32_C(16777619);
    }
    return h;
}
static uint32_t fingerprint_vector(uint32_t h, Vec3 p) {
    h = fingerprint_word(h, llround(p.x * 10000));
    h = fingerprint_word(h, llround(p.y * 10000));
    return fingerprint_word(h, llround(p.z * 10000));
}
static uint32_t geometry_fingerprint(const Chunk *c) {
    uint32_t h = fingerprint_word(UINT32_C(2166136261), (int64_t)c->box_count);
    for (size_t i = 0; i < c->box_count; i++) {
        const Box *b = &c->boxes[i];
        h = fingerprint_vector(h, b->min);
        h = fingerprint_vector(h, b->max);
        h = fingerprint_vector(h, v3(b->color.r, b->color.g, b->color.b));
        h = fingerprint_word(h, llround(b->glow * 10000));
        h = fingerprint_word(h, b->solid);
    }
    /* Old chunks have no flights: retain historical fixture hashes verbatim. */
    if (c->flight_count) {
        h = fingerprint_word(h, c->flight_count);
        for (int i = 0; i < c->flight_count; i++) {
            const Flight *f = &c->flights[i];
            h = fingerprint_vector(h, f->min);
            h = fingerprint_vector(h, f->max);
            h = fingerprint_vector(h, v3(f->color.r, f->color.g, f->color.b));
            h = fingerprint_word(h, f->reverse);
            h = fingerprint_word(h, f->stairs);
        }
    }
    h = fingerprint_word(h, c->light_count);
    for (int i = 0; i < c->light_count; i++) {
        const PointLight *p = &c->lights[i];
        h = fingerprint_vector(h, p->pos);
        h = fingerprint_vector(h, v3(p->color.r, p->color.g, p->color.b));
        h = fingerprint_word(h, llround(p->power * 10000));
        h = fingerprint_word(h, llround(p->radius * 10000));
        h = fingerprint_word(h, p->phase);
    }
    h = fingerprint_word(h, c->dressing_count);
    for (int i = 0; i < c->dressing_count; i++) {
        const Dressing *d = &c->dressing[i];
        h = fingerprint_vector(h, d->pos);
        h = fingerprint_vector(h, d->normal);
        h = fingerprint_word(h, d->kind);
        h = fingerprint_word(h, d->hash);
        h = fingerprint_word(h, llround(d->scale * 10000));
    }
    return h;
}
#endif
