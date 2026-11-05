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
    Vec3 pos[2] = {position_absolute(g->players[0].pos), position_absolute(g->players[1].pos)};
    CHECK(world_refresh(&g->world, pos, g->player_count));
    for (int i = 0; g->world.pending && i < SC_MAX_CHUNKS; i++)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed && !g->world.pending);
}
static Game *fresh(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    legacy_game_init(g, 713);
    g->players[0].pos = position_from(v3(0, .02, 12));
    g->players[0].pitch = 0;
    settle(g);
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void tick(Game *g, Actions a, int frames) {
    Actions actions[2] = {a, {0}};
    for (int i = 0; i < frames; i++)
        game_step(g, actions, 1. / 60);
}
static unsigned bullets(Game *g) {
    unsigned n = 0;
    for (int i = 0; i < SC_MAX_PROJECTILES; i++)
        n += g->projectiles[i].active;
    return n;
}
static unsigned flares(Game *g) {
    unsigned n = 0;
    for (int i = 0; i < SC_MAX_FLARES; i++)
        n += g->flares[i].active;
    return n;
}
static void fire(Game *g) {
    g->players[0].cooldown = 0;
    g->players[0].energy = 100;
    tick(g, (Actions){.fire = true}, 1);
}
static void mount(Game *g, RigKind kind) {
    game_rig_toggle(g, 0, kind);
    CHECK(g->rigs[0][kind - 1].deployed);
    game_interact(g, 0);
    CHECK(g->players[0].riding == (unsigned)kind);
}
static void test_rigs(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    CHECK(game_rig_max_armor(p, MECH) == 600 && game_rig_max_armor(p, WAGON) == 150);
    CHECK(game_body_radius(p) == .34 && game_body_height(p) == 1.78);
    p->crouched = true;
    CHECK(game_body_height(p) == 1);
    p->crouched = false;
    CHECK(!game_rig_interact(g, -1) && !game_rig_interact(g, 2) && !game_rig_interact(g, 0));
    CHECK(game_rig_near(g, -1) == ON_FOOT && game_rig_near(g, 2) == ON_FOOT);
    game_rig_toggle(g, -1, MECH);
    game_rig_toggle(g, 2, MECH);
    game_rig_toggle(g, 0, ON_FOOT);
    game_rig_toggle(g, 0, (RigKind)3);
    p->pos = position_from(v3(0, 4, -20));
    game_rig_toggle(g, 0, MECH);
    CHECK(!g->rigs[0][0].deployed);
    p->pos = position_from(v3(0, .02, 12));
    p->sheltered = true;
    game_rig_toggle(g, 0, MECH);
    CHECK(!g->rigs[0][0].deployed && game_rig_near(g, 0) == ON_FOOT);
    p->sheltered = false;
    p->flight = 10;
    p->cloaked = true;
    p->leap = 20;
    p->nova = 2;
    mount(g, MECH);
    CHECK(!p->cloaked && p->flight == 0 && p->leap == 0 && p->nova == 0);
    CHECK(game_body_radius(p) == .8 && game_body_height(p) == 2.8 &&
          fabs(player_eye(p).y - 2.37) < .001);
    game_rig_toggle(g, 0, WAGON);
    CHECK(!g->rigs[0][1].deployed);
    game_psionic(g, 0, PSI_FLIGHT);
    game_nova(g, 0);
    CHECK(p->flight == 0 && p->nova == 0);
    p->shield = 50;
    float health = p->health, armor = p->armor;
    game_hurt(g, 0, 150, 0);
    CHECK(p->shield == 0 && g->rigs[0][0].armor == 500 && p->health == health && p->armor == armor);
    for (unsigned gun = 0; gun < SC_MECH_WEAPONS; gun++) {
        p->rig_weapon = gun;
        CHECK(game_gun(p) == &sc_mech_weapons[gun]);
        memset(g->projectiles, 0, sizeof(g->projectiles));
        fire(g);
        CHECK(gun == 2 ? g->beams[0].life > 0 && bullets(g) == 0
                       : bullets(g) > 0 && game_shot_speed(p) > 0);
    }
    game_weapon(g, 0, 1);
    CHECK(p->rig_weapon == 0);
    game_weapon(g, 0, -1);
    CHECK(p->rig_weapon == 2);
    Vec3 before = position_absolute(p->pos);
    tick(g, (Actions){.forward = true, .jump = true, .crouch = true}, 30);
    CHECK(position_absolute(p->pos).z > before.z + 2 && position_absolute(p->pos).y < .1 &&
          !p->crouched);
    CHECK(fabs(p->drive_speed - 5.5) < .05 &&
          vlen(position_delta(g->rigs[0][0].pos, p->pos)) < .001);
    game_interact(g, 0);
    CHECK(!p->riding && g->rigs[0][0].deployed);
    CHECK(game_rig_blocks(g, 0, position_absolute(g->rigs[0][0].pos), .34, 1.78));
    CHECK(
        !game_rig_blocks(g, 0, vadd(position_absolute(g->rigs[0][0].pos), v3(0, 5, 0)), .34, 1.78));
    game_rig_toggle(g, 0, MECH);
    CHECK(!g->rigs[0][0].deployed && g->rigs[0][0].armor == 500);
    game_rig_toggle(g, 0, MECH);
    CHECK(g->rigs[0][0].armor == 500);
    game_interact(g, 0);
    CHECK(p->riding == MECH);
    p->shield = 0;
    game_hurt(g, 0, 510, 0);
    CHECK(!p->riding && g->rigs[0][0].armor == 0 && p->health < health);
    game_interact(g, 0);
    CHECK(!p->riding && strstr(g->message, "DEPLETED"));
    game_rig_repair(g, 0);
    CHECK(g->rigs[0][0].armor == 600);
    game_rig_toggle(g, 0, MECH);
    mount(g, WAGON);
    CHECK(game_body_height(p) == 1.6 && game_body_radius(p) == .65 &&
          game_gun(p) == &sc_weapons[p->weapon]);
    CHECK(fabs(player_eye(p).y - 1.27) < .03);
    before = position_absolute(p->pos);
    tick(g, (Actions){.forward = true}, 60);
    CHECK(position_absolute(p->pos).z > before.z + 15 && p->drive_speed > 19);
    tick(g, (Actions){.forward = true, .crouch = true}, 60);
    CHECK(p->drive_speed < .1);
    p->yaw = SC_PI;
    tick(g, (Actions){.forward = true}, 180);
    CHECK(position_absolute(p->pos).z >= .79);
    game_interact(g, 0);
    CHECK(!p->riding);
    game_rig_toggle(g, 0, WAGON);
    p->mods[0] = 2;
    game_rig_repair(g, 0);
    CHECK(g->rigs[0][0].armor == 720 && g->rigs[0][1].armor == 180);
    game_join(g);
    g->players[1].pos = position_from(v3(10, .02, 12));
    settle(g);
    game_rig_toggle(g, 1, MECH);
    CHECK(g->rigs[1][0].deployed && game_rig_near(g, 0) == ON_FOOT);
    g->players[0].pos = g->rigs[1][0].pos;
    CHECK(!game_rig_interact(g, 0));
    p->yaw = 0;
    p->pos = position_from(vadd(position_absolute(g->rigs[1][0].pos), v3(0, 0, -3)));
    game_rig_toggle(g, 0, MECH);
    CHECK(!g->rigs[0][0].deployed); /* other rig blocks deployment */
    game_rig_toggle(g, 1, MECH);
    CHECK(!g->rigs[1][0].deployed);
    g->beams[1].life = .14f;
    game_leave(g);
    game_equipment_step(g, .2);
    CHECK(g->beams[1].life == 0);
    game_new_ship(g, 42);
    CHECK(!p->riding && !g->rigs[0][0].deployed && !flares(g));
    destroy(g);
}
/* Collision fixtures use ordinary chunk boxes; no production test hooks. */
static Chunk *bay_chunk(Game *g) {
    for (int i = 0; i < g->world.chunk_count; i++)
        if (g->world.chunks[i].active && cell_equal(g->world.chunks[i].cell, (Cell){0, 0, 0}))
            return &g->world.chunks[i];
    CHECK(false);
    return NULL;
}
static void obstacle(Chunk *c, Vec3 min, Vec3 max) {
    c->boxes = realloc(c->boxes, (c->box_count + 1) * sizeof(Box));
    CHECK(c->boxes);
    c->boxes[c->box_count++] = (Box){vsub(min, world_center(c->cell)),
                                     vsub(max, world_center(c->cell)),
                                     {.1f, .1f, .1f},
                                     0,
                                     true};
}
static void test_clearance(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    Chunk *c = bay_chunk(g);
    size_t original = c->box_count;
    /* A low ceiling rejects the mech but admits the wagon. */
    obstacle(c, v3(-3, 2.2, 13), v3(3, 2.4, 18));
    game_rig_toggle(g, 0, MECH);
    CHECK(!g->rigs[0][0].deployed);
    mount(g, WAGON);
    game_summon(g, 0);
    CHECK(!g->shelter.deployed && game_shelter_near(g, 0) == SHELTER_NONE);
    c->box_count = original;
    game_interact(g, 0);
    game_rig_toggle(g, 0, WAGON);
    p->pos = position_from(v3(0, .02, 12));
    mount(g, MECH);
    Vec3 at = position_absolute(p->pos);
    game_interact(g, 0);
    CHECK(!p->riding);
    obstacle(c, vadd(at, v3(-1, 2, -1)), vadd(at, v3(1, 2.2, 1)));
    CHECK(game_rig_interact(g, 0) && !p->riding && strstr(g->message, "NOT READY"));
    c->box_count = original;
    game_interact(g, 0);
    CHECK(p->riding == MECH);
    /* four walls leave no safe ejection corridor */
    obstacle(c, vadd(at, v3(.95, 0, -3)), vadd(at, v3(1.05, 4, 3)));
    obstacle(c, vadd(at, v3(-1.05, 0, -3)), vadd(at, v3(-.95, 4, 3)));
    obstacle(c, vadd(at, v3(-3, 0, .95)), vadd(at, v3(3, 4, 1.05)));
    obstacle(c, vadd(at, v3(-3, 0, -1.05)), vadd(at, v3(3, 4, -.95)));
    game_interact(g, 0);
    CHECK(p->riding == MECH && strstr(g->message, "NO SAFE EXIT"));
    p->shield = 0;
    game_hurt(g, 0, 610, 0);
    CHECK(!p->riding && g->rigs[0][0].armor == 0 && p->health > 0);
    c->box_count = original;
    game_rig_toggle(g, 0, MECH);
    /* Empty plating remains depleted after packing; treatment repairs both rigs. */
    p->pos = position_from(v3(0, .02, 12));
    game_summon(g, 0);
    game_interact(g, 0);
    CHECK(p->sheltered);
    p->shelter_pos = shelter_station(SHELTER_MED);
    game_interact(g, 0);
    CHECK(g->rigs[0][0].armor == 600);
    destroy(g);
}
static void test_beams(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    Room *r = world_find(&g->world, (Cell){0, 0, 0});
    CHECK(r);
    for (unsigned gun = 16; gun < 19; gun++) {
        r->enemy_count = 4;
        for (int i = 0; i < 4; i++)
            r->enemies[i] = (Enemy){.pos = position_from(v3(0, .97, 17 + i * 2)),
                                    .health = 1000,
                                    .kind = 0,
                                    .id = (uint8_t)i,
                                    .alive = true,
                                    .cooldown = 100,
                                    .normal = {0, 1, 0}};
        p->weapon = gun;
        p->health = 50;
        p->hurt = 2;
        p->cloaked = true;
        fire(g);
        CHECK(!p->cloaked && g->beams[0].life > 0 && bullets(g) == 0 && game_shot_speed(p) == 0);
        CHECK(r->enemies[0].health < 1000);
        CHECK(gun != 17 || r->enemies[3].health < 1000);
        CHECK(gun != 16 || r->enemies[0].burn > 0);
        CHECK(gun != 18 || p->health > 50);
    }
    r->enemy_count = 1;
    r->enemies[0].pos = position_from(v3(0, .97, 20));
    r->enemies[0].health = 1000;
    Chunk *chunk = bay_chunk(g);
    size_t original = chunk->box_count;
    obstacle(chunk, v3(-3, 0, 18), v3(3, 4, 18.4));
    p->weapon = 17;
    fire(g);
    CHECK(r->enemies[0].health == 1000 && position_absolute(g->beams[0].to).z < 18.01);
    chunk->box_count = original;
    r->enemy_count = 0;
    p->pitch = -.8;
    fire(g);
    CHECK(position_absolute(g->beams[0].to).y < .1); /* hull/floor stops ray */
    /* An occupied projectile pool must not reject a beam. */
    for (int i = 0; i < SC_MAX_PROJECTILES; i++)
        g->projectiles[i] = (Projectile){.active = true, .life = 2, .owner = 0, .pos = p->pos};
    unsigned shots = g->shots;
    fire(g);
    CHECK(g->shots == shots + 1);
    memset(g->projectiles, 0, sizeof(g->projectiles));
    p->energy = 0;
    p->cooldown = 0;
    tick(g, (Actions){.fire = true}, 1);
    CHECK(g->shots == shots + 1);
    tick(g, (Actions){0}, 20);
    CHECK(g->beams[0].life == 0);
    destroy(g);
}
static void test_flares_and_save(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    CHECK(!game_flare_upgrade(g, -1, false) && !game_flare_upgrade(g, 2, true));
    p->points = p->credits = 0;
    CHECK(!game_flare_upgrade(g, 0, false));
    p->points = 1;
    CHECK(game_flare_upgrade(g, 0, false) && p->points == 0 && p->flare_lifetime == 1);
    p->credits = 80;
    CHECK(game_flare_upgrade(g, 0, true) && p->credits == 0 && p->flare_strength == 1);
    p->points = 10;
    for (int i = 1; i < 5; i++) {
        CHECK(game_flare_upgrade(g, 0, false));
        CHECK(game_flare_upgrade(g, 0, true));
    }
    CHECK(!game_flare_upgrade(g, 0, false) && !game_flare_upgrade(g, 0, true));
    p->weapon = SC_FLARE_CANNON;
    Chunk *chunk = bay_chunk(g);
    size_t original = chunk->box_count;
    obstacle(chunk, v3(-3, 0, 18), v3(3, 4, 18.4));
    Room *room = world_find(&g->world, (Cell){0, 0, 0});
    CHECK(room);
    room->enemy_count = 1;
    room->enemies[0] = (Enemy){.pos = position_from(v3(0, .97, 16)),
                               .health = 1000,
                               .kind = 0,
                               .alive = true,
                               .cooldown = 100,
                               .normal = {0, 1, 0}};
    fire(g);
    tick(g, (Actions){0}, 30);
    CHECK(flares(g) == 1 && g->flares[0].normal.z < -.99 &&
          position_absolute(g->flares[0].pos).z < 18);
    CHECK(room->enemies[0].health == 1000); /* light shots pass through monsters */
    chunk->box_count = original;
    room->enemy_count = 0;
    memset(g->flares, 0, sizeof(g->flares));
    p->pitch = -.75;
    fire(g);
    CHECK(bullets(g) > 0);
    tick(g, (Actions){0}, 30);
    CHECK(flares(g) == 1 && bullets(g) == 0);
    CHECK(g->flares[0].life > 104 && g->flares[0].normal.y > .99 && g->flares[0].radius == 22 &&
          g->flares[0].power == 6.25);
    game_attach_flare(g, -1, v3(0, 0, 15), v3(0, -1, 0));
    game_attach_flare(g, 2, v3(0, 0, 15), v3(0, -1, 0));
    game_attach_flare(g, 0, v3(0, 0, -100), v3(0, -1, 0));
    CHECK(flares(g) == 1);
    for (int i = 1; i < SC_MAX_FLARES; i++)
        game_attach_flare(g, 0, v3(i * .1, 0, 15), v3(0, -1, 0));
    CHECK(flares(g) == SC_MAX_FLARES);
    g->flares[7].life = 1;
    game_attach_flare(g, 0, v3(0, 0, 16), v3(0, -1, 0));
    CHECK(g->flares[7].life == 105);
    g->flares[7].active = false;
    game_attach_flare(g, 0, v3(42, 4, -2), v3(1, 0, 0));
    CHECK(g->flares[7].active && g->flares[7].normal.x < -.99 &&
          position_absolute(g->flares[7].pos).z < 0);
    p->pitch = 0;
    mount(g, WAGON);
    p->rig_weapon = 2;
    g->rigs[0][1].armor = 123;
    g->flares[0].life = 17.5f;
    CHECK(game_save(g, "sc-equipment.sav"));
    Game *copy = fresh();
    CHECK(game_load(copy, "sc-equipment.sav"));
    CHECK(copy->players[0].riding == WAGON && copy->players[0].rig_weapon == 2 &&
          copy->rigs[0][1].armor == 123 && copy->rigs[0][1].deployed);
    CHECK(copy->players[0].flare_lifetime == 5 && copy->players[0].flare_strength == 5 &&
          flares(copy) == 32);
    CHECK(copy->flares[0].life == 17.5f && copy->flares[0].normal.y > .99);
    CHECK(position_absolute(copy->flares[7].pos).z < 0 && copy->flares[7].normal.x < -.99);
    settle(copy);
    tick(copy, (Actions){.forward = true}, 20);
    CHECK(copy->players[0].drive_speed > 15);
    game_equipment_step(copy, 106);
    CHECK(flares(copy) == 0);
    remove("sc-equipment.sav");
    destroy(copy);
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v5.sav"));
    p = &g->players[0];
    CHECK(g->world.seed == 713 && g->mission == 2 && p->level == 7 && p->sheltered &&
          g->shelter.deployed);
    CHECK(p->cloaked && p->flight == 10 && (p->weapons & SC_NEW_WEAPONS) == SC_NEW_WEAPONS);
    CHECK(!p->riding && !p->flare_lifetime && !p->flare_strength && !g->rigs[0][0].deployed &&
          g->rigs[0][0].armor == 600);
    CHECK(game_save(g, "sc-equipment.sav"));
    CHECK(game_load(g, "sc-equipment.sav"));
    CHECK(g->players[0].sheltered && g->players[0].cloaked);
    remove("sc-equipment.sav");
    destroy(g);
}
static void journey(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    legacy_game_init(g, 713);
    settle(g);
    /* Drive normal actions all the way from the space arrival to usable floor. */
    for (int i = 0; i < 600 && position_absolute(g->players[0].pos).z < 12; i++)
        tick(g, (Actions){.forward = 1}, 1);
    tick(g, (Actions){0}, 120);
    CHECK(position_absolute(g->players[0].pos).z >= 12 && g->players[0].health > 0);
    mount(g, MECH);
    fire(g);
    CHECK(bullets(g) > 0);
    game_interact(g, 0);
    game_rig_toggle(g, 0, MECH);
    mount(g, WAGON);
    Vec3 before = position_absolute(g->players[0].pos);
    tick(g, (Actions){.forward = 1}, 45);
    CHECK(position_absolute(g->players[0].pos).z > before.z + 10);
    tick(g, (Actions){.crouch = true}, 60);
    game_interact(g, 0);
    game_rig_toggle(g, 0, WAGON);
    g->players[0].weapon = SC_FLARE_CANNON;
    game_look(&g->players[0], 0, -.7);
    fire(g);
    tick(g, (Actions){0}, 45);
    CHECK(flares(g) > 0);
    CHECK(game_save(g, "sc-equipment-journey.sav"));
    CHECK(game_load(g, "sc-equipment-journey.sav"));
    CHECK(flares(g) > 0);
    remove("sc-equipment-journey.sav");
    destroy(g);
}
int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--journey"))
        journey();
    else {
        test_rigs();
        test_clearance();
        test_beams();
        test_flares_and_save();
    }
    printf("PASS %u equipment assertions: beams, owned rigs, driving, plating, attached flares and "
           "format-6 migration\n",
           checks);
    return 0;
}
