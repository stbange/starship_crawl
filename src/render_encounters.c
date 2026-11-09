#include "render_internal.h"

void sc_render_encounter_scene(Renderer *r, const Game *g, int player) {
    if (g->world.generator < 11)
        return;
    Vec3 eye = player_eye(&g->players[player]);
    for (int i = 0; i < g->world.count; i++) {
        const Room *room = &g->world.rooms[i];
        if (!room->ready || vlen(vsub(world_center(room->cell), eye)) > 100)
            continue;
        uint32_t state = world_local_state(&g->world, room);
        Color mint = rgb(.2f, .85f, .57f), amber = rgb(.95f, .52f, .17f);
        if (room->habitat.reveal) {
            Vec3 at = frontier_focus(room);
            if (vlen(vsub(eye, world_lift(room->cell))) < 18)
                sc_render_ship_sign(r, vadd(at, v3(0, -3, -6)),
                                    frontier_reveal(room->habitat.reveal), rgb(.4f, .56f, .58f),
                                    eye);
        }
        if (room->habitat.pace == PACE_FRONTIER) {
            Vec3 at = encounter_console(room);
            Color color = state & FRONTIER_CLEARED ? mint : rgb(.8f, .85f, .83f);
            sc_render_block(r, vadd(at, v3(0, .6, 0)), v3(.8, 1.2, .8), rgb(.12f, .15f, .16f), 0);
            sc_render_octahedron(r, vadd(at, v3(0, 1.55, 0)), .25, color, 1.5f);
            if (vlen(vsub(at, eye)) > 7)
                sc_render_ship_sign(r, vadd(at, v3(0, 2.4, 0)),
                                    state & FRONTIER_CLEARED ? "REWARD" : "OPTIONAL", color, eye);
            for (int e = 1; e < 4; e++)
                if (encounter_linked(&g->world, room, e) ||
                    (frontier_ward(g, room, player) && room->enemies[e].alive)) {
                    Vec3 from = enemy_center(&room->enemies[0]),
                         to = enemy_center(&room->enemies[e]);
                    Color link = room->frontier_attacker == e ? amber : rgb(.26f, .43f, .65f);
                    sc_render_tube(r, from, to, .025, .025, link, .8f);
                    sc_render_octahedron(r, to, .13, link, 1);
                }
            if (room->habitat.challenge == CH_CIRCUIT)
                for (unsigned pylon = 0; pylon < 2; pylon++) {
                    Vec3 at = frontier_pylon(room, pylon);
                    Color color = room->circuit & (1u << pylon) ? mint : rgb(.3f, .7f, 1);
                    sc_render_block(r, vadd(at, v3(0, 1.2, 0)), v3(.35, 2.4, .35), color, 1);
                    sc_render_ship_sign(r, vadd(at, v3(0, 3, 0)), pylon ? "CIRCUIT B" : "CIRCUIT A",
                                        color, eye);
                }
            if (room->frontier_active && room->habitat.contract && room->hazard_time < 2.6f) {
                Vec3 at = frontier_hazard(room);
                Color color = room->habitat.contract == RELIC_COLD + 1    ? rgb(.3f, .7f, 1)
                              : room->habitat.contract == RELIC_ALIEN + 1 ? rgb(.7f, .3f, 1)
                                                                          : amber;
                bool live = frontier_hazard_live(room);
                for (int n = 0; n < 16; n++) {
                    double a = n * SC_PI / 8, b = (n + 1) * SC_PI / 8;
                    sc_render_tube(r, vadd(at, v3(cos(a) * 3.5, 0, sin(a) * 3.5)),
                                   vadd(at, v3(cos(b) * 3.5, 0, sin(b) * 3.5)), .045, .045, color,
                                   live ? 2 : .8f);
                    if (live && n % 2 == 0)
                        sc_render_tube(r, vadd(at, v3(cos(a) * 2.5, 0, sin(a) * 2.5)),
                                       vadd(at, v3(cos(a) * 2.5, 1.7, sin(a) * 2.5)), .08, .01,
                                       color, 1);
                }
            }
            if (room->frontier_disrupt > 0 && room->enemies[0].alive)
                sc_render_octahedron(r, enemy_center(&room->enemies[0]), .38, mint, 1.6f);
        } else if (local_available(&g->world, room) || (state & LOCAL_RESOLVED)) {
            Vec3 at = local_console(room);
            Color color = state & LOCAL_RESOLVED ? mint : amber;
            sc_render_block(r, vadd(at, v3(0, .6, .4)), v3(.6, 1.2, .5), rgb(.18f, .22f, .21f), 0);
            sc_render_block(r, vadd(at, v3(0, 1.25, .4)), v3(.65, .1, .55), color, 1.3f);
            if (vlen(vsub(at, eye)) > 7)
                sc_render_ship_sign(r, vadd(at, v3(0, 2, .5)),
                                    state & LOCAL_RESOLVED ? "RESTORED" : "LOCAL", color, eye);
            if (state & LOCAL_RESOLVED)
                for (int n = 0; n < 6; n++)
                    sc_render_block(r, vadd(at, v3((n - 2.5) * .9, .05, -1.5)), v3(.4, .035, .2),
                                    mint, .7f);
        }
        for (int j = 0; j < room->enemy_count; j++) {
            const Enemy *e = &room->enemies[j];
            if (!e->alive || e->motion != MOTION_WARN || e->behavior == ALIEN_CONDUCTOR)
                continue;
            Vec3 from = enemy_center(e);
            bool ranged = e->behavior == ALIEN_GAZER || e->behavior == ALIEN_REFLECTION ||
                          e->behavior == ALIEN_PRESSURE;
            Vec3 direction = ranged ? vnorm(vsub(e->aim, from)) : e->aim;
            double length = world_raycast(&g->world, from, direction,
                                          ranged                      ? vlen(vsub(e->aim, from))
                                          : e->behavior == ALIEN_SEAM ? fmax(7, e->climb_time)
                                                                      : 7);
            Vec3 end = vadd(from, vmul(direction, length));
            sc_render_tube(r, from, end, .018, .018, amber, .9f);
            if (!ranged) {
                end.y = room->cell.y * SC_DECK + .06;
                for (int n = 0; n < 8; n++) {
                    double a = n * SC_PI / 4, b = (n + 1) * SC_PI / 4;
                    sc_render_tube(r, vadd(end, v3(cos(a), 0, sin(a))),
                                   vadd(end, v3(cos(b), 0, sin(b))), .035, .035, amber, 1);
                }
            }
        }
    }
}
static float encounter_label(Renderer *r, float w, float y, float s, const char *text,
                             Color color) {
    if (w < 620)
        return sc_render_wrapped(r, 12, y, fmaxf(1, s), w - 24, text, color) + 3;
    sc_render_centered(r, w * .5f, y, s, text, color);
    return y + 16 * s;
}
void sc_render_encounter_hud(Renderer *r, const Game *g, int player, float w, float h, float s,
                             bool touch) {
    const Player *p = &g->players[player];
    if (p->sheltered || p->lift_active || g->matrix.divers[player].active)
        return;
    Vec3 feet = position_absolute(p->pos);
    const Room *room = world_find((World *)&g->world, world_cell(feet));
    char label[160];
    Color color = rgb(.78f, .85f, .72f);
    float width = w - 24 * s;
    const RewardDrill *drill = &g->drills[player];
    if (PLAYER_VIEW(g, p).guide_mode != GUIDE_OFF && !PLAYER_VIEW(g, p).lesson &&
        !onboarding_danger(g, player) && drill->time > 0 &&
        vlen(vsub(feet, world_center(drill->site))) < 35) {
        const char *hint =
            drill->complete ? "FIELD DRILL COMPLETE" : reward_drill_hint(drill->reward);
        if (touch && !drill->complete) {
            if (drill->reward.kind == REWARD_POWER)
                hint = "CAST / TRY YOUR EARNED POWER / POWER SWITCHES";
            else if (drill->reward.kind == REWARD_RIG)
                hint = "MENU / DEPLOY YOUR RIG / USE TO ENTER";
        }
        sc_render_centered(r, w * .5f, h * .38f, fminf(.9f * s, width / (6 * strlen(hint))), hint,
                           color);
    }
    if (!room || !room->ready || g->world.generator < 11)
        return;
    uint32_t state = world_local_state(&g->world, room);
    float tactic_y = h * (touch && w < 620 ? .34f : .25f);
    if (room->frontier_active && vlen(vsub(feet, encounter_console(room))) >= 4) {
        const char *hint = frontier_hint(room->habitat.challenge);
        tactic_y = encounter_label(r, w, tactic_y, fminf(.75f * s, width / (6 * strlen(hint))),
                                   hint, color);
    }
    if (room->frontier_active && room->habitat.challenge == CH_CIRCUIT) {
        snprintf(label, sizeof(label), "CIRCUIT %u/2 / %.0f SEC",
                 (room->circuit & 1) + ((room->circuit >> 1) & 1), room->circuit_time);
        encounter_label(r, w, fmaxf(tactic_y, h * .29f), .8f * s, label, color);
    }
    if (room->habitat.pace == PACE_FRONTIER && vlen(vsub(feet, encounter_console(room))) < 4) {
        bool claimed = (state & FRONTIER_CLEARED) &&
                       (!(state & world_eligible_bit(player)) || (state & world_claim_bit(player)));
        const char *action = claimed                    ? "COMPLETE"
                             : state & FRONTIER_CLEARED ? (touch ? "USE / CLAIM" : "G / CLAIM")
                             : room->frontier_active    ? "CHALLENGE ACTIVE"
                                                        : (touch ? "USE / AWAKEN" : "G / AWAKEN");
        snprintf(label, sizeof(label), "%s / %s", encounter_name(room), action);
        float y = h * .59f;
        if (w < 620 && h > 500)
            sc_render_rect(r, 8, y - 5, w - 16, 120, rgb(.025f, .035f, .04f));
        y = encounter_label(r, w, y, fminf(s, width / (6 * strlen(label))), label, color);
        if (!claimed) {
            encounter_reward_name(g, player, room, label, sizeof(label));
            y = encounter_label(r, w, y, fminf(.8f * s, width / (6 * strlen(label))), label, color);
        }
        const char *hint = frontier_hint(room->habitat.challenge);
        y = encounter_label(r, w, y, fminf(.75f * s, width / (6 * strlen(hint))), hint,
                            rgb(.5f, .65f, .68f));
        if (room->habitat.contract) {
            frontier_offer(g, player, room, label, sizeof(label));
            y = encounter_label(r, w, y, fminf(.75f * s, width / (6 * strlen(label))), label,
                                color);
            hint = frontier_risk(room->habitat.contract);
            encounter_label(r, w, y, fminf(.7f * s, width / (6 * strlen(hint))), hint,
                            rgb(.9f, .6f, .35f));
        }
    } else if (local_available(&g->world, room) && vlen(vsub(feet, local_console(room))) < 5) {
        snprintf(label, sizeof(label), "%s / %s", touch ? "USE" : "G",
                 local_action(room->habitat.tale));
        sc_render_centered(r, w * .5f, h * .59f, fminf(s, width / (6 * strlen(label))), label,
                           color);
    }
}
