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
static Game *fresh(void) {
    Game *g = calloc(1, sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    return g;
}
static void finish(Game *g) {
    game_destroy(g);
    free(g);
}
static void settle(Game *g) {
    Vec3 at[2] = {position_absolute(g->players[0].pos), position_absolute(g->players[1].pos)};
    CHECK(world_refresh(&g->world, at, g->player_count));
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, 2);
    CHECK(!g->world.failed);
}
static void win(Game *g, unsigned id) {
    g->players[0].pos = position_from(game_terminal(campaign_site(id)));
    settle(g);
    CHECK(campaign_interact(g, 0));
    Room *r = world_find(&g->world, campaign_site(id));
    CHECK(r && g->campaign.attempts[id].active);
    game_damage_enemy(g, r, 0, 0, 1000000, 0);
    CHECK(g->campaign.defeated & (1u << id));
}
static void starters(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    CHECK(onboarding_active(g) && p->weapons == 1);
    CHECK(g->campaign.profiles[0].guns[1] == SC_EMPTY_GUN);
    CHECK(!onboarding_capabilities(g, -1) && !onboarding_capabilities(g, 2));
    CHECK(!onboarding_capabilities(g, 0));
    CHECK(!onboarding_action(g, -1, 8));
    for (int action = 1; action <= 241; action++)
        (void)onboarding_action(g, 0, action);
    CHECK(onboarding_action(g, 0, 14) && onboarding_action(g, 0, 240));
    p->points = 2;
    CHECK(!game_upgrade(g, 0, 0));
    game_summon(g, 0);
    CHECK(!g->shelter.deployed);
    CHECK(!matrix_toggle(g, 0) && !matrix_equip(g, 0, MX_VEIL) && !matrix_cast(g, 0, 0));
    CHECK(!matrix_upgrade(g, 0));
    Actions actions[2] = {0};
    p->kills = 3;
    onboarding_step(g, actions, .02);
    CHECK(p->weapons == 3 && (onboarding_capabilities(g, 0) & OB_GUNS));
    CHECK(game_upgrade(g, 0, 0) && !game_upgrade(g, 0, 3));
    CHECK(!(onboarding_capabilities(g, 0) & OB_FULL_STATS));
    unsigned before = p->points;
    onboarding_step(g, actions, .02);
    CHECK(p->points == before); /* Award once, never convert to duplicate salvage. */
    win(g, 8);
    CHECK(onboarding_capabilities(g, 0) & OB_SHELTER);
    CHECK(onboarding_capabilities(g, 1) & OB_SHELTER); /* Shared refuge for a late partner. */
    CHECK(!(onboarding_capabilities(g, 0) & OB_MATRIX));
    win(g, 0);
    CHECK(onboarding_capabilities(g, 0) & OB_FULL_STATS);
    p->points = 2;
    CHECK(game_upgrade(g, 0, 3));
    for (unsigned id = 0; id < 9; id++)
        for (unsigned choice = 0; choice < 3; choice++) {
            CampaignReward r = campaign_offer(g, id, choice);
            CHECK((id > 1 && id != 8) || r.kind == REWARD_GUN);
            CHECK(id > 3 || r.kind < REWARD_RIG);
        }
    campaign_grant(g, 0, (CampaignReward){REWARD_POWER, PSI_VEIL});
    campaign_grant(g, 0, (CampaignReward){REWARD_RIG, MECH});
    p->weapons |= 1u << SC_FLARE_CANNON;
    for (int action = 1; action <= 241; action++)
        (void)onboarding_action(g, 0, action);
    CHECK(onboarding_action(g, 0, 15) && !onboarding_action(g, 0, 16));
    CHECK(onboarding_action(g, 0, 60) && !onboarding_action(g, 0, 61));
    CHECK(onboarding_action(g, 0, 70) && onboarding_action(g, 0, 17));
    campaign_journal(g, 0);
    CHECK(onboarding_action(g, 0, 226));
    CHECK(game_save(g, "onboarding-start.sav") && game_load(g, "onboarding-start.sav"));
    game_new_ship(g, 42);
    CHECK(!g->players[0].intro && !g->players[0].lessons && g->players[0].weapons == 1);
    g->world.generator = 14;
    campaign_init(g);
    matrix_init(&g->matrix);
    CHECK(!onboarding_active(g) && (onboarding_capabilities(g, 0) & OB_MATRIX));
    CHECK(onboarding_action(g, 0, 206));
    CHECK(game_save(g, "onboarding-old.sav") && game_load(g, "onboarding-old.sav"));
    CHECK(g->world.generator == 14 && g->players[0].weapons == 3);
    g->campaign.version = 0;
    CHECK((onboarding_capabilities(g, 0) & (OB_RIGS | OB_POWERS)) == (OB_RIGS | OB_POWERS));
    onboarding_step(g, actions, .02);
    CHECK(onboarding_reward(g, (CampaignReward){REWARD_RIG, MECH}, 0).kind == REWARD_RIG);
    remove("onboarding-start.sav");
    remove("onboarding-old.sav");
    finish(g);
}
static void matrix_introduction(void) {
    Game *g = fresh();
    CHECK(!onboarding_interface_ready(g, -1) && !onboarding_interface_ready(g, 2));
    CHECK(!onboarding_interact(g, 0));
    win(g, 0);
    Player *p = &g->players[0];
    Cell cell = {4, 0, 2};
    RoomSpec spec = world_describe_version(g->world.seed, g->world.generator, cell);
    p->pos = position_from(vadd(matrix_terminal(spec.cell), v3(0, 0, -2.5)));
    settle(g);
    Room *room = world_find(&g->world, cell);
    CHECK(room && room->ready && campaign_guardian(room->cell) < 0);
    p->hurt = 1;
    CHECK(!onboarding_interface_ready(g, 0));
    CHECK(onboarding_interact(g, 0) && !p->intro);
    for (int i = 0; i < room->enemy_count; i++)
        if (room->enemies[i].alive)
            world_kill(&g->world, room, i);
    p->hurt = 0;
    for (int n = 0; n < g->world.count; n++)
        for (int i = 0; i < g->world.rooms[n].enemy_count; i++) {
            Enemy *e = &g->world.rooms[n].enemies[i];
            if (e->alive && vlen(vsub(enemy_center(e), player_eye(p))) < 28)
                world_kill(&g->world, &g->world.rooms[n], i);
        }
    CHECK(onboarding_interface_ready(g, 0));
    if (room->enemy_count) {
        room->enemies[0].alive = true;
        CHECK(encounter_passive(room, 0));
        CHECK(onboarding_interface_ready(g, 0));
        world_kill(&g->world, room, 0);
    }
    CHECK(!matrix_toggle(g, 0)); /* Recover, then separately choose to log in. */
    CHECK(p->intro == INTRO_INTERFACE && !g->matrix.divers[0].active);
    CHECK(onboarding_action(g, 0, 200) && !onboarding_action(g, 0, 206));
    CHECK(!onboarding_interface_ready(g, 0));
    CHECK(!onboarding_matrix_intro(g, -1) && !onboarding_matrix_intro(g, 2));
    game_interact(g, 0);
    CHECK(g->matrix.divers[0].active && onboarding_matrix_intro(g, 0));
    CHECK(matrix_protected(g, 0));
    Actions actions[2] = {0};
    p->points = 2;
    p->guide_quiet = 3;
    onboarding_step(g, actions, .02);
    CHECK(p->lesson == LESSON_MATRIX); /* The active interface takes priority over body lessons. */
    MatrixDiver *d = &g->matrix.divers[0];
    CHECK(d->service == MX_POWER && !matrix_cast(g, 0, 0));
    matrix_cycle_service(g, 0, 1);
    CHECK(d->service == MX_POWER && !matrix_equip(g, 0, MX_INTERRUPT));
    CHECK(game_save(g, "onboarding-link.sav") && game_load(g, "onboarding-link.sav"));
    CHECK(onboarding_matrix_intro(g, 0) && g->matrix.divers[0].active);
    matrix_use(g, 0);
    CHECK(!g->matrix.divers[0].active && g->players[0].intro == 3);
    CHECK(world_systems(&g->world, spec.cell) & SYSTEM_POWER);
    CHECK(g->matrix.profiles[0].programs == 7);
    CHECK(onboarding_action(g, 0, 206) && onboarding_action(g, 0, 204));
    settle(g);
    game_interact(g, 0);
    CHECK(g->matrix.divers[0].active && !onboarding_matrix_intro(g, 0));
    g->players[0].guide_quiet = 3;
    onboarding_step(g, actions, .02);
    CHECK(g->players[0].lesson == LESSON_NONE); /* No body controls while inside the matrix. */
    CHECK(matrix_equip(g, 0, MX_INTERRUPT));
    matrix_disconnect(g, 0, false);
    CHECK(game_save(g, "onboarding-link.sav") && game_load(g, "onboarding-link.sav"));
    CHECK(g->players[0].intro == 3 && g->players[1].intro == 0);
    remove("onboarding-link.sav");
    finish(g);
}
static void instruction(void) {
    Game *g = fresh();
    Player *p = &g->players[0];
    Actions actions[2] = {0};
    for (int i = 0; i < 110; i++)
        onboarding_step(g, actions, .02);
    CHECK(p->lesson == LESSON_BOARD);
    onboarding_dismiss(g, -1);
    onboarding_dismiss(g, 2);
    onboarding_dismiss(g, 1);
    onboarding_dismiss(g, 0);
    CHECK(p->lessons & (1u << LESSON_BOARD));
    CHECK(!g->players[1].lessons && !p->lesson);
    CHECK(game_save(g, "onboarding-hints.sav") && game_load(g, "onboarding-hints.sav"));
    p = &g->players[0];
    CHECK(p->lessons & (1u << LESSON_BOARD));
    p->guide_mode = GUIDE_OFF;
    for (int i = 0; i < 150; i++)
        onboarding_step(g, actions, .02);
    CHECK(!p->lesson && p->weapons == 1);
    p->guide_mode = GUIDE_MINIMAL;
    p->pos = position_from(v3(0, .02, 12));
    for (int i = 0; i < 110; i++)
        onboarding_step(g, actions, .02);
    CHECK(!p->lesson);
    p->guide_mode = GUIDE_GUIDED;
    g->message_time = 0;
    onboarding_step(g, actions, .02);
    CHECK(p->lesson == LESSON_COMMISSION);
    actions[0].fire = true;
    onboarding_step(g, actions, .02);
    CHECK(!p->lesson);
    actions[0].fire = false;
    p->hurt = 1;
    CHECK(onboarding_danger(g, 0));
    p->hurt = 0;
    g->story.done |= 1;
    p->shots_fired = 1;
    p->health = 30;
    for (int i = 0; i < 110; i++)
        onboarding_step(g, actions, .02);
    CHECK(p->lesson == LESSON_RECOVER);
    for (int i = 0; i < 610; i++)
        onboarding_step(g, actions, .02);
    CHECK(p->lessons & (1u << LESSON_RECOVER));
    p->sheltered = true;
    p->power = 1;
    p->intro = 3;
    p->cloaked = true;
    p->riding = MECH;
    g->matrix.divers[0].active = true;
    onboarding_step(g, actions, .02);
    CHECK(p->lessons & (1u << LESSON_LINK));
    CHECK(p->lessons & (1u << LESSON_MATRIX));
    CHECK(p->lessons & (1u << LESSON_SHELTER));
    CHECK(p->lessons & (1u << LESSON_POWER));
    CHECK(p->lessons & (1u << LESSON_RIG));
    char text[180];
    const char *upgrades[] = {"F4 / CHARACTER", "DPAD LEFT / UP / RIGHT", "MENU / CHARACTER",
                              "Y / CHARACTER UPGRADES", "F4 / CHARACTER"};
    for (unsigned device = 0; device <= 4; device++)
        for (unsigned lesson = 0; lesson <= LESSON_COUNT; lesson++) {
            onboarding_text(lesson, device, text, sizeof(text));
            CHECK((lesson == 0 || lesson == LESSON_COUNT) || strlen(text) > 10);
            if (lesson == LESSON_UPGRADE)
                CHECK(strstr(text, upgrades[device]));
        }
    for (unsigned device = 0; device < 4; device++)
        for (unsigned item = 0; item < 4; item++) {
            p->guide_device = device;
            p->selected_power = item;
            p->lesson = LESSON_POWER;
            onboarding_hint_text(g, 0, text, sizeof(text));
            CHECK(strstr(text, sc_psionic_names[item]));
            p->lesson = LESSON_RIG;
            g->campaign.profiles[0].rig = item % 2 + 1;
            onboarding_hint_text(g, 0, text, sizeof(text));
            CHECK(strstr(text, item % 2 ? "WAGON" : "MECH"));
        }
    p->lesson = LESSON_BOARD;
    onboarding_hint_text(g, 0, text, sizeof(text));
    CHECK(strstr(text, "LOADING BAY"));
    CHECK(!strcmp(onboarding_mode(0), "GUIDED") && !strcmp(onboarding_mode(1), "MINIMAL"));
    CHECK(!strcmp(onboarding_mode(2), "OFF") && !strcmp(onboarding_mode(99), "GUIDED"));
    CHECK(onboarding_valid(0, 0, 14, 0) && onboarding_valid(3, 2, 15, 1));
    CHECK(!onboarding_valid(4, 0, 15, 1) && !onboarding_valid(2, 0, 15, 1));
    CHECK(!onboarding_valid(1, 0, 15, 0) && !onboarding_valid(1, 0, 14, 1));
    CHECK(!onboarding_valid(0, 1, 15, 0) && !onboarding_valid(0, 1u << LESSON_COUNT, 15, 0));
    remove("onboarding-hints.sav");
    finish(g);
}
static void generated_opening(void) {
    Game *g = fresh();
    CHECK(onboarding_opening((Cell){0, 0, 0}) && !onboarding_opening((Cell){4, 0, 2}));
    for (unsigned seed = 1; seed <= 8; seed++) {
        game_new_ship(g, seed);
        g->players[0].pos = position_from(world_lift((Cell){0, 0, 1}));
        settle(g);
        for (int n = 0; n < g->world.count; n++) {
            Room *r = &g->world.rooms[n];
            if (!r->ready || r->type == BAY || campaign_guardian(r->cell) >= 0 ||
                !onboarding_opening(r->cell))
                continue;
            CHECK(r->enemy_count <= 3 && !r->habitat.challenge);
            for (int i = 0; i < r->enemy_count; i++)
                CHECK(r->enemies[i].kind == 1 || r->enemies[i].kind == 2);
            if (r->enemy_count) {
                world_kill(&g->world, r, 0);
                onboarding_populate(&g->world, r);
                CHECK(!r->enemies[0].alive);
            }
        }
    }
    CHECK(!hidden_director_propose(g, EDIT_NEST, 0, (Cell){0, 0, 1}, 0));
    CHECK(factions_safe(g, (Cell){0, 0, 1}));
    CHECK(onboarding_reward(g, (CampaignReward){REWARD_POWER, PSI_FLIGHT}, 0).kind == REWARD_GUN);
    CHECK(onboarding_reward(g, (CampaignReward){REWARD_RIG, MECH}, 1).kind == REWARD_GUN);
    CHECK(onboarding_reward(g, (CampaignReward){REWARD_RIG, MECH}, 2).kind == REWARD_RIG);
    finish(g);
}
static void malformed_intro(void) {
    Game *g = fresh();
    CHECK(game_save(g, "onboarding-valid.sav"));
    FILE *f = fopen("onboarding-valid.sav", "rb");
    CHECK(f && !fseek(f, 0, SEEK_END));
    long length = ftell(f);
    CHECK(length > 20);
    rewind(f);
    unsigned char *original = malloc((size_t)length), *data = malloc((size_t)length);
    CHECK(original && data && fread(original, 1, (size_t)length, f) == (size_t)length);
    fclose(f);
    World *before = malloc(sizeof(*before));
    CHECK(before);
    const uint32_t values[] = {4, 2, 1, 1u << LESSON_COUNT};
    const unsigned offsets[] = {0, 0, 4, 12};
    for (unsigned n = 0; n < 4; n++) {
        memcpy(data, original, (size_t)length);
        size_t at = (size_t)length - 20 + offsets[n];
        for (unsigned b = 0; b < 4; b++)
            data[at + b] = (unsigned char)(values[n] >> (8 * b));
        uint32_t hash = 2166136261u;
        for (long i = 0; i < length - 4; i++)
            hash = (hash ^ data[i]) * 16777619u;
        for (unsigned b = 0; b < 4; b++)
            data[length - 4 + b] = (unsigned char)(hash >> (8 * b));
        f = fopen("onboarding-invalid.sav", "wb");
        CHECK(f && fwrite(data, 1, (size_t)length, f) == (size_t)length);
        fclose(f);
        *before = g->world;
        Player player = g->players[0];
        CHECK(!game_load(g, "onboarding-invalid.sav"));
        CHECK(!memcmp(before, &g->world, sizeof(*before)) &&
              !memcmp(&player, &g->players[0], sizeof(player)));
    }
    CHECK(game_load(g, "onboarding-valid.sav"));
    free(before);
    free(data);
    free(original);
    remove("onboarding-valid.sav");
    remove("onboarding-invalid.sav");
    finish(g);
}
int main(void) {
    starters();
    matrix_introduction();
    instruction();
    generated_opening();
    malformed_intro();
    puts("PASS staged equipment, safe matrix introduction, optional lessons, co-op receipts, saves "
         "and opening generation");
    return 0;
}
