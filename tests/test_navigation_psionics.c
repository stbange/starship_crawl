#include "game.h"
#include "legacy_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        checks++;                                                                                  \
        if (!(x)) {                                                                                \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                           \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static void settle(Game *g) {
    Vec3 p[2] = {position_absolute(g->players[0].pos), position_absolute(g->players[1].pos)};
    CHECK(world_refresh(&g->world, p, g->player_count));
    for (int i = 0; g->world.pending && i < SC_MAX_CHUNKS; i++)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed && !g->world.pending);
}
static Game *fresh(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    legacy_game_init(g, 713);
    g->players[0].pos = position_from(v3(96, .02, 24));
    settle(g);
    for (int i = 0; i < g->world.count; i++) {
        g->world.rooms[i].enemy_count = 0;
        g->world.rooms[i].pickup_count = 0;
    }
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void tick(Game *g, Actions a, int frames) {
    Actions all[2] = {a, {0}};
    for (int i = 0; i < frames; i++)
        game_step(g, all, 1.0 / 60);
}
static Enemy *enemy(Game *g, int kind, Vec3 feet) {
    Room *r = world_find(&g->world, (Cell){1, 0, 0});
    CHECK(r && r->enemy_count < SC_MAX_ENEMIES);
    int at = r->enemy_count++;
    Enemy *e = &r->enemies[at];
    *e = (Enemy){.pos = position_from(feet),
                 .health = 1000,
                 .cooldown = 100,
                 .kind = (uint8_t)kind,
                 .id = (uint8_t)at,
                 .alive = true,
                 .normal = {0, 1, 0},
                 .heading = {0, 0, -1}};
    return e;
}
static void test_maps(void) {
    Game *g = fresh();
    MapView map;
    Player *p = &g->players[0];
    CHECK(!game_map_build(g, -1, (Cell){0}, 2, &map) && map.side == 0);
    CHECK(!game_map_build(g, 2, (Cell){0}, 2, &map));
    game_map_toggle(g, -1);
    game_map_toggle(g, 2);
    CHECK(!PLAYER_VIEW(g, p).map_open);
    game_map_pan(g, 0, 1, 1, 1);
    CHECK(!PLAYER_VIEW(g, p).map_open);
    game_map_toggle(g, 0);
    CHECK(PLAYER_VIEW(g, p).map_open &&
          cell_equal(PLAYER_VIEW(g, p).map_center, world_cell(position_absolute(p->pos))));
    game_map_pan(g, -1, 1, 1, 1);
    game_map_pan(g, 2, 1, 1, 1);
    Cell old = PLAYER_VIEW(g, p).map_center;
    game_map_pan(g, 0, 4, -4, 7);
    CHECK(PLAYER_VIEW(g, p).map_center.x == old.x + 1 &&
          PLAYER_VIEW(g, p).map_center.y == old.y - 1 &&
          PLAYER_VIEW(g, p).map_center.z == old.z + 1);
    PLAYER_VIEW(g, p).map_center = (Cell){9000000, 4000000, 9000000};
    game_map_pan(g, 0, 1, 1, 1);
    CHECK(cell_equal(PLAYER_VIEW(g, p).map_center, (Cell){9000000, 4000000, 9000000}));
    PLAYER_VIEW(g, p).map_center = (Cell){-9000000, -4000000, 0};
    game_map_pan(g, 0, -1, -1, -1);
    CHECK(cell_equal(PLAYER_VIEW(g, p).map_center, (Cell){-9000000, -4000000, 0}));
    const Cell centers[] = {{0, 0, 0}, {3, 1, 3}, {-3, -1, 6}, {3000000, 2000, 3000000}};
    unsigned generated = g->world.generated, revision = g->world.revision;
    for (unsigned seed = 1; seed <= 3; seed++)
        for (unsigned c = 0; c < sizeof(centers) / sizeof(centers[0]); c++) {
            g->world.seed = seed;
            CHECK(game_map_build(g, 0, centers[c], 4, &map));
            CHECK(map.side == 9 && map.plans <= 9);
            for (int z = 0; z < map.side; z++)
                for (int x = 0; x < map.side; x++) {
                    int at = z * map.side + x;
                    Cell cell = {map.origin.x + x, map.origin.y, map.origin.z + z};
                    CHECK(map.valid[at] == (cell.z >= 0));
                    if (!map.valid[at])
                        continue;
                    RoomSpec r = world_describe_version(seed, g->world.generator, cell);
                    CHECK(cell_equal(r.cell, map.rooms[at].cell) &&
                          r.theme == map.rooms[at].theme && r.span_x == map.rooms[at].span_x);
                    for (int d = 0; d < 4; d++)
                        CHECK(!!(map.doors[at] & (1 << d)) ==
                              world_door_version(seed, g->world.generator, cell, (Direction)d));
                }
        }
    CHECK(g->world.generated == generated && g->world.revision == revision);
    CHECK(game_map_build(g, 0, (Cell){0}, 0, &map) && map.side == 3 && map.has_objective);
    CHECK(cell_equal(map.objective, (Cell){0, 0, 0}));
    g->story = (Story){0}; /* Legacy objectives retain their original map route. */
    g->mission = 2;
    g->shelter.deployed = true;
    g->shelter.door = p->pos;
    CHECK(game_map_build(g, 0, (Cell){0}, 99, &map) && map.side == 9 && map.has_shelter &&
          map.objective.y == 1);
    CHECK(cell_equal(map.shelter, world_cell(position_absolute(p->pos))));
    g->mission = 6;
    CHECK(game_map_build(g, 0, (Cell){0}, 2, &map) && !map.has_objective);
    game_join(g);
    game_map_toggle(g, 1);
    CHECK(g->views[1].map_open);
    game_map_toggle(g, 0);
    CHECK(!PLAYER_VIEW(g, p).map_open && g->views[1].map_open);
    destroy(g);
}
static void test_catalog_and_projectiles(void) {
    unsigned mask = 0;
    for (int i = 0; i < SC_ENEMY_TYPES; i++) {
        EnemyBody b = sc_enemy_bodies[i];
        CHECK(b.form < SC_BODY_FORMS && b.variant < SC_BODY_VARIANTS);
        CHECK(sc_body_names[b.form][0] && sc_body_variants[b.form][b.variant][0]);
        mask |= 1u << (b.form * 5 + b.variant);
    }
    CHECK(mask == ((1u << (SC_BODY_FORMS * SC_BODY_VARIANTS)) - 1));
    for (int weapon = 0; weapon < SC_GENERATED_WEAPONS; weapon++) {
        Game *g = fresh();
        Player *p = &g->players[0];
        p->weapon = (unsigned)weapon;
        Vec3 feet = position_absolute(p->pos);
        Enemy *e = enemy(g, 16, vadd(feet, v3(0, 0, 10)));
        p->pitch = atan2(.65 - 1.62, 10);
        CHECK((sc_weapons[weapon].flags & W_PROJECTILE) && sc_projectile_speeds[weapon] > 0);
        tick(g, (Actions){.fire = true}, 1);
        CHECK(g->shots == 1 && e->health == 1000);
        int shots = 0;
        for (int i = 0; i < SC_MAX_PROJECTILES; i++)
            if (g->projectiles[i].active) {
                shots++;
                CHECK(g->projectiles[i].owner == 0 && g->projectiles[i].life > 0);
            }
        CHECK(shots == sc_weapons[weapon].pellets);
        tick(g, (Actions){0}, 120);
        CHECK(e->health < 1000 && p->energy >= 0);
        destroy(g);
    }
    Game *g = fresh();
    Player *p = &g->players[0];
    p->weapon = 3;
    p->pitch = -.12;
    Vec3 at = position_absolute(p->pos);
    Enemy *a = enemy(g, 16, vadd(at, v3(0, 0, 4))), *b = enemy(g, 16, vadd(at, v3(0, 0, 7))),
          *c = enemy(g, 16, vadd(at, v3(0, 0, 10)));
    tick(g, (Actions){.fire = true}, 1);
    tick(g, (Actions){0}, 30);
    CHECK(a->health < 1000 && b->health < 1000 && c->health < 1000 && g->hits == 3);
    destroy(g);
}
static void test_veil_and_costs(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    Vec3 at = position_absolute(p->pos);
    Enemy *e = enemy(g, 1, vadd(at, v3(0, 0, 10)));
    e->cooldown = 0;
    game_psionic(g, -1, PSI_VEIL);
    game_psionic(g, 2, PSI_VEIL);
    game_psionic(g, 0, (Psionic)-1);
    game_psionic(g, 0, (Psionic)4);
    CHECK(p->energy == 100 && !p->cloaked);
    p->energy = 0;
    game_psionic(g, 0, PSI_VEIL);
    CHECK(!p->cloaked && p->energy == 0);
    p->health = 0;
    game_psionic(g, 0, PSI_FLIGHT);
    CHECK(p->flight == 0);
    p->health = 100;
    p->energy = 100;
    game_psionic(g, 0, PSI_VEIL);
    CHECK(p->cloaked && p->energy == 75);
    tick(g, (Actions){0}, 60);
    CHECK(p->cloaked && e->windup == 0 && g->enemy_shots == 0);
    g->projectiles[0] = (Projectile){.pos = position_from(vadd(player_eye(p), v3(0, 0, -2))),
                                     .velocity = {0, 0, 100},
                                     .life = 1,
                                     .damage = 10,
                                     .active = true,
                                     .owner = -1};
    tick(g, (Actions){0}, 3);
    CHECK(p->cloaked && g->projectile_hits == 1 && p->shield < 50);
    p->weapon = 1;
    p->cooldown = 0;
    p->energy = 0;
    tick(g, (Actions){.fire = true}, 1);
    CHECK(p->cloaked && g->shots == 0);
    p->weapon = 0;
    p->energy = 100;
    p->cooldown = 0;
    for (int i = 0; i < SC_MAX_PROJECTILES; i++)
        g->projectiles[i] = (Projectile){.pos = p->pos, .life = 1, .active = true, .owner = 0};
    tick(g, (Actions){.fire = true}, 1);
    CHECK(p->cloaked && g->shots == 0);
    memset(g->projectiles, 0, sizeof(g->projectiles));
    p->cooldown = 0;
    tick(g, (Actions){.fire = true}, 1);
    CHECK(!p->cloaked && g->shots == 1);
    game_psionic(g, 0, PSI_VEIL);
    CHECK(p->cloaked);
    float energy = p->energy;
    game_psionic(g, 0, PSI_VEIL);
    CHECK(!p->cloaked && p->energy == energy);
    destroy(g);
}
static double jump_peak(bool charged, double *travel) {
    Game *g = fresh();
    Player *p = &g->players[0];
    tick(g, (Actions){0}, 5);
    if (charged)
        game_psionic(g, 0, PSI_LEAP);
    double max = position_absolute(p->pos).y, start = position_absolute(p->pos).z;
    tick(g, (Actions){.jump = true, .forward = 1}, 1);
    for (int i = 0; i < 90; i++) {
        tick(g, (Actions){.forward = 1}, 1);
        max = fmax(max, position_absolute(p->pos).y);
    }
    *travel = position_absolute(p->pos).z - start;
    destroy(g);
    return max;
}
static void test_mobility_and_nova(void) {
    double normal, boost;
    double low = jump_peak(false, &normal), high = jump_peak(true, &boost);
    CHECK(high > low + 1 && boost > normal + 1);
    Game *g = fresh();
    Player *p = &g->players[0];
    tick(g, (Actions){0}, 5);
    double start = position_absolute(p->pos).y;
    game_psionic(g, 0, PSI_FLIGHT);
    CHECK(p->flight == 18 && p->energy == 80);
    tick(g, (Actions){.jump = true}, 600);
    CHECK(position_absolute(p->pos).y > start + 2);
    RoomSpec r = world_describe(g->world.seed, world_cell(position_absolute(p->pos)));
    CHECK(position_absolute(p->pos).y + 1.78 <= r.height + .03);
    CHECK(!world_overlap(&g->world, position_absolute(p->pos), .34, 1.78));
    p->flight = 18;
    tick(g, (Actions){.strafe = 1}, 600);
    CHECK(!world_overlap(&g->world, position_absolute(p->pos), .34, 1.78));
    tick(g, (Actions){.crouch = true}, 60);
    CHECK(isfinite(position_absolute(p->pos).y));
    game_psionic(g, 0, PSI_FLIGHT);
    CHECK(p->flight == 0);
    game_psionic(g, 0, PSI_FLIGHT);
    p->flight = .01f;
    p->leap = .01f;
    tick(g, (Actions){0}, 1);
    CHECK(p->flight == 0 && p->leap == 0);
    game_psionic(g, 0, PSI_LEAP);
    CHECK(p->leap == 45);
    game_psionic(g, 0, PSI_LEAP);
    CHECK(p->leap == 0);
    p->pos = position_from(v3(96, .02, 24));
    p->energy = 100;
    p->velocity = v3(0, 0, 0);
    settle(g);
    Vec3 at = position_absolute(p->pos);
    Enemy *near = enemy(g, 16, vadd(at, v3(0, 0, 3))), *far = enemy(g, 16, vadd(at, v3(0, 0, 12)));
    game_join(g);
    float hp = g->players[1].health;
    game_psionic(g, 0, PSI_VEIL);
    game_psionic(g, 0, PSI_NOVA);
    CHECK(!p->cloaked && p->nova == 8 && p->energy == 40);
    tick(g, (Actions){0}, 1);
    CHECK(near->health < 1000 && far->health == 1000 && g->players[1].health == hp &&
          p->nova_flash > 0);
    unsigned hits = g->hits;
    tick(g, (Actions){0}, 10);
    CHECK(g->hits == hits);
    tick(g, (Actions){0}, 25);
    CHECK(g->hits > hits);
    game_psionic(g, 0, PSI_VEIL);
    CHECK(p->cloaked && p->nova == 0 && p->nova_tick == 0);
    p->energy = 100;
    game_psionic(g, 0, PSI_NOVA);
    game_psionic(g, 0, PSI_NOVA);
    CHECK(p->nova == 0);
    p->nova = .01f;
    p->nova_tick = .1f;
    tick(g, (Actions){0}, 1);
    CHECK(p->nova == 0 && p->nova_tick == 0);
    game_nova(g, -1);
    game_nova(g, 2);
    destroy(g);
}
static void test_nova_occlusion(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    RoomSpec r = world_describe(g->world.seed, (Cell){1, 0, 0});
    Vec3 wall = vadd(world_center(r.cell), v3(r.width * .5, .02, 12));
    p->pos = position_from(vadd(wall, v3(-1.8, 0, 0)));
    Enemy *hidden = enemy(g, 16, vadd(wall, v3(1.8, 0, 0)));
    Vec3 d = vsub(enemy_center(hidden), player_eye(p));
    CHECK(vlen(d) < 7 && world_raycast(&g->world, player_eye(p), vnorm(d), vlen(d)) < vlen(d) - .1);
    game_nova(g, 0);
    CHECK(hidden->health == 1000);
    destroy(g);
}
static void test_sanctuary_and_save(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    p->pos = position_from(v3(0, .02, 12));
    settle(g);
    game_summon(g, 0);
    game_interact(g, 0);
    CHECK(p->sheltered);
    float energy = p->energy;
    for (int power = 0; power < 4; power++)
        game_psionic(g, 0, (Psionic)power);
    CHECK(p->energy == energy && !p->cloaked && !p->flight && !p->nova);
    p->nova = 1;
    unsigned hits = g->hits;
    tick(g, (Actions){0}, 1);
    game_nova(g, 0);
    CHECK(g->hits == hits && p->nova < 1);
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v4.sav"));
    CHECK(g->world.seed == 713 && p->level == 7 && g->mission == 2 && p->sheltered &&
          g->shelter.deployed && g->dirty);
    CHECK(!p->cloaked && p->flight == 0 && p->leap == 0 && p->nova == 0);
    p->cloaked = true;
    p->flight = 12;
    p->leap = 30;
    CHECK(game_save(g, "sc-psionics.sav"));
    game_new_ship(g, 99);
    CHECK(!p->cloaked && !p->flight && !p->leap && !p->nova);
    CHECK(game_load(g, "sc-psionics.sav"));
    CHECK(g->world.seed == 713 && p->sheltered && p->cloaked && p->flight == 12 && p->leap == 30 &&
          !g->dirty);
    p->cloaked = false;
    p->nova = 4;
    p->nova_tick = .25f;
    CHECK(game_save(g, "sc-psionics.sav"));
    CHECK(game_load(g, "sc-psionics.sav"));
    CHECK(p->nova == 4 && p->nova_tick == .25f);
    remove("sc-psionics.sav");
    destroy(g);
}
int main(void) {
    test_maps();
    test_catalog_and_projectiles();
    test_veil_and_costs();
    test_mobility_and_nova();
    test_nova_occlusion();
    test_sanctuary_and_save();
    printf("PASS %u navigation, body, projectile and psionic assertions\n", checks);
    return 0;
}
