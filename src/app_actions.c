#include "app_internal.h"
#include "client_actions.h"

bool app_command(int player, CommandKind kind, int argument, int auxiliary) {
    GameCommand command = {kind, game_player_id(&app.game, player), argument, auxiliary, 0, 0};
    return session_command(&app.session, &command);
}
void app_look(int player, double yaw, double pitch) {
    GameCommand command = {CMD_LOOK, game_player_id(&app.game, player), 0, 0, yaw, pitch};
    session_command(&app.session, &command);
}
void app_aim(int player, double yaw, double pitch) {
    GameCommand command = {CMD_AIM, game_player_id(&app.game, player), 0, 0, yaw, pitch};
    session_command(&app.session, &command);
}

void interact_player(int player) {
    bool reviewing = app.game.views[player].journal;
    app_command(player, CMD_INTERACT, 0, 0);
    if (app.game.views[player].journal)
        set_pause(true);
    else if (reviewing)
        set_pause(false);
}
bool choose_player(int player, unsigned choice) {
    if (app.game.views[player].journal_page == 6)
        return app_command(player, CMD_POLITICS_CHOOSE, choice, 0);
    return app.game.matrix.divers[player].choice ? app_command(player, CMD_MATRIX_CHOOSE, choice, 0)
                                                 : app_command(player, CMD_STORY_CHOOSE, choice, 0);
}
void phase_journal(int player) {
    if (!(onboarding_capabilities(&app.game, player) & OB_BUILD))
        return;
    campaign_journal(&app.game, player);
    set_pause(true);
}
void matrix_journal(int player) {
    if (!(onboarding_capabilities(&app.game, player) & OB_PROGRAMS))
        return;
    Player *p = &app.game.players[player];
    PLAYER_VIEW(&app.game, p).journal = true;
    PLAYER_VIEW(&app.game, p).journal_page = 4;
    set_pause(true);
}
EMSCRIPTEN_KEEPALIVE void sc_web_action(int action) {
    if (action == CLIENT_HELP) {
        app.game.views[0].journal = true;
        app.game.views[0].journal_page = 7;
        set_pause(true);
        return;
    }
    if (action == CLIENT_DISMISS_HINT) {
        app_command(0, CMD_DISMISS_HINT, 0, 0);
        return;
    }
    if (!onboarding_action(&app.game, 0, action))
        return;
    MatrixDiver *d = &app.game.matrix.divers[0];
    if (action >= CLIENT_COURTS && action <= CLIENT_TRACK_COURT) {
        if (!app.game.politics.version)
            return;
        if (action == CLIENT_COURTS) {
            politics_journal(&app.game, 0);
            set_pause(true);
        }
        if (action == CLIENT_COURT_NEXT)
            politics_read(&app.game, 0);
        if (action == CLIENT_TRACK_COURT)
            politics_track(&app.game, 0);
        return;
    }
    if (action >= CLIENT_BUILD && action <= 228) {
        if (action == CLIENT_BUILD)
            phase_journal(0);
        else if (action == CLIENT_LOADOUT_PREVIOUS || action == CLIENT_LOADOUT_NEXT)
            app.game.views[0].loadout_slot =
                (app.game.views[0].loadout_slot + (action == CLIENT_LOADOUT_PREVIOUS ? 5 : 1)) % 6;
        else if (action == CLIENT_REFIT_PREVIOUS || action == CLIENT_REFIT_NEXT)
            app_command(0, CMD_CAMPAIGN_REFIT, app.game.views[0].loadout_slot,
                        action == CLIENT_REFIT_PREVIOUS ? -1 : 1);
        else if (action == CLIENT_TRACK_PHASE)
            campaign_track(&app.game);
        else if (app_command(0, CMD_CAMPAIGN_CHOOSE, (unsigned)(action - CLIENT_TRACK_PHASE), 0))
            campaign_journal(&app.game, 0);
        return;
    }
    if (action == CLIENT_MATRIX_LINK) {
        app_command(0, CMD_MATRIX_TOGGLE, 0, 0);
        return;
    }
    if (action == CLIENT_MATRIX_SERVICE) {
        app_command(0, CMD_MATRIX_SERVICE, 0, 0);
        return;
    }
    if (action == CLIENT_SERVICE_PREVIOUS || action == CLIENT_SERVICE_NEXT) {
        app_command(0, CMD_MATRIX_CYCLE, action == CLIENT_SERVICE_PREVIOUS ? -1 : 1, 0);
        return;
    }
    if (action == CLIENT_PROGRAM_CAST_FIRST || action == 205) {
        app_command(0, CMD_MATRIX_CAST, (unsigned)(action - CLIENT_PROGRAM_CAST_FIRST), 0);
        return;
    }
    if (action == CLIENT_PROGRAMS) {
        matrix_journal(0);
        return;
    }
    if (action == CLIENT_INTERFACE_UPGRADE) {
        app_command(0, CMD_MATRIX_UPGRADE, 0, 0);
        return;
    }
    if (action == CLIENT_PROGRAM_SLOT_FIRST || action == 209) {
        app_command(0, CMD_MATRIX_SELECT, (unsigned)(action - CLIENT_PROGRAM_SLOT_FIRST), 0);
        return;
    }
    if (action >= CLIENT_PROGRAM_EQUIP_FIRST && action < 216) {
        app_command(0, CMD_MATRIX_EQUIP, (unsigned)(action - CLIENT_PROGRAM_EQUIP_FIRST), 0);
        return;
    }
    if (d->active) {
        if (action == CLIENT_JOURNAL || action == CLIENT_MISSION || action == CLIENT_CHARACTER ||
            action == CLIENT_RECORDS || action == CLIENT_DEBRIEF) {
            matrix_journal(0);
            return;
        }
        if (action == CLIENT_JOURNAL_NEXT || (action >= CLIENT_LOG_FIRST && action < 112))
            return;
        if (action == CLIENT_LAMP) {
            app_command(0, CMD_MATRIX_SERVICE, 0, 0);
            return;
        }
        if (action == CLIENT_WEAPON_NEXT || action == CLIENT_WEAPON_PREVIOUS) {
            app_command(0, CMD_MATRIX_SELECT, action == CLIENT_WEAPON_NEXT ? 1 : 0, 0);
            return;
        }
        if (action >= CLIENT_PSIONIC_FIRST && action < 64) {
            app_command(0, CMD_MATRIX_CAST, d->selected, 0);
            return;
        }
        if (action == CLIENT_SHELTER) {
            app_command(0, CMD_MATRIX_TOGGLE, 0, 0);
            return;
        }
        if ((action >= CLIENT_DAMAGE && action <= CLIENT_TINT) ||
            (action >= CLIENT_MECH && action < CLIENT_MAP_PAN_FIRST) ||
            (action >= CLIENT_WEAPON_FIRST && action < CLIENT_PSIONIC_FIRST) ||
            (action >= CLIENT_MECH_WEAPON_FIRST && action < 74))
            return;
    }
    if (action >= CLIENT_CHOICE_FIRST && action <= 92) {
        if (choose_player(0, (unsigned)(action - 89)))
            set_pause(false);
    } else if (action == CLIENT_MISSION || action == CLIENT_CHARACTER || action == CLIENT_RECORDS ||
               action == CLIENT_DEBRIEF) {
        Player *p = &app.game.players[0];
        PLAYER_VIEW(&app.game, p).journal = true;
        PLAYER_VIEW(&app.game, p).journal_page =
            action == CLIENT_DEBRIEF                             ? 3
            : action == CLIENT_RECORDS                           ? 2
            : action == CLIENT_MISSION && app.game.story.version ? 1
                                                                 : 0;
        set_pause(true);
    } else if (action == CLIENT_TRACK_STORY) {
        game_story_track(&app.game);
        if (app.game.campaign.version)
            app.game.campaign.tracked = SC_GUARDIANS;
    } else if (action == CLIENT_EVENT_NEXT)
        game_story_event_read(&app.game, 0);
    else if (action == CLIENT_STORY_NEXT)
        game_story_read(&app.game, 0);
    else if (action >= CLIENT_PSIONIC_FIRST && action < CLIENT_PSIONIC_FIRST + SC_PSIONICS)
        app_command(0, CMD_PSIONIC, (Psionic)(action - CLIENT_PSIONIC_FIRST), 0);
    else if (action >= CLIENT_MECH_WEAPON_FIRST &&
             action < CLIENT_MECH_WEAPON_FIRST + SC_MECH_WEAPONS) {
        app_command(0, CMD_RIG_WEAPON, action - CLIENT_MECH_WEAPON_FIRST, 0);
    } else if (action >= CLIENT_MAP_PAN_FIRST && action < 36) {
        const int pan[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}};
        game_map_pan(&app.game, 0, pan[action - CLIENT_MAP_PAN_FIRST][0],
                     pan[action - CLIENT_MAP_PAN_FIRST][1], pan[action - CLIENT_MAP_PAN_FIRST][2]);
    } else if (action == CLIENT_MAP)
        game_map_toggle(&app.game, 0);
    else if (action >= CLIENT_LOG_FIRST && action < 112) {
        Player *p = &app.game.players[0];
        if (p->logs & (1u << (action - CLIENT_LOG_FIRST))) {
            PLAYER_VIEW(&app.game, p).reading = (uint8_t)(action - CLIENT_LOG_FIRST);
            PLAYER_VIEW(&app.game, p).journal = true;
            PLAYER_VIEW(&app.game, p).journal_page = 0;
            set_pause(true);
        }
    } else if (action == CLIENT_JOURNAL_NEXT) {
        Player *p = &app.game.players[0];
        if (PLAYER_VIEW(&app.game, p).journal && PLAYER_VIEW(&app.game, p).journal_page == 3) {
            game_story_event_read(&app.game, 0);
            return;
        }
        if (PLAYER_VIEW(&app.game, p).journal && PLAYER_VIEW(&app.game, p).journal_page &&
            app.game.story.version) {
            game_story_read(&app.game, 0);
            return;
        }
        for (int i = 0; i < 12; i++) {
            PLAYER_VIEW(&app.game, p).reading = (PLAYER_VIEW(&app.game, p).reading + 1) % 12;
            if (p->logs & (1u << PLAYER_VIEW(&app.game, p).reading))
                break;
        }
    } else if (action >= CLIENT_ATTRIBUTE_FIRST && action < CLIENT_ATTRIBUTE_FIRST + SC_ATTRIBUTES)
        app_command(0, CMD_UPGRADE, action - CLIENT_ATTRIBUTE_FIRST, 0);
    else if (action >= CLIENT_WEAPON_FIRST && action < CLIENT_WEAPON_FIRST + SC_WEAPONS) {
        app_command(0, CMD_WEAPON_SELECT, action - CLIENT_WEAPON_FIRST, 0);
    } else if (action == CLIENT_MECH || action == CLIENT_WAGON)
        app_command(0, CMD_RIG_TOGGLE, action == CLIENT_MECH ? MECH : WAGON, 0);
    else if (action == CLIENT_FLARE) {
        app_command(0, CMD_WEAPON_SELECT, SC_FLARE_CANNON, 0);
    } else if (action == CLIENT_FLARE_LIFETIME || action == CLIENT_FLARE_STRENGTH)
        app_command(0, CMD_FLARE_UPGRADE, action == CLIENT_FLARE_STRENGTH, 0);
    else if (action == CLIENT_SHELTER)
        app_command(0, CMD_SHELTER, 0, 0);
    else if (action == CLIENT_INTERACT)
        interact_player(0);
    else if (action == CLIENT_WEAPON_NEXT)
        app_command(0, CMD_WEAPON_CYCLE, 1, 0);
    else if (action == CLIENT_WEAPON_PREVIOUS)
        app_command(0, CMD_WEAPON_CYCLE, -1, 0);
    else if (action == CLIENT_JOURNAL) {
        Player *p = &app.game.players[0];
        PLAYER_VIEW(&app.game, p).journal =
            !PLAYER_VIEW(&app.game, p).journal ||
            (app.game.story.version && PLAYER_VIEW(&app.game, p).journal_page != 1);
        PLAYER_VIEW(&app.game, p).journal_page = app.game.story.version ? 1 : 0;
        PLAYER_INTERACTION(&app.game, p).decision_open = false;
        set_pause(PLAYER_VIEW(&app.game, p).journal);
    } else if (action >= CLIENT_DAMAGE && action <= CLIENT_HASTE)
        app_command(0, CMD_UPGRADE, action - CLIENT_DAMAGE, 0);
    else if (action == CLIENT_TINT) {
        app_command(0, CMD_TINT, 0, 0);
    } else if (action == CLIENT_LOCAL_PARTY) {
        if (app.game.player_count == 1)
            game_join(&app.game);
        else
            game_leave(&app.game);
    } else if (action == CLIENT_NEW_SHIP) {
        game_new_ship(&app.game, new_seed());
        app.renderer.world_revision = 0;
        save_game();
    } else if (action == CLIENT_LAMP)
        app_command(0, CMD_FLASHLIGHT, 0, 0);
    else if (action == CLIENT_PAUSE)
        set_pause(true);
}
