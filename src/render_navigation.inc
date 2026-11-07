/* Renderer-only signage: no new obstacles or changes to saved geometry. */
static void ship_sign(Renderer *r, Vec3 at, const char *label, Color color, Vec3 eye) {
    double pixel = .055, direction = eye.z >= at.z ? -1 : 1;
    size_t length = strlen(label);
    block(r, at, v3(length * pixel * 6 + .4, .65, .12), rgb(.025f, .045f, .05f), .1f);
    at = vsub(vadd(at, v3(-direction * length * pixel * 3, .2, -direction * .07)), r->origin);
    for (size_t i = 0; i < length; i++) {
        int code = glyph(label[i]);
        if (code < 0)
            continue;
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (glyphs[code][y] & (1 << (4 - x))) {
                    Vec3 a = vadd(at, v3(direction * (i * 6 + x) * pixel, -y * pixel, 0));
                    quad(&r->mesh, a, vadd(a, v3(direction * pixel, 0, 0)),
                         vadd(a, v3(direction * pixel, -pixel, 0)), vadd(a, v3(0, -pixel, 0)),
                         color, .85f);
                }
    }
}
static void navigation_scene(Renderer *r, const Game *g, int player) {
    const Player *p = &g->players[player];
    Vec3 eye = player_eye(p), feet = position_absolute(p->pos);
    Cell cell = world_cell(feet);
    Color cyan = rgb(.18f, .66f, .78f), amber = rgb(.8f, .46f, .14f);
    if (g->world.generator >= 9)
        for (int dy = -1; dy <= 1; dy++) {
            Cell c = cell;
            c.y += dy;
            if (!world_ready(&g->world, c))
                continue;
            Vec3 origin = world_center(c);
            if (vlen(vsub(vadd(origin, v3(0, 2, -7)), eye)) > 75)
                continue;
            char sign[40];
            snprintf(sign, sizeof(sign), "DECK %+d / EXIT", c.y);
            ship_sign(r, vadd(origin, v3(0, 3.2, -5.8)), sign, cyan, eye);
            /* Floor chevrons open into the room rather than continuing the shaft. */
            for (int i = 0; i < 3; i++) {
                Vec3 tip = vadd(origin, v3(0, .06, -3 + i * 2));
                tube(r, vadd(tip, v3(-.5, 0, -.6)), tip, .035, .035, cyan, .8f);
                tube(r, vadd(tip, v3(.5, 0, -.6)), tip, .035, .035, cyan, .8f);
            }
            if (world_service_column(g->world.seed, c)) {
                ship_sign(r, vadd(origin, v3(-8, 2.7, -5.8)), "RAMPS", cyan, eye);
                ship_sign(r, vadd(origin, v3(8, 2.7, -5.8)), "STAIRS", amber, eye);
                for (int k = 1; k < 8; k++) {
                    Vec3 at = vadd(
                        origin, v3(feet.x >= origin.x ? 8 : -8, k * 8 + 1.3, k % 2 ? -6.5 : -33.4));
                    if (vlen(vsub(at, eye)) < 23) {
                        snprintf(sign, sizeof(sign), "%d/8 / TURN", k);
                        ship_sign(r, at, sign, feet.x >= origin.x ? amber : cyan, eye);
                    }
                }
            }
        }
    const Navigation *nav = &p->navigation;
    if (nav->kind == NAV_DOOR && cell_equal(nav->from, cell)) {
        Vec3 at = vadd(world_center(cell),
                       v3((nav->next.x - cell.x) * 47, 1.5, (nav->next.z - cell.z) * 47));
        if (vlen(vsub(at, eye)) < 75) {
            Vec3 across = nav->next.x != cell.x ? v3(0, 0, 1) : v3(1, 0, 0);
            for (int side = -1; side <= 1; side += 2) {
                Vec3 edge = vadd(at, vmul(across, side * 1.6));
                tube(r, vadd(edge, v3(0, -.6, 0)), vadd(edge, v3(0, .6, 0)), .045, .045, cyan, 1);
            }
        }
    }
    /* One elevated silhouette in the occupied tall room, visible from its landing. */
    const Room *room = world_find((World *)&g->world, cell);
    if (!room || !room->ready || room->type == BAY || room->height < 9)
        return;
    Vec3 at = focal_anchor(room);
    Color color = focal_color(room);
    double radius = fmin(4, room->height * .08);
    if (room->district % 3 == 0) {
        for (int i = 0; i < 12; i++) {
            double a = i * SC_PI / 6, b = (i + 1) * SC_PI / 6;
            tube(r, vadd(at, v3(cos(a) * radius, 0, sin(a) * radius)),
                 vadd(at, v3(cos(b) * radius, 0, sin(b) * radius)), .08, .08, color, .65f);
        }
    } else if (room->district % 3 == 1) {
        for (int i = -1; i <= 1; i++)
            block(r, vadd(at, v3(i * radius * .65, abs(i) * radius * .4, 0)),
                  v3(.22, radius * 1.7, .22), color, .6f);
    } else
        octahedron(r, at, radius * .6, color, .5f);
    tube(r, vadd(at, v3(0, radius, 0)),
         v3(at.x, world_center(room->cell).y + room->height - .4, at.z), .055, .055,
         rgb(.15f, .19f, .19f), 0);
}
static void navigation_hud(Renderer *r, const Game *g, int player, float w, float h, float s,
                           bool touch) {
    const Player *p = &g->players[player];
    const Navigation *nav = &p->navigation;
    if (!nav->kind || p->sheltered || p->lift_active)
        return;
    char label[120], action[70];
    float left = w < 700 || touch ? 148 * s : 0;
    float available = (touch && w >= 700 ? w * .51f - 36 * s : w - 20 * s) - left;
    float center = left + available * .5f;
    float y = touch && w < 700 ? h * .1f + 32 * s : 78 * s;
    if (nav->kind == NAV_LIFT)
        snprintf(action, sizeof(action), "LIFT %s / DECK %+d",
                 nav->next.y > nav->from.y ? "UP" : "DOWN", nav->next.y);
    else if (nav->kind == NAV_LANDING)
        snprintf(action, sizeof(action), "FOLLOW LANDING / DECK %+d", nav->next.y);
    else
        snprintf(action, sizeof(action), "%s",
                 nav->kind == NAV_BOARD     ? "ENTER LOADING BAY"
                 : nav->kind == NAV_DOOR    ? "NEXT DOOR"
                 : nav->kind == NAV_BLOCKED ? "ROUTE SEALED / F10"
                                            : "MISSION SITE / G USE");
    Vec3 delta = vsub(nav->point, position_absolute(p->pos));
    Vec3 local = vsub(position_absolute(p->pos), world_center(nav->from));
    if (p->grounded && local.y > 2 && nav->kind != NAV_LANDING) {
        centered(r, center, y, fminf(.9f * s, available / 252),
                 "ON A LEDGE / BACKTRACK TO FLOOR / M MAP", rgb(.85f, .65f, .3f));
        return;
    }
    double angle = atan2(delta.x, delta.z) - p->yaw;
    angle = atan2(sin(angle), cos(angle));
    const char *turn = fabs(angle) > 2.4 ? "BEHIND"
                       : angle > .55     ? "RIGHT"
                       : angle < -.55    ? "LEFT"
                                         : "AHEAD";
    snprintf(label, sizeof(label), "%s / %s / %.0fM", turn, action, vlen(delta));
    float scale = fminf(.95f * s, available / (6 * strlen(label)));
    centered(r, center, y, scale, label, rgb(.35f, .8f, .84f));
}
