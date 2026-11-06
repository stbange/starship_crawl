#include "game.h"
#include "habitat.h"
#include "geometry_fingerprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static void settle(World *w, Cell c) {
    Vec3 at = world_lift(c);
    CHECK(world_refresh(w, &at, 1));
    while (w->pending && !w->failed)
        CHECK(world_stream(w, 2) <= 2);
    CHECK(!w->failed && w->chunk_count <= SC_MAX_CHUNKS);
}
static Game *fresh(unsigned seed) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, seed);
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void legacy(void) {
    Game *g = fresh(713);
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v9-format11.sav"));
    CHECK(g->world.generator == 9 && g->players[0].xp == 17 && g->players[0].credits == 29);
    FILE *f = fopen(SC_FIXTURE_DIR "/generator-v9-geometry.txt", "r");
    CHECK(f);
    unsigned seed, expected, population, count = 0;
    Cell cell;
    while (fscanf(f, "%u %d %d %d %u %u", &seed, &cell.x, &cell.y, &cell.z, &expected,
                  &population) == 6) {
        world_destroy(&g->world);
        world_init(&g->world, seed);
        g->world.generator = 9;
        settle(&g->world, cell);
        const Chunk *c = world_chunk(&g->world, cell);
        CHECK(c && !c->sprite_count && geometry_fingerprint(c) == expected);
        Room *r = world_find(&g->world, cell);
        CHECK(r && !r->habitat.mask && !r->habitat.form);
        uint32_t h = 2166136261u;
        for (int i = 0; i < r->enemy_count; i++) {
            h = fingerprint_word(h, r->enemies[i].kind);
            h = fingerprint_vector(h, position_absolute(r->enemies[i].pos));
            CHECK(!r->enemies[i].ecology);
        }
        CHECK(h == population);
        count++;
    }
    fclose(f);
    CHECK(count == 20);
    destroy(g);
    puts("PASS immutable generator-9 save, geometry and encounter fixtures");
}
static void plans(void) {
    unsigned forms = 0, climates = 0, tales = 0, ecologies = 0, volumes = 0;
    for (unsigned seed = 1; seed <= 48; seed++)
        for (int y = -3; y <= 3; y++) {
            RegionPlan p;
            world_plan(seed, (Cell){-2, y, 2}, &p);
            int seen[16] = {0};
            for (int i = 0; i < p.count; i++) {
                RoomSpec r = p.rooms[i];
                forms |= 1u << r.habitat.form;
                climates |= 1u << r.habitat.atmosphere;
                tales |= 1u << r.habitat.tale;
                ecologies |= 1u << r.habitat.ecology;
                volumes |= 1u << r.habitat.volume;
                int tiles = 0;
                for (int bit = 0; bit < 16; bit++)
                    tiles += (r.habitat.mask >> bit) & 1;
                CHECK(tiles <= 6);
                char account[600];
                habitat_account(&r, account, sizeof(account));
                CHECK(strstr(account, sc_themes[r.theme].name));
                CHECK(strstr(account, sc_atmospheres[r.habitat.atmosphere].name));
                CHECK(strstr(account, sc_ecologies[r.habitat.ecology]));
                CHECK(strlen(account) > 120 && strlen(account) < sizeof(account) - 1);
                for (int z = 0; z < 4; z++)
                    for (int x = 0; x < 4; x++) {
                        Cell c = {p.origin.x + x, y, p.origin.z + z};
                        if (!habitat_owns(r.cell, r.span_x, r.span_z, r.habitat.mask, c))
                            continue;
                        seen[z * 4 + x]++;
                        CHECK(p.owner[z * 4 + x] == i);
                        if (r.habitat.volume) {
                            c.y += r.habitat.volume == 1 ? 1 : -1;
                            RoomSpec other = world_describe(seed, c);
                            CHECK(other.habitat.mask == r.habitat.mask &&
                                  other.habitat.form == r.habitat.form);
                            CHECK(other.habitat.volume == 3 - r.habitat.volume &&
                                  other.cell.x == r.cell.x && other.cell.z == r.cell.z);
                        }
                    }
                if (r.habitat.form)
                    CHECK(r.height == (r.habitat.volume == 1 ? 64 : 58));
            }
            for (int i = 0; i < 16; i++)
                CHECK(seen[i] == 1);
        }
    CHECK(forms == 511 && climates == 127 && tales == 255 && ecologies == 63 && volumes == 7);
    CHECK(!habitat_owns((Cell){0}, 2, 2, 0x13, (Cell){1, 0, 1}));
    CHECK(habitat_owns((Cell){0}, 2, 2, 0, (Cell){1, 0, 1}));
    CHECK(!habitat_owns((Cell){0}, 2, 2, 0, (Cell){0, 1, 0}));
    RoomSpec bay = world_describe(713, (Cell){0});
    char arrival[256];
    habitat_account(&bay, arrival, sizeof(arrival));
    CHECK(strstr(arrival, "safe foothold"));
    puts("PASS paired vault reservations, eight macro forms, all weather/tales/ecologies and "
         "complete masked ownership");
}
static unsigned sprite_fingerprint(const Chunk *c) {
    uint32_t h = geometry_fingerprint(c);
    for (int i = 0; i < c->sprite_count; i++) {
        const EnvSprite *s = &c->sprites[i];
        h = fingerprint_vector(h, s->pos);
        h = fingerprint_vector(h, s->normal);
        h = fingerprint_word(h, s->kind);
        h = fingerprint_word(h, s->atmosphere);
        h = fingerprint_word(h, s->hash);
        h = fingerprint_word(h, llround(s->width * 1000));
        h = fingerprint_word(h, llround(s->height * 1000));
    }
    return h;
}
static void geometry(void) {
    World *a = calloc(1, sizeof(*a)), *b = calloc(1, sizeof(*b));
    CHECK(a && b);
    unsigned forms = 0, climates = 0, tales = 0, sprites = 0;
    size_t maximum = 0;
    for (unsigned seed = 1; seed <= 32; seed++) {
        world_init(a, seed);
        world_init(b, seed);
        Cell center = {-6, (int)(seed % 5) - 2, 10};
        settle(a, center);
        settle(b, (Cell){22, 6, 40});
        settle(b, center);
        for (int i = 0; i < a->count; i++) {
            Room *r = &a->rooms[i];
            if (!r->ready)
                continue;
            forms |= 1u << r->habitat.form;
            climates |= 1u << r->habitat.atmosphere;
            tales |= 1u << r->habitat.tale;
            CHECK(world_find(a, r->cell) == r);
            for (int e = 0; e < r->enemy_count; e++) {
                if (!(habitat_spawn(a, r, position_absolute(r->enemies[e].pos)))) {
                    Vec3 pos = position_absolute(r->enemies[e].pos);
                    fprintf(stderr,
                            "bad spawn seed%u room %d %d %d form%u vol%u e%d kind%d pos %.2f %.2f "
                            "%.2f\n",
                            seed, r->cell.x, r->cell.y, r->cell.z, r->habitat.form,
                            r->habitat.volume, e, r->enemies[e].kind, pos.x, pos.y, pos.z);
                    CHECK(false);
                }
                CHECK(r->enemies[e].ecology == r->habitat.ecology + 1);
                CHECK(sc_enemies[r->enemies[e].kind].tier <= world_difficulty(r->cell));
            }
            if (r->habitat.form)
                for (int e = 3; e < 5; e++)
                    CHECK(!world_overlap(a, position_absolute(r->pickups[e].pos), .15, .15));
        }
        for (int i = 0; i < SC_MAX_CHUNKS; i++)
            if (a->chunks[i].active) {
                const Chunk *c = &a->chunks[i], *d = world_chunk(b, c->cell);
                CHECK(d && sprite_fingerprint(c) == sprite_fingerprint(d));
                CHECK(c->box_count <= SC_CHUNK_BOXES && c->sprite_count <= SC_CHUNK_SPRITES);
                if (c->box_count > maximum)
                    maximum = c->box_count;
                Room *r = world_find(a, c->cell);
                CHECK(r);
                for (int n = 0; n < c->sprite_count; n++) {
                    const EnvSprite *s = &c->sprites[n];
                    sprites |= 1u << s->kind;
                    CHECK(s->width > 0 && s->height > 0 && s->pos.x >= -48 && s->pos.x <= 48 &&
                          s->pos.z >= -48 && s->pos.z <= 48);
                    CHECK(s->atmosphere == r->habitat.atmosphere);
                    if (s->normal.y > .5)
                        CHECK(world_overlap(
                            a, vadd(world_center(c->cell), v3(s->pos.x, s->pos.y - .1, s->pos.z)),
                            .2, .1));
                }
                /* Every service/mission pad and horizontal route stays walkable. */
                CHECK(!world_overlap(a, world_lift(c->cell), .34, 1.4));
                if (r->habitat.form && r->ready) {
                    for (int dir = 0; dir < 4; dir++)
                        if (world_door(a->seed, c->cell, (Direction)dir)) {
                            Vec3 axis = v3(dir == WEST   ? -1
                                           : dir == EAST ? 1
                                                         : 0,
                                           0,
                                           dir == SOUTH   ? -1
                                           : dir == NORTH ? 1
                                                          : 0);
                            for (int step = 0; step < 48; step++) {
                                Vec3 point = vadd(world_center(c->cell),
                                                  vadd(v3(0, .02, 0), vmul(axis, step)));
                                if (world_overlap(a, point, .34, 1.4)) {
                                    fprintf(
                                        stderr,
                                        "blocked port seed%u chunk%d %d %d form%u dir%d step%d\n",
                                        seed, c->cell.x, c->cell.y, c->cell.z, r->habitat.form, dir,
                                        step);
                                    CHECK(false);
                                }
                                CHECK(world_overlap(a, vadd(point, v3(0, -.12, 0)), .3, .1));
                            }
                        }
                }
                if (r->habitat.volume && !r->landmark) {
                    Vec3 void_ray =
                        vadd(world_center(c->cell), v3(24, r->habitat.volume == 1 ? 60 : 4, 24));
                    double hit = world_raycast(
                        a, void_ray, r->habitat.volume == 1 ? v3(0, 1, 0) : v3(0, -1, 0), 8);
                    CHECK(hit == 8);
                    if (r->habitat.volume == 2) {
                        Cell below = c->cell;
                        below.y--;
                        CHECK(world_overlap(a, vadd(world_center(c->cell), v3(24, -.1, 24)), .3,
                                            .1) == !world_ready(a, below));
                        CHECK(world_overlap(a, vadd(world_center(c->cell), v3(10, .1, 25)), .3, 1));
                        CHECK(
                            world_overlap(a, vadd(world_center(c->cell), v3(0, -.1, 25)), .3, .1));
                    }
                }
            }
        world_destroy(a);
        world_destroy(b);
    }
    CHECK(forms == 511 && climates == 127 && tales == 255 && sprites == (1u << SPR_SPRITES) - 1);
    free(a);
    free(b);
    printf("PASS surface support, open vaults/guard rails, reproducible detail, spawn fit; maximum "
           "%zu boxes\n",
           maximum);
}
static void cooperative_streaming(void) {
    World *w = calloc(1, sizeof(*w));
    CHECK(w);
    /* Four touched regions per deck; a six-tile footprint adds at most five
       cells per touched room: (9 + 4*5) * 3 decks * 2 players = 174. */
    for (unsigned seed = 1; seed <= 32; seed++) {
        world_init(w, seed);
        for (int step = 0; step < 4; step++) {
            Cell a = {3 + step % 2, -1, 3 + step / 2}, b = {12003 + step % 2, 21, 12003 + step / 2};
            Vec3 positions[2] = {world_lift(a), world_lift(b)};
            CHECK(world_refresh(w, positions, 2) && w->desired_count <= 174);
            while (w->pending && !w->failed)
                CHECK(world_stream(w, 2) <= 2);
            CHECK(!w->failed && w->chunk_count <= 192 && world_ready(w, a) && world_ready(w, b));
        }
        world_destroy(w);
    }
    free(w);
    puts(
        "PASS separated co-op at region corners within the proven 174-desired/192-resident budget");
}
static void brood_ecology(void) {
    Game *g = fresh(713);
    g->campaign = (Campaign){0};
    g->world.open_layer = 0;
    bool born = false;
    for (int x = 4; x < 16 && !born; x++) {
        settle(&g->world, (Cell){x, 2, 8});
        for (int n = 0; n < g->world.count && !born; n++) {
            Room *r = &g->world.rooms[n];
            if (!r->ready || !r->habitat.form)
                continue;
            for (int i = 0; i < r->enemy_count && !born; i++) {
                Enemy *parent = &r->enemies[i];
                if (!(sc_enemies[parent->kind].flags & BROOD))
                    continue;
                Vec3 at = position_absolute(parent->pos), child = vadd(at, v3(1.5, 0, 1.5));
                if (world_overlap(&g->world, child, .42, 1.25))
                    continue;
                Vec3 observer = vadd(at, v3(4, 0, 0));
                if (!world_spawn_supported(&g->world, observer, 1.8))
                    continue;
                CHECK(r->enemy_count < SC_MAX_ENEMIES);
                int before = r->enemy_count;
                unsigned colony = parent->ecology;
                for (int room = 0; room < g->world.count; room++)
                    for (int e = 0; e < g->world.rooms[room].enemy_count; e++)
                        g->world.rooms[room].enemies[e].alive = false;
                parent->alive = true;
                parent->special = 0;
                g->players[0].pos = position_from(observer);
                Actions actions[2] = {{0}, {0}};
                game_step(g, actions, .02);
                CHECK(r->enemy_count == before + 1 && r->enemies[before].alive &&
                      r->enemies[before].ecology == colony);
                born = true;
            }
        }
    }
    CHECK(born);
    destroy(g);
    puts("PASS macro-room brood spawning retains capacity and colony coloration");
}
static void ecology(void) {
    unsigned all = 0;
    for (unsigned ecology = 0; ecology < ECOLOGIES; ecology++) {
        Room r = {.height = 10, .habitat = {.ecology = (uint8_t)ecology}};
        for (unsigned difficulty = 0; difficulty < 8; difficulty++)
            for (unsigned h = 0; h < 50; h++) {
                unsigned e = habitat_enemy(&r, difficulty, h);
                CHECK(e < SC_ENEMY_TYPES && e != 26 && sc_enemies[e].tier <= difficulty);
                all |= 1u << e;
            }
        r.height = 1.5;
        for (unsigned h = 0; h < 10; h++)
            CHECK(sc_enemies[habitat_enemy(&r, 0, h)].size <= .8f);
        Enemy e = {.kind = 1, .ecology = (uint8_t)(ecology + 1)};
        Color c = habitat_enemy_color(&e);
        CHECK(c.r > 0 && c.g > 0 && c.b > 0);
        e.ecology = 0;
        c = habitat_enemy_color(&e);
        CHECK(c.r == sc_enemies[1].color.r);
    }
    CHECK((all & (1u << 27)) && (all & (1u << 23)) && (all & (1u << 12)) && (all & (1u << 14)));
    puts("PASS matching mixed silhouettes, tier/ceiling limits and coherent palette variation");
}
static RoomSpec find_volume(unsigned seed, unsigned volume) {
    for (int x = 1; x < 5; x++)
        for (int z = 1; z < 5; z++) {
            RegionPlan p;
            world_plan(seed, (Cell){x, volume == 2 ? 3 : 2, z}, &p);
            for (int i = 0; i < p.count; i++)
                if (p.rooms[i].habitat.volume == volume)
                    return p.rooms[i];
        }
    CHECK(false);
    return (RoomSpec){0};
}
static void journey(void) {
    Game *g = fresh(713);
    for (unsigned id = 0; id < 8; id += 2) {
        Cell site = campaign_site(id);
        g->players[0].pos = position_from(game_terminal(site));
        settle(&g->world, site);
        game_interact(g, 0);
        g->campaign.attempts[id].opening = 10;
        Room *boss = world_find(&g->world, site);
        CHECK(boss && boss->ready);
        game_damage_enemy(g, boss, 0, 0, 1000000, 0);
    }
    CHECK(campaign_phase(&g->campaign) == 4);
    for (unsigned volume = 1; volume <= 2; volume++) {
        RoomSpec r = find_volume(713, volume);
        g->players[0].pos = position_from(vadd(world_center(r.cell), v3(0, .02, 0)));
        settle(&g->world, r.cell);
        for (int i = 0; i < g->world.count; i++)
            for (int e = 0; e < g->world.rooms[i].enemy_count; e++)
                g->world.rooms[i].enemies[e].alive = false;
        Vec3 start = position_absolute(g->players[0].pos);
        g->players[0].yaw = SC_PI * .5;
        for (int frame = 0; frame < 1600; frame++) {
            Vec3 pos = position_absolute(g->players[0].pos);
            CHECK(world_refresh(&g->world, &pos, 1));
            world_stream(&g->world, 2);
            Actions a[2] = {{.forward = 1}, {0}};
            game_step(g, a, 1.0 / 60);
            CHECK(!g->world.failed && fabs(position_absolute(g->players[0].pos).y - start.y) < .05);
            if (position_absolute(g->players[0].pos).x - start.x > 100)
                break;
        }
        CHECK(position_absolute(g->players[0].pos).x - start.x > 96);
        CHECK(campaign_valid(&g->campaign, g->players, &g->world));
        CHECK(game_save(g, "habitat-journey.sav"));
        CHECK(game_load(g, "habitat-journey.sav"));
        CHECK(g->world.generator == 10);
        remove("habitat-journey.sav");
    }
    destroy(g);
    puts("PASS normal movement across vault/gallery seams and save/resume");
}
int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--journey")) {
        journey();
        return 0;
    }
    legacy();
    plans();
    ecology();
    geometry();
    cooperative_streaming();
    brood_ecology();
    return 0;
}
