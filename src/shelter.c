#include "game.h"

/* Interior metres are independent of ship sectors. Collision and rendering use
   these same furnishings; the doorway is a transition, never a ship cell. */
static const Box furniture[] = {{{-7.6, 0, 4}, {-5, .8, 7.1}, {.28f, .32f, .37f}, 0, true},
                                {{5, 0, 4.8}, {8, 1.25, 6.5}, {.25f, .42f, .4f}, 0, true},
                                {{-8, 0, -2.8}, {-4.5, 1.1, -1.2}, {.42f, .32f, .23f}, 0, true},
                                {{-1.8, 0, 1.2}, {1.8, .85, 2.7}, {.35f, .25f, .18f}, 0, true},
                                {{-1.6, 0, 3.2}, {1.6, .5, 3.8}, {.3f, .24f, .2f}, 0, true},
                                {{6.5, 0, -5.8}, {8.4, 2.2, -4.8}, {.2f, .29f, .32f}, 0, true}};
const Box *shelter_furniture(size_t *count) {
    *count = sizeof(furniture) / sizeof(furniture[0]);
    return furniture;
}
Vec3 shelter_station(ShelterUse use) {
    const Vec3 stations[] = {
        {0, 0, 0}, {0, 0, -7.5}, {-6.3, 0, 3.3}, {6.5, 0, 4.1}, {-6.2, 0, -3.5}};
    return stations[use >= SHELTER_DOOR && use <= SHELTER_KITCHEN ? use : SHELTER_NONE];
}
bool shelter_clear(Vec3 feet) {
    if (!isfinite(feet.x) || !isfinite(feet.y) || !isfinite(feet.z) || feet.y < 0 || feet.y > .1 ||
        fabs(feet.x) > 8.6 || fabs(feet.z) > 7.6)
        return false;
    for (size_t i = 0; i < sizeof(furniture) / sizeof(furniture[0]); i++) {
        const Box *b = &furniture[i];
        if (feet.x + .34 > b->min.x && feet.x - .34 < b->max.x && feet.z + .34 > b->min.z &&
            feet.z - .34 < b->max.z)
            return false;
    }
    return true;
}
void game_summon(Game *g, int player) {
    if (player < 0 || player >= g->player_count)
        return;
    for (int i = 0; i < g->player_count; i++)
        if (g->players[i].sheltered) {
            game_message(g, "FOLD SHELTER OCCUPIED / THE DOOR CANNOT MOVE");
            return;
        }
    Player *p = &g->players[player];
    Vec3 feet = position_absolute(p->pos);
    Vec3 door = vadd(feet, v3(sin(p->yaw) * 2.2, 0, cos(p->yaw) * 2.2));
    if (feet.z < 0 || !world_ready(&g->world, world_cell(door)) ||
        world_overlap(&g->world, door, 1, 2.8) ||
        !world_overlap(&g->world, vadd(door, v3(0, -.15, 0)), .8, .1) ||
        world_raycast(&g->world, player_eye(p),
                      vnorm(vsub(vadd(door, v3(0, 1.4, 0)), player_eye(p))), 2.2) < 2.1) {
        game_message(g, "SHELTER NEEDS CLEAR SHIP FLOOR / TRY A WIDER PASSAGE");
        return;
    }
    g->shelter = (Shelter){position_from(door), p->yaw, true};
    g->dirty = true;
    game_message(g, "FOLD SHELTER SUMMONED / G AT THE DOOR TO ENTER");
}
ShelterUse game_shelter_near(const Game *g, int player) {
    if (player < 0 || player >= g->player_count)
        return SHELTER_NONE;
    const Player *p = &g->players[player];
    if (p->sheltered) {
        for (int use = SHELTER_DOOR; use <= SHELTER_KITCHEN; use++)
            if (vlen(vsub(p->shelter_pos, shelter_station((ShelterUse)use))) < 2.2)
                return (ShelterUse)use;
    } else if (position_absolute(p->pos).z >= 0 && g->shelter.deployed &&
               vlen(position_delta(p->pos, g->shelter.door)) < 3 &&
               world_raycast(&g->world, player_eye(p),
                             vnorm(vsub(vadd(position_absolute(g->shelter.door), v3(0, 1.4, 0)),
                                        player_eye(p))),
                             3) >=
                   vlen(vsub(vadd(position_absolute(g->shelter.door), v3(0, 1.4, 0)),
                             player_eye(p))) -
                       .1)
        return SHELTER_DOOR;
    return SHELTER_NONE;
}
bool game_shelter_interact(Game *g, int player) {
    if (player < 0 || player >= g->player_count)
        return false;
    Player *p = &g->players[player];
    ShelterUse use = game_shelter_near(g, player);
    if (!p->sheltered && use == SHELTER_NONE)
        return false;
    if (use == SHELTER_DOOR) {
        if (p->sheltered) {
            Vec3 outside = position_absolute(p->pos);
            if (!world_ready(&g->world, world_cell(outside)) ||
                world_overlap(&g->world, outside, .34, 1.0)) {
                game_message(g, "RETURN PASSAGE NOT CLEAR / WAIT FOR THE SHIP SURVEY");
                return true;
            }
            p->sheltered = false;
            p->yaw = p->return_yaw;
            p->pitch = p->return_pitch;
            p->hurt = 1;
            game_message(g, "RETURNED TO THE SHIP / THE SHELTER REMAINS");
        } else {
            p->return_yaw = p->yaw;
            p->return_pitch = p->pitch;
            p->sheltered = true;
            p->shelter_pos = v3(0, .02, -5.6);
            p->yaw = p->pitch = 0;
            p->hurt = p->slow = 0;
            game_message(g, "THE FOLD / SAFE FROM THE INFESTATION / MAKE YOURSELF AT HOME");
        }
        p->resting = 0;
        p->crouched =
            !p->sheltered && world_overlap(&g->world, position_absolute(p->pos), .34, 1.78);
        p->velocity = v3(0, 0, 0);
    } else if (use == SHELTER_BED) {
        p->resting = 6;
        game_message(g, "RESTING / SIX SECONDS TO RECOVER / MOVE TO WAKE");
    } else if (use == SHELTER_MED) {
        p->health = player_max_health(p);
        p->armor = player_max_armor(p);
        p->hurt = p->slow = 0;
        game_message(g, "MEDICAL STATION / WOUNDS TREATED / ARMOR REPAIRED");
    } else if (use == SHELTER_KITCHEN) {
        p->health = fminf(player_max_health(p), p->health + player_max_health(p) * .3f);
        p->nourished = 120;
        game_message(g, "STARGRAIN STEW COOKED / RECOVERY BOOST FOR 120 SECONDS");
    } else
        game_message(g, "SAFE IN THE FOLD / G NEAR THE BED, MED STATION, KITCHEN OR EXIT");
    g->dirty = true;
    return true;
}
void game_shelter_step(Game *g, Player *p, const Actions *a, double dt) {
    Vec3 wish = vadd(vmul(v3(sin(p->yaw), 0, cos(p->yaw)), a->forward),
                     vmul(v3(cos(p->yaw), 0, -sin(p->yaw)), a->strafe));
    if (vlen(wish) > 1)
        wish = vnorm(wish);
    if (vlen(wish) > .01 || a->jump || a->fire)
        p->resting = 0;
    if (p->resting > 0) {
        p->resting = fmaxf(0, p->resting - (float)dt);
        p->health = fminf(player_max_health(p), p->health + player_max_health(p) * dt / 5);
        p->shield = fminf(player_max_shield(p), p->shield + player_max_shield(p) * dt / 5);
        p->energy = fminf(player_max_energy(p), p->energy + player_max_energy(p) * dt / 5);
        g->dirty = true;
    }
    p->crouched = a->crouch;
    double speed = p->crouched ? 3 : 5;
    Vec3 step = vmul(wish, speed * dt), next = p->shelter_pos;
    next.x += step.x;
    if (shelter_clear(next))
        p->shelter_pos = next;
    next = p->shelter_pos;
    next.z += step.z;
    if (shelter_clear(next))
        p->shelter_pos = next;
}
