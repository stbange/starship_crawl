#include "app_internal.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_JS(void, browser_flush, (void), {
    if (window.scStorage)
        window.scStorage.flush();
})
EM_JS(int, browser_storage_status, (void),
      { return window.scStorage ? window.scStorage.status : 0; })
EM_JS(int, browser_touch, (void), { return navigator.maxTouchPoints > 0; })
EM_JS(void, browser_pointer, (int enabled), {
    if (window.scPointer)
        window.scPointer.set(!!enabled);
})
EM_JS(void, browser_ready, (void), {
    if (window.scReady)
        window.scReady();
})
#else
#define EMSCRIPTEN_KEEPALIVE
#endif
void set_pause(bool paused) {
    app.paused = paused;
    app.mouse_fire = false;
    memset(app.fingers, 0, sizeof(app.fingers));
    app.session.accumulator = 0;
    SDL_ResetKeyboard();
    if (app.window && !app.touch) {
#ifdef __EMSCRIPTEN__
        /* The browser look bridge owns capture. SDL and the HTML menu must not
           issue competing pointer-lock requests when a journal closes. */
        browser_pointer(!paused);
#else
        SDL_SetWindowRelativeMouseMode(app.window, !paused);
#endif
    }
    if (!paused) {
        for (int i = 0; i < SC_PLAYERS; i++) {
            app.game.views[i].journal = false;
            app_command(i, CMD_CLOSE_INTERACTION, 0, 0);
        }
        /* Discard menu motion and the browser's initial pointer-lock warp. */
        SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION);
        app.look_after = SDL_GetTicksNS() + UINT64_C(120000000);
        app.discard_look = true;
    }
}
uint32_t new_seed(void) {
    return world_hash((uint32_t)time(NULL), (Cell){(int32_t)(SDL_GetTicks() % 1000000), 0, 0},
                      app.game.world.seed + 13);
}
void save_game(void) {
    if (app.no_save || !*app.save_path)
        return;
    if (!game_save(&app.game, app.save_path)) {
        snprintf(app.save_status, sizeof(app.save_status), "SAVE FAILED / PREVIOUS SAVE RETAINED");
        return;
    }
#ifdef __EMSCRIPTEN__
    browser_flush();
    snprintf(app.save_status, sizeof(app.save_status), "SAVING TO THIS BROWSER...");
#else
    snprintf(app.save_status, sizeof(app.save_status), "EXPEDITION SAVED");
#endif
    app.game.dirty = false;
}
EMSCRIPTEN_KEEPALIVE void sc_web_start(void) {
    app.started = true;
    app.options = false;
    app.new_armed = false;
    set_pause(false);
}
EMSCRIPTEN_KEEPALIVE void sc_web_capture(void) {
    /* Chromium can deliver its centering warp well after the capture promise. */
    SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION);
    app.discard_look = true;
}
EMSCRIPTEN_KEEPALIVE void sc_web_look(double dx, double dy) {
    if (app.paused || app.touch || (app.game.views[0].map_open || app.game.views[0].matrix_map) ||
        !isfinite(dx) || !isfinite(dy) || SDL_GetTicksNS() < app.look_after || (dx == 0 && dy == 0))
        return;
    if (app.discard_look) {
        app.discard_look = false;
        return;
    }
    const ViewPreferences *v = &app.renderer.preferences.view[0];
    app_look(0, dx * .0023 * v->mouse, dy * .0023 * v->mouse * (v->inverted ? 1 : -1));
}
EMSCRIPTEN_KEEPALIVE void sc_web_reset_input(void) {
    if (!app.window)
        return;
    app.mouse_fire = false;
    memset(app.fingers, 0, sizeof(app.fingers));
    SDL_ResetKeyboard();
    SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION);
    app.session.accumulator = 0;
    app.discard_look = true;
}
bool save_preferences(void) {
    if (app.no_save || !*app.preferences_path)
        return false;
    if (!preferences_save(&app.renderer.preferences, app.preferences_path)) {
        game_message(&app.game, "SETTINGS APPLY THIS SESSION / COULD NOT SAVE DEVICE PREFERENCES");
        return false;
    }
#ifdef __EMSCRIPTEN__
    browser_flush();
#endif
    return true;
}
EMSCRIPTEN_KEEPALIVE int sc_web_setting(int player, int setting, double value) {
    if (player < 0 || player >= SC_LOCAL_VIEWS)
        return 0;
    if (setting == -1) {
        Preferences defaults;
        preferences_default(&defaults);
        app.renderer.preferences.view[player] = defaults.view[player];
    } else if (!preferences_set(&app.renderer.preferences, player, setting, value))
        return 0;
    return save_preferences() ? 1 : 2;
}
EMSCRIPTEN_KEEPALIVE const char *sc_web_preferences(void) {
    static char result[1024];
    const ViewPreferences *a = &app.renderer.preferences.view[0],
                          *b = &app.renderer.preferences.view[1];
    const Player *p = &app.game.players[1];
    Vec3 v = position_absolute(p->pos);
    snprintf(
        result, sizeof(result),
        "{\"views\":[[%.4f,%.4f,%.4f,%.4f,%u,%u,%u],[%.4f,%.4f,%.4f,%.4f,%u,%u,%u]],"
        "\"pads\":[%d,%d],\"height\":[%d,%d],\"resizes\":[%u,%u],\"options\":%s,\"selected\":%d,"
        "\"p2\":[%.5f,%.5f,%.5f,%.5f,%.5f],\"transit\":%s,\"progress\":%.3f,\"deck\":%d}",
        a->mouse, a->touch, a->stick, a->deadzone, (unsigned)a->inverted, a->quality, a->guidance,
        b->mouse, b->touch, b->stick, b->deadzone, (unsigned)b->inverted, b->quality, b->guidance,
        app.devices.pads[0] ? app.devices.owner[0] : -1,
        app.devices.pads[1] ? app.devices.owner[1] : -1, app.renderer.view_height[0],
        app.renderer.view_height[1], app.renderer.target_resizes[0], app.renderer.target_resizes[1],
        app.options ? "true" : "false", app.option_player, v.x, v.y, v.z, p->yaw, p->pitch,
        app.game.players[0].lift_active ? "true" : "false", app.game.players[0].lift_progress,
        app.game.players[0].lift_to.sector.y);
    return result;
}
void options_open(int player) {
    set_pause(true);
    app.options = true;
    app.option_player = player;
    app.option_row = 0;
}
void options_adjust(int direction) {
    if (!app.option_row) {
        app.option_player = 1 - app.option_player;
        return;
    }
    if (app.option_row == 8) {
        app.game.views[app.option_player].journal = true;
        app.game.views[app.option_player].journal_page = 7;
        app.options = false;
        return;
    }
    if (app.option_row == 9) {
        app_command(app.option_player, CMD_DISMISS_HINT, 0, 0);
        return;
    }
    if (app.option_row == 10) {
        sc_web_setting(app.option_player, -1, 0);
        return;
    }
    int setting = app.option_row - 1;
    double value = preferences_get(&app.renderer.preferences, app.option_player, setting);
    if (setting == PREF_INVERT)
        value = !value;
    else if (setting == PREF_GUIDANCE)
        value = ((int)value + direction + 3) % 3;
    else if (setting == PREF_QUALITY)
        value = ((int)value + direction + 5) % 5;
    else
        value += direction * (setting == PREF_DEADZONE ? .02 : .25);
    sc_web_setting(app.option_player, setting, value);
}
EMSCRIPTEN_KEEPALIVE void sc_web_save(void) {
    save_game();
}
EMSCRIPTEN_KEEPALIVE int sc_web_import(void) {
    if (!game_load(&app.game, "/persist/import.sav")) {
        remove("/persist/import.sav");
        return 0;
    }
    remove("/persist/import.sav");
    app.renderer.world_revision = 0;
    save_game();
    return 1;
}
