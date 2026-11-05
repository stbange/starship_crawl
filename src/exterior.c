#include "exterior.h"

const Vec3 sc_arrival_spawn = {0, 4, -84};
/* A small stationary launch, stern hatch toward the bay. Absolute coordinates
   remain near the origin; renderer and collision consume these same surfaces. */
const Box sc_skiff_boxes[] = {
    /* Deck, roof, side walls and the open stern frame. */
    {{-4.6, 3.1, -106}, {4.6, 3.6, -88}, {.23f, .25f, .27f}, 0, true},
    {{-4.6, 8.3, -106}, {4.6, 8.8, -88}, {.27f, .30f, .33f}, 0, true},
    {{-4.6, 3.6, -106}, {-4, 8.3, -88}, {.26f, .28f, .30f}, 0, true},
    {{4, 3.6, -106}, {4.6, 8.3, -88}, {.26f, .28f, .30f}, 0, true},
    {{-4, 3.6, -88.6}, {-2, 8.3, -88}, {.30f, .32f, .34f}, 0, true},
    {{2, 3.6, -88.6}, {4, 8.3, -88}, {.30f, .32f, .34f}, 0, true},
    {{-2, 7.5, -88.6}, {2, 8.3, -88}, {.30f, .32f, .34f}, 0, true},
    /* Solid forward bulkhead and stepped cockpit nose. */
    {{-4, 3.6, -106}, {4, 8.3, -105.5}, {.22f, .26f, .29f}, 0, true},
    {{-3.8, 3.6, -108}, {3.8, 7.6, -106}, {.22f, .26f, .30f}, 0, true},
    {{-2.8, 4, -110}, {2.8, 6.7, -108}, {.20f, .24f, .28f}, 0, true},
    {{-2.7, 6.7, -108.2}, {2.7, 7.1, -106.5}, {.06f, .14f, .19f}, .12f, true},
    /* Short wings, engine pods and idle exhaust collars. */
    {{-8.4, 3.9, -102}, {-4.6, 4.5, -94}, {.21f, .24f, .27f}, 0, true},
    {{4.6, 3.9, -102}, {8.4, 4.5, -94}, {.21f, .24f, .27f}, 0, true},
    {{-8.7, 3.2, -104}, {-6.5, 5.8, -89}, {.18f, .21f, .24f}, 0, true},
    {{6.5, 3.2, -104}, {8.7, 5.8, -89}, {.18f, .21f, .24f}, 0, true},
    {{-8.4, 3.6, -89}, {-6.8, 5.4, -88.6}, {.035f, .055f, .065f}, 0, true},
    {{6.8, 3.6, -89}, {8.4, 5.4, -88.6}, {.035f, .055f, .065f}, 0, true},
    /* Roof spine and landing skids establish a recognizable silhouette. */
    {{-.35, 8.8, -103}, {.35, 10.7, -94}, {.22f, .25f, .29f}, 0, true},
    {{-3.4, 1.9, -103}, {-2.9, 3.1, -92}, {.13f, .16f, .19f}, 0, true},
    {{2.9, 1.9, -103}, {3.4, 3.1, -92}, {.13f, .16f, .19f}, 0, true},
    /* Two plain seats leave a generous center aisle. */
    {{-3.5, 3.6, -102.2}, {-2.2, 4.5, -100.4}, {.16f, .20f, .22f}, 0, true},
    {{2.2, 3.6, -102.2}, {3.5, 4.5, -100.4}, {.16f, .20f, .22f}, 0, true},
    {{-3.5, 4.5, -100.7}, {-2.2, 6.2, -100.4}, {.19f, .23f, .24f}, 0, true},
    {{2.2, 4.5, -100.7}, {3.5, 6.2, -100.4}, {.19f, .23f, .24f}, 0, true},
    /* Inactive cockpit consoles and shallow lockers; no service functions. */
    {{-3.6, 3.6, -105.5}, {-1.9, 4.9, -103.4}, {.20f, .24f, .27f}, 0, true},
    {{1.9, 3.6, -105.5}, {3.6, 4.9, -103.4}, {.20f, .24f, .27f}, 0, true},
    {{-3.4, 4.91, -104.8}, {-2.1, 4.95, -103.7}, {.10f, .27f, .30f}, .35f, false},
    {{2.1, 4.91, -104.8}, {3.4, 4.95, -103.7}, {.10f, .27f, .30f}, .35f, false},
    {{-4, 3.6, -96}, {-3.4, 6.3, -93}, {.26f, .27f, .25f}, 0, true},
    {{3.4, 3.6, -96}, {4, 6.3, -93}, {.26f, .27f, .25f}, 0, true},
    /* Fixed warm hatch/cabin lamps and tiny navigation lights. */
    {{-1.8, 7.51, -87.99}, {1.8, 7.67, -87.88}, {1, .68f, .31f}, 2.4f, false},
    {{-1.5, 8.14, -94.2}, {1.5, 8.29, -94}, {1, .75f, .45f}, 2, false},
    {{-1.5, 8.14, -102.2}, {1.5, 8.29, -102}, {1, .75f, .45f}, 2, false},
    {{-8.75, 5.81, -91.5}, {-8.35, 6, -90.7}, {.8f, .16f, .09f}, 1.8f, false},
    {{8.35, 5.81, -91.5}, {8.75, 6, -90.7}, {.12f, .65f, .38f}, 1.8f, false},
    /* Modest identification stripes and an aisle guide. */
    {{-4.62, 5.5, -99}, {-4.60, 6.1, -90}, {.46f, .28f, .13f}, .13f, false},
    {{4.60, 5.5, -99}, {4.62, 6.1, -90}, {.46f, .28f, .13f}, .13f, false},
    {{-.07, 3.61, -104}, {.07, 3.63, -88}, {.36f, .32f, .22f}, .18f, false}};
const size_t sc_skiff_box_count = sizeof(sc_skiff_boxes) / sizeof(sc_skiff_boxes[0]);
const PointLight sc_skiff_lights[SC_SKIFF_LIGHTS] = {
    {{0, 7.1, -87.5}, {1, .69f, .38f}, 13, 2.6f, 0},
    {{0, 7.8, -94}, {1, .77f, .49f}, 11, 2.3f, 0},
    {{0, 7.8, -102}, {1, .77f, .49f}, 10, 2.3f, 0}};

bool exterior_cabin(const World *world, Vec3 p) {
    return world->generator >= SC_SKIFF_GENERATOR && fabs(p.x) < 4 && p.y >= 3.6 && p.y < 8.3 &&
           p.z > -105.5 && p.z < -88;
}
bool exterior_sky_visible(Vec3 eye, Vec3 direction) {
    if (eye.z < 0 && direction.z <= 0)
        return true;
    if (eye.z >= 0 && direction.z >= 0)
        return false;
    double t = -eye.z / direction.z;
    Vec3 aperture = vadd(eye, vmul(direction, t));
    return fabs(aperture.x) < 42 && aperture.y > 0 && aperture.y < 28;
}
