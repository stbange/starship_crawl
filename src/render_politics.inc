static void politics_scene(Renderer *r, const Game *g, int player) {
    if (!g->politics.version)
        return;
    Vec3 eye = player_eye(&g->players[player]);
    for (int i = 0; i < g->world.count; i++) {
        const Room *room = &g->world.rooms[i];
        if (!room->ready || vlen(vsub(world_center(room->cell), eye)) > 150)
            continue;
        unsigned owner = politics_owner(g, room->cell);
        if (owner) {
            Color color = politics_color(g->world.seed, owner - 1);
            Vec3 at = vadd(world_lift(room->cell), v3(3, 3.2, 0));
            block(r, at, v3(.9, 2, .05), color, .12f);
            for (unsigned mark = 0; mark < owner; mark++)
                block(r, vadd(at, v3(-.26 + mark * .25, .2, -.06)), v3(.10, .7, .08),
                      rgb(.8f, .77f, .5f), .5f);
            if (vlen(vsub(eye, at)) < 28)
                ship_sign(r, vadd(at, v3(0, 1.5, 0)), politics_name(g->world.seed, owner - 1),
                          color, eye);
        }
        if (room->habitat.court) {
            unsigned nation = room->habitat.country - 1;
            Color color = politics_color(g->world.seed, nation), metal = rgb(.10f, .12f, .15f);
            Vec3 at = encounter_console(room);
            const CountryState *state = &g->politics.countries[nation];
            ship_sign(r, vadd(at, v3(0, 4.1, 6)),
                      room->habitat.court == COURT_AUDIENCE  ? "ROYAL AUDIENCE"
                      : room->habitat.court == COURT_CONDUIT ? "DISPUTED CONDUIT"
                                                             : "ROYAL GUARD",
                      color, eye);
            if (room->habitat.court == COURT_AUDIENCE) {
                /* Ceremonial projections occupy the clear aisle without new collision. */
                block(r, vadd(at, v3(0, .05, 6)), v3(7, .05, 5), metal, .1f);
                Government gov = politics_government(g->world.seed, nation);
                for (int n = 0; n < 3; n++) {
                    Vec3 body = vadd(at, v3((n - 1) * 2.2, 2.4 + .15 * sin(g->time + n), 6));
                    if (gov == DIVIDED_CROWN) {
                        octahedron(r, body, 1.1, metal, .2f);
                        for (int j = 0; j < 3; j++)
                            block(r, vadd(body, v3((j - 1) * .4, 1.2, 0)), v3(.16, .8, .18), color,
                                  .8f);
                        block(r, vadd(body, v3(0, .2, -.8)), v3(.7, .06, .12), color, 1.2f);
                    } else if (gov == MOLTING_COURT) {
                        for (int j = 0; j < 5; j++) {
                            double angle = (j - 2) * .55;
                            tube(r, vadd(body, v3(sin(angle) * 1.1, cos(angle), .1)),
                                 vadd(body, v3(sin(angle) * .8, -.7, -.5)), .13, .04, color, .25f);
                        }
                        octahedron(r, body, .65, metal, .2f);
                    } else {
                        octahedron(r, body, .7, color, .7f);
                        for (int j = 0; j < 4; j++)
                            block(r, vadd(body, v3(0, (j - 2) * .5, 0)), v3(1.6, .04, 1.6), color,
                                  .4f);
                    }
                }
                if (state->stage == 3)
                    ship_sign(r, vadd(at, v3(0, 4.5, 5)),
                              state->pledge == 2   ? "SUCCESSOR RECOGNIZED"
                              : state->pledge == 1 ? "TREATY HONORED"
                                                   : "CONDUIT ANNEXED",
                              color, eye);
                octahedron(r, vadd(at, v3(0, 1.4, 0)), .25, color, 1.2f);
            }
        }
        for (int e = 0; e < room->enemy_count; e++)
            if (room->enemies[e].alive && room->enemies[e].country)
                block(r, vadd(enemy_center(&room->enemies[e]), v3(0, .4, 0)), v3(.7, .12, .7),
                      politics_color(g->world.seed, room->enemies[e].country - 1), .55f);
    }
    if (g->director.kind && vlen(vsub(director_point(&g->director), eye)) < 140) {
        Vec3 at = director_point(&g->director);
        Color cyan = rgb(.25f, .85f, .95f);
        block(r, vadd(at, v3(0, .35, 0)), v3(.9, .7, .8), rgb(.12f, .19f, .20f), .1f);
        octahedron(r, vadd(at, v3(0, 1.7 + .1 * sin(g->time * 2), 0)), .25, cyan, 1);
        ship_sign(r, vadd(at, v3(0, 2.5, 0)), director_name(g->director.kind), cyan, eye);
    }
}
static void politics_journal_draw(Renderer *r, const Game *g, int player, float w, float h) {
    float width = fminf(w - 20, 850), x = (w - width) * .5f, y = 12;
    float scale = h >= 650 ? (w >= 900 ? 1.6f : 1.25f) : 1.f;
    Color gold = rgb(.9f, .72f, .4f), ink = rgb(.8f, .85f, .83f), mint = rgb(.32f, .85f, .65f);
    rect(r, x, y, width, h - 24, rgb(.02f, .035f, .045f));
    frame(r, x, y, width, h - 24, gold);
    y = wrapped(r, x + 12, y + 12, scale, width - 24, "COURTS / KINSHIPS / CROWNS", gold) + 8;
    char text[256];
    for (int line = 0; line <= 10; line++) {
        politics_line(g, player, line, text, sizeof(text));
        y = wrapped(r, x + 12, y, scale, width - 24, text, line >= 6 && line <= 8 ? mint : ink) + 9;
    }
    wrapped(r, x + 12, y + 2, scale, width - 24,
            "L NEXT COURT / U TRACK / ENTER CLOSE / PAD X READ, Y TRACK, RB PAGE", gold);
}
static void politics_hud(Renderer *r, const Game *g, int player, float w, float h, float scale) {
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
        rect(r, (w - width) * .5f, h * .54f, width, 42 * s, rgb(.02f, .04f, .05f));
        wrapped(r, (w - width) * .5f + 6, h * .54f + 7, s, width - 12, hint, rgb(.9f, .75f, .45f));
    }
}
