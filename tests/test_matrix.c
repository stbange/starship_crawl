#include "game.h"
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
#define BIT(n) (UINT32_C(1) << (n))
static Game *fresh(uint32_t seed) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, seed);
    g->players[0].pos = position_from(matrix_terminal((Cell){0}));
    Vec3 at = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &at, 1));
    while (g->world.pending)
        world_stream(&g->world, 2);
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static MatrixNetwork *network(Game *g) {
    return &g->matrix.networks[g->matrix.divers[0].network];
}
static MatrixRecord *record(Game *g) {
    return &g->matrix.records[network(g)->record];
}
static void clear_guards(Game *g) {
    for (int i = 0; i < SC_MATRIX_GUARDS; i++)
        network(g)->guards[i].active = false;
}
static void at_node(Game *g, unsigned n, Vec3 offset) {
    g->matrix.divers[0].pos = vadd(network(g)->nodes[n].at, offset);
    record(g)->seen |= BIT(n);
}
static void ready(Game *g) {
    g->matrix.divers[0].cooldown = 0;
    g->matrix.divers[0].bandwidth = 100;
    g->matrix.divers[0].trace = 0;
}
static void generation(void) {
    unsigned themes = 0, motifs = 0;
    bool rare = false, common = false;
    for (unsigned seed = 1; seed <= 128; seed++)
        for (int gen = 3; gen <= 6; gen++) {
            Cell key = {4 * (int)(seed % 5) - 8, (int)(seed % 3) - 1, 4 * (int)(seed % 7)};
            MatrixNetwork a, b;
            matrix_generate(seed, gen, key, &a);
            matrix_generate(seed, gen, key, &b);
            CHECK(a.mask == b.mask && a.box_count == b.box_count && a.core == b.core);
            CHECK(a.count >= 3 && a.count <= 16 && a.box_count <= SC_MATRIX_BOXES);
            CHECK(a.credential != a.entry && a.core != a.entry && a.switch_b == a.credential);
            themes |= BIT(a.theme);
            motifs |= BIT(a.motif);
            rare |= a.rare;
            common |= !a.rare;
            unsigned visited = BIT(a.entry);
            for (int step = 0; step < 16; step++)
                for (unsigned i = 0; i < 16; i++)
                    if (visited & BIT(i))
                        visited |= a.nodes[i].links;
            CHECK(visited == a.mask);
            for (unsigned i = 0; i < 16; i++)
                if (a.nodes[i].valid) {
                    CHECK(cell_equal(world_describe_version(seed, gen, a.nodes[i].room).cell,
                                     a.nodes[i].room));
                    for (unsigned j = 0; j < 16; j++)
                        if (a.nodes[i].links & BIT(j))
                            CHECK(a.nodes[j].links & BIT(i));
                }
            MatrixRecord r = {0};
            CHECK(!matrix_overlap(&a, &r, vadd(a.nodes[a.entry].at, v3(0, .02, -3)), 1.5));
            CHECK(matrix_overlap(&a, &r, vadd(a.nodes[a.entry].at, v3(0, -.2, 0)), 1.5));
            CHECK(matrix_raycast(&a, &r, vadd(a.nodes[a.entry].at, v3(0, 3, 0)), v3(0, -1, 0), 20) <
                  3.1);
            CHECK(matrix_raycast(&a, &r, v3(-50, 20, -50), v3(0, 1, 0), 8) == 8);
            for (unsigned i = 0; i < a.box_count; i++) {
                unsigned tag = a.boxes[i].tag;
                if (tag == 1) {
                    CHECK(!matrix_box_active(&a, &r, i));
                    a.bridge = 1;
                    CHECK(matrix_box_active(&a, &r, i));
                    a.bridge = 0;
                    r.done[MX_TRAFFIC] = 1;
                    CHECK(matrix_box_active(&a, &r, i));
                    r.done[MX_TRAFFIC] = 0;
                }
                if (tag == 2) {
                    CHECK(matrix_box_active(&a, &r, i));
                    a.gate = 1;
                    CHECK(!matrix_box_active(&a, &r, i));
                    a.gate = 0;
                    r.keys = 1;
                    r.flags = MX_ROUTED;
                    CHECK(!matrix_box_active(&a, &r, i));
                    r.keys = r.flags = 0;
                }
            }
            CHECK(strlen(matrix_controller(&a)) > 5);
        }
    CHECK(themes == 511 && motifs == 15 && rare && common);
}
static void entry_and_programs(void) {
    Game *g = fresh(713);
    Position body = g->players[0].pos;
    CHECK(!matrix_toggle(g, -1) && !matrix_toggle(g, 2) && !matrix_near(g, -1));
    g->players[0].riding = MECH;
    CHECK(!matrix_toggle(g, 0));
    g->players[0].riding = 0;
    g->players[0].yaw = .7;
    g->players[0].pitch = .2;
    CHECK(matrix_near(g, 0) && matrix_toggle(g, 0));
    CHECK(matrix_protected(g, 0) && !matrix_protected(g, -1));
    CHECK(matrix_valid(&g->matrix, g->world.seed, g->world.generator, g->players, &g->shelter));
    MatrixDiver *d = &g->matrix.divers[0];
    MatrixProfile *p = &g->matrix.profiles[0];
    CHECK(matrix_node(network(g), d->pos, 8) >= 0 &&
          matrix_node(network(g), v3(-90, 5, -90), 2) < 0);
    Actions a = {.forward = 1};
    Vec3 before = d->pos;
    for (int i = 0; i < 10; i++)
        game_step(g, (Actions[2]){a, {0}}, .02);
    CHECK(vlen(vsub(d->pos, before)) > 1 && vlen(position_delta(body, g->players[0].pos)) < .001);
    d->map = true;
    before = d->pos;
    matrix_player_step(g, 0, &a, .02);
    CHECK(vlen(vsub(d->pos, before)) == 0);
    d->map = false;
    game_map_toggle(g, 0);
    CHECK(d->map);
    game_map_toggle(g, 0);
    CHECK(!d->map);
    CHECK(!matrix_equip(g, 0, MX_FORK) && !matrix_equip(g, -1, 0) && !matrix_equip(g, 0, 8));
    p->programs = 63;
    p->data = 30;
    matrix_select(g, -1, 0);
    matrix_select(g, 0, 2);
    CHECK(d->selected == 0);
    for (unsigned program = 0; program < 6; program++) {
        matrix_select(g, 0, program % 2);
        CHECK(matrix_equip(g, 0, program));
        unsigned slot = d->selected;
        ready(g);
        at_node(g, network(g)->entry, v3(-3, 0, 0));
        record(g)->flags = 0;
        record(g)->done[MX_TRAFFIC] = 0;
        CHECK(matrix_cast(g, 0, slot));
        CHECK(d->bandwidth < 100 && d->cooldown > 0);
        CHECK(!matrix_cast(g, 0, slot));
        ready(g);
        d->map = true;
        CHECK(!matrix_cast(g, 0, slot));
        d->map = false;
        d->choice = true;
        CHECK(!matrix_cast(g, 0, slot));
        d->choice = false;
        d->bandwidth = 0;
        CHECK(!matrix_cast(g, 0, slot));
        ready(g);
    }
    CHECK(!matrix_cast(g, -1, 0) && !matrix_cast(g, 0, 2));
    CHECK(d->siphon > 0 && d->echo_life > 0 && d->veil > 0 && network(g)->bridge > 0);
    matrix_select(g, 0, 0);
    CHECK(matrix_equip(g, 0, MX_VEIL));
    for (int i = 0; i < 3; i++)
        CHECK(matrix_upgrade(g, 0));
    CHECK(!matrix_upgrade(g, 0) && !matrix_upgrade(g, -1));
    matrix_select(g, 0, 1);
    CHECK(matrix_equip(g, 0, MX_INTERRUPT));
    p->data = 0;
    CHECK(!matrix_upgrade(g, 0));
    ready(g);
    record(g)->keys = BIT(network(g)->credential);
    record(g)->flags = MX_ROUTED;
    clear_guards(g);
    CHECK(!matrix_cast(g, 0, 1));
    network(g)->guards[network(g)->credential].active = true;
    network(g)->guards[network(g)->credential].pos = vadd(d->pos, v3(1, 1, 0));
    CHECK(matrix_cast(g, 0, 1) && !matrix_guarded(network(g), d->pos));
    ready(g);
    CHECK(matrix_equip(g, 0, MX_FORK));
    CHECK(!matrix_cast(g, 0, 1));
    record(g)->flags = 0;
    d->pos = v3(-50, 0, -50);
    CHECK(!matrix_cast(g, 0, 1));
    CHECK(matrix_equip(g, 0, MX_BRIDGE));
    record(g)->done[MX_TRAFFIC] = 1;
    CHECK(!matrix_cast(g, 0, 1));
    d->unbanked = 3;
    p->data = 0;
    CHECK(matrix_toggle(g, 0));
    CHECK(p->data == 3 && p->dives == 1);
    CHECK(fabs(g->players[0].yaw - .7) < .001 && fabs(g->players[0].pitch - .2) < .001);
    CHECK(!matrix_cast(g, 0, 0));
    matrix_disconnect(g, -1, false);
    CHECK(matrix_toggle(g, 0));
    d->unbanked = 5;
    d->trace = 100;
    matrix_disconnect(g, 0, true);
    CHECK(d->lockout == 8 && p->data == 3 && !matrix_toggle(g, 0));
    for (int i = 0; i < 161; i++)
        matrix_step(g, .05);
    CHECK(matrix_toggle(g, 0));
    char hint[160];
    Vec3 goal;
    matrix_hint(g, 0, hint, sizeof(hint));
    CHECK(strstr(hint, "ENTRY"));
    CHECK(matrix_goal(g, 0, &goal));
    matrix_disconnect(g, 0, false);
    matrix_hint(g, 0, hint, sizeof(hint));
    CHECK(strstr(hint, "TERMINAL"));
    CHECK(!matrix_goal(g, 0, &goal));
    unsigned owned = p->programs, level = p->upgrades[MX_VEIL];
    game_new_ship(g, 42);
    CHECK(g->matrix.count == 0 && p->programs == owned && p->upgrades[MX_VEIL] == level);
    destroy(g);
}
static void consequences(void) {
    for (unsigned choice = 1; choice <= 3; choice++) {
        Game *g = fresh(713 + choice);
        CHECK(matrix_toggle(g, 0));
        clear_guards(g);
        MatrixNetwork *w = network(g);
        MatrixRecord *r = record(g);
        MatrixDiver *d = &g->matrix.divers[0];
        CHECK(!matrix_service(g, -1));
        d->map = true;
        CHECK(!matrix_service(g, 0));
        d->map = false;
        d->pos = v3(-30, 0, -30);
        CHECK(!matrix_service(g, 0));
        at_node(g, w->entry, v3(0, 0, -2));
        w->guards[0].active = true;
        w->guards[0].pos = w->nodes[w->entry].at;
        CHECK(!matrix_service(g, 0));
        clear_guards(g);
        game_join(g);
        unsigned credits = g->players[0].credits;
        for (unsigned service = 0; service < 6; service++) {
            ready(g);
            d->service = service;
            CHECK(matrix_service(g, 0));
            CHECK(matrix_operation_done(g, w->nodes[w->entry].room, service));
            unsigned paid = g->players[0].credits;
            ready(g);
            CHECK(matrix_service(g, 0));
            CHECK(g->players[0].credits == paid);
            CHECK(g->players[0].credits == g->players[1].credits);
        }
        CHECK(g->players[0].credits > credits && !matrix_operation_done(g, (Cell){0}, 6));
        CHECK((world_systems(&g->world, w->nodes[w->entry].room) & 7) == 7);
        matrix_cycle_service(g, 0, 1);
        CHECK(d->service == 0);
        matrix_cycle_service(g, 0, -1);
        CHECK(d->service == 5);
        matrix_cycle_service(g, 0, 3);
        CHECK(d->service == 5);
        d->bandwidth = 0;
        CHECK(!matrix_service(g, 0));
        ready(g);
        at_node(g, w->credential, v3(3, 0, 0));
        matrix_use(g, 0);
        CHECK(r->keys == BIT(w->credential));
        matrix_use(g, 0);
        Vec3 goal;
        char hint[180];
        CHECK(matrix_goal(g, 0, &goal));
        matrix_hint(g, 0, hint, sizeof(hint));
        CHECK(strstr(hint, "ROUTE"));
        at_node(g, w->entry, v3(-3, 0, 0));
        matrix_use(g, 0);
        CHECK(w->switches[0] > 0);
        at_node(g, w->switch_b, v3(-3, 0, 0));
        matrix_use(g, 0);
        CHECK(r->flags & MX_ROUTED);
        matrix_use(g, 0);
        CHECK(w->switches[0] == 0);
        CHECK(matrix_goal(g, 0, &goal));
        matrix_hint(g, 0, hint, sizeof(hint));
        CHECK(strstr(hint, "CONTROLLER"));
        at_node(g, w->credential, v3(-3, 0, -3));
        unsigned data = g->matrix.profiles[0].data;
        matrix_use(g, 0);
        CHECK(g->matrix.profiles[0].data > data);
        data = g->matrix.profiles[0].data;
        matrix_use(g, 0);
        CHECK(g->matrix.profiles[0].data == data);
        at_node(g, w->vault, v3(-3, 0, -3));
        if (w->rare)
            matrix_use(g, 0);
        if (!w->rare) {
            at_node(g, w->core, v3(0, 0, -2));
            d->service = MX_ARCHIVE;
            ready(g);
            CHECK(matrix_service(g, 0));
        }
        at_node(g, w->core, v3(0, 0, -2));
        matrix_use(g, 0);
        CHECK(d->choice);
        CHECK(!matrix_choose(g, 0, 0));
        CHECK(matrix_choose(g, 0, choice) && r->outcome == choice);
        data = g->matrix.profiles[0].data;
        CHECK(!matrix_choose(g, 0, choice));
        matrix_use(g, 0);
        CHECK(g->matrix.profiles[0].data == data);
        uint32_t flags = world_systems(&g->world, w->nodes[w->core].room);
        CHECK(choice == 2
                  ? (flags & SYSTEM_BLACKOUT) && !(flags & SYSTEM_POWER)
                  : (flags & (SYSTEM_POWER | SYSTEM_BEACON)) == (SYSTEM_POWER | SYSTEM_BEACON));
        CHECK(matrix_valid(&g->matrix, g->world.seed, g->world.generator, g->players, &g->shelter));
        CHECK(game_save(g, "matrix-roundtrip.sav"));
        Game *loaded = fresh(1);
        CHECK(game_load(loaded, "matrix-roundtrip.sav"));
        CHECK(loaded->matrix.divers[0].active && loaded->matrix.records[0].outcome == choice);
        CHECK(loaded->matrix.profiles[0].data == data && loaded->player_count == 1);
        destroy(loaded);
        remove("matrix-roundtrip.sav");
        destroy(g);
    }
}
static void co_op_and_body(void) {
    Game *g = fresh(713);
    game_join(g);
    g->players[1].pos = g->players[0].pos;
    CHECK(matrix_toggle(g, 0) && matrix_toggle(g, 1));
    CHECK(g->matrix.divers[0].network == g->matrix.divers[1].network);
    CHECK(game_save(g, "matrix-coop.sav"));
    CHECK(game_load(g, "matrix-coop.sav"));
    CHECK(g->player_count == 1 && g->matrix.divers[1].active);
    Vec3 saved = g->matrix.divers[1].pos;
    game_join(g);
    CHECK(vlen(vsub(saved, g->matrix.divers[1].pos)) == 0);
    game_leave(g);
    CHECK(!g->matrix.divers[1].active && g->matrix.divers[0].active);
    game_hurt(g, 0, 50, 0);
    CHECK(g->matrix.divers[0].active);
    matrix_disconnect(g, 0, false);
    Cell room = world_describe_version(g->world.seed, g->world.generator, (Cell){4, 0, 8}).cell;
    g->players[0].pos = position_from(matrix_terminal(room));
    Vec3 pos = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &pos, 1));
    while (g->world.pending)
        world_stream(&g->world, 2);
    CHECK(matrix_toggle(g, 0) && !matrix_protected(g, 0));
    game_hurt(g, 0, 5, 0);
    CHECK(!g->matrix.divers[0].active);
    g->shelter = (Shelter){g->players[0].pos, 0, true};
    g->players[0].sheltered = true;
    g->players[0].shelter_pos = shelter_station(SHELTER_TERMINAL);
    CHECK(game_shelter_near(g, 0) == SHELTER_TERMINAL && game_shelter_interact(g, 0));
    CHECK(g->matrix.divers[0].active && matrix_protected(g, 0));
    CHECK(game_save(g, "matrix-coop.sav") && game_load(g, "matrix-coop.sav"));
    CHECK(g->players[0].sheltered && g->matrix.divers[0].active);
    CHECK(g->matrix.divers[1].active == false);
    remove("matrix-coop.sav");
    destroy(g);
}
static void distinct_networks(void) {
    Game *g = fresh(713);
    game_join(g);
    CHECK(matrix_toggle(g, 0));
    Cell remote = world_describe_version(g->world.seed, g->world.generator, (Cell){4, 0, 8}).cell;
    g->players[1].pos = position_from(matrix_terminal(remote));
    Vec3 anchors[2] = {position_absolute(g->players[0].pos), position_absolute(g->players[1].pos)};
    CHECK(world_refresh(&g->world, anchors, 2));
    while (g->world.pending)
        world_stream(&g->world, 2);
    CHECK(matrix_toggle(g, 1));
    CHECK(g->matrix.divers[0].network != g->matrix.divers[1].network);
    unsigned network1 = g->matrix.divers[1].network;
    Cell key = g->matrix.divers[1].key;
    Vec3 avatar = g->matrix.divers[1].pos;
    CHECK(game_save(g, "matrix-distinct.sav") && game_load(g, "matrix-distinct.sav"));
    CHECK(g->player_count == 1 && g->matrix.divers[1].active);
    for (int i = 0; i < 30; i++)
        matrix_step(g, .02);
    CHECK(vlen(vsub(g->matrix.divers[1].pos, avatar)) == 0);
    game_join(g);
    CHECK(g->matrix.divers[1].network == network1 && cell_equal(g->matrix.divers[1].key, key));
    CHECK(vlen(vsub(g->matrix.divers[1].pos, avatar)) == 0);
    game_leave(g);
    // A reserved shelter link prevents packing its occupied entrance in solo mode.
    g->matrix.divers[1].active = true;
    g->players[1].sheltered = true;
    g->shelter.deployed = true;
    bool deployed = g->shelter.deployed;
    matrix_disconnect(g, 0, false);
    game_summon(g, 0);
    CHECK(g->shelter.deployed == deployed);
    g->matrix.divers[1].active = false;
    g->players[1].sheltered = false;
    remove("matrix-distinct.sav");
    destroy(g);
}
static void combat(void) {
    Game *g = fresh(713);
    CHECK(matrix_toggle(g, 0));
    clear_guards(g);
    MatrixDiver *d = &g->matrix.divers[0];
    MatrixNetwork *w = network(g);
    MatrixRecord *r = record(g);
    d->pos = v3(0, 0, 0);
    g->players[0].yaw = 0;
    g->players[0].pitch = 0;
    d->siphon = 8;
    w->guards[w->credential] = (MatrixGuard){.pos = {0, 1.4, 3}, .health = 10, .active = true};
    matrix_player_step(g, 0, &(Actions){.fire = true}, .02);
    CHECK(g->shots == 1 && d->veil == 0);
    for (int i = 0; i < 8; i++)
        matrix_step(g, .02);
    CHECK(r->killed & BIT(w->credential));
    CHECK(d->unbanked == 1);
    d->grace = 0;
    d->bandwidth = 80;
    w->shots[0] = (MatrixShot){.pos = {0, .8, 2},
                               .velocity = {0, 0, -14},
                               .life = 3,
                               .damage = 10,
                               .owner = -1,
                               .role = 2,
                               .active = true};
    for (int i = 0; i < 10; i++)
        matrix_step(g, .02);
    CHECK(d->integrity < 100 && d->bandwidth < 80);
    d->echo = v3(1, 1, 1);
    d->echo_life = 5;
    w->shots[0] = (MatrixShot){.pos = {1, 1, 2},
                               .velocity = {0, 0, -14},
                               .life = 3,
                               .damage = 10,
                               .owner = -1,
                               .active = true};
    for (int i = 0; i < 5; i++)
        matrix_step(g, .02);
    CHECK(d->echo_life == 0);
    d->trace = 100;
    w->guards[0] = (MatrixGuard){.pos = {0, 1, 2}, .health = 100, .active = true, .stun = 10};
    matrix_step(g, .02);
    CHECK(d->counter > 0 && r->alert == 3);
    for (int i = 0; i < 260 && d->active; i++)
        matrix_step(g, .02);
    CHECK(!d->active && d->lockout > 0);
    for (int i = 0; i < 170; i++)
        matrix_step(g, .05);
    CHECK(matrix_toggle(g, 0));
    d->pos = v3(-100, -10.1, -100);
    d->integrity = 5;
    matrix_player_step(g, 0, &(Actions){0}, .02);
    CHECK(!d->active && d->integrity == 0);
    matrix_step(g, 0);
    matrix_step(g, NAN);
    matrix_step(g, .1);
    destroy(g);
}
static void physical_robots_and_story(void) {
    Game *g = fresh(713);
    CHECK(matrix_toggle(g, 0));
    clear_guards(g);
    MatrixProfile *programs = &g->matrix.profiles[0];
    programs->programs |= BIT(MX_SIPHON);
    programs->data = 3;
    CHECK(matrix_equip(g, 0, MX_SIPHON) && matrix_upgrade(g, 0));
    at_node(g, network(g)->entry, v3(0, 0, -2));
    g->matrix.divers[0].service = MX_ROBOT;
    CHECK(matrix_service(g, 0));
    CHECK(g->matrix.controls[0].kind == 2 && g->matrix.controls[0].life == 20);
    Room *room = world_find(&g->world, (Cell){0});
    CHECK(room);
    room->enemy_count = 2;
    room->enemies[0] = (Enemy){.kind = 27,
                               .alive = true,
                               .health = 100,
                               .pos = position_from(v3(0, 0, 12)),
                               .normal = {0, 1, 0}};
    room->enemies[1] = (Enemy){.kind = 0,
                               .id = 1,
                               .alive = true,
                               .health = 100,
                               .pos = position_from(v3(0, 0, 16)),
                               .normal = {0, 1, 0}};
    Enemy *robot = &room->enemies[0];
    for (unsigned kind = 1; kind <= 3; kind++) {
        g->matrix.controls[0] = (MatrixControl){room->cell, 12, kind, 0};
        robot->cooldown = 0;
        CHECK(matrix_control_enemy(g, room, robot, .02) && robot->allegiance == kind);
        if (kind == 2) {
            CHECK(room->enemies[1].health < 100 && g->matrix.links[0].life > 0);
            CHECK(matrix_control_enemy(g, room, robot, .02));
        }
    }
    g->matrix.controls[0].kind = 1;
    CHECK(!matrix_control_enemy(g, room, &room->enemies[1], .02));
    g->matrix.controls[0].life = 0;
    CHECK(!matrix_control_enemy(g, room, robot, .02) && robot->allegiance == 0);
    // Actual AI consults the same room-owned control and returns to hostility on expiry.
    g->matrix.controls[0] = (MatrixControl){room->cell, .02f, 1, 0};
    game_step(g, (Actions[2]){{0}, {0}}, .02);
    CHECK(robot->allegiance == 1);
    game_step(g, (Actions[2]){{0}, {0}}, .02);
    CHECK(robot->allegiance == 0);
    matrix_disconnect(g, 0, false);
    g->players[0].pos = position_from(game_terminal((Cell){0}));
    game_interact(g, 0);
    CHECK(g->story.done & BIT(STORY_BRIEFING));
    Cell owner = world_describe_version(g->world.seed, g->world.generator, (Cell){4, 0, 8}).cell;
    g->story.assigned |= BIT(STORY_REACTOR) | BIT(STORY_POWER) | BIT(STORY_CIPHER);
    g->story.sites[STORY_REACTOR] = g->story.sites[STORY_POWER] = g->story.sites[STORY_CIPHER] =
        owner;
    unsigned credits = g->players[0].credits;
    CHECK(matrix_story_operation(g, 0, owner, MX_POWER));
    CHECK(g->story.done & BIT(STORY_REACTOR));
    CHECK(g->story.tracked == game_story_target(g));
    CHECK(g->players[0].credits == credits + 75);
    CHECK(matrix_story_operation(g, 0, owner, MX_POWER));
    CHECK(g->players[0].credits == credits + 75);
    g->story.assigned &= ~BIT(STORY_REACTOR);
    CHECK(matrix_story_operation(g, 0, owner, MX_POWER));
    CHECK(g->story.done & BIT(STORY_POWER));
    CHECK(matrix_story_operation(g, 0, owner, MX_ARCHIVE));
    CHECK(g->story.done & BIT(STORY_CIPHER));
    CHECK(!matrix_story_operation(g, 0, owner, MX_TRAFFIC));
    // A prior generic hack suppresses a subsequently assigned physical preparation payment.
    MatrixRecord *r = matrix_record(&g->matrix, matrix_region(owner), true);
    CHECK(r);
    unsigned node = (unsigned)((owner.z - r->key.z) * 4 + owner.x - r->key.x);
    r->done[MX_BEACON] = BIT(node);
    r->seen |= BIT(node);
    g->story.sites[STORY_BEACON] = owner;
    g->story.assigned |= BIT(STORY_BEACON);
    credits = g->players[0].credits;
    story_complete(g, 0, STORY_BEACON);
    CHECK(g->players[0].credits == credits);
    unsigned before = g->story.event_count;
    matrix_story_event(g, owner, 0, 0);
    matrix_story_event(g, owner, 0, 0);
    CHECK(g->story.event_count == before + 1);
    for (int i = 0; i < 40; i++)
        matrix_story_event(
            g, world_describe_version(g->world.seed, g->world.generator, (Cell){i * 4, 0, 16}).cell,
            6, 3);
    CHECK(g->story.event_count == SC_STORY_EVENTS);
    char text[640];
    game_story_debrief(g, 0, 8, text, sizeof(text));
    CHECK(strstr(text, "CHOICE 3"));
    destroy(g);
}
static void routing_and_guards(void) {
    bool vault = false;
    for (unsigned seed = 1; seed <= 12; seed++) {
        Game *g = fresh(seed);
        CHECK(matrix_toggle(g, 0));
        MatrixNetwork *w = network(g);
        MatrixDiver *d = &g->matrix.divers[0];
        MatrixRecord *r = record(g);
        clear_guards(g);
        if (w->rare) {
            at_node(g, w->vault, v3(-3, 0, -3));
            matrix_use(g, 0);
            CHECK(!(r->loot & BIT(w->vault)));
            r->flags = MX_ROUTED;
            matrix_use(g, 0);
            CHECK(r->flags & MX_VAULT);
            vault = true;
        }
        at_node(g, w->credential, v3(0, 0, -2));
        g->story.assigned |= BIT(STORY_OVERRIDE);
        g->story.sites[STORY_OVERRIDE] = w->nodes[w->credential].room;
        ready(g);
        d->service = MX_SECURITY;
        float bandwidth = d->bandwidth;
        CHECK(!matrix_service(g, 0) && d->bandwidth == bandwidth);
        CHECK(!(world_systems(&g->world, w->nodes[w->credential].room) & SYSTEM_SILENCED));
        g->story.assigned &= ~BIT(STORY_OVERRIDE);
        r->flags = 0;
        at_node(g, w->core, v3(0, 0, -2));
        matrix_use(g, 0);
        CHECK(!d->choice);
        r->flags = MX_ROUTED;
        d->bandwidth = 0;
        matrix_use(g, 0);
        CHECK(!d->choice);
        ready(g);
        matrix_use(g, 0);
        CHECK(d->choice && w->gate > 0 && d->trace >= 20);
        CHECK(!matrix_choose(g, 0, 3));
        d->choice = false;
        Vec3 goal;
        CHECK(matrix_goal(g, 0, &goal));
        r->outcome = 1;
        CHECK(matrix_goal(g, 0, &goal));
        r->outcome = 0;
        d->map = true;
        matrix_use(g, 0);
        d->map = false;
        d->pos = v3(-70, 0, -70);
        matrix_use(g, 0);
        at_node(g, w->entry, v3(0, 0, -2));
        w->guards[0] = (MatrixGuard){.pos = w->nodes[w->entry].at, .health = 100, .active = true};
        matrix_use(g, 0);
        clear_guards(g);
        d->grace = 0;
        d->pos = v3(0, 0, 0);
        for (unsigned role = 0; role < 4; role++) {
            clear_guards(g);
            ready(g);
            d->integrity = 100;
            d->veil = 0;
            d->grace = 0;
            d->pos = v3(0, 0, 0);
            d->counter = 0;
            w->guards[0] = (MatrixGuard){.pos = {0, 1, role == 1 ? 1 : 12},
                                         .health = 100,
                                         .role = (uint8_t)role,
                                         .active = true};
            float health = d->integrity;
            for (int i = 0; i < 100; i++)
                matrix_step(g, .02);
            if (role == 1)
                CHECK(d->integrity < health);
            else
                CHECK(g->matrix.networks[d->network].guards[0].cooldown > 0);
        }
        clear_guards(g);
        ready(g);
        d->echo = v3(0, 1, 1);
        d->echo_life = 5;
        d->veil = 10;
        w->guards[0] = (MatrixGuard){.pos = {0, 1, 1.2}, .health = 10, .role = 1, .active = true};
        matrix_step(g, .02);
        CHECK(d->echo_life == 0);
        clear_guards(g);
        d->grace = 1;
        d->integrity = 100;
        w->shots[0] = (MatrixShot){.pos = {0, .8, 0},
                                   .velocity = {0, 0, 14},
                                   .damage = 10,
                                   .owner = -1,
                                   .life = 2,
                                   .active = true};
        matrix_step(g, .02);
        CHECK(d->integrity == 100);
        // Disconnect while a pulse is in flight; committed kill data still has its owner.
        clear_guards(g);
        d->pos = v3(0, 0, 0);
        w->guards[w->credential] = (MatrixGuard){.pos = {0, 1.4, 2}, .health = 1, .active = true};
        w->shots[0] = (MatrixShot){.pos = {0, 1.4, 1.8},
                                   .velocity = {0, 0, 14},
                                   .damage = 10,
                                   .owner = 0,
                                   .life = 2,
                                   .active = true};
        game_join(g);
        g->players[1].pos = g->players[0].pos;
        CHECK(matrix_toggle(g, 1));
        matrix_disconnect(g, 0, false);
        unsigned data = g->matrix.profiles[0].data;
        matrix_step(g, .02);
        CHECK(g->matrix.profiles[0].data == data + 1);
        destroy(g);
    }
    CHECK(vault);
}
int main(void) {
    generation();
    entry_and_programs();
    consequences();
    co_op_and_body();
    combat();
    physical_robots_and_story();
    routing_and_guards();
    distinct_networks();
    puts("PASS matrix generation, programs, encounters, consequences, co-op and persistence");
    return 0;
}
