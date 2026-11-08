#include "game.h"
#include <string.h>

/* Seven regions around each body; work is independent of the number of saved regions. */
unsigned factions_interest(Game *g, FactionRegion **sites, unsigned capacity) {
    unsigned count = 0;
    for (int p = 0; p < g->player_count; p++) {
        if (g->players[p].sheltered)
            continue;
        Cell cell = world_cell(position_absolute(g->players[p].pos)), r = world_region(cell);
        const int dx[] = {0, -1, 1, 0, 0, 0, 0}, dy[] = {0, 0, 0, 0, 0, -1, 1},
                  dz[] = {0, 0, 0, -1, 1, 0, 0};
        for (unsigned i = 0; i < 7; i++) {
            Cell at = {(r.x + dx[i]) * 4, r.y + dy[i], (r.z + dz[i]) * 4};
            FactionRegion *site = (FactionRegion *)factions_at(&g->factions, at);
            if (!site || !site->kind || !site->owner)
                continue;
            bool duplicate = false;
            for (unsigned j = 0; j < count; j++)
                duplicate |= sites[j] == site;
            if (!duplicate && count < capacity)
                sites[count++] = site;
        }
    }
    return count;
}
static uint32_t random_next(Factions *f) {
    f->random = f->random * 1664525u + 1013904223u;
    return f->random;
}
static unsigned total(const FactionNation *n) {
    return n->supplies[0] + n->supplies[1] + n->supplies[2];
}
static void relations(Game *g, unsigned a) {
    Factions *f = &g->factions;
    FactionNation *n = &f->nations[a];
    if (f->tick < n->next_relation)
        return;
    n->next_relation = f->tick + 120 + random_next(f) % 60;
    unsigned b = (a + 1 + random_next(f) % 2) % 3;
    unsigned relation;
    if (total(n) < 18 || total(&f->nations[b]) < 18)
        relation = REL_PEACE;
    else if (n->losses >= 6 * (n->ruler + 1)) {
        n->ruler++;
        n->cycle = 0;
        relation = REL_TRIBUTE;
        factions_log(g, FE_SUCCESSION, a + 1, 0, (Cell){0}, n->ruler);
    } else
        relation = random_next(f) % 4;
    if (f->relations[a][b] == relation)
        return;
    f->relations[a][b] = f->relations[b][a] = relation;
    f->nations[b].next_relation = n->next_relation;
    factions_log(g, FE_RELATION, a + 1, 0, (Cell){0}, b * 4 + relation);
}
static bool target(Game *g, FactionRegion **sites, unsigned count, unsigned country, unsigned gov,
                   FactionRegion *source, ForceOrder *order, Cell *goal) {
    Factions *f = &g->factions;
    FactionNation *n = &f->nations[country - 1];
    double best = -1;
    for (unsigned i = 0; i < count; i++) {
        FactionRegion *site = sites[i];
        if (site == source || factions_safe(g, site->site) ||
            (g->world.open_layer && campaign_layer(site->site) > g->world.open_layer))
            continue;
        unsigned relation = factions_relation(g, country, site->owner);
        ForceOrder candidate = ORDER_NONE;
        double score = 0;
        double distance = fabs((double)site->region.x - source->region.x) +
                          fabs((double)site->region.y - source->region.y) +
                          fabs((double)site->region.z - source->region.z);
        if (site->owner != country && relation == REL_WAR) {
            candidate = ORDER_RAID;
            unsigned resource = site->kind >= SITE_POWER ? site->kind - SITE_POWER : 2;
            score = 40 + (100 - fmin(n->supplies[resource], 100)) * .3;
            if (gov == MOLTING_COURT && site->kind == SITE_BIOMASS)
                score += 25;
            if (gov == ECHO_PARLIAMENT && site->kind == SITE_POWER)
                score += 25;
        } else if (site->owner == country || relation == REL_SYMBIOSIS || relation == REL_TRIBUTE) {
            candidate = site->flags & FS_RAIDED ? ORDER_REINFORCE : ORDER_ESCORT;
            score = 20 + ((site->flags & FS_RAIDED) ? 35 : 0);
            if (gov == MOLTING_COURT && n->cycle % 3 == 2)
                score += 45;
        }
        if (gov == DIVIDED_CROWN && (f->next_id & 1))
            score += site->region.y > source->region.y ? 18 : -18;
        score -= distance * 5;
        if (candidate && score > best) {
            best = score;
            *order = candidate;
            *goal = site->site;
        }
    }
    if (best >= 0)
        return true;
    for (unsigned i = 0; i < 4; i++) {
        Direction d = (Direction)((i + f->tick + country) % 4);
        Cell next = world_neighbor(source->site, d);
        if (world_door_version(g->world.seed, g->world.generator, source->site, d) && next.z >= 0 &&
            !factions_safe(g, next) &&
            (!g->world.open_layer || campaign_layer(next) <= g->world.open_layer)) {
            *order = ORDER_PATROL;
            *goal = next;
            return true;
        }
    }
    return false;
}
void factions_orders(Game *g) {
    Factions *f = &g->factions;
    FactionRegion *sites[14];
    unsigned count = factions_interest(g, sites, 14);
    if (f->tick % 10 == 0)
        for (unsigned i = 0; i < count; i++) {
            FactionRegion *r = sites[i];
            if (r->kind < SITE_POWER || r->systems & SYSTEM_BLACKOUT)
                continue;
            unsigned *s = &f->nations[r->owner - 1].supplies[r->kind - SITE_POWER];
            *s = (unsigned)fmin(300, *s + ((r->flags & FS_RAIDED) ? 1 : 4));
        }
    for (unsigned c = 0; c < 3; c++) {
        FactionNation *n = &f->nations[c];
        relations(g, c);
        if (f->tick % 30 == 0) {
            n->cycle++;
            for (unsigned b = c + 1; b < 3; b++)
                if (f->relations[c][b] >= REL_TRIBUTE) {
                    unsigned resource = (c + b + f->tick / 30) % 3;
                    unsigned *from = &n->supplies[resource],
                             *to = &f->nations[b].supplies[resource];
                    if (f->relations[c][b] == REL_SYMBIOSIS && *to > *from) {
                        unsigned *swap = from;
                        from = to;
                        to = swap;
                    }
                    if (*from > 3 && *to < 297) {
                        *from -= 3;
                        *to += 3;
                    }
                }
        }
        if (f->tick < n->next_order)
            continue;
        unsigned gov = politics_government(g->world.seed, c);
        n->next_order = f->tick + (gov == ECHO_PARLIAMENT ? 15 : gov == MOLTING_COURT ? 24 : 18);
        unsigned active = 0;
        for (unsigned i = 0; i < SC_FACTION_FORCES; i++)
            active += f->forces[i].id && f->forces[i].country == c + 1;
        if (active >= 8)
            continue;
        for (unsigned k = 0; k < count; k++) {
            FactionRegion *source = sites[(k + f->tick / 5) % count];
            if (source->owner != c + 1 ||
                (gov == ECHO_PARLIAMENT && source->systems & SYSTEM_SILENCED))
                continue;
            ForceOrder order = ORDER_PATROL;
            Cell goal = {0};
            if (!target(g, sites, count, c + 1, gov, source, &order, &goal))
                continue;
            f->decisions++;
            if (factions_launch(g, c + 1, order, source->site, goal))
                break;
        }
    }
}
void factions_attack(Game *g, const Enemy *e, int player, float damage) {
    if (!g->factions.version || !e->country || e->country > 3 || player < 0 ||
        player >= g->player_count || damage <= 0 || !e->alive)
        return;
    FactionIncident *in = &g->factions.incidents[player][e->country - 1];
    bool lethal = damage >= e->health;
    if (in->grace > 0 && !lethal)
        return;
    in->site = world_cell(position_absolute(e->pos));
    in->strikes = (uint32_t)fmin(20, in->strikes + (lethal ? 2 : 1));
    in->grace = .9f;
    if (in->strikes == 1) {
        game_message(g, "A SHARP WARNING CALL / THE GUARDS TURN TOWARDS YOU");
        in->local_time = 8;
    } else {
        in->local_time = 45;
        if (!in->transmit) {
            in->witness = e->force ? e->force : UINT32_MAX;
            in->member = e->member;
            in->transmit = 5;
        }
    }
    g->dirty = true;
}
void factions_incidents(Game *g, double dt) {
    for (int p = 0; p < SC_PLAYERS; p++)
        for (unsigned c = 0; c < 3; c++) {
            FactionIncident *in = &g->factions.incidents[p][c];
            in->grace = fmaxf(0, in->grace - (float)dt);
            in->local_time = fmaxf(0, in->local_time - (float)dt);
            if (in->transmit > 0) {
                in->transmit = fmaxf(0, in->transmit - (float)dt);
                if (in->transmit <= 0) {
                    FactionForce *a = factions_force(g, in->witness);
                    bool witness = a && factions_strength(a) > 0;
                    Room *room = world_find(&g->world, in->site);
                    if (in->witness == UINT32_MAX && room)
                        for (int i = 0; i < room->enemy_count; i++)
                            witness |= room->enemies[i].alive && room->enemies[i].country == c + 1;
                    if (witness && !(world_systems(&g->world, in->site) & SYSTEM_SILENCED)) {
                        in->reports = (uint32_t)fmin(3, in->reports + 1);
                        in->local_time = fmaxf(in->local_time, 60);
                    }
                    in->witness = 0;
                }
            }
            if (!in->local_time && !in->grace && !in->transmit) {
                in->strikes = 0;
                if (in->reports) {
                    in->reports--;
                    in->local_time = 60;
                }
            }
        }
}
void factions_systems(Game *g, Cell site, uint32_t systems, int player) {
    if (!g->factions.version || systems > 15 || player < 0 || player >= g->player_count)
        return;
    FactionRegion *r = factions_record(g, site);
    if (!r || r->systems == systems)
        return;
    r->systems = systems;
    r->flags |= systems & SYSTEM_BLACKOUT ? FS_DARK : FS_REPAIRED;
    if (systems & SYSTEM_POWER)
        r->flags &= ~FS_DARK;
    factions_log(g, FE_SYSTEM, r->owner, 0, site, systems);
    if (r->owner && systems & SYSTEM_SILENCED) {
        FactionNation *n = &g->factions.nations[r->owner - 1];
        n->next_order = g->factions.tick + 30;
        for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
            FactionForce *a = &g->factions.forces[i];
            if (a->id && a->country == r->owner && a->order != ORDER_RETREAT)
                a->wait = fmaxf(a->wait, 6);
        }
    }
}
