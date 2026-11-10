#include "game.h"

unsigned guardian_pattern(unsigned id) {
    return (id / 2 + id % 2) % 4;
}
const char *guardian_name(const Game *g, unsigned id) {
    if (g->campaign.version < 2 || id == 8)
        return campaign_guardian_name(g->world.seed, id);
    static const char *names[] = {"SUSPENDED FURNACE", "INFESTED RELAY", "CRYO PROCESSIONAL",
                                  "GARDEN LOCOMOTIVE"};
    return names[guardian_pattern(id)];
}
const char *guardian_system_name(unsigned id, unsigned system) {
    if (guardian_pattern(id) == 2)
        return system ? "SHARD MIRROR / STOP THE SHARD FAN" : "VENT ARRAY / STOP THE COLD CROSS";
    if (guardian_pattern(id) == 3)
        return system ? "SEED MORTAR / STOP THE SATELLITES" : "ROOT DRIVE / HALT THE CHARGE";
    return guardian_pattern(id)
               ? (system ? "SPORE LUNG / DISABLE THE CROSS-FIRE" : "ROOT HEART / BREAK ITS PURSUIT")
               : (system ? "GUN PLATFORM / DISABLE THE FAN" : "COOLANT ASSEMBLY / STOP THE RING");
}
Vec3 guardian_console(unsigned id, unsigned system) {
    return vadd(world_center(campaign_site(id)), v3(system ? 10 : -10, .02, -18));
}
void guardian_populate(World *w, Room *r, unsigned id) {
    if (w->generator < 16 || id == 8)
        return;
    r->enemy_count = 3;
    r->enemies[0].behavior = 0;
    static const uint8_t bodies[] = {28, 25, 27, 34};
    r->enemies[0].kind = bodies[guardian_pattern(id)];
    r->enemies[0].pos = position_from(vadd(world_center(r->cell), v3(0, .02, 22)));
    for (unsigned i = 0; i < 2; i++) {
        r->enemies[i + 1] = (Enemy){
            .alive = r->enemies[0].alive,
            .id = (uint8_t)(i + 1),
            .kind = guardian_pattern(id) ? 25 : 16,
            .health = 180 + id / 2 * 70.f,
            .pos = position_from(vadd(world_center(r->cell), v3(i ? 8 : -8, .02, i ? 28 : 14))),
            .heading = {0, 0, -1}};
    }
}
void guardian_begin(Game *g, Room *r, unsigned id) {
    if (g->campaign.version < 2 || id == 8)
        return;
    GuardianAttempt *a = &g->campaign.attempts[id];
    uint32_t systems = world_systems(&g->world, r->cell);
    a->stage = GUARDIAN_CYCLE;
    a->broken = ((systems & SYSTEM_POWER) ? GUARDIAN_COOLANT : 0) |
                ((systems & SYSTEM_SILENCED) ? GUARDIAN_WEAPONS : 0);
    guardian_populate(&g->world, r, id);
    for (unsigned i = 0; i < 2; i++) {
        r->enemies[i + 1].alive = !(a->broken & (1u << i));
        if (a->participants != 1 && a->participants != 2)
            r->enemies[i + 1].health *= 1.45f;
    }
    if (a->broken)
        a->opening = 5;
}
bool guardian_prepare(Game *g, int player) {
    if (g->campaign.version < 2 || player < 0 || player >= g->player_count ||
        g->players[player].sheltered || g->players[player].health <= 0 ||
        g->matrix.divers[player].active)
        return false;
    Vec3 feet = position_absolute(g->players[player].pos);
    for (unsigned id = 0; id < 8; id++)
        for (unsigned system = 0; system < 2; system++) {
            if (vlen(vsub(feet, guardian_console(id, system))) > 2.5)
                continue;
            Room *room = world_find(&g->world, campaign_site(id));
            if (!room || !room->ready || g->campaign.attempts[id].active ||
                (g->campaign.defeated & (1u << id)))
                return true;
            uint32_t old = world_systems(&g->world, room->cell);
            uint32_t flags =
                system ? old | SYSTEM_SILENCED : (old | SYSTEM_POWER) & ~SYSTEM_BLACKOUT;
            if (!world_set_systems(&g->world, room->cell, flags))
                return true;
            g->dirty |= old != flags;
            game_message(g, system ? "SECOND ASSEMBLY ISOLATED / DISABLED ON EVERY RETRY"
                                   : "FIRST ASSEMBLY ISOLATED / DISABLED ON EVERY RETRY");
            return true;
        }
    return false;
}
bool guardian_component_damage(Game *g, Room *r, int index, int owner, float damage) {
    int id = campaign_guardian(r->cell);
    if (g->campaign.version < 2 || id < 0 || id == 8 || index < 1 || index > 2)
        return false;
    GuardianAttempt *a = &g->campaign.attempts[id];
    if (!a->active || !(a->participants & (1u << owner)) || !campaign_engaged(g, owner) ||
        world_find(&g->world, world_cell(position_absolute(g->players[owner].pos))) != r)
        return true;
    Enemy *e = &r->enemies[index];
    e->health -= damage;
    e->flash = .2f;
    e->last_player = (uint8_t)owner;
    g->hits++;
    if (e->health > 0)
        return true;
    e->health = 0;
    e->alive = false;
    a->broken |= 1u << (index - 1);
    a->opening = 5;
    r->enemies[0].windup = 0;
    a->timer = 5;
    game_enemy_death(g, e, owner);
    game_message(g, index == 1 ? "FIRST ASSEMBLY DESTROYED / CORE EXPOSED / SAFE FLOOR RETAINED"
                               : "GUN ASSEMBLY DESTROYED / CROSS-FIRE STOPPED / CORE EXPOSED");
    g->dirty = true;
    return true; /* No actor kill, pickup or XP: these belong to the attempt. */
}
static void shot(Game *g, unsigned id, Vec3 from, Vec3 toward, float speed, Color color,
                 uint32_t flags) {
    if (world_raycast(&g->world, from, toward, 1.5) < 1.5)
        return;
    for (int i = 0; i < SC_MAX_PROJECTILES; i++)
        if (!g->projectiles[i].active) {
            g->projectiles[i] = (Projectile){.active = true,
                                             .owner = -1,
                                             .guardian = (uint8_t)(id + 1),
                                             .pos = position_from(vadd(from, vmul(toward, 1.5))),
                                             .velocity = vmul(toward, speed),
                                             .life = 8,
                                             .damage = 16 + id / 2 * 4.f,
                                             .flags = flags,
                                             .color = color};
            g->enemy_shots++;
            return;
        }
}
static void volley(Game *g, unsigned id, Vec3 from, Vec3 aim, unsigned count, bool ring) {
    Vec3 direction = vnorm(vsub(aim, from));
    for (unsigned i = 0; i < count; i++) {
        double angle = ring ? SC_PI * 2 * i / count : ((double)i - (count - 1) * .5) * .18;
        Vec3 d = ring ? v3(sin(angle), 0, cos(angle))
                      : vnorm(v3(direction.x * cos(angle) - direction.z * sin(angle), direction.y,
                                 direction.x * sin(angle) + direction.z * cos(angle)));
        shot(g, id, from, d, ring ? 9 : 12, ring ? rgb(.2f, .85f, 1) : rgb(1, .4f, .12f), 0);
    }
}
bool guardian_step(Game *g, Room *r, int index, double dt) {
    int id = campaign_guardian(r->cell);
    if (g->campaign.version < 2 || id < 0 || id == 8)
        return false;
    Enemy *e = &r->enemies[index];
    e->flash = fmaxf(0, e->flash - (float)dt);
    if (index)
        return true;
    GuardianAttempt *a = &g->campaign.attempts[id];
    if (!a->active)
        return true;
    int target = -1;
    double nearest = 260;
    for (int p = 0; p < g->player_count; p++) {
        if (!(a->participants & (1u << p)) || !campaign_engaged(g, p) || g->players[p].cloaked ||
            world_find(&g->world, world_cell(position_absolute(g->players[p].pos))) != r)
            continue;
        double distance = vlen(position_delta(e->pos, g->players[p].pos));
        if (distance < nearest) {
            target = p;
            nearest = distance;
        }
    }
    if (target < 0)
        return true;
    if (a->stage == GUARDIAN_CYCLE &&
        (e->health < campaign_guardian_health((unsigned)id, a->participants) * .55f ||
         a->broken == 3)) {
        a->stage = GUARDIAN_OVERRUN;
        a->timer = fmaxf(a->timer, 1.5f);
        game_message(g, guardian_pattern((unsigned)id) == 2
                            ? "CRYO OVERRUN / ROTATING COLD CROSS / CROSS BETWEEN THE SPOKES"
                        : guardian_pattern((unsigned)id) == 3
                            ? "GARDEN OVERRUN / LOCKED CHARGE / STEP ASIDE AFTER ITS WARNING"
                        : guardian_pattern((unsigned)id)
                            ? "RELAY OVERRUN / WIDER SPORE FAN / DODGE SIDEWAYS"
                            : "FURNACE OVERRUN / ALTERNATING RING AND FAN / KEEP MOVING");
    }
    e->slow = fmaxf(0, e->slow - (float)dt);
    if (e->burn > 0) {
        e->burn = fmaxf(0, e->burn - (float)dt);
        game_damage_enemy(g, r, 0, e->last_player, (float)dt * 7, 0);
        if (!e->alive)
            return true;
    }
    a->opening = fmaxf(0, a->opening - (float)dt);
    a->timer -= (float)dt;
    Vec3 feet = position_absolute(g->players[target].pos), origin = enemy_center(e);
    e->heading = vnorm(vsub(feet, position_absolute(e->pos)));
    if (e->windup > 0) {
        e->windup = fmaxf(0, e->windup - (float)dt);
        if (e->windup > 0)
            return true;
        unsigned pattern = guardian_pattern((unsigned)id);
        bool infested = pattern == 1;
        unsigned stage = a->stage == GUARDIAN_OVERRUN;
        /* An intact coolant/heart supplies the ring; the second assembly supplies
           a second origin. Destroyed systems never silently respawn their attack. */
        if (pattern == 2) {
            volley(g, (unsigned)id, origin, a->aim, 1 + stage * 2, false);
            if (!(a->broken & GUARDIAN_COOLANT))
                for (unsigned arm = 0; arm < 4; arm++)
                    for (unsigned lane = 0; lane < 3; lane++) {
                        double angle = arm * SC_PI * .5 + (a->attack % 2) * SC_PI * .25;
                        Vec3 forward = v3(sin(angle), 0, cos(angle)),
                             side = v3(cos(angle), 0, -sin(angle));
                        shot(g, (unsigned)id,
                             vadd(enemy_center(&r->enemies[1]),
                                  vmul(side, ((double)lane - 1) * 1.4)),
                             forward, 8 + stage * 2.f, rgb(.4f, .8f, 1), CHILL);
                    }
            if (!(a->broken & GUARDIAN_WEAPONS))
                volley(g, (unsigned)id, enemy_center(&r->enemies[2]), a->aim, 3 + stage * 2, false);
        } else if (pattern == 3) {
            shot(g, (unsigned)id, origin, vnorm(vsub(a->aim, origin)), 10, rgb(.4f, 1, .3f), DRAIN);
            if (!(a->broken & GUARDIAN_WEAPONS))
                volley(g, (unsigned)id, enemy_center(&r->enemies[2]), a->aim, 3 + stage * 2, false);
        } else {
            volley(g, (unsigned)id, origin, a->aim, 3 + stage * 2, false);
            if (!(a->broken & GUARDIAN_COOLANT) && (!infested || a->attack % 2))
                volley(g, (unsigned)id, enemy_center(&r->enemies[1]), a->aim, 10 + stage * 4, true);
            if (!(a->broken & GUARDIAN_WEAPONS))
                volley(g, (unsigned)id, enemy_center(&r->enemies[2]), a->aim, 5 + stage * 2, false);
        }
        a->opening = infested ? 2.8f : 2.2f;
        a->timer = stage ? 3.4f : 4.8f;
        a->attack++;
    } else if (a->timer <= 0) {
        a->aim = vadd(feet, v3(0, g->players[target].crouched ? .5 : .95, 0));
        e->windup = 1.2f; /* Locked aim and the same visible/audio windup as other enemies. */
    } else if (guardian_pattern((unsigned)id) == 3 && a->attack &&
               !(a->broken & GUARDIAN_COOLANT)) {
        Vec3 delta = vsub(a->aim, origin);
        delta.y = 0;
        if (vlen(delta) > 2)
            world_move(&g->world, &e->pos, vmul(vnorm(delta), dt * 7), .7, 2, true, NULL);
        if (nearest < 2.2)
            game_hostile_hit_from(g, target, (float)dt * 18, 0, origin);
    } else if (guardian_pattern((unsigned)id) == 1 && !(a->broken & GUARDIAN_COOLANT) &&
               nearest > 4) {
        Vec3 side = vcross(e->heading, v3(0, 1, 0));
        Vec3 move = vnorm(vadd(e->heading, vmul(side, a->attack % 2 ? 1.5 : -1.5)));
        bool grounded = true;
        world_move(&g->world, &e->pos, vmul(move, dt * (e->slow > 0 ? 2 : 4)), .7, 2, true,
                   &grounded);
    }
    return true;
}
