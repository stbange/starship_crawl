static void encounter_scene(Renderer *r, const Game *g, int player) {
    if (g->world.generator < 11)
        return;
    Vec3 eye = player_eye(&g->players[player]);
    for (int i = 0; i < g->world.count; i++) {
        const Room *room = &g->world.rooms[i];
        if (!room->ready || vlen(vsub(world_center(room->cell), eye)) > 100)
            continue;
        uint32_t state = world_local_state(&g->world, room);
        Color mint = rgb(.2f, .85f, .57f), amber = rgb(.95f, .52f, .17f);
        if (room->habitat.pace == PACE_FRONTIER) {
            Vec3 at = encounter_console(room);
            Color color = state & FRONTIER_CLEARED ? mint : rgb(.8f, .85f, .83f);
            block(r, vadd(at, v3(0, .6, 0)), v3(.8, 1.2, .8), rgb(.12f, .15f, .16f), 0);
            octahedron(r, vadd(at, v3(0, 1.55, 0)), .25, color, 1.5f);
            if (vlen(vsub(at, eye)) > 7)
                ship_sign(r, vadd(at, v3(0, 2.4, 0)),
                          state & FRONTIER_CLEARED ? "REWARD" : "OPTIONAL", color, eye);
            for (int e = 1; e < 4; e++)
                if (encounter_linked(&g->world, room, e)) {
                    Vec3 from = enemy_center(&room->enemies[0]),
                         to = enemy_center(&room->enemies[e]);
                    Color link = room->frontier_attacker == e ? amber : rgb(.26f, .43f, .65f);
                    tube(r, from, to, .025, .025, link, .8f);
                    octahedron(r, to, .13, link, 1);
                }
            if (room->frontier_disrupt > 0 && room->enemies[0].alive)
                octahedron(r, enemy_center(&room->enemies[0]), .38, mint, 1.6f);
        } else if (local_available(&g->world, room) || (state & LOCAL_RESOLVED)) {
            Vec3 at = local_console(room);
            Color color = state & LOCAL_RESOLVED ? mint : amber;
            block(r, vadd(at, v3(0, .6, .4)), v3(.6, 1.2, .5), rgb(.18f, .22f, .21f), 0);
            block(r, vadd(at, v3(0, 1.25, .4)), v3(.65, .1, .55), color, 1.3f);
            if (vlen(vsub(at, eye)) > 7)
                ship_sign(r, vadd(at, v3(0, 2, .5)), state & LOCAL_RESOLVED ? "RESTORED" : "LOCAL",
                          color, eye);
            if (state & LOCAL_RESOLVED)
                for (int n = 0; n < 6; n++)
                    block(r, vadd(at, v3((n - 2.5) * .9, .05, -1.5)), v3(.4, .035, .2), mint, .7f);
        }
        for (int j = 0; j < room->enemy_count; j++) {
            const Enemy *e = &room->enemies[j];
            if (!e->alive || e->motion != MOTION_WARN || e->behavior == ALIEN_CONDUCTOR)
                continue;
            Vec3 from = enemy_center(e);
            Vec3 direction = e->behavior == ALIEN_GAZER ? vnorm(vsub(e->aim, from)) : e->aim;
            double length =
                world_raycast(&g->world, from, direction,
                              e->behavior == ALIEN_GAZER ? vlen(vsub(e->aim, from)) : 7);
            Vec3 end = vadd(from, vmul(direction, length));
            tube(r, from, end, .018, .018, amber, .9f);
            if (e->behavior != ALIEN_GAZER) {
                end.y = room->cell.y * SC_DECK + .06;
                for (int n = 0; n < 8; n++) {
                    double a = n * SC_PI / 4, b = (n + 1) * SC_PI / 4;
                    tube(r, vadd(end, v3(cos(a), 0, sin(a))), vadd(end, v3(cos(b), 0, sin(b))),
                         .035, .035, amber, 1);
                }
            }
        }
    }
}
static void encounter_hud(Renderer *r, const Game *g, int player, float w, float h, float s,
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
    if (drill->time > 0 && vlen(vsub(feet, world_center(drill->site))) < 35) {
        const char *hint =
            drill->complete ? "FIELD DRILL COMPLETE" : reward_drill_hint(drill->reward);
        if (touch && !drill->complete) {
            if (drill->reward.kind == REWARD_POWER)
                hint = "CAST / TRY YOUR EARNED POWER / POWER SWITCHES";
            else if (drill->reward.kind == REWARD_RIG)
                hint = "MENU / DEPLOY YOUR RIG / USE TO ENTER";
        }
        centered(r, w * .5f, h * .38f, fminf(.9f * s, width / (6 * strlen(hint))), hint, color);
    }
    if (!room || !room->ready || g->world.generator < 11)
        return;
    uint32_t state = world_local_state(&g->world, room);
    if (room->habitat.pace == PACE_FRONTIER && vlen(vsub(feet, encounter_console(room))) < 4) {
        bool claimed = (state & FRONTIER_CLEARED) && (!(state & (FRONTIER_ELIGIBLE << player)) ||
                                                      (state & (FRONTIER_CLAIMED << player)));
        const char *action = claimed                    ? "COMPLETE"
                             : state & FRONTIER_CLEARED ? (touch ? "USE / CLAIM" : "G / CLAIM")
                             : room->frontier_active    ? "BREAK THE LINKS"
                                                        : (touch ? "USE / AWAKEN" : "G / AWAKEN");
        snprintf(label, sizeof(label), "%s / %s", encounter_name(room), action);
        centered(r, w * .5f, h * .59f, fminf(s, width / (6 * strlen(label))), label, color);
        if (!claimed) {
            encounter_reward_name(g, player, room, label, sizeof(label));
            centered(r, w * .5f, h * .59f + 18 * s, fminf(.8f * s, width / (6 * strlen(label))),
                     label, color);
        }
        centered(r, w * .5f, h * .59f + 33 * s, fminf(.75f * s, width / 270),
                 "OPTIONAL / ALL WEAPONS WORK / BEAMS BREAK LINKS", rgb(.5f, .65f, .68f));
    } else if (local_available(&g->world, room) && vlen(vsub(feet, local_console(room))) < 5) {
        snprintf(label, sizeof(label), "%s / %s", touch ? "USE" : "G",
                 local_action(room->habitat.tale));
        centered(r, w * .5f, h * .59f, fminf(s, width / (6 * strlen(label))), label, color);
    }
}
