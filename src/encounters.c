#include "game.h"
#include "habitat.h"
#include <stdio.h>
#include <stdlib.h>

void encounter_plan(uint32_t seed, RoomSpec *r) {
    Habitat *a = &r->habitat;
    ShipTraits t = campaign_traits(seed, r->cell);
    unsigned variant = (r->hash >> 12) % 3;
    if (r->type == BAY) {
        a->pace = PACE_QUIET;
        return;
    }
    if (t.heat >= 2) {
        static const uint8_t hot[DISTRICT_TYPES][3] = {
            {ASH_AIR, HEAT_HAZE, EMBER_AIR},    {PRESSURE_AIR, STEAM_LEAK, HEAT_HAZE},
            {ASH_AIR, EMBER_AIR, HEAT_HAZE},    {BOILING_AIR, ASH_AIR, HEAT_HAZE},
            {HEAT_HAZE, ASH_AIR, PRESSURE_AIR}, {ASH_AIR, PRESSURE_AIR, HEAT_HAZE},
            {EMBER_AIR, HEAT_HAZE, ASH_AIR},    {BOILING_AIR, STEAM_LEAK, PRESSURE_AIR},
            {ASH_AIR, EMBER_AIR, BOILING_AIR}};
        a->atmosphere = hot[r->district][variant];
    } else if (t.heat <= -2) {
        static const uint8_t cold[DISTRICT_TYPES][3] = {
            {COLD_CLEAR, ICE_SHARDS, RIME_AIR},      {ICE_SHARDS, RIME_AIR, COLD_CLEAR},
            {ICE_SHARDS, COLD_CLEAR, RIME_AIR},      {FROZEN_RUNOFF, COLD_CLEAR, ICE_SHARDS},
            {COLD_CLEAR, ICE_SHARDS, FROZEN_RUNOFF}, {ICE_SHARDS, COLD_CLEAR, RIME_AIR},
            {COLD_CLEAR, FROZEN_RUNOFF, ICE_SHARDS}, {RIME_AIR, ICE_SHARDS, FROZEN_RUNOFF},
            {ICE_SHARDS, RIME_AIR, COLD_CLEAR}};
        a->atmosphere = cold[r->district][variant];
    }
    /* Cold adaptation is one compatible population, not the whole frontier. */
    if (abs(t.heat) >= 2) {
        static const uint8_t by_use[DISTRICT_TYPES] = {FERAL,     MECHANICAL, PHANTOM,
                                                       SCAVENGER, MECHANICAL, PHANTOM,
                                                       SCAVENGER, CRYOPHILE,  BROOD_NEST};
        a->ecology = t.heat < 0 && variant == 0 ? CRYOPHILE : by_use[r->district];
        if (r->incident == BLOOM && variant == 2)
            a->ecology = BROOD_NEST;
    }
    bool staging = false;
    for (int d = 0; d < 4; d++)
        staging |= campaign_guardian(world_neighbor(r->cell, (Direction)d)) >= 0;
    if (campaign_guardian(r->cell) >= 0 || r->height < 5)
        return;
    if (staging || r->history_role == HISTORY_SURVIVORS || r->hash % 6 == 0)
        a->pace = PACE_QUIET;
    else if (r->hash % 6 == 1)
        a->pace = PACE_TRACE;
    else if (!r->history_role && campaign_layer(r->cell) >= 2 && r->hash % 6 == 2)
        a->pace = PACE_FRONTIER;
}

static unsigned behavior(unsigned kind) {
    if (sc_enemies[kind].flags & CLIMB)
        return ALIEN_SEAM;
    if (sc_enemy_bodies[kind].form == HUSK && sc_enemies[kind].role != 1)
        return ALIEN_PILGRIM;
    if (sc_enemy_bodies[kind].form == GAZER && !(sc_enemies[kind].flags & TURRET))
        return ALIEN_GAZER;
    return ALIEN_NONE;
}
void encounter_populate(World *w, Room *r) {
    r->recipe = 0;
    if (campaign_guardian(r->cell) >= 0)
        return;
    if (r->habitat.pace == PACE_QUIET)
        r->enemy_count = 0;
    if (r->habitat.pace == PACE_TRACE)
        r->enemy_count = r->enemy_count < 3 ? r->enemy_count : 3;
    if (r->habitat.pace == PACE_FRONTIER) {
        bool machine = r->habitat.ecology == MECHANICAL;
        const unsigned kinds[] = {machine ? 29u : 9u, 4, 1, 0};
        static const unsigned recipes[6][4] = {{9, 3, 3, 22},   {10, 7, 11, 21}, {29, 16, 2, 25},
                                               {27, 14, 26, 8}, {9, 34, 38, 19}, {29, 40, 37, 41}};
        if (w->generator >= 16 && !r->habitat.challenge)
            r->recipe = (uint8_t)(1 + (r->hash >> 8) % 6);
        if (w->generator >= 16 && !r->habitat.challenge && r->journey.condition == 2)
            r->recipe = (r->hash & 1) ? 4 : 2;
        if (w->generator >= 16 && !r->habitat.challenge && r->journey.condition == 3)
            r->recipe = (r->hash & 1) ? 5 : 6;
        const Vec3 offsets[] = {{0, .02, 14}, {-2, .02, 7}, {0, .02, 25}, {2, .02, 7}};
        r->enemy_count = 4;
        for (int i = 0; i < 4; i++) {
            Enemy *e = &r->enemies[i];
            e->kind = (uint8_t)(r->recipe ? recipes[r->recipe - 1][i] : kinds[i]);
            e->pos = position_from(vadd(world_center(r->cell), offsets[i]));
            e->health = (i ? sc_enemies[e->kind].health : 160) +
                        (float)fmin(campaign_layer(r->cell), 8) * (i ? 6 : 10);
        }
    }
    for (int i = 0; i < r->enemy_count; i++) {
        Enemy *e = &r->enemies[i];
        e->behavior = (uint8_t)behavior(e->kind);
        e->body_heading = e->heading;
        if (!i && r->habitat.pace == PACE_FRONTIER)
            e->behavior = ALIEN_CONDUCTOR;
        else if (r->recipe == 4)
            e->behavior = ALIEN_BRITTLE;
        else if (r->recipe == 5)
            e->behavior = ALIEN_COLONY;
        else if (r->recipe == 6)
            e->behavior = (i & 1) ? ALIEN_LISTENER : ALIEN_REVERSER;
    }
    (void)w;
}
const char *encounter_name(const Room *r) {
    static const char *recipes[] = {"",
                                    "SHIELDED SWARM / CEILING PURSUIT",
                                    "SUPPRESSOR / FLANKING PACK",
                                    "MACHINE RELAY / DENIAL LINE",
                                    "COLD MIRROR / BRITTLE PACK",
                                    "ROOT COLONY / CARRION WAVE",
                                    "LISTENERS / RETURN FIRE"};
    if (r->recipe && r->recipe <= 6)
        return recipes[r->recipe];
    if (r->habitat.challenge)
        return frontier_name(r->habitat.challenge);
    return r->habitat.ecology == MECHANICAL ? "CHOIR RELAY" : "NEST CONDUCTOR";
}
Vec3 encounter_console(const Room *r) {
    return vadd(world_center(r->cell), v3(0, .02, 2));
}
Vec3 local_console(const Room *r) {
    return vadd(world_center(r->cell), v3(r->hash & 1 ? 18 : -18, .02, 3.7));
}
bool local_available(const World *w, const Room *r) {
    return w->generator >= 11 && r && r->type != BAY && r->height >= 5 &&
           (r->habitat.pace == PACE_QUIET || r->habitat.pace == PACE_TRACE) &&
           !(world_local_state(w, r) & LOCAL_RESOLVED);
}
const char *local_action(unsigned tale) {
    static const char *actions[LOCAL_TALES] = {
        "LIGHT EVACUATION MARKERS",  "RESTORE THE GALLEY",
        "REOPEN THE TRIAGE STATION", "CLOSE THE FAILED CONTAINMENT VALVE",
        "REPAIR THE AUXILIARY GRID", "ENERGIZE THE PILGRIM WARD",
        "DISABLE THE MUTINY RELAY",  "RESTORE THE GARDEN WARD"};
    return actions[tale % LOCAL_TALES];
}
CampaignReward encounter_reward(const Game *g, int player, const Room *r) {
    if (r->habitat.challenge) {
        static const CampaignReward tools[CHALLENGES - 1] = {{REWARD_POWER, PSI_FLIGHT},
                                                             {REWARD_POWER, PSI_NOVA},
                                                             {REWARD_RIG, MECH},
                                                             {REWARD_RIG, WAGON},
                                                             {REWARD_MODULE, MOD_SABOTEUR}};
        CampaignReward tool =
            onboarding_reward(g, tools[r->habitat.challenge - 1], campaign_layer(r->cell) - 1);
        if (g->campaign.version >= 2 && tool.kind == REWARD_MODULE)
            tool.item = 1 + (r->hash >> 16) % SC_BUILD_BRANCHES;
        if (!campaign_owns(g, player, tool))
            return tool;
    }
    static const CampaignReward rewards[2][7] = {{{REWARD_GUN, 10},
                                                  {REWARD_POWER, PSI_NOVA},
                                                  {REWARD_GUN, 5},
                                                  {REWARD_RIG, WAGON},
                                                  {REWARD_GUN, 15},
                                                  {REWARD_POWER, PSI_FLIGHT},
                                                  {REWARD_MODULE, MOD_STORM}},
                                                 {{REWARD_GUN, 16},
                                                  {REWARD_RIG, MECH},
                                                  {REWARD_GUN, 17},
                                                  {REWARD_POWER, PSI_VEIL},
                                                  {REWARD_GUN, 8},
                                                  {REWARD_POWER, PSI_LEAP},
                                                  {REWARD_MODULE, MOD_SABOTEUR}}};
    unsigned row = r->habitat.ecology == MECHANICAL, start = (r->hash >> 17) % 7;
    for (unsigned i = 0; i < 7; i++) {
        CampaignReward reward =
            onboarding_reward(g, rewards[row][(start + i) % 7], campaign_layer(r->cell) - 1);
        if (g->campaign.version >= 2 && reward.kind == REWARD_MODULE)
            reward.item = 1 + ((r->hash >> 16) + i) % SC_BUILD_BRANCHES;
        if (!campaign_owns(g, player, reward))
            return reward;
    }
    CampaignReward fallback =
        onboarding_reward(g, rewards[row][start], campaign_layer(r->cell) - 1);
    if (g->campaign.version >= 2 && fallback.kind == REWARD_MODULE)
        fallback.item = 1 + (r->hash >> 16) % SC_BUILD_BRANCHES;
    return fallback;
}
void encounter_reward_name(const Game *g, int player, const Room *r, char *text, size_t size) {
    CampaignReward reward = encounter_reward(g, player, r);
    const char *name = reward.kind == REWARD_GUN     ? sc_weapons[reward.item].name
                       : reward.kind == REWARD_POWER ? sc_psionic_names[reward.item]
                       : reward.kind == REWARD_RIG
                           ? (reward.item == MECH ? "FOLD MECH" : "FOLD WAGON")
                           : campaign_build_name(g, reward.item);
    snprintf(text, size, "%s%s", name,
             campaign_owns(g, player, reward) ? " / +2 POINTS +100 SALVAGE" : "");
}
static void disrupt(Room *r, float seconds) {
    r->frontier_disrupt = seconds;
    r->frontier_attacker = 0;
    for (int i = 0; i < r->enemy_count; i++) {
        Enemy *e = &r->enemies[i];
        e->windup = 0;
        e->motion = MOTION_RECOVER;
        e->motion_time = seconds;
    }
}
bool encounter_passive(const Room *r, int index) {
    return r->habitat.pace == PACE_FRONTIER && index < 4 && !r->frontier_active;
}
bool encounter_attack(const Room *r, int index) {
    return r->habitat.pace != PACE_FRONTIER || index >= 4 ||
           (r->frontier_active && r->frontier_disrupt <= 0 &&
            (!r->enemies[0].alive || (r->frontier_attacker == index && r->enemies[0].windup <= 0)));
}
bool encounter_linked(const World *w, const Room *r, int index) {
    if (r->habitat.challenge || r->habitat.pace != PACE_FRONTIER || !r->frontier_active ||
        index <= 0 || index >= 4 || r->frontier_disrupt > 0 || !r->enemies[0].alive ||
        !r->enemies[index].alive)
        return false;
    Vec3 from = enemy_center(&r->enemies[0]), delta = vsub(enemy_center(&r->enemies[index]), from);
    double length = vlen(delta);
    return length < 26 && world_raycast(w, from, vnorm(delta), length) >= length - .1;
}
float encounter_damage(Game *g, Room *r, int index, float damage, uint32_t flags) {
    if (encounter_passive(r, index))
        return 0;
    if (r->habitat.pace != PACE_FRONTIER || index >= 4)
        return damage;
    bool linked = !r->habitat.challenge && encounter_linked(&g->world, r, index);
    if ((linked || index == 0) && (flags & W_BEAM) && r->enemies[0].alive) {
        Enemy *core = &r->enemies[0];
        core->link_strain += damage / 50;
        if (core->link_strain >= 1) {
            core->link_strain = 0;
            disrupt(r, 2);
            game_message(g, "SUPPORT LINK BROKEN / TWO SECONDS TO PRESS THE ATTACK");
        }
    }
    return linked && r->frontier_disrupt <= 0 ? damage * .55f : damage;
}
static bool nearby_enemy(const Room *r, Vec3 at) {
    for (int i = 0; i < r->enemy_count; i++)
        if (r->enemies[i].alive && vlen(vsub(enemy_center(&r->enemies[i]), at)) < 10)
            return true;
    return false;
}
bool game_encounter_interact(Game *g, int player) {
    if (player < 0 || player >= g->player_count || g->world.generator < 11 ||
        g->players[player].sheltered || g->players[player].lift_active ||
        g->matrix.divers[player].active)
        return false;
    Player *p = &g->players[player];
    Vec3 feet = position_absolute(p->pos);
    Room *r = world_find(&g->world, world_cell(feet));
    if (!r || !r->ready)
        return false;
    uint32_t state = world_local_state(&g->world, r);
    if (r->habitat.pace == PACE_FRONTIER && vlen(vsub(feet, encounter_console(r))) < 3) {
        if (state & FRONTIER_CLEARED) {
            if (!(state & world_eligible_bit(player)) || (state & world_claim_bit(player))) {
                game_message(g, "CHALLENGE CLEARED / YOUR CLAIM IS COMPLETE");
                return true;
            }
            CampaignReward reward = encounter_reward(g, player, r);
            if (!world_mark_local(&g->world, r, world_claim_bit(player)))
                return true;
            bool owned = campaign_owns(g, player, reward);
            campaign_grant(g, player, reward);
            if (!owned)
                game_reward_drill(g, player, reward, r->cell);
            game_message(g, owned ? "FRONTIER REWARD / +2 POINTS / +100 SALVAGE"
                                  : "FRONTIER TOOL EARNED / TRY IT HERE / F10 LOADOUT");
            frontier_claim(g, player, r);
        } else {
            if (r->frontier_active) {
                game_message(g,
                             r->habitat.challenge
                                 ? frontier_hint(r->habitat.challenge)
                                 : "CHALLENGE ACTIVE / DESTROY THE SUPPORT AND ITS THREE GUARDS");
                return true;
            }
            uint32_t participants = 0;
            for (int i = 0; i < g->player_count; i++)
                if (!g->players[i].sheltered && !g->matrix.divers[i].active &&
                    world_find(&g->world, world_cell(position_absolute(g->players[i].pos))) == r)
                    participants |= world_eligible_bit(i);
            if (!world_mark_local(&g->world, r, participants))
                return true;
            r->frontier_active = true;
            r->frontier_timer = 1;
            r->hazard_time = 0;
            r->circuit = 0;
            r->circuit_time = 0;
            game_message(
                g, r->habitat.challenge
                       ? frontier_hint(r->habitat.challenge)
                       : "CHALLENGE AWAKENED / BREAK THE VISIBLE SUPPORT / EXITS REMAIN OPEN");
        }
        g->dirty = true;
        return true;
    }
    if (local_available(&g->world, r) && vlen(vsub(feet, local_console(r))) < 3) {
        if (nearby_enemy(r, feet)) {
            game_message(g, "LOCAL STATION / CLEAR NEARBY HOSTILES FIRST");
            return true;
        }
        uint32_t systems = world_systems(&g->world, r->cell);
        systems = (systems & ~SYSTEM_BLACKOUT) | SYSTEM_POWER;
        if (r->habitat.tale == MUTINY)
            systems |= SYSTEM_SILENCED;
        if (r->habitat.tale == PILGRIMAGE || r->habitat.tale == CULTIVATION ||
            r->habitat.tale == TRIAGE)
            systems |= SYSTEM_BEACON;
        if (!world_set_systems(&g->world, r->cell, systems) ||
            !world_mark_local(&g->world, r, LOCAL_RESOLVED))
            return true;
        player_add_salvage(p, 35);
        if (r->habitat.tale == LAST_FEAST || r->habitat.tale == CULTIVATION)
            p->nourished = fmaxf(p->nourished, 90);
        if (r->habitat.tale == TRIAGE) {
            p->health = player_max_health(p);
            p->armor = player_max_armor(p);
        }
        game_message(g, "LOCAL SYSTEM RESTORED / LIGHTS ON / +35 SALVAGE");
        g->dirty = true;
        return true;
    }
    return false;
}
const char *reward_drill_hint(CampaignReward r) {
    if (r.kind == REWARD_GUN)
        return sc_weapons[r.item].flags & W_BEAM    ? "TRY FIRE / BEAMS BREAK SUPPORT LINKS"
               : sc_weapons[r.item].flags & W_BLAST ? "TRY FIRE / SPLASH CLEARS CLUSTERS"
                                                    : "TRY FIRE / YOUR NEW WEAPON IS EQUIPPED";
    if (r.kind == REWARD_POWER) {
        static const char *hints[] = {
            "TRY V / VEIL / HIDE UNTIL YOU FIRE", "TRY B / FLIGHT / REACH HIGHER SIGHTLINES",
            "TRY J / LEAP / CHARGE A LONG JUMP", "TRY H / NOVA / CLEAR A SURROUNDING SWARM"};
        return hints[r.item];
    }
    if (r.kind == REWARD_RIG)
        return r.item == MECH ? "T DEPLOY / G ENTER MECH / ARMOR AND HEAVY GUNS"
                              : "Y DEPLOY / G ENTER WAGON / FAST EXPLORATION";
    return "TRY FIRE / SPECIALIZATION EQUIPPED / F10 DETAILS";
}
void game_reward_drill(Game *g, int player, CampaignReward reward, Cell site) {
    g->drills[player] = (RewardDrill){reward, site, 90, g->players[player].shots_fired, false};
}
void game_encounters_step(Game *g, double dt) {
    for (int player = 0; player < g->player_count; player++) {
        RewardDrill *d = &g->drills[player];
        Player *p = &g->players[player];
        if (d->time <= 0)
            continue;
        d->time = fmaxf(0, d->time - (float)dt);
        if (p->sheltered || g->matrix.divers[player].active ||
            vlen(vsub(position_absolute(p->pos), world_center(d->site))) > 35)
            continue;
        bool used = d->reward.kind == REWARD_POWER ? (d->reward.item == PSI_VEIL     ? p->cloaked
                                                      : d->reward.item == PSI_FLIGHT ? p->flight > 0
                                                      : d->reward.item == PSI_LEAP   ? p->leap > 0
                                                                                     : p->nova > 0)
                    : d->reward.kind == REWARD_RIG
                        ? p->riding == d->reward.item
                        : p->shots_fired > d->shots && p->muzzle > 0 &&
                              (d->reward.kind == REWARD_MODULE || p->weapon == d->reward.item);
        if (used && !d->complete) {
            d->complete = true;
            d->time = 5;
            game_message(g, "FIELD DRILL COMPLETE / THE NEXT OPTIONAL CHALLENGE REWARDS THIS TOOL");
        }
    }
    for (int room = 0; room < g->world.count; room++) {
        Room *r = &g->world.rooms[room];
        if (!r->ready || !r->frontier_active)
            continue;
        uint32_t present = 0;
        for (int i = 0; i < g->player_count; i++) {
            Player *p = &g->players[i];
            if (!p->sheltered && p->health > 0 && !g->matrix.divers[i].active &&
                world_find(&g->world, world_cell(position_absolute(p->pos))) == r)
                present |= world_eligible_bit(i);
        }
        if (!present) {
            r->frontier_active = false;
            disrupt(r, 1.5f);
            continue;
        }
        uint32_t state = world_local_state(&g->world, r);
        if ((state & present) != present && world_mark_local(&g->world, r, present))
            g->dirty = true;
        unsigned living = 0;
        for (int i = 0; i < 4; i++)
            living += r->enemies[i].alive;
        if (!living) {
            if (world_mark_local(&g->world, r, FRONTIER_CLEARED)) {
                r->frontier_active = false;
                g->dirty = true;
                game_message(g,
                             "CHALLENGE CLEARED / RETURN TO THE WHITE BEACON TO CLAIM YOUR TOOL");
            }
            continue;
        }
        frontier_step(g, r, dt);
        r->frontier_disrupt = fmaxf(0, r->frontier_disrupt - (float)dt);
        if (!r->enemies[0].alive) {
            if (r->frontier_attacker != 255) {
                disrupt(r, 1.5f);
                r->frontier_attacker = 255;
                game_message(g, "CONDUCTOR DESTROYED / LINKS COLLAPSE / THE PACK IS EXPOSED");
            }
            continue;
        }
        Enemy *core = &r->enemies[0];
        core->link_strain = fmaxf(0, core->link_strain - (float)dt * .12f);
        core->windup = fmaxf(0, core->windup - (float)dt);
        r->frontier_timer -= (float)dt;
        if (r->frontier_timer <= 0 && r->frontier_disrupt <= 0) {
            for (int i = 0; i < 3; i++) {
                r->frontier_attacker = (uint8_t)(1 + r->frontier_attacker % 3);
                if (r->enemies[r->frontier_attacker].alive)
                    break;
            }
            core->windup = 1;
            r->frontier_timer = 3.4f;
        }
    }
}
