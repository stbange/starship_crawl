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
    CHECK(!g->world.pending && !g->world.failed);
}
static Game *fresh(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    legacy_game_init(g, 713);
    g->players[0].pos = position_from(v3(0, .02, 12));
    g->players[0].yaw = .2;
    g->players[0].pitch = -.1;
    settle(g);
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
static void enter(Game *g) {
    game_summon(g, 0);
    CHECK(g->shelter.deployed);
    CHECK(game_shelter_near(g, 0) == SHELTER_DOOR);
    game_interact(g, 0);
    CHECK(g->players[0].sheltered);
}
static void test_placement_and_return(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    game_summon(g, -1);
    game_summon(g, 2);
    CHECK(!g->shelter.deployed);
    CHECK(game_shelter_near(g, -1) == SHELTER_NONE && game_shelter_near(g, 2) == SHELTER_NONE);
    CHECK(!game_shelter_interact(g, -1) && !game_shelter_interact(g, 2));
    CHECK(!game_shelter_interact(g, 0));
    Position original = p->pos;
    double yaw = p->yaw, pitch = p->pitch;
    p->pos = position_from(v3(0, 4, -34));
    game_summon(g, 0);
    CHECK(!g->shelter.deployed);
    p->pos = position_from(v3(0, 4, 12));
    game_summon(g, 0);
    CHECK(!g->shelter.deployed);
    p->pos = position_from(v3(9000, .02, 9000));
    game_summon(g, 0);
    CHECK(!g->shelter.deployed);
    p->pos = original;
    enter(g);
    Position door = g->shelter.door;
    CHECK(vlen(position_delta(p->pos, original)) == 0);
    CHECK(fabs(player_eye(p).y - 1.64) < 1e-6);
    CHECK(fabs(p->yaw) < 1e-6 && fabs(p->pitch) < 1e-6);
    game_summon(g, 0);
    CHECK(vlen(position_delta(door, g->shelter.door)) == 0);
    p->shelter_pos = v3(0, .02, -1);
    unsigned mission = g->mission;
    game_interact(g, 0);
    CHECK(g->mission == mission && p->sheltered);
    p->shelter_pos = vadd(shelter_station(SHELTER_DOOR), v3(0, .02, 0));
    p->pos = position_from(v3(9000, .02, 9000));
    game_interact(g, 0);
    CHECK(p->sheltered);
    p->pos = position_from(v3(0, -.5, 12));
    game_interact(g, 0);
    CHECK(p->sheltered);
    p->pos = original;
    game_interact(g, 0);
    CHECK(!p->sheltered && fabs(p->yaw - yaw) < 1e-9 && fabs(p->pitch - pitch) < 1e-9);
    CHECK(vlen(position_delta(original, p->pos)) == 0);
    p->pos = position_from(v3(15, .02, 12));
    CHECK(game_shelter_near(g, 0) == SHELTER_NONE);
    game_summon(g, 0);
    CHECK(!g->shelter.deployed);
    game_summon(g, 0);
    CHECK(g->shelter.deployed && vlen(position_delta(door, g->shelter.door)) > 10);
    destroy(g);
}
static void test_furniture_and_recovery(void) {
    Game *g = fresh();
    enter(g);
    Player *p = &g->players[0];
    size_t count;
    const Box *boxes = shelter_furniture(&count);
    CHECK(count >= 6);
    for (size_t i = 0; i < count; i++)
        CHECK(!shelter_clear(vmul(vadd(boxes[i].min, boxes[i].max), .5)));
    CHECK(!shelter_clear(v3(NAN, 0, 0)) && !shelter_clear(v3(0, 0, INFINITY)));
    CHECK(!shelter_clear(v3(9, 0, 0)) && !shelter_clear(v3(0, 0, 8)));
    CHECK(!shelter_clear(v3(0, -1, 0)) && !shelter_clear(v3(0, 1, 0)));
    CHECK(shelter_clear(v3(8.5, .02, 7.5)));
    CHECK(vlen(shelter_station(SHELTER_NONE)) == 0 && vlen(shelter_station((ShelterUse)99)) == 0);
    p->shelter_pos = v3(8.5, .02, 0);
    p->yaw = 0;
    tick(g, (Actions){.strafe = 1}, 120);
    CHECK(p->shelter_pos.x <= 8.6);
    p->shelter_pos = v3(0, .02, 7.5);
    tick(g, (Actions){.forward = 1}, 120);
    CHECK(p->shelter_pos.z <= 7.6);
    p->shelter_pos = v3(0, .02, 0);
    tick(g, (Actions){.forward = 1}, 120);
    CHECK(p->shelter_pos.z < .87);
    p->shelter_pos = v3(0, .02, -4);
    tick(g, (Actions){.forward = 1, .strafe = 1, .crouch = true}, 10);
    CHECK(p->crouched && p->shelter_pos.x > 0 && p->shelter_pos.z > -4);
    p->shelter_pos = vadd(shelter_station(SHELTER_MED), v3(0, .02, 0));
    p->health = 5;
    p->armor = 0;
    p->slow = 2;
    game_interact(g, 0);
    CHECK(p->health == player_max_health(p) && p->armor == player_max_armor(p) && p->slow == 0);
    p->shelter_pos = vadd(shelter_station(SHELTER_KITCHEN), v3(0, .02, 0));
    p->health = 5;
    game_interact(g, 0);
    CHECK(p->health == 35 && p->nourished == 120);
    p->shelter_pos = vadd(shelter_station(SHELTER_BED), v3(0, .02, 0));
    p->health = 5;
    p->shield = 0;
    p->energy = 0;
    game_interact(g, 0);
    CHECK(p->resting == 6);
    tick(g, (Actions){0}, 370);
    CHECK(p->resting == 0 && p->health == player_max_health(p));
    CHECK(p->shield == player_max_shield(p) && p->energy == player_max_energy(p));
    game_interact(g, 0);
    tick(g, (Actions){.strafe = 1}, 1);
    CHECK(p->resting == 0);
    p->shelter_pos = vadd(shelter_station(SHELTER_BED), v3(0, .02, 0));
    game_interact(g, 0);
    tick(g, (Actions){.fire = true}, 1);
    CHECK(p->resting == 0 && g->shots == 0);
    p->shelter_pos = vadd(shelter_station(SHELTER_BED), v3(0, .02, 0));
    game_interact(g, 0);
    tick(g, (Actions){.jump = true}, 1);
    CHECK(p->resting == 0);
    p->nourished = .01f;
    tick(g, (Actions){0}, 1);
    CHECK(p->nourished == 0);
    destroy(g);
}
static void test_isolation_and_coop(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    p->pos = position_from(vadd(world_center((Cell){1, 0, 0}), v3(0, .02, -24)));
    settle(g);
    for (int i = 0; i < g->world.count; i++) {
        g->world.rooms[i].enemy_count = 0;
        g->world.rooms[i].pickup_count = 0;
    }
    enter(g);
    game_hurt(g, 0, 10000, CHILL | DRAIN);
    CHECK(p->health == 100 && p->shield == 50 && p->slow == 0 && p->energy == 100);
    Room *room = world_find(&g->world, (Cell){1, 0, 0});
    CHECK(room);
    room->enemy_count = 1;
    room->enemies[0] =
        (Enemy){.pos = p->pos, .alive = true, .health = 100, .normal = {0, 1, 0}, .kind = 1};
    room->pickup_count = 1;
    room->pickups[0] = (Pickup){.pos = p->pos, .kind = PICK_SALVAGE, .active = true};
    g->projectiles[0] = (Projectile){.pos = p->pos,
                                     .velocity = {0, 0, 30},
                                     .life = 2,
                                     .damage = 10000,
                                     .active = true,
                                     .owner = -1};
    unsigned credits = p->credits, mission = g->mission;
    tick(g, (Actions){.fire = true}, 120);
    CHECK(p->health == 100 && p->shield == 50 && g->shots == 0 && g->projectile_hits == 0);
    CHECK(p->credits == credits && room->pickups[0].active && g->mission == mission);
    CHECK(g->enemy_shots == 0 && room->enemies[0].windup == 0);
    game_join(g);
    CHECK(g->players[1].sheltered && g->player_count == 2);
    g->players[1].shelter_pos = vadd(shelter_station(SHELTER_DOOR), v3(0, .02, 0));
    game_interact(g, 1);
    CHECK(!g->players[1].sheltered && p->sheltered);
    game_hurt(g, 1, 10, 0);
    CHECK(g->players[1].shield < 50);
    game_summon(g, 1);
    CHECK(strstr(g->message, "OCCUPIED"));
    tick(g, (Actions){0}, 120);
    CHECK(g->enemy_shots > 0);
    game_leave(g);
    CHECK(!g->players[1].sheltered && g->player_count == 1);
    game_new_ship(g, 42);
    CHECK(!g->shelter.deployed && !p->sheltered && p->nourished == 0);
    destroy(g);
}
static void test_persistence(void) {
    const char *path = "sc-shelter.sav";
    Game *g = fresh();
    CHECK(game_load(g, SC_FIXTURE_DIR "/generator-v3.sav"));
    CHECK(g->world.seed == 713 && g->mission == 2 && g->players[0].level == 7);
    CHECK(g->players[0].kills == 81 && g->players[0].credits == 123 && !g->shelter.deployed &&
          g->dirty);
    CHECK(g->world.record_count == 1 && g->world.records[0].killed == 1);
    CHECK(cell_equal(g->world.records[0].cell, (Cell){1, 0, 0}));
    CHECK(position_absolute(g->players[0].pos).x == 96 &&
          position_absolute(g->players[0].pos).z == 24);
    g->players[0].pos = position_from(v3(0, .02, 12));
    settle(g);
    enter(g);
    Player *p = &g->players[0];
    p->shelter_pos = vadd(shelter_station(SHELTER_BED), v3(0, .02, 0));
    game_interact(g, 0);
    p->nourished = 115;
    Position anchor = p->pos, door = g->shelter.door;
    CHECK(game_save(g, path));
    game_new_ship(g, 99);
    CHECK(game_load(g, path));
    CHECK(g->world.seed == 713 && g->mission == 2 && !g->dirty && g->player_count == 1);
    CHECK(p->sheltered && p->resting == 6 && p->nourished == 115 &&
          game_shelter_near(g, 0) == SHELTER_BED);
    CHECK(vlen(position_delta(anchor, p->pos)) == 0 &&
          vlen(position_delta(door, g->shelter.door)) == 0);
    settle(g);
    p->shelter_pos = vadd(shelter_station(SHELTER_DOOR), v3(0, .02, 0));
    game_interact(g, 0);
    CHECK(!p->sheltered && p->resting == 0);
    p->health = 50;
    p->hurt = 0;
    tick(g, (Actions){0}, 60);
    CHECK(p->health > 55.9f && p->nourished < 115);
    CHECK(game_save(g, path));
    CHECK(game_load(g, path));
    CHECK(!p->sheltered && g->shelter.deployed);
    remove(path);
    destroy(g);
}
/* Move through the real sanctuary instead of writing interior coordinates. */
static void walk(Game *g, Vec3 target) {
    Player *p = &g->players[0];
    for (int i = 0; i < 1800; i++) {
        Vec3 d = vsub(target, p->shelter_pos);
        d.y = 0;
        if (vlen(d) < .15)
            return;
        game_look(p, atan2(d.x, d.z) - p->yaw, -p->pitch);
        tick(g, (Actions){.forward = 1}, 1);
        CHECK(shelter_clear(p->shelter_pos));
    }
    CHECK(!"Sanctuary route timed out");
}
static void journey(void) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    legacy_game_init(g, 713);
    settle(g);
    for (int i = 0; i < 1800 && position_absolute(g->players[0].pos).z < 12; i++)
        tick(g, (Actions){.forward = 1}, 1);
    CHECK(position_absolute(g->players[0].pos).z >= 12);
    tick(g, (Actions){0}, 60);
    Position anchor = g->players[0].pos;
    enter(g);
    walk(g, v3(-3.3, 0, -5));
    walk(g, shelter_station(SHELTER_KITCHEN));
    game_interact(g, 0);
    CHECK(g->players[0].nourished > 119);
    walk(g, v3(-3.3, 0, -3.5));
    walk(g, v3(-3.3, 0, 3.3));
    walk(g, shelter_station(SHELTER_BED));
    game_interact(g, 0);
    tick(g, (Actions){0}, 370);
    CHECK(g->players[0].resting == 0);
    walk(g, v3(-3.3, 0, 3.3));
    walk(g, v3(-3.3, 0, -.3));
    walk(g, v3(3.3, 0, -.3));
    walk(g, v3(3.3, 0, 4.1));
    walk(g, shelter_station(SHELTER_MED));
    game_interact(g, 0);
    CHECK(strstr(g->message, "MEDICAL STATION"));
    CHECK(game_save(g, "sc-shelter-journey.sav"));
    CHECK(game_load(g, "sc-shelter-journey.sav"));
    settle(g);
    CHECK(g->players[0].sheltered);
    walk(g, v3(3.3, 0, 4.1));
    walk(g, v3(3.3, 0, -.3));
    walk(g, v3(0, 0, -5.6));
    game_interact(g, 0);
    CHECK(!g->players[0].sheltered && vlen(position_delta(anchor, g->players[0].pos)) == 0);
    remove("sc-shelter-journey.sav");
    destroy(g);
}
int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--journey"))
        journey();
    else {
        test_placement_and_return();
        test_furniture_and_recovery();
        test_isolation_and_coop();
        test_persistence();
    }
    printf("PASS %u sanctuary assertions\n", checks);
    return 0;
}
