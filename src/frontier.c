#include "game.h"
#include "habitat.h"
#include <stdio.h>
#include <stdlib.h>

const char *frontier_name(unsigned kind) {
    static const char *names[CHALLENGES] = {"LINKED FRONTIER", "HANGING AERIE",
                                            "COAT COLONY",     "SUPPRESSION BATTERY",
                                            "COURIER CIRCUIT", "CIPHER LOCK"};
    return names[kind % CHALLENGES];
}
const char *frontier_hint(unsigned kind) {
    static const char *hints[CHALLENGES] = {
        "BEAMS DISRUPT LINKS / EVERY GUN WORKS",
        "FLY ABOVE THE CORE TO BREAK ITS WARD / OR SHOOT FROM COVER",
        "NOVA AND SPLASH STRIKE LINKED CLUSTERS / KEEP AN EXIT",
        "MECH ARMOR ENDURES SUPPRESSION / COVER BETWEEN VOLLEYS",
        "TOUCH BOTH CYAN PYLONS IN 18 SECONDS / WAGON HELPS / OR FIGHT",
        "MATRIX SECURITY DISABLES THE WARD / OR BREAK ITS CORE"};
    return hints[kind % CHALLENGES];
}
const char *frontier_relic(unsigned relic) {
    static const char *names[RELICS] = {"CINDER HEART", "RIME LENS", "AEGIS COIL", "MOLT SEED",
                                        "RIFT SAIL"};
    return names[relic % RELICS];
}
const char *frontier_effect(unsigned relic) {
    static const char *effects[RELICS] = {
        "BLASTS BURN / II WIDER BLASTS / III LOWER ENERGY COST",
        "BEAMS CHILL / II LONGER REACH / III EXTRA PIERCING",
        "POWERED GRID RECHARGES SHIELDS / II MECH REPAIR / III FASTER RECOVERY",
        "NOVA CHILLS / II WIDER NOVA / III NOVA RESTORES ENERGY",
        "FASTER FLIGHT / II CHEAPER PSIONICS / III QUIETER MOVEMENT"};
    return effects[relic % RELICS];
}
const char *frontier_risk(unsigned contract) {
    static const char *risk[RELICS] = {
        "THERMAL PURGE / WATCH THE VENT RINGS", "BRITTLE HARVEST / FROST FIELDS SLOW",
        "CHOIR AUDIT / ION FIELDS DRAIN ENERGY", "COLONY EXCISION / SPORE FIELDS BURN",
        "MIRROR RECOVERY / VOID FIELDS DRAIN SHIELDS"};
    return contract ? risk[(contract - 1) % RELICS] : "OPTIONAL / EXITS STAY OPEN";
}
void frontier_plan(uint32_t seed, RoomSpec *r) {
    if (r->type == BAY || r->height < 5 || campaign_guardian(r->cell) >= 0)
        return;
    if (r->height >= 14 && (r->habitat.form || r->landmark || r->type == NAVE || r->type == ENGINE))
        r->habitat.reveal = (uint8_t)(r->district + 1);
    if (r->habitat.pace != PACE_FRONTIER)
        return;
    unsigned kind = (r->hash >> 8) % CHALLENGES;
    if ((kind == CH_AERIE && r->height < 10) ||
        (kind == CH_CIRCUIT && (r->width < 70 || r->habitat.volume)))
        kind = CH_COLONY;
    r->habitat.challenge = (uint8_t)kind;
    if (campaign_layer(r->cell) < 5)
        return;
    ShipTraits t = campaign_traits(seed, r->cell);
    unsigned candidates[3], count = 0;
    if (abs(t.heat) >= 2)
        candidates[count++] = t.heat > 0 ? RELIC_HEAT : RELIC_COLD;
    if (abs(t.order) >= 2)
        candidates[count++] = t.order > 0 ? RELIC_ORDER : RELIC_CHAOS;
    if (t.alien >= 2)
        candidates[count++] = RELIC_ALIEN;
    if (count)
        r->habitat.contract = (uint8_t)(1 + candidates[(r->hash >> 20) % count]);
}
static void locate(World *w, Room *r, Enemy *e, Vec3 offset) {
    Vec3 wanted = vadd(world_center(r->cell), offset);
    if (!world_spawn_clear(w, wanted, .5, 1.5) || (offset.y < 1 && !habitat_spawn(w, r, wanted)))
        wanted = vadd(world_center(r->cell), v3(0, .02, 5 + e->id * 2));
    e->pos = position_from(wanted);
}
void frontier_populate(World *w, Room *r) {
    if (campaign_guardian(r->cell) >= 0)
        return;
    unsigned challenge = r->habitat.challenge;
    if (r->habitat.pace == PACE_FRONTIER && challenge) {
        static const uint8_t kinds[CHALLENGES - 1][4] = {
            {1, 1, 33, 0}, {35, 35, 3, 34}, {30, 37, 7, 16}, {29, 37, 33, 31}, {29, 37, 27, 30}};
        static const Vec3 offsets[CHALLENGES - 1][4] = {
            {{0, 6, 16}, {-2, .02, 20}, {2, .02, 22}, {3, .02, 10}},
            {{0, .02, 14}, {-2, .02, 15}, {2, .02, 15}, {0, .02, 18}},
            {{0, .02, 28}, {-2, .02, 19}, {3, .02, 24}, {0, .02, 15}},
            {{0, .02, 25}, {-18, .02, 0}, {18, .02, 0}, {0, .02, 12}},
            {{0, .02, 20}, {-2, .02, 12}, {2, .02, 14}, {0, .02, 28}}};
        for (int i = 0; i < 4; i++) {
            Enemy *e = &r->enemies[i];
            e->kind = kinds[challenge - 1][i];
            e->health = sc_enemies[e->kind].health + 10.f * (float)fmin(campaign_layer(r->cell), 8);
            locate(w, r, e, offsets[challenge - 1][i]);
            e->behavior = i == 0                          ? ALIEN_CONDUCTOR
                          : e->kind >= 32                 ? ALIEN_LISTENER + e->kind - 32
                          : sc_enemies[e->kind].role == 1 ? ALIEN_GAZER
                                                          : ALIEN_PILGRIM;
        }
    } else if (r->habitat.pace != PACE_FRONTIER && r->height >= 5) {
        ShipTraits t = campaign_traits(w->seed, r->cell);
        for (int i = 0; i < r->enemy_count; i++) {
            if (i % 4 != 1 || r->enemies[i].kind == 26)
                continue;
            unsigned kind = 32 + (world_hash(w->seed, r->cell, 91000 + (unsigned)i) % 8);
            if (i == 1 && t.heat <= -2)
                kind = 40;
            else if (i == 1 && t.heat >= 2)
                kind = 41;
            if (sc_enemies[kind].tier > world_difficulty(r->cell))
                continue;
            Enemy *e = &r->enemies[i];
            e->kind = (uint8_t)kind;
            e->behavior = (uint8_t)(ALIEN_LISTENER + kind - 32);
            e->health = sc_enemies[kind].health + 10.f * (float)fmin(campaign_layer(r->cell), 8);
            /* Ambushers use a visible alcove beside a clear central route. */
            if (e->behavior == ALIEN_AMBUSH)
                locate(w, r, e, v3(7.5, .02, 14));
            if (e->behavior == ALIEN_REFLECTION)
                locate(w, r, e, v3(-12, .02, 3));
        }
    }
    for (int i = 0; i < r->enemy_count; i++) {
        Enemy *e = &r->enemies[i];
        e->home = position_absolute(e->pos);
        e->body_heading = e->heading;
        if (e->behavior == ALIEN_COLONY)
            alien_fragments(w, e);
    }
}
Vec3 frontier_pylon(const Room *r, unsigned index) {
    return vadd(world_center(r->cell), v3(index ? 28 : -28, .02, 0));
}
Vec3 frontier_hazard(const Room *r) {
    /* Local, predictable patches never occupy the console or all exits. */
    static const Vec3 points[] = {{-7, .05, 11}, {7, .05, 18}, {0, .05, 27}};
    return vadd(world_center(r->cell), points[r->hazard_stage % 3]);
}
bool frontier_hazard_live(const Room *r) {
    return r->faction_edit != EDIT_CANCEL && r->frontier_active && r->habitat.contract &&
           r->hazard_time > 1.4f && r->hazard_time < 2.6f;
}
void frontier_step(Game *g, Room *r, double dt) {
    if (!r->frontier_active)
        return;
    if (r->habitat.challenge == CH_CIRCUIT && r->circuit != 3) {
        r->circuit_time = fmaxf(0, r->circuit_time - (float)dt);
        if (!r->circuit_time)
            r->circuit = 0;
        for (int i = 0; i < g->player_count; i++) {
            Player *p = &g->players[i];
            if (p->sheltered || g->matrix.divers[i].active || p->health <= 0)
                continue;
            for (unsigned n = 0; n < 2; n++)
                if (vlen(vsub(position_absolute(p->pos), frontier_pylon(r, n))) < 3) {
                    if (!r->circuit)
                        r->circuit_time = 18;
                    r->circuit |= (uint8_t)(1u << n);
                }
        }
        if (r->circuit == 3) {
            r->frontier_disrupt = 6;
            game_message(g, "COURIER CIRCUIT COMPLETE / WARD DOWN / SIX SECONDS OF RECOVERY");
        }
    }
    if (!r->habitat.contract || r->faction_edit == EDIT_CANCEL)
        return;
    r->hazard_time += (float)dt;
    if (r->hazard_time >= 5.5f) {
        r->hazard_time = 0;
        r->hazard_stage = (uint8_t)((r->hazard_stage + 1) % 3);
    }
    if (!frontier_hazard_live(r))
        return;
    Vec3 at = frontier_hazard(r);
    for (int i = 0; i < g->player_count; i++) {
        Player *p = &g->players[i];
        Vec3 feet = position_absolute(p->pos), delta = vsub(player_eye(p), vadd(at, v3(0, .4, 0)));
        if (p->sheltered || p->lift_active || g->matrix.divers[i].active || p->health <= 0 ||
            fabs(feet.y - at.y) > 2.5 || vlen(v3(feet.x - at.x, 0, feet.z - at.z)) > 3.5 ||
            world_raycast(&g->world, vadd(at, v3(0, .4, 0)), vnorm(delta), vlen(delta)) <
                vlen(delta) - .1)
            continue;
        unsigned risk = r->habitat.contract - 1;
        if (risk == RELIC_COLD)
            p->slow = fmaxf(p->slow, .6f);
        if (risk == RELIC_ORDER)
            p->energy = fmaxf(0, p->energy - (float)dt * 9);
        else if (risk == RELIC_ALIEN)
            p->shield = fmaxf(0, p->shield - (float)dt * 12);
        else
            game_hurt(g, i, (float)dt * (risk == RELIC_HEAT ? 12 : 7),
                      risk == RELIC_COLD ? CHILL : 0);
    }
}
bool frontier_ward(const Game *g, const Room *r, int player) {
    unsigned kind = r->habitat.challenge;
    if (g->world.generator < 12 || r->habitat.pace != PACE_FRONTIER || !r->frontier_active ||
        !r->enemies[0].alive || r->frontier_disrupt > 0)
        return false;
    bool advantage = false;
    const Player *p = &g->players[player];
    if (kind == CH_AERIE)
        advantage = position_absolute(p->pos).y > enemy_center(&r->enemies[0]).y + 1;
    if (kind == CH_CIRCUIT)
        advantage = r->circuit == 3;
    if (kind == CH_CIPHER)
        advantage = (world_systems(&g->world, r->cell) & SYSTEM_SILENCED) != 0;
    return (kind == CH_AERIE || kind == CH_CIRCUIT || kind == CH_CIPHER) && !advantage;
}
float frontier_damage(Game *g, Room *r, int index, int player, float damage) {
    /* The core is always breakable; a positional/system solution also exposes guards. */
    if (frontier_ward(g, r, player))
        damage *= index ? .55f : .8f;
    return damage;
}
void frontier_offer(const Game *g, int player, const Room *r, char *text, size_t size) {
    if (!r->habitat.contract) {
        snprintf(text, size, "%s", frontier_hint(r->habitat.challenge));
        return;
    }
    unsigned relic = r->habitat.contract - 1, rank = g->frontier.ranks[player][relic];
    unsigned required = 5 + rank * 3;
    if (rank >= 3)
        snprintf(text, size, "%s COMPLETE / TOOL AND SALVAGE REMAIN", frontier_relic(relic));
    else
        snprintf(text, size, "%s %u / %s / REQUIRES LAYER %u", frontier_relic(relic), rank + 1,
                 campaign_layer(r->cell) >= required ? "EVOLUTION AVAILABLE" : "GO DEEPER",
                 required);
}
void frontier_claim(Game *g, int player, const Room *r) {
    if (!r->habitat.contract)
        return;
    unsigned relic = r->habitat.contract - 1;
    uint32_t *rank = &g->frontier.ranks[player][relic];
    if (*rank >= 3 || campaign_layer(r->cell) < 5 + *rank * 3)
        return;
    (*rank)++;
    char line[128];
    snprintf(line, sizeof(line), "%s EVOLVED TO %u / %s", frontier_relic(relic), *rank,
             frontier_effect(relic));
    game_message(g, line);
    g->dirty = true;
}
void frontier_shot(const Game *g, int player, WeaponDef *shot) {
    const uint32_t *r = g->frontier.ranks[player];
    if (r[RELIC_HEAT] && (shot->flags & W_BLAST)) {
        shot->flags |= W_BURN;
        if (r[RELIC_HEAT] >= 2)
            shot->blast += 1.5f;
        if (r[RELIC_HEAT] >= 3)
            shot->cost *= .75f;
    }
    if (r[RELIC_COLD] && (shot->flags & W_BEAM)) {
        shot->flags |= W_CHILL;
        if (r[RELIC_COLD] >= 2)
            shot->range += 25;
        if (r[RELIC_COLD] >= 3)
            shot->pierce++;
    }
}
void frontier_player_step(Game *g, int player, double dt) {
    Player *p = &g->players[player];
    p->relic_cooldown = fmaxf(0, p->relic_cooldown - (float)dt);
    const uint32_t *r = g->frontier.ranks[player];
    if (p->sheltered || g->matrix.divers[player].active || p->health <= 0)
        return;
    if (r[RELIC_ORDER] && p->hurt <= 0 &&
        (world_systems(&g->world, world_cell(position_absolute(p->pos))) & SYSTEM_POWER)) {
        p->shield = fminf(player_max_shield(p), p->shield + (float)dt * (2 + r[RELIC_ORDER]));
        if (r[RELIC_ORDER] >= 2 && p->riding == MECH) {
            Rig *rig = &g->rigs[player][0];
            rig->armor =
                fminf(game_rig_max_armor(p, MECH), rig->armor + (float)dt * r[RELIC_ORDER]);
        }
    }
}
bool frontier_valid(const FrontierProgress *f, unsigned generator) {
    for (int p = 0; p < SC_PLAYERS; p++)
        for (int r = 0; r < RELICS; r++)
            if (f->ranks[p][r] > 3 || (generator < 12 && f->ranks[p][r]))
                return false;
    return true;
}
