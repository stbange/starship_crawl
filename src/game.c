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
    return vadd(p->sheltered ? p->shelter_pos : position_absolute(p->pos),
                v3(0,
                   p->riding == MECH    ? 2.35
                   : p->riding == WAGON ? 1.25
                   : p->crouched        ? .85
                                        : 1.62,
                   0));
}
Vec3 enemy_center(const Enemy *e) {
    return vadd(position_absolute(e->pos), v3(0, .65, 0));
}
void game_flashlight(Game *g, int player) {
    if (player >= 0 && player < g->player_count)
        g->players[player].flashlight = !g->players[player].flashlight;
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
    p->flashlight = true;
    p->weapons = SC_START_WEAPONS;
    p->shield = 50;
    p->armor = 25;
    p->energy = 100;
}
void game_init(Game *g, uint32_t seed) {
    memset(g, 0, sizeof(*g));
    world_init(&g->world, seed);
    story_init(&g->story);
    g->player_count = 1;
    for (int i = 0; i < SC_PLAYERS; i++)
        player_init(&g->players[i]);
    game_equipment_reset(g);
    Vec3 p = position_absolute(g->players[0].pos);
    world_refresh(&g->world, &p, 1);
    game_message(g, story_commission(seed));
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
    p->sheltered = g->players[0].sheltered;
    p->shelter_pos = g->players[0].shelter_pos;
    p->return_yaw = g->players[0].return_yaw;
    p->return_pitch = g->players[0].return_pitch;
    p->resting = 0;
    p->riding = 0;
    p->drive_speed = 0;
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
    g->players[1].riding = 0;
    g->players[1].drive_speed = 0;
    g->players[1].sheltered = false;
    g->players[1].resting = 0;
    g->player_count = 1;
    g->dirty = true;
    game_message(g, "SOLO EXPLORATION");
}
void game_new_ship(Game *g, uint32_t seed) {
    world_destroy(&g->world);
    world_init(&g->world, seed);
    memset(g->projectiles, 0, sizeof(g->projectiles));
    g->mission = g->mission_kills = g->mission_loot = g->contracts = 0;
    story_init(&g->story);
    memset(&g->shelter, 0, sizeof(g->shelter));
    for (int i = 0; i < SC_PLAYERS; i++) {
        Player *p = &g->players[i];
        p->pos = position_from(v3(i * 1.4, 4, -34));
        p->checkpoint = position_from(v3(0, .02, 12));
        p->velocity = v3(0, 0, 0);
        p->yaw = 0;
        p->pitch = -.04;
        p->health = player_max_health(p);
        p->shield = player_max_shield(p);
        p->armor = player_max_armor(p);
        p->energy = player_max_energy(p);
        p->grounded = false;
        p->crouched = false;
        p->slow = p->hurt = p->cooldown = 0;
        p->journal = false;
        p->decision_open = false;
        p->journal_page = p->story_reading = 0;
        p->map_open = p->cloaked = false;
        p->flight = p->leap = p->nova = p->nova_tick = p->nova_flash = 0;
        p->sheltered = false;
        p->resting = p->nourished = 0;
    }
    game_equipment_reset(g);
    Vec3 positions[2] = {position_absolute(g->players[0].pos),
                         position_absolute(g->players[1].pos)};
    world_refresh(&g->world, positions, g->player_count);
    g->dirty = true;
    game_message(g, "NEW SHIP / YOUR CHARACTER ENDURES");
}
bool game_upgrade(Game *g, int player, int upgrade) {
    if (player < 0 || player >= g->player_count || upgrade < 0 || upgrade >= SC_ATTRIBUTES)
        return false;
    Player *p = &g->players[player];
    uint32_t *attribute = player_attribute(p, upgrade);
    unsigned cost = 60 + *attribute * 25;
    if (!p->points && p->credits < cost)
        return false;
    if (*attribute >= 50)
        return false;
    (*attribute)++;
    if (p->points)
        p->points--;
    else
        p->credits -= cost;
    p->health = player_max_health(p);
    g->dirty = true;
    p->shield = player_max_shield(p);
    p->armor = player_max_armor(p);
    p->energy = player_max_energy(p);
    game_message(g, sc_attributes[upgrade]);
    return true;
}
static void award(Game *g, Player *p, unsigned xp) {
    p->kills++;
    g->mission_kills++;
    p->credits += 5 + 2 * p->mods[6];
    p->xp += xp;
    while (p->xp >= player_next_level(p) && p->level < 1000) {
        p->xp -= player_next_level(p);
        p->level++;
        p->points++;
        p->health = player_max_health(p);
        p->shield = player_max_shield(p);
        p->armor = player_max_armor(p);
        p->energy = player_max_energy(p);
        game_message(g, "ASCENSION / OPEN JOURNAL WITH I / CHOOSE AN UPGRADE");
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
static void damage_enemy(Game *g, Room *room, int index, int owner, float damage, uint32_t flags) {
    Enemy *e = &room->enemies[index];
    if (!e->alive)
        return;
    const EnemyDef *def = &sc_enemies[e->kind];
    if (def->flags & ARMORED)
        damage *= .72f;
    if (def->flags & WARD) {
        Vec3 toward = vnorm(position_delta(g->players[owner].pos, e->pos));
        if (vdot(toward, e->heading) > .6 && e->windup <= 0)
            damage *= .5f;
    }
    e->health -= damage;
    e->flash = .12f;
    e->last_player = (uint8_t)owner;
    if (flags & W_CHILL)
        e->slow = 3;
    if (flags & W_BURN)
        e->burn = 3;
    if (flags & W_LEECH) {
        Player *p = &g->players[owner];
        p->health = fminf(player_max_health(p), p->health + damage * .12f);
    }
    g->hits++;
    if (e->health <= 0) {
        world_kill(&g->world, room, index);
        award(g, &g->players[owner],
              20 + def->tier * 5 + (unsigned)fmin(world_difficulty(room->cell), 30));
    }
}
static void blast_enemies(Game *g, Vec3 pos, int owner, float damage, double radius,
                          uint32_t flags) {
    for (int r = 0; r < g->world.count; r++)
        for (int i = 0; i < g->world.rooms[r].enemy_count; i++) {
            Enemy *e = &g->world.rooms[r].enemies[i];
            Vec3 diff = vsub(enemy_center(e), pos);
            double distance = vlen(diff);
            if (e->alive && distance < radius &&
                world_raycast(&g->world, pos, vnorm(diff), distance) >= distance - .1)
                damage_enemy(g, &g->world.rooms[r], i, owner,
                             damage * (float)(1 - .65 * distance / radius), flags);
        }
}
void game_nova(Game *g, int player) {
    if (player < 0 || player >= g->player_count ||
        (g->players[player].sheltered || g->players[player].riding))
        return;
    Player *p = &g->players[player];
    blast_enemies(g, player_eye(p), player, 14 * (1 + .2f * p->power), 7, 0);
}
static void beam_shot(Game *g, Player *p, const WeaponDef *w, Vec3 origin);
static void shoot(Game *g, Player *p) {
    const WeaponDef *w = game_gun(p);
    if (p->energy < w->cost) {
        p->cooldown = .2f;
        game_message(g, "ENERGY RECHARGING / Z OR X SWITCHES WEAPONS");
        return;
    }
    int free = 0;
    for (int i = 0; i < SC_MAX_PROJECTILES; i++)
        free += !g->projectiles[i].active;
    if (!(w->flags & W_BEAM) && free < w->pellets) {
        p->cooldown = .15f;
        return;
    }
    p->energy -= w->cost;
    p->cooldown = w->interval / (1 + .10f * p->haste);
    p->muzzle = .075f;
    p->cloaked = false;
    g->shots++;
    int owner = (int)(p - g->players), slot = 0;
    Vec3 origin = vadd(player_eye(p), v3(cos(p->yaw) * .16, -.13, -sin(p->yaw) * .16));
    if (w->flags & W_BEAM) {
        beam_shot(g, p, w, origin);
        return;
    }
    for (int pellet = 0; pellet < w->pellets; pellet++) {
        uint32_t hash = world_hash(g->shots, p->pos.sector, (uint32_t)pellet * 31 + owner);
        double spread = w->spread / (1 + .12 * p->mods[3]);
        Vec3 dir = facing(p->yaw + ((int)(hash % 2001) - 1000) * spread / 1000,
                          p->pitch + ((int)((hash >> 12) % 2001) - 1000) * spread / 1000);
        float damage = w->damage * (1 + .35f * p->power);
        if (hash % 100 < 3 + p->mods[3] * 2)
            damage *= 1.75f;
        while (g->projectiles[slot].active)
            slot++;
        float speed = game_shot_speed(p);
        g->projectiles[slot] = (Projectile){.pos = position_from(origin),
                                            .velocity = vmul(dir, speed),
                                            .life = w->range / speed,
                                            .damage = damage,
                                            .active = true,
                                            .owner = owner,
                                            .flags = w->flags,
                                            .blast = w->blast,
                                            .color = w->color,
                                            .pierce = w->pierce};
    }
}
static void move_player(Game *g, Player *p, const Actions *a, double dt) {
    bool riding = p->riding != ON_FOOT;
    bool outside = !riding && position_absolute(p->pos).z < 0, flying = !riding && p->flight > 0;
    if (riding)
        p->crouched = false;
    else if (a->crouch)
        p->crouched = true;
    else if (!world_overlap(&g->world, position_absolute(p->pos), .34, 1.78))
        p->crouched = false;
    Vec3 forward = (outside || flying) ? facing(p->yaw, p->pitch) : v3(sin(p->yaw), 0, cos(p->yaw));
    Vec3 right = v3(cos(p->yaw), 0, -sin(p->yaw));
    Vec3 wish = vadd(vmul(forward, a->forward), vmul(right, a->strafe));
    if (outside || flying)
        wish.y += a->vertical + (a->jump ? 1 : 0) - (a->crouch ? 1 : 0);
    if (vlen(wish) > 1)
        wish = vnorm(wish);
    double speed = outside ? 14 : (p->crouched ? 3.4 : (a->sprint ? 11 : 7.5));
    if (riding)
        speed = p->riding == MECH ? (a->sprint ? 8 : 5.5) : 20;
    if (p->riding == WAGON && a->crouch)
        wish = v3(0, 0, 0);
    if (!riding && !outside && !flying && p->leap > 0 && !p->grounded)
        speed *= 1.65;
    speed *= (1 + .025 * p->mods[2]) * (p->slow > 0 ? .55 : 1);
    if (outside || flying)
        p->velocity = vmul(wish, speed);
    else {
        Room *room = world_find(&g->world, world_cell(position_absolute(p->pos)));
        double gravity = room && room->type == GARDEN ? 9 : 22;
        double blend = p->riding == WAGON ? fmin(1, dt * 6) : 1;
        p->velocity.x += (wish.x * speed - p->velocity.x) * blend;
        p->velocity.z += (wish.z * speed - p->velocity.z) * blend;
        if (p->grounded) {
            p->velocity.y = 0;
            if (!riding && a->jump && !p->crouched)
                p->velocity.y = p->leap > 0 ? 13 : 7.5;
        }
        p->velocity.y = fmax(-30, p->velocity.y - gravity * dt);
    }
    p->grounded = false;
    double before_y = p->pos.local.y;
    Position before = p->pos;
    Vec3 delta = vmul(p->velocity, dt);
    if (riding && position_absolute(p->pos).z + delta.z < .8)
        delta.z = fmax(0, .8 - position_absolute(p->pos).z);
    move_body(&g->world, &p->pos, delta, game_body_radius(p), game_body_height(p), &p->grounded);
    if (game_rig_blocks(g, (int)(p - g->players), position_absolute(p->pos), game_body_radius(p),
                        game_body_height(p)))
        p->pos = before;
    Vec3 travelled = position_delta(p->pos, before);
    travelled.y = 0;
    p->drive_speed = (float)(vlen(travelled) / dt);
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
        if (riding &&
            world_overlap(&g->world, world_lift(cell), game_body_radius(p), game_body_height(p))) {
            game_message(g, "LIFT DESTINATION TOO SMALL / DISMOUNT FIRST");
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
/* All enemy shots are finite-lived world objects, with a swept collision test.
   A bright charge locks aim before release, so movement can avoid every shot. */
static double ray_sphere(Vec3 origin, Vec3 dir, Vec3 center, double radius, double limit) {
    Vec3 oc = vsub(origin, center);
    double b = vdot(oc, dir), c = vdot(oc, oc) - radius * radius, disc = b * b - c;
    if (c <= 0)
        return 0;
    if (disc < 0)
        return limit;
    double t = -b - sqrt(disc);
    return t >= 0 && t < limit ? t : limit;
}
static double player_intercept(const Player *p, Vec3 origin, Vec3 dir, double limit) {
    Vec3 feet = position_absolute(p->pos);
    double radius = p->riding ? game_body_radius(p) : .53;
    double lo = feet.y + (p->riding ? radius : .34),
           hi = feet.y + (p->riding     ? game_body_height(p) - radius
                          : p->crouched ? .65
                                        : 1.4);
    double best = ray_sphere(origin, dir, v3(feet.x, lo, feet.z), radius, limit);
    best = ray_sphere(origin, dir, v3(feet.x, hi, feet.z), radius, best);
    double x = origin.x - feet.x, z = origin.z - feet.z, a = dir.x * dir.x + dir.z * dir.z;
    double b = x * dir.x + z * dir.z, c = x * x + z * z - radius * radius;
    if (c <= 0 && origin.y >= lo && origin.y <= hi)
        return 0;
    double disc = b * b - a * c;
    if (a > 1e-12 && disc >= 0) {
        double t = (-b - sqrt(disc)) / a, y = origin.y + t * dir.y;
        if (t >= 0 && t < best && y >= lo && y <= hi)
            best = t;
    }
    return best;
}
static bool vulnerable(const Player *p) {
    Vec3 feet = position_absolute(p->pos);
    return !p->sheltered && feet.z >= 0 && world_difficulty(world_cell(feet)) != 0 && p->health > 0;
}
static bool already_hit(const Projectile *shot, Cell room, int id) {
    for (int i = 0; i < shot->hit_count; i++)
        if (shot->hit_ids[i] == id && cell_equal(shot->hit_rooms[i], room))
            return true;
    return false;
}
static void beam_shot(Game *g, Player *p, const WeaponDef *w, Vec3 origin) {
    Vec3 dir = facing(p->yaw, p->pitch), ray = origin, end = origin;
    double left = w->range;
    int owner = (int)(p - g->players);
    Projectile history = {0};
    for (int pass = 0; pass < w->pierce && pass < 4; pass++) {
        double nearest = world_raycast(&g->world, ray, dir, left);
        int target = -1;
        Room *room = NULL;
        for (int r = 0; r < g->world.count; r++)
            for (int e = 0; e < g->world.rooms[r].enemy_count; e++) {
                Enemy *enemy = &g->world.rooms[r].enemies[e];
                if (!enemy->alive || already_hit(&history, g->world.rooms[r].cell, e))
                    continue;
                double hit = ray_sphere(ray, dir, enemy_center(enemy),
                                        fmax(.45, sc_enemies[enemy->kind].size * .92), nearest);
                if (hit < nearest) {
                    nearest = hit;
                    target = e;
                    room = &g->world.rooms[r];
                }
            }
        end = vadd(ray, vmul(dir, nearest));
        if (target < 0)
            break;
        damage_enemy(g, room, target, owner, w->damage * (1 + .35f * p->power), w->flags);
        int at = history.hit_count++;
        history.hit_rooms[at] = room->cell;
        history.hit_ids[at] = (uint8_t)target;
        left -= nearest + .01;
        ray = vadd(end, vmul(dir, .01));
        if (left <= 0)
            break;
    }
    Vec3 muzzle =
        vadd(origin, v3(cos(p->yaw) * (p->riding == MECH ? .65 : .3), p->riding == MECH ? -.5 : -.2,
                        -sin(p->yaw) * (p->riding == MECH ? .65 : .3)));
    g->beams[owner] = (Beam){position_from(muzzle), position_from(end), w->color, .14f};
}
static void projectiles_step(Game *g, double dt) {
    for (int i = 0; i < SC_MAX_PROJECTILES; i++) {
        Projectile *shot = &g->projectiles[i];
        if (!shot->active)
            continue;
        Vec3 dir = vnorm(shot->velocity);
        double travel = vlen(shot->velocity) * fmin(dt, fmax(0, shot->life));
        shot->life -= (float)dt;
        for (int pass = 0; pass < 4 && shot->active; pass++) {
            Vec3 origin = position_absolute(shot->pos);
            double wall = world_raycast(&g->world, origin, dir, travel), nearest = wall;
            int player = -1, target = -1;
            Room *room = NULL;
            if (shot->owner < 0) {
                for (int p = 0; p < g->player_count; p++)
                    if (vulnerable(&g->players[p])) {
                        double t = player_intercept(&g->players[p], origin, dir, nearest);
                        if (t < nearest) {
                            nearest = t;
                            player = p;
                        }
                    }
            } else if (!(shot->flags & W_FLARE)) {
                for (int r = 0; r < g->world.count; r++)
                    for (int e = 0; e < g->world.rooms[r].enemy_count; e++) {
                        Enemy *enemy = &g->world.rooms[r].enemies[e];
                        if (!enemy->alive || already_hit(shot, g->world.rooms[r].cell, e))
                            continue;
                        double t =
                            ray_sphere(origin, dir, enemy_center(enemy),
                                       fmax(.45, sc_enemies[enemy->kind].size * .92), nearest);
                        if (t < nearest) {
                            nearest = t;
                            target = e;
                            room = &g->world.rooms[r];
                        }
                    }
            }
            position_move(&shot->pos, vmul(dir, nearest));
            bool impact = wall < travel || player >= 0 || target >= 0;
            if (shot->owner >= 0 && (shot->flags & W_FLARE) && wall < travel)
                game_attach_flare(g, shot->owner, position_absolute(shot->pos), dir);
            if (player >= 0) {
                game_hurt(g, player, shot->damage, shot->flags);
                g->projectile_hits++;
            }
            if (target >= 0 && !(shot->flags & W_BLAST)) {
                damage_enemy(g, room, target, shot->owner, shot->damage, shot->flags);
                if (shot->flags & W_CHAIN)
                    blast_enemies(g, enemy_center(&room->enemies[target]), shot->owner,
                                  shot->damage * .55f, shot->blast, shot->flags & ~W_CHAIN);
            }
            if (shot->owner >= 0 && impact && (shot->flags & W_BLAST))
                blast_enemies(g, vsub(position_absolute(shot->pos), vmul(dir, .05)), shot->owner,
                              shot->damage, shot->blast, shot->flags);
            Cell cell = world_cell(position_absolute(shot->pos));
            bool pierces =
                target >= 0 && !(shot->flags & W_BLAST) && shot->pierce > 1 && shot->hit_count < 4;
            if (pierces) {
                int at = shot->hit_count++;
                shot->hit_rooms[at] = room->cell;
                shot->hit_ids[at] = (uint8_t)target;
                shot->pierce--;
                travel -= nearest + .01;
                position_move(&shot->pos, vmul(dir, .01));
            }
            if ((impact && !pierces) || shot->life <= 0 || !world_ready(&g->world, cell) ||
                (shot->owner < 0 && world_difficulty(cell) == 0))
                shot->active = false;
            if (!pierces || travel <= 0)
                break;
        }
    }
}
static void spit(Game *g, const Room *room, Enemy *e) {
    const EnemyDef *def = &sc_enemies[e->kind];
    Vec3 center = enemy_center(e), direction = vnorm(vsub(e->aim, center));
    int count = (def->flags & BURST) ? 5 : 1;
    for (int n = 0; n < count; n++) {
        double angle = (n - (count - 1) * .5) * .14;
        Vec3 dir = vnorm(v3(direction.x * cos(angle) - direction.z * sin(angle), direction.y,
                            direction.x * sin(angle) + direction.z * cos(angle)));
        if (world_raycast(&g->world, center, dir, .9) < .9)
            continue;
        for (int i = 0; i < SC_MAX_PROJECTILES; i++)
            if (!g->projectiles[i].active) {
                g->projectiles[i] = (Projectile){
                    .pos = position_from(vadd(center, vmul(dir, .9))),
                    .velocity = vmul(dir, e->kind == 7 ? 19 : 12),
                    .life = 5,
                    .damage = def->damage + (float)fmin(world_difficulty(room->cell), 12),
                    .active = true,
                    .owner = -1,
                    .flags = def->flags,
                    .color = (def->flags & CHILL) ? rgb(.25f, .65f, 1)
                                                  : ((def->flags & DRAIN) ? rgb(.7f, .2f, .8f)
                                                                          : rgb(.32f, .9f, .38f))};
                g->enemy_shots++;
                break;
            }
    }
}
static Vec3 nearby_wall(const World *w, const Enemy *e) {
    Vec3 center = enemy_center(e), normal = v3(0, 0, 0);
    double best = 1.1;
    const Vec3 dirs[] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};
    for (int d = 0; d < 4; d++) {
        double hit = world_raycast(w, center, dirs[d], best);
        if (hit < best && hit > .25) {
            best = hit;
            normal = vmul(dirs[d], -1);
        }
    }
    return normal;
}
static void move_crawler(Game *g, Room *room, Enemy *e, Vec3 direction, double distance,
                         double dt) {
    Vec3 before = position_absolute(e->pos);
    Vec3 wall = nearby_wall(&g->world, e);
    double floor = room->cell.y * SC_DECK;
    if (!e->climbing && vlen(wall) > .5 && e->cooldown < .9 && distance > 4 && room->height > 5) {
        e->climbing = true;
        e->normal = wall;
        e->climb_time = 2.5f;
        e->vertical_speed = 0;
    }
    if (e->climbing) {
        e->climb_time -= (float)dt;
        if (vlen(wall) < .5 || e->climb_time <= 0 || distance < 3 ||
            before.y > floor + fmin(room->height - 2, 5.5)) {
            e->climbing = false;
            e->normal = v3(0, 1, 0);
            e->vertical_speed = -2;
            e->cooldown = 1.2f;
        } else {
            e->normal = wall;
            Vec3 along = vsub(direction, vmul(wall, vdot(direction, wall)));
            if (vlen(along) < .1)
                along = vcross(v3(0, 1, 0), wall);
            Vec3 delta = vadd(vmul(vnorm(along), 3 * dt), v3(0, 3.8 * dt, 0));
            /* Gentle adhesion keeps the body near a real surface. */
            delta = vsub(delta, vmul(wall, .5 * dt));
            move_body(&g->world, &e->pos, delta, .42, 1.25, NULL);
            e->heading = vnorm(delta);
            return;
        }
    }
    double speed =
        (sc_enemies[e->kind].speed + 1.5 * sin(g->time * 5 + e->id)) * (e->slow > 0 ? .4 : 1);
    if (distance > 1.15) {
        Vec3 delta = vmul(direction, speed * dt);
        move_body(&g->world, &e->pos, delta, .42, 1.25, NULL);
        if (vlen(vsub(position_absolute(e->pos), before)) < vlen(delta) * .4) {
            Vec3 side = vcross(direction, v3(0, 1, 0));
            if (e->id % 2)
                side = vmul(side, -1);
            move_body(&g->world, &e->pos, vmul(side, speed * .7 * dt), .42, 1.25, NULL);
        }
    }
    bool grounded = false;
    e->vertical_speed = fmaxf(-16, e->vertical_speed - (float)(22 * dt));
    move_body(&g->world, &e->pos, v3(0, e->vertical_speed * dt, 0), .42, 1.25, &grounded);
    if (grounded)
        e->vertical_speed = 0;
    e->heading = direction;
}
static void enemies_step(Game *g, double dt) {
    bool protected[SC_PLAYERS] = {false, false};
    for (int p = 0; p < g->player_count; p++)
    protected[p] = game_story_safe(g, position_absolute(g->players[p].pos));
    for (int r = 0; r < g->world.count; r++) {
        Room *room = &g->world.rooms[r];
        for (int i = 0; i < room->enemy_count; i++) {
            Enemy *e = &room->enemies[i];
            if (!e->alive)
                continue;
            if (game_story_inert(g, e)) {
                e->windup = 0;
                continue;
            }
            const EnemyDef *def = &sc_enemies[e->kind];
            e->flash = fmaxf(0, e->flash - (float)dt);
            e->cooldown = fmaxf(0, e->cooldown - (float)dt);
            e->slow = fmaxf(0, e->slow - (float)dt);
            if (e->burn > 0) {
                e->burn -= dt;
                damage_enemy(g, room, i, e->last_player, (float)dt * 7, 0);
                if (!e->alive)
                    continue;
            }
            Player *target = NULL;
            double best = 55;
            for (int p = 0; p < g->player_count; p++) {
                Player *t = &g->players[p];
                if (!vulnerable(t) || t->cloaked || protected[p] ||
                    fabs(position_delta(t->pos, e->pos).y) > 9)
                    continue;
                double distance = vlen(position_delta(t->pos, e->pos));
                if (distance < best) {
                    best = distance;
                    target = t;
                }
            }
            if (!target) {
                e->windup = 0;
                continue;
            }
            double maxhealth = def->health + world_difficulty(room->cell) * 5;
            if ((def->flags & REGENERATE) && e->flash <= 0)
                e->health = fminf((float)maxhealth, e->health + (float)dt * 5);
            e->special -= (float)dt;
            Vec3 diff = position_delta(target->pos, e->pos);
            diff.y = 0;
            Vec3 direction = vnorm(diff);
            if ((def->flags & STRAFE) && def->role == 0 && best > 5)
                direction = vnorm(vadd(direction, vmul(vcross(direction, v3(0, 1, 0)),
                                                       sin(g->time * 2 + e->id) * .65)));
            e->heading = direction;
            Vec3 to = vsub(player_eye(target), enemy_center(e));
            bool sight =
                world_raycast(&g->world, enemy_center(e), vnorm(to), vlen(to)) >= vlen(to) - .1;
            double speed = def->speed * (e->slow > 0 ? .4 : 1);
            if ((def->flags & HEALER) && e->special <= 0) {
                for (int j = 0; j < room->enemy_count; j++) {
                    Enemy *ally = &room->enemies[j];
                    if (ally->alive && j != i && vlen(position_delta(ally->pos, e->pos)) < 14)
                        ally->health =
                            fminf(sc_enemies[ally->kind].health + world_difficulty(room->cell) * 5,
                                  ally->health + 18);
                }
                e->special = 3;
                e->flash = .3f;
            }
            if ((def->flags & BROOD) && e->special <= 0 && room->enemy_count < SC_MAX_ENEMIES) {
                int id = room->enemy_count;
                Position spawn = e->pos;
                position_move(&spawn, v3(1.5, 0, 1.5));
                if (!world_overlap(&g->world, position_absolute(spawn), .42, 1.25)) {
                    room->enemies[id] = (Enemy){.pos = spawn,
                                                .health = 24,
                                                .kind = 3,
                                                .id = (uint8_t)id,
                                                .alive = true,
                                                .normal = {0, 1, 0},
                                                .heading = {0, 0, 1}};
                    if (room->record == SIZE_MAX ||
                        !(g->world.records[room->record].killed & (1u << id)))
                        room->enemy_count++;
                }
                e->special = 5;
                e->flash = .3f;
            }
            if (def->flags & HOVER) {
                double floor = room->cell.y * SC_DECK;
                double wanted = floor + 1.5 + sin(g->time * 2 + e->id) * .5;
                move_body(&g->world, &e->pos,
                          v3(0, clampd(wanted - position_absolute(e->pos).y, -2 * dt, 2 * dt), 0),
                          .42, 1.25, NULL);
            }
            if (def->role == 1) {
                if (e->windup > 0) {
                    e->windup = fmaxf(0, e->windup - (float)dt);
                    if (e->windup <= 0) {
                        spit(g, room, e);
                        e->cooldown = def->interval;
                    }
                } else if (best < (e->kind == 7 ? 48 : 30) && best > 3 && e->cooldown <= 0 &&
                           sight) {
                    e->aim = player_eye(target);
                    e->windup = .8f;
                } else if (!(def->flags & TURRET)) {
                    if (best > 16 || !sight)
                        move_body(&g->world, &e->pos, vmul(direction, speed * dt), .42, 1.25, NULL);
                    else if (best < 7)
                        move_body(&g->world, &e->pos, vmul(direction, -speed * dt), .42, 1.25,
                                  NULL);
                    else if (def->flags & STRAFE)
                        move_body(
                            &g->world, &e->pos,
                            vmul(vcross(direction, v3(0, 1, 0)), sin(g->time + e->id) * speed * dt),
                            .42, 1.25, NULL);
                }
            } else {
                if ((def->flags & CHARGE) && best > 4 && best < 24 && sight && e->special <= 0) {
                    e->aim = direction;
                    e->windup = .6f;
                    e->special = 4;
                }
                if ((def->flags & CHARGE) && e->windup > 0)
                    e->windup = fmaxf(0, e->windup - (float)dt);
                else if ((def->flags & CHARGE) && e->special > 2.5f)
                    move_body(&g->world, &e->pos, vmul(e->aim, 13 * dt * (e->slow > 0 ? .4 : 1)),
                              .42, 1.25, NULL);
                else if (def->flags & CLIMB)
                    move_crawler(g, room, e, direction, best, dt);
                else {
                    if ((def->flags & LEAP) && e->special <= 0 && best > 3 && best < 15 && sight) {
                        e->vertical_speed = 7;
                        e->special = 3;
                    }
                    if (best > 1.4)
                        move_body(&g->world, &e->pos, vmul(direction, speed * dt), .42, 1.25, NULL);
                    bool grounded = false;
                    e->vertical_speed = fmaxf(-16, e->vertical_speed - 22 * (float)dt);
                    move_body(&g->world, &e->pos, v3(0, e->vertical_speed * dt, 0), .42, 1.25,
                              &grounded);
                    if (grounded)
                        e->vertical_speed = 0;
                }
                if ((def->flags & DETONATE) && best < 3 && sight) {
                    game_hurt(g, (int)(target - g->players),
                              def->damage + (float)world_difficulty(room->cell), 0);
                    damage_enemy(g, room, i, (int)(target - g->players), 10000, 0);
                } else if (best < (def->role == 2 ? 2.8 : 1.7) && e->cooldown <= 0 && sight) {
                    game_hurt(g, (int)(target - g->players), def->damage, def->flags);
                    e->cooldown = def->interval;
                }
            }
        }
    }
}
void game_step(Game *g, const Actions *actions, double dt) {
    if (!isfinite(dt) || dt <= 0 || dt > .05)
        return;
    g->time += dt;
    game_equipment_step(g, dt);
    g->message_time = fmaxf(0, g->message_time - (float)dt);
    Vec3 positions[2] = {position_absolute(g->players[0].pos),
                         position_absolute(g->players[1].pos)};
    if (!world_refresh(&g->world, positions, g->player_count))
        return;
    for (int i = 0; i < g->player_count; i++) {
        Player *p = &g->players[i];
        game_psionics_step(g, p, dt);
        if (!p->sheltered && position_absolute(p->pos).z >= 0 &&
            !world_ready(&g->world, world_cell(position_absolute(p->pos))))
            continue;
        p->cooldown = fmaxf(0, p->cooldown - (float)dt);
        p->muzzle = fmaxf(0, p->muzzle - (float)dt);
        p->hurt = fmaxf(0, p->hurt - (float)dt);
        if (p->hurt <= 0 && p->health > 0)
            p->health =
                fminf(player_max_health(p),
                      p->health + (float)dt * (3 + 1.5f * p->mods[4] + (p->nourished > 0 ? 3 : 0)));
        if (p->sheltered) {
            game_shelter_step(g, p, &actions[i], dt);
            continue;
        }
        game_story_recovery(g, p, dt);
        move_player(g, p, &actions[i], dt);
        if (p->riding) {
            g->rigs[i][p->riding - 1].pos = p->pos;
            g->rigs[i][p->riding - 1].yaw = p->yaw;
        }
        if (actions[i].fire && p->cooldown <= 0)
            shoot(g, p);
    }
    game_progression_step(g, dt);
    game_story_step(g);
    enemies_step(g, dt);
    projectiles_step(g, dt);
    for (int i = 0; i < g->player_count; i++) {
        Player *p = &g->players[i];
        if (p->health <= 0 || position_absolute(p->pos).z < -400) {
            p->riding = 0;
            p->drive_speed = 0;
            p->pos = p->checkpoint;
            p->velocity = v3(0, 0, 0);
            p->health = player_max_health(p);
            p->shield = player_max_shield(p);
            p->armor = player_max_armor(p);
            p->energy = player_max_energy(p);
            p->hurt = 0;
            p->slow = 0;
            p->cloaked = false;
            p->flight = p->leap = p->nova = p->nova_tick = p->nova_flash = 0;
            p->crouched = true;
            for (int j = 0; j < SC_MAX_PROJECTILES; j++)
                if (vlen(position_delta(g->projectiles[j].pos, p->pos)) < 12)
                    g->projectiles[j].active = false;
            g->respawns++;
            g->dirty = true;
            game_message(g, "RECONSTITUTED / YOUR PROGRESS IS SAFE");
        }
    }
}
