#include "game.h"
#include <stdio.h>
#include <stdlib.h>

bool onboarding_active(const Game *g) {
    return g->world.generator >= 15 && g->campaign.version;
}
unsigned onboarding_capabilities(const Game *g, int player) {
    if (player < 0 || player >= SC_PLAYERS)
        return 0;
    const Player *p = &g->players[player];
    const CampaignProfile *c = &g->campaign.profiles[player];
    unsigned caps = 0;
    if (!onboarding_active(g))
        caps = OB_SHELTER | OB_MATRIX | OB_UPGRADES | OB_BUILD | OB_FULL_STATS | OB_PROGRAMS;
    else {
        if (g->campaign.defeated)
            caps |= OB_SHELTER;
        if (p->intro & INTRO_INTERFACE)
            caps |= OB_MATRIX;
        if (p->intro & INTRO_GRID)
            caps |= OB_PROGRAMS;
        if (p->kills >= 3 || g->campaign.defeated)
            caps |= OB_UPGRADES | OB_BUILD;
        if (campaign_phase(&g->campaign))
            caps |= OB_FULL_STATS;
    }
    if (p->weapons & (p->weapons - 1))
        caps |= OB_GUNS;
    if (!g->campaign.version || c->powers)
        caps |= OB_POWERS;
    if (!g->campaign.version || c->rigs)
        caps |= OB_RIGS;
    return caps;
}
bool onboarding_danger(const Game *g, int player) {
    const Player *p = &g->players[player];
    if (p->sheltered || g->matrix.divers[player].active)
        return false;
    if (p->hurt > 0 || p->muzzle > 0 || campaign_engaged(g, player))
        return true;
    for (int r = 0; r < g->world.count; r++)
        for (int i = 0; i < g->world.rooms[r].enemy_count; i++) {
            const Enemy *e = &g->world.rooms[r].enemies[i];
            int guardian = campaign_guardian(g->world.rooms[r].cell);
            if (e->alive && !encounter_passive(&g->world.rooms[r], i) &&
                (guardian < 0 || g->campaign.attempts[guardian].active) && politics_threat(g, e) &&
                vlen(vsub(enemy_center(e), player_eye(p))) < 28) {
                Vec3 delta = vsub(enemy_center(e), player_eye(p));
                if (world_raycast(&g->world, player_eye(p), vnorm(delta), vlen(delta)) >=
                    vlen(delta) - .1)
                    return true;
            }
        }
    return false;
}
bool onboarding_action(const Game *g, int player, int action) {
    if (player < 0 || player >= SC_PLAYERS)
        return false;
    if (!onboarding_active(g))
        return true;
    const Player *p = &g->players[player];
    unsigned caps = onboarding_capabilities(g, player);
    const CampaignProfile *c = &g->campaign.profiles[player];
    if (action == 13)
        return (caps & OB_SHELTER) != 0;
    if (action == 15 || action == 16)
        return (c->rigs & (1u << (action - 15))) != 0;
    if (action >= 17 && action <= 19)
        return (p->weapons & (1u << SC_FLARE_CANNON)) != 0;
    if (action >= 60 && action <= 63)
        return (c->powers & (1u << (action - 60))) != 0;
    if (action >= 70 && action <= 72)
        return (c->rigs & 1) != 0;
    if (action >= 40 && action < 40 + SC_WEAPONS)
        return (p->weapons & (1u << (action - 40))) != 0;
    if (action == 85)
        return p->logs || (g->story.seen & ~1u);
    if (action == 86 || action == 88)
        return p->victories || g->story.chapter;
    if (action >= 30 && action <= 35)
        return p->map_open;
    if (action == 9 || action == 10)
        return (caps & OB_GUNS) != 0;
    if (action == 84 || (action >= 1 && action <= 3))
        return (caps & OB_UPGRADES) != 0;
    if (action >= 20 && action < 30)
        return (caps & OB_UPGRADES) && (action < 23 || (caps & OB_FULL_STATS));
    if (action >= 220 && action <= 224)
        return (caps & OB_BUILD) != 0;
    if (action >= 226 && action <= 228)
        return g->campaign.choice[player] != 0;
    if (action == 200)
        return (caps & OB_MATRIX) != 0;
    if (action == 206)
        return (caps & OB_PROGRAMS) != 0;
    if (action == 201)
        return g->matrix.divers[player].active;
    if (action >= 202 && action <= 215)
        return (caps & OB_PROGRAMS) != 0;
    return true;
}
bool onboarding_opening(Cell c) {
    return c.x >= -1 && c.x <= 1 && c.y >= -1 && c.y <= 1 && c.z >= 0 && c.z <= 1;
}
void onboarding_plan(RegionPlan *plan) {
    for (int i = 0; i < plan->count; i++) {
        RoomSpec *r = &plan->rooms[i];
        if (r->type != BAY && campaign_guardian(r->cell) < 0 && onboarding_opening(r->cell)) {
            r->habitat.pace = PACE_SWARM;
            r->habitat.challenge = r->habitat.contract = 0;
        }
    }
}
void onboarding_populate(World *w, Room *r) {
    if (!onboarding_opening(r->cell) || r->type == BAY || campaign_guardian(r->cell) >= 0)
        return;
    r->enemy_count = (int)fmin(r->enemy_count, 3);
    for (int i = 0; i < r->enemy_count; i++) {
        Enemy *e = &r->enemies[i];
        e->kind = r->height < 3 || (r->cell.y && i == 2) ? 1 : 2;
        e->health = 55;
        e->behavior = e->motion = e->country = e->allegiance = 0;
        e->force = 0;
        e->cooldown = 3;
        e->alive = r->record == SIZE_MAX || !(w->records[r->record].killed & (1u << i));
    }
}
CampaignReward onboarding_reward(const Game *g, CampaignReward r, unsigned tier) {
    if (onboarding_active(g) &&
        ((r.kind == REWARD_POWER && tier < 1) || (r.kind >= REWARD_RIG && tier < 2)))
        return (CampaignReward){REWARD_GUN, 2 + (r.kind * 3 + r.item) % 10};
    return r;
}
bool onboarding_interface_site(const Game *g, int player) {
    if (player < 0 || player >= g->player_count || !onboarding_active(g) ||
        (g->players[player].intro & INTRO_INTERFACE) || !campaign_phase(&g->campaign) ||
        g->players[player].sheltered || g->players[player].riding ||
        g->players[player].lift_active || g->players[player].health <= 0 || !matrix_near(g, player))
        return false;
    Cell cell = world_cell(position_absolute(g->players[player].pos));
    const Room *room = world_find((World *)&g->world, cell);
    if (!room || !room->ready || campaign_layer(cell) < 2 || campaign_guardian(room->cell) >= 0)
        return false;
    return true;
}
bool onboarding_interface_ready(const Game *g, int player) {
    return onboarding_interface_site(g, player) && !onboarding_danger(g, player);
}
bool onboarding_interact(Game *g, int player) {
    if (!onboarding_interface_site(g, player))
        return false;
    if (onboarding_danger(g, player)) {
        game_message(g, "MATRIX INTERFACE HERE / CLEAR NEARBY THREATS BEFORE RECOVERING IT");
        return true;
    }
    g->players[player].intro |= INTRO_INTERFACE;
    g->dirty = true;
    game_message(g, "MATRIX INTERFACE RECOVERED / USE THIS TERMINAL AGAIN TO RESTORE ITS LIGHTS");
    return true;
}
bool onboarding_matrix_intro(const Game *g, int player) {
    return onboarding_active(g) && player >= 0 && player < SC_PLAYERS &&
           (g->players[player].intro & INTRO_INTERFACE) && !(g->players[player].intro & INTRO_GRID);
}
static Lesson next_lesson(const Game *g, int player) {
    const Player *p = &g->players[player];
    if (g->matrix.divers[player].active)
        return p->lessons & (1u << LESSON_MATRIX) ? LESSON_NONE : LESSON_MATRIX;
    unsigned caps = onboarding_capabilities(g, player);
    bool eligible[LESSON_COUNT] = {false};
    eligible[LESSON_BOARD] = position_absolute(p->pos).z < 0;
    eligible[LESSON_COMMISSION] = !eligible[LESSON_BOARD] && !(g->story.done & 1);
    eligible[LESSON_EXPLORE] = (g->story.done & 1) && p->kills < 3;
    eligible[LESSON_RECOVER] = p->health < player_max_health(p) * .65f;
    eligible[LESSON_WEAPON] = caps & OB_GUNS;
    eligible[LESSON_UPGRADE] = (caps & OB_UPGRADES) && p->points;
    eligible[LESSON_SHELTER] = caps & OB_SHELTER;
    eligible[LESSON_INTERFACE] = campaign_phase(&g->campaign) && !(caps & OB_MATRIX);
    eligible[LESSON_LINK] = caps & OB_MATRIX;
    eligible[LESSON_POWER] = caps & OB_POWERS;
    eligible[LESSON_RIG] = caps & OB_RIGS;
    for (unsigned i = 1; i < LESSON_COUNT; i++)
        if (eligible[i] && !(p->lessons & (1u << i)) &&
            (p->guide_mode != GUIDE_MINIMAL || i >= LESSON_WEAPON))
            return (Lesson)i;
    return LESSON_NONE;
}
void onboarding_dismiss(Game *g, int player) {
    if (player < 0 || player >= g->player_count || !g->players[player].lesson)
        return;
    Player *p = &g->players[player];
    p->lessons |= 1u << p->lesson;
    p->lesson = 0;
    p->lesson_time = p->guide_quiet = 0;
    g->dirty = true;
}
void onboarding_step(Game *g, const Actions *actions, double dt) {
    if (!onboarding_active(g))
        return;
    for (int i = 0; i < g->player_count; i++) {
        Player *p = &g->players[i];
        uint32_t before = p->lessons;
        if (p->kills >= 3 && !(p->weapons & 2) && !onboarding_danger(g, i)) {
            campaign_grant(g, i, (CampaignReward){REWARD_GUN, 1});
            game_message(g, "BREACH SHOTGUN EARNED / FIRST SKIRMISH SURVIVED");
        }
        if (position_absolute(p->pos).z >= 0)
            p->lessons |= 1u << LESSON_BOARD;
        if (g->story.done & 1)
            p->lessons |= 1u << LESSON_COMMISSION;
        if (p->shots_fired)
            p->lessons |= 1u << LESSON_EXPLORE;
        if (p->sheltered)
            p->lessons |= 1u << LESSON_SHELTER;
        if (p->power || p->vitality || p->haste)
            p->lessons |= 1u << LESSON_UPGRADE;
        if (p->intro & INTRO_INTERFACE)
            p->lessons |= 1u << LESSON_INTERFACE;
        if (g->matrix.divers[i].active)
            p->lessons |= 1u << LESSON_LINK;
        if (p->intro & INTRO_GRID)
            p->lessons |= 1u << LESSON_MATRIX;
        if (p->cloaked || p->flight || p->leap || p->nova)
            p->lessons |= 1u << LESSON_POWER;
        if (p->riding)
            p->lessons |= 1u << LESSON_RIG;
        g->dirty |= before != p->lessons;
        if (p->guide_mode == GUIDE_OFF || actions[i].fire || onboarding_danger(g, i)) {
            p->guide_quiet = 0;
            p->lesson = 0;
            continue;
        }
        p->guide_quiet += (float)dt;
        Lesson next = next_lesson(g, i);
        if (p->lesson != (unsigned)next)
            p->lesson_time = 0;
        p->lesson = p->guide_quiet >= 2 ? (unsigned)next : 0;
        if (p->lesson && g->message_time <= 0 && (p->lesson_time += (float)dt) >= 12)
            onboarding_dismiss(g, i);
    }
}
const char *onboarding_mode(unsigned mode) {
    static const char *names[] = {"GUIDED", "MINIMAL", "OFF"};
    return names[mode <= GUIDE_OFF ? mode : GUIDE_GUIDED];
}
void onboarding_text(unsigned lesson, unsigned device, char *text, size_t size) {
    static const char *move[] = {"WASD", "LEFT STICK", "MOVE PAD", "LEFT STICK"};
    static const char *use[] = {"G", "DPAD DOWN", "USE", "RIGHT GRIP"};
    static const char *fire[] = {"CLICK", "RIGHT TRIGGER", "FIRE", "RIGHT TRIGGER"};
    static const char *gun[] = {"Z / X", "RIGHT SHOULDER", "WEAPON", "X"};
    static const char *upgrade[] = {"F4 / CHARACTER", "DPAD LEFT / UP / RIGHT", "MENU / CHARACTER",
                                    "Y / CHARACTER UPGRADES"};
    static const char *fold[] = {"R", "BACK / SELECT", "MENU / SHELTER", "MENU / EQUIPMENT"};
    static const char *grid[] = {"F", "X", "MENU / RUN SERVICE", "RIGHT GRIP"};
    if (device > GUIDE_QUEST)
        device = GUIDE_KEYBOARD;
    const char *format = "", *key = "";
    switch (lesson) {
    case LESSON_BOARD:
        format = "%s / APPROACH THE LIT LOADING BAY";
        key = move[device];
        break;
    case LESSON_COMMISSION:
        format = "%s / ACCEPT YOUR MISSION AT THE BAY CONSOLE";
        key = use[device];
        break;
    case LESSON_EXPLORE:
        format = "%s / FIRE AT ATTACKERS / EXPLORE A PASSAGE FROM THE BAY";
        key = fire[device];
        break;
    case LESSON_RECOVER:
        format = "RETURN TO THE BAY TO RECOVER / YOUR MAP MARKS THE ROUTE%s";
        break;
    case LESSON_WEAPON:
        format = "%s / SWITCH YOUR EARNED WEAPONS";
        key = gun[device];
        break;
    case LESSON_UPGRADE:
        format = "%s / CHOOSE DAMAGE, VITALITY OR HASTE WHEN READY";
        key = upgrade[device];
        break;
    case LESSON_SHELTER:
        format = "%s / UNFOLD SHELTER ON CLEAR FLOOR / REST OR USE MEDICAL";
        key = fold[device];
        break;
    case LESSON_INTERFACE:
        format = "%s / RECOVER AN INTERFACE AT A CLEARED TERMINAL IN LAYER 2";
        key = use[device];
        break;
    case LESSON_LINK:
        format = "%s / LINK AT A TERMINAL WHEN READY / FIRST TASK: RESTORE POWER";
        key = use[device];
        break;
    case LESSON_MATRIX:
        format = "%s / FIRST LINK: RESTORE POWER / RETURN TO SEE THE LIGHTS";
        key = grid[device];
        break;
    case LESSON_POWER:
        format = "ABILITIES / TRY YOUR EARNED POWER / RECHARGE BETWEEN USES%s";
        break;
    case LESSON_RIG:
        format = "EQUIPMENT / DEPLOY YOUR EARNED RIG / USE TO BOARD%s";
        break;
    default:
        break;
    }
    snprintf(text, size, format, key);
}
void onboarding_hint_text(const Game *g, int player, char *text, size_t size) {
    const Player *p = &g->players[player];
    onboarding_text(p->lesson, p->guide_device, text, size);
    if (p->lesson == LESSON_POWER) {
        unsigned power = p->selected_power % SC_PSIONICS;
        const char *keys[] = {"V", "B", "J", "H"};
        const char *pad[] = {"LEFT TRIGGER + DPAD LEFT", "LEFT TRIGGER + DPAD UP",
                             "LEFT TRIGGER + DPAD RIGHT", "LEFT TRIGGER + DPAD DOWN"};
        const char *key = p->guide_device == GUIDE_KEYBOARD ? keys[power]
                          : p->guide_device == GUIDE_PAD    ? pad[power]
                          : p->guide_device == GUIDE_TOUCH  ? "CAST"
                                                            : "Y / ABILITIES AND EQUIPMENT";
        snprintf(text, size, "%s / %s / ENERGY RECHARGES BETWEEN USES", key,
                 sc_psionic_names[power]);
    } else if (p->lesson == LESSON_RIG) {
        unsigned rig = g->campaign.profiles[player].rig;
        const char *key = p->guide_device == GUIDE_KEYBOARD ? (rig == MECH ? "T" : "Y")
                          : p->guide_device == GUIDE_PAD
                              ? (rig == MECH ? "LEFT TRIGGER + A" : "LEFT TRIGGER + B")
                              : "MENU / EQUIPMENT";
        snprintf(text, size, "%s / DEPLOY %s / USE TO BOARD", key, rig == MECH ? "MECH" : "WAGON");
    }
}
bool onboarding_valid(uint32_t intro, uint32_t lessons, unsigned generator, unsigned defeated) {
    return !(intro & ~3u) && !(lessons & ~((1u << LESSON_COUNT) - 2)) && (intro != INTRO_GRID) &&
           (!intro || (generator >= 15 && (defeated & 3)));
}
