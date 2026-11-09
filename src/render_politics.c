#include "render_internal.h"

void sc_render_politics_scene(Renderer *r, const Game *g, int player) {
    if (!g->politics.version && !g->factions.version)
        return;
    Vec3 eye = player_eye(&g->players[player]);
    for (int i = 0; i < g->world.count; i++) {
        const Room *room = &g->world.rooms[i];
        if (!room->ready || vlen(vsub(world_center(room->cell), eye)) > 150)
            continue;
        unsigned owner = politics_owner(g, room->cell);
        const FactionRegion *region =
            g->factions.version ? factions_at(&g->factions, room->cell) : NULL;
        if (owner) {
            Color color = politics_color(g->world.seed, owner - 1);
            Vec3 at = vadd(world_lift(room->cell), v3(3, 3.2, 0));
            sc_render_block(r, at, v3(.9, 2, .05), color, .12f);
            for (unsigned mark = 0; mark < owner; mark++)
                sc_render_block(r, vadd(at, v3(-.26 + mark * .25, .2, -.06)), v3(.10, .7, .08),
                                rgb(.8f, .77f, .5f), .5f);
            if (g->politics.version && vlen(vsub(eye, at)) < 28)
                sc_render_ship_sign(r, vadd(at, v3(0, 1.5, 0)),
                                    politics_name(g->world.seed, owner - 1), color, eye);
        }
        if (room->habitat.court) {
            unsigned nation = room->habitat.country - 1;
            Color color = politics_color(g->world.seed, nation), metal = rgb(.10f, .12f, .15f);
            Vec3 at = encounter_console(room);
            const CountryState *state = &g->politics.countries[nation];
            if (g->factions.version && room->habitat.court >= SITE_POWER) {
                Vec3 machine = vadd(at, v3(7, 0, 7));
                Color glow = room->habitat.court == SITE_BIOMASS ? rgb(.18f, .55f, .3f)
                             : room->habitat.court == SITE_POWER ? rgb(.2f, .4f, .8f)
                                                                 : rgb(.8f, .3f, .12f);
                float power = region && (region->flags & FS_DARK) ? .12f : .7f;
                for (int n = 0; n < 3; n++) {
                    Vec3 base = vadd(machine, v3(n * 2, 0, 0));
                    sc_render_block(r, vadd(base, v3(0, .35, 0)), v3(1.8, .7, 2.2), metal, .04f);
                    if (room->habitat.court == SITE_BIOMASS) {
                        sc_render_octahedron(r, vadd(base, v3(0, 1.6, 0)), .9, glow, power);
                        sc_render_tube(r, vadd(base, v3(0, .7, 0)), vadd(base, v3(.3, 2.6, 0)), .16,
                                       .07, color, .1f);
                    } else {
                        sc_render_block(r, vadd(base, v3(-.55, 1.6, 0)), v3(.15, 2.5, .7), metal,
                                        .08f);
                        sc_render_block(r, vadd(base, v3(.55, 1.6, 0)), v3(.15, 2.5, .7), metal,
                                        .08f);
                        for (int j = 0; j < 4; j++)
                            sc_render_block(r, vadd(base, v3(0, 1 + j * .4, 0)), v3(1, .08, .6),
                                            glow, power);
                    }
                }
            }
            if (g->politics.version)
                sc_render_ship_sign(r, vadd(at, v3(0, 4.1, 6)),
                                    room->habitat.court == COURT_AUDIENCE  ? "ROYAL AUDIENCE"
                                    : room->habitat.court == COURT_CONDUIT ? "DISPUTED CONDUIT"
                                                                           : "ROYAL GUARD",
                                    color, eye);
            if (room->habitat.court == COURT_AUDIENCE) {
                /* Ceremonial projections occupy the clear aisle without new collision. */
                sc_render_block(r, vadd(at, v3(0, .05, 6)), v3(7, .05, 5), metal, .1f);
                Government gov = politics_government(g->world.seed, nation);
                for (int n = 0; n < 3; n++) {
                    Vec3 body = vadd(at, v3((n - 1) * 2.2, 2.4 + .15 * sin(g->time + n), 6));
                    if (gov == DIVIDED_CROWN) {
                        sc_render_octahedron(r, body, 1.1, metal, .2f);
                        for (int j = 0; j < 3; j++)
                            sc_render_block(r, vadd(body, v3((j - 1) * .4, 1.2, 0)),
                                            v3(.16, .8, .18), color, .8f);
                        sc_render_block(r, vadd(body, v3(0, .2, -.8)), v3(.7, .06, .12), color,
                                        1.2f);
                    } else if (gov == MOLTING_COURT) {
                        for (int j = 0; j < 5; j++) {
                            double angle = (j - 2) * .55;
                            sc_render_tube(r, vadd(body, v3(sin(angle) * 1.1, cos(angle), .1)),
                                           vadd(body, v3(sin(angle) * .8, -.7, -.5)), .13, .04,
                                           color, .25f);
                        }
                        sc_render_octahedron(r, body, .65, metal, .2f);
                    } else {
                        sc_render_octahedron(r, body, .7, color, .7f);
                        for (int j = 0; j < 4; j++)
                            sc_render_block(r, vadd(body, v3(0, (j - 2) * .5, 0)),
                                            v3(1.6, .04, 1.6), color, .4f);
                    }
                }
                if (state->stage == 3)
                    sc_render_ship_sign(r, vadd(at, v3(0, 4.5, 5)),
                                        state->pledge == 2   ? "SUCCESSOR RECOGNIZED"
                                        : state->pledge == 1 ? "TREATY HONORED"
                                                             : "CONDUIT ANNEXED",
                                        color, eye);
                sc_render_octahedron(r, vadd(at, v3(0, 1.4, 0)), .25, color, 1.2f);
            }
        }
        if (region && cell_equal(region->site, room->cell) &&
            (region->flags & (FS_CAPTURED | FS_RAIDED))) {
            Vec3 at = vadd(encounter_console(room), v3(8, 0, -7));
            Color wreck = rgb(.15f, .12f, .1f), shell = rgb(.26f, .25f, .2f);
            unsigned count = 3 + (region->casualties % 4);
            for (unsigned n = 0; n < count; n++) {
                Vec3 p = vadd(at, v3((n % 3) * 1.2, .13, (n / 3) * 1.7));
                sc_render_block(r, p, v3(.8, .2, 1.2), wreck, .04f);
                sc_render_octahedron(r, vadd(p, v3(.3, .15, .4)), .3, shell, .08f);
            }
            /* Broken emplacements stay beside the protected crossing and lift aisle. */
            for (int n = 0; n < 3; n++)
                sc_render_block(r, vadd(at, v3(n * 2.0, .6, 3)), v3(1.6, 1.2, .35), wreck, .04f);
            if (region->flags & FS_GROWTH)
                for (int n = 0; n < 5; n++)
                    sc_render_tube(r, vadd(at, v3(n, 0, 1)),
                                   vadd(at, v3(n + .5, 1.6 + .3 * sin(g->time + n), 1.4)), .13,
                                   .025, rgb(.16f, .3f, .23f), .15f);
        }
        if (region && region->edit == EDIT_STASH && !region->edit_claims &&
            cell_equal(
                world_describe_version(g->world.seed, g->world.generator, region->edit_site).cell,
                room->cell)) {
            Vec3 kit = vadd(world_center(region->edit_site), v3(0, .3, 3));
            sc_render_block(r, kit, v3(1, .6, .7), rgb(.17f, .25f, .2f), .1f);
            sc_render_block(r, vadd(kit, v3(0, .32, 0)), v3(.5, .05, .15), rgb(.4f, .8f, .55f),
                            .8f);
        }
        for (int e = 0; e < room->enemy_count; e++)
            if (room->enemies[e].alive && room->enemies[e].country) {
                sc_render_block(r, vadd(enemy_center(&room->enemies[e]), v3(0, .4, 0)),
                                v3(.7, .12, .7),
                                politics_color(g->world.seed, room->enemies[e].country - 1), .55f);
                const Enemy *enemy = &room->enemies[e];
                if (g->factions.version) {
                    unsigned gov = politics_government(g->world.seed, enemy->country - 1);
                    Vec3 at = enemy_center(enemy);
                    Color ink = politics_color(g->world.seed, enemy->country - 1);
                    if (gov == DIVIDED_CROWN)
                        for (int n = 0; n < 3; n++)
                            sc_render_block(r, vadd(at, v3((n - 1) * .2, .7, 0)), v3(.08, .3, .08),
                                            ink, .7f);
                    else if (gov == MOLTING_COURT)
                        sc_render_tube(r, vadd(at, v3(-.5, .4, 0)), vadd(at, v3(.5, .9, 0)), .10,
                                       .03, ink, .5f);
                    else
                        sc_render_octahedron(r, vadd(at, v3(0, .75, 0)), .22, ink, .8f);
                    bool warning = false;
                    for (int p = 0; p < g->player_count; p++) {
                        const FactionIncident *incident =
                            &g->factions.incidents[p][enemy->country - 1];
                        warning |=
                            incident->strikes == 1 && incident->grace > 0 &&
                            cell_equal(world_region(incident->site),
                                       world_region(world_cell(position_absolute(enemy->pos))));
                    }
                    if (warning)
                        for (int n = 0; n < 2; n++)
                            sc_render_block(r, vadd(at, v3(0, 1.05 + n * .2, 0)), v3(.8, .07, .8),
                                            rgb(.9f, .7f, .3f), 1.1f);
                    if (enemy->force && !enemy->member) {
                        const FactionForce *force = factions_force((Game *)g, enemy->force);
                        if (force && force->cargo)
                            sc_render_block(r, vadd(at, v3(0, -.6, -.5)), v3(.9, .7, .5),
                                            rgb(.3f, .25f, .17f), .15f);
                        if (force && force->order == ORDER_RETREAT)
                            sc_render_tube(r, vadd(at, v3(-.45, 0, 0)), vadd(at, v3(.45, .45, 0)),
                                           .07, .07, ink, .7f);
                    }
                }
            }
    }
    if (g->director.kind && vlen(vsub(director_point(&g->director), eye)) < 140) {
        Vec3 at = director_point(&g->director);
        Color cyan = rgb(.25f, .85f, .95f);
        sc_render_block(r, vadd(at, v3(0, .35, 0)), v3(.9, .7, .8), rgb(.12f, .19f, .20f), .1f);
        sc_render_octahedron(r, vadd(at, v3(0, 1.7 + .1 * sin(g->time * 2), 0)), .25, cyan, 1);
        sc_render_ship_sign(r, vadd(at, v3(0, 2.5, 0)), director_name(g->director.kind), cyan, eye);
    }
}
void sc_render_politics_journal_draw(Renderer *r, const Game *g, int player, float w, float h) {
    float width = fminf(w - 20, 850), x = (w - width) * .5f, y = 12;
    float scale = h >= 650 ? (w >= 900 ? 1.6f : 1.25f) : 1.f;
    Color gold = rgb(.9f, .72f, .4f), ink = rgb(.8f, .85f, .83f), mint = rgb(.32f, .85f, .65f);
    sc_render_rect(r, x, y, width, h - 24, rgb(.02f, .035f, .045f));
    sc_render_frame(r, x, y, width, h - 24, gold);
    y = sc_render_wrapped(r, x + 12, y + 12, scale, width - 24, "COURTS / KINSHIPS / CROWNS",
                          gold) +
        8;
    char text[256];
    for (int line = 0; line <= 10; line++) {
        politics_line(g, player, line, text, sizeof(text));
        y = sc_render_wrapped(r, x + 12, y, scale, width - 24, text,
                              line >= 6 && line <= 8 ? mint : ink) +
            9;
    }
    sc_render_wrapped(r, x + 12, y + 2, scale, width - 24,
                      "L NEXT COURT / U TRACK / ENTER CLOSE / PAD X READ, Y TRACK, RB PAGE", gold);
}
void sc_render_politics_hud(Renderer *r, const Game *g, int player, float w, float h, float scale) {
    const Player *p = &g->players[player];
    if (!g->politics.version || p->sheltered || g->matrix.divers[player].active ||
        position_absolute(p->pos).z < 0)
        return;
    Vec3 feet = position_absolute(p->pos);
    Room *room = world_find((World *)&g->world, world_cell(feet));
    char text[160];
    const char *hint = NULL;
    if (room && room->habitat.court && vlen(vsub(feet, encounter_console(room))) < 4)
        hint = room->habitat.court == COURT_AUDIENCE ? "G / REQUEST ROYAL AUDIENCE"
                                                     : "G / POLITICAL COMMISSION SEAL";
    if (g->director.kind && vlen(vsub(feet, director_point(&g->director))) < 4) {
        snprintf(text, sizeof(text), "G / %s / %u SECONDS", director_name(g->director.kind),
                 g->director.remaining / 1000);
        hint = text;
    }
    if (hint) {
        float s = fmaxf(1, scale), width = fminf(w - 32, 560 * s);
        sc_render_rect(r, (w - width) * .5f, h * .54f, width, 42 * s, rgb(.02f, .04f, .05f));
        sc_render_wrapped(r, (w - width) * .5f + 6, h * .54f + 7, s, width - 12, hint,
                          rgb(.9f, .75f, .45f));
    }
}
