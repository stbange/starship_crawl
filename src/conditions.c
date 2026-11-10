#include "game.h"
unsigned district_condition(uint32_t seed) {
    return (seed % 3 + 1) % 3 + 1;
}
bool district_compatible(unsigned family, unsigned condition) {
    static const unsigned masks[] = {0, 10, 6, 14, 12};
    return family >= 1 && family <= 4 && condition >= 1 && condition <= 3 &&
           (masks[family] & (1u << condition));
}
float condition_phase(const Game *g, const DistrictMark *mark) {
    return fmodf(g->condition_clock + (float)((mark->identity >> 8) % 3) * 4, 24);
}
bool condition_surface(const Room *r, Vec3 at) {
    Vec3 p = vsub(at, world_center(r->cell));
    return (r->journey.role & JOURNEY_VISTA) && fabs(p.x) > 4 && fabs(p.x) < 17 && p.z > 9 &&
           p.z < fmin(37, r->depth * .5 - 4) && p.y >= -.1 && p.y < .5;
}
bool condition_live(const Game *g, const Room *r) {
    if (g->world.generator < 16 || r->journey.condition < 2 ||
        (world_district_state(&g->world, r->journey.anchor) & DISTRICT_CONDUCTOR) ||
        (world_record_systems(&g->world, r->journey.anchor) & SYSTEM_SILENCED))
        return false;
    float phase = condition_phase(g, &r->journey);
    return r->journey.condition == 2 ? phase >= 3 && phase < 8
                                     : r->district_powered && phase >= 4 && phase < 12;
}
void condition_step(Game *g, double dt) {
    if (g->world.generator < 16)
        return;
    bool active = false;
    for (int p = 0; p < g->player_count; p++)
        active |= !g->players[p].sheltered && !g->matrix.divers[p].active &&
                  position_absolute(g->players[p].pos).z >= 0;
    if (!active)
        return;
    float step = (float)clampd(dt, 0, .25);
    g->condition_clock = fmodf(g->condition_clock + step, 24);
    for (int n = 0; n < g->world.count; n++) {
        Room *r = &g->world.rooms[n];
        if (!r->ready || !condition_live(g, r))
            continue;
        for (int p = 0; p < g->player_count; p++)
            if (!g->players[p].sheltered && !g->matrix.divers[p].active &&
                condition_surface(r, position_absolute(g->players[p].pos)))
                game_hurt(g, p, step * (r->journey.condition == 2 ? 12 : 10),
                          r->journey.condition == 2 ? CHILL : DRAIN);
        if (campaign_guardian(r->cell) >= 0)
            continue;
        for (int i = 0; i < r->enemy_count; i++) {
            Enemy *e = &r->enemies[i];
            if (!e->alive || e->force || !condition_surface(r, position_absolute(e->pos)))
                continue;
            if (r->journey.condition == 2)
                e->slow = fmaxf(e->slow, .8f);
            else if (sc_enemy_bodies[e->kind].form != ROBOT && e->burn <= 0)
                e->health =
                    fmaxf(e->health, fminf(e->health + step * 8,
                                           sc_enemies[e->kind].health +
                                               10 * (float)fmin(campaign_layer(r->cell), 8)));
        }
    }
}
