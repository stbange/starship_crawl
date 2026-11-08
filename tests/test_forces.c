#include "faction_fixture.h"

static void identities(void) {
    Game *g = fixture(713);
    Cell source = ordinary(g, (Cell){2, 0, 4}), goal = ordinary(g, (Cell){4, 0, 4});
    CHECK(!factions_launch(g, 0, ORDER_RAID, source, goal));
    CHECK(!factions_launch(g, 1, ORDER_NONE, source, goal));
    CHECK(!factions_launch(g, 1, ORDER_RAID, (Cell){0}, goal));
    g->world.open_layer = 1;
    CHECK(!factions_launch(g, 1, ORDER_RAID, source, goal));
    g->world.open_layer = 0;
    load_area(g, source);
    empty_actors(g);
    CHECK(factions_launch(g, 1, ORDER_RAID, source, goal));
    FactionForce *a = factions_force(g, 1);
    CHECK(a && factions_strength(a) == 3);
    CHECK(!factions_force(g, 99));
    CHECK(g->factions.nations[0].supplies[2] == 194);
    force_frame(g, .05);
    CHECK(actors(g) == 3);
    Room *r = world_find(&g->world, source);
    CHECK(r && r->enemy_count == 3);
    unsigned kills = g->players[0].kills, xp = g->players[0].xp;
    factions_damage(g, r, 0, 1000, -1);
    CHECK(!a->members[0].alive && g->factions.casualties == 1);
    CHECK(g->players[0].kills == kills && g->players[0].xp == xp);
    CHECK(r->record == SIZE_MAX);
    game_damage_enemy(g, r, 1, 0, 25, 0);
    CHECK(a->members[1].health < sc_enemies[a->members[1].kind].health + 80);
    float health = a->members[1].health;
    factions_forces_sync(g, true);
    CHECK(!actors(g));
    force_frame(g, .05);
    CHECK(actors(g) == 2 && a->members[1].health == health);
    g->players[0].pos = g->players[0].checkpoint;
    CHECK(factions_valid(&g->factions, 14));
    CHECK(game_save(g, "force-resume.sav"));
    CHECK(game_load(g, "force-resume.sav"));
    a = factions_force(g, 1);
    CHECK(a && !a->members[0].alive && a->members[1].health == health);
    CHECK(factions_valid(&g->factions, 14));
    world_stream(&g->world, 192);
    force_frame(g, .05);
    CHECK(actors(g) <= 2);
    factions_damage(g, NULL, 0, 1, -1);
    factions_damage(g, world_find(&g->world, source), -1, 1, -1);
    factions_forces_sync(g, true);
    /* Destroyed forces never return as room-local killed bits after streaming churn. */
    for (unsigned m = 0; m < 3; m++)
        a->members[m].alive = false;
    factions_forces_tick(g);
    CHECK(!factions_force(g, 1));
    remove("force-resume.sav");
    finish(g);
}
static void capacity_and_coarse(void) {
    Game *g = fixture(42);
    Cell source = ordinary(g, (Cell){8, 0, 8}), goal = ordinary(g, (Cell){10, 0, 8});
    for (unsigned c = 0; c < 3; c++)
        for (unsigned r = 0; r < 3; r++)
            g->factions.nations[c].supplies[r] = 1000;
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++)
        CHECK(factions_launch(g, 1 + i % 3, ORDER_PATROL, source, goal));
    CHECK(!factions_launch(g, 1, ORDER_PATROL, source, goal));
    FactionForce *a = factions_force(g, 1);
    a->wait = 2;
    factions_forces_tick(g);
    CHECK(!a->wait);
    for (unsigned i = 0; i < 30; i++) {
        g->factions.tick++;
        factions_forces_tick(g);
    }
    CHECK(g->factions.casualties > 0 && g->factions.retreats > 0);
    CHECK(g->players[0].kills == 0);
    memset(g->factions.forces, 0, sizeof(g->factions.forces));
    g->factions.nations[0].supplies[2] = 0;
    CHECK(!factions_launch(g, 1, ORDER_PATROL, source, goal));
    g->factions.nations[0].supplies[2] = 100;
    CHECK(factions_launch(g, 1, ORDER_ESCORT, source, source));
    a = factions_force(g, g->factions.next_id - 1);
    a->age = 19;
    unsigned resource = 0;
    const FactionRegion *r = factions_at(&g->factions, source);
    if (r && r->kind >= SITE_POWER)
        resource = r->kind - SITE_POWER;
    unsigned before = g->factions.nations[0].supplies[resource];
    factions_forces_tick(g);
    CHECK(!a->cargo && a->order == ORDER_RETREAT);
    CHECK(g->factions.nations[0].supplies[resource] == before + 12);
    for (int i = 0; i < 16; i++)
        factions_forces_tick(g);
    CHECK(!a->id);
    g->factions.next_id = UINT32_MAX;
    CHECK(!factions_launch(g, 1, ORDER_PATROL, source, goal));
    finish(g);
}
static void movement(void) {
    Game *g = fixture(713);
    Cell from = ordinary(g, (Cell){2, 0, 4}), to = from;
    bool found = false;
    for (int d = 0; d < 4; d++) {
        Cell c = world_neighbor(from, (Direction)d);
        if (!factions_safe(g, c) && world_door_version(g->world.seed, 14, from, (Direction)d)) {
            to = c;
            found = true;
            break;
        }
    }
    CHECK(found);
    load_area(g, from);
    empty_actors(g);
    CHECK(factions_launch(g, 1, ORDER_PATROL, from, to));
    FactionForce *a = factions_force(g, 1);
    Vec3 begin = position_absolute(a->members[0].pos);
    double distance = 0;
    for (int i = 0; i < 1000; i++) {
        force_frame(g, .05);
        distance = fmax(distance, vlen(vsub(begin, position_absolute(a->members[0].pos))));
    }
    CHECK(distance > 20);
    CHECK(g->factions.arrived > 0 ||
          cell_equal(world_cell(position_absolute(a->members[0].pos)), to));
    CHECK(actors(g) == 3 && factions_valid(&g->factions, 14));
    /* A full room defers actors; no static actor is removed to make space. */
    factions_forces_sync(g, true);
    Room *room = world_find(&g->world, world_cell(position_absolute(a->members[0].pos)));
    CHECK(room);
    room->enemy_count = SC_MAX_ENEMIES;
    for (int i = 0; i < SC_MAX_ENEMIES; i++)
        room->enemies[i] = (Enemy){.alive = true, .health = 10};
    force_frame(g, .05);
    CHECK(room->enemy_count == SC_MAX_ENEMIES);
    g->players[0].sheltered = true;
    Vec3 before = position_absolute(a->members[0].pos);
    force_frame(g, .05);
    CHECK(vlen(vsub(before, position_absolute(a->members[0].pos))) == 0);
    finish(g);
}
static void combat(void) {
    Game *g = fixture(42);
    Cell site = ordinary(g, (Cell){2, 0, 4});
    load_area(g, site);
    empty_actors(g);
    CHECK(factions_launch(g, 1, ORDER_OCCUPY, site, site));
    CHECK(factions_launch(g, 2, ORDER_OCCUPY, site, site));
    for (unsigned i = 0; i < 2; i++) {
        FactionForce *a = &g->factions.forces[i];
        a->wait = 100;
        for (unsigned m = 0; m < 3; m++)
            a->members[m].pos =
                position_from(vadd(world_center(site), v3((m - 1.0) * .9, .02, 6 + i * 12)));
    }
    Actions idle[2] = {0};
    for (int frame = 0; frame < 1000; frame++)
        game_step(g, idle, .05);
    CHECK(g->enemy_shots > 3 && g->factions.casualties > 0 && !g->players[0].kills);
    CHECK(factions_valid(&g->factions, 14));
    finish(g);

    g = fixture(713);
    site = ordinary(g, (Cell){2, 0, 4});
    load_area(g, site);
    empty_actors(g);
    CHECK(factions_launch(g, 1, ORDER_OCCUPY, site, site));
    FactionForce *a = &g->factions.forces[0];
    a->wait = 100;
    force_frame(g, .05);
    Room *room = world_find(&g->world, site);
    CHECK(room && room->enemy_count == 3);
    Enemy feral = {.pos = position_from(vadd(world_center(site), v3(-.9, .02, 4))),
                   .alive = true,
                   .health = 100,
                   .kind = 0};
    float hp = a->members[0].health;
    CHECK(factions_feral(g, room, &feral, .05));
    CHECK(a->members[0].health < hp);
    feral.pos = position_from(vadd(world_center(site), v3(0, .02, 17)));
    feral.kind = 7;
    feral.cooldown = 0;
    for (int frame = 0; frame < 50; frame++)
        CHECK(factions_feral(g, room, &feral, .05));
    CHECK(g->enemy_shots > 0);
    /* Player participation is required, and only surviving identities are counted. */
    for (int i = 0; i < room->enemy_count; i++)
        game_damage_enemy(g, room, i, 0, 1000, W_BURN | W_CHILL);
    unsigned credit = g->players[0].credits;
    CHECK(credit >= 3 && !factions_strength(a));
    for (int i = 0; i < room->enemy_count; i++)
        game_damage_enemy(g, room, i, 0, 1000, 0);
    CHECK(g->players[0].credits == credit);
    finish(g);
}
static void vertical(void) {
    Game *g = fixture(713);
    Cell from = {0}, to = {0};
    bool found = false;
    for (int z = 2; z < 7 && !found; z++)
        for (int x = 0; x < 8 && !found; x++) {
            Cell c = {x, 0, z}, d = {x, 1, z};
            if (!factions_safe(g, c) && !factions_safe(g, d) &&
                cell_equal(world_describe_version(713, 14, c).cell, c) &&
                cell_equal(world_describe_version(713, 14, d).cell, d)) {
                from = c;
                to = d;
                found = true;
            }
        }
    CHECK(found);
    game_join(g);
    g->players[1].pos = position_from(vadd(world_center(to), v3(0, .02, -30)));
    load_area(g, from);
    empty_actors(g);
    CHECK(factions_launch(g, 1, ORDER_PATROL, from, to));
    CHECK(factions_launch(g, 1, ORDER_PATROL, from, to));
    bool riding = false, arrived = false;
    for (int frame = 0; frame < 1600; frame++) {
        force_frame(g, .05);
        FactionForce *a = &g->factions.forces[0], *b = &g->factions.forces[1];
        CHECK(!(a->route_time > 0 && b->route_time > 0));
        riding |= a->route_time > 0;
        arrived |= world_cell(position_absolute(a->members[0].pos)).y == to.y;
        if (arrived)
            break;
    }
    CHECK(riding && arrived && factions_valid(&g->factions, 14));
    finish(g);
    g = fixture(713);
    found = false;
    for (int z = 2; z < 12 && !found; z++)
        for (int x = 0; x < 16 && !found; x++) {
            Cell c = {x, 0, z}, d = {x, 1, z};
            if (world_service_column(713, c) && !factions_safe(g, c) && !factions_safe(g, d) &&
                cell_equal(world_describe_version(713, 14, c).cell, c)) {
                from = c;
                to = d;
                found = true;
            }
        }
    CHECK(found);
    game_join(g);
    g->players[1].pos = position_from(vadd(world_center(to), v3(0, .02, -30)));
    load_area(g, from);
    empty_actors(g);
    g->factions.next_id = 3;
    CHECK(factions_launch(g, 1, ORDER_PATROL, from, to));
    FactionForce *a = factions_force(g, 3);
    double peak = 0;
    for (int frame = 0; frame < 3000; frame++) {
        force_frame(g, .05);
        peak = fmax(peak, position_absolute(a->members[0].pos).y);
        CHECK(a->route_time == 0);
        if (peak >= SC_DECK)
            break;
    }
    CHECK(peak >= SC_DECK);
    finish(g);
}
static void occupation_and_handoff(void) {
    Game *g = fixture(713);
    Cell site = ordinary(g, (Cell){2, 0, 4});
    load_area(g, site);
    empty_actors(g);
    FactionRegion *region = factions_record(g, site);
    region->owner = 2;
    Room *room = world_find(&g->world, site);
    CHECK(room);
    room->enemy_count = 1;
    room->enemies[0] = (Enemy){.alive = true,
                               .country = 2,
                               .health = 1000000,
                               .kind = 24,
                               .pos = position_from(vadd(world_center(site), v3(30, .02, 30))),
                               .normal = {0, 1, 0}};
    CHECK(factions_launch(g, 1, ORDER_RAID, site, site));
    FactionForce *a = factions_force(g, 1);
    for (int i = 0; i < 120; i++)
        force_frame(g, .05);
    CHECK(region->owner == 2 && !g->factions.captures);
    factions_damage(g, room, 0, 1000001, -1);
    CHECK(!room->enemies[0].alive && !g->players[0].kills);
    for (int i = 0; i < 150; i++)
        force_frame(g, .05);
    CHECK(region->owner == 1 && g->factions.captures == 1 && a->order == ORDER_OCCUPY);
    a->wait = 0;
    force_frame(g, .05);
    CHECK(a->order == ORDER_RETREAT);
    for (int i = 0; i < 220; i++)
        force_frame(g, .05);
    CHECK(a->id && (a->arrivals & 0x80000000u));
    g->players[0].pos = position_from(vadd(world_center(site), v3(0, .02, 120)));
    g->players[0].yaw = 0;
    for (int i = 0; i < 25; i++)
        force_frame(g, .05);
    CHECK(!a->id);
    finish(g);
    g = fixture(42);
    site = ordinary(g, (Cell){8, 0, 8});
    CHECK(factions_launch(g, 1, ORDER_PATROL, site, site));
    a = factions_force(g, 1);
    a->route_time = 2;
    a->waypoint = world_lift(site);
    a->next = site;
    a->next.y++;
    for (unsigned m = 0; m < 3; m++)
        a->members[m].pos = position_from(a->waypoint);
    CHECK(game_save(g, "mid-lift.sav") && game_load(g, "mid-lift.sav"));
    a = factions_force(g, 1);
    CHECK(a->route_time == 2);
    factions_forces_tick(g);
    CHECK(a->route_time == 0 && world_cell(position_absolute(a->members[0].pos)).y == site.y + 1);
    remove("mid-lift.sav");
    finish(g);
}
static void player_contact(void) {
    Game *g = fixture(713);
    Cell site = ordinary(g, (Cell){2, 0, 4});
    load_area(g, site);
    empty_actors(g);
    CHECK(factions_launch(g, 1, ORDER_OCCUPY, site, site));
    FactionForce *a = factions_force(g, 1);
    a->wait = 100;
    force_frame(g, .05);
    g->players[0].pos = position_from(vadd(world_center(site), v3(0, .02, -15)));
    g->factions.incidents[0][0] = (FactionIncident){.site = site, .strikes = 2, .local_time = 45};
    for (int i = 0; i < 80; i++)
        force_frame(g, .05);
    CHECK(g->enemy_shots > 0);
    Room *room = world_find(&g->world, site);
    CHECK(room && room->enemy_count == 3);
    room->enemies[0].burn = 1;
    room->enemies[0].health = .1f;
    force_frame(g, .05);
    CHECK(!a->members[0].alive);
    g->matrix.controls[0] = (MatrixControl){.room = site, .kind = 3, .owner = 0, .life = 4};
    force_frame(g, .05);
    CHECK(a->members[1].allegiance == 3);
    finish(g);
}
static void crowded_arrivals(void) {
    Game *g = fixture(713);
    Cell site = ordinary(g, (Cell){2, 0, 4});
    load_area(g, site);
    empty_actors(g);
    Room *room = world_find(&g->world, site);
    for (unsigned i = 0; i < 14; i++)
        room->enemies[i] = (Enemy){.alive = true,
                                   .health = 100,
                                   .country = 1,
                                   .pos = position_from(vadd(world_center(site), v3(20, .02, 20)))};
    room->enemy_count = 14;
    for (unsigned i = 0; i < 6; i++) {
        CHECK(factions_launch(g, 1, ORDER_OCCUPY, site, site));
        factions_force(g, i + 1)->wait = 100;
    }
    force_frame(g, .05);
    CHECK(room->enemy_count == 20 && actors(g) == 6);
    /* A full room retains its six leased actors when both cameras turn towards it. */
    game_join(g);
    g->players[1].pos = g->players[0].pos;
    g->players[0].yaw = g->players[1].yaw = 0;
    for (unsigned frame = 0; frame < 50; frame++) {
        force_frame(g, .05);
        CHECK(actors(g) == 6 && room->enemy_count == 20);
        for (unsigned i = 14; i < 20; i++)
            CHECK(room->enemies[i].force <= 2);
    }
    factions_damage(g, room, 14, 10000, -1);
    force_frame(g, .05);
    CHECK(actors(g) == 5);
    CHECK(!factions_force(g, 1)->members[0].alive);
    /* A spare slot alone cannot make an unobserved arrival pop into either view. */
    for (unsigned i = 0; i < 20; i++)
        force_frame(g, .05);
    CHECK(actors(g) == 5 && !g->players[0].kills && !g->players[1].kills);
    g->players[0].yaw = g->players[1].yaw = SC_PI;
    force_frame(g, .05);
    CHECK(actors(g) == 6 && factions_force(g, 3)->present == 1);
    finish(g);
}
static void separated_resume(void) {
    Game *g = fixture(42);
    Cell a = ordinary(g, (Cell){2, 0, 4}), b = ordinary(g, (Cell){30, 2, 40});
    CHECK(factions_launch(g, 1, ORDER_ESCORT, a, a));
    CHECK(factions_launch(g, 1, ORDER_REINFORCE, b, b));
    factions_force(g, 1)->wait = factions_force(g, 2)->wait = 100;
    load_area(g, a);
    game_join(g);
    g->players[1].pos = position_from(vadd(world_center(b), v3(0, .02, -30)));
    g->players[1].yaw = SC_PI;
    load_area(g, a);
    empty_actors(g);
    force_frame(g, .05);
    CHECK(actors(g) == 6);
    Room *room = world_find(&g->world, a);
    factions_damage(g, room, 0, 10000, -1);
    factions_forces_sync(g, true);
    unsigned stock[3];
    memcpy(stock, g->factions.nations[0].supplies, sizeof(stock));
    for (unsigned cycle = 0; cycle < 12; cycle++) {
        g->players[0].sheltered = true;
        g->matrix.divers[1].active = true;
        CHECK(factions_active(g));
        force_frame(g, .05);
        g->players[1].sheltered = true;
        CHECK(!factions_active(g));
        unsigned tick = g->factions.tick;
        factions_step(g, 2);
        CHECK(g->factions.tick == tick);
        g->players[0].sheltered = g->players[1].sheltered = false;
        g->matrix.divers[1].active = false;
        load_area(g, (Cell){120, -3, 120});
        force_frame(g, .05);
        CHECK(!factions_force(g, 1)->physical && factions_force(g, 2)->physical);
        CHECK(game_save(g, "separated-war.sav") && game_load(g, "separated-war.sav"));
        CHECK(g->player_count == 1 && factions_valid(&g->factions, 14));
        game_join(g);
        g->players[1].pos = position_from(vadd(world_center(b), v3(0, .02, -30)));
        g->players[1].yaw = SC_PI;
        load_area(g, a);
        empty_actors(g);
        force_frame(g, .05);
        CHECK(actors(g) == 5 && !factions_force(g, 1)->members[0].alive);
        CHECK(factions_force(g, 1)->cargo == 12 && factions_force(g, 2)->cargo == 12);
        CHECK(!memcmp(stock, g->factions.nations[0].supplies, sizeof(stock)));
        CHECK(g->factions.casualties == 1 && !g->players[0].kills && !g->players[1].kills);
    }
    remove("separated-war.sav");
    finish(g);
}
int main(void) {
    identities();
    capacity_and_coarse();
    movement();
    combat();
    vertical();
    occupation_and_handoff();
    player_contact();
    crowded_arrivals();
    separated_resume();
    puts("PASS force ownership, casualties, real travel, capacity and convoy conservation");
}
