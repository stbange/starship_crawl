#include "app_internal.h"
#include "client_actions.h"

void open_pad(SDL_JoystickID id) {
    input_devices_add(&app.devices, id, app.game.player_count);
}
static int pad_player(int slot) {
    input_devices_players(&app.devices, app.game.player_count);
    return app.devices.owner[slot] >= 0 ? app.devices.owner[slot] : 0;
}
static float axis(SDL_Gamepad *pad, SDL_GamepadAxis axis_id, int player) {
    return input_axis(SDL_GetGamepadAxis(pad, axis_id),
                      app.renderer.preferences.view[player].deadzone);
}
static int finger_role(float x, float y) {
    const float xs[] = {.90f, .77f, .64f, .90f, .77f, .64f, .90f, .77f, .64f, .51f, .51f, .51f},
                ys[] = {.72f, .85f, .85f, .48f, .48f, .48f, .27f, .27f, .27f, .27f, .48f, .1f};
    int w, h;
    SDL_GetWindowSize(app.window, &w, &h);
    float radius = fminf((float)w, (float)h) * .075f;
    if ((app.game.views[0].map_open || app.game.views[0].matrix_map))
        return fabsf((x - .51f) * w) < radius && fabsf((y - .1f) * h) < radius ? 13 : -1;
    for (int i = 0; i < 12; i++)
        if (!(onboarding_active(&app.game) && !app.game.matrix.divers[0].active &&
              ((i == 7 && !onboarding_action(&app.game, 0, 9)) ||
               (i == 8 && !onboarding_action(&app.game, 0, 13)) ||
               ((i == 9 || i == 10) && !(onboarding_capabilities(&app.game, 0) & OB_POWERS)))) &&
            !(app.game.matrix.divers[0].active && onboarding_matrix_intro(&app.game, 0) && i != 6 &&
              i != 8) &&
            fabsf((x - xs[i]) * w) < radius && fabsf((y - ys[i]) * h) < radius)
            return i + 2;
    if (x < .42f)
        return 0;
    return 1;
}
static void touch_event(const SDL_TouchFingerEvent *e, Uint32 type) {
    app.touch = true;
    if (type == SDL_EVENT_FINGER_DOWN) {
        if (app.paused) {
            sc_web_start();
            return;
        }
        for (int i = 0; i < 10; i++)
            if (!app.fingers[i].active) {
                app.fingers[i] =
                    (Finger){e->fingerID, true, finger_role(e->x, e->y), e->x, e->y, e->x, e->y};
                if (app.game.matrix.divers[0].active &&
                    (app.fingers[i].role == 5 || app.fingers[i].role == 6))
                    app_command(0, CMD_MATRIX_CYCLE, app.fingers[i].role == 5 ? 1 : -1, 0);
                if (app.fingers[i].role == 7)
                    sc_web_action(CLIENT_LAMP);
                if (app.fingers[i].role == 8)
                    interact_player(0);
                if (app.fingers[i].role == 9)
                    sc_web_action(CLIENT_WEAPON_NEXT);
                if (app.fingers[i].role == 10)
                    sc_web_action(CLIENT_SHELTER);
                if (app.fingers[i].role == 11) {
                    if (app.game.matrix.divers[0].active)
                        app_command(0, CMD_MATRIX_CAST, app.game.matrix.divers[0].selected, 0);
                    else
                        app_command(0, CMD_PSIONIC, (Psionic)app.game.players[0].selected_power, 0);
                }
                if (app.fingers[i].role == 12 && app.game.matrix.divers[0].active)
                    app_command(0, CMD_MATRIX_SELECT, 1 - app.game.matrix.divers[0].selected, 0);
                else if (app.fingers[i].role == 12)
                    app.game.players[0].selected_power =
                        (app.game.players[0].selected_power + 1) % SC_PSIONICS;
                if (app.fingers[i].role == 13)
                    game_map_toggle(&app.game, 0);
                break;
            }
    } else
        for (int i = 0; i < 10; i++)
            if (app.fingers[i].active && app.fingers[i].id == e->fingerID) {
                Finger *f = &app.fingers[i];
                if (type == SDL_EVENT_FINGER_UP || type == SDL_EVENT_FINGER_CANCELED)
                    f->active = false;
                else {
                    if (f->role == 1 && !app.paused && !app.game.views[0].map_open &&
                        !app.game.views[0].matrix_map)
                        app_look(0, (e->x - f->x) * 3.5 * app.renderer.preferences.view[0].touch,
                                 (e->y - f->y) * 2.8 * app.renderer.preferences.view[0].touch *
                                     (app.renderer.preferences.view[0].inverted ? 1 : -1));
                    f->x = e->x;
                    f->y = e->y;
                }
                break;
            }
}
void events(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_EVENT_QUIT:
            app.running = false;
            break;
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            set_pause(true);
            break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        case SDL_EVENT_DISPLAY_ORIENTATION:
            sc_web_reset_input();
            break;
        case SDL_EVENT_MOUSE_MOTION:
#ifndef __EMSCRIPTEN__
            if (!app.paused && e.motion.which != SDL_TOUCH_MOUSEID &&
                e.motion.timestamp >= app.look_after) {
                if (app.discard_look) {
                    app.discard_look = false;
                    break;
                }
                const ViewPreferences *v = &app.renderer.preferences.view[0];
                app_look(0, e.motion.xrel * .0023 * v->mouse,
                         e.motion.yrel * .0023 * v->mouse * (v->inverted ? 1 : -1));
            }
#endif
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (e.button.which == SDL_TOUCH_MOUSEID)
                break;
            if (app.paused && !app.new_armed)
                sc_web_start();
            else if (e.button.button == SDL_BUTTON_LEFT)
                app.mouse_fire = true;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (e.button.button == SDL_BUTTON_LEFT)
                app.mouse_fire = false;
            break;
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_MOTION:
        case SDL_EVENT_FINGER_CANCELED:
            touch_event(&e.tfinger, e.type);
            break;
        case SDL_EVENT_GAMEPAD_ADDED:
            open_pad(e.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            if (input_devices_remove(&app.devices, e.gdevice.which)) {
                set_pause(true);
                game_message(&app.game,
                             "CONTROLLER DISCONNECTED / OTHER PLAYER KEEPS THEIR CONTROLS");
            }
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            for (int i = 0; i < 2; i++)
                if (app.devices.pads[i] &&
                    SDL_GetGamepadID(app.devices.pads[i]) == e.gbutton.which) {
                    int player = pad_player(i);
                    Player *p = &app.game.players[player];
                    MatrixDiver *d = &app.game.matrix.divers[player];
                    int mb = e.gbutton.button;
                    if (app.options) {
                        if (mb == SDL_GAMEPAD_BUTTON_DPAD_UP || mb == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                            app.option_row =
                                (app.option_row + (mb == SDL_GAMEPAD_BUTTON_DPAD_UP ? 10 : 1)) % 11;
                        if (mb == SDL_GAMEPAD_BUTTON_DPAD_LEFT ||
                            mb == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                            options_adjust(mb == SDL_GAMEPAD_BUTTON_DPAD_LEFT ? -1 : 1);
                        if (mb == SDL_GAMEPAD_BUTTON_START || mb == SDL_GAMEPAD_BUTTON_SOUTH)
                            sc_web_start();
                        if (mb == SDL_GAMEPAD_BUTTON_BACK || mb == SDL_GAMEPAD_BUTTON_EAST)
                            app.options = false;
                        break;
                    }
                    if (app.paused && !PLAYER_VIEW(&app.game, p).journal &&
                        mb == SDL_GAMEPAD_BUTTON_BACK) {
                        options_open(player);
                        break;
                    }
                    if (PLAYER_VIEW(&app.game, p).journal &&
                        PLAYER_VIEW(&app.game, p).journal_page == 5) {
                        if (mb == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ||
                            mb == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                            app.game.views[player].loadout_slot =
                                (app.game.views[player].loadout_slot +
                                 (mb == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ? 5 : 1)) %
                                6;
                        if (mb == SDL_GAMEPAD_BUTTON_DPAD_LEFT ||
                            mb == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                            app_command(player, CMD_CAMPAIGN_REFIT,
                                        app.game.views[player].loadout_slot,
                                        mb == SDL_GAMEPAD_BUTTON_DPAD_LEFT ? -1 : 1);
                        if (mb == SDL_GAMEPAD_BUTTON_WEST)
                            campaign_track(&app.game);
                        unsigned choice = mb == SDL_GAMEPAD_BUTTON_SOUTH   ? 1
                                          : mb == SDL_GAMEPAD_BUTTON_NORTH ? 2
                                          : mb == SDL_GAMEPAD_BUTTON_EAST  ? 3
                                                                           : 0;
                        if (choice && app_command(player, CMD_CAMPAIGN_CHOOSE, choice, 0))
                            campaign_journal(&app.game, player);
                        if (mb == SDL_GAMEPAD_BUTTON_START)
                            sc_web_start();
                        continue;
                    }
                    if (mb == SDL_GAMEPAD_BUTTON_START &&
                        SDL_GetGamepadAxis(app.devices.pads[i], SDL_GAMEPAD_AXIS_LEFT_TRIGGER) >
                            16000) {
                        phase_journal(player);
                        continue;
                    }
                    if (PLAYER_VIEW(&app.game, p).journal &&
                        PLAYER_VIEW(&app.game, p).journal_page == 4 && !d->choice) {
                        if (mb == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ||
                            mb == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                            app_command(player, CMD_MATRIX_SELECT,
                                        mb == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0);
                        if (mb == SDL_GAMEPAD_BUTTON_DPAD_LEFT ||
                            mb == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) {
                            unsigned program = app.game.matrix.profiles[player].slots[d->selected];
                            for (int step = 0; step < 6; step++) {
                                program =
                                    (program + (mb == SDL_GAMEPAD_BUTTON_DPAD_RIGHT ? 1 : 5)) % 6;
                                if (app_command(player, CMD_MATRIX_EQUIP, program, 0))
                                    break;
                            }
                        }
                        if (mb == SDL_GAMEPAD_BUTTON_DPAD_UP)
                            app_command(player, CMD_MATRIX_UPGRADE, 0, 0);
                        if (mb == SDL_GAMEPAD_BUTTON_EAST || mb == SDL_GAMEPAD_BUTTON_START)
                            sc_web_start();
                        continue;
                    }
                    if (mb == SDL_GAMEPAD_BUTTON_RIGHT_STICK &&
                        SDL_GetGamepadAxis(app.devices.pads[i], SDL_GAMEPAD_AXIS_LEFT_TRIGGER) >
                            16000) {
                        matrix_journal(player);
                        continue;
                    }
                    if (d->active) {
                        int b = e.gbutton.button;
                        if (d->choice) {
                            unsigned choice = b == SDL_GAMEPAD_BUTTON_DPAD_LEFT    ? 1
                                              : b == SDL_GAMEPAD_BUTTON_DPAD_UP    ? 2
                                              : b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT ? 3
                                                                                   : 0;
                            if (choice && app_command(player, CMD_MATRIX_CHOOSE, choice, 0))
                                set_pause(false);
                            if (b == SDL_GAMEPAD_BUTTON_START || b == SDL_GAMEPAD_BUTTON_EAST)
                                sc_web_start();
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_START) {
                            set_pause(!app.paused);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_BACK) {
                            app_command(player, CMD_MATRIX_TOGGLE, 0, 0);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_LEFT_STICK) {
                            game_map_toggle(&app.game, player);
                            continue;
                        }
                        if (DIVER_VIEW(&app.game, d).matrix_map)
                            continue;
                        if (b == SDL_GAMEPAD_BUTTON_WEST)
                            app_command(player, CMD_MATRIX_SERVICE, 0, 0);
                        if (b == SDL_GAMEPAD_BUTTON_NORTH)
                            app_command(player, CMD_MATRIX_CAST, d->selected, 0);
                        if (b == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ||
                            b == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                            app_command(player, CMD_MATRIX_SELECT,
                                        b == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0);
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_LEFT || b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                            app_command(player, CMD_MATRIX_CYCLE,
                                        b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT ? 1 : -1, 0);
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                            interact_player(player);
                        continue;
                    }
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_BACK &&
                        matrix_near(&app.game, player) && !app.paused) {
                        app_command(player, CMD_MATRIX_TOGGLE, 0, 0);
                        continue;
                    }
                    if (PLAYER_VIEW(&app.game, p).journal &&
                        PLAYER_VIEW(&app.game, p).journal_page == 7) {
                        if (mb == SDL_GAMEPAD_BUTTON_EAST || mb == SDL_GAMEPAD_BUTTON_START)
                            sc_web_start();
                        continue;
                    }
                    if (PLAYER_VIEW(&app.game, p).journal &&
                        PLAYER_VIEW(&app.game, p).journal_page && app.game.story.version) {
                        int b = e.gbutton.button;
                        unsigned choice = b == SDL_GAMEPAD_BUTTON_DPAD_LEFT    ? 1
                                          : b == SDL_GAMEPAD_BUTTON_DPAD_UP    ? 2
                                          : b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT ? 3
                                                                               : 0;
                        if ((PLAYER_VIEW(&app.game, p).journal_page == 1 ||
                             PLAYER_VIEW(&app.game, p).journal_page == 6) &&
                            choice && choose_player(player, choice))
                            set_pause(false);
                        if (b == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) {
                            do {
                                PLAYER_VIEW(&app.game, p).journal_page =
                                    PLAYER_VIEW(&app.game, p).journal_page == 6 ? 1
                                    : PLAYER_VIEW(&app.game, p).journal_page == 3
                                        ? (app.game.politics.version ? 6 : 1)
                                        : PLAYER_VIEW(&app.game, p).journal_page + 1;
                            } while ((PLAYER_VIEW(&app.game, p).journal_page == 2 &&
                                      !onboarding_action(&app.game, player, 85)) ||
                                     (PLAYER_VIEW(&app.game, p).journal_page == 3 &&
                                      !onboarding_action(&app.game, player, 86)));
                        }
                        if (b == SDL_GAMEPAD_BUTTON_WEST) {
                            if (PLAYER_VIEW(&app.game, p).journal_page == 6)
                                politics_read(&app.game, player);
                            else if (PLAYER_VIEW(&app.game, p).journal_page == 3)
                                game_story_event_read(&app.game, player);
                            else
                                game_story_read(&app.game, player);
                        }
                        if (b == SDL_GAMEPAD_BUTTON_NORTH &&
                            PLAYER_VIEW(&app.game, p).journal_page == 6)
                            politics_track(&app.game, player);
                        else if (b == SDL_GAMEPAD_BUTTON_NORTH) {
                            game_story_track(&app.game);
                            if (app.game.campaign.version)
                                app.game.campaign.tracked = SC_GUARDIANS;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_EAST || b == SDL_GAMEPAD_BUTTON_START)
                            sc_web_start();
                        continue;
                    }
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_STICK) {
                        game_map_toggle(&app.game, player);
                        continue;
                    }
                    if (PLAYER_VIEW(&app.game, p).map_open) {
                        int b = e.gbutton.button;
                        if (b == SDL_GAMEPAD_BUTTON_BACK)
                            game_map_toggle(&app.game, player);
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_LEFT)
                            game_map_pan(&app.game, player, -1, 0, 0);
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                            game_map_pan(&app.game, player, 1, 0, 0);
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_UP)
                            game_map_pan(&app.game, player, 0, 0, 1);
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                            game_map_pan(&app.game, player, 0, 0, -1);
                        if (b == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                            game_map_pan(&app.game, player, 0, 1, 0);
                        if (b == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)
                            game_map_pan(&app.game, player, 0, -1, 0);
                        if (b != SDL_GAMEPAD_BUTTON_START)
                            continue;
                    }
                    if (SDL_GetGamepadAxis(app.devices.pads[i], SDL_GAMEPAD_AXIS_LEFT_TRIGGER) >
                        16000) {
                        int b = e.gbutton.button;
                        if (b == SDL_GAMEPAD_BUTTON_BACK) {
                            PLAYER_VIEW(&app.game, p).journal = true;
                            PLAYER_VIEW(&app.game, p).journal_page = app.game.story.version ? 1 : 0;
                            set_pause(true);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_SOUTH || b == SDL_GAMEPAD_BUTTON_EAST) {
                            app_command(player, CMD_RIG_TOGGLE,
                                        b == SDL_GAMEPAD_BUTTON_SOUTH ? MECH : WAGON, 0);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_WEST) {
                            app_command(player, CMD_WEAPON_SELECT, SC_FLARE_CANNON, 0);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_LEFT) {
                            app_command(player, CMD_PSIONIC, PSI_VEIL, 0);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_UP) {
                            app_command(player, CMD_PSIONIC, PSI_FLIGHT, 0);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) {
                            app_command(player, CMD_PSIONIC, PSI_LEAP, 0);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
                            app_command(player, CMD_PSIONIC, PSI_NOVA, 0);
                            continue;
                        }
                    }
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START) {
                        app.started = true;
                        set_pause(!app.paused);
                    }
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_BACK)
                        app_command(pad_player(i), CMD_SHELTER, 0, 0);
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                        app_command(pad_player(i), CMD_WEAPON_CYCLE, 1, 0);
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                        interact_player(pad_player(i));
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_STICK)
                        app_command(pad_player(i), CMD_FLASHLIGHT, 0, 0);
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT)
                        app_command(pad_player(i), CMD_UPGRADE, 0, 0);
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
                        app_command(pad_player(i), CMD_UPGRADE, 1, 0);
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                        app_command(pad_player(i), CMD_UPGRADE, 2, 0);
                }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            if (e.wheel.y != 0) {
                if (app.game.matrix.divers[0].active)
                    app_command(0, CMD_MATRIX_SELECT, e.wheel.y > 0 ? 1 : 0, 0);
                else
                    app_command(0, CMD_WEAPON_CYCLE, e.wheel.y > 0 ? 1 : -1, 0);
            }
            break;
        case SDL_EVENT_KEY_DOWN:
            if (e.key.repeat)
                break;
            if (e.key.key == SDLK_F1) {
                if (app.options)
                    sc_web_start();
                else
                    options_open(0);
                break;
            }
            if (app.options) {
                if (e.key.key == SDLK_UP || e.key.key == SDLK_DOWN)
                    app.option_row = (app.option_row + (e.key.key == SDLK_UP ? 10 : 1)) % 11;
                if (e.key.key == SDLK_LEFT || e.key.key == SDLK_RIGHT)
                    options_adjust(e.key.key == SDLK_LEFT ? -1 : 1);
                if (e.key.key == SDLK_RETURN)
                    sc_web_start();
                if (e.key.key == SDLK_ESCAPE)
                    app.options = false;
                break;
            }
            {
                Player *p = &app.game.players[0];
                MatrixDiver *d = &app.game.matrix.divers[0];
                SDL_Keycode key = e.key.key;
                if (PLAYER_VIEW(&app.game, p).journal &&
                    PLAYER_VIEW(&app.game, p).journal_page == 6) {
                    if (key == SDLK_L || key == SDLK_U) {
                        sc_web_action(key == SDLK_L ? 231 : 232);
                        break;
                    }
                }
                if (key == SDLK_F10) {
                    phase_journal(0);
                    break;
                }
                if (PLAYER_VIEW(&app.game, p).journal &&
                    PLAYER_VIEW(&app.game, p).journal_page == 5) {
                    if (key == SDLK_Q || key == SDLK_E) {
                        sc_web_action(key == SDLK_Q ? 221 : 222);
                        break;
                    }
                    if (key == SDLK_Z || key == SDLK_X) {
                        sc_web_action(key == SDLK_Z ? 223 : 224);
                        break;
                    }
                    if (key == SDLK_U) {
                        sc_web_action(CLIENT_TRACK_PHASE);
                        break;
                    }
                    if (key >= SDLK_1 && key <= SDLK_3) {
                        sc_web_action(226 + (int)(key - SDLK_1));
                        break;
                    }
                }
                if (key == SDLK_F8) {
                    matrix_journal(0);
                    break;
                }
                if (key == SDLK_C && !app.paused) {
                    app_command(0, CMD_MATRIX_TOGGLE, 0, 0);
                    break;
                }
                if (PLAYER_VIEW(&app.game, p).journal &&
                    PLAYER_VIEW(&app.game, p).journal_page == 4 && !d->choice) {
                    if (key >= SDLK_1 && key <= SDLK_6) {
                        app_command(0, CMD_MATRIX_EQUIP, (unsigned)(key - SDLK_1), 0);
                        break;
                    }
                    if (key == SDLK_Z || key == SDLK_X) {
                        app_command(0, CMD_MATRIX_SELECT, key == SDLK_X, 0);
                        break;
                    }
                    if (key == SDLK_O) {
                        app_command(0, CMD_MATRIX_UPGRADE, 0, 0);
                        break;
                    }
                }
                if (d->active) {
                    if (d->choice && key >= SDLK_1 && key <= SDLK_3) {
                        if (app_command(0, CMD_MATRIX_CHOOSE, (unsigned)(key - SDLK_1 + 1), 0))
                            set_pause(false);
                        break;
                    }
                    if (key == SDLK_C || key == SDLK_R) {
                        app_command(0, CMD_MATRIX_TOGGLE, 0, 0);
                        break;
                    }
                    if (key == SDLK_F) {
                        app_command(0, CMD_MATRIX_SERVICE, 0, 0);
                        break;
                    }
                    if (key == SDLK_Q || key == SDLK_E) {
                        app_command(0, CMD_MATRIX_CYCLE, key == SDLK_E ? 1 : -1, 0);
                        break;
                    }
                    if (key == SDLK_Z || key == SDLK_X) {
                        app_command(0, CMD_MATRIX_SELECT, key == SDLK_X, 0);
                        break;
                    }
                    if (key == SDLK_V) {
                        app_command(0, CMD_MATRIX_CAST, d->selected, 0);
                        break;
                    }
                    if (key == SDLK_O) {
                        app_command(0, CMD_MATRIX_UPGRADE, 0, 0);
                        break;
                    }
                    if (key == SDLK_1 || key == SDLK_2) {
                        app_command(0, CMD_MATRIX_CAST, (unsigned)(key - SDLK_1), 0);
                        break;
                    }
                    if (key == SDLK_ESCAPE && DIVER_VIEW(&app.game, d).matrix_map) {
                        game_map_toggle(&app.game, 0);
                        break;
                    }
                    if (key == SDLK_T || key == SDLK_Y || key == SDLK_K || key == SDLK_P ||
                        key == SDLK_B || key == SDLK_J || key == SDLK_H ||
                        (key >= SDLK_3 && key <= SDLK_9) || key == SDLK_0)
                        break;
                }
            }
            switch (e.key.key) {
            case SDLK_ESCAPE:
                if ((app.game.views[0].map_open || app.game.views[0].matrix_map)) {
                    game_map_toggle(&app.game, 0);
                    break;
                }
                if (app.new_armed) {
                    app.new_armed = false;
                    game_message(&app.game, "NEW EXPEDITION CANCELLED");
                } else
                    set_pause(!app.paused);
                break;
            case SDLK_RETURN:
                if (app.new_armed) {
                    game_new_ship(&app.game, new_seed());
                    app.renderer.world_revision = 0;
                    save_game();
                }
                sc_web_start();
                break;
            case SDLK_TAB:
                set_pause(!app.paused);
                break;
            case SDLK_F2:
                sc_web_action(CLIENT_LOCAL_PARTY);
                break;
            case SDLK_L:
                sc_web_action(CLIENT_JOURNAL_NEXT);
                break;
            case SDLK_M:
                game_map_toggle(&app.game, 0);
                break;
            case SDLK_V:
                app_command(0, CMD_PSIONIC, PSI_VEIL, 0);
                break;
            case SDLK_B:
                app_command(0, CMD_PSIONIC, PSI_FLIGHT, 0);
                break;
            case SDLK_J:
                app_command(0, CMD_PSIONIC, PSI_LEAP, 0);
                break;
            case SDLK_H:
                app_command(0, CMD_PSIONIC, PSI_NOVA, 0);
                break;
            case SDLK_LEFT:
                game_map_pan(&app.game, 0, -1, 0, 0);
                break;
            case SDLK_RIGHT:
                game_map_pan(&app.game, 0, 1, 0, 0);
                break;
            case SDLK_UP:
                game_map_pan(&app.game, 0, 0, 0, 1);
                break;
            case SDLK_DOWN:
                game_map_pan(&app.game, 0, 0, 0, -1);
                break;
            case SDLK_PAGEUP:
                game_map_pan(&app.game, 0, 0, 1, 0);
                break;
            case SDLK_PAGEDOWN:
                game_map_pan(&app.game, 0, 0, -1, 0);
                break;
            case SDLK_HOME:
                if ((app.game.views[0].map_open || app.game.views[0].matrix_map)) {
                    game_map_toggle(&app.game, 0);
                    game_map_toggle(&app.game, 0);
                }
                break;
            case SDLK_T:
                sc_web_action(CLIENT_MECH);
                break;
            case SDLK_Y:
                sc_web_action(CLIENT_WAGON);
                break;
            case SDLK_K:
                sc_web_action(CLIENT_FLARE);
                break;
            case SDLK_O:
                sc_web_action(CLIENT_FLARE_LIFETIME);
                break;
            case SDLK_P:
                sc_web_action(CLIENT_FLARE_STRENGTH);
                break;
            case SDLK_R:
                app_command(0, CMD_SHELTER, 0, 0);
                break;
            case SDLK_G:
                interact_player(0);
                break;
            case SDLK_Z:
                app_command(0, CMD_WEAPON_CYCLE, -1, 0);
                break;
            case SDLK_X:
                app_command(0, CMD_WEAPON_CYCLE, 1, 0);
                break;
            case SDLK_I:
                sc_web_action(CLIENT_JOURNAL);
                break;
            case SDLK_F7:
                sc_web_action(CLIENT_DEBRIEF);
                break;
            case SDLK_F6:
                sc_web_action(app.game.politics.version && app.game.views[0].journal &&
                                      app.game.views[0].journal_page == 2
                                  ? 230
                                  : 85);
                break;
            case SDLK_F4:
                sc_web_action(CLIENT_CHARACTER);
                break;
            case SDLK_U:
                game_story_track(&app.game);
                if (app.game.campaign.version)
                    app.game.campaign.tracked = SC_GUARDIANS;
                break;
            case SDLK_F:
                app_command(0, CMD_FLASHLIGHT, 0, 0);
                break;
            case SDLK_F3:
                app.debug = !app.debug;
                break;
            case SDLK_F5:
                save_game();
                break;
            case SDLK_SLASH:
                sc_web_action(CLIENT_HELP);
                break;
            case SDLK_BACKSPACE:
                sc_web_action(CLIENT_DISMISS_HINT);
                break;
            case SDLK_F9:
                if (!app.no_save && game_load(&app.game, app.save_path)) {
                    app.renderer.world_revision = 0;
                    set_pause(true);
                } else
                    game_message(&app.game, "NO VALID SAVE FOUND");
                break;
            case SDLK_N:
                set_pause(true);
                app.new_armed = true;
                game_message(&app.game,
                             "NEW SHIP RESETS BOTH CHARACTERS / ENTER CONFIRMS / ESC CANCELS");
                break;
            case SDLK_C:
                sc_web_action(CLIENT_TINT);
                break;
            case SDLK_1:
            case SDLK_2:
            case SDLK_3:
            case SDLK_4:
            case SDLK_5:
            case SDLK_6:
            case SDLK_7:
            case SDLK_8:
            case SDLK_9:
                if (app.game.views[0].journal && app.game.views[0].journal_page) {
                    if (((app.game.views[0].journal_page == 1 &&
                          app.game.interactions[0].decision_open) ||
                         app.game.views[0].journal_page == 6) &&
                        e.key.key <= SDLK_3)
                        sc_web_action(90 + (int)(e.key.key - SDLK_1));
                } else
                    app_command(0, CMD_UPGRADE, (int)(e.key.key - SDLK_1), 0);
                break;
            case SDLK_0:
                app_command(0, CMD_UPGRADE, 9, 0);
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
}
void read_actions(Actions actions[SC_LOCAL_VIEWS], double dt) {
    memset(actions, 0, SC_LOCAL_VIEWS * sizeof(*actions));
#ifdef SC_WEBXR
    if (quest_running) {
        actions[0] = quest_actions;
        return;
    }
#endif
    input_devices_players(&app.devices, app.game.player_count);
    const bool *k = SDL_GetKeyboardState(NULL);
    Actions *a = &actions[0];
    a->forward = (float)(k[SDL_SCANCODE_W] - k[SDL_SCANCODE_S]);
    a->strafe = (float)(k[SDL_SCANCODE_D] - k[SDL_SCANCODE_A]);
    a->jump = k[SDL_SCANCODE_SPACE];
    a->crouch = k[SDL_SCANCODE_LCTRL];
    a->sprint = k[SDL_SCANCODE_LSHIFT];
    a->fire = app.mouse_fire;
    a->lift_up = k[SDL_SCANCODE_E];
    a->lift_down = k[SDL_SCANCODE_Q];
    for (int i = 0; i < 10; i++)
        if (app.fingers[i].active) {
            Finger *f = &app.fingers[i];
            switch (f->role) {
            case 0:
                a->strafe = (float)clampd((f->x - f->start_x) * 9, -1, 1);
                a->forward = (float)clampd((f->start_y - f->y) * 9, -1, 1);
                break;
            case 2:
                a->fire = true;
                break;
            case 3:
                a->jump = true;
                break;
            case 4:
                a->crouch = true;
                break;
            case 5:
                a->lift_up = true;
                break;
            case 6:
                a->lift_down = true;
                break;
            default:
                break;
            }
        }
    for (int i = 0; i < 2; i++)
        if (app.devices.pads[i]) {
            if (app.game.player_count == 1 && i == 1 && app.devices.pads[0])
                continue;
            SDL_Gamepad *pad = app.devices.pads[i];
            int player = pad_player(i);
            a = &actions[player];
            a->strafe += axis(pad, SDL_GAMEPAD_AXIS_LEFTX, player);
            a->forward -= axis(pad, SDL_GAMEPAD_AXIS_LEFTY, player);
            const ViewPreferences *v = &app.renderer.preferences.view[player];
            app_look(player, axis(pad, SDL_GAMEPAD_AXIS_RIGHTX, player) * dt * 2.3 * v->stick,
                     axis(pad, SDL_GAMEPAD_AXIS_RIGHTY, player) * dt * 1.8 * v->stick *
                         (v->inverted ? 1 : -1));
            a->fire |= SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 8000;
            if (SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) <= 16000) {
                a->jump |= SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH);
                a->crouch |= SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST);
                a->lift_up |= SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_WEST);
                a->lift_down |= SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_NORTH);
            }
            a->sprint |= SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
        }
    for (int i = 0; i < SC_LOCAL_VIEWS && i < app.game.player_count; i++)
        if (app.game.views[i].map_open || app.game.views[i].matrix_map)
            actions[i] = (Actions){0};
}
