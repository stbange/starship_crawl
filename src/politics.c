#include "game.h"
#include <stdio.h>
#include <string.h>

void politics_init(Game *g) {
    memset(&g->politics, 0, sizeof(g->politics));
    g->politics.version = g->world.generator == 13;
    director_init(&g->director, g->world.generator);
    factions_init(g);
}
bool politics_valid(const Politics *p, unsigned generator) {
    if (p->version != (generator == 13) || p->tracked > SC_COUNTRIES)
        return false;
    for (unsigned i = 0; i < SC_COUNTRIES; i++) {
        const CountryState *c = &p->countries[i];
        if (c->met > 1 || c->pledge > 3 || c->stage > 3 || c->claimed > 3 || c->offended > 3 ||
            (!!c->pledge != !!c->stage) || (c->stage && !c->met) || (c->claimed && c->stage != 3) ||
            (!p->version && (c->met || c->pledge || c->stage || c->claimed || c->offended)))
            return false;
    }
    return p->version || !p->tracked;
}
Relation politics_relation(const Game *g, unsigned a, unsigned b) {
    if (a == b)
        return REL_PEACE;
    a %= SC_COUNTRIES;
    b %= SC_COUNTRIES;
    const CountryState *ca = &g->politics.countries[a], *cb = &g->politics.countries[b];
    if ((ca->stage == 3 && ca->pledge == 3 && (a + 1) % 3 == b) ||
        (cb->stage == 3 && cb->pledge == 3 && (b + 1) % 3 == a))
        return REL_WAR;
    if ((ca->stage == 3 && ca->pledge == 1 && (a + 1) % 3 == b) ||
        (cb->stage == 3 && cb->pledge == 1 && (b + 1) % 3 == a))
        return REL_PEACE;
    if ((ca->stage == 3 && ca->pledge == 2 && (a + 1) % 3 == b) ||
        (cb->stage == 3 && cb->pledge == 2 && (b + 1) % 3 == a))
        return REL_TRIBUTE;
    unsigned low = a < b ? a : b, high = a < b ? b : a;
    return low == 0 && high == 1
               ? REL_WAR
               : (Relation)(world_hash(g->world.seed, (Cell){(int)low, (int)high, 0}, 7104) % 4);
}
bool politics_world_valid(const Politics *p, const Director *d, const World *w) {
    if (!p->version)
        return true;
    for (unsigned n = 0; n < SC_COUNTRIES; n++) {
        const CountryState *c = &p->countries[n];
        for (unsigned role = COURT_CONDUIT; role <= COURT_FINALE && role <= c->stage; role++) {
            Cell site = politics_site(w->seed, n, (CourtRole)role);
            const RoomRecord *record = NULL;
            for (size_t i = 0; i < w->record_count; i++)
                if (cell_equal(w->records[i].cell, site))
                    record = &w->records[i];
            if (!record || !(record->local & FRONTIER_CLEARED) || !(record->local & 12))
                return false;
            if (role == COURT_FINALE && c->claimed != ((record->local >> 2) & 3))
                return false;
        }
    }
    if (d->kind) {
        RoomSpec site = world_describe_version(w->seed, w->generator, d->site);
        if (!cell_equal(site.cell, d->site) || site.type == BAY || site.habitat.court)
            return false;
    }
    return true;
}
unsigned politics_owner(const Game *g, Cell cell) {
    if (g->factions.version)
        return factions_owner(g, cell);
    if (!g->politics.version)
        return 0;
    for (unsigned i = 0; i < SC_COUNTRIES; i++) {
        const CountryState *c = &g->politics.countries[i];
        if (c->stage == 3 && c->pledge == 3 &&
            cell_equal(world_region(cell), politics_region(g->world.seed, i, COURT_CONDUIT)))
            return i + 1;
    }
    return politics_country(g->world.seed, cell);
}
int politics_reputation(const Game *g, unsigned country, int player) {
    if (!country || country > SC_COUNTRIES || player < 0 || player >= SC_PLAYERS)
        return 0;
    const CountryState *c = &g->politics.countries[country - 1];
    const CountryState *invader = &g->politics.countries[(country + 1) % 3];
    if ((c->offended & (1u << player)) || (invader->stage == 3 && invader->pledge == 3))
        return -3;
    return c->stage == 3 ? 3 : c->met ? 1 : 0;
}
bool politics_threat(const Game *g, const Enemy *enemy) {
    if (g->factions.version && enemy->country) {
        for (int p = 0; p < g->player_count; p++)
            if (!g->players[p].sheltered && factions_hostile(g, enemy, p))
                return true;
        return false;
    }
    if (!g->politics.version || !enemy->country)
        return true;
    for (int p = 0; p < g->player_count; p++)
        if (!g->players[p].sheltered && !g->matrix.divers[p].active &&
            politics_reputation(g, enemy->country, p) < 0)
            return true;
    return false;
}
void politics_journal(Game *g, int player) {
    if (!g->politics.version || player < 0 || player >= g->player_count)
        return;
    g->players[player].journal = true;
    g->players[player].journal_page = 6;
    g->politics.audience[player] = 0;
}
void politics_read(Game *g, int player) {
    if (!g->politics.version || player < 0 || player >= g->player_count)
        return;
    g->politics.reading[player] = (g->politics.reading[player] + 1) % SC_COUNTRIES;
    g->politics.audience[player] = 0;
}
void politics_track(Game *g, int player) {
    if (!g->politics.version || player < 0 || player >= g->player_count)
        return;
    unsigned choice = g->politics.reading[player] + 1;
    g->politics.tracked = g->politics.tracked == choice ? 0 : choice;
    g->dirty = true;
}
bool politics_objective(const Game *g, Cell *cell) {
    if (!g->politics.version || !g->politics.tracked)
        return false;
    unsigned n = g->politics.tracked - 1, stage = g->politics.countries[n].stage;
    *cell = politics_site(g->world.seed, n,
                          stage == 1   ? COURT_CONDUIT
                          : stage == 2 ? COURT_FINALE
                                       : COURT_AUDIENCE);
    return true;
}
const char *politics_goal(const Game *g) {
    static const char *goals[] = {"SEEK AN AUDIENCE AT THE COURT", "SECURE THE DISPUTED CONDUIT",
                                  "DEFEAT THE ROYAL GUARD", "RETURN TO THE CHANGED COURT"};
    unsigned n = g->politics.tracked ? g->politics.tracked - 1 : 0;
    return goals[g->politics.countries[n].stage];
}
static Room *near_court(Game *g, int player) {
    Player *p = &g->players[player];
    if (p->sheltered || p->lift_active || p->riding || g->matrix.divers[player].active)
        return NULL;
    Room *r = world_find(&g->world, world_cell(position_absolute(p->pos)));
    return r && r->ready && r->habitat.court &&
                   vlen(vsub(position_absolute(p->pos), encounter_console(r))) < 3
               ? r
               : NULL;
}
bool politics_choose(Game *g, int player, unsigned choice) {
    if (!g->politics.version || player < 0 || player >= g->player_count || choice < 1 ||
        choice > 3 || !g->players[player].journal || g->players[player].journal_page != 6 ||
        !g->politics.audience[player])
        return false;
    Room *r = near_court(g, player);
    unsigned n = g->politics.audience[player] - 1;
    CountryState *c = &g->politics.countries[n];
    if (!r || r->habitat.court != COURT_AUDIENCE || r->habitat.country != n + 1 || c->stage)
        return false;
    c->pledge = choice;
    c->stage = 1;
    c->met = 1;
    g->politics.tracked = n + 1;
    for (int p = 0; p < SC_PLAYERS; p++) {
        g->politics.audience[p] = 0;
        if (g->players[p].journal_page == 6)
            g->players[p].journal = false;
    }
    game_message(g, "COMMISSION ACCEPTED / CONDUIT MARKED IN GOLD / CLEAR IT THEN USE ITS SEAL");
    g->dirty = true;
    return true;
}
static void resolve(Game *g, unsigned nation, Room *r) {
    CountryState *c = &g->politics.countries[nation];
    /* Commit the fallible world record before the shared political outcome. */
    Cell site =
        politics_site(g->world.seed, nation, c->pledge == 1 ? COURT_CONDUIT : COURT_AUDIENCE);
    uint32_t systems = (world_systems(&g->world, site) & ~SYSTEM_BLACKOUT) | SYSTEM_POWER;
    if (c->pledge == 1)
        systems |= SYSTEM_BEACON;
    if (!world_set_systems(&g->world, site, systems))
        return;
    c->stage = 3;
    uint32_t participants = (world_local_state(&g->world, r) >> 2) & 3;
    for (int p = 0; p < SC_PLAYERS; p++)
        if (participants & (1u << p)) {
            c->claimed |= 1u << p;
            g->players[p].points = (uint32_t)fmin(1000, g->players[p].points + 2);
            g->players[p].credits = (uint32_t)fmin(1000000000, g->players[p].credits + 100);
        }
    game_message(g, c->pledge == 1
                        ? "TREATY SIGNED / CONDUIT WARD RESTORED / +2 POINTS +100 SALVAGE"
                    : c->pledge == 2
                        ? "SUCCESSOR INSTALLED / COURT WORKSHOP OPEN / +2 POINTS +100 SALVAGE"
                        : "BORDER ANNEXED / RIVAL PATROLS NOW HOSTILE / +2 POINTS +100 SALVAGE");
}
bool politics_interact(Game *g, int player) {
    if (!g->politics.version || player < 0 || player >= g->player_count)
        return false;
    Room *r = near_court(g, player);
    if (!r)
        return false;
    unsigned n = r->habitat.country - 1;
    CountryState *c = &g->politics.countries[n];
    c->met = 1;
    if (r->habitat.court == COURT_AUDIENCE) {
        politics_journal(g, player);
        g->politics.reading[player] = n;
        g->politics.audience[player] = c->stage ? 0 : n + 1;
    } else if (!c->stage || (r->habitat.court == COURT_FINALE && c->stage < 2)) {
        game_message(g, "ROYAL SEAL / SEEK THE COURT FIRST, THEN SECURE ITS DISPUTED CONDUIT");
    } else {
        game_encounter_interact(g, player);
        uint32_t state = world_local_state(&g->world, r);
        if ((state & FRONTIER_CLEARED) && (state & (FRONTIER_ELIGIBLE << player))) {
            if (r->habitat.court == COURT_CONDUIT && c->stage == 1) {
                c->stage = 2;
                g->politics.tracked = n + 1;
                game_message(g, "CONDUIT EVIDENCE RECOVERED / ROYAL GUARD MARKED IN GOLD");
            } else if (r->habitat.court == COURT_FINALE && c->stage == 2)
                resolve(g, n, r);
        }
    }
    g->dirty = true;
    return true;
}
void politics_attack(Game *g, const Enemy *e, int player) {
    if (g->politics.version && e->country &&
        !(g->politics.countries[e->country - 1].offended & (1u << player))) {
        g->politics.countries[e->country - 1].offended |= 1u << player;
        game_message(g, "PATROL ATTACKED / THIS COUNTRY NOW TREATS YOU AS HOSTILE");
        g->dirty = true;
    }
}
bool politics_enemy_step(Game *g, Room *room, Enemy *e, double dt) {
    if (g->factions.version && !e->country && !e->behavior && !e->allegiance)
        return factions_feral(g, room, e, dt);
    if ((!g->politics.version && !g->factions.version) || !e->country || e->allegiance)
        return false;
    bool hostile = false, nearby = false;
    for (int p = 0; p < g->player_count; p++)
        if (!g->players[p].sheltered && !g->matrix.divers[p].active) {
            double d = vlen(position_delta(g->players[p].pos, e->pos));
            nearby |= d < 60;
            hostile |= d < 60 && (g->factions.version ? factions_hostile(g, e, p)
                                                      : politics_reputation(g, e->country, p) < 0);
        }
    if (hostile)
        return false;
    if (!nearby || (g->factions.version && factions_safe(g, room->cell)))
        return true;
    e->cooldown = fmaxf(0, e->cooldown - (float)dt);
    e->flash = fmaxf(0, e->flash - (float)dt);
    /* Local patrols defend against infestation and warring patrols. No player XP. */
    for (int i = 0; i < room->enemy_count; i++) {
        Enemy *other = &room->enemies[i];
        if (!other->alive || other == e || other->allegiance ||
            (other->country &&
             (g->factions.version
                  ? factions_relation(g, e->country, other->country)
                  : politics_relation(g, e->country - 1, other->country - 1)) != REL_WAR))
            continue;
        Vec3 delta = vsub(enemy_center(other), enemy_center(e));
        double length = vlen(delta);
        if (length < 10 && e->cooldown <= 0 &&
            world_raycast(&g->world, enemy_center(e), vnorm(delta), length) >= length - .1) {
            e->cooldown = 1.4f;
            e->warning_sound = .3f;
            e->heading = vnorm(delta);
            other->flash = .2f;
            if (g->factions.version)
                factions_damage(g, room, i, 14, -1);
            else
                other->health -= 14;
            if (!g->factions.version && other->health <= 0) {
                world_kill(&g->world, room, i);
                if (!other->alive)
                    game_enemy_death(g, other, -1);
            }
            g->dirty = true;
            return true;
        }
    }
    Vec3 target =
        vadd(e->home, v3(sin(g->time * .3 + e->id) * 2, 0, cos(g->time * .3 + e->id) * 2));
    Vec3 delta = vsub(target, position_absolute(e->pos));
    delta.y = 0;
    if (vlen(delta) > .1) {
        e->heading = vnorm(delta);
        bool grounded = false;
        world_move(&g->world, &e->pos, vmul(e->heading, dt * .9), .42, 1.25, true, &grounded);
    }
    return true;
}
void politics_step(Game *g, double dt) {
    if (!g->politics.version)
        return;
    for (int p = 0; p < g->player_count; p++) {
        Player *player = &g->players[p];
        if (player->sheltered || g->matrix.divers[p].active)
            continue;
        Room *r = world_find(&g->world, world_cell(position_absolute(player->pos)));
        if (!r || !r->ready || !r->habitat.country)
            continue;
        CountryState *c = &g->politics.countries[r->habitat.country - 1];
        if (!c->met) {
            c->met = 1;
            g->dirty = true;
        }
        if (r->habitat.court == COURT_AUDIENCE && c->stage == 3 &&
            politics_reputation(g, r->habitat.country, p) >= 0 && player->hurt <= 0 &&
            vlen(vsub(position_absolute(player->pos), encounter_console(r))) < 8) {
            player->armor = fminf(player_max_armor(player), player->armor + (float)dt * 4);
            if (c->pledge == 2)
                player->energy = fminf(player_max_energy(player), player->energy + (float)dt * 8);
        }
    }
}
void politics_line(const Game *g, int player, int line, char *text, size_t size) {
    unsigned n = g->politics.reading[player] % SC_COUNTRIES;
    const CountryState *c = &g->politics.countries[n];
    Government gov = politics_government(g->world.seed, n);
    static const char *premise[] = {
        "ONE MONARCH, THREE BODIES. ONE BODY HAS CUT THE OTHERS' HEAT.",
        "ROYAL SHELLS CONVEY RANK. THE HEIR'S SHELL WAS STOLEN WITH THE FOOD TITHE.",
        "THE DEAD GOVERN THROUGH MEMORY. A FORGED VOTE CUT THE ARCHIVE'S POWER."};
    static const char *rulers[] = {"QUEEN", "KING", "REGENT", "HEIR"};
    static const char *names[] = {"VESPER", "SERE", "IX", "ORU", "NACRE", "THREN"};
    uint32_t hash = world_hash(g->world.seed, (Cell){(int)n, 0, 0}, 7110);
    Cell site = politics_site(g->world.seed, n,
                              c->stage == 1   ? COURT_CONDUIT
                              : c->stage == 2 ? COURT_FINALE
                                              : COURT_AUDIENCE);
    switch (line) {
    case 0:
        snprintf(text, size, "%s / %s", politics_name(g->world.seed, n),
                 politics_government_name(gov));
        break;
    case 1:
        snprintf(text, size, "%s %s / %s", gov == ECHO_PARLIAMENT ? "SPEAKER" : rulers[hash % 4],
                 names[((hash >> 8) + (c->stage == 3 && c->pledge == 2)) % 6],
                 c->stage == 3 && c->pledge == 2 ? "NEW SUCCESSOR" : "INCUMBENT");
        break;
    case 2:
        snprintf(text, size, "%s", premise[gov]);
        break;
    case 3:
        snprintf(text, size, "NEIGHBOR %s / %s", politics_name(g->world.seed, (n + 1) % 3),
                 politics_relation_name(politics_relation(g, n, (n + 1) % 3)));
        break;
    case 4:
        snprintf(text, size, "YOUR STANDING %+d / %s / L NEXT COURT",
                 politics_reputation(g, n + 1, player),
                 c->met ? "CONTACT MADE" : "PUBLIC BROADCAST");
        break;
    case 5:
        snprintf(text, size, "%s [%d,%d,%d] / U %s",
                 c->stage == 1   ? "CONDUIT"
                 : c->stage == 2 ? "ROYAL GUARD"
                                 : "COURT",
                 site.x, site.y, site.z, g->politics.tracked == n + 1 ? "UNTRACK" : "TRACK");
        break;
    case 6:
        snprintf(text, size, "1 RESTORE SHARED SUPPLY / PEACE, SAFE CONDUIT, REPAIR ACCESS");
        break;
    case 7:
        snprintf(text, size, "2 INSTALL SUCCESSOR / TRIBUTE, NEW RULER, ENERGY WORKSHOP");
        break;
    case 8:
        snprintf(text, size, "3 SEIZE THE CONDUIT / ANNEX REGION, RIVAL PATROLS HOSTILE");
        break;
    case 9:
        snprintf(text, size, "%s",
                 !g->politics.version ? "LEGACY SHIP / NEW SHIP ENABLES COURTS"
                 : c->stage == 3 ? "SETTLEMENT ENDURES / REWARDS CLAIMED ONCE / COURT REPAIRS ARMOR"
                 : c->stage      ? "PLEDGE RECORDED / SECURE CONDUIT THEN DEFEAT THE ROYAL GUARD"
                 : g->politics.audience[player]
                     ? "CHOOSE 1 / 2 / 3 / ENTER LEAVES UNDECIDED"
                     : "G AT THE COURT OPENS AN AUDIENCE / ALL ROUTES USE ORDINARY GUNS");
        break;
    default: {
        unsigned k = (g->director.sequence + SC_DIRECTOR_LOG - 1) % SC_DIRECTOR_LOG;
        snprintf(text, size, "FIELD REPORT: %s", director_name(g->director.history[k]));
        break;
    }
    }
}
