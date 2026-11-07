#include "game.h"

const char *alien_hint(unsigned behavior) {
    static const char *hints[ALIEN_BEHAVIORS] = {
        "",
        "DODGE THE LOCKED RUSH",
        "MOVE AFTER THE EYE LOCKS",
        "WATCH THE CEILING AND LANDING MARK",
        "BREAK THE SUPPORT LINKS",
        "CROUCH QUIETLY / DESTROY THE LISTENER",
        "WATCH IT TO STOP IT",
        "WATCH WHICH MOUTH LIGHTS / BOTH ENDS ARE VULNERABLE",
        "THREE BODIES / ONE CORE / SPLASH HELPS",
        "ONE MOLT / STRIKE DURING THE SPLIT",
        "HYDRAULIC CLICKS / SIDESTEP THE SWEEP",
        "MOVING SEAM / BAIT ITS LUNGE",
        "FOLLOW THE RIPPLE TO THE REAL BODY",
        "CRACKING SHELL / DODGE THE SHORT BURST",
        "INFLATING BELLY / ATTACK AFTER VENTING"};
    return hints[behavior < ALIEN_BEHAVIORS ? behavior : 0];
}
static void state(Enemy *e, unsigned motion, float duration) {
    e->motion = (uint8_t)motion;
    e->motion_time = duration;
    e->windup = motion == MOTION_WARN ? duration : 0;
    if (motion == MOTION_WARN)
        e->warning_sound = 1;
    if (motion == MOTION_COMMIT)
        e->attack_done = false;
}
static void move(Game *g, Enemy *e, Vec3 delta) {
    world_move(&g->world, &e->pos, delta, .42, 1.25, true, NULL);
}
bool alien_observed(const Game *g, const Enemy *e) {
    Vec3 at = enemy_center(e);
    for (int i = 0; i < g->player_count; i++) {
        const Player *p = &g->players[i];
        if (p->sheltered || p->health <= 0 || g->matrix.divers[i].active)
            continue;
        Vec3 eye = player_eye(p), ray = vsub(at, eye);
        double length = vlen(ray);
        if (length < 65 && vdot(vnorm(ray), facing(p->yaw, p->pitch)) > .72 &&
            world_raycast(&g->world, eye, vnorm(ray), length) >= length - .15)
            return true;
    }
    return false;
}
void alien_fragments(const World *w, Enemy *e) {
    Vec3 center = enemy_center(e), forward = vnorm(e->heading);
    Vec3 right = vcross(v3(0, 1, 0), forward);
    for (int n = 0; n < 3; n++) {
        Vec3 offset = vadd(vmul(right, (n - 1) * 2.8 * e->unfold),
                           vmul(forward, n == 1 ? 0 : -1.8 * e->unfold));
        double length = vlen(offset), hit = world_raycast(w, center, vnorm(offset), length);
        e->fragments[n] = vadd(center, vmul(vnorm(offset), fmax(0, hit - .45)));
    }
}
float alien_damage(Enemy *e, float damage) {
    if (e->behavior == ALIEN_MOLTER) {
        damage *= e->molted ? (e->motion == MOTION_RECOVER ? 1.4f : 1) : .65f;
        if (!e->molted && e->health - damage > 0 &&
            e->health - damage < sc_enemies[e->kind].health * .6f) {
            e->molted = true;
            e->home = position_absolute(e->pos);
            state(e, MOTION_RECOVER, 1.6f);
            e->warning_sound = 1;
        }
    }
    if (e->behavior == ALIEN_BRITTLE && e->motion == MOTION_WARN)
        damage *= 1.5f;
    if (e->behavior == ALIEN_MACHINE && e->motion == MOTION_RECOVER)
        damage *= 1.35f;
    return damage;
}
static bool visible(const World *w, Vec3 from, Vec3 to) {
    Vec3 delta = vsub(to, from);
    return world_raycast(w, from, vnorm(delta), vlen(delta)) >= vlen(delta) - .1;
}
static void strike(Game *g, Enemy *e, int player, Vec3 from) {
    Player *p = &g->players[player];
    if (!e->attack_done && vlen(vsub(player_eye(p), from)) < 2.1 &&
        visible(&g->world, from, player_eye(p))) {
        game_hurt(g, player, sc_enemies[e->kind].damage, sc_enemies[e->kind].flags);
        e->attack_done = true;
    }
}
bool alien_special_step(Game *g, Room *room, int index, int player, bool attack, double dt) {
    Enemy *e = &room->enemies[index];
    if (e->behavior < ALIEN_LISTENER)
        return false;
    Player *p = &g->players[player];
    unsigned kind = e->behavior;
    Vec3 start = position_absolute(e->pos), delta = position_delta(p->pos, e->pos);
    Vec3 direction = vnorm(v3(delta.x, 0, delta.z));
    double distance = vlen(delta), speed = sc_enemies[e->kind].speed * (e->slow > 0 ? .4 : 1);
    bool sight = visible(&g->world, enemy_center(e), player_eye(p));
    e->heading = direction;
    e->observed = kind == ALIEN_WATCHED && alien_observed(g, e);
    if (e->observed) {
        /* Either player's actual line of sight freezes the shared body and
           cancels its pending attack. No camera-space teleportation. */
        state(e, MOTION_ALERT, .65f);
        e->travel_speed = 0;
        return true;
    }
    e->motion_time = fmaxf(0, e->motion_time - (float)dt);
    if (e->motion == MOTION_DORMANT)
        state(e, MOTION_ALERT, .5f);
    else if ((e->motion == MOTION_ALERT || e->motion == MOTION_RECOVER) && !e->motion_time)
        state(e, MOTION_APPROACH, 0);
    if (e->motion == MOTION_WARN) {
        e->windup = e->motion_time;
        if (!attack || e->flash > 0) {
            state(e, MOTION_RECOVER, kind == ALIEN_BRITTLE ? 1.6f : .9f);
            e->cooldown = 1;
        } else if (!e->motion_time) {
            state(e, MOTION_COMMIT, kind == ALIEN_COLONY ? 1.3f : .5f);
            if (kind == ALIEN_LISTENER) {
                /* The audible ripple reaches farther than ordinary awareness;
                   quiet feet never trigger it. Existing slots bound the response. */
                for (int i = 0; i < room->enemy_count; i++)
                    if (i != index && room->enemies[i].alive) {
                        room->enemies[i].special = 0;
                        room->enemies[i].cooldown = 0;
                        room->enemies[i].warning_sound = .5f;
                    }
                room->alarm_time = 20;
                e->attack_done = true;
            } else if (kind == ALIEN_REFLECTION || kind == ALIEN_PRESSURE) {
                game_enemy_shot(g, room, e);
                e->attack_done = true;
            }
        }
    }
    if (e->motion == MOTION_COMMIT) {
        if (kind != ALIEN_LISTENER && kind != ALIEN_REFLECTION && kind != ALIEN_PRESSURE) {
            double rush = kind == ALIEN_BRITTLE ? 18 : kind == ALIEN_MACHINE ? 9 : 12;
            move(g, e, vmul(e->aim, rush * dt));
            strike(g, e, player, enemy_center(e));
            if (kind == ALIEN_COLONY) {
                e->unfold = fminf(1, e->unfold + (float)dt * 3);
                alien_fragments(&g->world, e);
                for (int n = 0; n < 3; n++)
                    strike(g, e, player, e->fragments[n]);
            }
        }
        if (!e->motion_time) {
            state(e, MOTION_RECOVER, kind == ALIEN_PRESSURE ? 2 : 1.25f);
            e->cooldown = sc_enemies[e->kind].interval;
        }
    } else if (e->motion == MOTION_APPROACH) {
        double range = kind == ALIEN_LISTENER                               ? 35
                       : kind == ALIEN_REFLECTION || kind == ALIEN_PRESSURE ? 30
                                                                            : 10;
        bool trigger = attack && sight && distance < range && e->cooldown <= 0;
        if (kind == ALIEN_LISTENER)
            trigger = attack && p->noise * 35 > distance && e->cooldown <= 0;
        if (kind == ALIEN_AMBUSH)
            trigger &= distance < 7;
        if (kind == ALIEN_PRESSURE)
            /* Vents discharge in the first part of the cycle. The creature
               inflates as they subside; its shot occupies the recovery gap. */
            trigger &=
                fmod(g->time + room->hash % 7, 7) >= 3.5 && fmod(g->time + room->hash % 7, 7) < 5;
        if (trigger) {
            if (kind == ALIEN_REVERSER)
                e->reversed = !e->reversed;
            e->aim = kind == ALIEN_REFLECTION || kind == ALIEN_PRESSURE ? player_eye(p) : direction;
            state(e, MOTION_WARN, kind == ALIEN_BRITTLE ? 1.5f : kind == ALIEN_MACHINE ? 1.3f : 1);
        } else if (kind != ALIEN_LISTENER && kind != ALIEN_AMBUSH && distance > 2) {
            if (kind == ALIEN_MACHINE)
                speed *= fmod(g->time + e->id, 1.2) < .72 ? 0 : 2.8;
            if (kind == ALIEN_MOLTER && e->molted)
                speed *= 2.2;
            if ((kind == ALIEN_PRESSURE || kind == ALIEN_REFLECTION) && distance < 14)
                direction = vmul(vcross(direction, v3(0, 1, 0)), e->id & 1 ? 1 : -1);
            move(g, e, vmul(direction, speed * dt));
        }
    }
    if (kind == ALIEN_AMBUSH && e->motion == MOTION_RECOVER) {
        Vec3 back = vsub(e->home, position_absolute(e->pos));
        move(g, e, vmul(vnorm(back), fmin(vlen(back), dt * 8)));
    }
    if (kind == ALIEN_COLONY && e->motion != MOTION_COMMIT) {
        e->unfold = fmaxf(0, e->unfold - (float)dt);
        alien_fragments(&g->world, e);
    }
    bool grounded = false;
    e->vertical_speed = fmaxf(-16, e->vertical_speed - (float)(22 * dt));
    world_move(&g->world, &e->pos, v3(0, e->vertical_speed * dt, 0), .42, 1.25, false, &grounded);
    if (grounded)
        e->vertical_speed = 0;
    Vec3 movement = vsub(position_absolute(e->pos), start);
    e->travel_speed = (float)(vlen(movement) / dt);
    e->stride += (float)vlen(movement);
    Vec3 facing = kind == ALIEN_REVERSER && e->reversed ? vmul(e->heading, -1) : e->heading;
    e->body_heading =
        vnorm(vadd(vmul(e->body_heading, 1 - fmin(1, dt * 3)), vmul(facing, fmin(1, dt * 3))));
    return true;
}
void alien_surface_step(Game *g, Room *r, Enemy *e, Vec3 direction, double dt) {
    static const Vec3 rays[5] = {{0, 1, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};
    Vec3 center = enemy_center(e), normal = {0};
    for (int i = 0; i < 5; i++) {
        double hit = world_raycast(&g->world, center, rays[i], i ? 1.8 : 1.6);
        if (hit > .15 && hit < (i ? 1.8 : 1.6)) {
            normal = vmul(rays[i], -1);
            break;
        }
    }
    e->climbing = vlen(normal) > .5;
    e->normal = e->climbing ? normal : v3(0, 1, 0);
    if (!e->climbing) {
        move(g, e, vmul(direction, sc_enemies[e->kind].speed * dt));
        return;
    }
    e->vertical_speed = 0;
    Vec3 along = vsub(direction, vmul(normal, vdot(direction, normal)));
    if (normal.y > -.5) {
        if (vlen(along) < .1)
            along = vcross(v3(0, 1, 0), normal);
        along = vadd(vmul(vnorm(along), 1.4), v3(0, 3.5, 0));
    } else
        along = vmul(vnorm(along), 3.2);
    double grip = fmod(g->time + e->id * .17, .55) < .14 ? 0 : 1;
    move(g, e, vmul(along, dt * grip * (e->slow > 0 ? .4 : 1)));
    Vec3 tangent = vnorm(vcross(normal, vlen(along) > .1 ? vnorm(along) : v3(0, 0, 1)));
    if (vlen(tangent) < .1)
        tangent = v3(1, 0, 0);
    center = enemy_center(e);
    for (int i = 0; i < 4; i++) {
        if (((int)(e->stride * 2) + i) % 2 && vlen(e->contacts[i]) > .01 &&
            vlen(vsub(e->contacts[i], center)) < 2.5)
            continue;
        Vec3 from = vadd(
            center, vadd(vmul(tangent, i & 1 ? .65 : -.65), vmul(vnorm(along), i & 2 ? .5 : -.5)));
        double hit = world_raycast(&g->world, from, vmul(normal, -1), 2);
        e->contacts[i] = hit < 2 ? vadd(from, vmul(normal, -hit)) : v3(0, 0, 0);
    }
    (void)r;
}
