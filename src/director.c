#include "game.h"
#include <stdio.h>
#include <string.h>

void director_init(Director *d, unsigned generator) {
    memset(d, 0, sizeof(*d));
    d->version = generator >= 13;
    if (d->version) {
        d->cooldown = 30000;
        d->budget = 2;
    }
}
static bool valid_cell(Cell c) {
    return c.x >= -10000000 && c.x <= 10000000 && c.y >= -10000000 && c.y <= 10000000 && c.z >= 0 &&
           c.z <= 10000000;
}
bool director_valid(const Director *d, unsigned generator) {
    if (d->version != (generator >= 13) || d->sequence > 1000000000 || d->cooldown > 180000 ||
        d->budget > 3 || d->recharge >= 30000 || d->quiet > 120000 || d->kind >= EVENTS ||
        d->remaining > 180000 || d->claimed > 3 || (!!d->kind != !!d->remaining) ||
        (!d->kind && d->claimed) || !valid_cell(d->site))
        return false;
    for (int p = 0; p < 2; p++)
        if (d->seen[p] > 1 || !valid_cell(d->last_room[p]))
            return false;
    for (int i = 0; i < SC_DIRECTOR_LOG; i++)
        if (d->history[i] >= EVENTS || d->reasons[i] >= DIRECTOR_REASONS ||
            (!d->version && (d->history[i] || d->reasons[i])))
            return false;
    return d->version || !(d->sequence || d->cooldown || d->budget || d->recharge || d->quiet ||
                           d->kind || d->remaining || d->claimed || d->seen[0] || d->seen[1]);
}
const char *director_name(unsigned kind) {
    static const char *names[] = {"NO DISPATCH", "FIELD SUPPLY CACHE", "DIPLOMATIC COURIER",
                                  "ARCHITECTURAL SURVEY", "BORDER CONFLICT"};
    return names[kind % EVENTS];
}
const char *director_reason(unsigned reason) {
    static const char *names[] = {"RECOVERY AFTER PRESSURE", "UNRESOLVED COURT", "NEW LANDMARK",
                                  "OPTIONAL CHALLENGE"};
    return names[reason % DIRECTOR_REASONS];
}
Vec3 director_point(const Director *d) {
    return vadd(world_center(d->site), v3(0, .02, 2));
}
static bool pressure(const Game *g, const Room *r, Vec3 point) {
    for (int i = 0; i < r->enemy_count; i++) {
        const Enemy *e = &r->enemies[i];
        if (e->alive && politics_threat(g, e) && !e->allegiance && !encounter_passive(r, i) &&
            vlen(vsub(enemy_center(e), point)) < 22)
            return true;
    }
    return r->frontier_active || campaign_guardian(r->cell) >= 0 || g->world.pending > 0;
}
static uint32_t down(uint32_t value, uint32_t delta) {
    return value > delta ? value - delta : 0;
}
void director_step(Game *g, double dt) {
    Director *d = &g->director;
    if (!d->version)
        return;
    uint32_t ms = (uint32_t)(dt * 1000 + .5);
    bool active = false, stress = false;
    for (int p = 0; p < g->player_count; p++) {
        const Player *player = &g->players[p];
        if (player->sheltered || player->lift_active || g->matrix.divers[p].active ||
            position_absolute(player->pos).z < 0)
            continue;
        active = true;
        stress |= player->hurt > 0 || player->health < player_max_health(player) * .55f ||
                  player->energy < 20;
    }
    if (!active)
        return; /* Reading/paused frames do not call game_step at all. */
    g->dirty = true;
    d->cooldown = down(d->cooldown, ms);
    d->quiet = stress ? 0 : (uint32_t)fmin(120000, d->quiet + ms);
    d->recharge += ms;
    if (d->recharge >= 30000) {
        d->recharge -= 30000;
        if (d->budget < 3)
            d->budget++;
    }
    if (d->kind) {
        d->remaining = down(d->remaining, ms);
        if (!d->remaining) {
            d->kind = 0;
            d->claimed = 0;
        }
        return;
    }
    if (d->cooldown || !d->budget)
        return;
    for (int p = 0; p < g->player_count; p++) {
        const Player *player = &g->players[p];
        if (player->sheltered || player->lift_active || g->matrix.divers[p].active ||
            position_absolute(player->pos).z < 0)
            continue;
        Room *r = world_find(&g->world, world_cell(position_absolute(player->pos)));
        if (!r || !r->ready || r->type == BAY || r->habitat.court ||
            (d->seen[p] && cell_equal(r->cell, d->last_room[p])))
            continue;
        if (pressure(g, r, position_absolute(player->pos)))
            continue;
        unsigned last = d->sequence ? d->history[(d->sequence - 1) % SC_DIRECTOR_LOG] : 0;
        unsigned kind = EVENT_NONE, reason = REASON_RECOVERY;
        unsigned country = politics_owner(g, r->cell);
        bool war = false;
        if (country)
            for (unsigned other = 0; other < SC_COUNTRIES; other++)
                war |= politics_relation(g, country - 1, other) == REL_WAR;
        if (stress && last != EVENT_SUPPLIES)
            kind = EVENT_SUPPLIES;
        else if (!stress && country && !g->politics.countries[country - 1].stage &&
                 last != EVENT_COURIER) {
            kind = EVENT_COURIER;
            reason = REASON_DIPLOMACY;
        } else if (!stress && r->habitat.reveal && last != EVENT_REVEAL) {
            kind = EVENT_REVEAL;
            reason = REASON_VARIETY;
        } else if (!stress && war && d->quiet >= 20000 && d->budget >= 2 &&
                   r->habitat.pace == PACE_FRONTIER &&
                   !(world_local_state(&g->world, r) & FRONTIER_CLEARED) &&
                   last != EVENT_CONFLICT) {
            kind = EVENT_CONFLICT;
            reason = REASON_CHALLENGE;
        }
        if (!kind)
            continue;
        d->site = r->cell;
        d->kind = kind;
        d->remaining = 180000;
        d->claimed = 0;
        d->last_room[p] = r->cell;
        d->seen[p] = 1;
        d->history[d->sequence % SC_DIRECTOR_LOG] = kind;
        d->reasons[d->sequence % SC_DIRECTOR_LOG] = reason;
        d->sequence = d->sequence >= 1000000000 ? SC_DIRECTOR_LOG : d->sequence + 1;
        d->budget -= kind == EVENT_CONFLICT ? 2 : 1;
        d->cooldown = 60000;
        char message[128];
        snprintf(message, sizeof(message),
                 "%s / CYAN DISPATCH SEAL IN THIS ROOM / G / AVAILABLE 3 MINUTES",
                 director_name(kind));
        game_message(g, message);
        g->dirty = true;
        return;
    }
}
bool director_interact(Game *g, int player) {
    Director *d = &g->director;
    if (!d->version || !d->kind || player < 0 || player >= g->player_count)
        return false;
    Player *p = &g->players[player];
    if (p->sheltered || p->riding || p->lift_active || g->matrix.divers[player].active ||
        (d->claimed & (1u << player)) ||
        vlen(vsub(position_absolute(p->pos), director_point(d))) >= 3)
        return false;
    Room *r = world_find(&g->world, d->site);
    if (!r || !r->ready || pressure(g, r, director_point(d))) {
        game_message(g, "DISPATCH SEAL / CLEAR NEARBY HOSTILES FIRST");
        return true;
    }
    unsigned kind = d->kind;
    if (kind == EVENT_SUPPLIES) {
        p->health = fminf(player_max_health(p), p->health + 35);
        p->shield = fminf(player_max_shield(p), p->shield + 25);
        p->energy = fminf(player_max_energy(p), p->energy + 30);
        game_message(g, "FIELD SUPPLIES / +35 HEALTH +25 SHIELD +30 ENERGY");
    } else if (kind == EVENT_COURIER) {
        unsigned country = politics_owner(g, d->site);
        if (country) {
            g->politics.countries[country - 1].met = 1;
            politics_journal(g, player);
            g->politics.reading[player] = country - 1;
        }
        game_message(g, "COURIER INVITATION / U TO TRACK THE COURT / L TO READ OTHER COURTS");
    } else if (kind == EVENT_REVEAL) {
        p->credits = (uint32_t)fmin(1000000000, p->credits + 25);
        game_message(g,
                     "SURVEY RECORDED / +25 SALVAGE / LOOK UP AT THE DISTRICT'S FOCAL STRUCTURE");
    }
    d->claimed |= 1u << player;
    if (d->claimed == ((1u << g->player_count) - 1)) {
        d->kind = 0;
        d->remaining = 0;
        d->claimed = 0;
    }
    g->dirty = true;
    return kind != EVENT_CONFLICT; /* The same USE can accept the existing optional fight. */
}
