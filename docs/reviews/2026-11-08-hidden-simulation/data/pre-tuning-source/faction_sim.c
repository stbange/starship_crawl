#include "game.h"
#include <stdlib.h>
#include <string.h>

static int region_compare(Cell a, Cell b) {
    if (a.y != b.y)
        return a.y < b.y ? -1 : 1;
    if (a.z != b.z)
        return a.z < b.z ? -1 : 1;
    return (a.x > b.x) - (a.x < b.x);
}
static size_t region_index(const Factions *f, Cell region) {
    size_t lo = 0, hi = f->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (region_compare(f->regions[mid].region, region) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}
const FactionRegion *factions_at(const Factions *f, Cell cell) {
    Cell region = world_region(cell);
    size_t at = region_index(f, region);
    return at < f->count && cell_equal(f->regions[at].region, region) ? &f->regions[at] : NULL;
}
void factions_destroy(Factions *f) {
    free(f->regions);
    memset(f, 0, sizeof(*f));
}
void factions_init(Game *g) {
    factions_destroy(&g->factions);
    Factions *f = &g->factions;
    if (g->world.generator < 14)
        return;
    f->version = 1;
    f->random = g->world.seed ^ 0xa127510bu;
    f->next_id = 1;
    f->director.budget = 3;
    f->director.cooldown = 20;
    for (unsigned a = 0; a < 3; a++) {
        FactionNation *n = &f->nations[a];
        for (unsigned r = 0; r < 3; r++)
            n->supplies[r] = 30 + world_hash(g->world.seed, (Cell){(int)a, (int)r, 0}, 0xa11) % 25;
        n->next_order = 5 + a * 4;
        n->next_relation = 60 + a * 13;
        for (unsigned b = 0; b < 3; b++) {
            unsigned low = a < b ? a : b, high = a < b ? b : a;
            f->relations[a][b] =
                a == b ? REL_PEACE
                : low == 0 && high == 1
                    ? REL_WAR
                    : world_hash(g->world.seed, (Cell){(int)low, (int)high, 0}, 7104) % 4;
        }
    }
}
void factions_plan(uint32_t seed, RegionPlan *plan) {
    int best = -1;
    for (int i = 0; i < plan->count; i++) {
        RoomSpec *r = &plan->rooms[i];
        r->habitat.country = (uint8_t)politics_country(seed, r->cell);
        if (r->type == BAY || r->history_role || r->height < 5 || campaign_guardian(r->cell) >= 0 ||
            r->habitat.pace == PACE_FRONTIER)
            continue;
        if (best < 0 || r->hash < plan->rooms[best].hash)
            best = i;
    }
    if (best < 0)
        return;
    RoomSpec *r = &plan->rooms[best];
    if (!r->habitat.country)
        return;
    r->habitat.court = (uint8_t)(SITE_POWER + (r->hash >> 10) % 3);
    Cell region = world_region(plan->origin);
    for (unsigned n = 0; n < 3; n++)
        if (cell_equal(region, politics_region(seed, n, COURT_AUDIENCE))) {
            r->habitat.court = SITE_CROWN;
            r->habitat.country = (uint8_t)(n + 1);
        }
}
void factions_populate(World *w, Room *r) {
    if (!r->habitat.country || r->type == BAY || campaign_guardian(r->cell) >= 0 ||
        r->history_role || r->habitat.pace == PACE_FRONTIER || r->height < 3)
        return;
    if (!r->habitat.court && r->hash % 3)
        return;
    if (r->enemy_count < 2)
        r->enemy_count = 2;
    for (int i = 0; i < 2; i++) {
        Enemy *e = &r->enemies[i];
        unsigned country = r->habitat.country;
        e->country = (uint8_t)country;
        e->kind = politics_government(w->seed, country - 1) == ECHO_PARLIAMENT ? 28
                  : politics_government(w->seed, country - 1) == MOLTING_COURT ? 23
                                                                               : 24;
        e->health = sc_enemies[e->kind].health + 10.f * (float)fmin(campaign_layer(r->cell), 8);
        e->pos = position_from(vadd(world_center(r->cell), v3(i ? 2 : -2, .02, 7)));
        e->home = position_absolute(e->pos);
        e->normal = v3(0, 1, 0);
        e->heading = v3(0, 0, 1);
        e->behavior = ALIEN_NONE;
        e->id = (uint8_t)i;
        e->alive = r->record == SIZE_MAX || !(w->records[r->record].killed & (1u << i));
    }
}
void factions_log(Game *g, unsigned kind, unsigned country, unsigned force, Cell site,
                  unsigned detail) {
    Factions *f = &g->factions;
    f->events[f->event_count % SC_FACTION_EVENTS] =
        (FactionEvent){f->tick, kind, country, force, detail, site};
    f->event_count = f->event_count == UINT32_MAX ? SC_FACTION_EVENTS : f->event_count + 1;
    g->dirty = true;
}
FactionRegion *factions_record(Game *g, Cell cell) {
    Factions *f = &g->factions;
    if (!f->version || cell.z < 0)
        return NULL;
    Cell region = world_region(cell);
    size_t at = region_index(f, region);
    if (at < f->count && cell_equal(f->regions[at].region, region))
        return &f->regions[at];
    if (f->count >= SC_FACTION_REGIONS)
        return NULL;
    if (f->count == f->capacity) {
        size_t capacity = f->capacity ? f->capacity * 2 : 32;
        if (capacity > SC_FACTION_REGIONS)
            capacity = SC_FACTION_REGIONS;
        FactionRegion *records = realloc(f->regions, capacity * sizeof(*records));
        if (!records)
            return NULL;
        f->regions = records;
        f->capacity = capacity;
    }
    RegionPlan plan;
    world_plan_version(g->world.seed, g->world.generator, region, &plan);
    FactionRegion record = {
        .region = region, .site = cell, .owner = politics_country(g->world.seed, cell)};
    for (int i = 0; i < plan.count; i++)
        if (plan.rooms[i].habitat.court) {
            record.site = plan.rooms[i].cell;
            record.kind = plan.rooms[i].habitat.court;
            record.owner = plan.rooms[i].habitat.country;
            break;
        }
    memmove(&f->regions[at + 1], &f->regions[at], (f->count - at) * sizeof(record));
    f->regions[at] = record;
    f->count++;
    factions_log(g, FE_REGION, record.owner, 0, record.site, record.kind);
    return &f->regions[at];
}
unsigned factions_owner(const Game *g, Cell cell) {
    const FactionRegion *r = factions_at(&g->factions, cell);
    return r ? r->owner : politics_country(g->world.seed, cell);
}
unsigned factions_relation(const Game *g, unsigned a, unsigned b) {
    return a && a <= 3 && b && b <= 3 ? g->factions.relations[a - 1][b - 1] : REL_WAR;
}
bool factions_hostile(const Game *g, const Enemy *e, int player) {
    if (!e->country || e->country > 3 || player < 0 || player >= SC_PLAYERS)
        return !e->country;
    const FactionIncident *in = &g->factions.incidents[player][e->country - 1];
    return in->reports >= 3 || (in->strikes >= 2 && in->local_time > 0 &&
                                cell_equal(world_region(in->site),
                                           world_region(world_cell(position_absolute(e->pos)))));
}
static bool protected_site(const Game *g, Cell cell, bool optional) {
    if (cell.z < 0 || world_difficulty(cell) == 0 || campaign_guardian(cell) >= 0)
        return true;
    for (int d = 0; d < 4; d++)
        if (campaign_guardian(world_neighbor(cell, (Direction)d)) >= 0)
            return true;
    const Room *room = world_find((World *)&g->world, cell);
    RoomSpec r = room ? (RoomSpec){.cell = room->cell,
                                   .type = room->type,
                                   .history_role = room->history_role,
                                   .habitat = room->habitat}
                      : world_describe_version(g->world.seed, g->world.generator, cell);
    if (r.type == BAY || r.history_role || (optional && r.habitat.pace == PACE_FRONTIER))
        return true;
    for (unsigned i = 0; i < SC_STORY_SCENES; i++)
        if ((g->story.assigned & (1u << i)) && cell_equal(g->story.sites[i], r.cell))
            return true;
    return false;
}
bool factions_safe(const Game *g, Cell cell) {
    return protected_site(g, cell, true);
}
bool factions_required(const Game *g, Cell cell) {
    return protected_site(g, cell, false);
}
bool factions_active(const Game *g) {
    if (!g->factions.version || g->world.generator < 14)
        return false;
    for (int p = 0; p < g->player_count; p++)
        if (!g->players[p].sheltered && position_absolute(g->players[p].pos).z >= 0)
            return true;
    return false;
}
bool factions_observed(const Game *g, Vec3 point, double margin) {
    for (int p = 0; p < g->player_count; p++) {
        const Player *player = &g->players[p];
        if (player->sheltered)
            continue;
        Vec3 delta = vsub(point, player_eye(player));
        double distance = vlen(delta);
        if (distance < margin)
            return true;
        if (g->matrix.divers[p].active || distance > 220)
            continue;
        Vec3 aim = v3(sin(player->yaw) * cos(player->pitch), sin(player->pitch),
                      cos(player->yaw) * cos(player->pitch));
        if (vdot(vnorm(delta), aim) > .45 &&
            world_raycast(&g->world, player_eye(player), vnorm(delta), distance) >= distance - .2)
            return true;
    }
    for (int i = 0; i < SC_MAX_PROJECTILES; i++)
        if (g->projectiles[i].active &&
            vlen(vsub(position_absolute(g->projectiles[i].pos), point)) < 30)
            return true;
    return false;
}
void factions_step(Game *g, double dt) {
    if (!factions_active(g))
        return;
    factions_incidents(g, dt);
    Factions *f = &g->factions;
    f->remainder += (float)dt;
    if (f->remainder < 2)
        return;
    f->remainder -= 2;
    f->tick++;
    for (int p = 0; p < g->player_count; p++) {
        if (g->players[p].sheltered)
            continue;
        Cell cell = world_cell(position_absolute(g->players[p].pos));
        for (int d = -1; d < 6; d++) {
            Cell near = cell;
            if (d >= 0 && d < 4) {
                Cell r = world_region(cell);
                near = (Cell){r.x * 4 + (d == 0   ? -1
                                         : d == 1 ? 4
                                                  : 0),
                              cell.y,
                              r.z * 4 + (d == 2   ? -1
                                         : d == 3 ? 4
                                                  : 0)};
            } else if (d >= 4)
                near.y += d == 4 ? -1 : 1;
            if (near.z < 0 || (g->world.open_layer && campaign_layer(near) > g->world.open_layer))
                continue;
            FactionRegion *r = factions_record(g, near);
            if (r && d == -1)
                r->flags |= FS_SEEN;
        }
    }
    factions_orders(g);
    factions_forces_tick(g);
    g->dirty = true;
}
static bool finite_range(float value, float high) {
    return isfinite(value) && value >= 0 && value <= high;
}
bool factions_world_valid(const Factions *f, const World *world, unsigned layer) {
    if (!f->version)
        return true;
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        const FactionForce *a = &f->forces[i];
        if (!a->id)
            continue;
        if ((layer && (campaign_layer(a->home) > layer || campaign_layer(a->goal) > layer)) ||
            campaign_guardian(a->home) >= 0 || campaign_guardian(a->goal) >= 0)
            return false;
    }
    for (size_t i = 0; i < f->count; i++)
        if (f->regions[i].edit) {
            const FactionRegion *r = &f->regions[i];
            RoomSpec room = world_describe_version(world->seed, world->generator, r->edit_site);
            if ((layer && campaign_layer(r->edit_site) > layer) || room.type == BAY ||
                room.history_role || campaign_guardian(room.cell) >= 0 ||
                (room.habitat.pace == PACE_FRONTIER && r->edit != EDIT_CANCEL))
                return false;
        }
    return !f->director.proposal.phase || !layer ||
           campaign_layer(f->director.proposal.site) <= layer;
}
static bool valid_cell(Cell c) {
    return c.x >= -9000000 && c.x <= 9000000 && c.y >= -4000000 && c.y <= 4000000 && c.z >= 0 &&
           c.z <= 9000000;
}
static bool valid_point(Vec3 p) {
    return isfinite(p.x) && isfinite(p.y) && isfinite(p.z) && fabs(p.x) < 9e8 && fabs(p.y) < 3e8 &&
           fabs(p.z) < 9e8;
}
bool factions_valid(const Factions *f, unsigned generator) {
    if (f->version != (generator >= 14) || f->count > SC_FACTION_REGIONS ||
        (f->count && !f->regions) || !isfinite(f->remainder) || f->remainder < 0 ||
        f->remainder >= 2.05)
        return false;
    if (!f->version)
        return !f->count && !f->tick && !f->next_id;
    if (!f->next_id)
        return false;
    for (unsigned a = 0; a < 3; a++) {
        for (unsigned b = 0; b < 3; b++)
            if (f->relations[a][b] > REL_SYMBIOSIS || f->relations[a][b] != f->relations[b][a] ||
                (a == b && f->relations[a][b] != REL_PEACE) || f->nations[a].supplies[b] > 1000000)
                return false;
        for (unsigned p = 0; p < 2; p++) {
            const FactionIncident *in = &f->incidents[p][a];
            if (!valid_cell(in->site) || in->strikes > 20 || in->reports > 3 ||
                in->member >= SC_FORCE_MEMBERS ||
                (in->witness != UINT32_MAX && in->witness >= f->next_id) ||
                !finite_range(in->local_time, 180) || !finite_range(in->grace, 1) ||
                !finite_range(in->transmit, 5))
                return false;
        }
    }
    for (size_t i = 0; i < f->count; i++) {
        const FactionRegion *r = &f->regions[i];
        if (r->owner > 3 || r->kind > SITE_FABRICATOR || r->flags > 255 || r->systems > 15 ||
            r->region.z < 0 || r->region.z > 2250000 || r->region.x < -2250000 ||
            r->region.x > 2250000 || r->region.y < -4000000 || r->region.y > 4000000 ||
            !valid_cell(r->site) || !cell_equal(world_region(r->site), r->region) ||
            r->seen > 65535 || r->edit >= DIRECTOR_EDITS || r->edit_claims > 1 ||
            (r->edit &&
             (!valid_cell(r->edit_site) || !cell_equal(world_region(r->edit_site), r->region))) ||
            (i && region_compare(f->regions[i - 1].region, r->region) >= 0))
            return false;
    }
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        const FactionForce *a = &f->forces[i];
        if (!a->id) {
            if (a->count || a->country || a->order)
                return false;
            continue;
        }
        if (a->id >= f->next_id || a->country < 1 || a->country > 3 ||
            a->count != SC_FORCE_MEMBERS || a->order <= ORDER_NONE || a->order >= FORCE_ORDERS ||
            a->cargo > 12 || a->claimed > 3 || a->siege > 3 || a->present > 7 ||
            a->generation > f->tick || !valid_cell(a->home) || !valid_cell(a->goal) ||
            !valid_cell(a->next) || !finite_range(a->age, 20.01f) || !finite_range(a->wait, 120) ||
            !finite_range(a->route_time, 3) || !finite_range(a->stuck, 1000000) ||
            !valid_point(a->waypoint) || a->participation[0] > 1000000 ||
            a->participation[1] > 1000000)
            return false;
        for (unsigned j = 0; j < i; j++)
            if (f->forces[j].id == a->id)
                return false;
        if (a->route_time > 0) {
            Cell from = world_cell(a->waypoint);
            if (from.x != a->next.x || from.z != a->next.z || abs(from.y - a->next.y) != 1)
                return false;
        }
        for (unsigned m = 0; m < a->count; m++) {
            const Enemy *e = &a->members[m];
            if (e->force != a->id || e->member != m || e->country != a->country ||
                e->kind >= SC_ENEMY_TYPES || e->allegiance > 3 || e->ally_owner > 1 ||
                e->last_player > 1 || !finite_range(e->health, 1000000) ||
                (e->alive && e->health <= 0) || !finite_range(e->cooldown, 60) ||
                !finite_range(e->slow, 10) || !finite_range(e->burn, 10) ||
                !valid_point(position_absolute(e->pos)) ||
                !valid_cell(world_cell(position_absolute(e->pos))) || !valid_point(e->heading))
                return false;
        }
    }
    const HiddenDirector *d = &f->director;
    if (d->budget > 3 || !finite_range(d->cooldown, 120) || !finite_range(d->recharge, 50.01f))
        return false;
    for (unsigned p = 0; p < 2; p++)
        if (d->pace[p] > PACE_DISCOVERY || !finite_range(d->pressure[p], 1) ||
            !finite_range(d->quiet[p], 600) || !finite_range(d->recovery[p], 45) ||
            !finite_range(d->defenses[p], 1000000) || !valid_cell(d->last_room[p]))
            return false;
    for (unsigned i = 0; i < 16; i++)
        if (d->history[i] >= DIRECTOR_EDITS)
            return false;
    const DirectionProposal *p = &d->proposal;
    if (p->kind >= DIRECTOR_EDITS || p->phase > 4 || p->player >= SC_PLAYERS || p->cost > 3 ||
        !finite_range(p->remaining, 60) || !valid_cell(p->site) || p->force >= f->next_id ||
        (p->phase && (!p->kind || p->cost != 1 || !factions_at(f, p->site))) ||
        (p->phase == 1 && p->sequence != d->sequence + 1))
        return false;
    for (unsigned i = 0; i < SC_FACTION_EVENTS; i++) {
        const FactionEvent *e = &f->events[i];
        if (e->kind >= FE_EVENTS || e->country > 3 || e->force >= f->next_id || e->tick > f->tick ||
            !valid_cell(e->site))
            return false;
    }
    return true;
}
