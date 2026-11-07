#include "frontier.h"
#include "habitat.h"
#include <stdlib.h>

const char *frontier_reveal(unsigned reveal) {
    static const char *names[DISTRICT_TYPES + 1] = {
        "PASSAGE",           "SLEEPER SPINDLE", "BROKEN TURBINE", "INVERTED CHOIR",
        "FALLEN CHANDELIER", "COMMAND IRIS",    "SPECIMEN CROWN", "CARGO RIB CAGE",
        "FROZEN LEVIATHAN",  "ROOTED SUN"};
    return names[reveal <= DISTRICT_TYPES ? reveal : 0];
}
Vec3 frontier_focus(const Room *r) {
    /* The owner cell always exists, including bent/masked footprints. Keep the
       focal object high over the main aisle, clear of side-bay towers and the service shaft. */
    return vadd(world_center(r->cell), v3(0, fmax(10, r->height * .60), 26));
}
static void box(Builder *b, Vec3 at, Vec3 size, Color color, float glow, bool solid) {
    world_box(b, at, size, color, glow, solid);
}
void frontier_build(Builder *b, const Room *r) {
    unsigned reveal = r->habitat.reveal;
    Vec3 center = world_center(b->chunk->cell);
    Color metal = rgb(.13f, .16f, .18f), bone = rgb(.29f, .25f, .20f);
    Color tint = sc_atmospheres[r->habitat.atmosphere].tint;
    if (reveal) {
        /* Repeated, dim marks give distance and height a readable cadence. */
        for (int side = -1; side <= 1; side += 2) {
            for (int level = 0; level < 4; level++) {
                double y = 4 + level * (r->height - 6) / 4;
                box(b, vadd(center, v3(side * 36, y, 36)), v3(.35, 1.2, .5), tint, .38f, false);
                box(b, vadd(center, v3(side * 36, y, -36)), v3(.35, 1.2, .5), tint, .28f, false);
            }
            /* A dark vestibule frame opens onto the full-height silhouette.
               Its nine metre opening also accommodates both deployable rigs. */
            for (int end = -1; end <= 1; end += 2) {
                box(b, vadd(center, v3(side * 6.2, 3, end * 37)), v3(1.2, 6, 5), metal, 0, true);
                box(b, vadd(center, v3(side * 3.4, 6.4, end * 37)), v3(6.8, .8, 5), metal, 0, true);
            }
        }
        /* Landing arrows look into the hall, while the opposite marks lead
           back to the lift. They are flush, never extra collision. */
        for (int i = 0; i < 5; i++)
            box(b, vadd(center, v3(4 + i * 2.5, .045, 4 + i * 2)), v3(.5, .035, .7), tint, .8f,
                false);
        Vec3 focus = frontier_focus(r);
        double scale = fmin(18, r->height * .27);
        if (reveal == HABITAT + 1) {
            for (int i = 0; i < 9; i++) {
                double y = (i - 4) * scale * .22;
                box(b, vadd(focus, v3(0, y, 0)), v3(6 + (i % 2) * 3, 1.3, 5), metal, 0, false);
                box(b, vadd(focus, v3(0, y, -2.6)), v3(5, .12, .1), tint, .55f, false);
            }
        } else if (reveal == MACHINERY + 1 || reveal == COMMAND + 1) {
            for (int i = 0; i < 12; i++) {
                if (reveal == MACHINERY + 1 && (i == 1 || i == 2))
                    continue;
                double a = i * SC_PI / 6;
                Vec3 at = vadd(focus, v3(cos(a) * scale * .55, sin(a) * scale * .55, 0));
                box(b, at, v3(2.5, 2.5, reveal == MACHINERY + 1 ? 9 : 1.5), metal, 0, false);
                box(b, vadd(at, v3(0, 0, reveal == MACHINERY + 1 ? -4.6 : -.9)), v3(.3, 1.4, .2),
                    tint, .8f, false);
                if (reveal == COMMAND + 1 && i % 3 == 0)
                    box(b, vadd(focus, v3(cos(a) * scale * .3, sin(a) * scale * .3, 0)),
                        v3(1.2, scale * .4, .6), bone, .1f, false);
            }
            box(b, focus, reveal == MACHINERY + 1 ? v3(2, 2, 11) : v3(3, 3, .8), bone, .05f, false);
        } else if (reveal == SANCTUM + 1 || reveal == LUXURY + 1) {
            for (int i = -3; i <= 3; i++) {
                double length = scale * (1 - abs(i) * .17);
                if (reveal == SANCTUM + 1) {
                    box(b, vadd(focus, v3(i * 2.4, -length * .25, 0)), v3(.8, length, 2), bone, 0,
                        false);
                    box(b, vadd(focus, v3(i * 2.4, -length * .75, -1.1)), v3(.3, 1, .3), tint, 1,
                        false);
                } else {
                    Vec3 arm = vadd(focus, v3(i * 2.4, i * .7, sin(i) * 3));
                    box(b, arm, v3(3, .7, 1), bone, 0, false);
                    box(b, vadd(arm, v3(0, -2, 0)), v3(.16, 4, .16), metal, .04f, false);
                    box(b, vadd(arm, v3(0, -4, 0)), v3(.6, 1.2, .6), tint, 1, false);
                }
            }
            if (reveal == SANCTUM + 1)
                box(b, vadd(focus, v3(0, scale * .3, 0)), v3(18, 1.6, 4), metal, 0, false);
        } else if (reveal == RESEARCH + 1 || reveal == BOTANY + 1) {
            for (int i = 0; i < 7; i++) {
                double a = i * SC_PI * 2 / 7;
                Vec3 at = vadd(focus, v3(cos(a) * 7, sin(a) * 7, 0));
                box(b, at, v3(2, 6, 3), reveal == BOTANY + 1 ? rgb(.17f, .24f, .13f) : metal, 0,
                    false);
                box(b, vadd(at, v3(0, 0, -1.6)), v3(.6, 4, .15), tint, .6f, false);
                if (reveal == BOTANY + 1)
                    box(b, vadd(at, v3(cos(a), -3 - i % 3, sin(a) * 2)), v3(.7, 7 + i % 3, .8),
                        bone, .05f, false);
            }
            box(b, focus, v3(4, 5, 4), bone, .1f, false);
        } else if (reveal == LOGISTICS + 1) {
            for (int i = -3; i <= 3; i++) {
                box(b, vadd(focus, v3(i * 3, 0, 0)), v3(.8, scale, 9), metal, 0, false);
                box(b, vadd(focus, v3(i * 3, -scale * .5, -4.6)), v3(.4, .4, .3), tint, .65f,
                    false);
            }
        } else {
            for (int i = -4; i <= 4; i++) {
                box(b, vadd(focus, v3(i * 2, sin(i * .6) * 2, 0)), v3(2.6, 3, 4), bone, .03f,
                    false);
                box(b, vadd(focus, v3(i * 2, sin(i * .6) * 2 + 2, 0)), v3(.5, 4, .7), tint, .35f,
                    false);
            }
        }
        /* A single weak point source outlines the focal object; darkness remains
           between it and the repeated scale marks. Owner-only avoids duplicates. */
        if (cell_equal(b->chunk->cell, r->cell)) {
            int slot = b->chunk->light_count < SC_CHUNK_LIGHTS ? b->chunk->light_count++
                                                               : SC_CHUNK_LIGHTS - 1;
            b->chunk->lights[slot] =
                (PointLight){vsub(vadd(focus, v3(0, 0, -6)), center), tint, 24, .9f, 1};
        }
    }
    if (r->habitat.pace == PACE_FRONTIER && r->habitat.challenge == CH_SIEGE) {
        /* Alternating cover leaves the central lane, console and lift clear. */
        for (int i = 0; i < 3; i++)
            box(b, vadd(world_center(r->cell), v3(i & 1 ? -3.2 : 3.2, 1.3, 8 + i * 7)),
                v3(3, 2.6, 1.4), metal, 0, true);
    }
    if (r->type != BAY && r->height >= 5 && cell_equal(b->chunk->cell, r->cell)) {
        /* A real recess beside the route: moving seams can be seen before an
           ambusher unfolds. Neither its walls nor its occupant seal a doorway. */
        Vec3 at = vadd(center, v3(9, 0, 14));
        box(b, vadd(at, v3(0, 1.5, 0)), v3(.5, 3, 4), metal, 0, true);
        box(b, vadd(at, v3(-1, 1.5, -2)), v3(2, 3, .5), metal, 0, true);
        box(b, vadd(at, v3(-1, 3, 0)), v3(2, .3, 4), metal, 0, true);
    }
}
