#include "snapshot_internal.h"
/* Structural validation precedes allocation/index rebuilding and all simulation access.
   Scalar/enum limits and finiteness are checked by the portable field walker. */
bool sc_snapshot_valid(const Game *g) {
    const World *w = &g->world;
    if (g->player_count < 1 || g->player_count > SC_PLAYERS || w->count < 0 ||
        w->count > SC_MAX_ROOMS || w->record_count > SC_SNAPSHOT_RECORDS ||
        (w->record_count && !w->records) || g->matrix.count > SC_SNAPSHOT_RECORDS ||
        (g->matrix.count && !g->matrix.records) || g->factions.count > SC_SNAPSHOT_RECORDS ||
        (g->factions.count && !g->factions.regions))
        return false;
    if (g->campaign.version >= 2 && !campaign_valid(&g->campaign, g->players, w))
        return false;
    if (w->generator >= 16 && g->campaign.version != 2)
        return false;
    if (!chronicle_valid(&g->chronicle, w->seed, w->generator))
        return false;
    if (!preparation_valid(&g->home, w, g->players))
        return false;
    if (g->campaign.version >= 2)
        for (unsigned id = 0; id < 8; id++) {
            const GuardianAttempt *a = &g->campaign.attempts[id];
            if (!a->active)
                continue;
            const Room *r = world_find((World *)w, campaign_site(id));
            if (!a->stage || !r || !r->ready || r->enemy_count != 3)
                return false;
            for (unsigned part = 0; part < 2; part++)
                if (r->enemies[part + 1].alive == ((a->broken & (1u << part)) != 0))
                    return false;
        }
    size_t boxes = 0;
    int chunks = 0;
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *c = &w->chunks[i];
        if (c->box_count > SC_CHUNK_BOXES || (c->box_count && !c->boxes) ||
            (!c->active && c->box_count))
            return false;
        boxes += c->box_count;
        chunks += c->active;
    }
    if (chunks != w->chunk_count || boxes != w->box_count)
        return false;
    for (int i = 0; i < w->count; i++) {
        const Room *r = &w->rooms[i];
        if (r->record != SIZE_MAX &&
            (r->record >= w->record_count || !cell_equal(r->cell, w->records[r->record].cell)))
            return false;
        if (!r->residency || r->residency > w->next_residency)
            return false;
        for (int j = 0; j < i; j++)
            if (r->residency == w->rooms[j].residency || cell_equal(r->cell, w->rooms[j].cell))
                return false;
    }
    for (int i = 0; i < SC_PLAYERS; i++) {
        const MatrixDiver *d = &g->matrix.divers[i];
        if (d->active && (d->network >= SC_PLAYERS || !g->matrix.networks[d->network].active ||
                          !cell_equal(d->key, g->matrix.networks[d->network].key)))
            return false;
        const MatrixNetwork *n = &g->matrix.networks[i];
        if (n->active &&
            (n->record >= g->matrix.count || !cell_equal(n->key, g->matrix.records[n->record].key)))
            return false;
    }
    for (size_t i = 0; i < w->record_count; i++)
        if (w->records[i].killed >= (1u << SC_MAX_ENEMIES) || w->records[i].collected >= 32)
            return false;
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        const FactionForce *force = &g->factions.forces[i];
        if (force->id &&
            (!force->country || force->country > SC_COUNTRIES || force->count != SC_FORCE_MEMBERS))
            return false;
    }
    return true;
}
