#include "game.h"

static void phase(Enemy *e, unsigned motion, float time) {
    e->motion = (uint8_t)motion;
    e->motion_time = time;
    e->windup = motion == MOTION_WARN ? time : 0;
    if (motion == MOTION_COMMIT)
        e->attack_done = false;
}
static void travel(Game *g, Enemy *e, Vec3 delta) {
    world_move(&g->world, &e->pos, delta, .42, 1.25, true, NULL);
}
static Vec3 surface(const World *w, const Enemy *e) {
    static const Vec3 directions[] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};
    Vec3 normal = {0};
    double nearest = 1.35;
    for (int i = 0; i < 4; i++) {
        double hit = world_raycast(w, enemy_center(e), directions[i], nearest);
        if (hit > .25 && hit < nearest) {
            nearest = hit;
            normal = vmul(directions[i], -1);
        }
    }
    return normal;
}
static void plant(Game *g, Enemy *e, Vec3 wall) {
    Vec3 side = vcross(v3(0, 1, 0), wall), center = enemy_center(e);
    for (int i = 0; i < 4; i++) {
        /* A diagonal pair stays planted while the other pair reaches. */
        if (((int)(e->stride * 2) + i) % 2 && vlen(e->contacts[i]) > .01)
            continue;
        Vec3 from = vadd(center, vadd(vmul(side, i & 1 ? .7 : -.7), v3(0, i & 2 ? .7 : -.3, 0)));
        double hit = world_raycast(&g->world, from, vmul(wall, -1), 2);
        e->contacts[i] = hit < 2 ? vadd(from, vmul(wall, -.98 * hit)) : v3(0, 0, 0);
    }
}
static void fall(Game *g, Enemy *e, double dt) {
    if (e->climbing)
        return;
    bool grounded = false;
    e->vertical_speed = fmaxf(-16, e->vertical_speed - (float)(22 * dt));
    world_move(&g->world, &e->pos, v3(0, e->vertical_speed * dt, 0), .42, 1.25, false, &grounded);
    if (grounded)
        e->vertical_speed = 0;
}
bool game_alien_step(Game *g, Room *r, int index, int player, bool attack, double dt) {
    Enemy *e = &r->enemies[index];
    if (!e->behavior)
        return false;
    if (alien_special_step(g, r, index, player, attack, dt))
        return true;
    Player *p = &g->players[player];
    Vec3 start = position_absolute(e->pos), feet = position_absolute(p->pos);
    Vec3 delta = vsub(feet, start), direction = vnorm(v3(delta.x, 0, delta.z));
    double distance = vlen(delta), speed = sc_enemies[e->kind].speed * (e->slow > 0 ? .4 : 1);
    Vec3 ray = vsub(player_eye(p), enemy_center(e));
    bool sight = world_raycast(&g->world, enemy_center(e), vnorm(ray), vlen(ray)) >= vlen(ray) - .1;
    e->heading = direction;
    if (e->behavior == ALIEN_CONDUCTOR) {
        e->body_heading = direction;
        e->motion = e->windup > 0 ? MOTION_WARN : MOTION_APPROACH;
        return true;
    }
    e->motion_time = fmaxf(0, e->motion_time - (float)dt);
    if (e->motion == MOTION_DORMANT)
        phase(e, MOTION_ALERT, .45f);
    else if ((e->motion == MOTION_ALERT || e->motion == MOTION_RECOVER) && e->motion_time <= 0)
        phase(e, MOTION_APPROACH, 0);
    if (e->motion == MOTION_WARN) {
        e->windup = e->motion_time;
        if (!attack || e->flash > 0) {
            phase(e, MOTION_RECOVER, .8f);
            e->cooldown = .9f;
        } else if (e->motion_time <= 0) {
            phase(e, MOTION_COMMIT,
                  e->behavior == ALIEN_GAZER ? .15f
                  : e->behavior == ALIEN_SEAM && g->world.generator >= 12
                      ? (float)clampd(e->climb_time / 18, .55, 7)
                      : .55f);
            if (e->behavior == ALIEN_GAZER) {
                game_enemy_shot(g, r, e);
                e->attack_done = true;
            } else {
                e->climbing = false;
                e->normal = v3(0, 1, 0);
                e->vertical_speed = -2;
            }
        }
    }
    if (e->motion == MOTION_COMMIT) {
        if (e->behavior != ALIEN_GAZER) {
            /* Fixed line; steering the camera or moving cannot retarget this rush. */
            travel(g, e, vmul(e->aim, (e->behavior == ALIEN_SEAM ? 11 : 14) * dt));
            Vec3 hit = vsub(player_eye(p), enemy_center(e));
            if (!e->attack_done && vlen(hit) < 1.9 &&
                world_raycast(&g->world, enemy_center(e), vnorm(hit), vlen(hit)) >=
                    vlen(hit) - .1) {
                game_hurt(g, player, sc_enemies[e->kind].damage, sc_enemies[e->kind].flags);
                e->attack_done = true;
            }
        }
        if (e->motion_time <= 0) {
            phase(e, MOTION_RECOVER, e->behavior == ALIEN_GAZER ? .9f : 1.2f);
            e->cooldown = sc_enemies[e->kind].interval;
        }
    } else if (e->motion == MOTION_APPROACH) {
        double range = e->behavior == ALIEN_GAZER ? 32
                       : e->behavior == ALIEN_SEAM
                           ? (g->world.generator >= 12 && e->normal.y < -.5 ? 140 : 9)
                           : 10;
        if (attack && sight && distance < range && e->cooldown <= 0) {
            e->aim = e->behavior == ALIEN_GAZER                              ? player_eye(p)
                     : e->behavior == ALIEN_SEAM && g->world.generator >= 12 ? vnorm(delta)
                                                                             : direction;
            e->warning_sound = 1;
            e->climb_time = (float)distance;
            phase(e, MOTION_WARN, e->behavior == ALIEN_GAZER ? 1.05f : .85f);
        } else if (e->behavior == ALIEN_SEAM) {
            if (g->world.generator >= 12) {
                alien_surface_step(g, r, e, direction, dt);
            } else {
                Vec3 wall = surface(&g->world, e);
                double height = start.y - r->cell.y * SC_DECK;
                if (vlen(wall) > .5 && distance > 3 && height < fmin(5.5, r->height - 2)) {
                    e->climbing = true;
                    e->normal = wall;
                    e->vertical_speed = 0;
                    plant(g, e, wall);
                    Vec3 along = vsub(direction, vmul(wall, vdot(direction, wall)));
                    if (vlen(along) < .1)
                        along = vmul(vcross(v3(0, 1, 0), wall), e->id & 1 ? 1 : -1);
                    /* Short planted pauses contrast with the committed drop. */
                    double grip = fmod(g->time + e->id * .17, .55) < .14 ? 0 : 1;
                    travel(g, e, vmul(vadd(vmul(vnorm(along), 2.3), v3(0, 2.6, 0)), dt * grip));
                } else {
                    e->climbing = false;
                    e->normal = v3(0, 1, 0);
                    if (distance > 2) {
                        Vec3 flank = vadd(direction, vmul(vcross(direction, v3(0, 1, 0)),
                                                          (e->id & 1 ? 1 : -1) * .45));
                        travel(g, e, vmul(vnorm(flank), speed * dt));
                    }
                }
            }
        } else if (e->behavior == ALIEN_GAZER) {
            Vec3 movement = distance > 18 || !sight ? direction
                            : distance < 7
                                ? vmul(direction, -1)
                                : vmul(vcross(direction, v3(0, 1, 0)), sin(g->time * .7 + e->id));
            travel(g, e, vmul(movement, speed * .7 * dt));
        } else if (distance > 1.8)
            travel(g, e, vmul(direction, speed * .75 * dt));
    }
    if (e->behavior == ALIEN_GAZER) {
        double wanted = r->cell.y * SC_DECK + .9 + sin(g->time * .7 + e->id) * .2;
        travel(g, e, v3(0, clampd(wanted - position_absolute(e->pos).y, -dt, dt), 0));
    } else
        fall(g, e, dt);
    Vec3 movement = vsub(position_absolute(e->pos), start);
    e->travel_speed = (float)(vlen(movement) / dt);
    e->stride += (float)vlen(movement);
    Vec3 facing = e->behavior == ALIEN_PILGRIM && e->motion != MOTION_COMMIT ? vmul(direction, -1)
                                                                             : direction;
    e->body_heading =
        vnorm(vadd(vmul(e->body_heading, 1 - fmin(1, dt * 4)), vmul(facing, fmin(1, dt * 4))));
    return true;
}
