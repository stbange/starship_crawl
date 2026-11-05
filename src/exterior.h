#ifndef SC_EXTERIOR_H
#define SC_EXTERIOR_H
#include "world.h"

#define SC_SKIFF_GENERATOR 8u
#define SC_SKIFF_LIGHTS 3
extern const Vec3 sc_arrival_spawn;
extern const Box sc_skiff_boxes[];
extern const size_t sc_skiff_box_count;
extern const PointLight sc_skiff_lights[SC_SKIFF_LIGHTS];

bool exterior_cabin(const World *world, Vec3 feet);
/* Visibility through the endless hull plane, independent of the mesh far clip. */
bool exterior_sky_visible(Vec3 eye, Vec3 direction);
#endif
