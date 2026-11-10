#include "game.h"
static bool eligible(const Game *g, const Room *r, int i) {
    const Enemy *e = &r->enemies[i];
    return e->alive && !e->allegiance && !encounter_passive(r, i) && politics_threat(g, e);
}
void game_build_wave(Game *g, int owner, Vec3 origin, float radius, float damage, bool frontal) {
    if (owner < 0 || owner >= g->player_count || g->players[owner].sheltered ||
        g->matrix.divers[owner].active)
        return;
    Player *p = &g->players[owner];
    unsigned struck = 0;
    Vec3 forward = v3(sin(p->yaw), 0, cos(p->yaw));
    g->builds[owner].pulse = position_from(origin);
    g->builds[owner].pulse_life = .4f;
    g->builds[owner].pulse_radius = radius;
    for (int r = 0; r < g->world.count && struck < 8; r++)
        for (int e = 0; e < g->world.rooms[r].enemy_count && struck < 8; e++) {
            Room *room = &g->world.rooms[r];
            if (!eligible(g, room, e))
                continue;
            Vec3 delta = vsub(enemy_center(&room->enemies[e]), origin);
            double distance = vlen(delta);
            if (distance > radius || (frontal && vdot(vnorm(delta), forward) < .4) ||
                world_raycast(&g->world, origin, vnorm(delta), distance) < distance - .1)
                continue;
            struck++;
            game_damage_enemy(g, room, e, owner, damage, W_BUILD);
        }
}
void game_build_hit(Game *g, Room *room, int index, int owner, uint32_t flags) {
    if (owner < 0 || owner >= g->player_count || !(flags & W_MARK) || (flags & W_BUILD) ||
        !g->builds[owner].mark_armed || !game_build_active(g, owner, BUILD_AMBUSH))
        return;
    g->builds[owner].mark_target = game_actor_id(g, room, &room->enemies[index]);
    g->builds[owner].mark_armed = false;
    g->builds[owner].mark_life = 6;
}
void game_build_kill(Game *g, Room *room, int index, int owner) {
    if (owner < 0 || owner >= g->player_count)
        return;
    BuildEffects *b = &g->builds[owner];
    if (b->mark_life <= 0 || !game_build_active(g, owner, BUILD_AMBUSH) ||
        !game_actor_same(b->mark_target, game_actor_id(g, room, &room->enemies[index])))
        return;
    b->mark_life = 0;
    game_build_wave(g, owner, enemy_center(&room->enemies[index]), 8,
                    70 + 8 * player_rank(&g->players[owner], g->players[owner].power), false);
}
void game_build_contact(Game *g, Room *room, int index, int owner, float damage) {
    if (!game_build_active(g, owner, BUILD_FOCUS) || !eligible(g, room, index))
        return;
    BuildEffects *b = &g->builds[owner];
    ActorId target = game_actor_id(g, room, &room->enemies[index]);
    float gap = (float)g->time - b->contact_time;
    float allowed = fmaxf(.3f, game_gun(&g->players[owner])->interval * 1.8f);
    b->focus = game_actor_same(target, b->focus_target) && gap <= allowed
                   ? fminf(1.2f, b->focus + fmaxf(0, gap))
                   : 0;
    b->focus_target = target;
    b->contact_time = (float)g->time;
    if (b->focus < 1.2f || g->players[owner].energy < 12)
        return;
    b->focus = 0;
    g->players[owner].energy -= 12;
    Vec3 origin = player_eye(&g->players[owner]);
    Vec3 delta = vsub(enemy_center(&room->enemies[index]), origin), direction = vnorm(delta);
    double range = world_raycast(&g->world, origin, direction, fmin(80, vlen(delta) + 18));
    b->relays[0] =
        (Beam){position_from(origin), position_from(vadd(origin, vmul(direction, range))),
               rgb(.75f, .85f, 1), .25f};
    unsigned struck = 0;
    for (int r = 0; r < g->world.count && struck < 3; r++)
        for (int e = 0; e < g->world.rooms[r].enemy_count && struck < 3; e++) {
            Room *other = &g->world.rooms[r];
            if (!eligible(g, other, e))
                continue;
            Vec3 ray = vsub(enemy_center(&other->enemies[e]), origin);
            double along = vdot(ray, direction);
            if (along < 0 || along > range || vlen(vsub(ray, vmul(direction, along))) > .9)
                continue;
            struck++;
            game_damage_enemy(g, other, e, owner, damage * 2, W_BUILD | W_BEAM);
        }
}
void game_build_branches_step(Game *g, double dt) {
    if (g->campaign.version != 2)
        return;
    float step = (float)clampd(dt, 0, .25);
    for (int i = 0; i < g->player_count; i++) {
        Player *p = &g->players[i];
        BuildEffects *b = &g->builds[i];
        b->pulse_life = fmaxf(0, b->pulse_life - step);
        if (p->sheltered || g->matrix.divers[i].active)
            continue;
        b->mark_life = fmaxf(0, b->mark_life - step);
        if (!game_build_active(g, i, BUILD_AMBUSH))
            b->mark_life = 0;
        if (!game_build_active(g, i, BUILD_FOCUS) ||
            g->time - b->contact_time > fmax(.3, game_gun(p)->interval * 1.8))
            b->focus = 0;
        bool brace = p->riding == MECH && p->grounded && fabs(p->velocity.y) < .5 &&
                     game_build_active(g, i, BUILD_BRACE) &&
                     hypot(p->velocity.x, p->velocity.z) < .35;
        b->brace_time = brace ? fminf(1, b->brace_time + step) : 0;
        if (!brace)
            b->brace_charge = 0;
        if (!game_build_active(g, i, BUILD_LANDING) || p->riding || p->flight > 0)
            b->landing_armed = false;
        if (b->landing_armed && p->grounded) {
            b->landing_armed = false;
            game_build_wave(g, i, player_eye(p), 7, 65, false);
        }
        b->field_tick = fmaxf(0, b->field_tick - step);
        if (game_build_active(g, i, BUILD_FIELD) && p->flight > 0 && !p->riding &&
            b->field_tick <= 0) {
            if (p->energy < 2) {
                p->flight = 0;
                continue;
            }
            p->energy -= 2;
            b->field_tick = .25f;
            game_build_wave(g, i, player_eye(p), 3.5f, 8, false);
        }
    }
}
