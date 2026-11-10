#include "game.h"
#include <string.h>
static bool exploring(const Game *g, int p) {
    return p >= 0 && p < g->player_count && !g->players[p].sheltered &&
           !g->matrix.divers[p].active && position_absolute(g->players[p].pos).z >= 0;
}
unsigned rhythm_reserved(const Rhythm *r) {
    unsigned total = 0;
    for (unsigned i = 0; i < SC_SEQUENCES; i++)
        total += r->sequences[i].reserved;
    return total;
}
Vec3 rhythm_trace(Cell site, unsigned kind) {
    return vadd(world_center(site), v3(kind == SEQUENCE_DISCOVERY ? 0 : -2, .08, 3));
}
bool rhythm_blocks(const Game *g, Cell site, bool pressure) {
    if (g->world.generator < 16)
        return false;
    if (pressure && g->rhythm.quiet > 0)
        return true;
    for (unsigned i = 0; i < SC_SEQUENCES; i++)
        if (g->rhythm.sequences[i].beat && cell_equal(g->rhythm.sequences[i].site, site))
            return true;
    return false;
}
static Enemy *actor(Game *g, const HiddenSequence *s) {
    if (s->force) {
        FactionForce *f = factions_force(g, s->force);
        return f && s->member < f->count ? &f->members[s->member] : NULL;
    }
    Room *room = world_find(&g->world, s->site);
    return room && room->ready && s->member < (unsigned)room->enemy_count
               ? &room->enemies[s->member]
               : NULL;
}
static bool dead(Game *g, const HiddenSequence *s) {
    if (s->kind == SEQUENCE_DISCOVERY)
        return false;
    if (s->force) {
        FactionForce *f = factions_force(g, s->force);
        return !f || !factions_strength(f) || f->order == ORDER_RETREAT;
    }
    Room *room = world_find(&g->world, s->site);
    if (room && room->ready && s->member < (unsigned)room->enemy_count)
        return !room->enemies[s->member].alive;
    for (size_t i = 0; i < g->world.record_count; i++)
        if (cell_equal(g->world.records[i].cell, s->site))
            return (g->world.records[i].killed & (1u << s->member)) != 0;
    return false;
}
bool rhythm_start(Game *g, SequenceKind kind, int player, Cell site, unsigned member,
                  uint32_t force) {
    HiddenDirector *d = &g->factions.director;
    if (g->world.generator < 16 || !exploring(g, player) || kind < SEQUENCE_HUNTER ||
        kind > SEQUENCE_DISCOVERY || !d->budget || d->proposal.phase || member >= SC_MAX_ENEMIES ||
        factions_safe(g, site) || onboarding_opening(site) ||
        rhythm_blocks(g, site, kind != SEQUENCE_DISCOVERY) ||
        (!g->players[player].kills && !g->campaign.defeated) || g->views[player].lesson ||
        g->drills[player].time > 0 ||
        (g->world.open_layer && campaign_layer(site) > g->world.open_layer) ||
        (kind != SEQUENCE_DISCOVERY && d->recovery[player] > 0))
        return false;
    unsigned slot = 0;
    while (slot < SC_SEQUENCES && g->rhythm.sequences[slot].beat)
        slot++;
    if (slot == SC_SEQUENCES || factions_observed(g, rhythm_trace(site, kind), 40))
        return false;
    const FactionRegion *r = factions_at(&g->factions, site);
    Room *room = world_find(&g->world, site);
    unsigned visited = 0;
    if (r && room)
        for (unsigned z = 0; z < room->span_z; z++)
            for (unsigned x = 0; x < room->span_x; x++) {
                unsigned local = (unsigned)(site.z - r->region.z * 4 + (int)z) * 4 +
                                 (unsigned)(site.x - r->region.x * 4 + (int)x);
                if (local < 16)
                    visited |= 1u << local;
            }
    if (r && ((r->seen & visited) || r->edit))
        return false;
    HiddenSequence next = {.identity = g->rhythm.next_id + 1,
                           .player = (unsigned)player,
                           .member = member,
                           .force = force,
                           .site = site,
                           .kind = kind,
                           .beat = BEAT_TRACE,
                           .reserved = true};
    Enemy *e = actor(g, &next);
    if (kind == SEQUENCE_DISCOVERY) {
        if (!room || !room->ready || room->habitat.pace == PACE_FRONTIER)
            return false;
        for (int i = 0; i < room->enemy_count; i++)
            if (room->enemies[i].alive)
                return false;
    } else if (!e || !e->alive || e->windup > 0 || e->observed ||
               factions_observed(g, enemy_center(e), 48) ||
               (kind == SEQUENCE_SIEGE) != (force != 0) ||
               (kind == SEQUENCE_MACHINE &&
                (sc_enemy_bodies[e->kind].form != ROBOT || !room || !room->journey.family)))
        return false;
    if (!factions_record(g, site))
        return false;
    g->rhythm.next_id++;
    g->rhythm.sequences[slot] = next;
    d->budget--; /* Reserve from the same budget used by ordinary interventions. */
    g->dirty = true;
    return true;
}
static bool witnessed(const Game *g, Vec3 at, double range) {
    for (int p = 0; p < g->player_count; p++) {
        if (!exploring(g, p))
            continue;
        Vec3 ray = vsub(at, player_eye(&g->players[p]));
        double distance = vlen(ray);
        Vec3 ahead = v3(sin(g->players[p].yaw), 0, cos(g->players[p].yaw));
        if (distance < range && (distance < 12 || vdot(vnorm(ray), ahead) > .35) &&
            world_raycast(&g->world, player_eye(&g->players[p]), vnorm(ray), distance) >=
                distance - .2)
            return true;
    }
    return false;
}
static void release(Game *g, HiddenSequence *s, bool refund) {
    if (!s->reserved)
        return;
    s->reserved = false;
    if (refund)
        g->factions.director.budget =
            (uint32_t)fmin(3 - rhythm_reserved(&g->rhythm), g->factions.director.budget + 1);
    else {
        g->factions.director.accepted++;
        g->factions.director.sequence++;
    }
}
static void aftermath(Game *g, HiddenSequence *s) {
    release(g, s, false);
    s->beat = BEAT_QUIET;
    s->clock = 0;
    g->rhythm.quiet = 45;
    g->rhythm.completed++;
    world_set_rhythm_state(&g->world, s->site,
                           world_rhythm_state(&g->world, s->site) | (17u << (s->kind - 1)));
    g->dirty = true;
}
void rhythm_step(Game *g, double dt) {
    if (g->world.generator < 16)
        return;
    bool active = false;
    for (int p = 0; p < g->player_count; p++)
        active |= exploring(g, p);
    if (!active)
        return;
    /* Session suspension supplies no steps. Clamp direct callers: no catch-up bursts. */
    float step = (float)clampd(dt, 0, .25);
    g->rhythm.quiet = fmaxf(0, g->rhythm.quiet - step);
    for (unsigned i = 0; i < SC_SEQUENCES; i++) {
        HiddenSequence *s = &g->rhythm.sequences[i];
        if (!s->beat)
            continue;
        if (s->beat == BEAT_QUIET) {
            s->clock += step;
            if (s->clock >= 45)
                *s = (HiddenSequence){0};
            continue;
        }
        if (dead(g, s)) {
            aftermath(g, s);
            continue;
        }
        bool near = false, recovery = false;
        for (int p = 0; p < g->player_count; p++)
            if (exploring(g, p)) {
                double distance =
                    vlen(vsub(world_center(s->site), position_absolute(g->players[p].pos)));
                near |= distance < 360;
                recovery |= distance < 360 && g->factions.director.recovery[p] > 0;
            }
        s->away = near ? 0 : s->away + step;
        if (s->away >= 30 || (s->reserved && recovery && s->kind != SEQUENCE_DISCOVERY)) {
            release(g, s, true);
            g->rhythm.cancelled++;
            *s = (HiddenSequence){0};
            g->dirty = true;
            continue;
        }
        if (!near)
            continue;
        Vec3 trace = rhythm_trace(s->site, s->kind);
        if (s->beat == BEAT_TRACE) {
            if (!witnessed(g, vadd(trace, v3(0, .4, 0)), 65))
                continue;
            s->witnessed = true;
            s->beat = BEAT_APPROACH;
            s->clock = 0;
            world_set_rhythm_state(&g->world, s->site,
                                   world_rhythm_state(&g->world, s->site) | (1u << (s->kind - 1)));
            g->dirty = true;
        } else if (s->beat == BEAT_APPROACH) {
            s->clock += step;
            if (s->clock < 5)
                continue;
            s->beat = BEAT_PRESSURE;
            s->clock = 0;
        } else {
            Enemy *e = actor(g, s);
            if (s->kind == SEQUENCE_DISCOVERY) {
                FactionRegion *r = (FactionRegion *)factions_at(&g->factions, s->site);
                if (!r)
                    continue;
                if (s->reserved) {
                    r->edit = EDIT_STASH;
                    r->edit_site = s->site;
                    r->edit_claims = 0;
                    release(g, s, false);
                }
                if (r->edit_claims)
                    aftermath(g, s);
            } else if (e && witnessed(g, enemy_center(e), 65)) {
                /* Existing actors keep their identity, health, equipment and commitments. */
                if (s->kind == SEQUENCE_MACHINE &&
                    !(world_systems(&g->world, s->site) & SYSTEM_POWER))
                    continue;
                release(g, s, false);
                s->beat = BEAT_FIGHT;
            }
        }
    }
}
void rhythm_select(Game *g, int player) {
    if (g->world.generator < 16 || !exploring(g, player))
        return;
    g->rhythm.cursor++;
    /* Search only resident actors; selection never generates a room or a campaign. */
    for (int j = 0; j < g->world.count; j++) {
        Room *room = &g->world.rooms[(j + g->rhythm.cursor) % (unsigned)g->world.count];
        if (!room->ready)
            continue;
        Vec3 delta = vsub(world_room_center(room), position_absolute(g->players[player].pos));
        Vec3 ahead = v3(sin(g->players[player].yaw), 0, cos(g->players[player].yaw));
        if (fabs(delta.y) > 20 || vlen(delta) > 200 || vdot(vnorm(delta), ahead) < -.2)
            continue;
        bool empty = true;
        for (int e = 0; e < room->enemy_count; e++)
            if (room->enemies[e].alive) {
                empty = false;
                Enemy *enemy = &room->enemies[e];
                SequenceKind kind =
                    enemy->force ? SEQUENCE_SIEGE
                    : sc_enemy_bodies[enemy->kind].form == ROBOT && room->journey.family
                        ? SEQUENCE_MACHINE
                        : SEQUENCE_HUNTER;
                if (rhythm_start(g, kind, player, room->cell,
                                 enemy->force ? enemy->member : (unsigned)e, enemy->force))
                    return;
            }
        if (empty && rhythm_start(g, SEQUENCE_DISCOVERY, player, room->cell, 0, 0))
            return;
    }
}
float rhythm_intensity(const Game *g, int player) {
    if (!exploring(g, player) || g->world.generator < 16)
        return 0;
    float intensity = g->rhythm.quiet > 0 ? -.5f : 0;
    for (unsigned i = 0; i < SC_SEQUENCES; i++) {
        const HiddenSequence *s = &g->rhythm.sequences[i];
        if (s->beat && s->beat < BEAT_QUIET && s->witnessed &&
            vlen(vsub(world_center(s->site), position_absolute(g->players[player].pos))) < 160)
            intensity = fmaxf(intensity, s->beat * .15f);
    }
    return intensity;
}
bool rhythm_valid(const Rhythm *r, unsigned budget) {
    if (!isfinite(r->quiet) || r->quiet < 0 || r->quiet > 45 || budget + rhythm_reserved(r) > 3)
        return false;
    for (unsigned i = 0; i < SC_SEQUENCES; i++) {
        const HiddenSequence *s = &r->sequences[i];
        if (!s->beat) {
            if (s->reserved)
                return false;
            continue;
        }
        if (!s->identity || s->identity > r->next_id || s->kind < SEQUENCE_HUNTER ||
            s->kind > SEQUENCE_DISCOVERY || s->beat > BEAT_QUIET || s->player >= SC_PLAYERS ||
            s->member >= SC_MAX_ENEMIES || s->site.z < 0 || !isfinite(s->clock) || s->clock < 0 ||
            s->clock > 45 || !isfinite(s->away) || s->away < 0 || s->away > 30 ||
            (s->beat >= BEAT_FIGHT && s->reserved))
            return false;
        for (unsigned j = 0; j < i; j++)
            if (r->sequences[j].beat && (r->sequences[j].identity == s->identity ||
                                         cell_equal(r->sequences[j].site, s->site)))
                return false;
    }
    return true;
}
