#include "game.h"

static void move_guardian(Game *g, Enemy *e, Vec3 delta) {
    for (int axis = 0; axis < 2; axis++) {
        Position next = e->pos;
        position_move(&next, v3(axis ? 0 : delta.x, 0, axis ? delta.z : 0));
        if (!world_overlap(&g->world, position_absolute(next), .65, 2))
            e->pos = next;
    }
}
static void guardian_shot(Game *g, Vec3 origin, Vec3 direction, unsigned family, unsigned phase,
                          unsigned guardian) {
    if (world_raycast(&g->world, origin, direction, 1.1) < 1.1)
        return;
    for (int i = 0; i < SC_MAX_PROJECTILES; i++)
        if (!g->projectiles[i].active) {
            g->projectiles[i] =
                (Projectile){.active = true,
                             .owner = -1,
                             .guardian = (uint8_t)(guardian + 1),
                             .pos = position_from(vadd(origin, vmul(direction, 1.1))),
                             .velocity = vmul(direction, 13 + phase),
                             .life = 7,
                             .damage = 18 + phase * 5.f,
                             .flags = family == 3   ? CHILL
                                      : family == 4 ? DRAIN
                                                    : 0,
                             .color = family == 3   ? rgb(.2f, .7f, 1)
                                      : family == 4 ? rgb(1, .3f, .65f)
                                                    : rgb(1, .55f, .12f)};
            g->enemy_shots++;
            return;
        }
}
bool campaign_enemy_step(Game *g, Room *room, int index, double dt) {
    if (guardian_step(g, room, index, dt))
        return true;
    if (!g->campaign.version || index != 0)
        return false;
    int id = campaign_guardian(room->cell);
    if (id < 0)
        return false;
    GuardianAttempt *a = &g->campaign.attempts[id];
    Enemy *e = &room->enemies[index];
    e->flash = fmaxf(0, e->flash - (float)dt);
    if (!a->active) {
        e->windup = 0;
        return true;
    }
    e->slow = fmaxf(0, e->slow - (float)dt);
    if (e->burn > 0) {
        e->burn = fmaxf(0, e->burn - (float)dt);
        game_damage_enemy(g, room, index, e->last_player, (float)dt * 7, 0);
        if (!e->alive)
            return true;
    }
    int target = -1;
    double closest = 260;
    for (int i = 0; i < g->player_count; i++) {
        Player *p = &g->players[i];
        if (!(a->participants & (1u << i)) || p->sheltered || p->health <= 0 || p->cloaked)
            continue;
        Room *at = world_find(&g->world, world_cell(position_absolute(p->pos)));
        if (at != room)
            continue;
        double distance = vlen(position_delta(e->pos, p->pos));
        if (distance < closest) {
            target = i;
            closest = distance;
        }
    }
    if (target < 0)
        return true;
    unsigned family = campaign_family(g->world.seed, (unsigned)id), phase = id == 8 ? 0 : id / 2;
    uint32_t systems = world_systems(&g->world, room->cell);
    if (g->story.done & ((1u << STORY_POWER) | (1u << STORY_REACTOR)))
        systems |= SYSTEM_POWER;
    if (g->story.done & (1u << STORY_OVERRIDE))
        systems |= SYSTEM_SILENCED;
    Vec3 origin = vadd(position_absolute(e->pos), v3(0, 1.1, 0));
    Vec3 feet = position_absolute(g->players[target].pos);
    Vec3 toward = vnorm(vsub(feet, position_absolute(e->pos)));
    toward.y = 0;
    e->heading = toward;
    a->opening = fmaxf(0, a->opening - (float)dt);
    a->timer -= (float)dt;
    if (e->windup > 0) {
        e->windup = fmaxf(0, e->windup - (float)dt);
        if (e->windup > 0)
            return true;
        Vec3 direction = vnorm(vsub(a->aim, origin));
        unsigned count = (family == 2 && a->attack % 2) ? 12 : family == 0 ? 7 : 5;
        if (systems & SYSTEM_SILENCED)
            count = 3;
        for (unsigned i = 0; i < count; i++) {
            double angle = family == 2 && count == 12 ? SC_PI * 2 * i / count
                                                      : ((double)i - (count - 1) * .5) * .13;
            Vec3 dir =
                family == 2 && count == 12
                    ? v3(sin(angle), 0, cos(angle))
                    : vnorm(v3(direction.x * cos(angle) - direction.z * sin(angle), direction.y,
                               direction.x * sin(angle) + direction.z * cos(angle)));
            guardian_shot(g, origin, dir, family, phase, (unsigned)id);
        }
        a->opening = 2.2f + ((systems & SYSTEM_POWER) ? 1.2f : 0);
        a->timer = 4.2f + ((systems & SYSTEM_SILENCED) ? 1 : 0);
        a->attack++;
        e->aim = direction;
    } else if (a->timer <= 0) {
        a->aim = vadd(feet, v3(0, g->players[target].crouched ? .5 : .95, 0));
        e->windup = .95f;
    } else {
        Vec3 direction = toward;
        double speed = closest > 20 ? 3.5 : 0;
        if (family == 1 && a->opening > 1.35f) {
            direction = e->aim;
            direction.y = 0;
            speed = 15;
        } else if (family == 2 || family == 3) {
            direction = vnorm(vadd(toward, vmul(vcross(toward, v3(0, 1, 0)),
                                                sin(g->time * .8) * (closest < 20 ? 2 : .5))));
            speed = 4;
        }
        move_guardian(g, e, vmul(direction, speed * dt * (e->slow > 0 ? .5 : 1)));
        if (closest < 2.6 && a->opening > 0 && a->timer > 3.5) {
            game_hostile_hit(g, target, 22 + phase * 5.f, 0);
            a->timer = 3.4f;
        }
    }
    return true;
}
