#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* A versioned little-endian stream, independent of C struct layout and platform. */
typedef struct {
    FILE *file;
    uint32_t hash;
    bool ok;
} Stream;
static void bytes(Stream *s, void *data, size_t n, bool read) {
    if (!s->ok)
        return;
    size_t done = read ? fread(data, 1, n, s->file) : fwrite(data, 1, n, s->file);
    if (done != n) {
        s->ok = false;
        return;
    }
    const unsigned char *b = data;
    for (size_t i = 0; i < n; i++) {
        s->hash ^= b[i];
        s->hash *= UINT32_C(16777619);
    }
}
static uint32_t number(Stream *s, uint32_t n, bool read) {
    unsigned char b[4];
    for (int i = 0; i < 4; i++)
        b[i] = (unsigned char)(n >> (i * 8));
    bytes(s, b, 4, read);
    n = 0;
    for (int i = 0; i < 4; i++)
        n |= (uint32_t)b[i] << (i * 8);
    return n;
}
static double real(Stream *s, double d, bool read) {
    uint64_t bits = 0;
    memcpy(&bits, &d, 8);
    uint32_t lo = number(s, (uint32_t)bits, read), hi = number(s, (uint32_t)(bits >> 32), read);
    bits = (uint64_t)lo | ((uint64_t)hi << 32);
    memcpy(&d, &bits, 8);
    return d;
}
static Vec3 vector(Stream *s, Vec3 v, bool read) {
    v.x = real(s, v.x, read);
    v.y = real(s, v.y, read);
    v.z = real(s, v.z, read);
    return v;
}
static bool valid_vector(Vec3 v) {
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z) && fabs(v.x) < 9e8 && fabs(v.y) < 3e8 &&
           fabs(v.z) < 9e8;
}
static Position location(Stream *s, Position p, bool read, bool legacy) {
    if (legacy) {
        Vec3 v = vector(s, position_absolute(p), read);
        if (!valid_vector(v)) {
            s->ok = false;
            return (Position){0};
        }
        return position_from(v);
    }
    p.sector.x = (int32_t)number(s, (uint32_t)p.sector.x, read);
    p.sector.y = (int32_t)number(s, (uint32_t)p.sector.y, read);
    p.sector.z = (int32_t)number(s, (uint32_t)p.sector.z, read);
    p.local = vector(s, p.local, read);
    if (!valid_vector(p.local) || p.local.x < -48 || p.local.x >= 48 || p.local.y < 0 ||
        p.local.y >= SC_DECK || p.local.z < -48 || p.local.z >= 48 ||
        !valid_vector(position_absolute(p)))
        s->ok = false;
    return p;
}
static void profile(Stream *s, Player *p, bool read, unsigned version) {
    bool legacy = version == 1;
    p->pos = location(s, p->pos, read, legacy);
    p->checkpoint = location(s, p->checkpoint, read, legacy);
    p->yaw = real(s, p->yaw, read);
    p->pitch = real(s, p->pitch, read);
    p->level = number(s, p->level, read);
    p->xp = number(s, p->xp, read);
    p->kills = number(s, p->kills, read);
    p->points = number(s, p->points, read);
    p->power = number(s, p->power, read);
    p->vitality = number(s, p->vitality, read);
    p->haste = number(s, p->haste, read);
    p->tint = number(s, p->tint, read);
    p->health = (float)real(s, p->health, read);
    if (version >= 3) {
        for (int i = 0; i < 7; i++)
            p->mods[i] = number(s, p->mods[i], read);
        p->weapons = number(s, p->weapons, read);
        p->weapon = number(s, p->weapon, read);
        p->credits = number(s, p->credits, read);
        p->logs = number(s, p->logs, read);
        p->victories = number(s, p->victories, read);
        p->pickups = number(s, p->pickups, read);
        p->shield = (float)real(s, p->shield, read);
        p->armor = (float)real(s, p->armor, read);
        p->energy = (float)real(s, p->energy, read);
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
        if (!valid_vector(position_absolute(p->pos)) ||
            !valid_vector(position_absolute(p->checkpoint)) || !isfinite(p->yaw) ||
            !isfinite(p->pitch) || p->level < 1 || p->level > 1000 || p->xp > 100000 ||
            p->points > 1000 || p->power > 50 || p->vitality > 50 || p->haste > 50 ||
            !isfinite(p->health) || p->health <= 0 || p->health > player_max_health(p) ||
            fabs(p->pitch) > 1.5 || fabs(p->yaw) > SC_PI + 1e-6)
            s->ok = false;
        p->crouched = true;
        p->flashlight = true;
    }
}
/* The v4 appendix preserves the v3 profile/record layout and expedition. */
static void sanctuary(Stream *s, Shelter *shelter, Player *players, bool read) {
    uint32_t deployed = number(s, shelter->deployed, read);
    shelter->deployed = deployed != 0;
    shelter->door = location(s, shelter->door, read, false);
    shelter->yaw = real(s, shelter->yaw, read);
    if (deployed > 1 || !isfinite(shelter->yaw) || fabs(shelter->yaw) > SC_PI + 1e-6 ||
        (deployed && position_absolute(shelter->door).z < 0))
        s->ok = false;
    for (int i = 0; i < SC_PLAYERS; i++) {
        Player *p = &players[i];
        uint32_t inside = number(s, p->sheltered, read);
        p->sheltered = inside != 0;
        p->shelter_pos = vector(s, p->shelter_pos, read);
        p->return_yaw = real(s, p->return_yaw, read);
        p->return_pitch = real(s, p->return_pitch, read);
        p->resting = (float)real(s, p->resting, read);
        p->nourished = (float)real(s, p->nourished, read);
        if (inside > 1 || !shelter_clear(p->shelter_pos) || !isfinite(p->return_yaw) ||
            fabs(p->return_yaw) > SC_PI + 1e-6 || !isfinite(p->return_pitch) ||
            fabs(p->return_pitch) > 1.5 || !isfinite(p->resting) || p->resting < 0 ||
            p->resting > 6 || !isfinite(p->nourished) || p->nourished < 0 || p->nourished > 120 ||
            (inside && (!deployed || position_absolute(p->pos).z < 0)) ||
            (!inside && p->resting != 0))
            s->ok = false;
    }
}
static void powers(Stream *s, Player *players, bool read) {
    for (int i = 0; i < SC_PLAYERS; i++) {
        Player *p = &players[i];
        uint32_t veil = number(s, p->cloaked, read);
        p->cloaked = veil != 0;
        p->flight = (float)real(s, p->flight, read);
        p->leap = (float)real(s, p->leap, read);
        p->nova = (float)real(s, p->nova, read);
        p->nova_tick = (float)real(s, p->nova_tick, read);
        if (veil > 1 || !isfinite(p->flight) || p->flight < 0 || p->flight > 18 ||
            !isfinite(p->leap) || p->leap < 0 || p->leap > 45 || !isfinite(p->nova) ||
            p->nova < 0 || p->nova > 8 || !isfinite(p->nova_tick) || p->nova_tick < 0 ||
            p->nova_tick > .5 || (p->cloaked && p->nova > 0) || (p->nova == 0 && p->nova_tick != 0))
            s->ok = false;
    }
}
static void equipment(Stream *s, Player *players, Rig rigs[SC_PLAYERS][2], Flare *flares,
                      bool read) {
    for (int p = 0; p < SC_PLAYERS; p++) {
        Player *v = &players[p];
        v->riding = number(s, v->riding, read);
        v->rig_weapon = number(s, v->rig_weapon, read);
        v->flare_lifetime = number(s, v->flare_lifetime, read);
        v->flare_strength = number(s, v->flare_strength, read);
        if (v->riding > WAGON || v->rig_weapon >= SC_MECH_WEAPONS || v->flare_lifetime > 5 ||
            v->flare_strength > 5 ||
            (v->riding &&
             (v->sheltered || v->cloaked || v->flight > 0 || v->leap > 0 || v->nova > 0)))
            s->ok = false;
        for (int k = 0; k < 2; k++) {
            Rig *r = &rigs[p][k];
            r->pos = location(s, r->pos, read, false);
            r->armor = (float)real(s, r->armor, read);
            uint32_t deployed = number(s, r->deployed, read);
            r->deployed = deployed != 0;
            r->yaw = real(s, r->yaw, read);
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
    count = number(s, count, read);
    if (count > SC_MAX_FLARES)
        s->ok = false;
    unsigned at = 0;
    for (unsigned n = 0; s->ok && n < count; n++) {
        if (!read)
            while (!flares[at].active)
                at++;
        Flare *f = &flares[read ? n : at++];
        f->pos = location(s, f->pos, read, false);
        f->normal = vector(s, f->normal, read);
        f->life = (float)real(s, f->life, read);
        f->radius = (float)real(s, f->radius, read);
        f->power = (float)real(s, f->power, read);
        f->owner = number(s, f->owner, read);
        f->active = true;
        if (!valid_vector(f->normal) || fabs(vlen(f->normal) - 1) > .01 || !isfinite(f->life) ||
            f->life <= 0 || f->life > 105 || !isfinite(f->radius) || f->radius < 12 ||
            f->radius > 22 || !isfinite(f->power) || f->power < 2.5 || f->power > 6.25 ||
            f->owner >= SC_PLAYERS)
            s->ok = false;
    }
}
/* Each format reads its original scene count; format 9 adds reactive state. */
static void narrative(Stream *s, Story *story, bool read, unsigned version) {
    story->version = number(s, story->version, read);
    story->assigned = number(s, story->assigned, read);
    story->seen = number(s, story->seen, read);
    story->done = number(s, story->done, read);
    story->tracked = number(s, story->tracked, read);
    story->choice = number(s, story->choice, read);
    for (int i = 0; i < (version < 8 ? 16 : version < 9 ? 21 : SC_STORY_SCENES); i++) {
        story->sites[i].x = (int32_t)number(s, (uint32_t)story->sites[i].x, read);
        story->sites[i].y = (int32_t)number(s, (uint32_t)story->sites[i].y, read);
        story->sites[i].z = (int32_t)number(s, (uint32_t)story->sites[i].z, read);
    }
}
static void aftermath(Stream *s, Story *story, Player *players, RoomRecord *records, size_t count,
                      bool read, unsigned version) {
    for (int i = 0; i < 2; i++)
        story->support[i] = number(s, story->support[i], read);
    story->council = number(s, story->council, read);
    story->chapter = number(s, story->chapter, read);
    story->arc = number(s, story->arc, read);
    story->chapter_choice = number(s, story->chapter_choice, read);
    story->remaining = number(s, story->remaining, read);
    for (int i = 0; i < SC_OUTCOMES; i++)
        story->last[i] = number(s, story->last[i], read);
    for (int i = 0; i < SC_OUTCOMES; i++)
        story->totals[i] = number(s, story->totals[i], read);
    story->event_count = number(s, story->event_count, read);
    for (int i = 0; i < SC_STORY_EVENTS; i++) {
        StoryEvent *e = &story->events[i];
        e->chapter = number(s, e->chapter, read);
        e->scene = number(s, e->scene, read);
        e->choice = number(s, e->choice, read);
        e->arc = number(s, e->arc, read);
        e->site.x = (int32_t)number(s, (uint32_t)e->site.x, read);
        e->site.y = (int32_t)number(s, (uint32_t)e->site.y, read);
        e->site.z = (int32_t)number(s, (uint32_t)e->site.z, read);
        for (int k = 0; k < 4; k++)
            e->outcome[k] = number(s, e->outcome[k], read);
    }
    for (int p = 0; p < SC_PLAYERS; p++)
        for (int k = 0; k < SC_OUTCOMES; k++) {
            players[p].career[k] = number(s, players[p].career[k], read);
            if (read && players[p].career[k] > 1000000000)
                s->ok = false;
        }
    for (size_t i = 0; s->ok && i < count; i++) {
        records[i].systems = number(s, records[i].systems, read);
        if (read &&
            (records[i].systems >= 16 ||
             ((records[i].systems & SYSTEM_POWER) && (records[i].systems & SYSTEM_BLACKOUT)) ||
             (records[i].systems && story->version < 3 && version < 10)))
            s->ok = false;
    }
}
/* Format 10 appends matrix state without changing any earlier payload offsets. */
static Cell matrix_cell(Stream *s, Cell c, bool read) {
    c.x = (int32_t)number(s, (uint32_t)c.x, read);
    c.y = (int32_t)number(s, (uint32_t)c.y, read);
    c.z = (int32_t)number(s, (uint32_t)c.z, read);
    return c;
}
static bool matrix_bool(Stream *s, bool value, bool read) {
    uint32_t n = number(s, value, read);
    if (n > 1)
        s->ok = false;
    return n != 0;
}
static void matrix_stream(Stream *s, MatrixState *m, bool read) {
    m->version = number(s, m->version, read);
    uint32_t count = number(s, (uint32_t)m->count, read);
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
        r.key = matrix_cell(s, r.key, read);
        for (int j = 0; j < SC_MATRIX_SERVICES; j++)
            r.done[j] = number(s, r.done[j], read);
        r.keys = number(s, r.keys, read);
        r.killed = number(s, r.killed, read);
        r.loot = number(s, r.loot, read);
        r.flags = number(s, r.flags, read);
        r.seen = number(s, r.seen, read);
        r.outcome = number(s, r.outcome, read);
        r.alert = number(s, r.alert, read);
        if (read)
            m->records[i] = r;
    }
    for (int i = 0; i < SC_PLAYERS; i++) {
        MatrixProfile *p = &m->profiles[i];
        MatrixDiver *d = &m->divers[i];
        p->programs = number(s, p->programs, read);
        for (int j = 0; j < 2; j++)
            p->slots[j] = number(s, p->slots[j], read);
        for (int j = 0; j < SC_MATRIX_PROGRAMS; j++)
            p->upgrades[j] = number(s, p->upgrades[j], read);
        p->data = number(s, p->data, read);
        p->dives = number(s, p->dives, read);
        p->knowledge = number(s, p->knowledge, read);
        d->key = matrix_cell(s, d->key, read);
        d->terminal = matrix_cell(s, d->terminal, read);
        d->blocked = matrix_cell(s, d->blocked, read);
        d->pos = vector(s, d->pos, read);
        d->velocity = vector(s, d->velocity, read);
        d->echo = vector(s, d->echo, read);
        d->body_yaw = real(s, d->body_yaw, read);
        d->body_pitch = real(s, d->body_pitch, read);
        d->integrity = (float)real(s, d->integrity, read);
        d->bandwidth = (float)real(s, d->bandwidth, read);
        d->trace = (float)real(s, d->trace, read);
        d->cooldown = (float)real(s, d->cooldown, read);
        d->hurt = (float)real(s, d->hurt, read);
        d->veil = (float)real(s, d->veil, read);
        d->siphon = (float)real(s, d->siphon, read);
        d->grace = (float)real(s, d->grace, read);
        d->counter = (float)real(s, d->counter, read);
        d->lockout = (float)real(s, d->lockout, read);
        d->echo_life = (float)real(s, d->echo_life, read);
        d->network = number(s, d->network, read);
        d->selected = number(s, d->selected, read);
        d->service = number(s, d->service, read);
        d->unbanked = number(s, d->unbanked, read);
        d->active = matrix_bool(s, d->active, read);
        d->map = matrix_bool(s, d->map, read);
        d->choice = matrix_bool(s, d->choice, read);
        d->grounded = matrix_bool(s, d->grounded, read);
    }
    for (int i = 0; i < 2; i++) {
        MatrixNetwork *w = &m->networks[i];
        w->active = matrix_bool(s, w->active, read);
        w->key = matrix_cell(s, w->key, read);
        for (int j = 0; j < 2; j++)
            w->switches[j] = (float)real(s, w->switches[j], read);
        w->bridge = (float)real(s, w->bridge, read);
        w->gate = (float)real(s, w->gate, read);
    }
    for (int i = 0; i < SC_MATRIX_CONTROLS; i++) {
        MatrixControl *c = &m->controls[i];
        c->room = matrix_cell(s, c->room, read);
        c->life = (float)real(s, c->life, read);
        c->kind = number(s, c->kind, read);
        c->owner = number(s, c->owner, read);
    }
}
static void campaign_stream(Stream *s, Campaign *c, bool read) {
    c->version = number(s, c->version, read);
    c->defeated = number(s, c->defeated, read);
    c->tracked = number(s, c->tracked, read);
    for (int i = 0; i < SC_GUARDIANS; i++) {
        c->eligible[i] = number(s, c->eligible[i], read);
        c->attempts[i].active = matrix_bool(s, c->attempts[i].active, read);
        c->attempts[i].participants = number(s, c->attempts[i].participants, read);
    }
    for (int i = 0; i < SC_PLAYERS; i++) {
        CampaignProfile *p = &c->profiles[i];
        p->powers = number(s, p->powers, read);
        p->rigs = number(s, p->rigs, read);
        p->modules = number(s, p->modules, read);
        p->claimed = number(s, p->claimed, read);
        for (int k = 0; k < 2; k++) {
            p->guns[k] = number(s, p->guns[k], read);
            p->power_slots[k] = number(s, p->power_slots[k], read);
        }
        p->rig = number(s, p->rig, read);
        p->module = number(s, p->module, read);
    }
}
bool game_save(const Game *g, const char *path) {
    if (!path || g->world.record_count > 1000000 || g->matrix.count > SC_MATRIX_RECORDS)
        return false;
    size_t len = strlen(path);
    char *tmp = malloc(len + 5);
    if (!tmp)
        return false;
    memcpy(tmp, path, len);
    memcpy(tmp + len, ".tmp", 5);
    FILE *f = fopen(tmp, "wb");
    if (!f) {
        free(tmp);
        return false;
    }
    Stream s = {f, UINT32_C(2166136261), true};
    number(&s, UINT32_C(0x5343524c), false);
    number(&s, 12, false);
    number(&s, g->world.seed, false);
    number(&s, g->world.generator, false);
    /* A saved session always opens in solo; the other profile remains available. */
    for (int i = 0; i < SC_PLAYERS; i++) {
        Player p = g->players[i];
        if (p.lift_active)
            p.pos = p.lift_from;
        profile(&s, &p, false, 10);
    }
    number(&s, g->mission, false);
    number(&s, g->mission_kills, false);
    number(&s, g->mission_loot, false);
    number(&s, g->contracts, false);
    number(&s, (uint32_t)g->world.record_count, false);
    for (size_t i = 0; i < g->world.record_count; i++) {
        const RoomRecord *r = &g->world.records[i];
        number(&s, (uint32_t)r->cell.x, false);
        number(&s, (uint32_t)r->cell.y, false);
        number(&s, (uint32_t)r->cell.z, false);
        number(&s, r->killed, false);
        number(&s, r->collected, false);
    }
    Shelter shelter = g->shelter;
    Player players[SC_PLAYERS];
    memcpy(players, g->players, sizeof(players));
    for (int i = 0; i < SC_PLAYERS; i++)
        if (players[i].lift_active)
            players[i].pos = players[i].lift_from;
    sanctuary(&s, &shelter, players, false);
    powers(&s, players, false);
    Rig rigs[SC_PLAYERS][2];
    Flare flares[SC_MAX_FLARES];
    memcpy(rigs, g->rigs, sizeof(rigs));
    for (int i = 0; i < SC_PLAYERS; i++)
        if (players[i].lift_active && players[i].riding)
            rigs[i][players[i].riding - 1].pos = players[i].lift_from;
    memcpy(flares, g->flares, sizeof(flares));
    equipment(&s, players, rigs, flares, false);
    Story story = g->story;
    narrative(&s, &story, false, 10);
    aftermath(&s, &story, players, g->world.records, g->world.record_count, false, 10);
    MatrixState *matrix = malloc(sizeof(*matrix));
    if (!matrix)
        s.ok = false;
    else {
        *matrix = g->matrix;
        matrix_stream(&s, matrix, false);
        free(matrix);
    }
    Campaign campaign = g->campaign;
    campaign_stream(&s, &campaign, false);
    for (size_t i = 0; i < g->world.record_count; i++)
        number(&s, g->world.records[i].local, false);
    uint32_t hash = s.hash;
    number(&s, hash, false);
    if (fflush(f) != 0)
        s.ok = false;
    if (fclose(f) != 0)
        s.ok = false;
    bool ok = false;
    if (s.ok) {
#ifdef _WIN32
        ok = MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        ok = rename(tmp, path) == 0;
#endif
    }
    if (!ok)
        remove(tmp);
    free(tmp);
    return ok;
}
bool game_load(Game *g, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    Stream s = {f, UINT32_C(2166136261), true};
    uint32_t magic = number(&s, 0, true), version = number(&s, 0, true), seed = number(&s, 0, true);
    uint32_t generator = version >= 2 ? number(&s, 0, true) : 1;
    if (magic != UINT32_C(0x5343524c) || (version < 1 || version > 12) ||
        (version == 2 && generator != 2) || (version < 11 && generator > 6) ||
        (version < 12 && generator > 10) ||
        (version >= 3 && (generator < SC_OLDEST_GENERATOR || generator > SC_GENERATOR_VERSION)))
        s.ok = false;
    Player players[SC_PLAYERS];
    memset(players, 0, sizeof(players));
    for (int i = 0; i < SC_PLAYERS; i++)
        profile(&s, &players[i], true, version);
    uint32_t mission = 0, kills = 0, loot = 0, contracts = 0;
    if (version >= 3) {
        mission = number(&s, 0, true);
        kills = number(&s, 0, true);
        loot = number(&s, 0, true);
        contracts = number(&s, 0, true);
        if (mission > 6 || contracts > 7 || kills > 1000000000 || loot > 1000000000)
            s.ok = false;
    }
    uint32_t count = number(&s, 0, true);
    if (count > 1000000)
        s.ok = false;
    RoomRecord *records = NULL;
    if (s.ok && count) {
        records = calloc(count, sizeof(*records));
        if (!records)
            s.ok = false;
    }
    for (uint32_t i = 0; s.ok && i < count; i++) {
        RoomRecord *r = &records[i];
        r->cell.x = (int32_t)number(&s, 0, true);
        r->cell.y = (int32_t)number(&s, 0, true);
        r->cell.z = (int32_t)number(&s, 0, true);
        r->killed = number(&s, 0, true);
        r->collected = version >= 3 ? number(&s, 0, true) : 0;
        if (r->collected >= 32)
            s.ok = false;
        if (r->cell.x < -10000000 || r->cell.x > 10000000 || r->cell.y < -10000000 ||
            r->cell.y > 10000000 || r->cell.z < 0 || r->cell.z > 10000000 ||
            r->killed >= (UINT32_C(1) << SC_MAX_ENEMIES))
            s.ok = false;
    }
    Shelter shelter = {0};
    if (version >= 4)
        sanctuary(&s, &shelter, players, true);
    if (version >= 5)
        powers(&s, players, true);
    Rig rigs[SC_PLAYERS][2] = {0};
    Flare flares[SC_MAX_FLARES] = {0};
    if (version >= 6)
        equipment(&s, players, rigs, flares, true);
    else
        for (int p = 0; p < SC_PLAYERS; p++) {
            players[p].weapons |= SC_NEW_WEAPONS;
            for (int k = 0; k < 2; k++)
                rigs[p][k] = (Rig){players[p].pos,
                                   game_rig_max_armor(&players[p], (RigKind)(k + 1)), false, 0};
        }
    Story story = {0};
    if (version >= 7) {
        narrative(&s, &story, true, version);
        if (version >= 9)
            aftermath(&s, &story, players, records, count, true, version);
        if (version < 10)
            for (unsigned i = 0; i < story.event_count && i < SC_STORY_EVENTS; i++)
                if (story.events[i].scene >= SC_STORY_SCENES)
                    s.ok = false;
        if (!story_valid(&story, seed, generator, mission))
            s.ok = false;
    }
    MatrixState *matrix = calloc(1, sizeof(*matrix));
    if (!matrix)
        s.ok = false;
    else {
        matrix_init(matrix);
        if (version >= 10)
            matrix_stream(&s, matrix, true);
    }
    Campaign campaign = {0};
    if (version >= 11)
        campaign_stream(&s, &campaign, true);
    for (uint32_t i = 0; s.ok && version >= 12 && i < count; i++) {
        RoomRecord *r = &records[i];
        r->local = number(&s, 0, true);
        uint32_t claims = (r->local >> 4) & 3, eligible = (r->local >> 2) & 3;
        if (r->local >= 64 || (r->local && generator < 11) || (claims & ~eligible) ||
            (claims && !(r->local & FRONTIER_CLEARED)) ||
            ((r->local & FRONTIER_CLEARED) && (r->killed & 15) != 15))
            s.ok = false;
    }
    uint32_t expected = s.hash, actual = number(&s, 0, true);
    if (actual != expected || fgetc(f) != EOF)
        s.ok = false;
    fclose(f);
    if (!s.ok) {
        if (matrix) {
            matrix_destroy(matrix);
            free(matrix);
        }
        free(records);
        return false;
    }
    if (version < 3) {
        /* Incompatible geometry must not reinterpret old doors, kills or positions.
           Keep both profiles and visibly begin a new expedition instead. */
        free(records);
        records = NULL;
        count = 0;
        seed = world_hash(seed, (Cell){0, 0, 0}, UINT32_C(0x4d494752));
        story_init(&story);
        for (int i = 0; i < SC_PLAYERS; i++) {
            players[i].pos = position_from(v3(i * 1.4, 4, -34));
            players[i].checkpoint = position_from(v3(0, .02, 12));
            players[i].health = player_max_health(&players[i]);
            players[i].yaw = 0;
            players[i].pitch = -.04;
        }
    }
    World *replacement = malloc(sizeof(*replacement));
    if (!replacement) {
        matrix_destroy(matrix);
        free(matrix);
        free(records);
        return false;
    }
    world_init(replacement, seed);
    replacement->generator = version >= 3 ? generator : 6;
    replacement->records = records;
    replacement->record_count = count;
    replacement->record_cap = count;
    Vec3 start = position_absolute(players[0].pos);
    if (!campaign_valid(&campaign, players, replacement) || !matrix_index(matrix) ||
        !matrix_rehydrate(matrix, seed, replacement->generator) ||
        !matrix_valid(matrix, seed, replacement->generator, players, &shelter) ||
        !world_index_records(replacement) || !world_refresh(replacement, &start, 1)) {
        matrix_destroy(matrix);
        free(matrix);
        world_destroy(replacement);
        free(replacement);
        return false;
    }
    matrix_destroy(&g->matrix);
    g->matrix = *matrix;
    free(matrix);
    world_destroy(&g->world);
    g->world = *replacement;
    free(replacement);
    memcpy(g->players, players, sizeof(players));
    for (int i = 0; i < SC_PLAYERS; i++)
        if (g->matrix.divers[i].active && g->matrix.divers[i].choice) {
            g->players[i].journal = true;
            g->players[i].journal_page = 4;
        }
    g->shelter = shelter;
    memcpy(g->rigs, rigs, sizeof(rigs));
    memcpy(g->flares, flares, sizeof(flares));
    memset(g->beams, 0, sizeof(g->beams));
    g->player_count = 1;
    memset(g->projectiles, 0, sizeof(g->projectiles));
    game_deaths_reset(g);
    g->mission = mission;
    g->mission_kills = kills;
    g->mission_loot = loot;
    g->contracts = contracts;
    g->story = story;
    g->campaign = campaign;
    memset(g->drills, 0, sizeof(g->drills));
    campaign_resume(g);
    Vec3 resumed = position_absolute(g->players[0].pos);
    world_refresh(&g->world, &resumed, 1);
    g->dirty = version < 6;
    game_message(g, version < 3 ? "SHIP GENERATOR UPDATED / CHARACTERS RETAINED / NEW EXPEDITION"
                                : "EXPEDITION RESUMED / WELCOME BACK");
    return true;
}
