#include "game.h"
#include <stdio.h>
#include <string.h>

unsigned campaign_phase(const Campaign *c) {
    unsigned phase = 0;
    while (phase < SC_PHASES && (c->defeated & (3u << (phase * 2))))
        phase++;
    return phase;
}
void campaign_sync(Game *g) {
    unsigned phase = campaign_phase(&g->campaign);
    g->world.open_layer = g->campaign.version && phase < SC_PHASES ? phase + 1 : 0;
}
void campaign_init(Game *g) {
    memset(&g->campaign, 0, sizeof(g->campaign));
    g->campaign.version = g->world.generator >= 16 ? 2 : 1;
    for (int i = 0; i < SC_PLAYERS; i++) {
        g->players[i].weapons = g->world.generator >= 15 ? 1 : 3;
        g->players[i].rules = g->world.generator >= 16 ? 2 : 0;
        g->campaign.profiles[i] =
            (CampaignProfile){.guns = {0, g->world.generator >= 15 ? SC_EMPTY_GUN : 1},
                              .power_slots = {SC_EMPTY_POWER, SC_EMPTY_POWER}};
    }
    campaign_sync(g);
}
bool campaign_weapon(const Game *g, int player, unsigned weapon) {
    if (player < 0 || player >= SC_PLAYERS || weapon >= SC_WEAPONS)
        return false;
    const CampaignProfile *p = &g->campaign.profiles[player];
    return !g->campaign.version || p->guns[0] == weapon || p->guns[1] == weapon;
}
bool campaign_power(const Game *g, int player, unsigned power) {
    if (player < 0 || player >= SC_PLAYERS || power >= SC_PSIONICS)
        return false;
    const CampaignProfile *p = &g->campaign.profiles[player];
    return !g->campaign.version || ((p->powers & (1u << power)) &&
                                    (p->power_slots[0] == power || p->power_slots[1] == power));
}
bool campaign_rig(const Game *g, int player, unsigned rig) {
    if (player < 0 || player >= SC_PLAYERS || rig < MECH || rig > WAGON)
        return false;
    const CampaignProfile *p = &g->campaign.profiles[player];
    return !g->campaign.version || ((p->rigs & (1u << (rig - 1))) && p->rig == rig);
}
bool campaign_safe_refit(const Game *g, int player) {
    if (player < 0 || player >= g->player_count)
        return false;
    const Player *p = &g->players[player];
    if (p->health <= 0 || p->riding || p->lift_active || g->matrix.divers[player].active)
        return false;
    if (p->sheltered || cell_equal(world_cell(position_absolute(p->pos)), (Cell){0}))
        return true;
    for (unsigned i = 0; i < SC_GUARDIANS; i++)
        if ((g->campaign.defeated & (1u << i)) &&
            vlen(vsub(position_absolute(p->pos), game_terminal(campaign_site(i)))) < 8)
            return true;
    return false;
}
bool campaign_refit(Game *g, int player, unsigned slot, int delta) {
    if (!g->campaign.version || slot >= 6 || !campaign_safe_refit(g, player)) {
        game_message(g, "REFIT AT THE BAY, SHELTER OR A DEFEATED GUARDIAN'S STAGING CONSOLE");
        return false;
    }
    CampaignProfile *c = &g->campaign.profiles[player];
    Player *p = &g->players[player];
    uint32_t *at = slot < 2    ? &c->guns[slot]
                   : slot < 4  ? &c->power_slots[slot - 2]
                   : slot == 4 ? &c->rig
                               : &c->module;
    unsigned count = slot < 2                   ? SC_WEAPONS
                     : slot < 4                 ? 5
                     : slot == 4                ? 3
                     : g->campaign.version >= 2 ? SC_BUILD_BRANCHES + 1
                                                : 5;
    unsigned next =
        slot == 5 && g->views[player].branch_preview ? g->views[player].branch_preview - 1 : *at;
    for (unsigned i = 0; i < count; i++) {
        next = (next + (delta < 0 ? count - 1 : 1)) % count;
        bool valid = slot < 2    ? (p->weapons & (1u << next)) && c->guns[slot ^ 1] != next
                     : slot < 4  ? next == SC_EMPTY_POWER || ((c->powers & (1u << next)) &&
                                                             c->power_slots[(slot - 2) ^ 1] != next)
                     : slot == 4 ? !next || (c->rigs & (1u << (next - 1)))
                                 : !next || (c->modules & (1u << (next - 1)));
        if (!valid)
            continue;
        if (slot == 5 && g->campaign.version >= 2) {
            g->views[player].branch_preview = next + 1;
            game_player_message(g, game_player_id(g, player),
                                "BRANCH PREVIEW / G OR PAD A TO EQUIP / ENTER CANCELS");
            return true;
        }
        *at = next;
        g->builds[player] = (BuildEffects){0};
        if (slot < 2)
            p->weapon = next;
        if (slot >= 2 && slot < 4) {
            p->cloaked = false;
            p->flight = p->leap = p->nova = p->nova_tick = 0;
        }
        if (slot == 4)
            for (int k = 0; k < 2; k++)
                if ((unsigned)k + 1 != next)
                    g->rigs[player][k].deployed = false;
        g->dirty = true;
        game_message(g, "LOADOUT UPDATED / OWNED EQUIPMENT STAYS IN STORAGE");
        return true;
    }
    return false;
}
void campaign_track(Game *g) {
    g->politics.tracked = 0;
    if (!g->campaign.version)
        return;
    unsigned phase = campaign_phase(&g->campaign);
    for (unsigned n = 0; n < SC_GUARDIANS; n++) {
        g->campaign.tracked = (g->campaign.tracked + 1) % (SC_GUARDIANS + 1);
        unsigned id = g->campaign.tracked;
        if (id == SC_GUARDIANS)
            break;
        if (!(g->campaign.defeated & (1u << id)) && (id == 8 || id / 2 <= phase))
            break;
    }
    g->dirty = true;
}
bool campaign_objective(const Game *g, Cell *cell) {
    if (!g->campaign.version || !(g->story.done & 1))
        return false;
    unsigned phase = campaign_phase(&g->campaign);
    unsigned id = g->campaign.tracked;
    if (id == SC_GUARDIANS)
        return false;
    if (g->campaign.defeated & (1u << id)) {
        if (phase >= SC_PHASES)
            return false;
        id = phase * 2;
    }
    if (phase > 0 && id < 8 && id / 2 >= phase &&
        !(g->story.assigned & (1u << game_story_target(g))))
        return false;
    *cell = campaign_site(id);
    return true;
}
void campaign_journal(Game *g, int player) {
    if (player < 0 || player >= g->player_count)
        return;
    g->views[player].branch_preview = 0;
    Player *p = &g->players[player];
    PLAYER_VIEW(g, p).journal = true;
    PLAYER_VIEW(g, p).journal_page = 5;
    g->campaign.choice[player] = 0;
    if (!g->campaign.version)
        return;
    for (unsigned i = 0; i < SC_GUARDIANS; i++)
        if ((g->campaign.defeated & (1u << i)) && (g->campaign.eligible[i] & (1u << player)) &&
            !(g->campaign.profiles[player].claimed & (1u << i))) {
            g->campaign.choice[player] = i + 1;
            return;
        }
    for (unsigned i = 0; i < SC_GUARDIANS; i++)
        if (vlen(vsub(position_absolute(p->pos), game_terminal(campaign_site(i)))) < 8) {
            g->campaign.tracked = i;
            break;
        }
}
CampaignReward campaign_reward(uint32_t seed, unsigned id, unsigned choice) {
    unsigned family = campaign_family(seed, id);
    uint32_t h = world_hash(seed, campaign_site(id), 0x52455744u);
    static const unsigned guns[] = {16, 4, 2, 7, 17, 10, 9, 11, 18, 19, 3, 5, 6, 8, 12, 13, 14, 15};
    if (choice == 0)
        return (CampaignReward){REWARD_GUN, guns[(family * 2 + h % 18) % 18]};
    if (choice == 1) {
        if (family < 2)
            return (CampaignReward){REWARD_RIG, family == 0 ? MECH : WAGON};
        return (CampaignReward){REWARD_POWER, (h >> 8) % SC_PSIONICS};
    }
    if (id == 8)
        return family < 2 ? (CampaignReward){REWARD_POWER, (h >> 8) % SC_PSIONICS}
                          : (CampaignReward){REWARD_RIG, 1 + (h >> 16) % 2};
    return (CampaignReward){REWARD_MODULE, 1 + (family == 0   ? 1
                                                : family == 1 ? 2
                                                : family == 4 ? 3
                                                              : 0)};
}
CampaignReward campaign_offer(const Game *g, unsigned id, unsigned choice) {
    unsigned tier = id == 8 ? 0 : id / 2;
    CampaignReward r = campaign_reward(g->world.seed, id, choice);
    if (g->campaign.version >= 2 && r.kind == REWARD_MODULE)
        r.item = 1 + 2 * ((r.item - 1) % 3);
    if (onboarding_active(g) && choice == 1 && tier)
        r = tier == 1
                ? (CampaignReward){REWARD_POWER, campaign_family(g->world.seed, id) % SC_PSIONICS}
                : (CampaignReward){REWARD_RIG, 1 + campaign_family(g->world.seed, id) % 2};
    return onboarding_reward(g, r, tier);
}
bool campaign_owns(const Game *g, int player, CampaignReward reward) {
    const CampaignProfile *c = &g->campaign.profiles[player];
    unsigned mask = reward.kind == REWARD_GUN     ? g->players[player].weapons
                    : reward.kind == REWARD_POWER ? c->powers
                    : reward.kind == REWARD_RIG   ? c->rigs
                                                  : c->modules;
    return (mask & (1u << (reward.item - (reward.kind >= REWARD_RIG)))) != 0;
}
void campaign_reward_name(const Game *g, int player, unsigned id, unsigned choice, char *text,
                          size_t size) {
    CampaignReward r = campaign_offer(g, id, choice);
    const char *name = r.kind == REWARD_GUN     ? sc_weapons[r.item].name
                       : r.kind == REWARD_POWER ? sc_psionic_names[r.item]
                       : r.kind == REWARD_RIG   ? (r.item == MECH ? "FOLD MECH" : "FOLD WAGON")
                                                : campaign_build_name(g, r.item);
    if (g->campaign.version >= 2 && r.kind == REWARD_MODULE && !campaign_owns(g, player, r)) {
        const char *requirement = r.item <= BUILD_FOCUS    ? "BEAM / RELAY ALSO NEEDS FLARE"
                                  : r.item <= BUILD_AMBUSH ? "VEIL"
                                  : r.item <= BUILD_BRACE  ? "MECH"
                                                           : "JUMP OR FLIGHT";
        snprintf(text, size, "%s / REQUIRES %s", name, requirement);
        return;
    }
    snprintf(text, size, "%s%s", name,
             campaign_owns(g, player, r) ? " / OWNED: +2 POINTS +100 SALVAGE" : "");
}
void campaign_grant(Game *g, int player, CampaignReward r) {
    CampaignProfile *build = &g->campaign.profiles[player];
    Player *p = &g->players[player];
    if (campaign_owns(g, player, r)) {
        p->points = (uint32_t)fmin(1000, p->points + 2);
        p->credits += 100;
    } else if (r.kind == REWARD_GUN) {
        p->weapons |= 1u << r.item;
        build->guns[1] = p->weapon = r.item;
    } else if (r.kind == REWARD_POWER) {
        build->powers |= 1u << r.item;
        build->power_slots[build->power_slots[0] == SC_EMPTY_POWER ? 0 : 1] = r.item;
        p->selected_power = (uint8_t)r.item;
        p->cloaked = false;
        p->flight = p->leap = p->nova = p->nova_tick = 0;
    } else if (r.kind == REWARD_RIG) {
        build->rigs |= 1u << (r.item - 1);
        if (!p->riding) {
            build->rig = r.item;
            for (int k = 0; k < 2; k++)
                g->rigs[player][k].deployed = false;
        }
    } else {
        build->modules |= 1u << (r.item - 1);
        build->module = r.item;
    }
    g->dirty = true;
}
bool campaign_choose(Game *g, int player, unsigned choice) {
    if (!g->campaign.version || player < 0 || player >= g->player_count || choice < 1 || choice > 3)
        return false;
    Campaign *c = &g->campaign;
    unsigned id = c->choice[player] - 1;
    CampaignProfile *build = &c->profiles[player];
    if (id >= SC_GUARDIANS || !(c->defeated & (1u << id)) || !(c->eligible[id] & (1u << player)) ||
        (build->claimed & (1u << id)))
        return false;
    CampaignReward reward = campaign_offer(g, id, choice - 1);
    campaign_grant(g, player, reward);
    game_reward_drill(g, player, reward, campaign_site(id));
    build->claimed |= 1u << id;
    c->choice[player] = 0;
    g->dirty = true;
    game_message(g, "GUARDIAN REWARD SECURED / F10 LOADOUT / YOUR BUILD BELONGS TO THIS SHIP");
    return true;
}
static bool in_arena(const Game *g, int player, unsigned id) {
    const Player *p = &g->players[player];
    if (p->sheltered || p->health <= 0)
        return false;
    RoomSpec room = world_describe_version(g->world.seed, g->world.generator,
                                           world_cell(position_absolute(p->pos)));
    return cell_equal(room.cell, campaign_site(id));
}
bool campaign_engaged(const Game *g, int player) {
    if (!g->campaign.version || player < 0 || player >= g->player_count)
        return false;
    for (unsigned i = 0; i < SC_GUARDIANS; i++)
        if (g->campaign.attempts[i].active &&
            (g->campaign.attempts[i].participants & (1u << player)) && in_arena(g, player, i))
            return true;
    return false;
}
bool campaign_interact(Game *g, int player) {
    if (!g->campaign.version || g->players[player].sheltered)
        return false;
    for (unsigned id = 0; id < SC_GUARDIANS; id++) {
        Cell site = campaign_site(id);
        if (vlen(vsub(position_absolute(g->players[player].pos), game_terminal(site))) > 4)
            continue;
        Room *r = world_find(&g->world, site);
        if (!r || !r->ready)
            return true;
        if (g->campaign.defeated & (1u << id)) {
            campaign_journal(g, player);
            return true;
        }
        GuardianAttempt *a = &g->campaign.attempts[id];
        if (a->active)
            return true;
        if (id != 8 && id / 2 > campaign_phase(&g->campaign))
            return true;
        *a = (GuardianAttempt){.active = true, .timer = 1.2f};
        for (int p = 0; p < g->player_count; p++)
            if (in_arena(g, p, id)) {
                a->participants |= 1u << p;
                g->players[p].checkpoint = position_from(game_terminal(site));
            }
        r->enemies[0].health = campaign_guardian_health(id, a->participants);
        guardian_begin(g, r, id);
        g->dirty = true;
        game_message(g, "GUARDIAN ENGAGED / DODGE THE WARNING / ATTACK WHEN ITS CORE OPENS");
        return true;
    }
    return false;
}
void campaign_defeat(Game *g, unsigned id) {
    if (!g->campaign.version || id >= SC_GUARDIANS || (g->campaign.defeated & (1u << id)))
        return;
    Campaign *c = &g->campaign;
    c->defeated |= 1u << id;
    c->eligible[id] = c->attempts[id].participants;
    c->attempts[id] = (GuardianAttempt){0};
    Room *room = world_find(&g->world, campaign_site(id));
    if (c->version >= 2 && id != 8 && room)
        for (int i = 1; i < room->enemy_count; i++)
            room->enemies[i].alive = false;
    for (int shot = 0; shot < SC_MAX_PROJECTILES; shot++)
        if (g->projectiles[shot].guardian == id + 1)
            g->projectiles[shot].active = false;
    for (int p = 0; p < g->player_count; p++)
        if (c->eligible[id] & (1u << p)) {
            g->players[p].checkpoint = position_from(game_terminal(campaign_site(id)));
            g->players[p].health = player_max_health(&g->players[p]);
            g->players[p].shield = player_max_shield(&g->players[p]);
        }
    campaign_sync(g);
    c->tracked = campaign_phase(c) < SC_PHASES ? campaign_phase(c) * 2 : SC_GUARDIANS;
    g->dirty = true;
    game_message(g, "GUARDIAN DEFEATED / F10 CLAIM YOUR REWARD / THE NEXT BOUNDARY IS OPEN");
}
void campaign_step(Game *g, double dt) {
    (void)dt;
    if (!g->campaign.version)
        return;
    for (unsigned id = 0; id < SC_GUARDIANS; id++) {
        GuardianAttempt *a = &g->campaign.attempts[id];
        if (!a->active)
            continue;
        bool engaged = false;
        for (int p = 0; p < g->player_count; p++)
            engaged |= (a->participants & (1u << p)) && in_arena(g, p, id);
        if (engaged)
            continue;
        *a = (GuardianAttempt){0};
        for (int shot = 0; shot < SC_MAX_PROJECTILES; shot++)
            if (g->projectiles[shot].guardian == id + 1)
                g->projectiles[shot].active = false;
        Room *r = world_find(&g->world, campaign_site(id));
        if (r && r->ready) {
            campaign_populate(&g->world, r);
            r->enemies[0].windup = r->enemies[0].slow = r->enemies[0].burn = 0;
        }
        g->dirty = true;
        game_message(g, "GUARDIAN ATTEMPT ENDED / RECOVER AND USE ITS STAGING CONSOLE TO RETRY");
    }
}
float campaign_attack_bonus(const Game *g, int player) {
    if (!g->campaign.version || g->campaign.version >= 2)
        return 1;
    const Player *p = &g->players[player];
    unsigned module = g->campaign.profiles[player].module;
    if (module == MOD_HUNTER && p->cloaked)
        return 2.5f;
    if (module == MOD_STORM && p->flight > 0)
        return 1.6f;
    if (module == MOD_SIEGE && p->riding == MECH)
        return 1.5f;
    if (module == MOD_SABOTEUR &&
        (world_systems(&g->world, world_cell(position_absolute(p->pos))) & SYSTEM_SILENCED))
        return 1.6f;
    return 1;
}
float campaign_damage(Game *g, const Room *r, int index, int owner, float damage) {
    if (!g->campaign.version || index != 0)
        return damage;
    int id = campaign_guardian(r->cell);
    if (id < 0)
        return damage;
    GuardianAttempt *a = &g->campaign.attempts[id];
    if (!a->active || !(a->participants & (1u << owner)) || !in_arena(g, owner, (unsigned)id))
        return 0;
    return damage * (a->opening > 0 ? 1.6f : g->campaign.version >= 2 ? 1 : .55f);
}
void campaign_resume(Game *g) {
    for (unsigned i = 0; i < SC_GUARDIANS; i++) {
        GuardianAttempt *a = &g->campaign.attempts[i];
        if (a->active)
            for (int p = 0; p < SC_PLAYERS; p++)
                if (a->participants & (1u << p)) {
                    matrix_disconnect(g, p, false);
                    g->players[p].pos = g->players[p].checkpoint;
                    g->players[p].health = player_max_health(&g->players[p]);
                    g->players[p].shield = player_max_shield(&g->players[p]);
                    g->players[p].energy = player_max_energy(&g->players[p]);
                    g->players[p].riding = 0;
                    g->players[p].sheltered = false;
                }
        *a = (GuardianAttempt){0};
    }
    campaign_sync(g);
}
bool campaign_valid(const Campaign *c, const Player *players, const World *w) {
    if (c->version == 0) {
        Campaign empty = {0};
        return w->generator < 7 && memcmp(c, &empty, sizeof(empty)) == 0;
    }
    if (c->version != (w->generator >= 16 ? 2u : 1u) || w->generator < 7 ||
        w->generator > SC_GENERATOR_VERSION || c->defeated >= (1u << SC_GUARDIANS) ||
        c->tracked > SC_GUARDIANS)
        return false;
    unsigned phase = campaign_phase(c);
    for (unsigned i = 0; i < SC_GUARDIANS; i++) {
        bool done = (c->defeated & (1u << i)) != 0;
        const GuardianAttempt *a = &c->attempts[i];
        if (a->broken > 3 || (unsigned)a->stage > GUARDIAN_OVERRUN ||
            (!a->active && (a->broken || a->stage)) ||
            (c->version < 2 && (a->broken || a->stage)))
            return false;
        if (c->eligible[i] > SC_PLAYER_MASK || done != (c->eligible[i] != 0) ||
            a->participants > SC_PLAYER_MASK || a->active != (a->participants != 0) ||
            (done && a->active) || (i < 8 && i / 2 > phase && (done || a->active)))
            return false;
        bool killed = false;
        for (size_t j = 0; j < w->record_count; j++)
            if (cell_equal(w->records[j].cell, campaign_site(i)))
                killed = (w->records[j].killed & 1) != 0;
        if (done != killed)
            return false;
    }
    for (int i = 0; i < SC_PLAYERS; i++) {
        const CampaignProfile *p = &c->profiles[i];
        Player profile = players[i];
        if (profile.rules != (w->generator >= 16 ? 2u : 0u))
            return false;
        if (c->version >= 2 && (profile.health > player_max_health(&profile) ||
                                profile.shield > player_max_shield(&profile) ||
                                profile.armor > player_max_armor(&profile) ||
                                profile.energy > player_max_energy(&profile)))
            return false;
        for (int s = 0; s < SC_ATTRIBUTES; s++)
            if (*player_attribute(&profile, s) > 12)
                return false;
        unsigned equipped = 0;
        for (int s = 0; s < 2; s++)
            if (p->power_slots[s] < SC_PSIONICS)
                equipped |= 1u << p->power_slots[s];
        if ((profile.cloaked && !(equipped & (1u << PSI_VEIL))) ||
            (profile.flight > 0 && !(equipped & (1u << PSI_FLIGHT))) ||
            (profile.leap > 0 && !(equipped & (1u << PSI_LEAP))) ||
            (profile.nova > 0 && !(equipped & (1u << PSI_NOVA))))
            return false;
        unsigned branches = c->version >= 2 ? SC_BUILD_BRANCHES : SC_SPECIALIZATIONS;
        if (p->powers >= 16 || p->rigs >= 4 || p->modules >= (1u << branches) ||
            p->claimed & ~c->defeated || p->guns[0] >= SC_WEAPONS || p->guns[1] > SC_EMPTY_GUN ||
            (p->guns[1] == SC_EMPTY_GUN && w->generator < 15) || p->guns[0] == p->guns[1] ||
            !(players[i].weapons & (1u << p->guns[0])) ||
            (p->guns[1] != SC_EMPTY_GUN && !(players[i].weapons & (1u << p->guns[1]))) ||
            (players[i].weapon != p->guns[0] && players[i].weapon != p->guns[1]) || p->rig > 2 ||
            (p->rig && !(p->rigs & (1u << (p->rig - 1)))) || p->module > branches ||
            (p->module && !(p->modules & (1u << (p->module - 1)))) ||
            (players[i].riding && players[i].riding != p->rig))
            return false;
        for (int s = 0; s < 2; s++)
            if (p->power_slots[s] > SC_EMPTY_POWER ||
                (p->power_slots[s] < SC_EMPTY_POWER &&
                 (!(p->powers & (1u << p->power_slots[s])) ||
                  p->power_slots[s] == p->power_slots[s ^ 1])))
                return false;
        for (unsigned n = 0; n < SC_GUARDIANS; n++)
            if ((p->claimed & (1u << n)) && !(c->eligible[n] & (1u << i)))
                return false;
        if (phase < SC_PHASES &&
            (campaign_layer(world_cell(position_absolute(players[i].pos))) > phase + 1 ||
             campaign_layer(world_cell(position_absolute(players[i].checkpoint))) > phase + 1))
            return false;
    }
    return true;
}
