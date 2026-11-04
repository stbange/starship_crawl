#include "game.h"
#include "interior.h"
#include "geometry_fingerprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                           \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

static uint32_t conduit_seed;
static Cell conduit_cell;

static void pump(World *w, Cell cell) {
    Vec3 p = vadd(world_center(cell), v3(0, .02, 0));
    CHECK(world_refresh(w, &p, 1));
    for (int frames = 0; w->pending && frames < 192; frames++) {
        CHECK(world_stream(w, SC_STREAM_BUDGET) <= SC_STREAM_BUDGET);
        CHECK(!w->failed);
    }
    CHECK(!w->pending && world_ready(w, cell));
}
static void walk(World *w, Vec3 a, Vec3 b) {
    double distance = vlen(vsub(b, a));
    int samples = (int)ceil(distance / .3) + 1;
    for (int i = 0; i <= samples; i++) {
        Vec3 p = vadd(a, vmul(vsub(b, a), (double)i / samples));
        if (world_overlap(w, p, .8, 2.8)) {
            fprintf(stderr, "Blocked seed %u at %.2f,%.2f,%.2f\n", w->seed, p.x, p.y, p.z);
            CHECK(false);
        }
        CHECK(world_overlap(w, vadd(p, v3(0, -.1, 0)), .34, .1));
    }
}
static void test_plans(void) {
    unsigned profiles = 0, layouts = 0, incidents = 0, landmarks = 0;
    int rare = 0, halls = 0;
    for (uint32_t seed = 1; seed <= 128; seed++) {
        profiles |= 1u << world_profile(seed);
        CHECK(strlen(world_profile_name(world_profile(seed))) > 4);
        for (int y = -2; y <= 2; y++) {
            RegionPlan p;
            world_plan(seed, (Cell){2, y, 2}, &p);
            int source = -1, affected = 0;
            for (int i = 0; i < p.count; i++) {
                const RoomSpec *r = &p.rooms[i];
                layouts |= 1u << r->layout;
                incidents |= 1u << r->incident;
                landmarks |= 1u << r->landmark;
                CHECK(r->layout < INTERIOR_LAYOUTS && r->incident < INCIDENT_TYPES);
                CHECK(r->intensity <= 3 && (r->incident == QUIET) == (r->intensity == 0));
                CHECK(strlen(world_layout_name(r->layout)) > 3);
                CHECK(strlen(world_incident_name(r->incident)) > 3);
                if (r->intensity == 3) {
                    CHECK(source == -1);
                    source = i;
                }
                affected += r->intensity > 0;
                halls += r->span_x == 2;
                if (r->landmark) {
                    CHECK(r->span_x == 2 && r->span_z == 2 && r->layout == ARCADES);
                    CHECK(strlen(world_landmark_name(r->landmark)) > 8);
                    rare++;
                    if (r->type == CONDUIT) {
                        conduit_seed = seed;
                        conduit_cell = r->cell;
                    }
                }
            }
            CHECK(source >= 0 && affected >= 2);
            /* Follow only affected open connections: every affected room must
               reach its source, with non-increasing intensity along the path. */
            bool seen[16] = {false};
            seen[source] = true;
            for (int pass = 0; pass < p.count; pass++)
                for (int i = 0; i < 16; i++) {
                    int a = p.owner[i];
                    if (!seen[a])
                        continue;
                    for (int d = 0; d < 4; d++) {
                        int x = i % 4 + (d == WEST ? -1 : d == EAST ? 1 : 0);
                        int z = i / 4 + (d == SOUTH ? -1 : d == NORTH ? 1 : 0);
                        if (x < 0 || x >= 4 || z < 0 || z >= 4 || !(p.doors[i] & (1 << d)))
                            continue;
                        int b = p.owner[z * 4 + x];
                        if (p.rooms[b].incident && p.rooms[b].intensity <= p.rooms[a].intensity) {
                            CHECK(p.rooms[b].incident == p.rooms[a].incident);
                            seen[b] = true;
                        }
                    }
                }
            for (int i = 0; i < p.count; i++)
                CHECK(seen[i] == (p.rooms[i].intensity > 0));
        }
        CHECK(world_describe(seed, (Cell){0, 0, 0}).intensity == 0);
    }
    CHECK(profiles == (1u << SHIP_PROFILES) - 1);
    CHECK(layouts == (1u << INTERIOR_LAYOUTS) - 1);
    CHECK(incidents == (1u << INCIDENT_TYPES) - 1);
    CHECK(landmarks == (1u << LANDMARK_TYPES) - 1);
    CHECK(rare > halls / 8 && rare < halls / 2);
    CHECK(conduit_seed);
    puts("PASS 640 plans: ship profiles, floor plans, connected incidents and rare discoveries");
}
static void test_interiors(void) {
    World *w = malloc(sizeof(*w)), *reverse = malloc(sizeof(*reverse));
    CHECK(w && reverse);
    unsigned roles = 0, layouts = 0, incidents = 0, landmarks = 0;
    size_t maximum = 0;
    for (uint32_t seed = 1; seed <= 24; seed++) {
        Cell cell = {(int)seed % 5 * 4 + 8, (int)seed % 9 - 4, (int)seed % 3 * 4 + 8};
        world_init(w, seed);
        world_init(reverse, seed);
        pump(w, cell);
        pump(reverse, world_neighbor(cell, EAST));
        pump(reverse, cell);
        for (int c = 0; c < SC_MAX_CHUNKS; c++) {
            const Chunk *chunk = &w->chunks[c];
            if (!chunk->active)
                continue;
            const Chunk *other = world_chunk(reverse, chunk->cell);
            if (other)
                CHECK(geometry_fingerprint(chunk) == geometry_fingerprint(other));
            CHECK(chunk->box_count <= SC_CHUNK_BOXES && chunk->light_count == SC_CHUNK_LIGHTS);
            CHECK(chunk->dressing_count > 0 && chunk->dressing_count <= SC_CHUNK_DRESSING);
            if (chunk->box_count > maximum)
                maximum = chunk->box_count;
        }
        for (int i = 0; i < w->count; i++) {
            Room *r = &w->rooms[i];
            if (r->type == BAY || r->height < 3)
                continue;
            roles |= 1u << sc_themes[r->theme].sector;
            layouts |= 1u << r->layout;
            incidents |= 1u << r->incident;
            landmarks |= 1u << r->landmark;
            CHECK(r->ready);
            for (int dz = 0; dz < r->span_z; dz++)
                for (int dx = 0; dx < r->span_x; dx++) {
                    Cell c = {r->cell.x + dx, r->cell.y, r->cell.z + dz};
                    Vec3 o = vadd(world_center(c), v3(0, .02, 0));
                    walk(w, vadd(o, v3(-24, 0, 0)), vadd(o, v3(24, 0, 0)));
                    walk(w, vadd(o, v3(0, 0, -24)), vadd(o, v3(0, 0, 24)));
                }
            for (int id = 3; id <= 4; id++) {
                Vec3 p = position_absolute(r->pickups[id].pos);
                CHECK(vlen(vsub(p, interior_cache(r, id))) < .0001);
                Vec3 o = vadd(world_center(r->cell), v3(0, .02, 0));
                Vec3 turn = v3(p.x, o.y, o.z);
                walk(w, o, turn);
                walk(w, turn, v3(p.x, o.y, p.z));
                /* The second entrance reaches the same side room without
                   backtracking through the front doorway. */
                double sign = id == 3 ? 1 : -1;
                Vec3 inner = v3(sign * 10.8 + o.x, o.y, p.z);
                walk(w, v3(p.x, o.y, p.z), inner);
                double side_mid =
                    (9 + fmin(44, (r->span_z - 1) * SC_CELL / 2 + r->depth * .5 - 3)) * .5;
                walk(w, inner, v3(inner.x, o.y, o.z + side_mid));
                walk(w, v3(inner.x, o.y, o.z + side_mid), v3(o.x, o.y, o.z + side_mid));
            }
        }
        world_destroy(w);
        world_destroy(reverse);
    }
    CHECK(roles == (1u << DISTRICT_TYPES) - 1 && layouts == 7);
    CHECK(incidents == (1u << INCIDENT_TYPES) - 1 && landmarks == (1u << LANDMARK_TYPES) - 1);
    printf("PASS furnishings, two entrances, caches, walking/mech clearance, load order; max %zu "
           "boxes\n",
           maximum);
    /* A large undercroft shares the CONDUIT family, but must still receive
       its rare hall structure, overhead light and side-space rewards. */
    world_init(w, conduit_seed);
    pump(w, conduit_cell);
    Room *hall = world_find(w, conduit_cell);
    CHECK(hall && hall->type == CONDUIT && hall->height == 58 && hall->landmark);
    CHECK(vlen(vsub(position_absolute(hall->pickups[3].pos), interior_cache(hall, 3))) == 0);
    CHECK(world_chunk(w, conduit_cell)->lights[5].radius == 58);
    world_destroy(w);
    free(w);
    free(reverse);
}
static void test_legacy(void) {
    FILE *f = fopen(SC_FIXTURE_DIR "/generator-v3-fingerprints.txt", "r");
    CHECK(f);
    unsigned seed, hash;
    Cell c;
    int count = 0;
    while (fscanf(f, "%u %d %d %d %u", &seed, &c.x, &c.y, &c.z, &hash) == 5) {
        CHECK(world_fingerprint_version(seed, 3, c) == hash);
        count++;
    }
    fclose(f);
    CHECK(count == 12);
    f = fopen(SC_FIXTURE_DIR "/generator-v3-geometry.txt", "r");
    CHECK(f);
    World *w = malloc(sizeof(*w));
    CHECK(w);
    count = 0;
    while (fscanf(f, "%u %d %d %d %u", &seed, &c.x, &c.y, &c.z, &hash) == 5) {
        world_init(w, seed);
        w->generator = 3;
        pump(w, c);
        CHECK(geometry_fingerprint(world_chunk(w, c)) == hash);
        world_destroy(w);
        count++;
    }
    fclose(f);
    CHECK(count == 14);
    free(w);
    for (unsigned kind = 0; kind < 2; kind++) {
        f = fopen(kind ? SC_FIXTURE_DIR "/generator-v4-geometry.txt"
                       : SC_FIXTURE_DIR "/generator-v4-fingerprints.txt",
                  "r");
        CHECK(f);
        count = 0;
        while (fscanf(f, "%u %d %d %d %u", &seed, &c.x, &c.y, &c.z, &hash) == 5) {
            if (kind) {
                World *old = malloc(sizeof(*old));
                CHECK(old);
                world_init(old, seed);
                old->generator = 4;
                pump(old, c);
                CHECK(geometry_fingerprint(world_chunk(old, c)) == hash);
                world_destroy(old);
                free(old);
            } else
                CHECK(world_fingerprint_version(seed, 4, c) == hash);
            count++;
        }
        fclose(f);
        CHECK(count == 24);
    }
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    game_init(g, 1);
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v6.sav"));
    CHECK(g->world.generator == 3 && g->world.seed == 713 && g->mission == 2);
    CHECK(g->players[0].level == 7 && g->players[0].sheltered && g->players[0].cloaked);
    CHECK(game_save(g, "interior-legacy.sav"));
    CHECK(game_load(g, "interior-legacy.sav") && g->world.generator == 3);
    MapView map;
    CHECK(game_map_build(g, 0, (Cell){1, 0, 0}, 1, &map));
    for (int i = 0; i < map.side * map.side; i++)
        CHECK(!map.rooms[i].landmark && !map.rooms[i].incident);
    game_new_ship(g, 713);
    CHECK(g->world.generator == SC_GENERATOR_VERSION && g->players[0].level == 7);
    game_destroy(g);
    free(g);
    CHECK(remove("interior-legacy.sav") == 0);
    puts("PASS original generator-3 plans, geometry, map, save round-trip and opt-in new ship");
}
static void test_cache_persistence(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    Cell cell = {1, 0, 0};
    pump(&g->world, cell);
    Room *r = world_find(&g->world, cell);
    Position point = r->pickups[3].pos;
    world_collect(&g->world, r, 3);
    world_collect(&g->world, r, 4);
    pump(&g->world, (Cell){100, 0, 100});
    CHECK(!world_find(&g->world, cell));
    pump(&g->world, cell);
    r = world_find(&g->world, cell);
    CHECK(!r->pickups[3].active && !r->pickups[4].active);
    CHECK(vlen(position_delta(point, r->pickups[3].pos)) == 0);
    g->players[0].pos = point;
    CHECK(game_save(g, "interior-cache.sav"));
    CHECK(game_load(g, "interior-cache.sav"));
    pump(&g->world, cell);
    r = world_find(&g->world, cell);
    CHECK(g->world.generator == SC_GENERATOR_VERSION && !r->pickups[3].active &&
          !r->pickups[4].active);
    CHECK(vlen(position_delta(point, r->pickups[3].pos)) == 0);
    game_destroy(g);
    free(g);
    CHECK(remove("interior-cache.sav") == 0);
    puts("PASS side-space cache identity survives streaming and save/resume");
}
int main(void) {
    test_plans();
    test_interiors();
    test_legacy();
    test_cache_persistence();
    return 0;
}
