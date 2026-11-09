/* Included after the shared HUD drawing helpers. */
static void matrix_schematic(Renderer *r, const Game *g, int player, float x, float y, float size) {
    const MatrixDiver *d = &g->matrix.divers[player];
    const MatrixNetwork *w = &g->matrix.networks[d->network];
    const MatrixRecord *record = &g->matrix.records[w->record];
    Color color = matrix_color(w->theme), gold = rgb(1, .7f, .25f);
    rect(r, x, y, size, size, rgb(.015f, .03f, .05f));
    frame(r, x, y, size, size, color);
    double max = 1;
    for (unsigned i = 0; i < 16; i++)
        if (w->nodes[i].valid)
            max = fmax(max, fmax(w->nodes[i].at.x, w->nodes[i].at.z));
    float scale = (size - 24) / (float)max;
    for (unsigned i = 0; i < 16; i++)
        if (w->nodes[i].valid) {
            Vec3 at = w->nodes[i].at;
            float px = x + 12 + (float)at.x * scale, py = y + size - 12 - (float)at.z * scale;
            for (int j = i + 1; j < 16; j++)
                if (w->nodes[i].links & (1u << j)) {
                    float tx = x + 12 + (float)w->nodes[j].at.x * scale,
                          ty = y + size - 12 - (float)w->nodes[j].at.z * scale;
                    rect(r, fminf(px, tx), py, fmaxf(1, fabsf(tx - px)), 1, rgb(.12f, .3f, .35f));
                    rect(r, tx, fminf(py, ty), 1, fmaxf(1, fabsf(ty - py)), rgb(.12f, .3f, .35f));
                }
            rect(r, px - 3, py - 3, 6, 6, record->seen & (1u << i) ? color : rgb(.17f, .22f, .28f));
            char mark[2] = {i == w->core         ? 'C'
                            : i == w->credential ? 'K'
                            : i == w->entry      ? 'A'
                            : i == w->switch_b   ? 'B'
                                                 : "PSBDRT"[w->nodes[i].service],
                            0};
            if (mark[0])
                text_at(r, px + 4, py - 8, .65f, mark, gold);
        }
    Vec3 goal = {0};
    matrix_goal(g, player, &goal);
    frame(r, x + 12 + (float)goal.x * scale - 5, y + size - 12 - (float)goal.z * scale - 5, 10, 10,
          gold);
    float px = x + 12 + (float)d->pos.x * scale, py = y + size - 12 - (float)d->pos.z * scale;
    rect(r, px - 2, py - 2, 4, 4, rgb(.9f, 1, .95f));
    rect(r, px + (float)sin(g->players[player].yaw) * 5 - 1,
         py - (float)cos(g->players[player].yaw) * 5 - 1, 2, 2, gold);
}
static void matrix_journal_draw(Renderer *r, const Game *g, int player, float width, float height) {
    const MatrixProfile *p = &g->matrix.profiles[player];
    const MatrixDiver *d = &g->matrix.divers[player];
    float s = fmaxf(.7f, fminf(width / 900, height / 540)), w = fminf(width - 24, 820 * s),
          h = fminf(height - 24, 480 * s), x = (width - w) * .5f, y = (height - h) * .5f;
    Color ink = rgb(.75f, .9f, .9f), cyan = rgb(.25f, .8f, .85f), gold = rgb(1, .7f, .3f);
    char text[220];
    rect(r, x, y, w, h, rgb(.015f, .035f, .055f));
    frame(r, x, y, w, h, cyan);
    if (d->choice && d->active) {
        const MatrixNetwork *n = &g->matrix.networks[d->network];
        const MatrixRecord *record = &g->matrix.records[n->record];
        centered(r, width * .5f, y + 20 * s, 1.6f * s, matrix_controller(n), gold);
        const char *voices[] = {
            "I WAS SPLIT TO KEEP THE CREW ALIVE. EACH FRAGMENT REMEMBERS A DIFFERENT FAILURE.",
            "THEY ASKED ME TO REMEMBER EVERY NAME. THE ARCHIVES HAVE BECOME MY PRISON.",
            "I STILL HEAR THE ENGINE CHOIR. RESTORE ITS POWER OR LET THE VOICES END.",
            "THE BREACH IS CONTAINED INSIDE MY MEMORY. FREE ME CAREFULLY."};
        wrapped(r, x + 24 * s, y + 62 * s, s, w - 48 * s, voices[n->motif % 4], ink);
        const char *options[] = {
            "1 / PRESERVE / RESTORE POWER AND BEACON / KEEPERS GAIN TRUST",
            "2 / PURGE / SILENCE SECURITY / LOCAL LIFE SUPPORT GOES DARK",
            "3 / PARTITION / KEEP KNOWLEDGE AND SAFETY / REQUIRES KEY AND VAULT OR ARCHIVE"};
        for (int i = 0; i < 3; i++)
            wrapped(r, x + 24 * s, y + (150 + i * 65) * s, s, w - 48 * s, options[i],
                    i == 2 && (!record->keys ||
                               !(n->rare ? (record->flags & MX_VAULT) : record->done[MX_ARCHIVE]))
                        ? rgb(.35f, .42f, .48f)
                        : cyan);
        centered(r, width * .5f, y + h - 32 * s, .9f * s,
                 "ENTER LEAVES UNDECIDED / C DISCONNECTS / CONFIRMED CHANGES PERSIST", ink);
    } else {
        centered(r, width * .5f, y + 18 * s, 1.5f * s, "SHIP MATRIX / PROGRAMS", gold);
        snprintf(text, sizeof(text), "DATA %u / DIVES %u / KNOWLEDGE %u / SLOT %u SELECTED",
                 p->data, p->dives, p->knowledge, d->selected + 1);
        text_at(r, x + 22 * s, y + 48 * s, s, text, ink);
        snprintf(text, sizeof(text), "Z SLOT 1: %s / X SLOT 2: %s / O UPGRADE SELECTED",
                 sc_matrix_programs[p->slots[0]], sc_matrix_programs[p->slots[1]]);
        text_at(r, x + 22 * s, y + 69 * s, .85f * s, text, cyan);
        const char *effects[] = {"HIDE UNTIL FIRING OR OPERATING A NODE",
                                 "DECOY ATTRACTS ROUTINES / REDUCES TRACE",
                                 "STUN NEARBY ROUTINES / BRIEFLY OPEN FIREWALLS",
                                 "RESTORE A BROKEN CROSSING TEMPORARILY",
                                 "SOLVE BOTH ROUTING SWITCHES FROM EITHER PAD",
                                 "KILLS RESTORE BANDWIDTH / UPGRADE ENABLES ROBOT ALLIES"};
        for (unsigned i = 0; i < 6; i++) {
            bool owned = (p->programs & (1u << i)) != 0;
            snprintf(text, sizeof(text), "%u / %s / %s / BW %.0f / LEVEL %u", i + 1,
                     sc_matrix_programs[i], owned ? "EQUIP" : "FIND CACHE", sc_matrix_costs[i],
                     p->upgrades[i]);
            text_at(r, x + 22 * s, y + (109 + i * 44) * s, s, text,
                    owned ? cyan : rgb(.38f, .42f, .48f));
            text_at(r, x + 22 * s, y + (125 + i * 44) * s, .72f * s, effects[i], ink);
        }
        matrix_hint(g, player, text, sizeof(text));
        wrapped(r, x + 22 * s, y + 390 * s, .85f * s, w - 44 * s, text, gold);
        centered(r, width * .5f, y + h - 28 * s, .8f * s,
                 "1-6 EQUIP / Z-X SELECT SLOT / O UPGRADE (3,5,7 DATA) / ENTER RETURN", ink);
    }
}
static void matrix_hud(Renderer *r, const Game *g, int player, float w, float h, bool paused,
                       bool touch, const char *save_status) {
    const Player *body = &g->players[player];
    const MatrixDiver *d = &g->matrix.divers[player];
    const MatrixProfile *p = &g->matrix.profiles[player];
    const MatrixNetwork *n = &g->matrix.networks[d->network];
    float s = fmaxf(.7f, fminf(w / 960, h / 540)), pad = 20 * s;
    Color ink = rgb(.75f, .9f, .92f), cyan = matrix_color(n->theme), gold = rgb(1, .72f, .25f),
          red = rgb(.9f, .2f, .35f);
    char text[200];
    if (onboarding_matrix_intro(g, player)) {
        centered(r, w * .5f, pad, 1.45f * s, "FIRST LINK / RESTORE THIS ROOM'S POWER", cyan);
        if (body->guide_mode != GUIDE_OFF) {
            onboarding_text(LESSON_MATRIX, body->guide_device, text, sizeof(text));
            wrapped(r, pad, h - 100 * s, s, w - 2 * pad, text, gold);
        }
        if (touch && !paused) {
            float size = fminf(w, h) * .12f;
            const float xs[] = {.90f, .64f};
            for (int i = 0; i < 2; i++) {
                rect(r, w * xs[i] - size * .5f, h * .27f - size * .5f, size, size,
                     rgb(.02f, .05f, .08f));
                centered(r, w * xs[i], h * .27f - 4 * s, s, i ? "EXIT" : "USE", cyan);
            }
        }
        return;
    }
    matrix_schematic(r, g, player, pad, pad, 116 * s);
    centered(r, w * .5f, pad, 1.45f * s, "SHIP MATRIX / UPLINK ACTIVE", cyan);
    snprintf(text, sizeof(text), "%s / %s", world_district_name((District)n->theme),
             matrix_controller(n));
    centered(r, w * .5f, pad + 24 * s, .9f * s, text, ink);
    int node = matrix_node(n, d->pos, 7);
    if (node >= 0) {
        const MatrixNode *owner = &n->nodes[node];
        snprintf(text, sizeof(text), "ROOM [%d,%d,%d] / %s", owner->room.x, owner->room.y,
                 owner->room.z, sc_matrix_services[owner->service]);
        centered(r, w * .5f, pad + 64 * s, .75f * s, text, cyan);
    }
    matrix_hint(g, player, text, sizeof(text));
    centered(r, w * .5f, h - 100 * s, .9f * s, text, gold);
    snprintf(text, sizeof(text), "F / %s / Q-E CYCLE / G DATA", sc_matrix_services[d->service]);
    centered(r, w * .5f, h - 78 * s, .9f * s, text, cyan);
    float bar = fminf(230 * s, w * .3f);
    snprintf(text, sizeof(text), "INTEGRITY %.0f / BANDWIDTH %.0f", d->integrity, d->bandwidth);
    text_at(r, pad, h - 58 * s, .9f * s, text, ink);
    rect(r, pad, h - 40 * s, bar, 5 * s, rgb(.1f, .16f, .22f));
    rect(r, pad, h - 40 * s, bar * fminf(1, d->integrity / (100 + 2 * body->mods[4])), 5 * s, cyan);
    rect(r, pad, h - 30 * s, bar, 5 * s, rgb(.1f, .16f, .22f));
    rect(r, pad, h - 30 * s, bar * fminf(1, d->bandwidth / (100 + 4 * body->mods[5])), 5 * s, gold);
    snprintf(text, sizeof(text), "TRACE %.0f / NEXT %u / BODY %.0f %s", d->trace,
             d->trace < 25   ? 25
             : d->trace < 50 ? 50
             : d->trace < 75 ? 75
                             : 100,
             body->health, matrix_protected(g, player) ? "SAFE" : "EXPOSED");
    text_at(r, w - bar - pad - 35 * s, h - 58 * s, .8f * s, text, d->trace >= 75 ? red : ink);
    rect(r, w - bar - pad, h - 40 * s, bar, 5 * s, rgb(.1f, .16f, .22f));
    rect(r, w - bar - pad, h - 40 * s, bar * d->trace / 100, 5 * s, d->trace >= 75 ? red : gold);
    snprintf(text, sizeof(text), "1 %s / 2 %s / DATA %u + %u", sc_matrix_programs[p->slots[0]],
             sc_matrix_programs[p->slots[1]], p->data, d->unbanked);
    centered(r, w * .5f, h - 18 * s, .8f * s, text, ink);
    if (!touch)
        centered(r, w * .5f, pad + 45 * s, .75f * s,
                 "CLICK PULSE / SPACE JUMP / M NETWORK / F8 PROGRAMS / C DISCONNECT", ink);
    if (n->switches[0] > 0 || n->switches[1] > 0) {
        snprintf(text, sizeof(text), "ROUTING WINDOW %.1f S",
                 fmaxf(n->switches[0], n->switches[1]));
        centered(r, w * .5f, h * .27f, s, text, gold);
    }
    if (d->counter > 0) {
        snprintf(text, sizeof(text), "COUNTERINTRUSION %.1f S / DISCONNECT OR REDUCE TRACE",
                 d->counter);
        centered(r, w * .5f, h * .32f, s, text, red);
    }
    if (d->veil > 0)
        text_at(r, pad, pad + 132 * s, s, "VEIL ACTIVE", cyan);
    if (d->siphon > 0)
        text_at(r, pad, pad + 148 * s, s, "SIPHON ACTIVE", gold);
    rect(r, w * .5f - 4 * s, h * .5f, 8 * s, 1 * s, ink);
    rect(r, w * .5f, h * .5f - 4 * s, 1 * s, 8 * s, ink);
    if (touch && !paused && !d->map) {
        float size = fminf(w, h) * .12f;
        const char *names[] = {"PULSE", "JUMP", "CROUCH", "SVC +", "SVC -", "SERVICE",
                               "USE",   "SLOT", "EXIT",   "CAST",  "SLOT",  "MAP"};
        const float xs[] = {.90f, .77f, .64f, .90f, .77f, .64f, .90f, .77f, .64f, .51f, .51f, .51f},
                    ys[] = {.72f, .85f, .85f, .48f, .48f, .48f, .27f, .27f, .27f, .27f, .48f, .1f};
        for (int i = 0; i < 12; i++) {
            float x = w * xs[i], y = h * ys[i];
            rect(r, x - size * .5f, y - size * .5f, size, size, rgb(.02f, .05f, .08f));
            frame(r, x - size * .5f, y - size * .5f, size, size, cyan);
            centered(r, x, y - 3 * s, .8f * s, names[i], ink);
        }
    }
    if (g->message_time > 0)
        centered(r, w * .5f, h * .19f, .85f * s, g->message, gold);
    if (d->map) {
        float size = fminf(w * .7f, h * .7f);
        matrix_schematic(r, g, player, (w - size) * .5f, (h - size) * .5f, size);
        centered(r, w * .5f, (h - size) * .5f - 20 * s, s,
                 "A/B ROUTING / K KEY / C CONTROLLER / GOLD NEXT GOAL / M CLOSE", gold);
    }
    if (paused && !body->journal)
        centered(r, w * .5f, h * .4f, 1.6f * s, "LINK PAUSED / ENTER TO RESUME", gold);
    if (save_status && *save_status)
        text_at(r, pad, h - 10 * s, .65f * s, save_status, ink);
    if (body->journal && body->journal_page == 4)
        matrix_journal_draw(r, g, player, w, h);
}
