#include "game.h"
#include <stdlib.h>
#include <string.h>

static unsigned cell_bit(Cell cell) {
    Cell r = world_region(cell);
    return 1u << ((cell.z - r.z * 4) * 4 + cell.x - r.x * 4);
}
static bool exploring(const Game *g, int player) {
    return player >= 0 && player < g->player_count && !g->players[player].sheltered &&
           !g->matrix.divers[player].active && position_absolute(g->players[player].pos).z >= 0;
}
static const FactionEvent *experience(const Game *g, int player, unsigned back) {
    const Factions *f = &g->factions;
    if (back >= f->event_count || back >= SC_FACTION_EVENTS)
        return NULL;
    const FactionEvent *e = &f->events[(f->event_count - 1 - back) % SC_FACTION_EVENTS];
    return e->kind == FE_EXPERIENCE && (e->detail & 1u) == (unsigned)player &&
                   f->tick - e->tick <= 90
               ? e
               : NULL;
}
static bool repetition(const Game *g, int player) {
    unsigned matches = 0, signature = 0, country = 0;
    for (unsigned i = 0; i < SC_FACTION_EVENTS; i++) {
        const FactionEvent *e = experience(g, player, i);
        if (!e || !(e->detail & EXPERIENCE_CONTENT) || e->detail & EXPERIENCE_AVOIDED)
            continue;
        if (!matches) {
            signature = e->detail >> 4;
            country = e->country;
        }
        if (e->country == country && e->detail >> 4 == signature && ++matches >= 3)
            return true;
    }
    return false;
}
static bool avoided(const Game *g, int player, unsigned country, uint32_t force) {
    for (unsigned i = 0; i < SC_FACTION_EVENTS; i++) {
        const FactionEvent *e = experience(g, player, i);
        if (e && (e->detail & EXPERIENCE_AVOIDED) &&
            (force ? e->force == force : e->country == country))
            return true;
    }
    return false;
}
static bool eligible(Game *g, Cell site, unsigned kind, uint32_t force) {
    if ((kind == EDIT_CANCEL ? factions_required(g, site) : factions_safe(g, site)) ||
        (g->world.open_layer && campaign_layer(site) > g->world.open_layer) ||
        factions_observed(g, vadd(world_center(site), v3(0, 1, 0)), 70))
        return false;
    if ((kind == EDIT_NEST || kind == EDIT_SUBSTITUTE) &&
        world_describe_version(g->world.seed, g->world.generator, site).height < 5)
        return false;
    const FactionRegion *r = factions_at(&g->factions, site);
    bool known = false;
    if (r && r->seen) {
        RegionPlan plan;
        world_plan_version(g->world.seed, g->world.generator, r->region, &plan);
        unsigned local = (site.z - r->region.z * 4) * 4 + site.x - r->region.x * 4;
        for (unsigned i = 0; i < 16; i++)
            if ((r->seen & (1u << i)) && plan.owner[i] == plan.owner[local])
                known = true;
    }
    if (kind != EDIT_WITHDRAW && kind != EDIT_DELAY && kind != EDIT_ROUTE && r &&
        (known || r->edit))
        return false;
    if (force) {
        FactionForce *a = factions_force(g, force);
        if (!a || !factions_strength(a) || a->route_time > 0)
            return false;
        for (unsigned m = 0; m < a->count; m++)
            if (a->members[m].alive && (a->members[m].windup > 0 ||
                                        factions_observed(g, enemy_center(&a->members[m]), 48)))
                return false;
    }
    return true;
}
bool hidden_director_propose(Game *g, unsigned kind, int player, Cell site, uint32_t force) {
    HiddenDirector *d = &g->factions.director;
    if (onboarding_active(g) && player >= 0 && player < g->player_count &&
        (onboarding_opening(site) || g->drills[player].time > 0 || g->views[player].lesson ||
         (!g->players[player].kills && !g->campaign.defeated)))
        return false;
    if (!g->factions.version || !exploring(g, player) || kind <= EDIT_NONE ||
        kind >= DIRECTOR_EDITS || d->proposal.phase || !d->budget ||
        !eligible(g, site, kind, force))
        return false;
    const FactionRegion *site_record = factions_at(&g->factions, site);
    if ((kind == EDIT_ROUTE && avoided(g, player, 0, force)) ||
        (kind == EDIT_SCOUT && site_record && avoided(g, player, site_record->owner, 0)))
        return false;
    unsigned recent = 0;
    for (unsigned i = 0; i < 16; i++)
        recent += d->history[i] == kind;
    if (recent >= 4) {
        /* A quiet interval ages repetition memory instead of permanently exhausting it. */
        for (unsigned i = 0; i < SC_FACTION_EVENTS; i++) {
            const FactionEvent *e = &g->factions.events[i];
            if (e->kind == FE_DIRECTOR && g->factions.tick - e->tick < 60)
                return false;
        }
        if (g->factions.tick < 60)
            return false;
    }
    if (!factions_record(g, site))
        return false;
    d->proposal = (DirectionProposal){.kind = kind,
                                      .phase = 1,
                                      .player = (unsigned)player,
                                      .force = force,
                                      .cost = 1,
                                      .sequence = d->sequence + 1,
                                      .site = site,
                                      .remaining = 3};
    g->dirty = true;
    return true;
}
static bool commit(Game *g, DirectionProposal *p) {
    FactionRegion *r = (FactionRegion *)factions_at(&g->factions, p->site);
    FactionForce *a = factions_force(g, p->force);
    if (!r)
        return false;
    if (p->kind == EDIT_SCOUT) {
        if (!r->owner)
            return false;
        Cell goal = p->site;
        for (unsigned i = 0; i < 4; i++) {
            goal = world_neighbor(p->site, (Direction)i);
            if (world_door_version(g->world.seed, g->world.generator, p->site, (Direction)i) &&
                factions_launch(g, r->owner, ORDER_PATROL, p->site, goal))
                return true;
        }
        return false;
    }
    if (p->kind == EDIT_ROUTE || p->kind == EDIT_DELAY || p->kind == EDIT_WITHDRAW) {
        if (!a)
            return false;
        if (p->kind == EDIT_DELAY)
            a->wait = fmaxf(a->wait, 12);
        else {
            a->goal = p->kind == EDIT_WITHDRAW ? a->home : p->site;
            if (p->kind == EDIT_WITHDRAW) {
                a->order = ORDER_RETREAT;
                g->factions.retreats++;
                factions_log(g, FE_RETREAT, a->country, a->id, p->site, 1);
            }
            memset(a->nav_time, 0, sizeof(a->nav_time));
        }
    } else {
        r->edit = p->kind;
        r->edit_site = p->site;
        r->edit_claims = 0;
    }
    return true;
}
static void observations(Game *g, double dt) {
    HiddenDirector *d = &g->factions.director;
    for (int p = 0; p < g->player_count; p++) {
        if (!exploring(g, p))
            continue;
        Player *player = &g->players[p];
        Cell cell = world_cell(position_absolute(player->pos));
        FactionRegion *r = (FactionRegion *)factions_at(&g->factions, cell);
        if (r)
            r->seen |= cell_bit(cell);
        bool moved = !cell_equal(cell, d->last_room[p]);
        if (moved)
            d->last_room[p] = cell;
        float defenses = player->health + player->shield + player->armor;
        float loss = d->defenses[p] > 0 ? fmaxf(0, d->defenses[p] - defenses) : 0;
        d->defenses[p] = defenses;
        float threat = 0;
        const Enemy *subject = NULL;
        double closest = 35;
        for (int j = 0; j < g->world.count; j++) {
            Room *room = &g->world.rooms[j];
            if (vlen(vsub(world_room_center(room), position_absolute(player->pos))) > 200)
                continue;
            for (int e = 0; e < room->enemy_count; e++) {
                Enemy *enemy = &room->enemies[e];
                double distance = vlen(position_delta(enemy->pos, player->pos));
                Vec3 ray = vsub(enemy_center(enemy), player_eye(player));
                if (enemy->alive && distance < closest &&
                    world_raycast(&g->world, player_eye(player), vnorm(ray), vlen(ray)) >=
                        vlen(ray) - .1) {
                    subject = enemy;
                    closest = distance;
                }
                if (enemy->alive && enemy->allegiance != 2 && distance < 35 &&
                    (!enemy->country || factions_hostile(g, enemy, p)))
                    threat += .08f;
            }
        }
        float stress = (1 - player->health / player_max_health(player)) * .5f +
                       (1 - player->energy / player_max_energy(player)) * .15f + fminf(threat, .7f);
        d->pressure[p] =
            fminf(1, fmaxf(0, d->pressure[p] + (stress - d->pressure[p]) * (float)dt * .7f +
                                  loss * .012f));
        d->quiet[p] = fminf(600, d->quiet[p] + (threat > .1f ? -2 : 1) * (float)dt);
        d->quiet[p] = fmaxf(0, d->quiet[p]);
        bool sample = moved || (g->factions.tick && g->factions.tick % 5 == 0 &&
                                g->factions.remainder < (float)dt);
        if (sample) {
            const FactionEvent *previous = NULL;
            for (unsigned i = 0; i < SC_FACTION_EVENTS && !previous; i++)
                previous = experience(g, p, i);
            FactionEvent old = previous ? *previous : (FactionEvent){0};
            FactionForce *left = factions_force(g, old.force);
            if (moved && previous && (old.detail & EXPERIENCE_CONTENT) &&
                !(old.detail & EXPERIENCE_AVOIDED) && left && factions_strength(left) &&
                (!subject || subject->force != old.force))
                factions_log(g, FE_EXPERIENCE, old.country, old.force, old.site,
                             old.detail | EXPERIENCE_AVOIDED);
            const FactionForce *a = subject ? factions_force(g, subject->force) : NULL;
            unsigned detail = (unsigned)p | (threat > .1f ? EXPERIENCE_COMBAT : 0);
            if (subject)
                detail |= EXPERIENCE_CONTENT | (sc_enemy_bodies[subject->kind].form << 4) |
                          (a ? a->order << 8 : 0);
            factions_log(g, FE_EXPERIENCE, subject ? subject->country : 0,
                         subject ? subject->force : 0, cell, detail);
            if (moved && subject && !repetition(g, p))
                d->quiet[p] = fmaxf(0, d->quiet[p] - 12);
            unsigned began = g->factions.tick;
            for (unsigned i = 0; i < SC_FACTION_EVENTS; i++) {
                const FactionEvent *e = experience(g, p, i);
                if (!e)
                    continue;
                if (!(e->detail & EXPERIENCE_COMBAT))
                    break;
                began = e->tick;
            }
            if (threat > .1f && g->factions.tick - began >= 23)
                d->recovery[p] = 45;
        }
        if (d->pressure[p] > .65f || player->health < player_max_health(player) * .35f)
            d->recovery[p] = 45;
        else
            d->recovery[p] = fmaxf(0, d->recovery[p] - (float)dt);
        d->pace[p] = d->recovery[p] > 0      ? PACE_RECOVERY
                     : d->pressure[p] > .35f ? PACE_PRESSURE
                     : d->quiet[p] > 70      ? PACE_DISCOVERY
                                             : PACE_BUILDUP;
    }
}
void hidden_director_step(Game *g, double dt) {
    if (!factions_active(g))
        return;
    HiddenDirector *d = &g->factions.director;
    observations(g, dt);
    bool active = false;
    for (int p = 0; p < g->player_count; p++)
        active |= exploring(g, p);
    if (!active)
        return;
    d->recharge += (float)dt;
    if (d->recharge >= 50) {
        d->recharge -= 50;
        d->budget = d->budget < 3 ? d->budget + 1 : 3;
    }
    d->cooldown = fmaxf(0, d->cooldown - (float)dt);
    DirectionProposal *proposal = &d->proposal;
    if (proposal->phase) {
        proposal->remaining = fmaxf(0, proposal->remaining - (float)dt);
        if (proposal->remaining > 0)
            return;
        if (proposal->phase >= 3) {
            memset(proposal, 0, sizeof(*proposal));
            return;
        }
        bool pressure_added = proposal->kind == EDIT_NEST || proposal->kind == EDIT_SCOUT ||
                              proposal->kind == EDIT_SUBSTITUTE || proposal->kind == EDIT_ROUTE;
        if (!exploring(g, (int)proposal->player) || proposal->cost > d->budget ||
            (pressure_added && d->recovery[proposal->player] > 0) ||
            !eligible(g, proposal->site, proposal->kind, proposal->force) || !commit(g, proposal)) {
            proposal->phase = 4;
            d->rejected++;
            factions_log(g, FE_REJECT, 0, proposal->force, proposal->site, proposal->kind);
        } else {
            proposal->phase = 3;
            d->accepted++;
            d->budget -= proposal->cost;
            d->history[d->sequence % 16] = proposal->kind;
            d->sequence++;
            factions_log(g, FE_DIRECTOR, 0, proposal->force, proposal->site, proposal->kind);
        }
        proposal->remaining = 1;
        d->cooldown = 35;
        return;
    }
    if (d->cooldown > 0 || !d->budget)
        return;
    d->cooldown = 12; /* No eligible change is a valid decision. */
    int player = -1;
    for (int p = 0; p < g->player_count; p++)
        if (exploring(g, p) && (player < 0 || d->pressure[p] > d->pressure[player]))
            player = p;
    if (player < 0)
        return;
    bool recovery = d->pace[player] == PACE_RECOVERY;
    if (d->pace[player] == PACE_PRESSURE)
        return;
    unsigned kind = recovery ? EDIT_CANCEL
                    : d->pace[player] == PACE_DISCOVERY
                        ? EDIT_STASH
                        : (g->factions.tick % 4 == 0   ? EDIT_SCOUT
                           : g->factions.tick % 4 == 1 ? EDIT_WEATHER
                           : g->factions.tick % 4 == 2 ? EDIT_NEST
                                                       : EDIT_SUBSTITUTE);
    bool repeated = repetition(g, player);
    if (!recovery && repeated) {
        if (g->factions.tick % 2)
            return;
        kind = EDIT_STASH;
    }
    Cell cell = world_cell(position_absolute(g->players[player].pos));
    for (unsigned i = 0; !repeated && i < SC_FACTION_FORCES; i++) {
        FactionForce *a = &g->factions.forces[(i + g->factions.tick) % SC_FACTION_FORCES];
        if (!a->id || !factions_strength(a) || a->order == ORDER_RETREAT)
            continue;
        Cell at = world_cell(position_absolute(a->members[0].pos));
        if (abs(at.x - cell.x) + abs(at.y - cell.y) + abs(at.z - cell.z) > 4)
            continue;
        unsigned edit = recovery ? (g->factions.tick % 2 ? EDIT_WITHDRAW : EDIT_DELAY) : EDIT_ROUTE;
        if (hidden_director_propose(g, edit, player, at, a->id))
            return;
    }
    for (unsigned distance = 1; distance <= 3; distance++)
        for (unsigned j = 0; j < 4; j++) {
            Direction dir = (Direction)((j + g->factions.tick) % 4);
            Cell site = cell;
            for (unsigned k = 0; k < distance; k++)
                site = world_neighbor(site, dir);
            if (hidden_director_propose(g, kind, player, site, 0))
                return;
        }
}
void factions_environment(Game *g) {
    if (!g->factions.version)
        return;
    for (int i = 0; i < g->world.count; i++) {
        Room *room = &g->world.rooms[i];
        if (!room->ready)
            continue;
        FactionRegion *r = (FactionRegion *)factions_at(&g->factions, room->cell);
        if (!r)
            continue;
        for (int p = 0; p < g->player_count; p++)
            if (exploring(g, p) &&
                vlen(vsub(world_room_center(room), player_eye(&g->players[p]))) < 160 &&
                factions_observed(g, vadd(world_room_center(room), v3(0, 1, 0)), 10))
                r->seen |= cell_bit(room->cell);
        if (!r->edit ||
            !cell_equal(
                world_describe_version(g->world.seed, g->world.generator, r->edit_site).cell,
                room->cell))
            continue;
        if (room->faction_edit != r->edit) {
            if (r->edit == EDIT_CANCEL) {
                /* Unknown optional infestation is omitted at generation, never counted as slain. */
                for (int e = 0; e < room->enemy_count; e++)
                    if (room->habitat.pace != PACE_FRONTIER && !room->enemies[e].country &&
                        !room->enemies[e].force)
                        room->enemies[e].alive = false;
                room->hazard_stage = 0;
                room->hazard_time = 60;
            } else if (r->edit == EDIT_NEST && room->enemy_count <= 14) {
                unsigned count = 14;
                for (unsigned e = count; e < count + 2; e++) {
                    Enemy *enemy = &room->enemies[e];
                    *enemy = (Enemy){.pos = position_from(vadd(world_center(room->cell),
                                                               v3((e - count) * 2 - 1.0, .02, 12))),
                                     .id = (uint8_t)e,
                                     .kind = 3,
                                     .health = sc_enemies[3].health,
                                     .normal = {0, 1, 0},
                                     .heading = {0, 0, 1},
                                     .alive = room->record == SIZE_MAX ||
                                              !(g->world.records[room->record].killed & (1u << e))};
                }
                room->enemy_count = 16;
            } else if (r->edit == EDIT_WEATHER)
                room->habitat.atmosphere = VOID_GLOW;
            else if (r->edit == EDIT_SUBSTITUTE) {
                static const unsigned roster[ECOLOGIES][2] = {{4, 15},  {27, 29}, {3, 8},
                                                              {14, 40}, {12, 22}, {5, 16}};
                for (int e = 0; e < room->enemy_count; e++) {
                    Enemy *enemy = &room->enemies[e];
                    if (!enemy->alive || enemy->country || enemy->force)
                        continue;
                    float scale = enemy->health / sc_enemies[enemy->kind].health;
                    enemy->kind = (uint8_t)roster[room->habitat.ecology][(room->hash + e) & 1];
                    enemy->health = sc_enemies[enemy->kind].health * scale;
                    enemy->behavior = ALIEN_NONE;
                }
            }
            room->faction_edit = (uint8_t)r->edit;
        }
        if (r->edit == EDIT_STASH && !r->edit_claims)
            for (int p = 0; p < g->player_count; p++) {
                Player *player = &g->players[p];
                Vec3 point = vadd(world_center(r->edit_site), v3(0, .02, 3));
                if (!exploring(g, p) || vlen(vsub(position_absolute(player->pos), point)) > 2.5)
                    continue;
                player->health = fminf(player_max_health(player), player->health + 25);
                player->energy = fminf(player_max_energy(player), player->energy + 30);
                player->credits += 2;
                player->pickups++;
                r->edit_claims = 1;
                game_message(g, "ABANDONED FIELD KIT / SUPPLIES RECOVERED");
                g->dirty = true;
                break;
            }
    }
}
