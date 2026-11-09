#include "game.h"
#include <stdlib.h>
#include <string.h>

const char *sc_matrix_programs[SC_MATRIX_PROGRAMS] = {"VEIL",   "ECHO", "INTERRUPT",
                                                      "BRIDGE", "FORK", "SIPHON"};
const char *sc_matrix_services[SC_MATRIX_SERVICES] = {"RESTORE POWER",       "SILENCE SECURITY",
                                                      "AUTHENTICATE BEACON", "COPY ARCHIVE",
                                                      "REBOOT ROBOTS",       "ROUTE MAINTENANCE"};
const float sc_matrix_costs[SC_MATRIX_PROGRAMS] = {15, 12, 18, 12, 14, 16};
void matrix_init(MatrixState *m) {
    memset(m, 0, sizeof(*m));
    m->version = SC_MATRIX_VERSION;
    for (int p = 0; p < SC_PLAYERS; p++) {
        m->profiles[p].programs = 7;
        m->profiles[p].slots[1] = MX_INTERRUPT;
    }
}
void matrix_destroy(MatrixState *m) {
    free(m->records);
    free(m->index);
    memset(m, 0, sizeof(*m));
}
static size_t slot_for(const MatrixState *m, Cell key) {
    size_t slot = world_hash(0, key, 0x4d4958u) & (m->index_capacity - 1);
    while (m->index[slot] && !cell_equal(m->records[m->index[slot] - 1].key, key))
        slot = (slot + 1) & (m->index_capacity - 1);
    return slot;
}
bool matrix_index(MatrixState *m) {
    if (!m->count)
        return true;
    size_t cap = 128;
    while (cap < m->count * 2)
        cap *= 2;
    size_t *index = calloc(cap, sizeof(*index));
    if (!index)
        return false;
    for (size_t i = 0; i < m->count; i++) {
        size_t slot = world_hash(0, m->records[i].key, 0x4d4958u) & (cap - 1);
        while (index[slot]) {
            if (cell_equal(m->records[index[slot] - 1].key, m->records[i].key)) {
                free(index);
                return false;
            }
            slot = (slot + 1) & (cap - 1);
        }
        index[slot] = i + 1;
    }
    free(m->index);
    m->index = index;
    m->index_capacity = cap;
    return true;
}
const MatrixRecord *matrix_find(const MatrixState *m, Cell key) {
    if (!m->index_capacity || !m->count)
        return NULL;
    size_t i = m->index[slot_for(m, key)];
    return i ? &m->records[i - 1] : NULL;
}
MatrixRecord *matrix_record(MatrixState *m, Cell key, bool create) {
    const MatrixRecord *found = matrix_find(m, key);
    if (found)
        return &m->records[found - m->records];
    if (!create || m->count == SC_MATRIX_RECORDS)
        return NULL;
    if (m->count == m->capacity) {
        size_t cap = m->capacity ? m->capacity * 2 : 16;
        MatrixRecord *r = realloc(m->records, cap * sizeof(*r));
        if (!r)
            return NULL;
        m->records = r;
        m->capacity = cap;
    }
    if (!m->index_capacity || (m->count + 1) * 2 > m->index_capacity) {
        size_t cap = m->index_capacity ? m->index_capacity * 2 : 128;
        size_t *index = calloc(cap, sizeof(*index));
        if (!index)
            return NULL;
        for (size_t i = 0; i < m->count; i++) {
            size_t at = world_hash(0, m->records[i].key, 0x4d4958u) & (cap - 1);
            while (index[at])
                at = (at + 1) & (cap - 1);
            index[at] = i + 1;
        }
        free(m->index);
        m->index = index;
        m->index_capacity = cap;
    }
    size_t slot = slot_for(m, key), i = m->count++;
    m->records[i] = (MatrixRecord){.key = key};
    m->index[slot] = i + 1;
    return &m->records[i];
}
static uint32_t guard_mask(const MatrixNetwork *w, unsigned alert) {
    uint32_t mask = w->mask & ~(1u << w->entry);
    unsigned extras = alert == 0 ? 0 : alert == 1 ? 2 : alert == 2 ? 5 : 8;
    return mask | (((1u << extras) - 1) << 16);
}
static bool key_valid(Cell c);
bool matrix_rehydrate(MatrixState *m, uint32_t seed, uint32_t generator) {
    for (int i = 0; i < SC_PLAYERS; i++) {
        MatrixNetwork *w = &m->networks[i];
        if (!w->active)
            continue;
        const MatrixRecord *r = matrix_find(m, w->key);
        if (!r || !key_valid(w->key))
            return false;
        Cell key = w->key;
        float a = w->switches[0], b = w->switches[1], bridge = w->bridge, gate = w->gate;
        matrix_generate(seed, generator, key, w);
        w->record = (size_t)(r - m->records);
        w->switches[0] = a;
        w->switches[1] = b;
        w->bridge = bridge;
        w->gate = gate;
        uint32_t mask = guard_mask(w, r->alert) & ~r->killed;
        for (unsigned g = 0; g < SC_MATRIX_GUARDS; g++)
            w->guards[g].active = (mask & (1u << g)) != 0;
    }
    return true;
}
static bool key_valid(Cell c) {
    return c.x >= -10000000 && c.x <= 10000000 && c.y >= -4000000 && c.y <= 4000000 && c.z >= 0 &&
           c.z <= 10000000 && c.x % 4 == 0 && c.z % 4 == 0;
}
static bool bounded(float x, float max) {
    return isfinite(x) && x >= 0 && x <= max;
}
bool matrix_valid(const MatrixState *m, uint32_t seed, uint32_t generator, const Player *players,
                  const Shelter *shelter) {
    if (m->version != SC_MATRIX_VERSION || m->count > SC_MATRIX_RECORDS ||
        (m->count && (!m->records || !m->index)))
        return false;
    for (size_t i = 0; i < m->count; i++) {
        const MatrixRecord *r = &m->records[i];
        if (!key_valid(r->key))
            return false;
        MatrixNetwork w;
        matrix_generate(seed, generator, r->key, &w);
        if ((r->flags & ~3u) || r->outcome > 3 || r->alert > 3 || (r->seen & ~w.mask) ||
            (r->keys & ~(1u << w.credential)) || (r->keys & ~r->seen) || (r->loot & ~r->seen) ||
            (r->loot & ~w.mask) || (r->killed & ~guard_mask(&w, r->alert)) ||
            ((r->flags & MX_VAULT) &&
             (!w.rare || !(r->loot & (1u << w.vault)) || !(r->flags & MX_ROUTED))) ||
            (r->outcome &&
             (!(r->flags & MX_ROUTED) || r->alert != 3 || r->killed != guard_mask(&w, 3))) ||
            (r->outcome == 3 &&
             (!r->keys || !(w.rare ? (r->flags & MX_VAULT) : r->done[MX_ARCHIVE]))))
            return false;
        for (int s = 0; s < SC_MATRIX_SERVICES; s++)
            if (r->done[s] & ~r->seen)
                return false;
    }
    for (int i = 0; i < SC_PLAYERS; i++) {
        const MatrixProfile *p = &m->profiles[i];
        const MatrixDiver *d = &m->divers[i];
        if ((p->programs & ~63u) ||
            (generator >= 15 ? !(p->programs & 1) : (p->programs & 7) != 7) ||
            p->data > 1000000000u || p->dives > 1000000000u || p->knowledge > 1000000000u ||
            p->slots[0] >= 6 || p->slots[1] >= 6 || !(p->programs & (1u << p->slots[0])) ||
            !(p->programs & (1u << p->slots[1])))
            return false;
        for (int j = 0; j < 6; j++)
            if (p->upgrades[j] > 3 || (!(p->programs & (1u << j)) && p->upgrades[j]))
                return false;
        if (d->selected >= 2 || d->service >= 6 || d->unbanked > 65536 ||
            !bounded(d->integrity, 100 + 2 * players[i].mods[4]) ||
            !bounded(d->bandwidth, 100 + 4 * players[i].mods[5]) || !bounded(d->trace, 100) ||
            !bounded(d->cooldown, 1) || !bounded(d->hurt, 4) || !bounded(d->veil, 20) ||
            !bounded(d->siphon, 15) || !bounded(d->grace, 2) || !bounded(d->counter, 6) ||
            !bounded(d->lockout, 8) || !bounded(d->echo_life, 20) || !isfinite(d->body_yaw) ||
            fabs(d->body_yaw) > SC_PI + 1e-6 || !isfinite(d->body_pitch) ||
            fabs(d->body_pitch) > 1.5 || !isfinite(d->pos.x) || !isfinite(d->pos.y) ||
            !isfinite(d->pos.z) || fabs(d->pos.x) > 256 || fabs(d->pos.z) > 256 || d->pos.y < -12 ||
            d->pos.y > 16 || !isfinite(d->velocity.x) || !isfinite(d->velocity.y) ||
            !isfinite(d->velocity.z) || vlen(d->velocity) > 30 || !isfinite(d->echo.x) ||
            !isfinite(d->echo.y) || !isfinite(d->echo.z) || vlen(d->echo) > 400)
            return false;
        if (d->active) {
            if (!key_valid(d->key) || d->network >= SC_PLAYERS || !m->networks[d->network].active ||
                !cell_equal(d->key, m->networks[d->network].key) || players[i].riding ||
                players[i].health <= 0 || !cell_equal(matrix_region(d->terminal), d->key) ||
                !cell_equal(world_describe_version(seed, generator, d->terminal).cell, d->terminal))
                return false;
            if (players[i].sheltered) {
                if (!shelter->deployed ||
                    vlen(vsub(players[i].shelter_pos, shelter_station(SHELTER_TERMINAL))) > 3.4)
                    return false;
            } else if (vlen(vsub(position_absolute(players[i].pos), matrix_terminal(d->terminal))) >
                       3.6)
                return false;
        }
    }
    for (int i = 0; i < SC_PLAYERS; i++) {
        const MatrixNetwork *w = &m->networks[i];
        if (!w->active)
            continue;
        if (!key_valid(w->key) || w->record >= m->count ||
            !cell_equal(m->records[w->record].key, w->key) || !bounded(w->switches[0], 30) ||
            !bounded(w->switches[1], 30) || !bounded(w->bridge, 30) || !bounded(w->gate, 15) ||
            (i && m->networks[0].active && cell_equal(w->key, m->networks[0].key)))
            return false;
    }
    for (int i = 0; i < SC_MATRIX_CONTROLS; i++) {
        const MatrixControl *c = &m->controls[i];
        if (!bounded(c->life, 30) || c->kind > 3 || c->owner >= SC_PLAYERS)
            return false;
        if (c->life > 0) {
            if (!c->kind || c->room.x < -10000000 || c->room.x > 10000000 || c->room.y < -4000000 ||
                c->room.y > 4000000 || c->room.z < 0 || c->room.z > 10000000 ||
                !cell_equal(world_describe_version(seed, generator, c->room).cell, c->room))
                return false;
            for (int j = 0; j < i; j++)
                if (m->controls[j].life > 0 && cell_equal(c->room, m->controls[j].room) &&
                    (c->kind == 3) == (m->controls[j].kind == 3))
                    return false;
        }
    }
    return true;
}
const char *matrix_controller_name(unsigned motif) {
    const char *names[] = {"ORISON FRAGMENT", "THE MEMORY CUSTODIAN", "THE CHOIR ENGINE",
                           "THE QUARANTINE WITNESS"};
    return names[motif % 4];
}

const char *matrix_controller(const MatrixNetwork *w) {
    return matrix_controller_name(w->motif);
}
