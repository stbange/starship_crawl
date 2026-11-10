#include "render_internal.h"

void sc_render_district_scene(Renderer *renderer, const Game *g, int player) {
    if (g->world.generator < 16)
        return;
    Vec3 eye = player_eye(&g->players[player]);
    for (unsigned i = 0; i < g->chronicle.count; i++) {
        const RecurringPerson *person = &g->chronicle.people[i];
        RecurringPerson trace = *person;
        if (person->fate == PERSON_TRAVELLING)
            trace.site = person->previous;
        Vec3 at = chronicle_position(&trace);
        if (!world_ready(&g->world, trace.site) || vlen(vsub(at, eye)) > 100)
            continue;
        Color color = person->role == 1 ? rgb(.8f, .55f, .2f) : rgb(.4f, .65f, .95f);
        if (person->fate == PERSON_WAITING || person->fate == PERSON_SETTLED) {
            sc_render_octahedron(renderer, vadd(at, v3(0, 1.25, 0)), .55, rgb(.12f, .2f, .24f),
                                 .1f);
            for (int limb = 0; limb < 3; limb++) {
                double a = limb * SC_PI * 2 / 3;
                sc_render_tube(renderer, vadd(at, v3(0, 1.1, 0)),
                               vadd(at, v3(sin(a) * .7, .05, cos(a) * .7)), .08, .03, color, .3f);
            }
            sc_render_octahedron(renderer, vadd(at, v3(0, 2, 0)), .22, color, .8f);
        } else
            sc_render_block(renderer, vadd(at, v3(0, .4, 0)), v3(.7, .8, .3), color, .15f);
        sc_render_world_label(renderer, vadd(at, v3(0, 2.7, 0)), chronicle_name(person), color, eye,
                              .012);
        sc_render_world_label(renderer, vadd(at, v3(0, 2.4, 0)),
                              person->fate == PERSON_TRAVELLING ? "SERVICE ROUTE / TRAVEL NOTE"
                              : person->fate >= PERSON_LOST     ? "RETAINED RECORDER"
                                                                : chronicle_role(person->role),
                              color, eye, .012);
    }
    for (int i = 0; i < g->world.count; i++) {
        const Room *r = &g->world.rooms[i];
        Vec3 center = world_center(r->cell);
        if (!r->ready || !r->journey.role || vlen(vsub(center, eye)) > 140 ||
            fabs(center.y - eye.y) > 50)
            continue;
        uint32_t state = world_district_state(&g->world, r->journey.anchor);
        Color gold = rgb(.9f, .55f, .15f), blue = rgb(.15f, .65f, .9f), green = rgb(.3f, .75f, .5f);
        if (r->journey.role & JOURNEY_VISTA) {
            sc_render_ship_sign(renderer, vadd(center, v3(-6, 5, 12)),
                                district_name(r->journey.family), blue, eye);
            for (int side = -1; side <= 1; side += 2) {
                bool live = side < 0 && r->district_arc > 0;
                Color water = live ? (r->district_arc > 5 ? gold : blue) : rgb(.06f, .17f, .2f);
                float glow = live ? .6f + .6f * (float)fabs(sin(g->time * 8)) : .1f;
                double width = r->district_powered ? 13 : 17, x = r->district_powered ? 10.5 : 8.5;
                double far = fmin(37, r->depth * .5 - 4), depth = far - 9;
                sc_render_block(renderer, vadd(center, v3(side * x, .035, 9 + depth * .5)),
                                v3(width, .015, depth), water, glow);
                for (int ripple = 0; ripple < 9; ripple++) {
                    double z = 9.5 + fmod(ripple * 3 + g->time * .35, depth - 1);
                    sc_render_block(renderer, vadd(center, v3(side * x, .05, z)),
                                    v3(width - .5, .008, .025),
                                    live ? water : rgb(.15f, .25f, .27f), live ? 1 : .15f);
                }
            }
            Vec3 source = district_control(r->cell, DISTRICT_SOURCE_CONTROL);
            sc_render_ship_sign(renderer, vadd(source, v3(0, 2.5, 0)),
                                state & DISTRICT_CONDUCTOR ? "SEVERED" : "LIVE CONDUCTOR", gold,
                                eye);
            sc_render_tube(renderer, vadd(source, v3(0, .8, 0)), vadd(center, v3(-8, .12, 22)),
                           .045, .045, state & DISTRICT_CONDUCTOR ? rgb(.15f, .2f, .2f) : gold,
                           .5f);
            if (!(r->journey.role & JOURNEY_COMPACT))
                for (unsigned side = 0; side < 2; side++) {
                    Vec3 at = district_control(r->cell, side ? DISTRICT_RETURN_CONTROL : 0);
                    Color color = state & DISTRICT_SHORTCUT ? green : gold;
                    for (int sign = -1; sign <= 1; sign += 2)
                        sc_render_block(renderer, vadd(at, v3(sign * 1.3, 1.6, 0)),
                                        v3(.18, 3.2, .18), color, .8f);
                    sc_render_block(renderer, vadd(at, v3(0, 3.1, 0)), v3(2.7, .18, .18), color,
                                    .8f);
                    sc_render_ship_sign(renderer, vadd(at, v3(0, 3.7, 0)),
                                        state & DISTRICT_SHORTCUT ? "RETURN CONDUIT / OPEN"
                                                                  : "RETURN CONDUIT / SEALED",
                                        color, eye);
                }
        }
        for (unsigned role = JOURNEY_PUMP; role <= JOURNEY_REFUGE; role <<= 1)
            if (r->journey.role & role) {
                Vec3 at = district_control(r->cell, role);
                sc_render_world_label(renderer, vadd(at, v3(0, 2.1, 0)),
                                      role == JOURNEY_PUMP ? "PUMP / POWER WAKES MACHINES"
                                                           : "WORKERS' REFUGE",
                                      blue, eye, .018);
                sc_render_octahedron(renderer, vadd(at, v3(0, 1.5, 0)), .18,
                                     r->district_warning > 0 ? gold
                                     : r->district_powered   ? green
                                                             : blue,
                                     1);
            }
    }
}
void sc_render_district_hud(Renderer *renderer, const Game *g, int player, float w, float h,
                            float s) {
    const Player *p = &g->players[player];
    if (p->sheltered || g->world.generator < 16)
        return;
    const Room *room = world_find((World *)&g->world, world_cell(position_absolute(p->pos)));
    if (!room || !room->journey.family)
        return;
    char text[160];
    snprintf(text, sizeof(text), "%s / %s", district_name(room->journey.family),
             room->district_warning > 0 ? "MACHINES WAKING / AMBER WARNING"
             : room->district_powered   ? "PUMPS ONLINE / DRY CENTER ROUTE"
                                        : "DORMANT MACHINES / OPTIONAL PUMP");
    sc_render_centered(renderer, w * .5f, 110 * s, .65f * s, text, rgb(.3f, .7f, .8f));
    for (unsigned role = JOURNEY_PUMP; role <= JOURNEY_REFUGE; role <<= 1)
        if ((room->journey.role & role) &&
            vlen(vsub(position_absolute(p->pos), district_control(room->cell, role))) < 3)
            sc_render_centered(renderer, w * .5f, h * .67f, .8f * s,
                               role == JOURNEY_PUMP ? "G / RESTORE PUMP / MACHINES WILL WAKE"
                                                    : "G / READ REFUGE NOTICE",
                               rgb(.3f, .8f, 1));
    if (room->district_arc > 0 && district_wet(room, position_absolute(p->pos)))
        sc_render_centered(renderer, w * .5f, h * .7f, s,
                           room->district_arc > 5 ? "WATER WARNING / MOVE TO STAIRS OR DRY CENTER"
                                                  : "LIVE WATER / REACH DRY GROUND",
                           rgb(1, .65f, .2f));
    int person = chronicle_near(g, player);
    if (person >= 0) {
        snprintf(text, sizeof(text), "G / %s / OPTIONAL REFUGE RECORD",
                 chronicle_name(&g->chronicle.people[person]));
        sc_render_centered(renderer, w * .5f, h * .73f, .8f * s, text, rgb(.9f, .75f, .4f));
    }
}
