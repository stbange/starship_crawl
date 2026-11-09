#include "save_internal.h"
void sc_save_profile(Stream *s, Player *p, bool read, unsigned version) {
    bool legacy = version == 1;
    p->pos = sc_save_location(s, p->pos, read, legacy);
    p->checkpoint = sc_save_location(s, p->checkpoint, read, legacy);
    p->yaw = sc_save_real(s, p->yaw, read);
    p->pitch = sc_save_real(s, p->pitch, read);
    p->level = sc_save_number(s, p->level, read);
    p->xp = sc_save_number(s, p->xp, read);
    p->kills = sc_save_number(s, p->kills, read);
    p->points = sc_save_number(s, p->points, read);
    p->power = sc_save_number(s, p->power, read);
    p->vitality = sc_save_number(s, p->vitality, read);
    p->haste = sc_save_number(s, p->haste, read);
    p->tint = sc_save_number(s, p->tint, read);
    p->health = (float)sc_save_real(s, p->health, read);
    if (version >= 3) {
        for (int i = 0; i < 7; i++)
            p->mods[i] = sc_save_number(s, p->mods[i], read);
        p->weapons = sc_save_number(s, p->weapons, read);
        p->weapon = sc_save_number(s, p->weapon, read);
        p->credits = sc_save_number(s, p->credits, read);
        p->logs = sc_save_number(s, p->logs, read);
        p->victories = sc_save_number(s, p->victories, read);
        p->pickups = sc_save_number(s, p->pickups, read);
        p->shield = (float)sc_save_real(s, p->shield, read);
        p->armor = (float)sc_save_real(s, p->armor, read);
        p->energy = (float)sc_save_real(s, p->energy, read);
    } else if (read) {
        p->weapons = 7;
        p->shield = 50;
        p->armor = 25;
        p->energy = 100;
    }
    if (read) {
        for (int i = 0; i < 7; i++)
            if (p->mods[i] > 50)
                s->ok = false;
        unsigned limit = version < 6 ? SC_GENERATED_WEAPONS : SC_WEAPONS;
        if (!p->weapons || p->weapons >= (1u << limit) || p->weapon >= limit ||
            !(p->weapons & (1u << p->weapon)) || p->credits > 1000000000 || p->logs >= 4096 ||
            p->victories > 1000000000 || p->pickups > 1000000000 || !isfinite(p->shield) ||
            p->shield < 0 || p->shield > player_max_shield(p) || !isfinite(p->armor) ||
            p->armor < 0 || p->armor > player_max_armor(p) || !isfinite(p->energy) ||
            p->energy < 0 || p->energy > player_max_energy(p))
            s->ok = false;
    }

    if (read) {
        if (!sc_save_valid_vector(position_absolute(p->pos)) ||
            !sc_save_valid_vector(position_absolute(p->checkpoint)) || !isfinite(p->yaw) ||
            !isfinite(p->pitch) || p->level < 1 || p->level > 1000 || p->xp > 100000 ||
            p->points > 1000 || p->power > 50 || p->vitality > 50 || p->haste > 50 ||
            !isfinite(p->health) || p->health <= 0 || p->health > player_max_health(p) ||
            fabs(p->pitch) > 1.5 || fabs(p->yaw) > SC_PI + 1e-6)
            s->ok = false;
        p->crouched = true;
        p->flashlight = true;
    }
}
void sc_save_sanctuary(Stream *s, Shelter *shelter, Player *players, bool read) {
    uint32_t deployed = sc_save_number(s, shelter->deployed, read);
    shelter->deployed = deployed != 0;
    shelter->door = sc_save_location(s, shelter->door, read, false);
    shelter->yaw = sc_save_real(s, shelter->yaw, read);
    if (deployed > 1 || !isfinite(shelter->yaw) || fabs(shelter->yaw) > SC_PI + 1e-6 ||
        (deployed && position_absolute(shelter->door).z < 0))
        s->ok = false;
    for (int i = 0; i < SC_LEGACY_PLAYERS; i++) {
        Player *p = &players[i];
        uint32_t inside = sc_save_number(s, p->sheltered, read);
        p->sheltered = inside != 0;
        p->shelter_pos = sc_save_vector(s, p->shelter_pos, read);
        p->return_yaw = sc_save_real(s, p->return_yaw, read);
        p->return_pitch = sc_save_real(s, p->return_pitch, read);
        p->resting = (float)sc_save_real(s, p->resting, read);
        p->nourished = (float)sc_save_real(s, p->nourished, read);
        if (inside > 1 || !shelter_clear(p->shelter_pos) || !isfinite(p->return_yaw) ||
            fabs(p->return_yaw) > SC_PI + 1e-6 || !isfinite(p->return_pitch) ||
            fabs(p->return_pitch) > 1.5 || !isfinite(p->resting) || p->resting < 0 ||
            p->resting > 6 || !isfinite(p->nourished) || p->nourished < 0 || p->nourished > 120 ||
            (inside && (!deployed || position_absolute(p->pos).z < 0)) ||
            (!inside && p->resting != 0))
            s->ok = false;
    }
}
void sc_save_powers(Stream *s, Player *players, bool read) {
    for (int i = 0; i < SC_LEGACY_PLAYERS; i++) {
        Player *p = &players[i];
        uint32_t veil = sc_save_number(s, p->cloaked, read);
        p->cloaked = veil != 0;
        p->flight = (float)sc_save_real(s, p->flight, read);
        p->leap = (float)sc_save_real(s, p->leap, read);
        p->nova = (float)sc_save_real(s, p->nova, read);
        p->nova_tick = (float)sc_save_real(s, p->nova_tick, read);
        if (veil > 1 || !isfinite(p->flight) || p->flight < 0 || p->flight > 18 ||
            !isfinite(p->leap) || p->leap < 0 || p->leap > 45 || !isfinite(p->nova) ||
            p->nova < 0 || p->nova > 8 || !isfinite(p->nova_tick) || p->nova_tick < 0 ||
            p->nova_tick > .5 || (p->cloaked && p->nova > 0) || (p->nova == 0 && p->nova_tick != 0))
            s->ok = false;
    }
}
void sc_save_equipment(Stream *s, Player *players, Rig rigs[SC_LEGACY_PLAYERS][2], Flare *flares,
                       bool read) {
    for (int p = 0; p < SC_LEGACY_PLAYERS; p++) {
        Player *v = &players[p];
        v->riding = sc_save_number(s, v->riding, read);
        v->rig_weapon = sc_save_number(s, v->rig_weapon, read);
        v->flare_lifetime = sc_save_number(s, v->flare_lifetime, read);
        v->flare_strength = sc_save_number(s, v->flare_strength, read);
        if (v->riding > WAGON || v->rig_weapon >= SC_MECH_WEAPONS || v->flare_lifetime > 5 ||
            v->flare_strength > 5 ||
            (v->riding &&
             (v->sheltered || v->cloaked || v->flight > 0 || v->leap > 0 || v->nova > 0)))
            s->ok = false;
        for (int k = 0; k < 2; k++) {
            Rig *r = &rigs[p][k];
            r->pos = sc_save_location(s, r->pos, read, false);
            r->armor = (float)sc_save_real(s, r->armor, read);
            uint32_t deployed = sc_save_number(s, r->deployed, read);
            r->deployed = deployed != 0;
            r->yaw = sc_save_real(s, r->yaw, read);
            if (!isfinite(r->yaw) || fabs(r->yaw) > SC_PI + 1e-6 || deployed > 1 ||
                !isfinite(r->armor) || r->armor < 0 ||
                r->armor > game_rig_max_armor(v, (RigKind)(k + 1)) ||
                (deployed && position_absolute(r->pos).z < 0))
                s->ok = false;
            if (v->riding == (unsigned)k + 1 &&
                (!r->deployed || r->armor <= 0 || vlen(position_delta(v->pos, r->pos)) > .01))
                s->ok = false;
        }
    }
    unsigned count = 0;
    if (!read)
        for (int i = 0; i < SC_MAX_FLARES; i++)
            count += flares[i].active;
    count = sc_save_number(s, count, read);
    if (count > SC_MAX_FLARES)
        s->ok = false;
    unsigned at = 0;
    for (unsigned n = 0; s->ok && n < count; n++) {
        if (!read)
            while (!flares[at].active)
                at++;
        Flare *f = &flares[read ? n : at++];
        f->pos = sc_save_location(s, f->pos, read, false);
        f->normal = sc_save_vector(s, f->normal, read);
        f->life = (float)sc_save_real(s, f->life, read);
        f->radius = (float)sc_save_real(s, f->radius, read);
        f->power = (float)sc_save_real(s, f->power, read);
        f->owner = sc_save_number(s, f->owner, read);
        f->active = true;
        if (!sc_save_valid_vector(f->normal) || fabs(vlen(f->normal) - 1) > .01 ||
            !isfinite(f->life) || f->life <= 0 || f->life > 105 || !isfinite(f->radius) ||
            f->radius < 12 || f->radius > 22 || !isfinite(f->power) || f->power < 2.5 ||
            f->power > 6.25 || f->owner >= SC_LEGACY_PLAYERS)
            s->ok = false;
    }
}
