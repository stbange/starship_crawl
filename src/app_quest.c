#include "app_internal.h"
#include "client_actions.h"
#ifdef SC_WEBXR
bool quest_running, quest_graphics_lost;
double quest_dt;
Actions quest_actions;
/* WebXR platform adapter. The simulation and save format remain shared. */
static struct {
    QuestView view;
    float packet[SC_QUEST_POSES * 16 + 32];
    unsigned buttons, frames, eyes;
    bool suspended, confirm, neutral, menu_latched, anchor, physical_crouch;
    bool tracking, vignette;
    int page, selection, count, info_page, info_lines, actions[96];
    char labels[96][80];
    double timestamp;
    float transition;
} xr;
enum {
    XR_HOME,
    XR_STORY,
    XR_BUILD,
    XR_CHARACTER,
    XR_TOOLS,
    XR_MATRIX,
    XR_MAP,
    XR_DOSSIER,
    XR_DEBRIEF,
    XR_WEAPONS,
    XR_NEW,
    XR_COURTS,
    XR_HELP
};
enum {
    XR_BACK = -1,
    XR_RESUME = -2,
    XR_CENTER = -3,
    XR_SAVE = -4,
    XR_EXIT = -5,
    XR_COMFORT = -6,
    XR_NEW_CONFIRM = -7,
    XR_TEXT = -8,
    XR_GUIDANCE = -9,
    XR_DISMISS = -10,
    XR_PAGE = -100
};
EM_JS(void, quest_exit, (void), {
    if (window.scQuest)
        window.scQuest.end();
})
EM_JS(void, quest_comfort_store, (int enabled), {
    if (window.scQuest)
        window.scQuest.comfort(!!enabled);
})
static Vec3 quest_base(void) {
    if (app.game.matrix.divers[0].active)
        return matrix_eye(&app.game, 0);
    Vec3 base = player_eye(&app.game.players[0]);
    if (xr.physical_crouch && app.game.players[0].crouched && !app.game.players[0].riding)
        base.y += .77;
    return base;
}
static void quest_anchor(void) {
    QuestView *q = &xr.view;
    Vec3 forward = quest_forward(&q->tracking, QUEST_HEAD);
    forward = vnorm(v3(forward.x, 0, forward.z));
    if (vlen(forward) < .1)
        forward = facing(app.game.players[0].yaw, 0);
    q->menu_right = v3(forward.z, 0, -forward.x);
    q->menu_up = v3(0, 1, 0);
    q->menu_center = vadd(q->head, vadd(vmul(forward, 1.5), v3(0, -.12, 0)));
    xr.anchor = true;
}
static void quest_open(int page) {
    xr.page = page;
    xr.selection = 0;
    xr.info_page = 0;
    xr.menu_latched = true;
    set_pause(true);
    Player *player = &app.game.players[0];
    PLAYER_VIEW(&app.game, player).journal = true;
    PLAYER_VIEW(&app.game, player).journal_page = page == XR_STORY     ? 1
                                                  : page == XR_DOSSIER ? 2
                                                  : page == XR_DEBRIEF ? 3
                                                  : page == XR_MATRIX  ? 4
                                                                       : 0;
    if (page == XR_BUILD)
        campaign_journal(&app.game, 0);
    if (page == XR_COURTS)
        politics_journal(&app.game, 0);
    app.game.views[0].map_open = page == XR_MAP;
    if (page == XR_MAP)
        app.game.views[0].map_center = world_cell(position_absolute(app.game.players[0].pos));
    quest_anchor();
}
static void quest_resume(void) {
    xr.suspended = false;
    set_pause(false);
    app.game.views[0].map_open = false;
    app.game.views[0].matrix_map = false;
    quest_actions = (Actions){0};
    xr.view.tracking.turn_latched = true;
    xr.transition = 1;
}
EMSCRIPTEN_KEEPALIVE void sc_xr_suspend(void) {
    if (!quest_running)
        return;
    if (!xr.suspended) {
        xr.suspended = true;
        quest_open(XR_HOME);
    }
    xr.confirm = true;
    xr.neutral = false;
    quest_actions = (Actions){0};
    app.session.accumulator = 0;
    app.last = (double)SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
    xr.timestamp = 0;
}
static void quest_add(int action, const char *label) {
    if (action >= CLIENT_PAUSE && !onboarding_action(&app.game, 0, action))
        return;
    if (xr.count >= 96)
        return;
    xr.actions[xr.count] = action;
    snprintf(xr.labels[xr.count++], 80, "%s", label);
}
static void quest_line(const char *text) {
    QuestView *q = &xr.view;
    /* Paginate complete accounts rather than shrink or silently truncate them. */
    while (*text) {
        size_t n = strlen(text);
        if (n > 36) {
            n = 36;
            while (n > 1 && text[n] != ' ')
                n--;
            if (n == 1)
                n = 36;
        }
        if (xr.info_lines / 14 == xr.info_page && q->line_count < 14)
            snprintf(q->lines[q->line_count++], 128, "%.*s", (int)n, text);
        xr.info_lines++;
        text += n;
        while (*text == ' ')
            text++;
    }
}
static void quest_menu(void) {
    QuestView *q = &xr.view;
    Player *p = &app.game.players[0];
    Campaign *c = &app.game.campaign;
    MatrixDiver *d = &app.game.matrix.divers[0];
    const char *titles[] = {"STARSHIP CRAWL / QUEST",
                            "EXPEDITION / STORY",
                            "PHASES / LOADOUT",
                            "CHARACTER",
                            "FIELD EQUIPMENT",
                            "SHIP MATRIX",
                            "DECK MAP",
                            "SHIP DOSSIER",
                            "EXPEDITION DEBRIEF",
                            "WEAPONS",
                            "BEGIN A NEW EXPEDITION?",
                            "LEGACY COURTS / KINSHIPS",
                            "FIELD GUIDE"};
    snprintf(q->title, sizeof(q->title), "%s", titles[xr.page]);
    q->line_count = 0;
    xr.count = 0;
    xr.info_lines = 0;
    char line[512];
    if (xr.suspended) {
        quest_line(xr.tracking ? "TRACKING READY" : "WAITING FOR HEADSET AND BOTH CONTROLLERS");
        quest_line("Release all buttons and sticks. Press A to resume when ready.");
    } else if (xr.page == XR_HOME) {
        unsigned caps = onboarding_capabilities(&app.game, 0);
        quest_line(game_mission(&app.game));
        quest_line(
            "Move: left stick. Turn: right stick. Fire: right trigger. Interact: right grip.");
        quest_line(caps & OB_GUNS
                       ? "A: jump / lift up. B: crouch / lift down. X: switch weapon. Y: menu."
                       : "A: jump / lift up. B: crouch / lift down. Y: menu.");
        quest_line("Left grip: wrist map. Left trigger: flashlight. Right stick click: recenter.");
        quest_add(XR_RESUME, "RESUME EXPEDITION");
        quest_add(XR_PAGE - XR_STORY, "STORY AND DECISIONS");
        if (caps & OB_BUILD)
            quest_add(XR_PAGE - XR_BUILD, "PHASES AND LOADOUT");
        if (caps & OB_UPGRADES)
            quest_add(XR_PAGE - XR_CHARACTER, "CHARACTER UPGRADES");
        if (caps & (OB_SHELTER | OB_POWERS | OB_RIGS))
            quest_add(XR_PAGE - XR_TOOLS, "ABILITIES AND EQUIPMENT");
        if (caps & OB_GUNS)
            quest_add(XR_PAGE - XR_WEAPONS, "WEAPONS");
        quest_add(XR_PAGE - XR_MAP, "MAP AND OBJECTIVE");
        if (caps & OB_PROGRAMS)
            quest_add(XR_PAGE - XR_MATRIX, "MATRIX PROGRAMS");
        else if (caps & OB_MATRIX)
            quest_add(200, "LINK AT TERMINAL");
        if (onboarding_action(&app.game, 0, 85))
            quest_add(XR_PAGE - XR_DOSSIER, "SHIP DOSSIER");
        if (onboarding_action(&app.game, 0, 86))
            quest_add(XR_PAGE - XR_DEBRIEF, "EXPEDITION DEBRIEF");
        if (app.game.politics.version)
            quest_add(XR_PAGE - XR_COURTS, "ALIEN COURTS");
        snprintf(line, sizeof(line), "GUIDANCE: %s",
                 onboarding_mode(app.renderer.preferences.view[0].guidance));
        quest_add(XR_GUIDANCE, line);
        quest_add(XR_PAGE - XR_HELP, "FIELD GUIDE / REVIEW INSTRUCTIONS");
        quest_add(XR_DISMISS, "DISMISS CURRENT HINT");
        quest_add(XR_CENTER, "RECENTER SEATED / STANDING");
        quest_add(XR_COMFORT, xr.vignette ? "MOVEMENT VIGNETTE: ON" : "MOVEMENT VIGNETTE: OFF");
        quest_add(XR_SAVE, "SAVE EXPEDITION");
        quest_add(XR_PAGE - XR_NEW, "NEW SHIP / NEW CHARACTER");
        quest_add(XR_EXIT, "SAVE AND EXIT VR");
    } else if (xr.page == XR_HELP) {
        quest_line("Explore, fight, recover. Three kills earn a shotgun. An elite or guardian "
                   "holds the fold shelter.");
        quest_line("After the first guardian, clear a layer-2 terminal to recover a matrix "
                   "interface. Hacking is optional.");
        for (unsigned lesson = 1; lesson < LESSON_COUNT; lesson++) {
            onboarding_text(lesson, GUIDE_QUEST, line, sizeof(line));
            quest_line(line);
        }
    } else if (xr.page == XR_STORY) {
        for (int i = 0; i < 6; i++) {
            game_story_line(&app.game, i, line, sizeof(line));
            quest_line(line);
        }
        game_story_entry(&app.game, PLAYER_VIEW(&app.game, p).story_reading, line, sizeof(line));
        quest_line(line);
        quest_add(81, "TRACK NEXT STORY LEAD");
        quest_add(83, "READ NEXT FIELD RECORD");
        if (PLAYER_INTERACTION(&app.game, p).decision_open)
            for (unsigned i = 1; i <= 3; i++) {
                snprintf(line, sizeof(line), "%u: %s", i, game_story_choice_for(&app.game, 0, i));
                quest_line(line);
                if (i == 3 && !game_story_choice_ready(&app.game, 0))
                    quest_line("THIRD OUTCOME REQUIRES OPTIONAL PREPARATION");
                quest_add(89 + (int)i, line);
            }
        quest_add(8, "INVESTIGATE NEARBY SITE");
    } else if (xr.page == XR_BUILD) {
        if (!c->version) {
            quest_line("LEGACY EXPEDITION / EXISTING CHARACTER AND EQUIPMENT RETAINED");
            quest_line(game_mission(&app.game));
            quest_add(XR_PAGE - XR_WEAPONS, "SELECT OWNED WEAPONS");
        } else {
            snprintf(line, sizeof(line), "PHASE %u / %s",
                     campaign_phase(c) < 4 ? campaign_phase(c) + 1 : 4,
                     campaign_phase_name(campaign_phase(c)));
            quest_line(line);
            CampaignProfile *profile = &c->profiles[0];
            snprintf(line, sizeof(line), "WEAPONS: %s / %s", sc_weapons[profile->guns[0]].name,
                     profile->guns[1] < SC_WEAPONS ? sc_weapons[profile->guns[1]].name : "EMPTY");
            quest_line(line);
            const char *slots[] = {"GUN 1", "GUN 2", "POWER 1", "POWER 2", "RIG", "MODULE"};
            snprintf(line, sizeof(line), "SELECTED SLOT: %s / REFIT AT SAFETY",
                     slots[app.game.views[0].loadout_slot]);
            quest_line(line);
            snprintf(
                line, sizeof(line), "POWERS: %s / %s",
                profile->power_slots[0] < SC_PSIONICS ? sc_psionic_names[profile->power_slots[0]]
                                                      : "EMPTY",
                profile->power_slots[1] < SC_PSIONICS ? sc_psionic_names[profile->power_slots[1]]
                                                      : "EMPTY");
            quest_line(line);
            snprintf(line, sizeof(line), "RIG: %s / MODULE: %s",
                     profile->rig == MECH    ? "MECH"
                     : profile->rig == WAGON ? "WAGON"
                                             : "NONE",
                     campaign_module_name(profile->module));
            quest_line(line);
            unsigned id = c->choice[0]                ? c->choice[0] - 1
                          : c->tracked < SC_GUARDIANS ? c->tracked
                                                      : 0;
            quest_line(campaign_guardian_name(app.game.world.seed, id));
            for (unsigned i = 0; i < 3; i++) {
                campaign_reward_name(&app.game, 0, id, i, line, sizeof(line));
                quest_line(line);
                if (c->choice[0])
                    quest_add(226 + (int)i, line);
            }
            quest_add(225, "TRACK NEXT GUARDIAN");
            quest_add(221, "PREVIOUS LOADOUT SLOT");
            quest_add(222, "NEXT LOADOUT SLOT");
            quest_add(223, "REFIT PREVIOUS ITEM");
            quest_add(224, "REFIT NEXT ITEM");
            quest_add(8, "ACTIVATE NEARBY STAGING");
        }
    } else if (xr.page == XR_CHARACTER) {
        snprintf(line, sizeof(line), "LEVEL %u / UPGRADE POINTS %u / SCRAP %u", p->level, p->points,
                 p->credits);
        quest_line(line);
        for (int i = 0; i < SC_ATTRIBUTES; i++) {
            snprintf(line, sizeof(line), "%s %u / UPGRADE", sc_attributes[i],
                     *player_attribute(p, i));
            quest_add(20 + i, line);
        }
        quest_add(4, "CHANGE SUIT COLOR");
    } else if (xr.page == XR_TOOLS) {
        quest_line("Equipment and powers must be earned and fitted. Deployment uses normal safety "
                   "and cooldown rules.");
        const char *labels[] = {"FOLD / UNFOLD SHELTER",
                                "DEPLOY / FOLD MECH",
                                "DEPLOY / FOLD WAGON",
                                "SELECT FLARE CANNON",
                                "UPGRADE FLARE LIFETIME",
                                "UPGRADE FLARE STRENGTH",
                                "VEIL / INVISIBILITY",
                                "FLIGHT",
                                "LONG JUMP",
                                "PSIONIC NOVA",
                                "MECH WEAPON 1",
                                "MECH WEAPON 2",
                                "MECH WEAPON 3",
                                "INTERACT / BOARD / EXIT"};
        const int actions[] = {13, 15, 16, 17, 18, 19, 60, 61, 62, 63, 70, 71, 72, 8};
        for (unsigned i = 0; i < sizeof(actions) / sizeof(*actions); i++)
            quest_add(actions[i], labels[i]);
    } else if (xr.page == XR_MATRIX) {
        matrix_hint(&app.game, 0, line, sizeof(line));
        quest_line(line);
        const MatrixProfile *profile = &app.game.matrix.profiles[0];
        snprintf(line, sizeof(line), "DATA %u / SLOT %u: %s", profile->data, d->selected + 1,
                 sc_matrix_programs[profile->slots[d->selected]]);
        quest_line(line);
        if (d->choice) {
            quest_line("PRESERVE: RESTORE POWER AND BEACON; KEEPERS GAIN TRUST.");
            quest_line("PURGE: SILENCE SECURITY; LOCAL LIFE SUPPORT GOES DARK.");
            quest_line(
                "PARTITION: KEEP KNOWLEDGE AND SAFETY; REQUIRES A KEY AND VAULT OR ARCHIVE.");
            quest_add(90, "PRESERVE CONTROLLER");
            quest_add(91, "PURGE CONTROLLER");
            quest_add(92, "PARTITION CONTROLLER");
        }
        quest_add(200, d->active ? "DISCONNECT / RETURN" : "CONNECT AT TERMINAL");
        quest_add(201, "OPERATE SELECTED SERVICE");
        quest_add(202, "PREVIOUS SERVICE");
        quest_add(203, "NEXT SERVICE");
        quest_add(204, "CAST PROGRAM IN SLOT 1");
        quest_add(205, "CAST PROGRAM IN SLOT 2");
        quest_add(208, "SELECT PROGRAM SLOT 1");
        quest_add(209, "SELECT PROGRAM SLOT 2");
        quest_add(207, "UPGRADE SELECTED PROGRAM");
        for (int i = 0; i < SC_MATRIX_PROGRAMS; i++) {
            snprintf(line, sizeof(line), "EQUIP %s", sc_matrix_programs[i]);
            quest_add(210 + i, line);
        }
    } else if (xr.page == XR_MAP) {
        quest_line("GOLD: MISSION / LEFT GRIP SHOWS WRIST MAP");
        const char *labels[] = {"PAN WEST", "PAN EAST", "PAN FORWARD",
                                "PAN BACK", "DECK UP",  "DECK DOWN"};
        for (int i = 0; i < 6; i++)
            quest_add(30 + i, labels[i]);
        quest_add(81, "TRACK STORY LEAD");
        quest_add(225, "TRACK GUARDIAN");
    } else if (xr.page == XR_DOSSIER || xr.page == XR_DEBRIEF) {
        for (int i = 0; i < (xr.page == XR_DOSSIER ? 8 : 10); i++) {
            if (xr.page == XR_DOSSIER)
                game_story_dossier(&app.game, i, line, sizeof(line));
            else
                game_story_debrief(&app.game, 0, i, line, sizeof(line));
            quest_line(line);
        }
        quest_add(88, "PREVIOUS EVENT");
        quest_add(12, "NEXT RECOVERED LOG");
    } else if (xr.page == XR_COURTS) {
        for (int i = 0; i <= 10; i++) {
            politics_line(&app.game, 0, i, line, sizeof(line));
            quest_line(line);
        }
        quest_add(231, "READ NEXT COURT");
        quest_add(232, "TRACK COURT");
        if (app.game.politics.audience[0]) {
            quest_add(90, "CHOOSE FIRST OUTCOME");
            quest_add(91, "CHOOSE SECOND OUTCOME");
            quest_add(92, "CHOOSE THIRD OUTCOME");
        }
    } else if (xr.page == XR_WEAPONS) {
        quest_line(game_gun(p)->name);
        quest_line(
            "Only owned, fitted weapons can be selected. Refit your loadout at a safe location.");
        for (unsigned i = 0; i < SC_WEAPONS; i++)
            if ((p->weapons & (1u << i)) && campaign_weapon(&app.game, 0, i))
                quest_add(40 + (int)i, sc_weapons[i].name);
    } else if (xr.page == XR_NEW) {
        quest_line("This replaces the Quest expedition with a new ship and a new character. Export "
                   "a backup from the launch page first if needed.");
        quest_add(XR_NEW_CONFIRM, "START A NEW SHIP");
    }
    if (xr.info_lines > 14)
        quest_add(XR_TEXT, "NEXT TEXT PAGE");
    if (xr.page != XR_HOME)
        quest_add(XR_BACK, "BACK TO MENU");
    if (xr.count)
        xr.selection = (xr.selection + xr.count) % xr.count;
    int start = xr.selection / 8 * 8;
    q->item_count = 0;
    q->selected = xr.selection - start;
    for (int i = start; i < xr.count && i < start + 8; i++)
        snprintf(q->items[q->item_count++], 80, "%s", xr.labels[i]);
    snprintf(q->footer, sizeof(q->footer), "TEXT %d/%d / ITEMS %d-%d OF %d / %.52s",
             xr.info_page + 1, xr.info_lines ? (xr.info_lines + 13) / 14 : 1,
             xr.count ? start + 1 : 0, start + q->item_count, xr.count,
             app.game.message_time > 0 ? app.game.message : app.save_status);
}
static void quest_activate(void) {
    if (!xr.count)
        return;
    int action = xr.actions[xr.selection];
    if (action <= XR_PAGE) {
        quest_open(XR_PAGE - action);
        return;
    }
    if (action == XR_BACK)
        quest_open(XR_HOME);
    else if (action == XR_TEXT)
        xr.info_page = (xr.info_page + 1) % ((xr.info_lines + 13) / 14);
    else if (action == XR_RESUME)
        quest_resume();
    else if (action == XR_CENTER) {
        quest_calibrate(&xr.view.tracking, app.game.players[0].yaw);
        xr.anchor = false;
    } else if (action == XR_SAVE)
        save_game();
    else if (action == XR_GUIDANCE) {
        sc_web_setting(0, PREF_GUIDANCE, (app.renderer.preferences.view[0].guidance + 1) % 3);
        app.game.views[0].guide_mode = app.renderer.preferences.view[0].guidance;
    } else if (action == XR_DISMISS) {
        app_command(0, CMD_DISMISS_HINT, 0, 0);
    } else if (action == XR_COMFORT) {
        xr.vignette = !xr.vignette;
        quest_comfort_store(xr.vignette);
    } else if (action == XR_EXIT) {
        save_game();
        quest_exit();
    } else if (action == XR_NEW_CONFIRM) {
        sc_web_action(CLIENT_NEW_SHIP);
        quest_open(XR_HOME);
        xr.transition = 1;
    } else {
        sc_web_action(action);
        xr.info_page = 0;
        bool gameplay = (action >= CLIENT_PSIONIC_FIRST && action <= 63) ||
                        action == CLIENT_PROGRAM_CAST_FIRST || action == 205;
        if (gameplay)
            quest_resume();
        else
            set_pause(true);
    }
}
EMSCRIPTEN_KEEPALIVE float *sc_xr_buffer(void) {
    return xr.packet;
}
EMSCRIPTEN_KEEPALIVE void sc_xr_settings(int vignette) {
    xr.vignette = vignette != 0;
}
EMSCRIPTEN_KEEPALIVE int sc_xr_begin(void) {
    if (!app.running || quest_running)
        return 0;
    bool vignette = xr.vignette;
    memset(&xr, 0, sizeof(xr));
    xr.vignette = vignette;
    quest_tracking_init(&xr.view.tracking);
    quest_running = true;
    xr.suspended = true;
    xr.transition = 1;
    app.started = true;
    app.options = false;
    if (app.game.player_count > 1)
        game_join(&app.game);
    set_pause(true);
    emscripten_pause_main_loop();
    return 1;
}
EMSCRIPTEN_KEEPALIVE void sc_xr_end(void) {
    if (!quest_running)
        return;
    save_game();
    set_pause(true);
    quest_actions = (Actions){0};
    quest_running = false;
    xr.view.active = false;
    app.last = (double)SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
    if (!quest_graphics_lost) {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        emscripten_resume_main_loop();
    }
}
EMSCRIPTEN_KEEPALIVE void sc_xr_context_lost(void) {
    sc_xr_suspend();
    quest_graphics_lost = true;
    set_pause(true);
    save_game();
    emscripten_pause_main_loop();
}
EMSCRIPTEN_KEEPALIVE int sc_xr_import(void) {
    if (quest_running)
        return 0;
    bool loaded = game_load(&app.game, "/quest-import.sav");
    remove("/quest-import.sav");
    if (!loaded)
        return 0;
    app.renderer.world_revision = 0;
    set_pause(true);
    save_game();
    return 1;
}
EMSCRIPTEN_KEEPALIVE int sc_xr_frame(double time_ms, unsigned valid, unsigned buttons, double lx,
                                     double ly, double rx, double ry) {
    if (!quest_running || !isfinite(time_ms))
        return 0;
    QuestView *q = &xr.view;
    Player *p = &app.game.players[0];
    Vec3 before = quest_base();
    bool was_matrix = app.game.matrix.divers[0].active, was_sheltered = p->sheltered;
    uint32_t was_riding = p->riding, respawns = app.game.respawns;
    quest_dt = xr.timestamp ? clampd((time_ms - xr.timestamp) / 1000, 0, .05) : 0;
    xr.timestamp = time_ms;
    xr.tracking = quest_tracking_update(&q->tracking, xr.packet, valid);
    if (!xr.tracking || !isfinite(lx) || !isfinite(ly) || !isfinite(rx) || !isfinite(ry)) {
        xr.tracking = false;
        sc_xr_suspend();
        /* Calibrate/draw from the head alone, including the very first frame.
           Missing controllers must never prevent a visible recovery panel. */
        if (!(q->tracking.valid & (1u << QUEST_HEAD))) {
            xr.buttons = buttons;
            return 0;
        }
    }
    if (!q->tracking.calibrated && !quest_calibrate(&q->tracking, p->yaw))
        return 0;
    q->base = quest_base();
    q->head = quest_position(&q->tracking, QUEST_HEAD, q->base);
    if (!xr.anchor)
        quest_anchor();
    unsigned pressed = buttons & ~xr.buttons;
    bool neutral = buttons == 0 && fabs(lx) < .2 && fabs(ly) < .2 && fabs(rx) < .2 && fabs(ry) < .2;
    quest_actions = (Actions){0};
    if (xr.suspended && (pressed & QUEST_CENTER)) {
        quest_calibrate(&q->tracking, p->yaw);
        xr.anchor = false;
    }
    if (xr.suspended && xr.tracking) {
        if (neutral)
            xr.neutral = true;
        if (xr.neutral && (!xr.confirm || (pressed & QUEST_A)))
            quest_resume();
    } else if (xr.tracking) {
        if (pressed & QUEST_CENTER) {
            quest_calibrate(&q->tracking, p->yaw);
            xr.anchor = false;
            xr.transition = 1;
        }
        if (pressed & QUEST_Y) {
            if (app.paused)
                quest_resume();
            else
                quest_open(XR_HOME);
        } else if (app.paused) {
            quest_menu();
            if (fabs(ry) < .25)
                xr.menu_latched = false;
            if (fabs(ry) > .65 && !xr.menu_latched) {
                xr.selection += ry > 0 ? 1 : -1;
                xr.menu_latched = true;
                quest_menu();
            }
            if (pressed & QUEST_B)
                quest_open(XR_HOME);
            else if (pressed & (QUEST_A | QUEST_FIRE))
                quest_activate();
        } else {
            if (quest_snap(&q->tracking, rx))
                xr.transition = 1;
            if (pressed & QUEST_USE)
                interact_player(0);
            if (pressed & QUEST_X)
                sc_web_action(buttons & QUEST_WRIST ? 10 : 9);
            if (pressed & QUEST_LAMP)
                sc_web_action(CLIENT_LAMP);
            if (app.paused) {
                quest_open(app.game.matrix.divers[0].choice ? XR_MATRIX
                           : app.game.campaign.choice[0]    ? XR_BUILD
                           : app.game.politics.audience[0]  ? XR_COURTS
                                                            : XR_STORY);
            } else {
                Actions *a = &quest_actions;
                a->strafe = (float)(fabs(lx) < .18 ? 0 : clampd(lx, -1, 1));
                a->forward = (float)(fabs(ly) < .18 ? 0 : -clampd(ly, -1, 1));
                a->jump = (buttons & QUEST_A) != 0;
                xr.physical_crouch =
                    quest_head_delta(&q->tracking).y < -.45 && !(buttons & QUEST_B);
                a->crouch = (buttons & QUEST_B) != 0 || xr.physical_crouch;
                a->lift_up = (buttons & QUEST_A) != 0;
                a->lift_down = (buttons & QUEST_B) != 0;
                a->sprint = (buttons & QUEST_SPRINT) != 0;
                a->fire = (buttons & QUEST_FIRE) != 0;
                a->aimed = true;
                a->aim_offset = quest_position(&q->tracking, QUEST_AIM_RIGHT, v3(0, 0, 0));
                if (xr.physical_crouch && !p->riding && !app.game.matrix.divers[0].active)
                    a->aim_offset.y += .77;
                a->aim_direction = quest_forward(&q->tracking, QUEST_AIM_RIGHT);
            }
        }
    }
    xr.buttons = buttons;
    if (was_matrix != app.game.matrix.divers[0].active || was_sheltered != p->sheltered ||
        was_riding != p->riding || vlen(vsub(before, quest_base())) > 5) {
        quest_calibrate(&q->tracking, p->yaw);
        xr.transition = 1;
        xr.anchor = false;
        q->base = quest_base();
        q->head = quest_position(&q->tracking, QUEST_HEAD, q->base);
        quest_actions = (Actions){0};
    }
    Vec3 forward = quest_forward(&q->tracking, QUEST_HEAD);
    double yaw = atan2(forward.x, forward.z);
    /* Save/body look retains the established range; eye rendering uses the
       complete tracked pose, including looking straight up or down. */
    app_aim(0, yaw, clampd(atan2(forward.y, hypot(forward.x, forward.z)), -1.48, 1.48));
    q->unsafe = !quest_lean_safe(&q->tracking);
    Vec3 lean = vsub(q->head, q->base);
    double reach = vlen(lean);
    if (!app.game.matrix.divers[0].active && !p->sheltered && reach > .01 &&
        world_raycast(&app.game.world, q->base, vmul(lean, 1 / reach), reach + .12) < reach + .1)
        q->unsafe = true;
    if (p->sheltered &&
        (fabs(q->head.x) > 8.85 || fabs(q->head.z) > 7.85 || q->head.y < .12 || q->head.y > 3.8))
        q->unsafe = true;
    if (app.game.matrix.divers[0].active && reach > .01) {
        const MatrixNetwork *network = &app.game.matrix.networks[app.game.matrix.divers[0].network];
        const MatrixRecord *record = &app.game.matrix.records[network->record];
        if (matrix_raycast(network, record, q->base, vmul(lean, 1 / reach), reach + .12) <
            reach + .1)
            q->unsafe = true;
    }
    if (q->unsafe)
        quest_actions = (Actions){0};
    before = quest_base();
    was_matrix = app.game.matrix.divers[0].active;
    was_sheltered = p->sheltered;
    was_riding = p->riding;
    frame(NULL);
    q->base = quest_base();
    q->head = quest_position(&q->tracking, QUEST_HEAD, q->base);
    if (was_matrix != app.game.matrix.divers[0].active || was_sheltered != p->sheltered ||
        was_riding != p->riding || respawns != app.game.respawns ||
        vlen(vsub(before, q->base)) > 5) {
        quest_calibrate(&q->tracking, p->yaw);
        xr.transition = 1;
        xr.anchor = false;
        q->base = quest_base();
        q->head = quest_position(&q->tracking, QUEST_HEAD, q->base);
    }
    if (app.paused && !xr.anchor)
        quest_anchor();
    q->active = true;
    q->menu = app.paused;
    q->recovery = xr.suspended;
    q->controllers = xr.tracking;
    q->wrist = xr.tracking && (buttons & QUEST_WRIST) != 0;
    q->vignette = xr.vignette;
    q->movement = (float)fmin(1, hypot(quest_actions.forward, quest_actions.strafe));
    if (p->lift_active || p->riding)
        q->movement = fmaxf(q->movement, .8f);
    q->fade = xr.transition;
    xr.transition = fmaxf(0, xr.transition - (float)quest_dt * 5);
    if (q->menu)
        quest_menu();
    if (!q->recovery)
        renderer_quest_prepare(&app.renderer, &app.game);
    xr.frames++;
    /* 1: head-tracked recovery panel, 2: all required poses usable. */
    return app.running ? (xr.tracking ? 2 : 1) : 0;
}
EMSCRIPTEN_KEEPALIVE int sc_xr_eye(unsigned framebuffer, int x, int y, int width, int height) {
    if (!quest_running || !xr.view.active)
        return 0;
    bool ok =
        renderer_quest_eye(&app.renderer, &app.game, &xr.view, framebuffer, x, y, width, height,
                           xr.packet + SC_QUEST_POSES * 16, xr.packet + SC_QUEST_POSES * 16 + 16);
    if (ok)
        xr.eyes++;
    return ok;
}
EMSCRIPTEN_KEEPALIVE const char *sc_xr_state(void) {
    static char text[640];
    Vec3 aim = quest_forward(&xr.view.tracking, QUEST_AIM_RIGHT);
    snprintf(text, sizeof(text),
             "{\"active\":%s,\"tracking\":%s,\"suspended\":%s,"
             "\"neutral\":%s,\"valid\":%u,\"frames\":%u,\"eyes\":%u,\"menu\":%d,\"selected\":%d,"
             "\"label\":\"%s\",\"unsafe\":%s,\"vignette\":%s,\"fade\":%.3f,"
             "\"aim\":[%.5f,%.5f,%.5f]}",
             quest_running ? "true" : "false", xr.tracking ? "true" : "false",
             xr.suspended ? "true" : "false", xr.neutral ? "true" : "false", xr.view.tracking.valid,
             xr.frames, xr.eyes, app.paused ? xr.page : -1, xr.selection,
             xr.count ? xr.labels[xr.selection] : "", xr.view.unsafe ? "true" : "false",
             xr.vignette ? "true" : "false", xr.view.fade, aim.x, aim.y, aim.z);
    return text;
}

#endif
