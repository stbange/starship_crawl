#include "save_internal.h"
void sc_save_matrix_stream(Stream *s, MatrixState *m, bool read) {
    m->version = sc_save_number(s, m->version, read);
    uint32_t count = sc_save_number(s, (uint32_t)m->count, read);
    if (count > SC_MATRIX_RECORDS || m->version != SC_MATRIX_VERSION)
        s->ok = false;
    if (read && s->ok && count) {
        m->records = calloc(count, sizeof(*m->records));
        if (!m->records)
            s->ok = false;
    }
    if (read)
        m->count = m->capacity = s->ok ? count : 0;
    for (uint32_t i = 0; s->ok && i < count; i++) {
        MatrixRecord r = m->records[i];
        r.key = sc_save_matrix_cell(s, r.key, read);
        for (int j = 0; j < SC_MATRIX_SERVICES; j++)
            r.done[j] = sc_save_number(s, r.done[j], read);
        r.keys = sc_save_number(s, r.keys, read);
        r.killed = sc_save_number(s, r.killed, read);
        r.loot = sc_save_number(s, r.loot, read);
        r.flags = sc_save_number(s, r.flags, read);
        r.seen = sc_save_number(s, r.seen, read);
        r.outcome = sc_save_number(s, r.outcome, read);
        r.alert = sc_save_number(s, r.alert, read);
        if (read)
            m->records[i] = r;
    }
    for (int i = 0; i < SC_LEGACY_PLAYERS; i++) {
        MatrixProfile *p = &m->profiles[i];
        MatrixDiver *d = &m->divers[i];
        p->programs = sc_save_number(s, p->programs, read);
        for (int j = 0; j < 2; j++)
            p->slots[j] = sc_save_number(s, p->slots[j], read);
        for (int j = 0; j < SC_MATRIX_PROGRAMS; j++)
            p->upgrades[j] = sc_save_number(s, p->upgrades[j], read);
        p->data = sc_save_number(s, p->data, read);
        p->dives = sc_save_number(s, p->dives, read);
        p->knowledge = sc_save_number(s, p->knowledge, read);
        d->key = sc_save_matrix_cell(s, d->key, read);
        d->terminal = sc_save_matrix_cell(s, d->terminal, read);
        d->blocked = sc_save_matrix_cell(s, d->blocked, read);
        d->pos = sc_save_vector(s, d->pos, read);
        d->velocity = sc_save_vector(s, d->velocity, read);
        d->echo = sc_save_vector(s, d->echo, read);
        d->body_yaw = sc_save_real(s, d->body_yaw, read);
        d->body_pitch = sc_save_real(s, d->body_pitch, read);
        d->integrity = (float)sc_save_real(s, d->integrity, read);
        d->bandwidth = (float)sc_save_real(s, d->bandwidth, read);
        d->trace = (float)sc_save_real(s, d->trace, read);
        d->cooldown = (float)sc_save_real(s, d->cooldown, read);
        d->hurt = (float)sc_save_real(s, d->hurt, read);
        d->veil = (float)sc_save_real(s, d->veil, read);
        d->siphon = (float)sc_save_real(s, d->siphon, read);
        d->grace = (float)sc_save_real(s, d->grace, read);
        d->counter = (float)sc_save_real(s, d->counter, read);
        d->lockout = (float)sc_save_real(s, d->lockout, read);
        d->echo_life = (float)sc_save_real(s, d->echo_life, read);
        d->network = sc_save_number(s, d->network, read);
        d->selected = sc_save_number(s, d->selected, read);
        d->service = sc_save_number(s, d->service, read);
        d->unbanked = sc_save_number(s, d->unbanked, read);
        d->active = sc_save_matrix_bool(s, d->active, read);
        (void)sc_save_matrix_bool(s, false, read); /* Historical local map flag; never imported. */
        d->choice = sc_save_matrix_bool(s, d->choice, read);
        d->grounded = sc_save_matrix_bool(s, d->grounded, read);
    }
    for (int i = 0; i < 2; i++) {
        MatrixNetwork *w = &m->networks[i];
        w->active = sc_save_matrix_bool(s, w->active, read);
        w->key = sc_save_matrix_cell(s, w->key, read);
        for (int j = 0; j < 2; j++)
            w->switches[j] = (float)sc_save_real(s, w->switches[j], read);
        w->bridge = (float)sc_save_real(s, w->bridge, read);
        w->gate = (float)sc_save_real(s, w->gate, read);
    }
    for (int i = 0; i < SC_MATRIX_CONTROLS; i++) {
        MatrixControl *c = &m->controls[i];
        c->room = sc_save_matrix_cell(s, c->room, read);
        c->life = (float)sc_save_real(s, c->life, read);
        c->kind = sc_save_number(s, c->kind, read);
        c->owner = sc_save_number(s, c->owner, read);
    }
}
