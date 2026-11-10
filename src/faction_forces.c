#include "game.h"
#include <string.h>

FactionForce *factions_force(Game *g, uint32_t id) {
    if (id)
        for (unsigned i = 0; i < SC_FACTION_FORCES; i++)
            if (g->factions.forces[i].id == id)
                return &g->factions.forces[i];
    return NULL;
}
unsigned factions_strength(const FactionForce *a) {
    unsigned n = 0;
    for (unsigned i = 0; i < a->count; i++)
        n += a->members[i].alive;
    return n;
}
bool factions_launch(Game *g, unsigned country, ForceOrder order, Cell source, Cell goal) {
    if (!g->factions.version || country < 1 || country > 3 || order <= ORDER_NONE ||
        order >= FORCE_ORDERS || factions_safe(g, source) || factions_safe(g, goal) ||
        (g->world.open_layer && (campaign_layer(goal) > g->world.open_layer ||
                                 campaign_layer(source) > g->world.open_layer)) ||
        factions_observed(g, vadd(world_center(source), v3(0, 1, 6)), 32))
        return false;
    FactionForce *a = NULL;
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++)
        if (!g->factions.forces[i].id) {
            a = &g->factions.forces[i];
            break;
        }
    FactionNation *n = &g->factions.nations[country - 1];
    const FactionRegion *destination = factions_at(&g->factions, goal);
    unsigned resource =
        destination && destination->kind >= SITE_POWER ? destination->kind - SITE_POWER : 0;
    unsigned cargo = order == ORDER_ESCORT || order == ORDER_REINFORCE ? 12 : 0;
    if (!a || n->supplies[0] < 3 || n->supplies[1] < 3 || n->supplies[2] < 6 ||
        n->supplies[resource] < cargo + (resource == 2 ? 6u : 3u) ||
        g->factions.next_id == UINT32_MAX)
        return false;
    *a = (FactionForce){.id = g->factions.next_id++,
                        .country = country,
                        .order = order,
                        .home = source,
                        .goal = goal,
                        .next = source,
                        .count = SC_FORCE_MEMBERS,
                        .cargo = cargo,
                        .generation = g->factions.tick};
    unsigned gov = politics_government(g->world.seed, country - 1);
    for (unsigned i = 0; i < a->count; i++) {
        unsigned kind = gov == ECHO_PARLIAMENT ? (i == 0 ? 28 : 27)
                        : gov == MOLTING_COURT ? (i == 0 ? 23 : 3)
                                               : (i == 0 ? 24 : 6);
        a->members[i] = (Enemy){
            .force = a->id,
            .member = (uint8_t)i,
            .kind = (uint8_t)kind,
            .country = (uint8_t)country,
            .alive = true,
            .normal = {0, 1, 0},
            .heading = {0, 0, 1},
            .health = sc_enemies[kind].health + (float)fmin(campaign_layer(source), 8) * 10,
            .pos = position_from(vadd(world_center(source), v3((i - 1.0) * .9, .02, 6 + i * 1.8)))};
    }
    n->supplies[0] -= 3;
    n->supplies[1] -= 3;
    n->supplies[2] -= 6;
    n->supplies[resource] -= cargo;
    g->factions.launched++;
    factions_log(g, FE_LAUNCH, country, a->id, source, order);
    return true;
}
void factions_forces_sync(Game *g, bool detach) {
    if (!g->factions.version)
        return;
    for (int r = 0; r < g->world.count; r++) {
        Room *room = &g->world.rooms[r];
        for (int i = 0; i < room->enemy_count;) {
            Enemy *e = &room->enemies[i];
            if (!e->force) {
                i++;
                continue;
            }
            FactionForce *a = factions_force(g, e->force);
            if (a && e->member < a->count) {
                a->members[e->member] = *e;
                if (e->alive)
                    a->present |= 1u << e->member;
                else
                    a->present &= ~(1u << e->member);
            }
            if (detach) {
                memmove(e, e + 1, (size_t)(room->enemy_count - i - 1) * sizeof(*e));
                room->enemy_count--;
            } else
                i++;
        }
    }
}
static bool near_force(const Game *g, const FactionForce *a) {
    for (unsigned m = 0; m < a->count; m++)
        if (a->members[m].alive) {
            for (int p = 0; p < g->player_count; p++)
                if (!g->players[p].sheltered &&
                    vlen(position_delta(a->members[m].pos, g->players[p].pos)) < 300 &&
                    (world_ready(&g->world, world_cell(position_absolute(a->members[m].pos))) ||
                     vlen(position_delta(a->members[m].pos, g->players[p].pos)) < 150))
                    return true;
            for (int s = 0; s < SC_MAX_PROJECTILES; s++)
                if (g->projectiles[s].active) {
                    Vec3 delta = position_delta(a->members[m].pos, g->projectiles[s].pos);
                    if (vlen(delta) < 120 && vdot(delta, g->projectiles[s].velocity) > 0)
                        return true;
                }
        }
    return false;
}
static void casualty(Game *g, FactionForce *a, Enemy *e) {
    if (!e->alive)
        return;
    e->alive = false;
    e->health = 0;
    g->factions.casualties++;
    g->factions.nations[a->country - 1].losses++;
    FactionRegion *r = factions_record(g, world_cell(position_absolute(e->pos)));
    if (r) {
        r->casualties++;
        r->flags |= FS_RAIDED;
    }
    factions_log(g, FE_LOSS, a->country, a->id, world_cell(position_absolute(e->pos)), e->member);
}
void factions_damage(Game *g, Room *r, int index, float damage, int player) {
    if (!r || index < 0 || index >= r->enemy_count || !isfinite(damage) || damage <= 0)
        return;
    Enemy *e = &r->enemies[index];
    if (!e->alive || e->allegiance == 2 || (player < 0 && factions_safe(g, r->cell)))
        return;
    FactionForce *a = factions_force(g, e->force);
    if (player >= 0 && player < SC_PLAYERS && a) {
        unsigned dealt = (unsigned)fmin(e->health, damage);
        a->participation[player] = (uint32_t)fmin(1000000, a->participation[player] + dealt);
    }
    e->health -= damage;
    e->flash = .18f;
    if (e->health <= 0) {
        if (a)
            casualty(g, a, e);
        else if (!factions_safe(g, r->cell))
            world_kill(&g->world, r, index);
        if (!e->alive)
            game_enemy_death(g, e, player);
    }
    if (a && e->member < a->count) {
        a->members[e->member] = *e;
        if (!factions_strength(a))
            for (int p = 0; p < g->player_count; p++)
                if (a->participation[p] >= 80 && !(a->claimed & (1u << p))) {
                    a->claimed |= 1u << p;
                    player_add_salvage(&g->players[p],
                                       3); /* Ordinary salvage, never an ability or phase key. */
                }
    }
    g->dirty = true;
}
static unsigned leader(const FactionForce *a) {
    for (unsigned m = 0; m < a->count; m++)
        if (a->members[m].alive)
            return m;
    return 0;
}
static bool allowed(const Game *g, Cell c) {
    return c.z >= 0 && world_difficulty(c) &&
           (!g->world.open_layer || campaign_layer(c) <= g->world.open_layer);
}
static void retreat(Game *g, FactionForce *a) {
    if (a->order == ORDER_RETREAT)
        return;
    a->order = ORDER_RETREAT;
    a->goal = a->home;
    a->wait = 0;
    memset(a->nav_time, 0, sizeof(a->nav_time));
    g->factions.retreats++;
    factions_log(g, FE_RETREAT, a->country, a->id,
                 world_cell(position_absolute(a->members[leader(a)].pos)), 0);
}
static void arrive(Game *g, FactionForce *a) {
    if (a->arrivals & 0x80000000u)
        return;
    if (a->order != ORDER_RETREAT && factions_safe(g, a->goal)) {
        retreat(g, a);
        return;
    }
    if (a->order == ORDER_OCCUPY && a->arrivals)
        return;
    if (chronicle_arrival(g, a->id))
        return;
    if (a->physical && a->order == ORDER_RAID) {
        Room *room = world_find(&g->world, a->goal);
        if (!room || !room->ready)
            return;
        for (int i = 0; i < room->enemy_count; i++) {
            Enemy *e = &room->enemies[i];
            if (e->alive && e->force != a->id && !e->allegiance &&
                (!e->country || factions_relation(g, a->country, e->country) == REL_WAR))
                return;
        }
    }
    FactionRegion *r = factions_record(g, a->goal);
    if (!r) {
        a->wait = 2;
        return;
    }
    a->arrivals++;
    g->factions.arrived++;
    factions_log(g, FE_ARRIVE, a->country, a->id, a->goal, a->order);
    if (a->order == ORDER_RETREAT) {
        a->arrivals |= 0x80000000u;
        g->factions.nations[a->country - 1].supplies[2] += factions_strength(a);
        a->wait = 8; /* Surviving troops disperse only when nobody observes them. */
    } else if (a->order == ORDER_ESCORT || a->order == ORDER_REINFORCE) {
        unsigned resource = r->kind >= SITE_POWER ? r->kind - SITE_POWER : 0;
        g->factions.nations[a->country - 1].supplies[resource] += a->cargo;
        r->flags = (r->flags | FS_REPAIRED) & ~FS_RAIDED;
        a->cargo = 0;
        retreat(g, a);
    } else if (a->order == ORDER_RAID) {
        if (r->owner && r->owner != a->country &&
            factions_relation(g, a->country, r->owner) == REL_WAR) {
            unsigned resource = r->kind >= SITE_POWER ? r->kind - SITE_POWER : 0;
            FactionNation *victim = &g->factions.nations[r->owner - 1];
            unsigned stolen = victim->supplies[resource] < 9 ? victim->supplies[resource] : 9;
            victim->supplies[resource] -= stolen;
            g->factions.nations[a->country - 1].supplies[resource] += stolen;
            r->owner = a->country;
            r->epoch++;
            r->flags |= FS_CAPTURED | FS_RAIDED;
            r->flags |= politics_government(g->world.seed, a->country - 1) == MOLTING_COURT
                            ? FS_GROWTH
                            : FS_DARK;
            g->factions.captures++;
            factions_log(g, FE_CAPTURE, a->country, a->id, a->goal, stolen);
        }
        a->order = ORDER_OCCUPY;
        a->wait = 30;
    } else if (a->order == ORDER_PATROL)
        retreat(g, a);
}
static bool coarse_route(Game *g, FactionForce *a) {
    Enemy *e = &a->members[leader(a)];
    Navigation nav;
    if (!game_navigation_from(g, e->pos, false, a->goal, &nav) || nav.kind == NAV_BLOCKED ||
        !allowed(g, nav.next))
        return false;
    Cell from = world_cell(position_absolute(e->pos));
    if (cell_equal(from, a->goal)) {
        const FactionRegion *region = factions_at(&g->factions, a->goal);
        if (a->order == ORDER_RAID && region && region->owner && region->owner != a->country &&
            factions_relation(g, a->country, region->owner) == REL_WAR &&
            !factions_safe(g, a->goal)) {
            /* Three distant exchanges with the station garrison. These are casualties,
               not a second physical fight: the same static kill mask is retained. */
            if (a->siege < 3) {
                a->siege++;
                e->health -= 18;
                if (e->health <= 0)
                    casualty(g, a, e);
                if (factions_strength(a) <= 1)
                    retreat(g, a);
                return true;
            }
            Room station = {
                .cell = world_describe_version(g->world.seed, g->world.generator, a->goal).cell,
                .record = SIZE_MAX,
                .enemy_count = 2};
            world_kill(&g->world, &station, 0);
            world_kill(&g->world, &station, 1);
            if (g->world.failed || station.record == SIZE_MAX)
                return false;
        }
        arrive(g, a);
        return true;
    }
    /* A coarse arrival must not materialize in either camera or projectile path. */
    if (factions_observed(g, vadd(world_center(nav.next), v3(0, 1, 7)), 32))
        return false;
    for (unsigned m = 0; m < a->count; m++)
        if (a->members[m].alive)
            a->members[m].pos =
                position_from(vadd(world_center(nav.next), v3((m - 1.0) * .9, .02, 6 + m * 1.8)));
    return true;
}
static void lift_move(FactionForce *a, double dt) {
    a->route_time = fmaxf(0, a->route_time - (float)dt);
    double progress = 1 - a->route_time / 3.0;
    double y = a->waypoint.y + (a->next.y * SC_DECK + .02 - a->waypoint.y) * progress;
    for (unsigned m = 0; m < a->count; m++)
        if (a->members[m].alive)
            a->members[m].pos = position_from(v3(a->waypoint.x + (m - 1.0) * .7, y, a->waypoint.z));
}
void factions_forces_tick(Game *g) {
    Factions *f = &g->factions;
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        FactionForce *a = &f->forces[i];
        if (!a->id)
            continue;
        if (!factions_strength(a)) {
            memset(a, 0, sizeof(*a));
            continue;
        }
        a->physical = near_force(g, a);
        if (a->physical)
            continue;
        if (a->route_time > 0) {
            lift_move(a, 2);
            continue;
        }
        if (a->wait > 0) {
            a->wait = fmaxf(0, a->wait - 2);
            continue;
        }
        if (a->order == ORDER_OCCUPY)
            retreat(g, a);
        if (a->order == ORDER_RETREAT && (a->arrivals & 0x80000000u) &&
            cell_equal(world_cell(position_absolute(a->members[leader(a)].pos)), a->home)) {
            memset(a, 0, sizeof(*a));
            continue;
        }
        a->age += 2;
        if (a->age >= 20) {
            a->age = 0;
            if (!coarse_route(g, a)) {
                a->stuck += 20;
                if (a->stuck >= 60)
                    retreat(g, a);
            } else
                a->stuck = 0;
        }
        for (unsigned j = i + 1; j < SC_FACTION_FORCES; j++) {
            if (!factions_strength(a))
                break;
            FactionForce *b = &f->forces[j];
            if (!b->id || b->physical || near_force(g, b) || !factions_strength(b) ||
                factions_relation(g, a->country, b->country) != REL_WAR ||
                !cell_equal(world_cell(position_absolute(a->members[leader(a)].pos)),
                            world_cell(position_absolute(b->members[leader(b)].pos))))
                continue;
            Enemy *x = &a->members[leader(a)], *y = &b->members[leader(b)];
            float da = 3.f * factions_strength(b), db = 3.f * factions_strength(a);
            x->health -= da;
            y->health -= db;
            if (x->health <= 0)
                casualty(g, a, x);
            if (y->health <= 0)
                casualty(g, b, y);
            if (factions_strength(a) == 1)
                retreat(g, a);
            if (factions_strength(b) == 1)
                retreat(g, b);
        }
    }
}
static unsigned occupied(const Game *g, const Room *room) {
    unsigned count = (unsigned)room->enemy_count;
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        const FactionForce *a = &g->factions.forces[i];
        if (!a->id)
            continue;
        for (unsigned m = 0; m < a->count; m++)
            if (a->members[m].alive && (a->present & (1u << m))) {
                const Room *here = world_find((World *)&g->world,
                                              world_cell(position_absolute(a->members[m].pos)));
                const Room *landing =
                    a->route_time > 0 ? world_find((World *)&g->world, a->next) : NULL;
                if (here == room || landing == room)
                    count++;
            }
    }
    return count;
}
static bool lift_clear(const Game *g, const FactionForce *a, Cell from, Cell to) {
    if (!world_ready(&g->world, from) || !world_ready(&g->world, to) || !allowed(g, to))
        return false;
    const Room *landing = world_find((World *)&g->world, to);
    if (!landing || occupied(g, landing) + factions_strength(a) > SC_MAX_ENEMIES)
        return false;
    for (int p = 0; p < g->player_count; p++)
        if (!g->players[p].sheltered &&
            (vlen(vsub(position_absolute(g->players[p].pos), world_lift(from))) < 4 ||
             vlen(vsub(position_absolute(g->players[p].pos), world_lift(to))) < 4))
            return false;
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        const FactionForce *b = &g->factions.forces[i];
        if (a != b && b->id && b->route_time > 0 &&
            (cell_equal(b->next, from) || cell_equal(b->next, to)))
            return false;
    }
    return true;
}
static bool tactics(Game *g, FactionForce *a, Enemy *e, double dt) {
    if (factions_safe(g, world_cell(position_absolute(e->pos))))
        return false;
    Vec3 eye = enemy_center(e), aim = {0};
    double best = 35;
    int player = -1;
    Enemy *victim = NULL;
    for (int p = 0; p < g->player_count; p++) {
        const Player *t = &g->players[p];
        if (t->sheltered || t->cloaked || t->health <= 0 || !factions_hostile(g, e, p) ||
            game_story_safe(g, position_absolute(t->pos)) || matrix_protected(g, p))
            continue;
        Vec3 delta = vsub(player_eye(t), eye);
        if (vlen(delta) < best &&
            world_raycast(&g->world, eye, vnorm(delta), vlen(delta)) >= vlen(delta) - .1) {
            best = vlen(delta);
            aim = player_eye(t);
            player = p;
        }
    }
    for (int r = 0; r < g->world.count; r++) {
        Room *room = &g->world.rooms[r];
        if (!room->ready || vlen(vsub(world_room_center(room), eye)) > 210 ||
            factions_safe(g, room->cell))
            continue;
        for (int i = 0; i < room->enemy_count; i++) {
            Enemy *other = &room->enemies[i];
            if (!other->alive || other->force == a->id || other->allegiance ||
                (other->country && factions_relation(g, e->country, other->country) != REL_WAR))
                continue;
            Vec3 delta = vsub(enemy_center(other), eye);
            if (vlen(delta) < best &&
                world_raycast(&g->world, eye, vnorm(delta), vlen(delta)) >= vlen(delta) - .1) {
                best = vlen(delta);
                aim = enemy_center(other);
                victim = other;
                player = -1;
            }
        }
    }
    if (!victim && player < 0) {
        e->windup = 0;
        return false;
    }
    e->heading = vnorm(vsub(aim, eye));
    e->warning_sound = .3f;
    if (e->cooldown <= 0) {
        if (e->windup <= 0) {
            e->windup = .7f;
            e->aim = aim;
        } else {
            e->windup -= (float)dt;
            if (e->windup <= 0) {
                for (int j = 0; j < SC_MAX_PROJECTILES; j++)
                    if (!g->projectiles[j].active) {
                        Vec3 dir = vnorm(vsub(e->aim, eye));
                        g->projectiles[j] =
                            (Projectile){.active = true,
                                         .owner = -1,
                                         .country = (uint8_t)a->country,
                                         .source_force = a->id,
                                         .pos = position_from(vadd(eye, vmul(dir, .85))),
                                         .velocity = vmul(dir, 14),
                                         .life = 3,
                                         .damage = 14,
                                         .color = politics_color(g->world.seed, a->country - 1)};
                        g->enemy_shots++;
                        break;
                    }
                e->cooldown = 1.8f;
            }
        }
    }
    if (best > 18 && a->order != ORDER_RETREAT) {
        Vec3 delta = e->heading;
        delta.y = 0;
        world_move(&g->world, &e->pos, vmul(vnorm(delta), 2.4 * dt), .38, 1.25, true, NULL);
    }
    return a->order != ORDER_RETREAT;
}
void factions_forces_step(Game *g, double dt) {
    if (!factions_active(g))
        return;
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        FactionForce *a = &g->factions.forces[i];
        if (!a->id)
            continue;
        a->physical = near_force(g, a);
        if (!a->physical)
            a->present = 0;
    }
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        FactionForce *a = &g->factions.forces[i];
        if (!a->id || !factions_strength(a))
            continue;
        if (!a->physical)
            continue;
        Enemy *lead = &a->members[leader(a)];
        Cell from = world_cell(position_absolute(lead->pos));
        if (a->wait > 0)
            a->wait = fmaxf(0, a->wait - (float)dt);
        if ((a->arrivals & 0x80000000u) && !a->wait) {
            bool observed = false;
            for (unsigned m = 0; m < a->count; m++)
                if (a->members[m].alive)
                    observed |= factions_observed(g, enemy_center(&a->members[m]), 80);
            if (!observed) {
                memset(a, 0, sizeof(*a));
                continue;
            }
            a->wait = 1;
        }
        if (a->order == ORDER_OCCUPY && !a->wait)
            retreat(g, a);
        bool lifting = a->route_time > 0;
        if (lifting) {
            if (!world_ready(&g->world, a->next)) {
                a->wait = 1;
                continue;
            }
            lift_move(a, dt);
        }
        for (unsigned m = 0; m < a->count; m++) {
            Enemy *e = &a->members[m];
            if (!e->alive)
                continue;
            Room *room = world_find(&g->world, world_cell(position_absolute(e->pos)));
            if (!room || !room->ready)
                continue;
            if (!(a->present & (1u << m))) {
                if (occupied(g, room) >= SC_MAX_ENEMIES ||
                    factions_observed(g, enemy_center(e), 32))
                    continue;
                a->present |= 1u << m;
            }
            Position before = e->pos;
            e->cooldown = fmaxf(0, e->cooldown - (float)dt);
            e->flash = fmaxf(0, e->flash - (float)dt);
            e->slow = fmaxf(0, e->slow - (float)dt);
            e->warning_sound = fmaxf(0, e->warning_sound - (float)dt);
            if (!e->windup && !lifting && !a->wait && e->allegiance != 1 && e->allegiance != 3) {
                a->nav_time[m] -= (float)dt;
                Cell cell = world_cell(position_absolute(e->pos));
                if (a->nav_time[m] <= 0 || !cell_equal(cell, a->route_from[m])) {
                    Navigation nav;
                    game_navigation_from(g, e->pos, false, a->goal, &nav);
                    /* Prefer the service stair for ascending processions. Collision handles ramps
                       and individual treads; the cached waypoint follows the same landing guide. */
                    Vec3 local = vsub(position_absolute(e->pos), world_center(cell));
                    if ((nav.kind == NAV_LIFT || (nav.kind == NAV_LANDING && local.y < .1)) &&
                        a->goal.y > cell.y && world_service_column(g->world.seed, cell) &&
                        (a->id % 3 == 0 || !lift_clear(g, a, cell, nav.next))) {
                        nav.kind = NAV_LANDING;
                        nav.point = vadd(world_center(cell), v3(6, .02, -31));
                        if (fabs(local.x - 6) < .65 && local.z < -29)
                            nav.point = vadd(world_center(cell), v3(6, 8, -9));
                    }
                    a->route_point[m] = nav.point;
                    a->route_from[m] = nav.from;
                    a->route_next[m] = nav.next;
                    a->nav_kind[m] = (uint8_t)nav.kind;
                    a->nav_time[m] = .55f;
                }
                Navigation nav = {.point = a->route_point[m],
                                  .next = a->route_next[m],
                                  .kind = (NavKind)a->nav_kind[m]};
                Vec3 at = position_absolute(e->pos);
                if (nav.kind == NAV_GOAL && vlen(vsub(at, nav.point)) < 8 && m == leader(a))
                    arrive(g, a);
                else if (nav.kind == NAV_LIFT && vlen(vsub(at, nav.point)) < 1.7 &&
                         m == leader(a) && lift_clear(g, a, from, nav.next)) {
                    bool assembled = true;
                    for (unsigned n = 0; n < a->count; n++)
                        if (a->members[n].alive &&
                            (!(a->present & (1u << n)) ||
                             vlen(vsub(position_absolute(a->members[n].pos), nav.point)) > 3.5))
                            assembled = false;
                    if (assembled) {
                        a->route_time = 3;
                        a->next = nav.next;
                        a->waypoint = world_lift(from);
                    }
                } else if (nav.kind != NAV_BLOCKED && allowed(g, nav.next)) {
                    Vec3 delta = vsub(nav.point, at);
                    delta.y = 0;
                    if (vlen(delta) > .4) {
                        e->heading = vnorm(delta);
                        world_move(&g->world, &e->pos,
                                   vmul(e->heading, dt * (e->slow > 0 ? 1.3 : 4.2)), .38, 1.25,
                                   true, NULL);
                        if (vlen(vsub(at, position_absolute(e->pos))) < dt * .5) {
                            Vec3 side = vcross(e->heading, v3(0, 1, 0));
                            world_move(&g->world, &e->pos, vmul(side, dt * (m % 2 ? 2 : -2)), .38,
                                       1.25, true, NULL);
                            if (m == leader(a))
                                a->stuck += (float)dt;
                        } else if (m == leader(a))
                            a->stuck = 0;
                    }
                }
                bool grounded = false;
                e->vertical_speed = fmaxf(-16, e->vertical_speed - (float)dt * 22);
                world_move(&g->world, &e->pos, v3(0, e->vertical_speed * dt, 0), .38, 1.25, true,
                           &grounded);
                if (grounded)
                    e->vertical_speed = 0;
                if (a->stuck > 12)
                    retreat(g, a);
            }
            Room *destination = world_find(&g->world, world_cell(position_absolute(e->pos)));
            if (destination != room && (!destination || occupied(g, destination) > SC_MAX_ENEMIES))
                e->pos = before;
        }
    }
    /* All leases are assigned before proxies are appended. Existing actors keep their
       slots, and arrivals wait at the doorway instead of evicting an observed soldier. */
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        FactionForce *a = &g->factions.forces[i];
        if (!a->id || !a->physical)
            continue;
        for (unsigned m = 0; m < a->count; m++) {
            Enemy *e = &a->members[m];
            if (!e->alive || !(a->present & (1u << m)))
                continue;
            Room *room = world_find(&g->world, world_cell(position_absolute(e->pos)));
            if (room && room->ready && room->enemy_count < SC_MAX_ENEMIES) {
                e->id = (uint8_t)room->enemy_count;
                room->enemies[room->enemy_count++] = *e;
            }
        }
    }
}
void factions_tactics(Game *g, double dt) {
    if (!factions_active(g))
        return;
    for (int r = 0; r < g->world.count; r++) {
        Room *room = &g->world.rooms[r];
        for (int i = 0; i < room->enemy_count; i++) {
            Enemy *e = &room->enemies[i];
            FactionForce *a = factions_force(g, e->force);
            if (!a || !e->alive || a->route_time > 0)
                continue;
            if (matrix_control_enemy(g, room, e, dt))
                continue;
            if (e->burn > 0) {
                e->burn = fmaxf(0, e->burn - (float)dt);
                game_damage_enemy(g, room, i, e->last_player, (float)dt * 7, 0);
                if (!e->alive)
                    continue;
            }
            tactics(g, a, e, dt);
        }
    }
}
bool factions_feral(Game *g, Room *room, Enemy *e, double dt) {
    if (factions_safe(g, room->cell))
        return false;
    Vec3 eye = enemy_center(e);
    double best = 24;
    for (int p = 0; p < g->player_count; p++)
        if (!g->players[p].sheltered && !g->players[p].cloaked)
            best = fmin(best, vlen(vsub(player_eye(&g->players[p]), eye)));
    Room *victim_room = NULL;
    int victim = -1;
    for (int r = 0; r < g->world.count; r++) {
        Room *candidate = &g->world.rooms[r];
        if (!candidate->ready || vlen(vsub(world_room_center(candidate), eye)) > 160 ||
            factions_safe(g, candidate->cell))
            continue;
        for (int n = 0; n < candidate->enemy_count; n++) {
            Enemy *other = &candidate->enemies[n];
            if (!other->alive || !other->country || other->allegiance)
                continue;
            Vec3 delta = vsub(enemy_center(other), eye);
            double distance = vlen(delta);
            if (distance < best &&
                world_raycast(&g->world, eye, vnorm(delta), distance) >= distance - .1) {
                best = distance;
                victim = n;
                victim_room = candidate;
            }
        }
    }
    if (victim < 0)
        return false;
    Vec3 delta = vsub(enemy_center(&victim_room->enemies[victim]), eye);
    e->heading = vnorm(delta);
    e->cooldown = fmaxf(0, e->cooldown - (float)dt);
    if (e->cooldown <= 0 && best < 2.8) {
        factions_damage(g, victim_room, victim, sc_enemies[e->kind].damage, -1);
        e->cooldown = fmaxf(.6f, sc_enemies[e->kind].interval);
        e->warning_sound = .4f;
    } else if (e->cooldown <= 0 && sc_enemies[e->kind].role == 1) {
        if (e->windup <= 0) {
            e->windup = .65f;
            e->aim = enemy_center(&victim_room->enemies[victim]);
        } else {
            e->windup -= (float)dt;
            if (e->windup <= 0) {
                for (int i = 0; i < SC_MAX_PROJECTILES; i++)
                    if (!g->projectiles[i].active) {
                        Vec3 dir = vnorm(vsub(e->aim, eye));
                        g->projectiles[i] =
                            (Projectile){.active = true,
                                         .owner = -1,
                                         .country = 4,
                                         .pos = position_from(vadd(eye, vmul(dir, .9))),
                                         .velocity = vmul(dir, 12),
                                         .life = 3,
                                         .damage = sc_enemies[e->kind].damage,
                                         .color = sc_enemies[e->kind].color};
                        g->enemy_shots++;
                        break;
                    }
                e->cooldown = 1.5f;
            }
        }
    }
    if (best > 2.4 && !e->windup) {
        delta.y = 0;
        world_move(&g->world, &e->pos, vmul(vnorm(delta), sc_enemies[e->kind].speed * dt), .4, 1.25,
                   true, NULL);
    }
    bool grounded = false;
    e->vertical_speed = fmaxf(-16, e->vertical_speed - (float)dt * 22);
    world_move(&g->world, &e->pos, v3(0, e->vertical_speed * dt, 0), .4, 1.25, true, &grounded);
    if (grounded)
        e->vertical_speed = 0;
    return true;
}
