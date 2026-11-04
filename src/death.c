#include "game.h"
#include <string.h>

void game_deaths_reset(Game *g) {
    memset(g->deaths, 0, sizeof(g->deaths));
    memset(g->death_hazards, 0, sizeof(g->death_hazards));
}
void game_enemy_death(Game *g, const Enemy *enemy, int owner) {
    unsigned slot = 0;
    for (unsigned i = 0; i < SC_MAX_DEATHS; i++) {
        if (!g->deaths[i].active) {
            slot = i;
            break;
        }
        if (g->deaths[i].age > g->deaths[slot].age)
            slot = i;
    }
    Vec3 center = enemy_center(enemy);
    double drop = world_raycast(&g->world, center, v3(0, -1, 0), SC_DECK);
    Vec3 landing = vsub(center, v3(0, drop < SC_DECK ? drop - .08 : .55, 0));
    g->deaths[slot] = (EnemyDeath){.body = *enemy, .landing = landing, .active = true};
    g->deaths[slot].body.flash = g->deaths[slot].body.windup = 0;
    DeathEffect kind = sc_death_effects[enemy->kind];
    if (kind == DEATH_NONE)
        return;
    for (unsigned i = 0; i < SC_MAX_DEATH_HAZARDS; i++)
        if (!g->death_hazards[i].active) {
            bool blast = kind == DEATH_BLAST;
            g->death_hazards[i] = (DeathHazard){.pos = position_from(blast ? center : landing),
                                                .kind = kind,
                                                .life = blast                ? .95f
                                                        : kind == DEATH_ACID ? 7
                                                                             : 9,
                                                .timer = kind == DEATH_ACID ? .5f : .95f,
                                                .radius = kind == DEATH_ACID ? 2.7f : 4.8f,
                                                .damage = kind == DEATH_ACID ? 5 : 22,
                                                .owner = owner,
                                                .active = true};
            return;
        }
}
static bool exposed(const Game *g, const DeathHazard *h, Vec3 target, double radius) {
    Vec3 at = vadd(position_absolute(h->pos), v3(0, .18, 0));
    Vec3 delta = vsub(target, at);
    double distance = vlen(delta);
    return world_cell(at).y == world_cell(target).y && distance < radius &&
           world_raycast(&g->world, at, vnorm(delta), distance) >= distance - .08;
}
static void burst(Game *g, DeathHazard *h) {
    h->spent = true;
    h->life = .4f;
    for (int p = 0; p < g->player_count; p++)
        if (!g->players[p].sheltered && exposed(g, h, player_eye(&g->players[p]), h->radius))
            game_hurt(g, p, h->damage, 0);
    /* Later deaths enqueue their own warning; never recurse through an immediate chain. */
    for (int r = 0; r < g->world.count; r++) {
        Room *room = &g->world.rooms[r];
        for (int i = 0; i < room->enemy_count; i++)
            if (room->enemies[i].alive && exposed(g, h, enemy_center(&room->enemies[i]), h->radius))
                game_damage_enemy(g, room, i, h->owner, h->damage * 2, 0);
    }
}
void game_deaths_step(Game *g, double dt) {
    if (!isfinite(dt) || dt <= 0 || dt > .05)
        return;
    for (int i = 0; i < SC_MAX_DEATHS; i++) {
        EnemyDeath *d = &g->deaths[i];
        if (d->active) {
            d->age += (float)dt;
            if (d->age >= SC_DEATH_LIFE)
                d->active = false;
        }
    }
    for (int i = 0; i < SC_MAX_DEATH_HAZARDS; i++) {
        DeathHazard *h = &g->death_hazards[i];
        if (!h->active)
            continue;
        h->age += (float)dt;
        h->life -= (float)dt;
        h->timer -= (float)dt;
        if (h->spent) {
            if (h->life <= 0)
                h->active = false;
            continue;
        }
        if (h->kind != DEATH_BLAST && h->life <= 0) {
            h->active = false;
            continue;
        }
        if (h->kind == DEATH_BLAST) {
            if (h->timer <= 0)
                burst(g, h);
        } else if (h->kind == DEATH_ACID) {
            if (h->timer <= 0) {
                h->timer = .5f;
                for (int p = 0; p < g->player_count; p++)
                    if (!g->players[p].sheltered &&
                        exposed(g, h, player_eye(&g->players[p]), h->radius))
                        game_hurt(g, p, h->damage, 0);
            }
        } else if (h->triggered) {
            if (h->timer <= 0)
                burst(g, h);
        } else if (h->age >= .95f) {
            for (int p = 0; p < g->player_count; p++)
                if (!g->players[p].sheltered && exposed(g, h, player_eye(&g->players[p]), 3.2) &&
                    !game_story_safe(g, position_absolute(g->players[p].pos))) {
                    h->triggered = true;
                    h->timer = .65f;
                    h->life = fmaxf(h->life, .7f);
                }
        }
        if (!h->spent && h->life <= 0)
            h->active = false;
    }
}
