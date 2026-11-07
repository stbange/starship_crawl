#include "habitat.h"
#include "campaign.h"
#include <stdio.h>

const AtmosphereDef sc_atmospheres[ATMOSPHERES] = {
    {{.48f, .56f, .62f}, {.004f, .008f, .012f}, .008f, "DUST AND SILENCE"},
    {{.66f, .78f, .80f}, {.012f, .021f, .025f}, .013f, "VENTING STEAM"},
    {{.18f, .63f, .68f}, {.005f, .020f, .025f}, .011f, "COLORED RUNOFF"},
    {{.45f, .62f, .19f}, {.013f, .021f, .007f}, .015f, "SPORE FALL"},
    {{.37f, .65f, .93f}, {.012f, .023f, .038f}, .014f, "CRYOGENIC MIST"},
    {{.94f, .35f, .10f}, {.028f, .010f, .005f}, .012f, "EMBER RAIN"},
    {{.67f, .26f, .86f}, {.019f, .006f, .028f}, .012f, "LUMINOUS CONDENSATE"},
    {{.65f, .52f, .28f}, {.019f, .017f, .009f}, .010f, "BOILING RESERVOIRS"},
    {{.83f, .54f, .31f}, {.021f, .014f, .009f}, .009f, "PULSING PRESSURE VENTS"},
    {{.62f, .46f, .36f}, {.019f, .010f, .007f}, .009f, "DRIFTING ASH"},
    {{.79f, .46f, .18f}, {.018f, .009f, .004f}, .006f, "BAKED SILENCE"},
    {{.53f, .76f, .88f}, {.008f, .015f, .025f}, .008f, "DRIFTING ICE CRYSTALS"},
    {{.35f, .64f, .79f}, {.007f, .016f, .025f}, .010f, "FROZEN RUNOFF"},
    {{.58f, .69f, .80f}, {.005f, .009f, .016f}, .005f, "BRITTLE STILLNESS"}};
const char *sc_habitat_forms[9] = {"CHAMBER",           "PROCESSIONAL VAULT", "LONG GALLERY",
                                   "TURBINE CANYON",    "BENT ARCADE",        "BRANCHING CONCOURSE",
                                   "HORSESHOE GALLERY", "RESERVOIR HALL",     "HANGING GARDEN"};
const char *sc_ecologies[ECOLOGIES] = {"FERAL REMNANTS",   "AUTONOMOUS MACHINES",
                                       "NEST COLONY",      "COLD-ADAPTED HUNTERS",
                                       "PHASE SCAVENGERS", "RUNOFF SCAVENGERS"};

bool habitat_owns(Cell owner, int sx, int sz, uint16_t mask, Cell c) {
    int x = c.x - owner.x, z = c.z - owner.z;
    return c.y == owner.y && x >= 0 && z >= 0 && x < sx && z < sz &&
           (!mask || (mask & (1u << (z * 4 + x))));
}
void habitat_reserve(uint32_t seed, Cell region, int *x, int *z, RoomSpec *macro) {
    Cell pair = region;
    pair.y = region.y - ((region.y % 2 + 2) % 2);
    Cell upper = pair;
    upper.y++;
    Cell origin = world_region(world_history(seed).origin);
    if ((!region.x && !region.z) || campaign_arena_region(pair) || campaign_arena_region(upper) ||
        (origin.x == pair.x && origin.z == pair.z && (origin.y == pair.y || origin.y == upper.y)))
        return;
    static const uint16_t masks[] = {0x33, 0x7, 0x111, 0x13, 0x27, 0x57, 0x77, 0x333};
    static const uint8_t widths[] = {2, 3, 1, 2, 3, 3, 3, 2}, depths[] = {2, 1, 3, 2, 2, 2, 2, 3};
    uint32_t h = world_hash(seed, pair, 0x564f4c55u);
    unsigned form = h % 8;
    macro->span_x = widths[form];
    macro->span_z = depths[form];
    macro->habitat.mask = masks[form];
    macro->habitat.form = (uint8_t)(form + 1);
    *x = (int)((h >> 7) % (5 - widths[form]));
    *z = (int)((h >> 11) % (5 - depths[form]));
    if (form == 0 || form >= 6)
        macro->habitat.volume = (uint8_t)(region.y == pair.y ? 1 : 2);
}
void habitat_plan(uint32_t seed, RoomSpec *r) {
    Habitat *a = &r->habitat;
    ShipTraits traits = campaign_traits(seed, r->cell);
    unsigned climate = STILL_AIR;
    if (r->incident == BLOOM || r->district == BOTANY)
        climate = SPORE_AIR;
    else if (r->incident == FROST || r->district == CRYONICS)
        climate = RIME_AIR;
    else if (r->incident == FIRE)
        climate = EMBER_AIR;
    else if (r->district == MACHINERY)
        climate = STEAM_LEAK;
    else if (r->district == LUXURY || r->theme == 11)
        climate = FLOODED;
    else if (r->district == SANCTUM || r->theme == 17)
        climate = VOID_GLOW;
    else if (r->hash % 3 == 0)
        climate = FLOODED;
    /* Strong directional extremes alter weather without erasing the room's use. */
    if (traits.heat <= -2)
        climate = RIME_AIR;
    if (traits.heat >= 2)
        climate = r->district == MACHINERY ? STEAM_LEAK : EMBER_AIR;
    a->atmosphere = (uint8_t)climate;
    a->ecology = (uint8_t)(climate == SPORE_AIR   ? BROOD_NEST
                           : climate == RIME_AIR  ? CRYOPHILE
                           : climate == VOID_GLOW ? PHANTOM
                           : climate == FLOODED   ? SCAVENGER
                           : r->district == MACHINERY || r->district == COMMAND || traits.order > 1
                               ? MECHANICAL
                               : FERAL);
    a->tale =
        (uint8_t)(r->district == LUXURY                                             ? LAST_FEAST
                  : r->district == RESEARCH                                         ? CONTAINMENT
                  : r->district == MACHINERY                                        ? REPAIR
                  : r->district == SANCTUM                                          ? PILGRIMAGE
                  : r->district == BOTANY                                           ? CULTIVATION
                  : r->district == COMMAND                                          ? MUTINY
                  : r->history_role == HISTORY_SURVIVORS || r->district == CRYONICS ? TRIAGE
                                                                                    : EVACUATION);
    if (r->history_role == HISTORY_SURVIVORS)
        a->tale = TRIAGE;
    if (r->incident == QUARANTINE)
        a->tale = CONTAINMENT;
    if (r->type == BAY) {
        a->atmosphere = STILL_AIR;
        a->ecology = FERAL;
        a->tale = EVACUATION;
    }
    if (a->form) {
        r->height = a->volume == 1 ? 64 : 58;
        r->layout = ARCADES;
    }
}
static void box(Builder *b, Vec3 p, Vec3 size, Color color, float glow, bool solid) {
    world_box(b, vadd(b->origin, p), size, color, glow, solid);
}
void habitat_floor(Builder *b, const Room *r) {
    Vec3 center = world_room_center(r);
    if (r->habitat.volume != 2) {
        world_box(b, vadd(center, v3(0, -.4, 0)), v3(r->width, .8, r->depth),
                  rgb(.105f, .12f, .12f), 0, true);
        return;
    }
    /* The gallery floor has four true openings per tile. Cross routes, the lift,
       the outer promenade and each story alcove retain a continuous floor. */
    Vec3 offset = vsub(center, b->origin);
    double lo_x = fmax(-48, offset.x - r->width * .5), hi_x = fmin(48, offset.x + r->width * .5);
    double lo_z = fmax(-48, offset.z - r->depth * .5), hi_z = fmin(48, offset.z + r->depth * .5);
    double xs[] = {lo_x, -38, -10, 10, 38, hi_x}, zs[] = {lo_z, -38, -12, 12, 38, hi_z};
    for (int z = 0; z < 5; z++)
        for (int x = 0; x < 5; x++) {
            if ((x == 1 || x == 3) && (z == 1 || z == 3))
                continue;
            box(b, v3((xs[x] + xs[x + 1]) * .5, -.4, (zs[z] + zs[z + 1]) * .5),
                v3(xs[x + 1] - xs[x], .8, zs[z + 1] - zs[z]), rgb(.15f, .17f, .18f), 0, true);
        }
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            for (int edge = 0; edge < 2; edge++) {
                double x = sx * (edge ? 38 : 10), z = sz * 25;
                Color rail = rgb(.22f, .25f, .25f);
                for (int bar = 0; bar < 2; bar++) {
                    box(b, v3(x, bar ? 1.25 : .55, z), v3(.18, .10, 26), rail, 0, true);
                    box(b, v3(sx * 24, bar ? 1.25 : .55, sz * (edge ? 38 : 12)), v3(28, .10, .18),
                        rail, 0, true);
                }
                for (int post = 0; post < 5; post++) {
                    box(b, v3(x, .65, sz * (12 + post * 6.5)), v3(.16, 1.3, .16), rail, 0, true);
                    box(b, v3(sx * (10 + post * 7), .65, sz * (edge ? 38 : 12)), v3(.16, 1.3, .16),
                        rail, 0, true);
                }
            }
        }
}
static void sprite(Builder *b, SpriteKind kind, Vec3 pos, Vec3 normal, float w, float h,
                   const Room *r, unsigned salt) {
    Chunk *c = b->chunk;
    Vec3 center = vsub(world_room_center(r), b->origin);
    double ex = fmin(47, center.x + r->width * .5 - .15),
           ez = fmin(47, center.z + r->depth * .5 - .15);
    double lx = fmax(-47, center.x - r->width * .5 + .15),
           lz = fmax(-47, center.z - r->depth * .5 + .15);
    if (pos.x < lx || pos.x > ex || pos.z < lz || pos.z > ez)
        return;
    if (normal.y > .5) {
        w = (float)fmin(w, 2 * fmin(pos.x - lx, ex - pos.x));
        h = (float)fmin(h, 2 * fmin(pos.z - lz, ez - pos.z));
    }
    if (c->sprite_count >= SC_CHUNK_SPRITES)
        return;
    c->sprites[c->sprite_count++] =
        (EnvSprite){pos,
                    normal,
                    w,
                    h,
                    world_hash(b->world->seed, c->cell, 0x53505200u + salt),
                    (uint8_t)kind,
                    r->habitat.atmosphere};
}
static void vignette(Builder *b, const Room *r) {
    /* An accessible side recess off the east-west cross. These are physical
       accounts, repeated in each tile with seed-varied evidence and survivors. */
    Color iron = rgb(.18f, .21f, .20f), bone = rgb(.40f, .36f, .24f),
          tint = sc_atmospheres[r->habitat.atmosphere].tint;
    double x = (r->hash & 1) ? 18 : -18, z = 7;
    switch (r->habitat.tale) {
    case LAST_FEAST:
        box(b, v3(x, .85, z), v3(7, .25, 2), bone, 0, true);
        for (int i = 0; i < 4; i++) {
            box(b, v3(x - 2.4 + i * 1.6, 1.01, z), v3(.65, .07, .65), iron, .1f, false);
            box(b, v3(x - 2.4 + i * 1.6, .4, z + 2), v3(.8, .8, .8), iron, 0, true);
        }
        break;
    case TRIAGE:
        for (int i = 0; i < 3; i++) {
            box(b, v3(x - 3 + i * 3, .55, z), v3(2, .5, 3), iron, 0, true);
            sprite(b, SPR_CLOTH, v3(x - 3 + i * 3, .82, z), v3(0, 1, 0), 1.8f, 2.8f, r, 20 + i);
        }
        break;
    case CONTAINMENT:
        for (int i = 0; i < 3; i++) {
            box(b, v3(x - 2 + i * 2, 1.3, z), v3(1.4, 2.6, 1.6), iron, 0, true);
            box(b, v3(x - 2 + i * 2, 1.3, z - .82), v3(1, 1.9, .08), tint, .7f, false);
        }
        break;
    case REPAIR:
        box(b, v3(x, 1, z), v3(5, 2, 3), iron, 0, true);
        for (int i = 0; i < 4; i++)
            box(b, v3(x - 1.5 + i, 2.3, z), v3(.35, .6, 2), bone, 0, true);
        sprite(b, SPR_SPARK, v3(x, 2.5, z - 1.7), v3(0, 0, 0), 1, 1, r, 24);
        break;
    case PILGRIMAGE:
        box(b, v3(x, .55, z), v3(5, 1.1, 2), bone, 0, true);
        for (int i = 0; i < 5; i++)
            box(b, v3(x - 2 + i, 1.4, z), v3(.12, .6, .12), tint, 1.3f, false);
        sprite(b, SPR_GLYPH, v3(x, 3, z), v3(0, 0, -1), 3, 3, r, 25);
        break;
    case MUTINY:
        for (int i = 0; i < 3; i++)
            box(b, v3(x - 2 + i * 2, .65, z), v3(1.5, 1.3, 2), iron, 0, true);
        sprite(b, SPR_CLOTH, v3(x, 2.7, z), v3(0, 0, -1), 3, 3, r, 26);
        break;
    case CULTIVATION:
        box(b, v3(x, .3, z), v3(6, .6, 3), iron, 0, true);
        for (int i = 0; i < 3; i++)
            sprite(b, SPR_FUNGUS, v3(x - 2 + i * 2, 1.4, z), v3(0, 0, -1), 2, 2.2f, r, 27 + i);
        break;
    default:
        for (int i = 0; i < 4; i++)
            box(b, v3(x - 3 + i * 2, .35, z + (i & 1)), v3(1.1, .7, 1.2), bone, 0, true);
        break;
    }
    sprite(b, SPR_PAPERS, v3(x, .045, z - 2), v3(0, 1, 0), 4, 2, r, 31);
    sprite(b, SPR_BONES, v3(x + 3, .06, z + 3), v3(0, 1, 0), 2, 1, r, 32);
}
void habitat_build(Builder *b, const Room *r) {
    Color metal = rgb(.16f, .19f, .20f), tint = sc_atmospheres[r->habitat.atmosphere].tint;
    double height = r->height;
    if (r->habitat.form) {
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2) {
                box(b, v3(sx * 40, height * .5, sz * 40), v3(2, height, 2), metal, 0, true);
                box(b, v3(sx * 40, 2, sz * 40), v3(4, 4, 4), metal, 0, true);
                for (int j = 0; j < 5; j++) {
                    box(b, v3(sx * (36 - j * 4), height - 6 + j, sz * 40), v3(7, 1.5, 2), metal, 0,
                        false);
                    box(b, v3(sx * 40, height - 6 + j, sz * (36 - j * 4)), v3(2, 1.5, 7), metal, 0,
                        false);
                }
                box(b, v3(sx * 39, 12, sz * 39), v3(.3, 6, .3), tint, 1, false);
                /* Faint strips reveal the full height and repeated spans in darkness. */
                box(b, v3(sx * 38.9, height * .5, sz * 40), v3(.12, height * .86, .35), tint, .23f,
                    false);
                box(b, v3(sx * 20, height - 2, sz * 40), v3(36, .16, .35), tint, .16f, false);
            }
        if (r->habitat.form == 3 || r->habitat.form == 6) {
            for (int j = 0; j < 5; j++) {
                box(b, v3(22, 18 + j * 6, 22), v3(15, 2, 15), metal, 0, true);
                box(b, v3(22, 19.1 + j * 6, 22), v3(14, .2, 14), tint, .4f, false);
            }
        }
        if (r->landmark == SUSPENDED_ENGINE) {
            box(b, v3(24, height - 12, 24), v3(18, 12, 18), metal, 0, true);
            for (int j = 0; j < 5; j++)
                box(b, v3(24, height - 17 + j * 2.4, 24), v3(19, .35, 19), tint, .45f, false);
            box(b, v3(24, height - 3, 24), v3(3, 6, 3), metal, 0, true);
        } else if (r->landmark == RELIQUARY) {
            for (int j = 0; j < 7; j++)
                box(b, v3(-24, height - 20 + j * 2, -24), v3(14 - j * 1.5, 1.6, 14 - j * 1.5),
                    rgb(.32f, .26f, .18f), j % 2 ? .15f : 0, false);
            box(b, v3(-24, height - 4, -24), v3(.25, 8, .25), tint, 1, false);
        } else if (r->landmark == CONSERVATORY) {
            for (int j = 0; j < 4; j++) {
                box(b, v3(-25 + j * 2, height * .35, -25), v3(.8, height * .7, .8),
                    rgb(.17f, .23f, .12f), 0, true);
                sprite(b, SPR_FUNGUS, v3(-25 + j * 2, height * .55, -25), v3(0, 0, 0), 12, 12, r,
                       60 + j);
            }
        } else if (r->landmark == BANQUET && r->habitat.volume != 2) {
            for (int side = -1; side <= 1; side += 2) {
                box(b, v3(side * 25, 1, 24), v3(5, .5, 24), rgb(.29f, .23f, .16f), 0, true);
                for (int j = 0; j < 8; j++) {
                    box(b, v3(side * 29, .5, 14 + j * 3), v3(1, 1, 1), metal, 0, true);
                    sprite(b, SPR_GLYPH, v3(side * 25, 1.27, 14 + j * 3), v3(0, 1, 0), 1, 1, r,
                           70 + j);
                }
            }
        }
        if (b->chunk->light_count < SC_CHUNK_LIGHTS)
            b->chunk->lights[b->chunk->light_count++] =
                (PointLight){v3(0, 5, 5), tint, 34, 1.4f, 1};
    }
    if (r->type == BAY || height < 3)
        return;
    vignette(b, r);
    /* Keep new anchors in the central supported strip, clear of doors and the
       service shaft. Wide water patches stay on the lower floor. */
    for (int i = 0; i < 8; i++) {
        uint32_t h = world_hash(b->world->seed, b->chunk->cell, 0x44454300u + (unsigned)i);
        double x = (i & 1 ? 1 : -1) * (8 + (h % 260) / 10.0),
               z = (i & 2 ? 1 : -1) * (2 + (h >> 12) % 30 / 10.0);
        sprite(b,
               (i % 3 == 0   ? SPR_BONES
                : i % 3 == 1 ? SPR_PAPERS
                             : SPR_GLYPH),
               v3(x, .035, z), v3(0, 1, 0), 2, 2, r, (unsigned)i);
    }
    unsigned climate = r->habitat.atmosphere;
    if (climate == FLOODED || climate == RIME_AIR || climate == VOID_GLOW ||
        climate == BOILING_AIR || climate == FROZEN_RUNOFF) {
        sprite(b, SPR_WATER, v3(-20, .035, 3), v3(0, 1, 0), 26, 5, r, 40);
        if (climate == BOILING_AIR)
            sprite(b, SPR_BUBBLE, v3(-20, .1, 3), v3(0, 0, 0), 24, 2, r, 41);
        else if (climate != FROZEN_RUNOFF)
            sprite(b, SPR_RAIN, v3(-20, fmin(height - 1, 25), 3), v3(0, 0, 0), 24,
                   (float)fmin(height - 1, 25), r, 41);
    }
    if (climate == STEAM_LEAK || climate == RIME_AIR || climate == PRESSURE_AIR ||
        climate == BOILING_AIR) {
        box(b, v3(7, .4, 18), v3(1, .8, 1), metal, 0, true);
        sprite(b, SPR_STEAM, v3(7, 1, 18), v3(0, 0, 0), 3, 7, r, 42);
    }
    if (climate == SPORE_AIR) {
        for (int i = 0; i < 4; i++)
            sprite(b, SPR_FUNGUS, v3(-28 + i * 3, 1, 7), v3(0, 0, -1), 2, 2, r, 43 + i);
    }
    if (climate == HEAT_HAZE || climate == COLD_CLEAR || climate == FROZEN_RUNOFF) {
        for (int i = 0; i < 4; i++)
            sprite(b, climate == HEAT_HAZE ? SPR_ASH : SPR_CRYSTAL,
                   v3(-28 + i * 3, .045, 3), v3(0, 1, 0), 1.6f, 2.4f, r, 80 + i);
    } else if (climate != STILL_AIR) {
        sprite(b, SPR_MIST, v3(0, .7, 16), v3(0, 0, 0), 18, 2, r, 48);
        sprite(b, climate == EMBER_AIR ? SPR_RAIN : climate == ASH_AIR ? SPR_ASH
                   : climate == ICE_SHARDS ? SPR_CRYSTAL : SPR_MOTE,
               v3(0, 9, 12), v3(0, 0, 0), 25, 8, r, 49);
    }
    double banner_top = fmin(height - 2, 10.5);
    box(b, v3(6, (banner_top + height) * .5, 15), v3(.08, height - banner_top, .08), metal, 0,
        false);
    box(b, v3(6, banner_top, 15), v3(2.2, .1, .1), metal, 0, false);
    sprite(b, SPR_CLOTH, v3(6, banner_top - 2.5, 15), v3(0, 0, -1), 2, 5, r, 50);
}
unsigned habitat_enemy(const Room *r, unsigned difficulty, uint32_t hash) {
    /* Weighted rosters retain mixed silhouettes/capabilities within an ecology.
       Every roster contains a tier-zero fallback; guardians are assigned later. */
    static const uint8_t pools[ECOLOGIES][10] = {
        {0, 2, 4, 11, 13, 18, 20, 22, 24, 3},  {27, 27, 16, 28, 29, 30, 31, 6, 2, 3},
        {0, 0, 1, 3, 8, 9, 19, 23, 25, 4},     {14, 14, 22, 15, 27, 6, 12, 7, 0, 3},
        {11, 11, 12, 15, 17, 20, 24, 7, 1, 4}, {1, 3, 4, 8, 15, 22, 18, 28, 0, 2}};
    for (unsigned i = 0; i < 10; i++) {
        unsigned kind = pools[r->habitat.ecology][(hash + i) % 10];
        if (sc_enemies[kind].tier <= difficulty && (r->height >= 3 || sc_enemies[kind].size <= .8f))
            return kind;
    }
    return 3;
}
bool habitat_spawn(const World *w, const Room *r, Vec3 p) {
    Cell cell = world_cell(p);
    if (!habitat_owns(r->cell, r->span_x, r->span_z, r->habitat.mask, cell))
        return false;
    Vec3 local = vsub(p, world_center(cell));
    /* Keep population outside traversal voids and neighbor seams. This also
       makes support independent of which adjacent deck streams first. */
    double shaft = world_service_column(w->seed, cell) ? 15 : 4;
    if (fabs(local.x) > 46.5 || fabs(local.z) > 46.5 ||
        (fabs(local.x) < shaft && local.z < -5 && local.z > (shaft > 4 ? -35 : -14)))
        return false;
    return world_spawn_supported(w, p, fmin(1.8, r->height - .1));
}
void habitat_account(const RoomSpec *r, char *text, size_t size) {
    if (r->type == BAY) {
        snprintf(
            text, size,
            "ARRIVAL / THE THRESHOLD. Broken fittings frame a deserted loading floor. The open bay "
            "still provides a safe foothold; the incident trails begin beyond its doors.");
        return;
    }
    if (r->height < 3) {
        snprintf(text, size,
                 "MAINTENANCE TRACE / %s. Ribbed casings and damaged conduit fittings carry the "
                 "district's incident trail into this crawlspace. Conditions: %s. Signs of %s.",
                 sc_themes[r->theme].name, sc_atmospheres[r->habitat.atmosphere].name,
                 sc_ecologies[r->habitat.ecology]);
        return;
    }
    static const char *titles[LOCAL_TALES] = {
        "THE QUEUE THAT NEVER BOARDED", "DINNER AFTER THE ALARM",
        "A WARD WITHOUT A DOCTOR",      "THE BROKEN SEAL",
        "THE LAST MAINTENANCE SHIFT",   "A LIGHT KEPT BURNING",
        "ORDERS AGAINST THE CREW",      "THE GARDEN THAT ANSWERED"};
    static const char *evidence[LOCAL_TALES] = {
        "Abandoned luggage and scattered manifests mark a queue towards the loading decks.",
        "Places are still set. Chairs face the exits; nobody finished this meal.",
        "Covered cots crowd a makeshift ward. Supplies were sorted before the staff vanished.",
        "Specimen cabinets stand beside discarded notices. The quarantine came too late.",
        "An open machine and a shower of sparks mark a repair nobody returned to finish.",
        "A row of votive lights survives beside a discarded pilgrim's banner.",
        "Crates form a barricade beneath a torn standard. The command deck turned on its crew.",
        "Fungal beds have overtaken their planters. The cultivated organism is feeding itself."};
    snprintf(text, size, "%s / %s. %s Conditions: %s. Signs of %s.", titles[r->habitat.tale],
             sc_themes[r->theme].name, evidence[r->habitat.tale],
             sc_atmospheres[r->habitat.atmosphere].name, sc_ecologies[r->habitat.ecology]);
}
Color habitat_enemy_color(const Enemy *e) {
    Color c = sc_enemies[e->kind].color;
    if (!e->ecology)
        return c;
    static const Color coats[ECOLOGIES] = {{.24f, .15f, .13f}, {.22f, .27f, .28f},
                                           {.17f, .26f, .10f}, {.17f, .30f, .37f},
                                           {.26f, .14f, .34f}, {.16f, .27f, .25f}};
    Color tint = coats[e->ecology - 1];
    return rgb(c.r * .55f + tint.r * .45f, c.g * .55f + tint.g * .45f, c.b * .55f + tint.b * .45f);
}
