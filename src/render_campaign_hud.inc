static void campaign_journal_draw(Renderer *r, const Game *g, int player, float w, float h) {
    /* Keep every bitmap stroke at least one pixel wide. Flowing wrapped lines
       use the tall phone viewport instead of shrinking a desktop panel. */
    bool compact = h < 500, narrow = w < 760;
    float s = fmaxf(1, fminf(1.3f, fminf((w - 24) / 760, (h - 24) / 480)));
    float pw = fminf(w - 24, 760 * s), ph = fminf(h - 24, (narrow ? 650 : 480) * s);
    float x = (w - pw) / 2, y = (h - ph) / 2;
    float left = x + 14 * s, width = pw - 28 * s, cursor = y + 12 * s;
    float gap = compact ? 2 : 7 * s;
    Color gold = rgb(.9f, .7f, .3f), ink = rgb(.85f, .9f, .87f), mint = rgb(.3f, .85f, .75f);
    rect(r, x, y, pw, ph, rgb(.025f, .045f, .055f));
    frame(r, x, y, pw, ph, gold);
    char text[150];
    const Campaign *c = &g->campaign;
    const CampaignProfile *p = &c->profiles[player];
    unsigned phase = campaign_phase(c);
    snprintf(text, sizeof(text), "EXPEDITION / %s / PHASE %u OF 4", campaign_phase_name(phase),
             phase < 4 ? phase + 1 : 4);
    cursor = wrapped(r, left, cursor, 1.1f * s, width, text, gold) + gap;
    if (!c->version) {
        cursor = wrapped(r, left, cursor, s, width,
                         "LEGACY EXPEDITION / EXISTING CHARACTER RETAINED", ink) +
                 gap;
        wrapped(r, left, cursor, s, width, "N THEN ENTER STARTS A NEW SHIP WITH NEW CHARACTERS",
                mint);
        return;
    }
    unsigned id = c->choice[player]           ? c->choice[player] - 1
                  : c->tracked < SC_GUARDIANS ? c->tracked
                                              : 0;
    Cell site = campaign_site(id);
    bool story_guidance = !c->choice[player] && c->tracked == SC_GUARDIANS;
    snprintf(text, sizeof(text), "%s / [%d,%d,%d] / %s", campaign_guardian_name(g->world.seed, id),
             site.x, site.y, site.z, c->choice[player] ? "CLAIM ONE REWARD" : "U CHANGES ROUTE");
    cursor =
        wrapped(r, left, cursor, s, width,
                story_guidance ? "STORY GUIDANCE / I READ LEADS / U GUARDIAN ROUTES" : text, ink) +
        gap;
    if (story_guidance)
        cursor = wrapped(r, left, cursor, s, width, game_mission(g), mint) + gap;
    for (unsigned i = 0; !story_guidance && i < 3; i++) {
        char reward[110];
        campaign_reward_name(g, player, id, i, reward, sizeof(reward));
        snprintf(text, sizeof(text), "%u / %s", i + 1, reward);
        cursor = wrapped(r, left, cursor, s, width, text, mint) + 1;
    }
    cursor = wrapped(r, left, cursor + gap, s, width,
                     story_guidance      ? "G AT A STORY CONSOLE / I READ LEADS"
                     : c->choice[player] ? "1 / 2 / 3 CLAIM / EACH VICTORY PAYS ONCE"
                                         : "G AT STAGING BEGINS THE FIGHT / VICTORY EARNS REWARDS",
                     gold) +
             gap;
    ShipTraits axes = campaign_traits(g->world.seed, (Cell){12, 0, 0});
    ShipTraits up = campaign_traits(g->world.seed, (Cell){0, 6, 0});
    snprintf(text, sizeof(text), "STARBOARD %s / PORT %s / UP %s / DOWN %s",
             axes.heat ? (axes.heat > 0 ? "HOT" : "COLD") : (axes.order > 0 ? "ORDER" : "CHAOS"),
             axes.heat ? (axes.heat > 0 ? "COLD" : "HOT") : (axes.order > 0 ? "CHAOS" : "ORDER"),
             up.heat ? (up.heat > 0 ? "HOT" : "COLD") : (up.order > 0 ? "ORDER" : "CHAOS"),
             up.heat ? (up.heat > 0 ? "COLD" : "HOT") : (up.order > 0 ? "CHAOS" : "ORDER"));
    cursor = wrapped(r, left, cursor, s, width, text, ink) + gap;
    const char *labels[] = {"GUN 1", "GUN 2", "POWER 1", "POWER 2", "RIG", "SPECIALIZATION"};
    for (unsigned i = 0; i < 6; i++) {
        const char *value =
            i < 2   ? sc_weapons[p->guns[i]].name
            : i < 4 ? (p->power_slots[i - 2] < SC_PSIONICS ? sc_psionic_names[p->power_slots[i - 2]]
                                                           : "EMPTY")
            : i == 4 ? (p->rig == MECH    ? "FOLD MECH"
                        : p->rig == WAGON ? "FOLD WAGON"
                                          : "EMPTY")
                     : campaign_module_name(p->module);
        snprintf(text, sizeof(text), "%c %-14s %s", c->slot[player] == i ? '>' : ' ', labels[i],
                 value);
        cursor = wrapped(r, left, cursor, s, width, text, c->slot[player] == i ? gold : ink) + 1;
    }
    static const char *effects[] = {"EARN SPECIALIZATIONS FROM GUARDIANS",
                                    "MOBILE STORM / +60% WEAPON AND NOVA DAMAGE WHILE FLYING",
                                    "SIEGE ENGINEER / +50% MECH WEAPON DAMAGE",
                                    "SILENT HUNTER / +150% DAMAGE ON THE SHOT THAT BREAKS VEIL",
                                    "SYSTEMS SABOTEUR / +60% DAMAGE IN SECURITY-SILENCED ROOMS"};
    cursor = wrapped(r, left, cursor + gap, s, width, effects[p->module], mint) + gap;
    cursor = wrapped(r, left, cursor, s, width,
                     campaign_safe_refit(g, player)
                         ? "REFIT READY / Q E SLOT / Z X CHANGE ITEM"
                         : "REFIT AT BAY, SHELTER OR A DEFEATED GUARDIAN'S CONSOLE",
                     gold) +
             gap;
    if (!compact)
        cursor = wrapped(r, left, cursor, s, width,
                         "PAD: SHOULDERS SLOT / LEFT RIGHT REFIT / X ROUTE / A Y B REWARD", ink) +
                 gap;
    wrapped(r, left, cursor, s, width,
            compact ? "TOUCH: MENU CONTROLS / ENTER OR START CLOSE"
                    : "ENTER / START CLOSE / NEW SHIP STARTS A NEW CHARACTER",
            ink);
}
static void campaign_hud(Renderer *r, const Game *g, int player, float w, float h, float s) {
    if (!g->campaign.version || g->players[player].sheltered)
        return;
    const Player *p = &g->players[player];
    Cell cell = world_cell(position_absolute(p->pos));
    ShipTraits t = campaign_traits(g->world.seed, cell);
    char label[160];
    snprintf(label, sizeof(label), "%s / LAYER %u / %s %d / %s %d / F10 BUILD",
             campaign_phase_name(campaign_phase(&g->campaign)), campaign_layer(cell),
             t.heat < 0 ? "COLD" : "HEAT", abs(t.heat), t.order < 0 ? "CHAOS" : "ORDER",
             abs(t.order));
    centered(r, w * .5f, 116 * s, .8f * s, label, rgb(.55f, .75f, .8f));
    for (unsigned id = 0; id < SC_GUARDIANS; id++) {
        const GuardianAttempt *a = &g->campaign.attempts[id];
        if (a->active && (a->participants & (1u << player))) {
            Room *room = world_find((World *)&g->world, campaign_site(id));
            if (!room || !room->ready)
                continue;
            const Enemy *e = &room->enemies[0];
            snprintf(label, sizeof(label), "%s / %s / %.0f",
                     campaign_guardian_name(g->world.seed, id),
                     e->windup > 0    ? "MOVE / ATTACK INCOMING"
                     : a->opening > 0 ? "CORE EXPOSED"
                                      : "ARMORED",
                     e->health);
            Color c = a->opening > 0 ? rgb(.25f, .9f, .65f) : rgb(1, .55f, .2f);
            centered(r, w * .5f, 139 * s, .85f * s, label, c);
            rect(r, w * .3f, 155 * s,
                 w * .4f * fmaxf(0, e->health) / campaign_guardian_health(id, a->participants),
                 4 * s, c);
        } else if (vlen(vsub(position_absolute(p->pos), game_terminal(campaign_site(id)))) < 4) {
            centered(r, w * .5f, h * .63f, s,
                     (g->campaign.defeated & (1u << id))
                         ? "G / REWARDS AND SAFE REFIT"
                         : "G / CHALLENGE GUARDIAN / F10 PREVIEW REWARDS",
                     rgb(.95f, .7f, .3f));
        }
    }
}
