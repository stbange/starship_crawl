#include "game.h"
#include <string.h>

static ContinuingCampaign *chain(Game *g, uint32_t force) {
    for (unsigned i = 0; i < SC_CONTINUING_CAMPAIGNS; i++)
        if (g->chronicle.campaigns[i].identity && g->chronicle.campaigns[i].force == force)
            return &g->chronicle.campaigns[i];
    return NULL;
}
bool chronicle_order(Game *g, unsigned country, unsigned order, Cell source, Cell goal) {
    if (g->world.generator < 16 || country < 1 || country > 3 ||
        (order != ORDER_RAID && order != ORDER_REINFORCE))
        return false;
    ContinuingCampaign *c = NULL;
    for (unsigned i = 0; i < SC_CONTINUING_CAMPAIGNS; i++) {
        ContinuingCampaign *candidate = &g->chronicle.campaigns[i];
        if (candidate->identity && candidate->country == country &&
            candidate->stage < CHAIN_OCCUPIED)
            return false;
        if (!candidate->identity || candidate->stage >= CHAIN_OCCUPIED)
            c = candidate;
    }
    if (!c || g->chronicle.next_id == UINT32_MAX)
        return false;
    Navigation nav;
    if (!game_navigation_from(g, position_from(vadd(world_center(source), v3(0, .02, 6))), false,
                              goal, &nav) ||
        nav.kind == NAV_BLOCKED)
        return false;
    Cell staging = order == ORDER_RAID ? nav.next : goal;
    if (cell_equal(staging, source) || factions_safe(g, staging))
        return false;
    uint32_t force = g->factions.next_id;
    if (!factions_launch(g, country, ORDER_ESCORT, source, staging))
        return false;
    *c = (ContinuingCampaign){.identity = g->chronicle.next_id++,
                              .country = country,
                              .pattern = order == ORDER_REINFORCE,
                              .force = force,
                              .source = source,
                              .staging = staging,
                              .goal = goal,
                              .stage = CHAIN_SUPPLY};
    g->dirty = true;
    return true;
}
bool chronicle_arrival(Game *g, uint32_t force) {
    if (g->world.generator < 16)
        return false;
    ContinuingCampaign *c = chain(g, force);
    FactionForce *a = factions_force(g, force);
    if (!c || !a || a->order == ORDER_RETREAT)
        return false;
    if (c->stage == CHAIN_SUPPLY && a->order == ORDER_ESCORT) {
        FactionRegion *r = factions_record(g, c->staging);
        if (!r)
            return true; /* Keep the arrived convoy until its durable fact can be recorded. */
        a->arrivals++;
        g->factions.arrived++;
        factions_log(g, FE_ARRIVE, a->country, a->id, a->goal, ORDER_ESCORT);
        a->cargo = 0; /* The paid supplies are consumed here, never returned to a nation. */
        a->wait = 6;
        a->arrivals = 0;
        a->siege = 0;
        memset(a->nav_time, 0, sizeof(a->nav_time));
        if (!c->pattern) {
            c->stage = CHAIN_SIEGE;
            a->order = ORDER_RAID;
            a->goal = c->goal;
        } else {
            r->flags = (r->flags | FS_REPAIRED) & ~FS_RAIDED;
            c->stage = CHAIN_FACTORY;
            c->clock = 30;
            a->order = ORDER_OCCUPY;
            a->arrivals = 1;
            a->wait = 30;
        }
        g->dirty = true;
        return true;
    }
    if (c->stage == CHAIN_REINFORCE && a->order == ORDER_REINFORCE) {
        c->stage = CHAIN_OCCUPIED;
        g->dirty = true;
    }
    return false;
}
static void withdraw(Game *g, ContinuingCampaign *c, FactionForce *a) {
    c->stage = CHAIN_RETREATED;
    c->clock = 0;
    FactionRegion *r = factions_record(g, c->staging);
    if (r)
        r->flags = (r->flags | FS_RAIDED) & ~FS_REPAIRED;
    if (a && factions_strength(a) && a->order != ORDER_RETREAT) {
        a->order = ORDER_RETREAT;
        a->goal = a->home;
        a->wait = a->stuck = 0;
        memset(a->nav_time, 0, sizeof(a->nav_time));
        g->factions.retreats++;
        factions_log(g, FE_RETREAT, a->country, a->id, c->staging, c->identity);
    }
    g->dirty = true;
}
void chronicle_campaigns(Game *g, double dt) {
    if (g->world.generator < 16)
        return;
    bool outside = false;
    for (int p = 0; p < g->player_count; p++)
        outside |= !g->players[p].sheltered;
    if (!outside)
        return;
    for (unsigned i = 0; i < SC_CONTINUING_CAMPAIGNS; i++) {
        ContinuingCampaign *c = &g->chronicle.campaigns[i];
        if (!c->identity || c->stage >= CHAIN_OCCUPIED)
            continue;
        FactionForce *a = factions_force(g, c->force);
        if (!a || factions_strength(a) <= 1 || a->order == ORDER_RETREAT) {
            withdraw(g, c, a);
            continue;
        }
        if (c->stage == CHAIN_SIEGE && a->order == ORDER_OCCUPY) {
            c->stage = CHAIN_OCCUPIED;
            g->dirty = true;
        }
        if (c->stage != CHAIN_FACTORY)
            continue;
        const FactionRegion *r = factions_at(&g->factions, c->staging);
        if (!r || r->owner != c->country || r->systems & SYSTEM_BLACKOUT) {
            withdraw(g, c, a);
            continue;
        }
        c->clock = fmaxf(0, c->clock - (float)dt);
        if (c->clock > 0 || factions_observed(g, world_center(c->staging), 40))
            continue;
        uint32_t next = g->factions.next_id;
        if (!factions_launch(g, c->country, ORDER_REINFORCE, c->staging, c->source))
            continue;
        /* Newly produced troops cost normal resources and have a new identity.
           The repair crew keeps its casualties and travels home independently. */
        a->order = ORDER_RETREAT;
        a->goal = a->home;
        a->wait = 0;
        memset(a->nav_time, 0, sizeof(a->nav_time));
        c->force = next;
        c->stage = CHAIN_REINFORCE;
        g->dirty = true;
    }
}
