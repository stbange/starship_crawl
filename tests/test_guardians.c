#include "session.h"
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
static Game *fresh(unsigned id, unsigned players) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    g->player_count = (int)players;
    for (unsigned previous = 0; previous < id / 2; previous++) {
        Cell site = campaign_site(previous * 2);
        CHECK(world_set_systems(&g->world, site, 0));
        for (size_t n = 0; n < g->world.record_count; n++)
            if (cell_equal(g->world.records[n].cell, site))
                g->world.records[n].killed |= 1;
        g->campaign.defeated |= 1u << (previous * 2);
        g->campaign.eligible[previous * 2] = 1;
    }
    campaign_sync(g);
    Vec3 positions[SC_PLAYERS];
    for (unsigned p = 0; p < players; p++) {
        g->players[p].pos = position_from(game_terminal(campaign_site(id)));
        positions[p] = position_absolute(g->players[p].pos);
    }
    CHECK(world_refresh(&g->world, positions, (int)players));
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed);
    return g;
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static Room *room(Game *g, unsigned id) {
    Room *r = world_find(&g->world, campaign_site(id));
    CHECK(r && r->ready && r->enemy_count == 3);
    return r;
}
static void start(Game *g, unsigned id) {
    for (int i = 0; i < g->player_count; i++)
        g->players[i].pos = position_from(game_terminal(campaign_site(id)));
    CHECK(campaign_interact(g, 0));
    CHECK(g->campaign.attempts[id].active && g->campaign.attempts[id].stage == GUARDIAN_CYCLE);
}
static void systems(void) {
    for (unsigned id = 0; id < 8; id++) {
        Game *g = fresh(id, 2), *copy = fresh(id, 2);
        Room *r = room(g, id);
        CHECK(strlen(guardian_name(g, id)) && strlen(guardian_name(g, 8)));
        CHECK(strlen(guardian_system_name(id, 0)) && strlen(guardian_system_name(id, 1)));
        CHECK(guardian_pattern(id) == (id / 2 + id % 2) % 4);
        float hp = r->enemies[1].health;
        game_damage_enemy(g, r, 1, 0, 40, 0);
        CHECK(r->enemies[1].health == hp); /* Staging cannot be farmed. */
        CHECK(guardian_step(g, r, 1, .1) && guardian_step(g, r, 0, .1));
        CHECK(!guardian_prepare(g, -1) && !guardian_prepare(g, 2) && !guardian_prepare(g, 0));
        start(g, id);
        CHECK(g->campaign.attempts[id].participants == 3);
        CHECK(r->enemies[1].health > hp);
        g->players[0].sheltered = true;
        game_damage_enemy(g, r, 1, 0, 40, 0);
        CHECK(r->enemies[1].health > hp);
        campaign_step(g, .1);
        CHECK(g->campaign.attempts[id].active); /* Partner still fighting. */
        g->players[0].sheltered = false;
        unsigned kills = g->players[0].kills, xp = g->players[0].xp;
        game_damage_enemy(g, r, 1, 0, 40, 0);
        game_damage_enemy(g, r, 1, 0, 1000000, 0);
        GuardianAttempt *a = &g->campaign.attempts[id];
        CHECK(a->broken == 1 && a->opening == 5 && !r->enemies[1].alive);
        CHECK(g->players[0].kills == kills && g->players[0].xp == xp);
        CHECK(r->record == SIZE_MAX || !(g->world.records[r->record].killed & 6));
        unsigned char *data;
        size_t size;
        CHECK(game_snapshot_encode(g, &data, &size));
        CHECK(game_snapshot_decode(copy, data, size));
        free(data);
        CHECK(copy->campaign.attempts[id].broken == 1 && copy->campaign.attempts[id].opening == 5 &&
              !room(copy, id)->enemies[1].alive);
        a->stage = GUARDIAN_STAGED;
        CHECK(!game_snapshot_encode(g, &data, &size));
        a->stage = GUARDIAN_CYCLE;
        r->enemies[1].alive = true;
        CHECK(!game_snapshot_encode(g, &data, &size));
        r->enemies[1].alive = false;
        a->broken = 4;
        CHECK(!campaign_valid(&g->campaign, g->players, &g->world));
        a->broken = 1;
        a->stage = (GuardianStage)99;
        CHECK(!campaign_valid(&g->campaign, g->players, &g->world));
        a->stage = GUARDIAN_CYCLE;
        CHECK(game_save(g, "guardian-attempt.sav") && game_load(copy, "guardian-attempt.sav"));
        CHECK(!copy->campaign.attempts[id].active && !copy->campaign.attempts[id].broken);
        for (int p = 0; p < 2; p++)
            g->players[p].sheltered = true;
        campaign_step(g, .1);
        CHECK(!a->active && !a->broken && r->enemies[1].alive);
        g->players[0].sheltered = false;
        g->players[1].sheltered = false;
        for (unsigned part = 0; part < 2; part++) {
            g->players[0].pos = position_from(guardian_console(id, part));
            r->ready = false;
            CHECK(guardian_prepare(g, 0) && !world_systems(&g->world, r->cell));
            r->ready = true;
            if (part)
                CHECK(world_set_systems(&g->world, r->cell, 0));
            g->matrix.divers[0].active = true;
            CHECK(!guardian_prepare(g, 0));
            g->matrix.divers[0].active = false;
            CHECK(guardian_prepare(g, 0) && guardian_prepare(g, 0));
            CHECK(world_systems(&g->world, r->cell) == (part ? SYSTEM_SILENCED : SYSTEM_POWER));
            start(g, id);
            CHECK(a->broken == (1u << part) && !r->enemies[part + 1].alive);
            /* Physical and matrix services use exactly these durable room bits. */
            g->players[0].pos = position_from(guardian_console(id, part));
            CHECK(guardian_prepare(g, 0) && a->active);
            CHECK(game_save(g, "guardian-prepared.sav") &&
                  game_load(copy, "guardian-prepared.sav"));
            CHECK(world_systems(&copy->world, r->cell) == (part ? SYSTEM_SILENCED : SYSTEM_POWER));
            g->players[0].sheltered = g->players[1].sheltered = true;
            campaign_step(g, .1);
            g->players[0].sheltered = g->players[1].sheltered = false;
            if (!part)
                CHECK(world_set_systems(&g->world, r->cell, 0));
        }
        start(g, id);
        game_damage_enemy(g, r, 1, 1, 1000000, 0);
        CHECK(a->broken == 3);
        guardian_step(g, r, 0, .01);
        CHECK(a->stage == GUARDIAN_OVERRUN);
        unsigned partner_points = g->players[1].points;
        /* Strong legal damage wins immediately; no extra health or capped phase. */
        game_damage_enemy(g, r, 0, 0, 1000000, 0);
        CHECK((g->campaign.defeated & (1u << id)) && !a->active && !r->enemies[2].alive);
        CHECK(g->campaign.eligible[id] == 3 && campaign_valid(&g->campaign, g->players, &g->world));
        CHECK(g->players[1].points == partner_points + 4);
        campaign_defeat(g, id);
        CHECK(g->players[1].points == partner_points + 4);
        g->players[0].pos = position_from(guardian_console(id, 0));
        CHECK(guardian_prepare(g, 0));
        CHECK(game_save(g, "guardian-victory.sav") && game_load(copy, "guardian-victory.sav"));
        CHECK(copy->campaign.defeated == g->campaign.defeated);
        g->campaign.version = 1;
        CHECK(!guardian_step(g, r, 0, .1) && !guardian_prepare(g, 0));
        CHECK(!guardian_component_damage(g, r, 1, 0, 20));
        CHECK(strlen(guardian_name(g, id)));
        destroy(copy);
        destroy(g);
    }
    remove("guardian-attempt.sav");
    remove("guardian-prepared.sav");
    remove("guardian-victory.sav");
}
static void attacks(void) {
    for (unsigned id = 0; id < 8; id++) {
        Game *g = fresh(id, 1);
        Room *r = room(g, id);
        start(g, id);
        GuardianAttempt *a = &g->campaign.attempts[id];
        Enemy *e = &r->enemies[0];
        g->players[0].pos = position_from(vadd(world_center(r->cell), v3(0, .02, 0)));
        g->players[0].cloaked = true;
        CHECK(guardian_step(g, r, 0, .1));
        g->players[0].cloaked = false;
        unsigned previous = 0;
        for (unsigned broken = 0; broken < 4; broken++) {
            a->broken = broken;
            a->attack = 1; /* Compare the same volley in both patterns. */
            a->timer = 0;
            e->windup = 0;
            if (broken)
                a->stage = GUARDIAN_OVERRUN;
            CHECK(guardian_step(g, r, 0, .01) && e->windup == 1.2f);
            CHECK(guardian_step(g, r, 0, .5) && e->windup > 0);
            memset(g->projectiles, 0, sizeof(g->projectiles));
            unsigned before = g->enemy_shots;
            guardian_step(g, r, 0, .8);
            unsigned count = g->enemy_shots - before;
            CHECK(count >= 1 && a->opening > 0 && a->attack);
            if (broken == 3)
                CHECK(guardian_pattern(id) == 3 ? count == previous : count < previous);
            previous = count;
            guardian_step(g, r, 0, .1);
        }
        for (int i = 0; i < SC_MAX_PROJECTILES; i++)
            g->projectiles[i].active = true;
        unsigned shots = g->enemy_shots;
        e->windup = .01f;
        guardian_step(g, r, 0, .1);
        CHECK(g->enemy_shots == shots);
        e->slow = 1;
        e->burn = 1;
        e->last_player = 0;
        float health = e->health;
        guardian_step(g, r, 0, .1);
        CHECK(e->health < health);
        e->health = .01f;
        e->burn = 1;
        guardian_step(g, r, 0, .1);
        CHECK(!e->alive);
        destroy(g);
    }
}
static void platforms(void) {
    for (unsigned id = 0; id < 8; id++) {
        Game *g = fresh(id, 2);
        Room *r = room(g, id);
        r->ready = false;
        CHECK(campaign_interact(g, 0) && !g->campaign.attempts[id].active);
        r->ready = true;
        start(g, id);
        bool occupied[3][3][4] = {0};
        for (unsigned transition = 0; transition < 3; transition++) {
            for (unsigned rig = 0; rig <= 2; rig++) {
                Player shape = {.riding = (uint8_t)rig};
                for (int x = -1; x <= 1; x++)
                    for (int z = -1; z <= 2; z++) {
                        Vec3 at = vadd(world_center(r->cell), v3(x * 8, .02, z * 12));
                        bool clear = !world_overlap(&g->world, at, game_body_radius(&shape),
                                                    game_body_height(&shape));
                        if (!transition)
                            occupied[rig][x + 1][z + 1] = clear;
                        CHECK(clear == occupied[rig][x + 1][z + 1]);
                        if (x == -1)
                            CHECK(clear); /* Continuous ordinary approach. */
                        if (clear)
                            CHECK(world_spawn_supported(&g->world, at, game_body_height(&shape)));
                    }
                /* Existing cover platforms remain usable under every gantry stage. */
                Vec3 top = vadd(world_center(r->cell), v3(14, 3.02, 18));
                CHECK(!world_overlap(&g->world, top, game_body_radius(&shape),
                                     game_body_height(&shape)));
                CHECK(world_spawn_supported(&g->world, top, game_body_height(&shape)));
            }
            if (transition < 2) {
                game_damage_enemy(g, r, (int)transition + 1, 0, 1000000, 0);
                guardian_step(g, r, 0, .01);
            } else
                game_damage_enemy(g, r, 0, 0, 1000000, 0);
        }
        CHECK(campaign_phase(&g->campaign) == id / 2 + 1 &&
              g->world.open_layer == (id / 2 == 3 ? 0 : id / 2 + 2));
        destroy(g);
    }
}
static void matrix_preparation(void) {
    Game *g = fresh(0, 1);
    g->players[0].pos = position_from(matrix_terminal(campaign_site(0)));
    CHECK(!matrix_toggle(g, 0)); /* A preparation route never grants early hacking. */
    start(g, 0);
    game_damage_enemy(g, room(g, 0), 0, 0, 1000000, 0);
    /* Separately labelled earned-interface fixture; onboarding tests exercise
       recovering it and restoring its first grid through the physical route. */
    g->players[0].intro = INTRO_INTERFACE | INTRO_GRID;
    g->players[0].pos = position_from(matrix_terminal(campaign_site(1)));
    Vec3 position = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &position, 1));
    while (g->world.pending)
        world_stream(&g->world, 2);
    CHECK(matrix_toggle(g, 0));
    MatrixDiver *d = &g->matrix.divers[0];
    MatrixNetwork *n = &g->matrix.networks[d->network];
    for (int i = 0; i < SC_MATRIX_GUARDS; i++)
        n->guards[i].active = false;
    unsigned node = 0;
    while (node < 16 &&
           (!n->nodes[node].valid || !cell_equal(n->nodes[node].room, campaign_site(1))))
        node++;
    CHECK(node < 16);
    d->pos = n->nodes[node].at;
    for (unsigned service = MX_POWER; service <= MX_SECURITY; service++) {
        d->service = service;
        d->bandwidth = 100;
        d->cooldown = 0;
        CHECK(matrix_service(g, 0));
    }
    unsigned credits = g->players[0].credits;
    CHECK(matrix_service(g, 0) && g->players[0].credits == credits);
    matrix_disconnect(g, 0, false);
    CHECK((world_systems(&g->world, campaign_site(1)) & (SYSTEM_POWER | SYSTEM_SILENCED)) ==
          (SYSTEM_POWER | SYSTEM_SILENCED));
    start(g, 1);
    CHECK(g->campaign.attempts[1].broken == 3);
    game_damage_enemy(g, room(g, 1), 0, 0, 1000000, 0);
    CHECK(g->campaign.defeated == 3);
    destroy(g);
}
static void journey(void) {
    for (unsigned id = 0; id < 8; id++)
        for (unsigned prepared = 0; prepared < 2; prepared++) {
            Game *g = fresh(id, 1);
            Room *r = room(g, id);
            for (int j = 0; j < g->world.count; j++)
                if (&g->world.rooms[j] != r)
                    g->world.rooms[j].enemy_count = 0;
            if (prepared) {
                g->players[0].pos = position_from(guardian_console(id, 0));
                game_interact(g, 0);
            }
            start(g, id);
            Player *p = &g->players[0];
            p->pos = position_from(vadd(world_center(r->cell), v3(0, .02, 0)));
            Session s;
            session_init(&s, g);
            unsigned frame;
            for (frame = 0; frame < 18000 && !g->respawns && !(g->campaign.defeated & (1u << id));
                 frame++) {
                Enemy *target = r->enemies[1].alive   ? &r->enemies[1]
                                : r->enemies[2].alive ? &r->enemies[2]
                                                      : &r->enemies[0];
                Vec3 to = vsub(enemy_center(target), player_eye(p));
                GameCommand aim = {.kind = CMD_AIM,
                                   .player = game_player_id(g, 0),
                                   .x = atan2(to.x, to.z),
                                   .y = asin(clampd(vnorm(to).y, -1, 1))};
                CHECK(session_command(&s, &aim));
                Actions input = {.fire = true, .strafe = frame % 360 < 180 ? .7f : -.7f};
                CHECK(session_input(&s, game_player_id(g, 0), &input));
                session_tick(&s);
            }
            printf("guardian %u prepared %u frames %u hp %.1f deaths %u core %.1f broken %u\n", id,
                   prepared, frame, p->health, g->respawns, r->enemies[0].health,
                   g->campaign.attempts[id].broken);
            CHECK(frame < 18000 && !g->respawns && (g->campaign.defeated & (1u << id)));
            CHECK(!p->power && !p->vitality && p->weapons == 1);
            destroy(g);
        }
}
int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--fixtures")) {
        for (unsigned id = 0; id < 8; id++) {
            Game *g = fresh(id, 1);
            g->campaign.tracked = id;
            char path[48];
            snprintf(path, sizeof(path), "coherent-guardian%u.sav", id);
            CHECK(game_save(g, path));
            destroy(g);
        }
    } else if (argc > 1)
        journey();
    else {
        systems();
        attacks();
        platforms();
        matrix_preparation();
    }
    puts("PASS guardian systems, preparation, warnings, stages and retry boundaries");
}
