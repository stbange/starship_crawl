#include "session.h"
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
/* Explicit post-first-guardian fixture. After setup, movement/combat/choices use
   Session controls only. No actor removal or in-fight stat/resource edits. */
static void command(Session *s, CommandKind kind, unsigned arg) {
    GameCommand c = {.kind = kind, .player = game_player_id(s->game, 0), .argument = (int)arg};
    CHECK(session_command(s, &c));
}
static void walk(Session *s, Cell goal, Vec3 finish) {
    Game *g = s->game;
    Vec3 waypoint = finish, last = position_absolute(g->players[0].pos);
    bool have = false;
    double stalled = 0;
    unsigned before = g->respawns;
    for (unsigned tick = 0; tick < 60000; tick++) {
        Player *p = &g->players[0];
        Vec3 at = position_absolute(p->pos);
        if (vlen(vsub(at, finish)) < 1.8)
            return;
        if (g->respawns != before) {
            have = false;
            before = g->respawns;
        }
        Cell cell = world_cell(at);
        if (!have) {
            Navigation nav;
            CHECK(game_navigation_to(g, 0, goal, &nav) && nav.kind != NAV_BLOCKED);
            waypoint = cell_equal(cell, goal) ? finish : nav.point;
            have = true;
        }
        Vec3 delta = vsub(waypoint, at);
        delta.y = 0;
        double distance = vlen(delta);
        if (distance < .65)
            have = false;
        Vec3 aim = vnorm(delta);
        Actions input = {0};
        double nearest = 45;
        for (int r = 0; r < g->world.count; r++)
            for (int e = 0; e < g->world.rooms[r].enemy_count; e++) {
                Room *room = &g->world.rooms[r];
                Enemy *enemy = &room->enemies[e];
                int guardian = campaign_guardian(room->cell);
                if (!enemy->alive || encounter_passive(room, e) || enemy->allegiance == 2 ||
                    (enemy->country && !factions_hostile(g, enemy, 0)) ||
                    (guardian >= 0 && !g->campaign.attempts[guardian].active))
                    continue;
                Vec3 ray = vsub(enemy_center(enemy), player_eye(p));
                double range = vlen(ray);
                if (range < nearest &&
                    world_raycast(&g->world, player_eye(p), vnorm(ray), range) >= range - .1) {
                    nearest = range;
                    aim = vnorm(ray);
                    input.fire = true;
                }
            }
        GameCommand look = {.kind = CMD_AIM,
                            .player = game_player_id(g, 0),
                            .x = atan2(aim.x, aim.z),
                            .y = asin(clampd(aim.y, -1, 1))};
        CHECK(session_command(s, &look));
        Vec3 move = vnorm(delta);
        input.forward = (float)(move.x * sin(p->yaw) + move.z * cos(p->yaw));
        input.strafe = (float)(move.x * cos(p->yaw) - move.z * sin(p->yaw));
        Room *here = world_find(&g->world, cell);
        input.crouch = here && here->height < 2;
        input.jump = stalled > 2 && tick % 60 < 2;
        if (vlen(vsub(at, last)) > .5) {
            stalled = 0;
            last = at;
        } else
            stalled += SC_TICK_SECONDS;
        if (stalled > 8)
            have = false;
        CHECK(session_input(s, game_player_id(g, 0), &input));
        session_tick(s);
        CHECK(!g->world.failed && g->world.last_work <= SC_STREAM_BUDGET);
    }
    Vec3 at = position_absolute(g->players[0].pos);
    fprintf(stderr, "Route stalled at %.2f %.2f %.2f toward %.2f %.2f %.2f\n", at.x, at.y, at.z,
            finish.x, finish.y, finish.z);
    exit(1);
}
int main(void) {
    for (unsigned choice = 2; choice <= 3; choice++) {
        Game *g = calloc(1, sizeof(*g));
        CHECK(g);
        game_init(g, 713);
        g->campaign.defeated = 1;
        g->campaign.eligible[0] = 1;
        campaign_sync(g);
        Room defeated = {.cell = campaign_site(0), .record = SIZE_MAX, .enemy_count = 1};
        world_kill(&g->world, &defeated, 0);
        g->players[0].credits = 60;
        RecurringPerson *person = &g->chronicle.people[0];
        uint32_t identity = person->identity;
        Cell original = person->site;
        Vec3 at = vadd(chronicle_position(person), v3(0, 0, -1));
        g->players[0].pos = position_from(at);
        CHECK(world_refresh(&g->world, &at, 1));
        while (g->world.pending)
            CHECK(world_stream(&g->world, 2) <= 2);
        Session session;
        session_init(&session, g);
        command(&session, CMD_INTERACT, 0);
        command(&session, CMD_STORY_CHOOSE, choice);
        printf("MEETING choice=%u person=%u credits=%u home=[%d,%d,%d]\n", choice, identity,
               g->players[0].credits, original.x, original.y, original.z);
        fflush(stdout);
        walk(&session, (Cell){0}, v3(0, .02, 12));
        Actions idle = {0};
        CHECK(session_input(&session, game_player_id(g, 0), &idle));
        for (unsigned tick = 0; tick < 36000; tick++)
            session_tick(&session);
        printf(
            "SAVE validity campaign=%d story=%d factions=%d forces=%d cast=%d matrix=%d\n",
            campaign_valid(&g->campaign, g->players, &g->world),
            story_valid(&g->story, g->world.seed, g->world.generator, g->mission),
            factions_valid(&g->factions, g->world.generator),
            factions_world_valid(&g->factions, &g->world, g->world.open_layer),
            chronicle_valid(&g->chronicle, g->world.seed, g->world.generator),
            matrix_valid(&g->matrix, g->world.seed, g->world.generator, g->players, &g->shelter));
        CHECK(game_save(g, "inhabitants-journey.sav") && game_load(g, "inhabitants-journey.sav"));
        session_init(&session, g);
        person = &g->chronicle.people[0];
        CHECK(person->identity == identity);
        CHECK(choice == 2 ? person->fate == PERSON_LEFT : person->fate == PERSON_SETTLED);
        walk(&session, person->site, chronicle_position(person));
        command(&session, CMD_INTERACT, 0);
        char record[512];
        chronicle_entry(g, 0, record, sizeof(record));
        CHECK(strstr(record, choice == 2 ? "EVACUATION" : "REMEMBER"));
        printf("REVISIT choice=%u person=%u fate=%u site=[%d,%d,%d] time=%.2f kills=%u deaths=%u "
               "record=%s\n",
               choice, identity, person->fate, person->site.x, person->site.y, person->site.z,
               g->time, g->players[0].kills, g->respawns, record);
        fflush(stdout);
        game_destroy(g);
        free(g);
    }
    remove("inhabitants-journey.sav");
    puts("PASS paired Session journeys: leave, distant simulation, save/import and causal revisit");
}
