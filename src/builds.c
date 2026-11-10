#include "game.h"

const char *campaign_build_name(const Game *g, unsigned branch) {
    static const char *names[] = {"NONE",
                                  "CONDUIT / FLARE RELAY",
                                  "CONDUIT / FOCUS",
                                  "VEIL / ECHO",
                                  "VEIL / AMBUSH",
                                  "BASTION / CAPACITOR",
                                  "BASTION / BRACE",
                                  "RIFT / LANDING",
                                  "RIFT / CLOSE FIELD"};
    return g->campaign.version < 2 ? campaign_module_name(branch)
                                   : names[branch <= SC_BUILD_BRANCHES ? branch : 0];
}
const char *campaign_build_hint(const Game *g, unsigned branch) {
    static const char *legacy[] = {"EARN SPECIALIZATIONS FROM GUARDIANS",
                                   "+60% DAMAGE WHILE FLYING", "+50% MECH DAMAGE",
                                   "+150% CLOAK BREAK DAMAGE", "+60% DAMAGE IN SILENCED ROOMS"};
    static const char *hints[] = {
        "ONE BRANCH ACTIVE / REFIT AT A SAFE STATION",
        "REQUIRES EQUIPPED BEAM + FLARE CANNON / HIT FLARE / 3 RELAYS / 3 ENERGY EACH",
        "REQUIRES BEAM / SUSTAIN CONTACT TO CHARGE A PIERCING PULSE",
        "REQUIRES VEIL / FIRING LEAVES A 3 SECOND LURE / NO BONUS DAMAGE",
        "REQUIRES VEIL / CLOAK SHOT MARKS A FOE / ITS DEATH BURSTS",
        "REQUIRES MECH / HOSTILE HITS CHARGE NEXT CANNON / PACKING CLEARS CHARGE",
        "REQUIRES MECH / STATIONARY BRACE PROTECTS FRONT / COUNTERWAVE",
        "REQUIRES LONG JUMP / LANDING RELEASES A LOCAL PULSE",
        "REQUIRES FLIGHT / CLOSE DAMAGE FIELD / EXTRA ENERGY DRAIN"};
    return g->campaign.version < 2 ? legacy[branch <= 4 ? branch : 0]
                                   : hints[branch <= SC_BUILD_BRANCHES ? branch : 0];
}
bool game_build_active(const Game *g, int player, BuildBranch branch) {
    if (g->campaign.version != 2 || player < 0 || player >= g->player_count ||
        branch <= BUILD_NONE || branch > BUILD_FIELD)
        return false;
    const CampaignProfile *p = &g->campaign.profiles[player];
    if (p->module != (unsigned)branch || !(p->modules & (1u << (branch - 1))))
        return false;
    if (branch <= BUILD_FOCUS) {
        bool beam = false, flare = false;
        for (unsigned i = 0; i < 2; i++)
            if (p->guns[i] < SC_WEAPONS && (g->players[player].weapons & (1u << p->guns[i]))) {
                beam |= (sc_weapons[p->guns[i]].flags & W_BEAM) != 0;
                flare |= p->guns[i] == SC_FLARE_CANNON;
            }
        return beam && (branch == BUILD_FOCUS || flare);
    }
    if (branch <= BUILD_AMBUSH)
        return campaign_power(g, player, PSI_VEIL);
    if (branch <= BUILD_BRACE)
        return campaign_rig(g, player, MECH);
    return campaign_power(g, player, branch == BUILD_LANDING ? PSI_LEAP : PSI_FLIGHT);
}
bool game_build_equip(Game *g, int player, unsigned branch) {
    if (g->campaign.version != 2 || branch > SC_BUILD_BRANCHES || !campaign_safe_refit(g, player))
        return false;
    CampaignProfile *p = &g->campaign.profiles[player];
    if (branch && !(p->modules & (1u << (branch - 1))))
        return false;
    if (p->module != branch) {
        p->module = branch;
        g->builds[player] = (BuildEffects){0};
        g->dirty = true;
    }
    g->views[player].branch_preview = 0;
    game_player_message(g, game_player_id(g, player), campaign_build_name(g, branch));
    return true;
}
void game_build_shot(Game *g, int player, WeaponDef *shot) {
    Player *p = &g->players[player];
    BuildEffects *b = &g->builds[player];
    if (p->cloaked && game_build_active(g, player, BUILD_ECHO)) {
        b->echo = p->pos;
        b->echo_life = 3;
    }
    if (p->riding == MECH && p->rig_weapon == 0 && game_build_active(g, player, BUILD_CAPACITOR)) {
        shot->damage += b->charge * 1.5f;
        b->charge = 0;
    }
}
void game_build_relay(Game *g, int player, Vec3 from, Vec3 to, float damage) {
    if (!game_build_active(g, player, BUILD_RELAY))
        return;
    Vec3 delta = vsub(to, from), dir = vnorm(delta);
    double length = vlen(delta), closest = length + 1;
    const Flare *source = NULL;
    for (unsigned i = 0; i < SC_MAX_FLARES; i++) {
        const Flare *f = &g->flares[i];
        if (!f->active || f->owner != (unsigned)player)
            continue;
        Vec3 offset = vsub(position_absolute(f->pos), from);
        double along = vdot(offset, dir);
        if (along >= 0 && along <= length + .2 && along < closest &&
            vlen(vsub(offset, vmul(dir, along))) < .65) {
            closest = along;
            source = f;
        }
    }
    if (!source)
        return;
    Vec3 origin = vadd(position_absolute(source->pos), vmul(source->normal, .15));
    Enemy *used[3] = {0};
    for (unsigned relay = 0; relay < 3 && g->players[player].energy >= 3; relay++) {
        Room *room = NULL;
        int target = -1;
        double nearest = 18;
        for (int r = 0; r < g->world.count; r++)
            for (int i = 0; i < g->world.rooms[r].enemy_count; i++) {
                Enemy *e = &g->world.rooms[r].enemies[i];
                if (!e->alive || e->allegiance || e == used[0] || e == used[1] || e == used[2] ||
                    encounter_passive(&g->world.rooms[r], i) || !politics_threat(g, e))
                    continue;
                Vec3 to_enemy = vsub(enemy_center(e), origin);
                double distance = vlen(to_enemy);
                if (distance < nearest &&
                    world_raycast(&g->world, origin, vnorm(to_enemy), distance) >= distance - .05) {
                    nearest = distance;
                    target = i;
                    room = &g->world.rooms[r];
                }
            }
        if (!room)
            break;
        used[relay] = &room->enemies[target];
        g->builds[player].relays[relay] =
            (Beam){position_from(origin), position_from(enemy_center(used[relay])),
                   rgb(.35f, .85f, 1), .18f};
        g->players[player].energy -= 3;
        game_damage_enemy(g, room, target, player, damage * .45f, W_BEAM);
    }
}
bool game_build_echo(Game *g, Enemy *enemy, double dt) {
    if (g->campaign.version != 2 || enemy->allegiance || enemy->force)
        return false;
    for (int i = 0; i < g->player_count; i++) {
        const BuildEffects *b = &g->builds[i];
        if (b->echo_life <= 0 || !game_build_active(g, i, BUILD_ECHO) || g->players[i].sheltered ||
            !politics_threat(g, enemy))
            continue;
        Vec3 to = vsub(vadd(position_absolute(b->echo), v3(0, .9, 0)), enemy_center(enemy));
        double length = vlen(to);
        if (length > 18 ||
            world_raycast(&g->world, enemy_center(enemy), vnorm(to), length) < length - .1)
            continue;
        Vec3 direction = vnorm(v3(to.x, 0, to.z));
        enemy->heading = direction;
        enemy->aim = vadd(position_absolute(b->echo), v3(0, .9, 0));
        enemy->windup = 0;
        if (length > 1.5 && !(sc_enemies[enemy->kind].flags & TURRET))
            world_move(&g->world, &enemy->pos, vmul(direction, sc_enemies[enemy->kind].speed * dt),
                       .42, 1.25, true, NULL);
        enemy->cooldown = fmaxf(enemy->cooldown, .3f);
        return true;
    }
    return false;
}
void game_builds_step(Game *g, double dt) {
    bool outside = false;
    for (int i = 0; i < g->player_count; i++)
        outside |= !g->players[i].sheltered;
    for (int i = 0; i < SC_PLAYERS; i++) {
        BuildEffects *b = &g->builds[i];
        if (outside)
            b->echo_life = fmaxf(0, b->echo_life - (float)dt);
        if (g->players[i].riding != MECH || !game_build_active(g, i, BUILD_CAPACITOR))
            b->charge = 0;
        for (unsigned j = 0; j < 3; j++)
            b->relays[j].life = fmaxf(0, b->relays[j].life - (float)dt);
    }
}
