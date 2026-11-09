#include "save_internal.h"
void sc_save_campaign_stream(Stream *s, Campaign *c, bool read) {
    c->version = sc_save_number(s, c->version, read);
    c->defeated = sc_save_number(s, c->defeated, read);
    c->tracked = sc_save_number(s, c->tracked, read);
    for (int i = 0; i < SC_GUARDIANS; i++) {
        c->eligible[i] = sc_save_number(s, c->eligible[i], read);
        c->attempts[i].active = sc_save_matrix_bool(s, c->attempts[i].active, read);
        c->attempts[i].participants = sc_save_number(s, c->attempts[i].participants, read);
    }
    for (int i = 0; i < SC_LEGACY_PLAYERS; i++) {
        CampaignProfile *p = &c->profiles[i];
        p->powers = sc_save_number(s, p->powers, read);
        p->rigs = sc_save_number(s, p->rigs, read);
        p->modules = sc_save_number(s, p->modules, read);
        p->claimed = sc_save_number(s, p->claimed, read);
        for (int k = 0; k < 2; k++) {
            p->guns[k] = sc_save_number(s, p->guns[k], read);
            p->power_slots[k] = sc_save_number(s, p->power_slots[k], read);
        }
        p->rig = sc_save_number(s, p->rig, read);
        p->module = sc_save_number(s, p->module, read);
    }
}
void sc_save_politics_stream(Stream *s, Politics *p, Director *d, bool read) {
    p->version = sc_save_number(s, p->version, read);
    p->tracked = sc_save_number(s, p->tracked, read);
    for (int i = 0; i < SC_COUNTRIES; i++) {
        CountryState *c = &p->countries[i];
        c->met = sc_save_number(s, c->met, read);
        c->pledge = sc_save_number(s, c->pledge, read);
        c->stage = sc_save_number(s, c->stage, read);
        c->claimed = sc_save_number(s, c->claimed, read);
        c->offended = sc_save_number(s, c->offended, read);
    }
    d->version = sc_save_number(s, d->version, read);
    d->sequence = sc_save_number(s, d->sequence, read);
    d->cooldown = sc_save_number(s, d->cooldown, read);
    d->budget = sc_save_number(s, d->budget, read);
    d->recharge = sc_save_number(s, d->recharge, read);
    d->quiet = sc_save_number(s, d->quiet, read);
    d->kind = sc_save_number(s, d->kind, read);
    d->remaining = sc_save_number(s, d->remaining, read);
    d->claimed = sc_save_number(s, d->claimed, read);
    d->site = sc_save_matrix_cell(s, d->site, read);
    for (int i = 0; i < SC_LEGACY_PLAYERS; i++) {
        d->last_room[i] = sc_save_matrix_cell(s, d->last_room[i], read);
        d->seen[i] = sc_save_number(s, d->seen[i], read);
    }
    for (int i = 0; i < SC_DIRECTOR_LOG; i++) {
        d->history[i] = sc_save_number(s, d->history[i], read);
        d->reasons[i] = sc_save_number(s, d->reasons[i], read);
    }
}
