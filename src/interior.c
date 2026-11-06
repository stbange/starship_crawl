#include "interior.h"
#include "campaign.h"

typedef struct {
    Builder *builder;
    double sx, sz, ex, ez, mx, mz;
    Color metal, trim, accent;
} Alcove;

static const Color metal = {.16f, .18f, .18f}, rust = {.28f, .13f, .075f},
                   bone = {.36f, .32f, .24f}, glass = {.10f, .33f, .34f};
static const Color incident_colors[INCIDENT_TYPES] = {{.50f, .42f, .29f}, {.20f, .31f, .44f},
                                                      {.24f, .58f, .72f}, {.80f, .33f, .10f},
                                                      {.78f, .24f, .12f}, {.38f, .52f, .24f}};

static void piece(Builder *b, Vec3 p, Vec3 size, Color color, float glow, bool solid) {
    /* Cosmetic pieces may use only the capacity left by the planned structure. */
    if (!solid && b->chunk->box_count >= SC_LEGACY_BOXES)
        return;
    world_box(b, vadd(b->origin, p), size, color, glow, solid);
}
static void ornament(Builder *b, DressingKind kind, Vec3 p, Vec3 normal, float scale, uint32_t h) {
    Chunk *c = b->chunk;
    if (c->dressing_count < SC_CHUNK_DRESSING)
        c->dressing[c->dressing_count++] = (Dressing){p, normal, h, kind, scale};
}
static Alcove alcove(Builder *b, const Room *r, int sx, int sz) {
    Vec3 center = vsub(world_room_center(r), b->origin);
    double ex = fmin(44, sx * center.x + r->width * .5 - 3);
    double ez = fmin(44, sz * center.z + r->depth * .5 - 3);
    const Color accents[DISTRICT_TYPES] = {
        {.22f, .42f, .46f}, {.48f, .20f, .08f}, {.32f, .22f, .42f},
        {.49f, .35f, .14f}, {.16f, .31f, .48f}, {.14f, .39f, .30f},
        {.43f, .26f, .12f}, {.18f, .40f, .48f}, {.22f, .35f, .12f}};
    return (Alcove){b,
                    sx,
                    sz,
                    ex,
                    ez,
                    (9 + ex) * .5,
                    (9 + ez) * .5,
                    metal,
                    r->district == LUXURY || r->district == SANCTUM ? bone : rust,
                    accents[r->district]};
}
static void part(Alcove *a, double x, double y, double z, double w, double h, double d, Color color,
                 float glow, bool solid) {
    piece(a->builder, v3(a->sx * x, y, a->sz * z), v3(w, h, d), color, glow, solid);
}
static void prop(Alcove *a, double x, double y, double z, double w, double h, double d, Color color,
                 float glow, bool solid) {
    double fit = fmin(1, (a->ex - 15) / 13);
    part(a, a->mx + x * fit, y, a->mz + z, w * fit, h, d, color, glow, solid);
}
static void doorway(Alcove *a, bool side, double height) {
    double mid = side ? a->mz : a->mx, edge = side ? a->ez : a->ex;
    double half = (edge - 9 - 6) * .5;
    for (int end = 0; end < 2; end++) {
        double along = end ? edge - half * .5 : 9 + half * .5;
        part(a, side ? 9 : along, height * .5, side ? along : 9, side ? .65 : half, height,
             side ? half : .65, a->metal, 0, true);
    }
    part(a, side ? 9 : mid, (height + 4.8) * .5, side ? mid : 9, side ? .8 : 6, height - 4.8,
         side ? 6 : .8, a->trim, 0, true);
    part(a, side ? 8.6 : mid, 4.5, side ? mid : 8.6, side ? .12 : 2.4, .22, side ? 2.4 : .12,
         a->accent, .35f, false);
}
static void structure(Alcove *a, const Room *r) {
    double height = fmin(r->height - .5, r->layout == ARCADES ? 15 : 7);
    if (r->layout == SUITES) {
        doorway(a, false, height);
        doorway(a, true, height);
        part(a, a->ex, height * .5, a->mz, .65, height, a->ez - 9, a->metal, 0, true);
        part(a, a->mx, height * .5, a->ez, a->ex - 9, height, .65, a->metal, 0, true);
        part(a, a->mx, height, a->mz, a->ex - 9, .24, a->ez - 9, a->metal, 0, true);
    } else if (r->layout == WORKBAYS) {
        /* Staggered partial walls leave a front and a lateral approach. */
        part(a, a->ex - 3, 2.8, 9, 6, 5.6, .65, a->metal, 0, true);
        part(a, 9, 2.8, a->ez - 4, .65, 5.6, 8, a->metal, 0, true);
        part(a, a->ex, 3, a->mz, .55, 6, a->ez - 9, a->metal, 0, true);
        part(a, a->mx, height, a->mz, a->ex - 9, .45, 1.1, a->trim, 0, true);
    } else {
        for (int x = 0; x < 2; x++)
            for (int z = 0; z < 2; z++) {
                if (r->landmark && !x && !z)
                    continue; /* Leave long sightlines toward the hall's centerpiece. */
                double px = x ? a->ex : 9, pz = z ? a->ez : 9;
                part(a, px, height * .5, pz, 1.1, height, 1.1, a->metal, 0, true);
                part(a, px, .5, pz, 1.8, 1, 1.8, a->trim, 0, true);
            }
        for (int side = 0; side < 2; side++) {
            part(a, a->mx, height, side ? a->ez : 9, a->ex - 8, .8, 1.2, a->trim, 0, true);
            part(a, side ? a->ex : 9, height, a->mz, 1.2, .8, a->ez - 8, a->trim, 0, true);
        }
    }
    /* Rugs, floor joints and service pipes make the empty space intentional. */
    part(a, a->mx, .015, a->mz, a->ex - 11, .025, a->ez - 11, a->accent, 0, false);
    part(a, a->ex - 1.2, height - .5, a->mz, .25, .25, a->ez - 9, a->trim, 0, false);
}
static void seat(Alcove *a, double x, double z) {
    prop(a, x, .6, z, 1.5, 1.2, 1.5, a->metal, 0, true);
    prop(a, x, 1.55, z + .6, 1.5, 1.3, .3, a->accent, 0, false);
}
static void furnishings(Alcove *a, District role, unsigned theme, uint32_t h) {
    if (theme == 8) {
        /* Drained thermal baths: a low rim, mineral stains and changing benches. */
        for (int side = -1; side <= 1; side += 2) {
            prop(a, side * 4, .45, 1, .6, .9, 8, bone, 0, true);
            prop(a, 0, .45, 1 + side * 4, 8, .9, .6, bone, 0, true);
            prop(a, side * 5.5, .6, 1, 1.4, 1.2, 6, a->trim, 0, true);
        }
        prop(a, 0, .045, 1, 7.4, .06, 7.4, glass, .12f, false);
        prop(a, 0, 2.2, 5, 3, .25, .35, a->metal, 0, false);
    } else if (theme == 9) {
        for (int j = -1; j <= 1; j++) {
            prop(a, j * 4, 1.1, 1, 2.8, 2.2, 3.8, a->trim, 0, true);
            prop(a, j * 4, 2.25, 1, 2.9, .12, 4, glass, 0, false);
            prop(a, j * 4, 2.45, 2, .6, .3, .6, bone, .2f, false);
            seat(a, j * 4, -2.5);
        }
    } else if (theme == 12) {
        prop(a, 0, 1.2, 2, 10, 2.4, 3.5, a->metal, 0, true);
        prop(a, 0, 4, 4, 10, 3, .4, a->trim, 0, true);
        for (int j = 0; j < 5; j++) {
            prop(a, (j - 2) * 1.8, 4, 3.7, .22, 1.6, .25, bone, 0, false);
            prop(a, (j - 2) * 1.8, 2.6, 2, .9, .4, 1.4, a->trim, 0, false);
        }
    } else if (theme == 13) {
        prop(a, 0, 2.8, 1, 10, 4, 5, a->metal, 0, true);
        for (int j = 0; j < 5; j++)
            prop(a, (j - 2) * 2, 2.8, 1, .35, 4.7, 5.7, a->trim, 0, false);
        prop(a, 0, .4, 1, 11, .8, 6, a->trim, 0, true);
        prop(a, 5.2, 2.8, 1, .25, 2.5, 2.5, glass, .25f, false);
    } else if (theme == 24) {
        for (int side = -1; side <= 1; side += 2) {
            prop(a, side * 3.6, 3.2, 2, 3.4, 6.4, 4, a->metal, 0, true);
            for (int row = 0; row < 5; row++)
                prop(a, side * 3.6, .8 + row * 1.15, -.1, 3.2, .4, .3, row % 2 ? bone : a->accent,
                     .12f, false);
        }
        prop(a, 0, 1.1, 5, 2, 2.2, 2, a->trim, 0, true);
    } else if (theme == 27) {
        prop(a, 0, .35, 1, 10, .7, 9, a->metal, 0, true);
        for (int j = 0; j < 3; j++) {
            double x = (j - 1) * 3.2, height = 3 + (h >> j) % 3;
            prop(a, x, height * .5, 1, .65, height, .65, bone, 0, false);
            prop(a, x, height, 1, 3.2, .8, 3.2, a->accent, .1f, false);
            prop(a, x, height + .6, 1, 2.2, .5, 2.2, a->accent, 0, false);
        }
    } else if (role == HABITAT) {
        /* Paired bunks, lockers and a shared desk. */
        for (int s = -1; s <= 1; s += 2) {
            prop(a, s * 4.4, 1.6, 1, 2.6, 3.2, 5.8, a->metal, 0, true);
            for (int level = 0; level < 2; level++)
                prop(a, s * 4.4, .8 + level * 2.1, .7, 2.7, .24, 5, bone, 0, false);
            prop(a, s * 4.4, 2.6, 5, 2.6, 5.2, 1.5, a->accent, 0, true);
        }
        prop(a, 0, .8, 4.5, 3, 1.6, 2, a->trim, 0, true);
        seat(a, 0, 1.8);
    } else if (role == MACHINERY) {
        prop(a, 0, 3, 1, 6, 6, 5, a->metal, 0, true);
        for (int level = 0; level < 4; level++)
            prop(a, 0, 1 + level * 1.25, 1, 6.2, .18, 5.2, a->accent, .22f, false);
        for (int s = -1; s <= 1; s += 2) {
            prop(a, s * 4.4, 2, 3.5, 1.2, 4, 1.2, a->trim, 0, true);
            prop(a, s * 4.4, 4, 1, .35, .35, 5, a->trim, 0, false);
        }
        prop(a, -4.4, 1.1, -4, 3, 2.2, 2, a->metal, 0, true);
        prop(a, -4.4, 2.3, -4, 2.8, .18, 1.8, glass, .2f, false);
    } else if (role == SANCTUM) {
        prop(a, 0, 1.2, 5, 7, 2.4, 2, bone, 0, true);
        prop(a, 0, 4, 5, .45, 5, .45, a->trim, 0, false);
        prop(a, 0, 5, 5, 3, .4, .4, a->accent, .3f, false);
        for (int row = 0; row < 3; row++) {
            prop(a, 0, .65, -4 + row * 2.5, 8, 1.3, .8, a->trim, 0, true);
            prop(a, 0, 1.6, -4.3 + row * 2.5, 8, 1, .24, a->metal, 0, false);
        }
    } else if (role == LUXURY) {
        prop(a, 0, 1.2, 0, 4, 2.4, 7, a->trim, 0, true);
        prop(a, 0, 2.45, 0, 4.2, .15, 7.2, bone, 0, false);
        for (int s = -1; s <= 1; s += 2)
            for (int row = 0; row < 3; row++)
                seat(a, s * 3.5, (row - 1) * 2.6);
        for (int j = 0; j < 3; j++)
            prop(a, 0, 2.65, (j - 1) * 2, .6, .25, .6, glass, .1f, false);
    } else if (role == COMMAND) {
        prop(a, 0, 1.1, 3, 10, 2.2, 2, a->metal, 0, true);
        for (int j = 0; j < 4; j++) {
            prop(a, (j - 1.5) * 2.3, 2.9, 3.6, 1.8, 1.2, .25, glass, .32f, false);
            prop(a, (j - 1.5) * 2.3, 2.25, 2.5, 1.6, .12, .6, a->accent, .2f, false);
        }
        seat(a, -2.5, 0);
        seat(a, 2.5, 0);
        prop(a, 0, 4.3, 5.4, 8, 2.5, .35, a->metal, 0, true);
        prop(a, 0, 4.3, 5.15, 7.5, 2.1, .08, a->accent, .25f, false);
    } else if (role == RESEARCH) {
        for (int s = -1; s <= 1; s += 2) {
            prop(a, s * 3.3, .8, 1, 2.6, 1.6, 5, a->metal, 0, true);
            prop(a, s * 3.3, 1.8, 1, 2.8, .35, 5.3, bone, 0, false);
            prop(a, s * 3.3, 3.6, 4.5, .25, 7.2, .25, a->trim, 0, true);
            prop(a, s * 3.3, 6.4, 1.8, 1.3, .3, 3, glass, .18f, false);
            prop(a, s * 3.3, 2.8, 4.5, 1.8, 1.3, .25, glass, .2f, false);
        }
    } else if (role == LOGISTICS) {
        for (int row = 0; row < 3; row++) {
            double x = (row - 1) * 4.4;
            prop(a, x, 2, 2, 3.5, 4, 6, a->metal, 0, true);
            prop(a, x, 4.2, 2, 3.1, .4, 5.6, a->trim, 0, false);
            for (int seam = 0; seam < 3; seam++)
                prop(a, x - 1.2 + seam * 1.2, 2, -1.1, .12, 3.8, .08, a->accent, 0, false);
        }
    } else if (role == CRYONICS) {
        for (int j = 0; j < 3; j++) {
            double x = (j - 1) * 4;
            prop(a, x, 2.4, 2, 2.7, 4.8, 3.8, a->metal, 0, true);
            prop(a, x, 2.8, -.02, 2.1, 3.1, .18, glass, .2f, false);
            prop(a, x, 5, 2, 2.8, .4, 4, bone, 0, false);
            prop(a, x, 1.1, -.2, .8, .18, .12, a->accent, .4f, false);
        }
    } else {
        prop(a, 0, .45, 1, 10, .9, 9, a->metal, 0, true);
        for (int j = 0; j < 5; j++) {
            double x = (j % 3 - 1) * 3.2, z = (j / 3) * 3.4 - .5;
            double height = 2.5 + ((h >> (j + 1)) % 4);
            prop(a, x, height * .5, z, .45, height, .45, a->trim, 0, false);
            prop(a, x, height, z, 3, 1.2, 2.8, a->accent, 0, false);
        }
    }
}
static void landmark(Builder *b, const Room *r) {
    Vec3 c = vsub(world_room_center(r), b->origin);
    if (r->landmark == BANQUET) {
        piece(b, vadd(c, v3(0, 22, 0)), v3(7, 1.6, 120), bone, 0, true);
        for (int j = -3; j <= 3; j++) {
            piece(b, vadd(c, v3(0, 20, j * 17)), v3(36, 1.2, 2.5), rust, 0, true);
            for (int s = -1; s <= 1; s += 2)
                piece(b, vadd(c, v3(s * 16, 18, j * 17)), v3(.8, 4, .8), glass, .3f, false);
        }
    } else if (r->landmark == CONSERVATORY) {
        piece(b, vadd(c, v3(0, 19, 0)), v3(6, 38, 6), rust, 0, true);
        for (int j = 0; j < 4; j++) {
            double reach = 70 - j * 14;
            piece(b, vadd(c, v3(0, 27 + j * 5, 0)), v3(reach, 2, 5), rust, 0, true);
            piece(b, vadd(c, v3(0, 29 + j * 5, 0)), v3(5, 2, reach), rust, 0, true);
            piece(b, vadd(c, v3(0, 29 + j * 5, 0)), v3(reach, 2, reach), rgb(.13f, .23f, .12f), 0,
                  false);
        }
        ornament(b, GROWTH, vadd(c, v3(0, .1, 0)), v3(0, 1, 0), 4, r->hash);
    } else if (r->landmark == SUSPENDED_ENGINE) {
        piece(b, vadd(c, v3(0, 32, 0)), v3(24, 16, 110), metal, 0, true);
        for (int j = -4; j <= 4; j++) {
            piece(b, vadd(c, v3(0, 32, j * 12)), v3(28, 18, 1), rust, 0, true);
            piece(b, vadd(c, v3(0, 23, j * 12)), v3(20, .2, .7), glass, .35f, false);
        }
    } else if (r->landmark == RELIQUARY) {
        piece(b, vadd(c, v3(0, 20, 0)), v3(5, 40, 5), bone, 0, true);
        for (int j = 0; j < 5; j++) {
            double reach = 54 - j * 8;
            piece(b, vadd(c, v3(0, 18 + j * 6, 0)), v3(reach, 1, 3), bone, 0, true);
            piece(b, vadd(c, v3(0, 18 + j * 6, 0)), v3(3, 1, reach), bone, 0, true);
            for (int s = -1; s <= 1; s += 2) {
                piece(b, vadd(c, v3(s * reach * .5, 15 + j * 6, 0)), v3(1.6, 6, 2.5), glass, .2f,
                      false);
                piece(b, vadd(c, v3(0, 15 + j * 6, s * reach * .5)), v3(2.5, 6, 1.6), glass, .2f,
                      false);
            }
        }
    }
    if (r->landmark)
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                piece(b, vadd(c, v3(sx * 20, 46, sz * 38)), v3(.8, 24, .8), rust, 0, true);
}
static void traces(Builder *b, const Room *r, Alcove *a, uint32_t h) {
    double x = a->sx * (a->ex - 1), z = a->sz * a->mz;
    /* A quiet room still has age and a little dormant growth. */
    ornament(b, SCRAP, v3(a->sx * (a->mx + 2), .06, a->sz * 13), v3(0, 1, 0), .8f, h);
    ornament(b, GROWTH, v3(x, .1, z), v3(0, 1, 0), .6f, h + 1);
    if (r->incident == BLACKOUT) {
        ornament(b, CABLE, v3(a->sx * a->mx, 1, a->sz * 9), v3(0, 1, 0), 1.4f, h + 2);
        part(a, a->ex - .6, 2, a->mz, .2, 3, 3, rgb(.05f, .06f, .07f), 0, false);
    } else if (r->incident == FROST) {
        for (int j = 0; j < r->intensity + 1; j++) {
            part(a, a->mx + j * 1.8, .045, a->mz + 5, 4, .07, 6, glass, .1f, false);
            part(a, a->ex - .8, 1.4 + j * .8, a->mz + j * 2, .25, 2, 1.8, bone, 0, false);
        }
    } else if (r->incident == QUARANTINE) {
        for (int j = 0; j < 3; j++)
            part(a, a->ex - 3, 1 + j * .7, 8.55, 5, .22, .16, j % 2 ? metal : rgb(.57f, .36f, .10f),
                 .15f, false);
        ornament(b, NEST, v3(x - a->sx * .3, 2.2, z), v3(-a->sx, 0, 0), 1.1f, h + 3);
    } else if (r->incident == FIRE) {
        for (int j = 0; j < r->intensity + 1; j++) {
            part(a, a->mx + (j - 1) * 2, .035, a->mz + 3, 3, .05, 7, rgb(.035f, .025f, .025f), 0,
                 false);
            part(a, a->ex - .4, 1.8 + j, a->mz + j * 2, .15, 3, 2, rgb(.05f, .035f, .03f), 0,
                 false);
        }
        ornament(b, CABLE, v3(x - a->sx, .4, z), v3(0, 1, 0), 1.3f, h + 4);
    } else if (r->incident == BLOOM) {
        for (int j = 0; j < r->intensity + 1; j++) {
            ornament(b, GROWTH, v3(a->sx * (6 + j * 2), .08, a->sz * (16 + j * 6)), v3(0, 1, 0),
                     1.1f + j * .35f, h + (uint32_t)j);
            ornament(b, NEST, v3(x, 2.4 + j * .6, z + j * 3), v3(-a->sx, 0, 0), 1.3f,
                     h + (uint32_t)j + 7);
        }
    }
    /* Functional wall panels, seams and a readable illuminated alcove threshold. */
    part(a, a->ex - .5, 3.8, a->mz - 5, .2, 2.2, 2.5, a->metal, 0, false);
    for (int slat = 0; slat < 3; slat++)
        part(a, a->ex - .65, 3 + slat * .5, a->mz - 5, .15, .12, 2, a->trim, 0, false);
    part(a, a->mx, .055, 9, 4, .06, .14, a->accent, .35f, false);
}
static void lighting(Builder *b, const Room *r) {
    for (int i = 0; i < SC_CHUNK_LIGHTS; i++) {
        uint32_t h = world_hash(b->world->seed, b->chunk->cell, 44000 + (uint32_t)i);
        Alcove a = alcove(b, r, i & 1 ? 1 : -1, i & 2 ? 1 : -1);
        Vec3 p = i < 4 ? v3(a.sx * a.mx, 4.4, a.sz * 9) : v3(a.sx * 3.2, 3.2, a.sz * (a.ez - 1));
        uint32_t phase = i % 2 ? 1 + h % 31 : 0;
        float power = r->incident == BLACKOUT ? .28f : .85f;
        Color color = incident_colors[r->incident];
        float radius = 23;
        if (r->landmark && i == 5) {
            /* A weak overhead fixture reveals the hall's silhouette without
               increasing the light budget or washing out the dark side rooms. */
            Vec3 center = vsub(world_room_center(r), b->origin);
            p = v3(center.x > 0 ? 30 : -30, 20, center.z > 0 ? 30 : -30);
            const Color landmark_colors[] = {
                {.70f, .43f, .18f}, {.27f, .48f, .23f}, {.20f, .45f, .65f}, {.53f, .30f, .48f}};
            color = landmark_colors[r->landmark - 1];
            phase = 3 + (h % 4) * 4;
            radius = 58;
        }
        b->chunk->lights[b->chunk->light_count++] = (PointLight){p, color, radius, power, phase};
        piece(b, p, v3(.75, .25, .35), metal, 0, false);
        piece(b, vadd(p, v3(0, -.15, 0)), v3(.55, .06, .28), color, 4 + (float)phase, false);
    }
}
void interior_conduit(Builder *b, const Room *r) {
    /* Crawl spaces keep their low structural clearance, but carry the same
       incident trail as the rooms reached through their doors. */
    Color color = incident_colors[r->incident];
    for (int i = 0; i < b->chunk->light_count; i++) {
        b->chunk->lights[i].color = color;
        b->chunk->lights[i].power = r->incident == BLACKOUT ? .28f : .85f;
    }
    for (size_t i = 0; i < b->chunk->box_count; i++)
        if (b->chunk->boxes[i].glow >= 4)
            b->chunk->boxes[i].color = color;
    if (r->incident)
        for (int sign = -1; sign <= 1; sign += 2)
            piece(b, v3(sign * 2.5, .035, sign * 18), v3(.8, .05, 16),
                  r->incident == FIRE ? rgb(.035f, .025f, .025f) : color, 0, false);
}
static void history_furnishings(Alcove *a, const Room *r) {
    ShipHistory past = world_history(a->builder->world->seed);
    Color glow = incident_colors[world_history_incident(past.cause)];
    /* Replace one alcove's furniture. Routes, doorways and cache approaches stay clear. */
    if (r->history_role == HISTORY_ORIGIN) {
        prop(a, 0, .4, 2, 8, .8, 7, metal, 0, true);
        for (int side = -1; side <= 1; side += 2) {
            prop(a, side * 3, 3, 2, .6, 5.2, 5, rust, 0, true);
            prop(a, side * 1.5, 1.2, 4, .3, 2.2, 2, glow, .5f, false);
        }
        if (past.cause == CRYO_RUPTURE) {
            prop(a, 0, 2, 2, 4.7, 2.2, 4, glass, .3f, true);
            prop(a, 0, 3.7, 4.5, 5, .3, 1, bone, 0, false);
        } else if (past.cause == XENO_OUTBREAK) {
            prop(a, 0, 2.4, 2, 3.6, 3.8, 3.6, glow, .45f, false);
            ornament(a->builder, NEST, v3(a->sx * a->mx, 2.4, a->sz * (a->mz + 2)), v3(0, 1, 0), 3,
                     r->hash);
        } else if (past.cause == MACHINE_PURGE) {
            prop(a, 0, 3, 3, 4, 5, 2, metal, 0, true);
            for (int i = 0; i < 3; i++)
                prop(a, 0, 1.5 + i * 1.4, 1.9, 2.8, .5, .12, glow, 4, false);
        } else {
            prop(a, 0, 2.8, 2, 2.5, 4, 2.5, glow, .8f, true);
            prop(a, 0, 5, 2, 4, .6, 4, metal, 0, false);
        }
    } else if (r->history_role == HISTORY_SURVIVORS) {
        for (int side = -1; side <= 1; side += 2) {
            prop(a, side * 3, .5, 2, 2.4, 1, 4.5, metal, 0, true);
            prop(a, side * 3, 1.05, 2, 2.3, .12, 4.2, bone, 0, false);
        }
        prop(a, 0, .6, 4, 1.3, 1.2, 1.3, rust, 0, true);
        prop(a, 0, 1.25, 4, .9, .1, .9, rgb(.7f, .3f, .1f), .8f, false);
    } else {
        prop(a, 0, 2, 3, 6, 4, 1.5, metal, 0, true);
        for (int i = -1; i <= 1; i++)
            prop(a, i * 1.7, 3, 2.2, 1.1, 1.6, .12, glow, 4, false);
        prop(a, 0, 4.8, 3, .25, 1.6, .25, rust, 0, false);
    }
    /* The illuminated plate marks the notice recorded on entering this room. */
    prop(a, 0, 2, -1, 3.8, 2, .25, metal, 0, false);
    prop(a, 0, 2, -1.15, 3.3, 1.6, .06, rgb(.32f, .6f, .5f), .45f, false);
}
void interior_build(Builder *b, const Room *r) {
    if (b->world->generator >= 7 && campaign_guardian(r->cell) >= 0) {
        Vec3 c = world_center(r->cell);
        for (int x = -1; x <= 1; x += 2)
            for (int z = 0; z < 3; z++) {
                Vec3 at = vadd(c, v3(x * 14, 1.5, -24 + z * 42));
                world_box(b, at, v3(4, 3, 5), rgb(.16f, .18f, .19f), 0, true);
                world_box(b, vadd(at, v3(0, 1.55, 0)), v3(4.2, .1, 5.2), rgb(.6f, .34f, .12f), .6f,
                          false);
            }
        lighting(b, r);
        return;
    }
    landmark(b, r);
    Alcove spaces[4];
    for (int i = 0; i < 4; i++) {
        spaces[i] = alcove(b, r, i & 1 ? 1 : -1, i & 2 ? 1 : -1);
        structure(&spaces[i], r);
        District role = sc_themes[r->theme].sector;
        unsigned theme = r->theme;
        if (r->landmark)
            role = r->landmark == BANQUET            ? LUXURY
                   : r->landmark == CONSERVATORY     ? BOTANY
                   : r->landmark == SUSPENDED_ENGINE ? MACHINERY
                                                     : SANCTUM;
        else if (i == (int)(r->hash % 4))
            role = (r->hash & 32) ? LOGISTICS : MACHINERY;
        if (r->landmark || i == (int)(r->hash % 4))
            theme = SC_THEMES;
        if (theme == 15)
            role = HABITAT;
        if (theme == 11 || theme == 19 || theme == 25)
            role = CRYONICS;
        if (i == 0 && r->history_role && cell_equal(b->chunk->cell, r->cell))
            history_furnishings(&spaces[i], r);
        else
            furnishings(&spaces[i], role, theme, r->hash + (unsigned)i);
    }
    lighting(b, r);
    if (b->world->generator >= 7) {
        ShipTraits t = campaign_traits(b->world->seed, r->cell);
        Vec3 center = world_center(b->chunk->cell);
        for (int side = -1; t.heat && side <= 1; side += 2) {
            Vec3 at = vadd(center, v3(side * fmin(30, r->width * .35), 0, 22));
            if (t.heat < 0) {
                world_box(b, vadd(at, v3(0, r->height - 3, 0)), v3(1.4, 5, 1.4),
                          rgb(.18f, .42f, .5f), .35f, false);
                world_box(b, vadd(at, v3(0, .06, 0)), v3(6, .1, 8), rgb(.12f, .3f, .4f), .22f,
                          false);
            } else {
                world_box(b, vadd(at, v3(0, 1, 0)), v3(2, 2, 3), rgb(.22f, .12f, .08f), 0, true);
                world_box(b, vadd(at, v3(0, 2.05, 0)), v3(1.8, .1, 2.8), rgb(.8f, .2f, .04f), .8f,
                          false);
            }
        }
    }
    for (int i = 0; i < 4; i++)
        traces(b, r, &spaces[i], world_hash(b->world->seed, b->chunk->cell, 45000 + (unsigned)i));
}
Vec3 interior_cache(const Room *r, int pickup) {
    if (r->habitat.form)
        return vadd(world_center(r->cell), v3(pickup == 3 ? 6 : -6, .47, 15));
    /* Just inside the front entrance, away from furniture and either doorway. */
    Builder b = {.origin = world_center(r->cell)};
    Alcove a = alcove(&b, r, pickup == 3 ? 1 : -1, 1);
    return vadd(b.origin, v3(a.sx * a.mx, .47, 12));
}
