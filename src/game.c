#include "game.h"
#include <stdio.h>
#include <string.h>

float player_max_health(const Player *p) {
    return 100.0f + 25.0f * p->vitality;
}
unsigned player_next_level(const Player *p) {
    return 80 + 40 * (p->level - 1);
}
Vec3 player_eye(const Player *p) {
    return vadd(position_absolute(p->pos), v3(0, p->crouched ? .85 : 1.62, 0));
}
void game_message(Game *g, const char *s) {
    snprintf(g->message, sizeof(g->message), "%s", s);
    g->message_time = 4;
}
static void player_init(Player *p) {
    memset(p, 0, sizeof(*p));
    p->pos = position_from(v3(0, 4, -34));
    p->checkpoint = position_from(v3(0, .02, 12));
    p->level = 1;
    p->health = 100;
    p->pitch = -.04;
}
void game_init(Game *g, uint32_t seed) {
    memset(g, 0, sizeof(*g));
    world_init(&g->world, seed);
    g->player_count = 1;
    for (int i = 0; i < SC_PLAYERS; i++)
        player_init(&g->players[i]);
    Vec3 p = position_absolute(g->players[0].pos);
    world_refresh(&g->world, &p, 1);
    game_message(g, "THE BAY IS SAFE. THE SHIP BEYOND IT IS NOT.");
}
void game_destroy(Game *g) {
    world_destroy(&g->world);
}
void game_look(Player *p, double yaw, double pitch) {
    p->yaw = remainder(p->yaw + yaw, SC_PI * 2);
    p->pitch = clampd(p->pitch + pitch, -1.48, 1.48);
}
void game_join(Game *g) {
    if (g->player_count == 2)
        return;
    Player *p = &g->players[1];
    p->pos = g->players[0].pos;
    position_move(&p->pos, v3(1.4, 0, 0));
    p->checkpoint = g->players[0].checkpoint;
    if (world_overlap(&g->world, position_absolute(p->pos), .34, 1.78))
        p->pos = g->players[0].pos;
    p->crouched = world_overlap(&g->world, position_absolute(p->pos), .34, 1.78);
    p->velocity = v3(0, 0, 0);
    p->yaw = g->players[0].yaw;
    p->pitch = 0;
    p->health = player_max_health(p);
    g->player_count = 2;
    g->dirty = true;
    game_message(g, "LOCAL CO-OP JOINED / PLAYER TWO USES A GAMEPAD");
}
void game_leave(Game *g) {
    g->player_count = 1;
    g->dirty = true;
    game_message(g, "SOLO EXPLORATION");
}
void game_new_ship(Game *g, uint32_t seed) {
    world_destroy(&g->world);
    world_init(&g->world, seed);
    for (int i = 0; i < SC_PLAYERS; i++) {
        Player *p = &g->players[i];
        p->pos = position_from(v3(i * 1.4, 4, -34));
        p->checkpoint = position_from(v3(0, .02, 12));
        p->velocity = v3(0, 0, 0);
        p->yaw = 0;
        p->pitch = -.04;
        p->health = player_max_health(p);
        p->grounded = false;
        p->crouched = false;
    }
    Vec3 positions[2] = {position_absolute(g->players[0].pos),
                         position_absolute(g->players[1].pos)};
    world_refresh(&g->world, positions, g->player_count);
    g->dirty = true;
    game_message(g, "NEW SHIP / YOUR CHARACTER ENDURES");
}
bool game_upgrade(Game *g, int player, int upgrade) {
    if (player < 0 || player >= g->player_count || upgrade < 0 || upgrade > 2)
        return false;
    Player *p = &g->players[player];
    if (!p->points)
        return false;
    uint32_t *attribute = upgrade == 0 ? &p->power : (upgrade == 1 ? &p->vitality : &p->haste);
    if (*attribute >= 50)
        return false;
    (*attribute)++;
    p->points--;
    p->health = player_max_health(p);
    g->dirty = true;
    game_message(g, upgrade == 0 ? "CORE UPGRADED / WEAPON DAMAGE"
                                 : (upgrade == 1 ? "CORE UPGRADED / MAXIMUM HEALTH"
                                                 : "CORE UPGRADED / FIRE RATE"));
    return true;
}
static void award(Game *g, Player *p, unsigned xp) {
    p->kills++;
    p->xp += xp;
    while (p->xp >= player_next_level(p) && p->level < 1000) {
        p->xp -= player_next_level(p);
        p->level++;
        p->points++;
        p->health = player_max_health(p);
        game_message(g, "ASCENSION / CHOOSE AN UPGRADE WITH 1, 2 OR 3");
    }
    g->dirty = true;
}
static void move_body(World *w, Position *p, Vec3 delta, double radius, double height,
                      bool *grounded) {
    int steps = (int)ceil(vlen(delta) / .16);
    if (steps < 1)
        steps = 1;
    if (steps > 64)
        steps = 64;
    delta = vmul(delta, 1.0 / steps);
    for (int i = 0; i < steps; i++)
        for (int axis = 0; axis < 3; axis++) {
            Position q = *p;
            Vec3 part =
                v3(axis == 0 ? delta.x : 0, axis == 2 ? delta.y : 0, axis == 1 ? delta.z : 0);
            position_move(&q, part);
            if (!world_overlap(w, position_absolute(q), radius, height))
                *p = q;
            else if (axis == 2 && delta.y < 0 && grounded)
                *grounded = true;
        }
}
static void shoot(Game *g, Player *p) {
    p->cooldown = (float)(.19 / (1 + .10 * p->haste));
    p->muzzle = .075f;
    g->shots++;
    Vec3 origin = player_eye(p), dir = facing(p->yaw, p->pitch);
    double nearest = world_raycast(&g->world, origin, dir, 110);
    Room *target_room = NULL;
    int target = -1;
    for (int r = 0; r < g->world.count; r++) {
        Room *room = &g->world.rooms[r];
        for (int i = 0; i < room->enemy_count; i++) {
            Enemy *e = &room->enemies[i];
            if (!e->alive)
                continue;
            Vec3 center = vadd(position_absolute(e->pos), v3(0, .75, 0)), oc = vsub(origin, center);
            double radius = e->kind == 2 ? 1.05 : .70;
            double b = vdot(oc, dir), c = vdot(oc, oc) - radius * radius, disc = b * b - c;
            if (disc < 0)
                continue;
            double t = -b - sqrt(disc);
            if (t < 0)
                t = -b + sqrt(disc);
            if (t >= 0 && t < nearest) {
                nearest = t;
                target_room = room;
                target = i;
            }
        }
    }
    if (target >= 0) {
        Enemy *e = &target_room->enemies[target];
        e->health -= 27 + 11 * p->power;
        e->flash = .12f;
        g->hits++;
        if (e->health <= 0) {
            unsigned xp =
                20 + e->kind * 10 + (unsigned)fmin(world_difficulty(target_room->cell), 30);
            world_kill(&g->world, target_room, target);
            award(g, p, xp);
        }
    }
}
static void move_player(Game *g, Player *p, const Actions *a, double dt) {
    bool outside = position_absolute(p->pos).z < 0;
    if (a->crouch)
        p->crouched = true;
    else if (!world_overlap(&g->world, position_absolute(p->pos), .34, 1.78))
        p->crouched = false;
    Vec3 forward = outside ? facing(p->yaw, p->pitch) : v3(sin(p->yaw), 0, cos(p->yaw));
    Vec3 right = v3(cos(p->yaw), 0, -sin(p->yaw));
    Vec3 wish = vadd(vmul(forward, a->forward), vmul(right, a->strafe));
    if (outside)
        wish.y += a->vertical + (a->jump ? 1 : 0) - (a->crouch ? 1 : 0);
    if (vlen(wish) > 1)
        wish = vnorm(wish);
    double speed = outside ? 14 : (p->crouched ? 3.4 : (a->sprint ? 11 : 7.5));
    if (outside)
        p->velocity = vmul(wish, speed);
    else {
        Room *room = world_find(&g->world, world_cell(position_absolute(p->pos)));
        double gravity = room && room->type == GARDEN ? 9 : 22;
        p->velocity.x = wish.x * speed;
        p->velocity.z = wish.z * speed;
        if (p->grounded) {
            p->velocity.y = 0;
            if (a->jump && !p->crouched)
                p->velocity.y = 7.5;
        }
        p->velocity.y = fmax(-30, p->velocity.y - gravity * dt);
    }
    p->grounded = false;
    double before_y = p->pos.local.y;
    move_body(&g->world, &p->pos, vmul(p->velocity, dt), .34, p->crouched ? 1.0 : 1.78,
              &p->grounded);
    if (fabs(p->pos.local.y - before_y) < 1e-7 && p->velocity.y > 0)
        p->velocity.y = 0;
    if (p->grounded)
        p->velocity.y = 0;
    p->lift_cooldown = fmaxf(0, p->lift_cooldown - (float)dt);
    Cell cell = world_cell(position_absolute(p->pos));
    Vec3 lift = world_lift(cell), diff = vsub(position_absolute(p->pos), lift);
    diff.y = 0;
    if (!outside && vlen(diff) < 3.3 && p->lift_cooldown <= 0 && (a->lift_up || a->lift_down)) {
        cell.y += a->lift_up ? 1 : -1;
        if (!world_ready(&g->world, cell)) {
            game_message(g, "SURVEYING DESTINATION / LIFT CHARGING");
            return;
        }
        p->pos = position_from(world_lift(cell));
        p->checkpoint = p->pos;
        p->velocity = v3(0, 0, 0);
        p->crouched = true;
        p->lift_cooldown = 1;
        Vec3 positions[2] = {position_absolute(g->players[0].pos),
                             position_absolute(g->players[1].pos)};
        world_refresh(&g->world, positions, g->player_count);
        game_message(g, "GRAV LIFT / CHECKPOINT SYNCHRONIZED");
        g->dirty = true;
    }
}
void game_step(Game *g, const Actions *actions, double dt) {
    if (!isfinite(dt) || dt <= 0 || dt > .05)
        return;
    g->time += dt;
    g->message_time = fmaxf(0, g->message_time - (float)dt);
    Vec3 positions[2] = {position_absolute(g->players[0].pos),
                         position_absolute(g->players[1].pos)};
    if (!world_refresh(&g->world, positions, g->player_count))
        return;
    for (int i = 0; i < g->player_count; i++) {
        Player *p = &g->players[i];
        if (position_absolute(p->pos).z >= 0 &&
            !world_ready(&g->world, world_cell(position_absolute(p->pos))))
            continue;
        p->cooldown = fmaxf(0, p->cooldown - (float)dt);
        p->muzzle = fmaxf(0, p->muzzle - (float)dt);
        p->hurt = fmaxf(0, p->hurt - (float)dt);
        if (p->hurt <= 0 && p->health > 0)
            p->health = fminf(player_max_health(p), p->health + (float)dt * 3);
        move_player(g, p, &actions[i], dt);
        if (actions[i].fire && p->cooldown <= 0)
            shoot(g, p);
    }
    for (int r = 0; r < g->world.count; r++) {
        Room *room = &g->world.rooms[r];
        for (int i = 0; i < room->enemy_count; i++) {
            Enemy *e = &room->enemies[i];
            if (!e->alive)
                continue;
            e->flash = fmaxf(0, e->flash - (float)dt);
            e->cooldown = fmaxf(0, e->cooldown - (float)dt);
            Player *target = NULL;
            double best = 48;
            for (int p = 0; p < g->player_count; p++) {
                Player *t = &g->players[p];
                if (position_absolute(t->pos).z < 0 ||
                    world_difficulty(world_cell(position_absolute(t->pos))) == 0)
                    continue;
                if (fabs(position_delta(t->pos, e->pos).y) > 3)
                    continue;
                double distance = vlen(position_delta(t->pos, e->pos));
                if (distance < best) {
                    best = distance;
                    target = t;
                }
            }
            if (!target)
                continue;
            Vec3 diff = position_delta(target->pos, e->pos);
            diff.y = 0;
            Vec3 direction = vnorm(diff);
            double range = e->kind == 1 ? 14 : 1.6;
            if (best > range * .85) {
                double speed = e->kind == 2 ? 2.0 : 3.5;
                move_body(&g->world, &e->pos, vmul(direction, speed * dt), .42, 1.25, NULL);
            }
            if (best < range && e->cooldown <= 0) {
                Vec3 eye = vadd(position_absolute(e->pos), v3(0, .8, 0)),
                     to = vsub(player_eye(target), eye);
                if (world_raycast(&g->world, eye, vnorm(to), vlen(to)) >= vlen(to) - .1) {
                    target->health -= 8 + (float)fmin(world_difficulty(room->cell) * 1.5, 24);
                    target->hurt = .8f;
                    e->cooldown = e->kind == 1 ? 1.8f : .9f;
                }
            }
        }
    }
    for (int i = 0; i < g->player_count; i++) {
        Player *p = &g->players[i];
        if (p->health <= 0 || position_absolute(p->pos).z < -400) {
            p->pos = p->checkpoint;
            p->velocity = v3(0, 0, 0);
            p->health = player_max_health(p);
            p->hurt = 0;
            p->crouched = true;
            g->respawns++;
            g->dirty = true;
            game_message(g, "RECONSTITUTED / YOUR PROGRESS IS SAFE");
        }
    }
}
