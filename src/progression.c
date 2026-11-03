#include "game.h"
#include <stdio.h>
float player_max_shield(const Player *p) {
    return 50 + 15.0f * p->mods[1];
}
float player_max_armor(const Player *p) {
    return 40 + 12.0f * p->mods[0];
}
float player_max_energy(const Player *p) {
    return 100 + 20.0f * p->mods[5];
}
uint32_t *player_attribute(Player *p, int attribute) {
    if (attribute == 0)
        return &p->power;
    if (attribute == 1)
        return &p->vitality;
    if (attribute == 2)
        return &p->haste;
    return attribute >= 3 && attribute < SC_ATTRIBUTES ? &p->mods[attribute - 3] : NULL;
}
void game_weapon(Game *g, int player, int delta) {
    if (player < 0 || player >= g->player_count)
        return;
    Player *p = &g->players[player];
    if (p->riding == MECH) {
        p->rig_weapon = (p->rig_weapon + SC_MECH_WEAPONS + (delta < 0 ? -1 : 1)) % SC_MECH_WEAPONS;
        p->cooldown = .15f;
        g->dirty = true;
        game_message(g, game_gun(p)->name);
        return;
    }

    for (int i = 0; i < SC_WEAPONS; i++) {
        p->weapon = (p->weapon + SC_WEAPONS + (delta < 0 ? -1 : 1)) % SC_WEAPONS;
        if (p->weapons & (1u << p->weapon))
            break;
    }
    p->cooldown = fmaxf(p->cooldown, .15f);
    g->dirty = true;
    game_message(g, sc_weapons[p->weapon].name);
}
void game_hurt(Game *g, int player, float damage, uint32_t flags) {
    if (player < 0 || player >= g->player_count || !isfinite(damage) || damage <= 0)
        return;
    Player *p = &g->players[player];
    if (p->sheltered)
        return;
    if (flags & CHILL)
        p->slow = 2;
    if (flags & DRAIN)
        p->energy = fmaxf(0, p->energy - damage);
    float shield = fminf(damage, p->shield);
    p->shield -= shield;
    damage -= shield;
    game_rig_damage(g, player, &damage);
    float absorb = fminf(p->armor, damage * fminf(.65f, .3f + .008f * p->mods[0]));
    p->armor -= absorb * .5f;
    damage -= absorb;
    p->health -= damage;
    p->hurt = 2.5f;
    g->dirty = true;
}
static const Cell objectives[] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 1}, {0, 1, 0}, {2, 1, 2}, {0, 0, 0}};
static const char *missions[] = {"ANSWER THE DISTRESS CONSOLE",
                                 "RESTORE THE CHOIR REACTOR",
                                 "RECOVER THE ARCHIVE KEY",
                                 "AUTHORIZE THE COMMAND RELAY",
                                 "SEVER ORISON'S NEURAL HEART",
                                 "RETURN AND IGNITE THE BEACON",
                                 "THE SLEEPERS HAVE A SKY / EXPEDITION COMPLETE"};
bool game_objective(const Game *g, Cell *cell) {
    if (g->mission >= 6)
        return false;
    *cell = objectives[g->mission];
    return true;
}
const char *game_mission(const Game *g) {
    return missions[g->mission < 6 ? g->mission : 6];
}
Vec3 game_terminal(Cell cell) {
    return vadd(world_lift(cell), v3(0, 0, 5));
}
void game_interact(Game *g, int player) {
    if (player < 0 || player >= g->player_count)
        return;
    Player *p = &g->players[player];
    if (game_rig_interact(g, player))
        return;
    if (game_shelter_interact(g, player))
        return;
    Cell target;
    if (!game_objective(g, &target)) {
        game_message(g, "BEACON LIT / N THEN ENTER BEGINS ANOTHER EXPEDITION");
        return;
    }
    Vec3 d = vsub(position_absolute(p->pos), game_terminal(target));
    if (vlen(d) > 4 || !world_ready(&g->world, target)) {
        game_message(g, "MISSION TERMINAL / FOLLOW THE TARGET COORDINATES IN YOUR JOURNAL");
        return;
    }
    if (g->mission == 4) {
        Room *heart = world_find(&g->world, target);
        if (heart)
            for (int i = 0; i < heart->enemy_count; i++)
                if (heart->enemies[i].kind == 26 && heart->enemies[i].alive) {
                    game_message(g, "NEURAL HEART SEALED / DEFEAT ORISON'S WARDEN");
                    return;
                }
    }
    /* Terminal use is safe at the bay; elsewhere nearby threats must be slain. */
    if (g->mission > 0 && g->mission < 5)
        for (int r = 0; r < g->world.count; r++)
            for (int i = 0; i < g->world.rooms[r].enemy_count; i++) {
                Enemy *e = &g->world.rooms[r].enemies[i];
                if (e->alive && vlen(vsub(enemy_center(e), game_terminal(target))) < 12) {
                    game_message(g, "TERMINAL LOCKED / CLEAR NEARBY INFESTATION");
                    return;
                }
            }
    g->mission++;
    for (int i = 0; i < g->player_count; i++) {
        Player *t = &g->players[i];
        t->points += 2;
        t->credits += 75;
        t->health = player_max_health(t);
        t->shield = player_max_shield(t);
        t->energy = player_max_energy(t);
        const unsigned rewards[] = {0, 1, 4, 3, 8, 15, 15};
        t->weapons |= 1u << rewards[g->mission];
        t->checkpoint = p->pos;
        if (g->mission == 6)
            t->victories++;
    }
    g->dirty = true;
    game_message(g, g->mission == 6 ? "BEACON LIT / SURVIVORS EXTRACTED / YOUR CHARACTER ENDURES"
                                    : game_mission(g));
}
void game_progression_step(Game *g, double dt) {
    for (int i = 0; i < g->player_count; i++) {
        Player *p = &g->players[i];
        p->nourished = fmaxf(0, p->nourished - (float)dt);
        p->slow = fmaxf(0, p->slow - (float)dt);
        p->energy = fminf(player_max_energy(p), p->energy + (float)dt * (12 + 2 * p->mods[5]));
        if (p->hurt <= 0)
            p->shield = fminf(player_max_shield(p), p->shield + (float)dt * (9 + 2 * p->mods[1]));
        if (p->sheltered)
            continue;
        for (int r = 0; r < g->world.count; r++) {
            Room *room = &g->world.rooms[r];
            if (!room->ready)
                continue;
            for (int j = 0; j < room->pickup_count; j++) {
                Pickup *loot = &room->pickups[j];
                if (!loot->active || vlen(position_delta(loot->pos, p->pos)) > 1.65)
                    continue;
                /* Leave useful supplies on the floor until they can help. */
                if ((loot->kind == PICK_MEDKIT && p->health >= player_max_health(p)) ||
                    (loot->kind == PICK_SHIELD && p->shield >= player_max_shield(p)) ||
                    (loot->kind == PICK_ENERGY && p->energy >= player_max_energy(p)) ||
                    (loot->kind == PICK_ARMOR && p->armor >= player_max_armor(p)))
                    continue;
                world_collect(&g->world, room, j);
                if (loot->active)
                    continue;
                p->pickups++;
                g->mission_loot++;
                g->dirty = true;
                if (loot->kind == PICK_ENERGY) {
                    p->energy = player_max_energy(p);
                    game_message(g, "REACTOR CELL / ENERGY RESTORED");
                }
                if (loot->kind == PICK_SALVAGE) {
                    p->credits += 25 + 5 * p->mods[6];
                    game_message(g, "SALVAGE CACHE / UPGRADE CURRENCY");
                }
                if (loot->kind == PICK_MEDKIT) {
                    p->health = fminf(player_max_health(p), p->health + 55);
                    game_message(g, "MEDICAL CACHE / VITALITY RESTORED");
                }
                if (loot->kind == PICK_SHIELD) {
                    p->shield = player_max_shield(p);
                    game_message(g, "SHIELD CELL / BARRIER RESTORED");
                }
                if (loot->kind == PICK_ARMOR) {
                    p->armor = player_max_armor(p);
                    game_message(g, "ARMOR PLATE / SUIT REPAIRED");
                }
                if (loot->kind == PICK_WEAPON) {
                    if (!(p->weapons & (1u << loot->item))) {
                        p->weapons |= 1u << loot->item;
                        p->weapon = loot->item;
                        game_message(g, sc_weapons[loot->item].name);
                    } else {
                        p->credits += 30;
                        game_message(g, "DUPLICATE WEAPON / 30 SALVAGE");
                    }
                }
                if (loot->kind == PICK_LOG) {
                    p->logs |= 1u << loot->item;
                    p->credits += 15;
                    game_message(g, sc_logs[loot->item]);
                }
            }
        }
    }
    unsigned progress[] = {g->mission_kills, g->mission_loot, g->mission};
    const unsigned goals[] = {12, 8, 3};
    const char *names[] = {"CONTRACT COMPLETE / PURGE 12 CREATURES",
                           "CONTRACT COMPLETE / SALVAGE 8 CACHES",
                           "CONTRACT COMPLETE / RESTORE 3 SYSTEMS"};
    for (int c = 0; c < 3; c++)
        if (!(g->contracts & (1u << c)) && progress[c] >= goals[c]) {
            g->contracts |= 1u << c;
            g->dirty = true;
            for (int p = 0; p < g->player_count; p++) {
                g->players[p].points += 2;
                g->players[p].credits += 100;
            }
            game_message(g, names[c]);
        }
}
