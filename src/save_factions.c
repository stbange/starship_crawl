#include "save_internal.h"
/* Portable appendix. Tactical room proxies are reconstructed, never serialized twice. */
void sc_save_factions_stream(Stream *s, Factions *f, bool read) {
#define FN(x) f->x = sc_save_number(s, f->x, read)
#define FR(x) f->x = (float)sc_save_real(s, f->x, read)
    FN(version);
    FN(tick);
    FN(random);
    FN(next_id);
    FN(event_count);
    FR(remainder);
    FN(launched);
    FN(arrived);
    FN(casualties);
    FN(captures);
    FN(retreats);
    FN(decisions);
    uint32_t count = sc_save_number(s, (uint32_t)f->count, read);
    if (count > SC_FACTION_REGIONS)
        s->ok = false;
    if (read && s->ok && count) {
        f->regions = calloc(count, sizeof(*f->regions));
        if (!f->regions)
            s->ok = false;
    }
    if (read)
        f->count = f->capacity = s->ok ? count : 0;
    for (size_t i = 0; s->ok && i < f->count; i++) {
        FactionRegion *r = &f->regions[i];
        r->region = sc_save_matrix_cell(s, r->region, read);
        r->site = sc_save_matrix_cell(s, r->site, read);
        r->owner = sc_save_number(s, r->owner, read);
        r->kind = sc_save_number(s, r->kind, read);
        r->flags = sc_save_number(s, r->flags, read);
        r->epoch = sc_save_number(s, r->epoch, read);
        r->casualties = sc_save_number(s, r->casualties, read);
        r->systems = sc_save_number(s, r->systems, read);
        r->edit_site = sc_save_matrix_cell(s, r->edit_site, read);
        r->seen = sc_save_number(s, r->seen, read);
        r->edit = sc_save_number(s, r->edit, read);
        r->edit_claims = sc_save_number(s, r->edit_claims, read);
    }
    for (unsigned a = 0; a < 3; a++) {
        for (unsigned b = 0; b < 3; b++) {
            FN(relations[a][b]);
            FN(nations[a].supplies[b]);
        }
        FN(nations[a].losses);
        FN(nations[a].ruler);
        FN(nations[a].cycle);
        FN(nations[a].next_order);
        FN(nations[a].next_relation);
        for (unsigned p = 0; p < 2; p++) {
            FactionIncident *in = &f->incidents[p][a];
            in->site = sc_save_matrix_cell(s, in->site, read);
            FN(incidents[p][a].strikes);
            FN(incidents[p][a].reports);
            FN(incidents[p][a].witness);
            FN(incidents[p][a].member);
            FR(incidents[p][a].local_time);
            FR(incidents[p][a].grace);
            FR(incidents[p][a].transmit);
        }
    }
    for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
        FactionForce *a = &f->forces[i];
        FN(forces[i].id);
        FN(forces[i].country);
        FN(forces[i].order);
        FN(forces[i].count);
        FN(forces[i].cargo);
        FN(forces[i].generation);
        a->home = sc_save_matrix_cell(s, a->home, read);
        a->goal = sc_save_matrix_cell(s, a->goal, read);
        a->next = sc_save_matrix_cell(s, a->next, read);
        FR(forces[i].age);
        FR(forces[i].wait);
        FR(forces[i].route_time);
        FR(forces[i].stuck);
        a->waypoint = sc_save_vector(s, a->waypoint, read);
        FN(forces[i].participation[0]);
        FN(forces[i].participation[1]);
        FN(forces[i].claimed);
        FN(forces[i].arrivals);
        FN(forces[i].siege);
        FN(forces[i].present);
        for (unsigned m = 0; m < SC_FORCE_MEMBERS; m++) {
            Enemy *e = &a->members[m];
            e->pos = sc_save_location(s, e->pos, read, false);
            e->health = (float)sc_save_real(s, e->health, read);
            e->cooldown = (float)sc_save_real(s, e->cooldown, read);
            e->slow = (float)sc_save_real(s, e->slow, read);
            e->burn = (float)sc_save_real(s, e->burn, read);
            uint32_t kind = sc_save_number(s, e->kind, read),
                     alive = sc_save_number(s, e->alive, read);
            uint32_t allegiance = sc_save_number(s, e->allegiance, read),
                     owner = sc_save_number(s, e->ally_owner, read);
            uint32_t last = sc_save_number(s, e->last_player, read);
            if (kind >= SC_ENEMY_TYPES || alive > 1 || allegiance > 3 || owner > 1 || last > 1)
                s->ok = false;
            e->kind = (uint8_t)kind;
            e->alive = alive != 0;
            e->allegiance = (uint8_t)allegiance;
            e->ally_owner = (uint8_t)owner;
            e->last_player = (uint8_t)last;
            e->heading = sc_save_vector(s, e->heading, read);
            if (read) {
                e->force = a->id;
                e->member = (uint8_t)m;
                e->country = (uint8_t)a->country;
                e->normal = v3(0, 1, 0);
            }
        }
        if (read)
            a->physical = false;
    }
    HiddenDirector *d = &f->director;
    FN(director.sequence);
    FN(director.budget);
    FN(director.accepted);
    FN(director.rejected);
    FR(director.cooldown);
    FR(director.recharge);
    for (unsigned p = 0; p < 2; p++) {
        FN(director.pace[p]);
        FR(director.pressure[p]);
        FR(director.quiet[p]);
        FR(director.recovery[p]);
        FR(director.defenses[p]);
        d->last_room[p] = sc_save_matrix_cell(s, d->last_room[p], read);
    }
    for (unsigned i = 0; i < 16; i++)
        FN(director.history[i]);
    FN(director.proposal.kind);
    FN(director.proposal.phase);
    FN(director.proposal.player);
    FN(director.proposal.force);
    FN(director.proposal.cost);
    FN(director.proposal.sequence);
    FR(director.proposal.remaining);
    d->proposal.site = sc_save_matrix_cell(s, d->proposal.site, read);
    for (unsigned i = 0; i < SC_FACTION_EVENTS; i++) {
        FN(events[i].tick);
        FN(events[i].kind);
        FN(events[i].country);
        FN(events[i].force);
        FN(events[i].detail);
        f->events[i].site = sc_save_matrix_cell(s, f->events[i].site, read);
    }
#undef FN
#undef FR
}
