#include "session.h"
#include "actors.h"
#include "snapshot.h"
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
static Game *fresh(void) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void identity_and_views(void) {
    Game *g = fresh();
    CHECK(!game_party_resize(g, 0) && !game_party_resize(g, SC_PLAYERS + 1));
    CHECK(game_party_resize(g, SC_PLAYERS));
    PlayerId last = game_player_id(g, SC_PLAYERS - 1);
    CHECK(game_player_slot(g, last) == SC_PLAYERS - 1);
    CHECK(game_player_slot(g, game_player_id(g, -1)) == -1);
    ViewBindings views = {0};
    CHECK(!game_bind_view(g, &views, SC_LOCAL_VIEWS, last));
    CHECK(!game_bind_view(g, &views, 0, (PlayerId){0}));
    CHECK(game_bind_view(g, &views, 0, last));
    CHECK(game_view_slot(g, &views, 0) == SC_PLAYERS - 1);
    CHECK(game_view_slot(g, &views, 1) == -1);
    g->views[0].guide_mode = GUIDE_OFF;
    g->views[0].journal = true;
    game_map_toggle(g, SC_PLAYERS - 1);
    CHECK(g->views[SC_PLAYERS - 1].map_open && !g->views[0].map_open);
    uint64_t a = g->presentation.sequence, b = a;
    game_player_message(g, last, "PRIVATE");
    PresentationEvent event;
    CHECK(!game_event_next(g, game_player_id(g, 0), &a, &event));
    CHECK(game_event_next(g, last, &b, &event) && !strcmp(event.text, "PRIVATE"));
    CHECK(strcmp(g->views[0].message, "PRIVATE"));
    CHECK(!game_event_next(g, (PlayerId){0}, &b, &event));
    game_player_message(g, (PlayerId){999, 0, 0}, "STALE");
    for (unsigned i = 0; i < SC_PRESENTATION_EVENTS + 3; i++)
        game_message(g, "BROADCAST");
    a = 0;
    unsigned seen = 0;
    while (game_event_next(g, last, &a, &event)) {
        seen++;
        CHECK(!event.recipient.epoch);
    }
    CHECK(seen == SC_PRESENTATION_EVENTS);
    game_views_reset(g);
    CHECK(g->views[0].guide_mode == GUIDE_OFF && !g->views[0].journal);
    CHECK(!g->presentation.sequence);
    CHECK(game_party_resize(g, 1));
    CHECK(game_player_slot(g, last) == -1);
    CHECK(game_party_resize(g, SC_PLAYERS));
    CHECK(game_player_slot(g, last) == -1); /* A departed identity never aliases its replacement. */
    last = game_player_id(g, 0);
    game_new_ship(g, 714);
    CHECK(game_player_slot(g, last) == -1 && game_view_slot(g, &views, 0) == -1);
    g->epoch = UINT32_MAX;
    game_identity_reset(g);
    CHECK(g->epoch == 1);
    destroy(g);
}
static void commands(void) {
    Game *g = fresh();
    GameCommand c = {CMD_LOOK, game_player_id(g, 0), 0, 0, .1, .2};
    CHECK(!game_command(g, NULL));
    CHECK(game_command(g, &c));
    CHECK(fabs(g->players[0].yaw - .1) < 1e-9);
    CHECK(fabs(g->players[0].pitch - .16) < 1e-9);
    c.kind = CMD_AIM;
    CHECK(game_command(g, &c));
    c.x = 4;
    CHECK(!game_command(g, &c));
    c.x = NAN;
    CHECK(!game_command(g, &c));
    c.x = 0;
    c.y = INFINITY;
    CHECK(!game_command(g, &c));
    c.y = 0;
    c.player.epoch++;
    CHECK(!game_command(g, &c));
    c.player = game_player_id(g, 0);
    c.kind = CMD_FLASHLIGHT;
    c.x = .1;
    CHECK(!game_command(g, &c));
    c.x = 0;
    c.kind = CMD_COUNT;
    CHECK(!game_command(g, &c));
    CHECK(!strcmp(game_command_binding(CMD_COUNT, 0), ""));
    CHECK(!strcmp(game_command_binding(CMD_INTERACT, 99), "G"));
    for (int k = 0; k < CMD_COUNT; k++) {
        c.kind = (CommandKind)k;
        c.argument = sc_commands[k].minimum - 1;
        CHECK(!game_command(g, &c));
        c.argument = sc_commands[k].maximum + 1;
        CHECK(!game_command(g, &c));
        c.argument = sc_commands[k].minimum;
        c.auxiliary = 5;
        CHECK(!game_command(g, &c));
        c.auxiliary = 0;
        CHECK(sc_commands[k].name[0]);
        for (unsigned device = 0; device < 4; device++)
            CHECK(game_command_binding((CommandKind)k, device)[0]);
    }
    c = (GameCommand){CMD_WEAPON_CYCLE, game_player_id(g, 0), 0, 0, 0, 0};
    CHECK(!game_command(g, &c));
    c.kind = CMD_MATRIX_CYCLE;
    CHECK(!game_command(g, &c));
    c.kind = CMD_WEAPON_SELECT;
    c.argument = 10;
    CHECK(!game_command(g, &c));
    c.kind = CMD_RIG_WEAPON;
    c.argument = 0;
    CHECK(!game_command(g, &c));
    c.kind = CMD_PSIONIC;
    CHECK(!game_command(g, &c));
    c.kind = CMD_SHELTER;
    CHECK(!game_command(g, &c));
    c.kind = CMD_RIG_TOGGLE;
    c.argument = MECH;
    CHECK(!game_command(g, &c));
    g->campaign.version = 0;
    g->world.generator = 6;
    g->world.open_layer = 0;
    g->players[0].weapons = (1u << SC_WEAPONS) - 1;
    g->players[0].points = 12;
    g->players[0].credits = 1000;
    for (int k = CMD_UPGRADE; k < CMD_COUNT; k++) {
        c = (GameCommand){(CommandKind)k, game_player_id(g, 0), sc_commands[k].minimum, 0, 0, 0};
        if (k == CMD_CAMPAIGN_REFIT)
            c.auxiliary = 1;
        (void)game_command(g, &c);
    }
    CHECK(g->players[0].power == 1);
    CHECK(g->players[0].tint == 1);
    CHECK(g->players[0].weapon == 0);
    CHECK(!g->players[0].flashlight);
    destroy(g);
}
static void clocks_and_party(void) {
    Game *reference = NULL;
    for (int rate = 30; rate <= 120; rate *= 2) {
        Game *g = fresh();
        Session s;
        session_init(&s, g);
        PlayerId id = game_player_id(g, 0);
        CHECK(session_assign(&s, id, (ConnectionId){7}));
        CHECK(session_owned(&s, id, (ConnectionId){7}));
        CHECK(!session_owned(&s, id, (ConnectionId){8}));
        CHECK(!session_assign(&s, (PlayerId){0}, (ConnectionId){7}));
        Actions input = {.forward = .7f};
        CHECK(session_input(&s, id, &input));
#ifdef SC_WEBXR
        input.aimed = true;
        input.aim_direction = v3(0, 0, 1);
        CHECK(session_input(&s, id, &input));
        input.aim_offset.x = NAN;
        CHECK(!session_input(&s, id, &input));
        input.aim_offset.x = 0;
        input.aim_direction = v3(0, 0, 0);
        CHECK(!session_input(&s, id, &input));
        input.aimed = false;
        CHECK(session_input(&s, id, &input));
#endif
        input.strafe = NAN;
        CHECK(!session_input(&s, id, &input));
        input.strafe = 3;
        CHECK(!session_input(&s, id, &input));
        input.strafe = 0;
        CHECK(!session_input(&s, (PlayerId){0}, &input));
        CHECK(!session_advance(&s, NAN, false));
        CHECK(!session_advance(&s, -1, false));
        uint64_t issued = UINT64_MAX;
        for (int frame = 0; frame < rate / 2; frame++) {
            if (s.tick % 6 == 0 && issued != s.tick) {
                GameCommand command = {CMD_LOOK, id, 0, 0, .03, .01};
                CHECK(session_command(&s, &command));
                issued = s.tick;
            }
            session_advance(&s, 1.0 / rate, false);
        }
        CHECK(s.tick == 30);
        if (reference) {
            unsigned char *a, *b;
            size_t na, nb;
            CHECK(game_snapshot_encode(g, &a, &na));
            CHECK(game_snapshot_encode(reference, &b, &nb));
            CHECK(na == nb && !memcmp(a, b, na));
            free(a);
            free(b);
            CHECK(g->world.generated == reference->world.generated);
            CHECK(g->world.count == reference->world.count);
            for (int p = 0; p < SC_PLAYERS; p++) {
                CHECK(vlen(position_delta(g->players[p].pos, reference->players[p].pos)) < 1e-12);
                CHECK(g->players[p].health == reference->players[p].health);
                CHECK(g->players[p].yaw == reference->players[p].yaw);
                CHECK(g->players[p].cooldown == reference->players[p].cooldown);
            }
            for (int r = 0; r < g->world.count; r++) {
                CHECK(cell_equal(g->world.rooms[r].cell, reference->world.rooms[r].cell));
                CHECK(g->world.rooms[r].hash == reference->world.rooms[r].hash);
                CHECK(g->world.rooms[r].enemy_count == reference->world.rooms[r].enemy_count);
            }
            destroy(g);
        } else
            reference = g;
    }
    Session s;
    session_init(&s, reference);
    CHECK(session_advance(&s, 100, false) == SC_MAX_CATCHUP_TICKS);
    double time = reference->time;
    CHECK(!session_advance(&s, 5, true));
    CHECK(reference->time == time && s.accumulator == 0);
    GameCommand c = {CMD_FLASHLIGHT, game_player_id(reference, 0), 0, 0, 0, 0};
    CHECK(session_command(&s, &c));
    game_new_ship(reference, 123);
    CHECK(!session_owned(&s, c.player, (ConnectionId){0}));
    session_sync(&s);
    CHECK(s.tick == 0 && s.views.count == 1);
    CHECK(game_party_resize(reference, SC_PLAYERS));
    session_sync(&s);
    CHECK(s.views.count == SC_LOCAL_VIEWS);
    CHECK(!session_set_local_views(&s, SC_LOCAL_VIEWS + 1));
    CHECK(session_set_local_views(&s, 1));
    CHECK(game_bind_view(reference, &s.views, 0, game_player_id(reference, SC_PLAYERS - 1)));
    session_tick(&s);
    CHECK(s.views.count == 1 && game_view_slot(reference, &s.views, 0) == SC_PLAYERS - 1);
    CHECK(session_set_local_views(&s, 0));
    session_tick(&s);
    CHECK(!s.views.count);
    CHECK(session_set_local_views(&s, SC_LOCAL_VIEWS));
    reference->campaign.version = 0;
    reference->world.open_layer = 0;
    for (int i = 0; i < SC_PLAYERS; i++) {
        reference->players[i].pos = position_from(v3(i * 9600, 64 * i + .02, 9600));
        reference->players[i].points = 1;
        reference->world.generator = 6;
        c = (GameCommand){CMD_UPGRADE, game_player_id(reference, i), 0, 0, 0, 0};
        CHECK(session_command(&s, &c));
        CHECK(reference->players[i].power == 1);
        Actions a = {.forward = .2f};
        CHECK(session_input(&s, c.player, &a));
    }
    for (int i = 0; i < 70; i++)
        session_tick(&s);
    CHECK(!reference->world.failed && reference->world.anchor_count == SC_PLAYERS);
    for (int i = 0; i < SC_PLAYERS; i++)
        CHECK(world_ready(&reference->world,
                          world_cell(position_absolute(reference->players[i].pos))));
    CHECK(session_assign(&s, game_player_id(reference, 1), (ConnectionId){9}));
    CHECK(game_party_resize(reference, 1));
    CHECK(game_party_resize(reference, SC_PLAYERS));
    CHECK(!session_owned(&s, game_player_id(reference, 1), (ConnectionId){9}));
    CHECK(game_party_resize(reference, 1));
    session_sync(&s);
    CHECK(s.views.count == 1);
    CHECK(!s.inputs[1].forward && !s.owners[1].value);
    CHECK(game_party_resize(reference, SC_PLAYERS));
    session_sync(&s);
    CHECK(!s.inputs[1].forward && !s.owners[1].value);
    reference->world.generator = 15;
    Room *room = &reference->world.rooms[0];
    uint32_t eligible = 0, claims = 0;
    for (unsigned i = 0; i < SC_PLAYERS; i++) {
        eligible |= world_eligible_bit(i);
        claims |= world_claim_bit(i);
    }
    CHECK(world_mark_local(&reference->world, room, eligible | claims));
    CHECK(world_participants(world_local_state(&reference->world, room)) == SC_PLAYER_MASK);
    CHECK(campaign_guardian_health(0, 2) == campaign_guardian_health(0, 1));
    CHECK(campaign_guardian_health(0, SC_PLAYER_MASK) >= campaign_guardian_health(0, 3));
    if (SC_PLAYERS > SC_LEGACY_PLAYERS)
        CHECK(campaign_guardian_health(0, SC_PLAYER_MASK) > campaign_guardian_health(0, 3));
    if (SC_PLAYERS != SC_LEGACY_PLAYERS) {
        CHECK(!game_save(reference, "unused-party.sav"));
        CHECK(!game_load(reference, "unused-party.sav"));
    }
    destroy(reference);
}
static void expanded_legacy_rewards(void) {
    Game *g = fresh();
    g->world.generator = 13;
    politics_init(g);
    unsigned player = SC_PLAYERS - 1;
    g->politics.countries[0] =
        (CountryState){.met = 1, .pledge = 1, .stage = 3, .claimed = 1u << player};
    for (unsigned role = COURT_CONDUIT; role <= COURT_FINALE; role++) {
        Room room = {.cell = politics_site(g->world.seed, 0, (CourtRole)role), .record = SIZE_MAX};
        CHECK(world_mark_local(&g->world, &room, FRONTIER_CLEARED | world_eligible_bit(player)));
    }
    CHECK(politics_valid(&g->politics, 13));
    CHECK(politics_world_valid(&g->politics, &g->director, &g->world));
    g->politics.countries[0].claimed = 1u << (player - 1);
    CHECK(!politics_world_valid(&g->politics, &g->director, &g->world));
    destroy(g);
}
static void actors(void) {
    Game *g = fresh();
    g->world.count = 2;
    Room *r = &g->world.rooms[1];
    *r = (Room){.cell = {2, 0, 1}, .residency = 42, .enemy_count = 1};
    r->enemies[0] = (Enemy){.id = 7, .alive = true};
    ActorId id = game_actor_id(g, r, &r->enemies[0]);
    CHECK(game_actor(g, id) == &r->enemies[0]);
    g->world.rooms[0] = *r;
    g->world.count = 1;
    CHECK(game_actor(g, id) == &g->world.rooms[0].enemies[0]);
    g->world.rooms[0].residency++;
    CHECK(!game_actor(g, id));
    r = &g->world.rooms[0];
    r->enemies[0].force = 17;
    r->enemies[0].member = 2;
    id = game_actor_id(g, r, &r->enemies[0]);
    r->cell.x++;
    r->residency++;
    CHECK(game_actor(g, id) == &r->enemies[0]);
    r->enemies[0].alive = false;
    CHECK(!game_actor(g, id));
    game_identity_reset(g);
    CHECK(!game_actor(g, id));
    CHECK(!game_actor(g, (ActorId){0}));
    destroy(g);
}
int main(void) {
    identity_and_views();
    commands();
    clocks_and_party();
    expanded_legacy_rewards();
    actors();
    printf("PASS: session architecture / capacity %d / local views %d\n", SC_PLAYERS,
           SC_LOCAL_VIEWS);
    return 0;
}
