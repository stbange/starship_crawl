#include "game.h"
#include <string.h>
float game_rig_max_armor(const Player *p, RigKind kind) {
    return kind == MECH ? 600 + 60.f * p->mods[0] : 150 + 15.f * p->mods[0];
}
double game_body_radius(const Player *p) {
    return p->riding == MECH ? .8 : p->riding == WAGON ? .65 : .34;
}
double game_body_height(const Player *p) {
    return p->riding == MECH ? 2.8 : p->riding == WAGON ? 1.6 : p->crouched ? 1 : 1.78;
}
const WeaponDef *game_gun(const Player *p) {
    return p->riding == MECH ? &sc_mech_weapons[p->rig_weapon] : &sc_weapons[p->weapon];
}
float game_shot_speed(const Player *p) {
    const float speed[] = {90, 35, 0};
    return p->riding == MECH ? speed[p->rig_weapon] : sc_projectile_speeds[p->weapon];
}
void game_equipment_reset(Game *g) {
    memset(g->beams, 0, sizeof(g->beams));
    memset(g->flares, 0, sizeof(g->flares));
    for (int p = 0; p < SC_PLAYERS; p++) {
        g->players[p].riding = 0;
        g->players[p].drive_speed = 0;
        for (int k = 0; k < 2; k++)
            g->rigs[p][k] = (Rig){g->players[p].pos,
                                  game_rig_max_armor(&g->players[p], (RigKind)(k + 1)), false, 0};
    }
}
bool game_rig_blocks(const Game *g, int player, Vec3 feet, double radius, double height) {
    for (int p = 0; p < SC_PLAYERS; p++)
        for (int k = 0; k < 2; k++) {
            const Rig *r = &g->rigs[p][k];
            if (!r->deployed || (p == player && g->players[player].riding == (unsigned)k + 1))
                continue;
            Vec3 at = position_absolute(r->pos);
            double h = k == 0 ? 2.8 : 1.6, rr = radius + (k == 0 ? .8 : .65);
            double dx = feet.x - at.x, dz = feet.z - at.z;
            if (feet.y + height > at.y && feet.y < at.y + h && dx * dx + dz * dz < rr * rr)
                return true;
        }
    return false;
}
void game_rig_toggle(Game *g, int player, RigKind kind) {
    if (player < 0 || player >= g->player_count || kind < MECH || kind > WAGON)
        return;
    if (g->players[player].lift_active)
        return;
    if (!campaign_rig(g, player, (unsigned)kind)) {
        game_message(g, "RIG LOCKED OR STORED / DEFEAT GUARDIANS / F10 LOADOUT");
        return;
    }
    Player *p = &g->players[player];
    Rig *r = &g->rigs[player][kind - 1];
    if (p->sheltered || p->riding) {
        game_message(g, "EXIT YOUR SHELTER OR RIG BEFORE FOLDING EQUIPMENT");
        return;
    }
    if (r->deployed) {
        r->deployed = false;
        g->dirty = true;
        game_message(g, kind == MECH ? "MECH PACKED / T TO DEPLOY" : "WAGON PACKED / Y TO DEPLOY");
        return;
    }
    Vec3 feet = position_absolute(p->pos), ahead = v3(sin(p->yaw), 0, cos(p->yaw));
    Vec3 at = vadd(feet, vmul(ahead, kind == MECH ? 3 : 2.5));
    double radius = kind == MECH ? .8 : .65, height = kind == MECH ? 2.8 : 1.6;
    if (feet.z < 0 || !world_ready(&g->world, world_cell(at)) ||
        world_overlap(&g->world, at, radius + .15, height) ||
        !world_overlap(&g->world, vadd(at, v3(0, -.15, 0)), radius, .1) ||
        game_rig_blocks(g, player, at, radius + .1, height) ||
        world_raycast(&g->world, player_eye(p), vnorm(vsub(vadd(at, v3(0, 1, 0)), player_eye(p))),
                      vlen(vsub(at, feet))) < vlen(vsub(at, feet)) - .2) {
        game_message(g, "RIG NEEDS CLEAR SUPPORTED FLOOR / FIND A WIDER BAY");
        return;
    }
    r->pos = position_from(at);
    r->yaw = p->yaw;
    r->deployed = true;
    g->dirty = true;
    game_message(g, kind == MECH ? "MECH UNFOLDED / G TO BOARD" : "WAGON UNFOLDED / G TO RIDE");
}
RigKind game_rig_near(const Game *g, int player) {
    if (player < 0 || player >= g->player_count || g->players[player].sheltered)
        return ON_FOOT;
    const Player *p = &g->players[player];
    for (int k = 0; k < 2; k++) {
        const Rig *r = &g->rigs[player][k];
        if (!r->deployed)
            continue;
        Vec3 point = vadd(position_absolute(r->pos), v3(0, 1, 0)),
             diff = vsub(point, player_eye(p));
        if (vlen(position_delta(r->pos, p->pos)) < 3.6 &&
            world_raycast(&g->world, player_eye(p), vnorm(diff), vlen(diff)) >= vlen(diff) - .1)
            return (RigKind)(k + 1);
    }
    return ON_FOOT;
}
bool game_rig_interact(Game *g, int player) {
    if (player < 0 || player >= g->player_count || g->players[player].lift_active)
        return false;
    Player *p = &g->players[player];
    if (p->riding) {
        Vec3 feet = position_absolute(p->pos);
        const Vec3 directions[] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, 0, 1}};
        for (int i = 0; i < 4; i++) {
            Vec3 at = vadd(feet, vmul(directions[i], 1.8));
            if (at.z < 0 || !world_ready(&g->world, world_cell(at)) ||
                world_overlap(&g->world, at, .34, 1.78) ||
                world_raycast(&g->world, player_eye(p),
                              vnorm(vsub(vadd(at, v3(0, 1.62, 0)), player_eye(p))),
                              vlen(vsub(vadd(at, v3(0, 1.62, 0)), player_eye(p)))) <
                    vlen(vsub(vadd(at, v3(0, 1.62, 0)), player_eye(p))) - .1 ||
                game_rig_blocks(g, player, at, .34, 1.78) ||
                !world_overlap(&g->world, vadd(at, v3(0, -.15, 0)), .34, .1))
                continue;
            p->pos = position_from(at);
            p->riding = 0;
            p->drive_speed = 0;
            p->velocity = v3(0, 0, 0);
            g->dirty = true;
            game_message(g, "DISMOUNTED / G TO BOARD AGAIN");
            return true;
        }
        game_message(g, "NO SAFE EXIT / MOVE TO CLEAR FLOOR");
        return true;
    }
    RigKind kind = game_rig_near(g, player);
    if (kind == ON_FOOT)
        return false;
    if (!campaign_rig(g, player, kind)) {
        game_message(g, "EQUIP THIS RIG AT A SAFE REFIT LOCATION FIRST");
        return true;
    }
    Rig *r = &g->rigs[player][kind - 1];
    if (r->armor <= 0) {
        game_message(g, "RIG DEPLETED / REPAIR AT YOUR SHELTER MED STATION");
        return true;
    }
    double height = kind == MECH ? 2.8 : 1.6, radius = kind == MECH ? .8 : .65;
    if (!world_ready(&g->world, world_cell(position_absolute(r->pos))) ||
        world_overlap(&g->world, position_absolute(r->pos), radius, height)) {
        game_message(g, "RIG NOT READY / WAIT FOR A CLEAR SURVEY");
        return true;
    }
    p->pos = r->pos;
    p->riding = kind;
    p->velocity = v3(0, 0, 0);
    p->crouched = false;
    p->cloaked = false;
    p->flight = p->leap = p->nova = p->nova_tick = p->nova_flash = 0;
    p->cooldown = .2f;
    g->dirty = true;
    game_message(g, kind == MECH ? "MECH ONLINE / Z-X BUILT-IN ARSENAL / G EXIT"
                                 : "WAGON ONLINE / WASD DRIVE / CTRL BRAKE / G EXIT");
    return true;
}
void game_rig_repair(Game *g, int player) {
    for (int k = 0; k < 2; k++)
        if (g->players[player].riding != (unsigned)k + 1)
            g->rigs[player][k].armor = game_rig_max_armor(&g->players[player], (RigKind)(k + 1));
}
void game_rig_damage(Game *g, int player, float *damage) {
    Player *p = &g->players[player];
    if (!p->riding)
        return;
    Rig *r = &g->rigs[player][p->riding - 1];
    float take = fminf(*damage, r->armor);
    r->armor -= take;
    *damage -= take;
    if (r->armor <= 0) {
        if (!game_rig_interact(g, player) || p->riding) {
            p->riding = 0;
            p->drive_speed = 0;
            p->velocity = v3(0, 0, 0);
        }
        game_message(g, "RIG PLATING LOST / PILOT EJECTED / MED STATION REPAIRS IT");
    }
}
bool game_flare_upgrade(Game *g, int player, bool strength) {
    if (player < 0 || player >= g->player_count)
        return false;
    Player *p = &g->players[player];
    if (g->campaign.version && !(p->weapons & (1u << 19))) {
        game_message(g, "EARN A FLARE CANNON FROM A GUARDIAN BEFORE UPGRADING IT");
        return false;
    }
    uint32_t *level = strength ? &p->flare_strength : &p->flare_lifetime;
    unsigned cost = 80 + 50 * (*level);
    if (*level >= 5) {
        game_message(g, "FLARE UPGRADE AT MAXIMUM");
        return false;
    }
    if (!p->points && p->credits < cost) {
        game_message(g, "FLARE UPGRADE NEEDS 1 POINT OR SALVAGE / O LIFETIME / P STRENGTH");
        return false;
    }
    if (p->points)
        p->points--;
    else
        p->credits -= cost;
    (*level)++;
    g->dirty = true;
    game_message(g,
                 strength ? "FLARE STRENGTH AND RADIUS IMPROVED" : "FLARE BURN LIFETIME IMPROVED");
    return true;
}
void game_attach_flare(Game *g, int owner, Vec3 point, Vec3 direction) {
    if (owner < 0 || owner >= g->player_count || !world_ready(&g->world, world_cell(point)))
        return;
    Vec3 origin = vsub(point, vmul(direction, .04)), normal = vmul(direction, -1);
    double best = .15;
    const Vec3 axes[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (int i = 0; i < 6; i++) {
        double hit = world_raycast(&g->world, origin, axes[i], best);
        if (hit < best) {
            best = hit;
            normal = vmul(axes[i], -1);
        }
    }
    if (best == .15)
        return;
    int slot = 0;
    for (int i = 0; i < SC_MAX_FLARES; i++) {
        if (!g->flares[i].active) {
            slot = i;
            break;
        }
        if (g->flares[i].life < g->flares[slot].life)
            slot = i;
    }
    const Player *p = &g->players[owner];
    g->flares[slot] = (Flare){.pos = position_from(vadd(origin, vmul(normal, .06))),
                              .normal = normal,
                              .life = 30 + 15.f * p->flare_lifetime,
                              .radius = 12 + 2.f * p->flare_strength,
                              .power = 2.5f + .75f * p->flare_strength,
                              .owner = (uint32_t)owner,
                              .active = true};
    g->dirty = true;
}
void game_equipment_step(Game *g, double dt) {
    for (int p = 0; p < SC_PLAYERS; p++)
        g->beams[p].life = fmaxf(0, g->beams[p].life - (float)dt);
    for (int i = 0; i < SC_MAX_FLARES; i++)
        if (g->flares[i].active) {
            g->flares[i].life = fmaxf(0, g->flares[i].life - (float)dt);
            if (g->flares[i].life <= 0)
                g->flares[i].active = false;
        }
}
