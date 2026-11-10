#include "game.h"
#include <stdio.h>
#define BIT(n) (UINT32_C(1) << (n))
static bool active(const Game *g, int p) {
    return p >= 0 && p < g->player_count && g->matrix.divers[p].active;
}
static uint32_t plus(uint32_t x, unsigned n) {
    return (uint32_t)fmin(1000000000.0, (double)x + n);
}
static void support(Game *g, int refuge, int keepers) {
    if (g->story.version < 3 || !(g->story.done & BIT(STORY_BRIEFING)))
        return;
    g->story.support[0] = (uint32_t)clampd((double)g->story.support[0] + refuge, 0, 100);
    g->story.support[1] = (uint32_t)clampd((double)g->story.support[1] + keepers, 0, 100);
}
bool matrix_operation_done(const Game *g, Cell owner, unsigned service) {
    if (service >= 6)
        return false;
    Cell key = matrix_region(owner);
    const MatrixRecord *r = matrix_find(&g->matrix, key);
    unsigned node = (unsigned)((owner.z - key.z) * 4 + owner.x - key.x);
    return r && node < 16 && (r->done[service] & BIT(node));
}
static void routed(Game *g, MatrixNetwork *w, MatrixRecord *r) {
    if (r->flags & MX_ROUTED)
        return;
    r->flags |= MX_ROUTED;
    for (int p = 0; p < g->player_count; p++)
        g->matrix.profiles[p].data = plus(g->matrix.profiles[p].data, 1);
    g->dirty = true;
    game_message(g, "ROUTING PAIR COMMITTED / CONTROLLER ACCESS PREPARED");
    w->switches[0] = w->switches[1] = 0;
}
void matrix_select(Game *g, int p, unsigned slot) {
    if (p >= 0 && p < SC_PLAYERS && slot < 2)
        g->matrix.divers[p].selected = slot;
}
bool matrix_equip(Game *g, int p, unsigned program) {
    if (!(onboarding_capabilities(g, p) & OB_PROGRAMS))
        return false;
    if (p < 0 || p >= SC_PLAYERS || program >= 6 ||
        !(g->matrix.profiles[p].programs & BIT(program)))
        return false;
    g->matrix.profiles[p].slots[g->matrix.divers[p].selected] = program;
    g->dirty = true;
    return true;
}
bool matrix_upgrade(Game *g, int p) {
    if (!(onboarding_capabilities(g, p) & OB_PROGRAMS))
        return false;
    if (p < 0 || p >= SC_PLAYERS)
        return false;
    MatrixProfile *m = &g->matrix.profiles[p];
    unsigned program = m->slots[g->matrix.divers[p].selected];
    unsigned level = m->upgrades[program], cost = 3 + level * 2;
    if (level >= 3 || m->data < cost) {
        game_message(g, "PROGRAM UPGRADE NEEDS MATRIX DATA / MAX LEVEL THREE");
        return false;
    }
    m->data -= cost;
    m->upgrades[program]++;
    g->dirty = true;
    game_message(g, "PROGRAM UPGRADED / CHARACTER UPGRADE RETAINED");
    return true;
}
bool matrix_cast(Game *g, int p, unsigned slot) {
    if (!(onboarding_capabilities(g, p) & OB_PROGRAMS))
        return false;
    if (!active(g, p) || slot >= 2)
        return false;
    MatrixDiver *d = &g->matrix.divers[p];
    MatrixProfile *profile = &g->matrix.profiles[p];
    unsigned program = profile->slots[slot], level = profile->upgrades[program];
    if (DIVER_VIEW(g, d).matrix_map || d->choice || d->cooldown > 0 ||
        d->bandwidth < sc_matrix_costs[program])
        return false;
    MatrixNetwork *w = &g->matrix.networks[d->network];
    MatrixRecord *r = &g->matrix.records[w->record];
    if (program == MX_FORK) {
        Vec3 a = vadd(w->nodes[w->entry].at, v3(-3, 0, 0)),
             b = vadd(w->nodes[w->switch_b].at, v3(-3, 0, 0));
        double reach = 5 + level * 3;
        if ((r->flags & MX_ROUTED) ||
            (vlen(vsub(d->pos, a)) > reach && vlen(vsub(d->pos, b)) > reach))
            return false;
        routed(g, w, r);
    } else if (program == MX_INTERRUPT) {
        bool effect = false;
        for (int i = 0; i < SC_MATRIX_GUARDS; i++)
            if (w->guards[i].active && vlen(vsub(w->guards[i].pos, d->pos)) < 14 + level * 3) {
                w->guards[i].stun = 3 + level + .02f * g->players[p].mods[3];
                w->guards[i].windup = 0;
                effect = true;
            }
        if (!(r->keys && (r->flags & MX_ROUTED))) {
            w->gate = 4 + level;
            effect = true;
        }
        if (!effect)
            return false;
    } else if (program == MX_BRIDGE) {
        if (r->done[MX_TRAFFIC])
            return false;
        w->bridge = 15 + level * 5;
    } else if (program == MX_VEIL)
        d->veil = 10 + level * 2;
    else if (program == MX_ECHO) {
        d->echo = vadd(matrix_eye(g, p), vmul(facing(g->players[p].yaw, 0), 6));
        d->echo_life = 8 + level * 3;
    } else
        d->siphon = 8 + level * 2;
    d->bandwidth -= sc_matrix_costs[program];
    d->cooldown = .4f;
    d->trace = program == MX_ECHO ? fmaxf(0, d->trace - 10) : fminf(100, d->trace + 2);
    d->selected = slot;
    g->dirty = true;
    game_message(g, sc_matrix_programs[program]);
    return true;
}
void matrix_cycle_service(Game *g, int p, int delta) {
    if (active(g, p) && !onboarding_matrix_intro(g, p) && (delta == 1 || delta == -1))
        g->matrix.divers[p].service = (unsigned)((int)g->matrix.divers[p].service + 6 + delta) % 6;
}
static bool control(Game *g, Cell owner, unsigned kind, unsigned player) {
    int free = -1;
    for (int i = 0; i < SC_MATRIX_CONTROLS; i++) {
        MatrixControl *c = &g->matrix.controls[i];
        if (c->life <= 0 && free < 0)
            free = i;
        if (c->life > 0 && cell_equal(c->room, owner) && ((kind == 3) == (c->kind == 3))) {
            free = i;
            break;
        }
    }
    if (free < 0)
        return false;
    g->matrix.controls[free] = (MatrixControl){owner,
                                               kind == 2   ? 20
                                               : kind == 1 ? 12
                                                           : 10,
                                               kind, player};
    return true;
}
static int console(Game *g, int p, MatrixNetwork **world, MatrixRecord **record) {
    if (!active(g, p))
        return -1;
    MatrixDiver *d = &g->matrix.divers[p];
    if (DIVER_VIEW(g, d).matrix_map || d->choice)
        return -1;
    *world = &g->matrix.networks[d->network];
    *record = &g->matrix.records[(*world)->record];
    int n = matrix_node(*world, d->pos, 3.5);
    if (n < 0) {
        game_message(g, "APPROACH A NETWORK CONSOLE / FOLLOW THE GOLD SIGNAL");
        return -1;
    }
    if (!onboarding_matrix_intro(g, p) && d->veil <= 0 &&
        matrix_guarded(*world, (*world)->nodes[n].at)) {
        game_message(g, "CONSOLE GUARDED / PULSE, VEIL OR INTERRUPT SECURITY");
        return -1;
    }
    return n;
}
bool matrix_service(Game *g, int p) {
    MatrixNetwork *w;
    MatrixRecord *r;
    int node = console(g, p, &w, &r);
    if (node < 0)
        return false;
    MatrixDiver *d = &g->matrix.divers[p];
    unsigned service = onboarding_matrix_intro(g, p) ? MX_POWER : d->service;
    const float costs[] = {5, 8, 8, 6, 12, 10};
    if (d->bandwidth < costs[service]) {
        game_message(g, "SERVICE NEEDS BANDWIDTH / WAIT TO RECOVER");
        return false;
    }
    Cell owner = w->nodes[node].room;
    if (g->campaign.version && campaign_guardian(owner) >= 0 &&
        (service == MX_ROBOT || service == MX_TRAFFIC)) {
        game_message(g, "GUARDIAN CONTROL HARDENED / RESTORE POWER OR SILENCE ITS SECURITY");
        return false;
    }
    uint32_t old = world_systems(&g->world, owner), flags = old;
    if (service == MX_POWER)
        flags = (old | SYSTEM_POWER) & ~SYSTEM_BLACKOUT;
    if (service == MX_SECURITY)
        flags = old | SYSTEM_SILENCED;
    if (service == MX_BEACON)
        flags = old | SYSTEM_BEACON;
    if (service <= MX_BEACON && !world_set_systems(&g->world, owner, flags))
        return false;
    unsigned kind = g->matrix.profiles[p].upgrades[MX_SIPHON] ? 2 : 1;
    if ((service == MX_ROBOT || service == MX_TRAFFIC) &&
        !control(g, owner, service == MX_TRAFFIC ? 3 : kind, (unsigned)p))
        return false;
    bool first = !(r->done[service] & BIT(node));
    int mission = matrix_story_operation(g, p, owner, service);
    if (mission < 0) {
        world_set_systems(&g->world, owner, old);
        game_message(g, "STORY PREPARATION NEEDS ITS PREREQUISITES / RETRY THE UPLINK");
        return false;
    }
    r->seen |= BIT(node);
    if (service <= MX_BEACON)
        factions_systems(g, owner, flags, p);
    r->done[service] |= BIT(node);
    if (service == MX_ARCHIVE)
        matrix_story_evidence(g, owner);
    if (service == MX_TRAFFIC)
        w->bridge = 30;
    if (first) {
        bool changed = service > MX_BEACON || old != flags;
        for (int i = 0; i < g->player_count; i++) {
            if (!mission && changed) {
                player_add_salvage(&g->players[i], 50);
                g->matrix.profiles[i].data = plus(g->matrix.profiles[i].data, 1);
            }
            if (service == MX_ARCHIVE) {
                g->matrix.profiles[i].knowledge = plus(g->matrix.profiles[i].knowledge, 1);
                g->players[i].career[1] = plus(g->players[i].career[1], 1);
            }
        }
        support(g, service == MX_ARCHIVE ? 0 : 2,
                service == MX_SECURITY  ? -1
                : service == MX_ARCHIVE ? 3
                                        : 1);
        matrix_story_event(g, owner, service, 0);
    }
    d->bandwidth -= costs[service];
    d->trace = fminf(100, d->trace + (service == MX_ARCHIVE ? 8 : 6));
    d->veil = 0;
    g->dirty = true;
    game_message(g, service == MX_ROBOT ? (kind == 2 ? "ROBOT LINK HIJACKED / FRIENDLY FIRE SUPPORT"
                                                     : "ROBOTS REBOOTING / TWELVE SECONDS")
                    : service == MX_TRAFFIC
                        ? "MAINTENANCE BROADCAST / HOSTILES DISTRACTED / DATA BRIDGE RESTORED"
                        : sc_matrix_services[service]);
    if (onboarding_matrix_intro(g, p)) {
        g->players[p].intro |= INTRO_GRID;
        g->matrix.profiles[p].programs |= 7;
        g->matrix.profiles[p].slots[1] = MX_INTERRUPT;
        matrix_disconnect(g, p, false);
        game_message(
            g, "POWER RESTORED / THE ROOM IS LIT / RECONNECT WHEN READY TO EXPLORE THE MATRIX");
    }
    return true;
}
static void cache(Game *g, int p, MatrixNetwork *w, MatrixRecord *r, unsigned node) {
    MatrixDiver *d = &g->matrix.divers[p];
    if (r->loot & BIT(node)) {
        game_message(g, "CACHE ALREADY RECOVERED");
        return;
    }
    bool vault = w->rare && node == w->vault;
    if (vault && !(r->flags & MX_ROUTED)) {
        game_message(g, "VAULT SEALED / COMPLETE THE ROUTING PAIR");
        return;
    }
    r->loot |= BIT(node);
    if (vault)
        r->flags |= MX_VAULT;
    for (int i = 0; i < g->player_count; i++) {
        MatrixProfile *m = &g->matrix.profiles[i];
        unsigned candidate = 3 + world_hash(g->world.seed, w->nodes[node].room, 0x505247u) % 3;
        for (unsigned j = 0; j < 3; j++)
            if (!(m->programs & BIT(3 + (candidate - 3 + j) % 3))) {
                candidate = 3 + (candidate - 3 + j) % 3;
                break;
            }
        m->programs |= BIT(candidate);
        m->data = plus(m->data, vault ? 5 : 2);
        if (vault) {
            m->knowledge = plus(m->knowledge, 1);
            g->players[i].career[1] = plus(g->players[i].career[1], 1);
        }
    }
    d->trace = fminf(100, d->trace + (vault ? 12 : 4));
    d->veil = 0;
    g->dirty = true;
    game_message(g, vault ? "RARE VAULT RECOVERED / THIRD CONTROLLER OUTCOME PREPARED"
                          : "PROGRAM CACHE RECOVERED / F8 EQUIP AND UPGRADE");
}
void matrix_use(Game *g, int p) {
    if (onboarding_matrix_intro(g, p)) {
        matrix_service(g, p);
        return;
    }
    if (!active(g, p))
        return;
    MatrixDiver *d = &g->matrix.divers[p];
    if (DIVER_VIEW(g, d).matrix_map || d->choice)
        return;
    MatrixNetwork *w = &g->matrix.networks[d->network];
    MatrixRecord *r = &g->matrix.records[w->record];
    int node = matrix_node(w, d->pos, 7);
    if (node < 0)
        return;
    Vec3 at = w->nodes[node].at;
    if (vlen(vsub(d->pos, vadd(at, v3(-3, 0, -3)))) < 2 && w->nodes[node].cache) {
        cache(g, p, w, r, (unsigned)node);
        return;
    }
    if (!onboarding_matrix_intro(g, p) && d->veil <= 0 && matrix_guarded(w, at)) {
        game_message(g, "SECURITY ACTIVE / DEFEAT, CLOAK OR INTERRUPT IT");
        return;
    }
    r->seen |= BIT(node);
    if ((unsigned)node == w->credential && vlen(vsub(d->pos, vadd(at, v3(3, 0, 0)))) < 2.4 &&
        !(r->keys & BIT(node))) {
        r->keys |= BIT(node);
        d->veil = 0;
        g->dirty = true;
        game_message(g, "MAINTENANCE CREDENTIAL RECOVERED / ROUTE THE PAIR");
        return;
    }
    if (((unsigned)node == w->entry || (unsigned)node == w->switch_b) &&
        vlen(vsub(d->pos, vadd(at, v3(-3, 0, 0)))) < 2.4 && !(r->flags & MX_ROUTED)) {
        unsigned side = (unsigned)node == w->entry ? 0 : 1;
        w->switches[side] = 16;
        d->veil = 0;
        if (w->switches[0] > 0 && w->switches[1] > 0)
            routed(g, w, r);
        else
            game_message(g, "ROUTER HELD / SIXTEEN SECONDS TO OPERATE ITS PARTNER");
        return;
    }
    if ((unsigned)node == w->core && vlen(vsub(d->pos, at)) < 3.5) {
        if (r->outcome) {
            game_message(g, "CONTROLLER RESOLVED / COMMITTED OUTCOME RETAINED");
            return;
        }
        if (!(r->flags & MX_ROUTED)) {
            game_message(g, "CONTROLLER NEEDS THE ROUTING PAIR");
            return;
        }
        if (!r->keys && w->gate <= 0) {
            if (d->bandwidth < 15) {
                game_message(g, "FORCE ACCESS NEEDS FIFTEEN BANDWIDTH OR A CREDENTIAL");
                return;
            }
            d->bandwidth -= 15;
            d->trace = fminf(100, d->trace + 20);
            w->gate = 6;
        }
        d->choice = true;
        g->views[p].journal = true;
        g->views[p].journal_page = 4;
        return;
    }
    game_message(g,
                 "F OPERATES THIS ROOM'S SERVICE / Q E CHOOSE / G USES KEYS, ROUTERS AND CACHES");
}
bool matrix_choose(Game *g, int p, unsigned choice) {
    if (!active(g, p) || choice < 1 || choice > 3)
        return false;
    MatrixDiver *d = &g->matrix.divers[p];
    MatrixNetwork *w = &g->matrix.networks[d->network];
    MatrixRecord *r = &g->matrix.records[w->record];
    if (!d->choice || r->outcome || !(r->flags & MX_ROUTED) || (!r->keys && w->gate <= 0) ||
        vlen(vsub(d->pos, w->nodes[w->core].at)) > 3.5 || matrix_guarded(w, w->nodes[w->core].at))
        return false;
    bool prepared = r->keys && (w->rare ? (r->flags & MX_VAULT) : r->done[MX_ARCHIVE]);
    if (choice == 3 && !prepared) {
        game_message(g, "THIRD OUTCOME NEEDS A CREDENTIAL AND AN ARCHIVE COPY / RARE VAULT");
        return false;
    }
    Cell owner = w->nodes[w->core].room;
    uint32_t old = world_systems(&g->world, owner);
    uint32_t flags =
        choice == 2 ? (old | SYSTEM_SILENCED | SYSTEM_BLACKOUT) & ~SYSTEM_POWER
                    : (old | SYSTEM_POWER | SYSTEM_BEACON | (choice == 3 ? SYSTEM_SILENCED : 0)) &
                          ~SYSTEM_BLACKOUT;
    if (!world_set_systems(&g->world, owner, flags))
        return false;
    r->outcome = choice;
    factions_systems(g, owner, flags, p);
    r->alert = 3;
    r->killed |= (w->mask & ~BIT(w->entry)) | 0xff0000u;
    for (int i = 0; i < SC_MATRIX_GUARDS; i++)
        w->guards[i].active = false;
    for (int i = 0; i < g->player_count; i++) {
        MatrixProfile *m = &g->matrix.profiles[i];
        m->data = plus(m->data, choice == 3 ? 6 : 4);
        unsigned knowledge = choice == 3 ? 3 : choice == 1 ? 1 : 0;
        m->knowledge = plus(m->knowledge, knowledge);
        g->players[i].career[1] = plus(g->players[i].career[1], knowledge);
        if (choice == 2)
            g->players[i].career[3] = plus(g->players[i].career[3], 1);
        if (g->matrix.divers[i].active && cell_equal(g->matrix.divers[i].key, w->key)) {
            g->matrix.divers[i].choice = false;
            g->views[i].journal = false;
        }
    }
    support(g, choice == 2 ? -3 : choice == 3 ? 8 : 5, choice == 2 ? -8 : choice == 3 ? 8 : 3);
    matrix_story_event(g, owner, 6, choice);
    g->dirty = true;
    game_message(g, choice == 2
                        ? "CONTROLLER PURGED / LOCAL LIFE SUPPORT DARK / FACTIONS RESPOND"
                        : "CONTROLLER RECONCILED / POWER AND WARD RESTORED / KNOWLEDGE RETAINED");
    return true;
}
bool matrix_goal(const Game *g, int p, Vec3 *at) {
    if (!active(g, p))
        return false;
    const MatrixDiver *d = &g->matrix.divers[p];
    const MatrixNetwork *w = &g->matrix.networks[d->network];
    const MatrixRecord *r = &g->matrix.records[w->record];
    if (!(r->done[MX_POWER] & BIT(w->entry)))
        *at = w->nodes[w->entry].at;
    else if (!r->keys)
        *at = vadd(w->nodes[w->credential].at, v3(3, 0, 0));
    else if (!(r->flags & MX_ROUTED))
        *at = vadd(w->nodes[w->switches[0] > 0 ? w->switch_b : w->entry].at, v3(-3, 0, 0));
    else if (!r->outcome)
        *at = w->nodes[w->core].at;
    else
        *at = w->nodes[(d->terminal.z - d->key.z) * 4 + d->terminal.x - d->key.x].at;
    return true;
}
void matrix_hint(const Game *g, int p, char *text, size_t size) {
    if (!active(g, p)) {
        snprintf(text, size, "C / LINK AT A SHIP TERMINAL OR SHELTER WORKSTATION");
        return;
    }
    const MatrixDiver *d = &g->matrix.divers[p];
    const MatrixNetwork *w = &g->matrix.networks[d->network];
    const MatrixRecord *r = &g->matrix.records[w->record];
    snprintf(text, size, "%s",
             !(r->done[MX_POWER] & BIT(w->entry)) ? "F / RESTORE THE ENTRY MICROGRID"
             : !r->keys                           ? "G / RECOVER THE MAINTENANCE CREDENTIAL"
             : !(r->flags & MX_ROUTED)            ? "G / ROUTE BOTH SWITCHES WITHIN SIXTEEN SECONDS"
             : !r->outcome                        ? "G / RESOLVE THE GUARDED CONTROLLER"
                           : "C / DISCONNECT AND BANK YOUR DATA / OPTIONAL VAULTS REMAIN");
}
bool matrix_control_enemy(Game *g, Room *room, Enemy *e, double dt) {
    e->allegiance = 0;
    for (int i = 0; i < SC_MATRIX_CONTROLS; i++) {
        const MatrixControl *c = &g->matrix.controls[i];
        if (c->life <= 0 || !cell_equal(c->room, room->cell))
            continue;
        if (c->kind != 3 && sc_enemy_bodies[e->kind].form != ROBOT && e->kind != 16)
            continue;
        e->allegiance = (uint8_t)c->kind;
        e->ally_owner = (uint8_t)c->owner;
        e->windup = 0;
        e->burn = 0;
        e->cooldown = fmaxf(0, e->cooldown - (float)dt);
        if (c->kind == 3) {
            e->heading = vnorm(vsub(matrix_terminal(c->room), enemy_center(e)));
            return true;
        }
        if (c->kind == 1 || e->cooldown > 0)
            return true;
        Room *target_room = NULL;
        int target = -1;
        double nearest = 28;
        for (int r = 0; r < g->world.count; r++)
            for (int k = 0; k < g->world.rooms[r].enemy_count; k++) {
                Enemy *t = &g->world.rooms[r].enemies[k];
                if (!t->alive || t == e || t->allegiance == 2 ||
                    (cell_equal(g->world.rooms[r].cell, c->room) &&
                     (sc_enemy_bodies[t->kind].form == ROBOT || t->kind == 16)))
                    continue;
                Vec3 delta = vsub(enemy_center(t), enemy_center(e));
                double distance = vlen(delta);
                if (distance < nearest && world_raycast(&g->world, enemy_center(e), vnorm(delta),
                                                        distance) >= distance - .1) {
                    target_room = &g->world.rooms[r];
                    target = k;
                    nearest = distance;
                }
            }
        if (target >= 0) {
            Vec3 end = enemy_center(&target_room->enemies[target]);
            for (int b = 0; b < 16; b++)
                if (g->matrix.links[b].life <= 0) {
                    g->matrix.links[b] =
                        (MatrixLink){position_from(enemy_center(e)), position_from(end), .12f};
                    break;
                }
            game_damage_enemy(g, target_room, target, (int)c->owner,
                              12 + 2 * g->matrix.profiles[c->owner].upgrades[MX_SIPHON], 0);
            e->cooldown = .4f;
        }
        return true;
    }
    return false;
}
