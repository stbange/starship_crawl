#define SDL_MAIN_HANDLED
#include "render.h"
#include "exterior.h"
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
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

typedef struct {
    SDL_FingerID id;
    bool active;
    int role;
    float x, y, start_x, start_y;
} Finger;
typedef struct {
    SDL_Window *window;
    SDL_GLContext context;
    SDL_AudioStream *audio;
    InputDevices devices;
    Game game;
    Renderer renderer;
    Finger fingers[10];
    bool running, paused, started, touch, debug, new_armed, no_save, failed, mouse_fire;
    bool demo_encounter, inspect_shelter, inspect_matrix, inspect_tentacles;
    int frame, smoke_frames, inspect_form, inspect_rig, inspect_alien;
    bool options;
    int option_player, option_row;
    double last, accumulator, fps, autosave, audio_time;
    Uint64 look_after;
    bool discard_look;
    float ambience[DISTRICT_TYPES];
    uint32_t audio_shots, audio_noise;
    int shot_sample;
    char save_path[1024], preferences_path[1024], save_status[96];
    const char *capture;
} App;
static App app;

static void set_pause(bool paused) {
    app.paused = paused;
    app.mouse_fire = false;
    memset(app.fingers, 0, sizeof(app.fingers));
    app.accumulator = 0;
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
            app.game.players[i].journal = false;
            app.game.players[i].decision_open = false;
            app.game.politics.audience[i] = 0;
            app.game.matrix.divers[i].choice = false;
        }
        /* Discard menu motion and the browser's initial pointer-lock warp. */
        SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION);
        app.look_after = SDL_GetTicksNS() + UINT64_C(120000000);
        app.discard_look = true;
    }
}
static uint32_t new_seed(void) {
    return world_hash((uint32_t)time(NULL), (Cell){(int32_t)(SDL_GetTicks() % 1000000), 0, 0},
                      app.game.world.seed + 13);
}
static void save_game(void) {
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
    if (app.paused || app.touch ||
        (app.game.players[0].map_open || app.game.matrix.divers[0].map) || !isfinite(dx) ||
        !isfinite(dy) || SDL_GetTicksNS() < app.look_after || (dx == 0 && dy == 0))
        return;
    if (app.discard_look) {
        app.discard_look = false;
        return;
    }
    const ViewPreferences *v = &app.renderer.preferences.view[0];
    game_look(&app.game.players[0], dx * .0023 * v->mouse,
              dy * .0023 * v->mouse * (v->inverted ? 1 : -1));
}
EMSCRIPTEN_KEEPALIVE void sc_web_reset_input(void) {
    if (!app.window)
        return;
    app.mouse_fire = false;
    memset(app.fingers, 0, sizeof(app.fingers));
    SDL_ResetKeyboard();
    SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION);
    app.accumulator = 0;
    app.discard_look = true;
}
static bool save_preferences(void) {
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
    if (player < 0 || player >= SC_PLAYERS)
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
        "{\"views\":[[%.4f,%.4f,%.4f,%.4f,%u,%u],[%.4f,%.4f,%.4f,%.4f,%u,%u]],"
        "\"pads\":[%d,%d],\"height\":[%d,%d],\"resizes\":[%u,%u],\"options\":%s,\"selected\":%d,"
        "\"p2\":[%.5f,%.5f,%.5f,%.5f,%.5f],\"transit\":%s,\"progress\":%.3f,\"deck\":%d}",
        a->mouse, a->touch, a->stick, a->deadzone, (unsigned)a->inverted, a->quality, b->mouse,
        b->touch, b->stick, b->deadzone, (unsigned)b->inverted, b->quality,
        app.devices.pads[0] ? app.devices.owner[0] : -1,
        app.devices.pads[1] ? app.devices.owner[1] : -1, app.renderer.view_height[0],
        app.renderer.view_height[1], app.renderer.target_resizes[0], app.renderer.target_resizes[1],
        app.options ? "true" : "false", app.option_player, v.x, v.y, v.z, p->yaw, p->pitch,
        app.game.players[0].lift_active ? "true" : "false", app.game.players[0].lift_progress,
        app.game.players[0].lift_to.sector.y);
    return result;
}
static void options_open(int player) {
    set_pause(true);
    app.options = true;
    app.option_player = player;
    app.option_row = 0;
}
static void options_adjust(int direction) {
    if (!app.option_row) {
        app.option_player = 1 - app.option_player;
        return;
    }
    if (app.option_row == 7) {
        sc_web_setting(app.option_player, -1, 0);
        return;
    }
    int setting = app.option_row - 1;
    double value = preferences_get(&app.renderer.preferences, app.option_player, setting);
    if (setting == PREF_INVERT)
        value = !value;
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
static void interact_player(int player) {
    bool reviewing = app.game.players[player].journal;
    game_interact(&app.game, player);
    if (app.game.players[player].journal)
        set_pause(true);
    else if (reviewing)
        set_pause(false);
}
static bool choose_player(int player, unsigned choice) {
    if (app.game.players[player].journal_page == 6)
        return politics_choose(&app.game, player, choice);
    return app.game.matrix.divers[player].choice ? matrix_choose(&app.game, player, choice)
                                                 : game_story_choose(&app.game, player, choice);
}
static void phase_journal(int player) {
    campaign_journal(&app.game, player);
    set_pause(true);
}
static void matrix_journal(int player) {
    Player *p = &app.game.players[player];
    p->journal = true;
    p->journal_page = 4;
    set_pause(true);
}
EMSCRIPTEN_KEEPALIVE void sc_web_action(int action) {
    MatrixDiver *d = &app.game.matrix.divers[0];
    if (action >= 230 && action <= 232) {
        if (action == 230) {
            politics_journal(&app.game, 0);
            set_pause(true);
        }
        if (action == 231)
            politics_read(&app.game, 0);
        if (action == 232)
            politics_track(&app.game, 0);
        return;
    }
    if (action >= 220 && action <= 228) {
        Campaign *c = &app.game.campaign;
        if (action == 220)
            phase_journal(0);
        else if (action == 221 || action == 222)
            c->slot[0] = (c->slot[0] + (action == 221 ? 5 : 1)) % 6;
        else if (action == 223 || action == 224)
            campaign_refit(&app.game, 0, c->slot[0], action == 223 ? -1 : 1);
        else if (action == 225)
            campaign_track(&app.game);
        else if (campaign_choose(&app.game, 0, (unsigned)(action - 225)))
            campaign_journal(&app.game, 0);
        return;
    }
    if (action == 200) {
        matrix_toggle(&app.game, 0);
        return;
    }
    if (action == 201) {
        matrix_service(&app.game, 0);
        return;
    }
    if (action == 202 || action == 203) {
        matrix_cycle_service(&app.game, 0, action == 202 ? -1 : 1);
        return;
    }
    if (action == 204 || action == 205) {
        matrix_cast(&app.game, 0, (unsigned)(action - 204));
        return;
    }
    if (action == 206) {
        matrix_journal(0);
        return;
    }
    if (action == 207) {
        matrix_upgrade(&app.game, 0);
        return;
    }
    if (action == 208 || action == 209) {
        matrix_select(&app.game, 0, (unsigned)(action - 208));
        return;
    }
    if (action >= 210 && action < 216) {
        matrix_equip(&app.game, 0, (unsigned)(action - 210));
        return;
    }
    if (d->active) {
        if (action == 11 || action == 80 || action == 84 || action == 85 || action == 86) {
            matrix_journal(0);
            return;
        }
        if (action == 12 || (action >= 100 && action < 112))
            return;
        if (action == 7) {
            matrix_service(&app.game, 0);
            return;
        }
        if (action == 9 || action == 10) {
            matrix_select(&app.game, 0, action == 9 ? 1 : 0);
            return;
        }
        if (action >= 60 && action < 64) {
            matrix_cast(&app.game, 0, d->selected);
            return;
        }
        if (action == 13) {
            matrix_toggle(&app.game, 0);
            return;
        }
        if ((action >= 1 && action <= 4) || (action >= 15 && action < 30) ||
            (action >= 40 && action < 60) || (action >= 70 && action < 74))
            return;
    }
    if (action >= 90 && action <= 92) {
        if (choose_player(0, (unsigned)(action - 89)))
            set_pause(false);
    } else if (action == 80 || action == 84 || action == 85 || action == 86) {
        Player *p = &app.game.players[0];
        p->journal = true;
        p->journal_page = action == 86                             ? 3
                          : action == 85                           ? 2
                          : action == 80 && app.game.story.version ? 1
                                                                   : 0;
        set_pause(true);
    } else if (action == 81) {
        game_story_track(&app.game);
        if (app.game.campaign.version)
            app.game.campaign.tracked = SC_GUARDIANS;
    } else if (action == 88)
        game_story_event_read(&app.game, 0);
    else if (action == 83)
        game_story_read(&app.game, 0);
    else if (action >= 60 && action < 60 + SC_PSIONICS)
        game_psionic(&app.game, 0, (Psionic)(action - 60));
    else if (action >= 70 && action < 70 + SC_MECH_WEAPONS) {
        app.game.players[0].rig_weapon = (unsigned)(action - 70);
        app.game.players[0].cooldown = .15f;
        app.game.dirty = true;
    } else if (action >= 30 && action < 36) {
        const int pan[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}};
        game_map_pan(&app.game, 0, pan[action - 30][0], pan[action - 30][1], pan[action - 30][2]);
    } else if (action == 14)
        game_map_toggle(&app.game, 0);
    else if (action >= 100 && action < 112) {
        Player *p = &app.game.players[0];
        if (p->logs & (1u << (action - 100))) {
            p->reading = (uint8_t)(action - 100);
            p->journal = true;
            p->journal_page = 0;
            set_pause(true);
        }
    } else if (action == 12) {
        Player *p = &app.game.players[0];
        if (p->journal && p->journal_page == 3) {
            game_story_event_read(&app.game, 0);
            return;
        }
        if (p->journal && p->journal_page && app.game.story.version) {
            game_story_read(&app.game, 0);
            return;
        }
        for (int i = 0; i < 12; i++) {
            p->reading = (p->reading + 1) % 12;
            if (p->logs & (1u << p->reading))
                break;
        }
    } else if (action >= 20 && action < 20 + SC_ATTRIBUTES)
        game_upgrade(&app.game, 0, action - 20);
    else if (action >= 40 && action < 40 + SC_WEAPONS) {
        Player *p = &app.game.players[0];
        unsigned gun = (unsigned)(action - 40);
        if (p->riding != MECH && (p->weapons & (1u << gun)) && campaign_weapon(&app.game, 0, gun)) {
            p->weapon = gun;
            p->cooldown = .15f;
            app.game.dirty = true;
        }
    } else if (action == 15 || action == 16)
        game_rig_toggle(&app.game, 0, action == 15 ? MECH : WAGON);
    else if (action == 17) {
        Player *p = &app.game.players[0];
        if (p->riding != MECH && campaign_weapon(&app.game, 0, SC_FLARE_CANNON)) {
            p->weapon = SC_FLARE_CANNON;
            p->cooldown = .15f;
            app.game.dirty = true;
        }
    } else if (action == 18 || action == 19)
        game_flare_upgrade(&app.game, 0, action == 19);
    else if (action == 13)
        game_summon(&app.game, 0);
    else if (action == 8)
        interact_player(0);
    else if (action == 9)
        game_weapon(&app.game, 0, 1);
    else if (action == 10)
        game_weapon(&app.game, 0, -1);
    else if (action == 11) {
        Player *p = &app.game.players[0];
        p->journal = !p->journal || (app.game.story.version && p->journal_page != 1);
        p->journal_page = app.game.story.version ? 1 : 0;
        p->decision_open = false;
        set_pause(p->journal);
    } else if (action >= 1 && action <= 3)
        game_upgrade(&app.game, 0, action - 1);
    else if (action == 4) {
        app.game.players[0].tint = (app.game.players[0].tint + 1) % 3;
        app.game.dirty = true;
    } else if (action == 5) {
        if (app.game.player_count == 1)
            game_join(&app.game);
        else
            game_leave(&app.game);
    } else if (action == 6) {
        game_new_ship(&app.game, new_seed());
        app.renderer.world_revision = 0;
        save_game();
    } else if (action == 7)
        game_flashlight(&app.game, 0);
    else if (action == 0)
        set_pause(true);
}
EMSCRIPTEN_KEEPALIVE uint32_t sc_web_fingerprint(uint32_t seed, int x, int y, int z) {
    if (x < -2000000 || x > 2000000 || y < -1000000 || y > 1000000 || z < 0 || z > 2000000)
        return 0;
    return world_fingerprint(seed, (Cell){x, y, z});
}
EMSCRIPTEN_KEEPALIVE const char *sc_web_state(void) {
    static char result[16384];
    const Player *p = &app.game.players[0];
    Vec3 feet = position_absolute(p->pos);
    unsigned projectiles = 0, climbers = 0, bodies = 0;
    for (int i = 0; i < SC_MAX_PROJECTILES; i++)
        projectiles += app.game.projectiles[i].active;
    for (int i = 0; i < app.game.world.count; i++)
        for (int j = 0; j < app.game.world.rooms[i].enemy_count; j++)
            climbers += app.game.world.rooms[i].enemies[j].climbing;
    for (int r = 0; r < app.game.world.count; r++)
        for (int j = 0; j < app.game.world.rooms[r].enemy_count; j++) {
            const Enemy *e = &app.game.world.rooms[r].enemies[j];
            if (e->alive && cell_equal(world_cell(position_absolute(e->pos)), world_cell(feet))) {
                EnemyBody body = sc_enemy_bodies[e->kind];
                bodies |= 1u << (body.form * 5 + body.variant);
            }
        }
    unsigned corpses = 0, death_forms = 0, hazards = 0, hazard_kinds = 0, triggered = 0, spent = 0;
    float death_age = 0;
    for (int i = 0; i < SC_MAX_DEATHS; i++)
        if (app.game.deaths[i].active) {
            corpses++;
            death_forms |= 1u << sc_enemy_bodies[app.game.deaths[i].body.kind].form;
            death_age = fmaxf(death_age, app.game.deaths[i].age);
        }
    for (int i = 0; i < SC_MAX_DEATH_HAZARDS; i++)
        if (app.game.death_hazards[i].active) {
            hazards++;
            hazard_kinds |= 1u << app.game.death_hazards[i].kind;
            triggered += app.game.death_hazards[i].triggered;
            spent += app.game.death_hazards[i].spent;
        }
    unsigned flares = 0, beams = 0;
    float flare_life = 0, flare_power = 0;
    for (int i = 0; i < SC_MAX_FLARES; i++)
        if (app.game.flares[i].active) {
            flares++;
            flare_life = fmaxf(flare_life, app.game.flares[i].life);
            flare_power = fmaxf(flare_power, app.game.flares[i].power);
        }
    for (int i = 0; i < SC_PLAYERS; i++)
        beams += app.game.beams[i].life > 0;
    float hull = p->riding ? app.game.rigs[0][p->riding - 1].armor : 0;
    Room *room = world_find(&app.game.world, world_cell(feet));
    Cell objective = {0};
    bool has_goal = game_objective(&app.game, &objective);
    ShipHistory past = world_history(app.game.world.seed);
    snprintf(
        result, sizeof(result),
        "{\"seed\":%u,\"players\":%d,\"paused\":%s,\"frames\":%d,\"rooms\":%d,"
        "\"x\":%.5f,\"y\":%.5f,\"z\":%.5f,\"rank\":%u,\"kills\":%u,\"shots\":%u,\"failed\":%s,"
        "\"generator\":%u,\"form\":%u,\"volume\":%u,\"atmosphere\":%u,\"ecology\":%u,\"tale\":%u,"
        "\"sprites\":%u,\"sprites2\":%u,\"tentacles\":%u,\"tentacles2\":%u,"
        "\"room_height\":%.1f,\"chunks\":%d,\"pending\":%d,"
        "\"generated\":%u,\"work\":%u,\"uploads\":%u,"
        "\"span\":%d,\"ready\":%s,\"fingerprint\":%u,\"flashlight\":%s,\"projectiles\":%u,\"enemy_"
        "shots\":%u,\"projectile_hits\":%u,\"climbers\":%u,\"mission\":%u,\"weapon\":%u,"
        "\"weapons\":%u,\"shield\":%.2f,\"armor\":%.2f,\"energy\":%.2f,\"credits\":%u,\"pickups\":%"
        "u,\"contracts\":%u,\"theme\":%u,\"logs\":%u,\"yaw\":%.4f,\"pitch\":%.4f,\"time\":%.3f,"
        "\"sheltered\":%s,\"shelter_deployed\":%s,\"sx\":%.3f,\"sz\":%.3f,\"resting\":%.2f,"
        "\"nourished\":%.2f,\"health\":%.2f,\"facility\":%d,"
        "\"map\":%s,\"map_x\":%d,\"map_y\":%d,\"map_z\":%d,\"veil\":%s,\"flight\":%.2f,\"leap\":%."
        "2f,\"nova\":%.2f,\"power\":%u,\"bodies\":%u,\"key_w\":%u,\"riding\":%u,\"rig_weapon\":%u,"
        "\"hull\":%.2f,\"drive_speed\":%.2f,\"mech_deployed\":%s,\"wagon_deployed\":%s,\"beams\":%"
        "u,\"flares\":%u,\"flare_lifetime\":%u,\"flare_strength\":%u,\"flare_life\":%.2f,\"flare_"
        "power\":%.2f,\"points\":%u,\"profile\":%u,\"layout\":%u,\"incident\":%u,\"intensity\":%u,"
        "\"landmark\":%u,\"room_depth\":%.1f,\"cache_x\":%.3f,\"cache_z\":%.3f,"
        "\"story_version\":%u,\"motivation\":%u,\"story_assigned\":%u,\"story_seen\":%u,"
        "\"story_done\":%u,\"story_choice\":%u,\"story_tracked\":%u,\"story_reading\":%u,"
        "\"journal\":%s,\"journal_page\":%u,\"decision\":%s,\"has_goal\":%s,"
        "\"goal_x\":%d,\"goal_y\":%d,\"goal_z\":%d,\"victories\":%u,"
        "\"choice1\":\"%s\",\"choice2\":\"%s\",\"choice3\":\"%s\","
        "\"prepared\":%s,\"history_cause\":%u,\"history_arc\":%u,\"history_role\":%u,"
        "\"origin_x\":%d,\"origin_y\":%d,\"origin_z\":%d,"
        "\"council\":%u,\"support0\":%u,\"support1\":%u,\"chapter\":%u,\"arc_kind\":%u,\"chapter_"
        "choice\":%u,"
        "\"remaining\":%u,\"events\":%u,\"systems\":%u,\"safe\":%s,\"event_reading\":%u,"
        "\"last0\":%u,\"last1\":%u,\"last2\":%u,\"last3\":%u,"
        "\"total0\":%u,\"total1\":%u,\"total2\":%u,\"total3\":%u,\"total4\":%u,"
        "\"career0\":%u,\"career1\":%u,\"career2\":%u,\"career3\":%u,\"career4\":%u,"
        "\"corpses\":%u,\"death_forms\":%u,\"death_age\":%.3f,\"hazards\":%u,"
        "\"hazard_kinds\":%u,\"hazard_triggered\":%u,\"hazard_spent\":%u,\"respawns\":%u}",
        app.game.world.seed, app.game.player_count, app.paused ? "true" : "false", app.frame,
        app.game.world.count, feet.x, feet.y, feet.z, p->level, p->kills, app.game.shots,
        app.failed ? "true" : "false", app.game.world.generator, room ? room->habitat.form : 0,
        room ? room->habitat.volume : 0, room ? room->habitat.atmosphere : 0,
        room ? room->habitat.ecology : 0, room ? room->habitat.tale : 0,
        app.renderer.sprite_drawn[0], app.renderer.sprite_drawn[1], app.renderer.tentacle_drawn[0],
        app.renderer.tentacle_drawn[1], room ? room->height : 0, app.game.world.chunk_count,
        app.game.world.pending, app.game.world.generated, app.game.world.last_work,
        app.renderer.last_uploads, room ? room->span_x * room->span_z : 0,
        room && room->ready ? "true" : "false",
        world_fingerprint_version(app.game.world.seed, app.game.world.generator,
                                  world_region(world_cell(feet))),
        p->flashlight ? "true" : "false", projectiles, app.game.enemy_shots,
        app.game.projectile_hits, climbers, app.game.mission, p->weapon, p->weapons, p->shield,
        p->armor, p->energy, p->credits, p->pickups, app.game.contracts, room ? room->theme : 0,
        p->logs, p->yaw, p->pitch, app.game.time, p->sheltered ? "true" : "false",
        app.game.shelter.deployed ? "true" : "false", p->shelter_pos.x, p->shelter_pos.z,
        p->resting, p->nourished, p->health, (int)game_shelter_near(&app.game, 0),
        p->map_open ? "true" : "false", p->map_center.x, p->map_center.y, p->map_center.z,
        p->cloaked ? "true" : "false", p->flight, p->leap, p->nova, p->selected_power, bodies,
        (unsigned)SDL_GetKeyboardState(NULL)[SDL_SCANCODE_W], p->riding, p->rig_weapon, hull,
        p->drive_speed, app.game.rigs[0][0].deployed ? "true" : "false",
        app.game.rigs[0][1].deployed ? "true" : "false", beams, flares, p->flare_lifetime,
        p->flare_strength, flare_life, flare_power, p->points, world_profile(app.game.world.seed),
        room ? room->layout : 0, room ? room->incident : 0, room ? room->intensity : 0,
        room ? room->landmark : 0, room ? room->depth : 0,
        room && room->ready ? position_absolute(room->pickups[3].pos).x : 0,
        room && room->ready ? position_absolute(room->pickups[3].pos).z : 0, app.game.story.version,
        story_motivation_version(app.game.world.seed, app.game.story.version),
        app.game.story.assigned, app.game.story.seen, app.game.story.done, app.game.story.choice,
        app.game.story.tracked, p->story_reading, p->journal ? "true" : "false", p->journal_page,
        (p->decision_open || app.game.matrix.divers[0].choice || app.game.politics.audience[0])
            ? "true"
            : "false",
        has_goal ? "true" : "false", objective.x, objective.y, objective.z, p->victories,
        game_story_choice(&app.game, 1), game_story_choice(&app.game, 2),
        game_story_choice(&app.game, 3), game_story_choice_ready(&app.game, 0) ? "true" : "false",
        past.cause, past.arc, room ? room->history_role : 0, past.origin.x, past.origin.y,
        past.origin.z, app.game.story.council, app.game.story.support[0], app.game.story.support[1],
        app.game.story.chapter, app.game.story.arc, app.game.story.chapter_choice,
        app.game.story.remaining, app.game.story.event_count,
        world_systems(&app.game.world, world_cell(feet)),
        game_story_safe(&app.game, feet) ? "true" : "false", p->event_reading,
        app.game.story.last[0], app.game.story.last[1], app.game.story.last[2],
        app.game.story.last[3], app.game.story.totals[0], app.game.story.totals[1],
        app.game.story.totals[2], app.game.story.totals[3], app.game.story.totals[4], p->career[0],
        p->career[1], p->career[2], p->career[3], p->career[4], corpses, death_forms, death_age,
        hazards, hazard_kinds, triggered, spent, app.game.respawns);
    size_t used = strlen(result);
    const MatrixDiver *d = &app.game.matrix.divers[0];
    const MatrixProfile *mp = &app.game.matrix.profiles[0];
    const MatrixNetwork *mw = &app.game.matrix.networks[d->active ? d->network : 0];
    const MatrixRecord *mr = mw->active ? matrix_find(&app.game.matrix, mw->key) : NULL;
    Vec3 goal = {0};
    matrix_goal(&app.game, 0, &goal);
    unsigned guards = 0, shots = 0;
    for (int i = 0; i < SC_MATRIX_GUARDS; i++)
        guards += mw->guards[i].active;
    for (int i = 0; i < SC_MATRIX_SHOTS; i++)
        shots += mw->shots[i].active;
    used--;
    int n = snprintf(result + used, sizeof(result) - used,
                     ",\"matrix\":%s,\"matrix_near\":%s,\"mx\":%.4f,\"my\":%.4f,\"mz\":%.4f,"
                     "\"integrity\":%.2f,\"bandwidth\":%.2f,\"trace\":%.2f,\"matrix_map\":%s,"
                     "\"matrix_choice\":%s,"
                     "\"programs\":%u,\"slot0\":%u,\"slot1\":%u,\"matrix_selected\":%u,\"matrix_"
                     "data\":%u,\"matrix_service\":%u,"
                     "\"matrix_keys\":%u,\"matrix_flags\":%u,\"matrix_outcome\":%u,\"matrix_seen\":"
                     "%u,\"matrix_killed\":%u,"
                     "\"matrix_entry\":%u,\"matrix_core\":%u,\"matrix_credential\":%u,\"matrix_"
                     "vault\":%u,\"matrix_rare\":%s,"
                     "\"matrix_theme\":%u,\"matrix_guards\":%u,\"matrix_shots\":%u,\"matrix_"
                     "switch0\":%.2f,\"matrix_switch1\":%.2f,"
                     "\"matrix_goal_x\":%.2f,\"matrix_goal_z\":%.2f,\"matrix_veil\":%.2f,\"matrix_"
                     "lock\":%.2f,\"matrix_nodes\":[",
                     d->active ? "true" : "false", matrix_near(&app.game, 0) ? "true" : "false",
                     d->pos.x, d->pos.y, d->pos.z, d->integrity, d->bandwidth, d->trace,
                     d->map ? "true" : "false", d->choice ? "true" : "false", mp->programs,
                     mp->slots[0], mp->slots[1], d->selected, mp->data, d->service,
                     mr ? mr->keys : 0, mr ? mr->flags : 0, mr ? mr->outcome : 0, mr ? mr->seen : 0,
                     mr ? mr->killed : 0, mw->entry, mw->core, mw->credential, mw->vault,
                     mw->rare ? "true" : "false", mw->theme, guards, shots, mw->switches[0],
                     mw->switches[1], goal.x, goal.z, d->veil, d->lockout);
    if (n > 0 && (size_t)n < sizeof(result) - used)
        used += (size_t)n;
    bool first = true;
    for (unsigned i = 0; i < 16; i++)
        if (mw->nodes[i].valid) {
            const MatrixNode *node = &mw->nodes[i];
            n = snprintf(result + used, sizeof(result) - used,
                         "%s{\"id\":%u,\"x\":%.2f,\"z\":%.2f,\"links\":%u,\"room_x\":%d,\"room_y\":"
                         "%d,\"room_z\":%d}",
                         first ? "" : ",", i, node->at.x, node->at.z, node->links, node->room.x,
                         node->room.y, node->room.z);
            if (n > 0 && (size_t)n < sizeof(result) - used)
                used += (size_t)n;
            first = false;
        }
    const Campaign *campaign = &app.game.campaign;
    const CampaignProfile *build = &campaign->profiles[0];
    unsigned selected = campaign->tracked < SC_GUARDIANS ? campaign->tracked : 0;
    Room *guardian_room = world_find(&app.game.world, campaign_site(selected));
    char offers[3][110];
    for (unsigned i = 0; i < 3; i++)
        campaign_reward_name(&app.game, 0, campaign->choice[0] ? campaign->choice[0] - 1 : selected,
                             i, offers[i], sizeof(offers[i]));
    snprintf(result + used, sizeof(result) - used,
             "],\"campaign\":%u,\"phase\":%u,\"defeated\":%u,\"guardian\":%u,"
             "\"guardian_active\":%s,\"guardian_health\":%.2f,\"reward_pending\":%u,"
             "\"powers_owned\":%u,\"rigs_owned\":%u,\"modules_owned\":%u,\"claimed\":%u,"
             "\"gun_slots\":[%u,%u],\"power_slots\":[%u,%u],\"rig_slot\":%u,\"module\":%u,"
             "\"build_slot\":%u,\"offer1\":\"%s\",\"offer2\":\"%s\",\"offer3\":\"%s\"}",
             campaign->version, campaign_phase(campaign), campaign->defeated, campaign->tracked,
             campaign->attempts[selected].active ? "true" : "false",
             guardian_room && guardian_room->ready ? guardian_room->enemies[0].health : 0,
             campaign->choice[0], build->powers, build->rigs, build->modules, build->claimed,
             build->guns[0], build->guns[1], build->power_slots[0], build->power_slots[1],
             build->rig, build->module, campaign->slot[0], offers[0], offers[1], offers[2]);
    used = strlen(result) - 1;
    const Navigation *nav = &p->navigation;
    snprintf(result + used, sizeof(result) - used,
             ",\"nav_kind\":%u,\"nav_next\":[%d,%d,%d],\"nav_point\":[%.3f,%.3f,%.3f],"
             "\"nav_searched\":%u,\"lift_latched\":%s,\"arrival\":%.2f,\"nav2_kind\":%u}",
             nav->kind, nav->next.x, nav->next.y, nav->next.z, nav->point.x, nav->point.y,
             nav->point.z, nav->searched, p->lift_latched ? "true" : "false", p->arrival_time,
             app.game.players[1].navigation.kind);
    used = strlen(result) - 1;
    Room *local = world_find(&app.game.world, world_cell(feet));
    unsigned living = 0, behaviors = 0, motions = 0;
    if (local)
        for (int i = 0; i < local->enemy_count; i++) {
            const Enemy *e = &local->enemies[i];
            living += e->alive;
            if (e->alive) {
                behaviors |= 1u << e->behavior;
                motions |= 1u << e->motion;
            }
        }
    char offer[120] = "";
    if (local && local->habitat.pace == PACE_FRONTIER)
        encounter_reward_name(&app.game, 0, local, offer, sizeof(offer));
    snprintf(result + used, sizeof(result) - used,
             ",\"pace\":%u,\"local_state\":%u,\"frontier_active\":%s,\"frontier_living\":%u,"
             "\"frontier_core\":%.2f,\"frontier_attacker\":%u,\"frontier_disrupt\":%.3f,"
             "\"alien_behaviors\":%u,\"alien_motions\":%u,\"frontier_offer\":\"%s\","
             "\"drill_time\":%.2f,\"drill_complete\":%s}",
             local ? local->habitat.pace : 0, world_local_state(&app.game.world, local),
             local && local->frontier_active ? "true" : "false", living,
             local && local->enemy_count ? local->enemies[0].health : 0,
             local ? local->frontier_attacker : 0, local ? local->frontier_disrupt : 0, behaviors,
             motions, offer, app.game.drills[0].time,
             app.game.drills[0].complete ? "true" : "false");
    used = strlen(result) - 1;
    Vec3 console = local ? encounter_console(local) : v3(0, 0, 0);
    snprintf(result + used, sizeof(result) - used,
             ",\"encounter_console\":[%.3f,%.3f,%.3f],\"regional_enemies\":[", console.x, console.y,
             console.z);
    if (local && local->habitat.pace == PACE_FRONTIER)
        for (int i = 0; i < 4; i++) {
            const Enemy *e = &local->enemies[i];
            Vec3 at = enemy_center(e);
            used = strlen(result);
            snprintf(result + used, sizeof(result) - used,
                     "%s{\"at\":[%.3f,%.3f,%.3f],\"alive\":%s,\"motion\":%u,\"health\":%.2f}",
                     i ? "," : "", at.x, at.y, at.z, e->alive ? "true" : "false", e->motion,
                     e->health);
        }
    used = strlen(result);
    snprintf(result + used, sizeof(result) - used, "]}");
    used = strlen(result) - 1;
    snprintf(result + used, sizeof(result) - used,
             ",\"challenge\":%u,\"reveal\":%u,\"contract\":%u,\"circuit\":%u,\"hazard_time\":%.3f,"
             "\"relics\":[%u,%u,%u,%u,%u],\"specialists\":[",
             local ? local->habitat.challenge : 0, local ? local->habitat.reveal : 0,
             local ? local->habitat.contract : 0, local ? local->circuit : 0,
             local ? local->hazard_time : 0, app.game.frontier.ranks[0][0],
             app.game.frontier.ranks[0][1], app.game.frontier.ranks[0][2],
             app.game.frontier.ranks[0][3], app.game.frontier.ranks[0][4]);
    if (local)
        for (int i = 0; i < local->enemy_count; i++) {
            const Enemy *e = &local->enemies[i];
            Vec3 at = enemy_center(e);
            used = strlen(result);
            snprintf(result + used, sizeof(result) - used,
                     "%s{\"kind\":%u,\"behavior\":%u,\"motion\":%u,\"alive\":%s,\"observed\":%s,"
                     "\"molted\":%s,\"unfold\":%.3f,\"sound\":%.3f,\"at\":[%.3f,%.3f,%.3f]}",
                     i ? "," : "", e->kind, e->behavior, e->motion, e->alive ? "true" : "false",
                     e->observed ? "true" : "false", e->molted ? "true" : "false", e->unfold,
                     e->warning_sound, at.x, at.y, at.z);
        }
    used = strlen(result);
    snprintf(result + used, sizeof(result) - used, "]}");
    used = strlen(result) - 1;
    Politics *pol = &app.game.politics;
    Director *dir = &app.game.director;
    unsigned pn = pol->reading[0] % SC_COUNTRIES;
    snprintf(result + used, sizeof(result) - used,
             ",\"politics\":%u,\"country\":%u,\"court\":%u,\"court_reading\":%u,\"audience\":%u,"
             "\"pledge\":%u,\"court_stage\":%u,\"political_goal\":%u,\"director_kind\":%u,"
             "\"director_sequence\":%u,\"director_cooldown\":%u,\"director_remaining\":%u}",
             pol->version, politics_owner(&app.game, world_cell(feet)),
             local ? local->habitat.court : 0, pn, pol->audience[0], pol->countries[pn].pledge,
             pol->countries[pn].stage, pol->tracked, dir->kind, dir->sequence, dir->cooldown,
             dir->remaining);
    used = strlen(result) - 1;
    snprintf(result + used, sizeof(result) - used,
             ",\"court_stages\":[%u,%u,%u],\"court_pledges\":[%u,%u,%u],\"court_claims\":[%u,%u,%u]"
             ",\"director_reason\":%u}",
             pol->countries[0].stage, pol->countries[1].stage, pol->countries[2].stage,
             pol->countries[0].pledge, pol->countries[1].pledge, pol->countries[2].pledge,
             pol->countries[0].claimed, pol->countries[1].claimed, pol->countries[2].claimed,
             dir->reasons[(dir->sequence + SC_DIRECTOR_LOG - 1) % SC_DIRECTOR_LOG]);
    return result;
}
static void audio_frame(void) {
    if (!app.audio)
        return;
    if (app.paused) {
        SDL_ClearAudioStream(app.audio);
        app.audio_shots = app.game.shots;
        app.shot_sample = 1800;
        return;
    }
    if (app.audio_shots != app.game.shots) {
        app.audio_shots = app.game.shots;
        app.shot_sample = 0;
        app.audio_noise = 27;
    }
    float target[DISTRICT_TYPES] = {0};
    for (int i = 0; i < app.game.player_count; i++) {
        Vec3 feet = position_absolute(app.game.players[i].pos);
        Room *room = world_find(&app.game.world, world_cell(feet));
        if (feet.z >= 0 && room && room->ready)
            target[room->district] += (room->type == CONDUIT ? .45f : 1.0f) / app.game.player_count;
    }
    /* A small single mixed output: district drones crossfade, including co-op
       views in different districts. Generation never touches the audio device. */
    double warning[ALIEN_BEHAVIORS] = {0};
    for (int room = 0; room < app.game.world.count; room++)
        for (int n = 0; n < app.game.world.rooms[room].enemy_count; n++) {
            const Enemy *e = &app.game.world.rooms[room].enemies[n];
            if (!e->alive || !e->behavior || e->warning_sound <= 0)
                continue;
            for (int player = 0; player < app.game.player_count; player++) {
                const Player *p = &app.game.players[player];
                if (p->sheltered || app.game.matrix.divers[player].active)
                    continue;
                double distance = vlen(vsub(player_eye(p), enemy_center(e)));
                if (distance < 35)
                    warning[e->behavior] =
                        fmax(warning[e->behavior], (1 - distance / 35) * e->warning_sound);
            }
        }
    int queued = SDL_GetAudioStreamQueued(app.audio);
    for (int block = 0; block < 4 && queued < 8192; block++) {
        float samples[512];
        for (int i = 0; i < 512; i++) {
            double t = app.audio_time;
            for (int d = 0; d < DISTRICT_TYPES; d++)
                app.ambience[d] += (target[d] - app.ambience[d]) * .0005f;
            double ambient = app.ambience[HABITAT] *
                             (sin(t * SC_PI * 2 * 54) + .3 * sin(t * SC_PI * 2 * 81)) * .009;
            ambient += app.ambience[MACHINERY] *
                       (sin(t * SC_PI * 2 * 37.5) + .45 * sin(t * SC_PI * 2 * 112.5)) * .014;
            ambient += app.ambience[SANCTUM] *
                       (sin(t * SC_PI * 2 * 65.4) + sin(t * SC_PI * 2 * 98.1)) * .007;
            for (int d = LUXURY; d < DISTRICT_TYPES; d++)
                ambient +=
                    app.ambience[d] *
                    (sin(t * SC_PI * 2 * (42 + d * 5)) + .25 * sin(t * SC_PI * 2 * (84 + d * 7))) *
                    .007;
            for (int k = 1; k < ALIEN_BEHAVIORS; k++) {
                if (warning[k] <= 0)
                    continue;
                double pulse = pow(fmax(0, sin(t * (k == ALIEN_MACHINE ? 28 : 12 + k))), 6);
                ambient += warning[k] * .024 * pulse * sin(t * SC_PI * 2 * (95 + k * 37));
            }
            double shot = 0;
            if (app.shot_sample < 1800) {
                double age = (double)app.shot_sample++ / 22050;
                app.audio_noise = app.audio_noise * 1664525u + 1013904223u;
                shot = (sin(age * (1600 - age * 5000)) * .11 +
                        ((double)(app.audio_noise % 1000) / 500 - 1) * .05) *
                       exp(-age * 65);
            }
            samples[i] = (float)(ambient + shot);
            app.audio_time += 1.0 / 22050;
        }
        SDL_PutAudioStreamData(app.audio, samples, sizeof(samples));
        queued += (int)sizeof(samples);
    }
}
static void open_pad(SDL_JoystickID id) {
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
    if ((app.game.players[0].map_open || app.game.matrix.divers[0].map))
        return fabsf((x - .51f) * w) < radius && fabsf((y - .1f) * h) < radius ? 13 : -1;
    for (int i = 0; i < 12; i++)
        if (fabsf((x - xs[i]) * w) < radius && fabsf((y - ys[i]) * h) < radius)
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
                    matrix_cycle_service(&app.game, 0, app.fingers[i].role == 5 ? 1 : -1);
                if (app.fingers[i].role == 7)
                    sc_web_action(7);
                if (app.fingers[i].role == 8)
                    interact_player(0);
                if (app.fingers[i].role == 9)
                    sc_web_action(9);
                if (app.fingers[i].role == 10)
                    sc_web_action(13);
                if (app.fingers[i].role == 11) {
                    if (app.game.matrix.divers[0].active)
                        matrix_cast(&app.game, 0, app.game.matrix.divers[0].selected);
                    else
                        game_psionic(&app.game, 0, (Psionic)app.game.players[0].selected_power);
                }
                if (app.fingers[i].role == 12 && app.game.matrix.divers[0].active)
                    matrix_select(&app.game, 0, 1 - app.game.matrix.divers[0].selected);
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
                    if (f->role == 1 && !app.paused && !app.game.players[0].map_open &&
                        !app.game.matrix.divers[0].map)
                        game_look(&app.game.players[0],
                                  (e->x - f->x) * 3.5 * app.renderer.preferences.view[0].touch,
                                  (e->y - f->y) * 2.8 * app.renderer.preferences.view[0].touch *
                                      (app.renderer.preferences.view[0].inverted ? 1 : -1));
                    f->x = e->x;
                    f->y = e->y;
                }
                break;
            }
}
static void events(void) {
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
                game_look(&app.game.players[0], e.motion.xrel * .0023 * v->mouse,
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
                                (app.option_row + (mb == SDL_GAMEPAD_BUTTON_DPAD_UP ? 7 : 1)) % 8;
                        if (mb == SDL_GAMEPAD_BUTTON_DPAD_LEFT ||
                            mb == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                            options_adjust(mb == SDL_GAMEPAD_BUTTON_DPAD_LEFT ? -1 : 1);
                        if (mb == SDL_GAMEPAD_BUTTON_START || mb == SDL_GAMEPAD_BUTTON_SOUTH)
                            sc_web_start();
                        if (mb == SDL_GAMEPAD_BUTTON_BACK || mb == SDL_GAMEPAD_BUTTON_EAST)
                            app.options = false;
                        break;
                    }
                    if (app.paused && !p->journal && mb == SDL_GAMEPAD_BUTTON_BACK) {
                        options_open(player);
                        break;
                    }
                    if (p->journal && p->journal_page == 5) {
                        Campaign *c = &app.game.campaign;
                        if (mb == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ||
                            mb == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                            c->slot[player] = (c->slot[player] +
                                               (mb == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ? 5 : 1)) %
                                              6;
                        if (mb == SDL_GAMEPAD_BUTTON_DPAD_LEFT ||
                            mb == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                            campaign_refit(&app.game, player, c->slot[player],
                                           mb == SDL_GAMEPAD_BUTTON_DPAD_LEFT ? -1 : 1);
                        if (mb == SDL_GAMEPAD_BUTTON_WEST)
                            campaign_track(&app.game);
                        unsigned choice = mb == SDL_GAMEPAD_BUTTON_SOUTH   ? 1
                                          : mb == SDL_GAMEPAD_BUTTON_NORTH ? 2
                                          : mb == SDL_GAMEPAD_BUTTON_EAST  ? 3
                                                                           : 0;
                        if (choice && campaign_choose(&app.game, player, choice))
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
                    if (p->journal && p->journal_page == 4 && !d->choice) {
                        if (mb == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ||
                            mb == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                            matrix_select(&app.game, player,
                                          mb == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
                        if (mb == SDL_GAMEPAD_BUTTON_DPAD_LEFT ||
                            mb == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) {
                            unsigned program = app.game.matrix.profiles[player].slots[d->selected];
                            for (int step = 0; step < 6; step++) {
                                program =
                                    (program + (mb == SDL_GAMEPAD_BUTTON_DPAD_RIGHT ? 1 : 5)) % 6;
                                if (matrix_equip(&app.game, player, program))
                                    break;
                            }
                        }
                        if (mb == SDL_GAMEPAD_BUTTON_DPAD_UP)
                            matrix_upgrade(&app.game, player);
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
                            if (choice && matrix_choose(&app.game, player, choice))
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
                            matrix_toggle(&app.game, player);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_LEFT_STICK) {
                            game_map_toggle(&app.game, player);
                            continue;
                        }
                        if (d->map)
                            continue;
                        if (b == SDL_GAMEPAD_BUTTON_WEST)
                            matrix_service(&app.game, player);
                        if (b == SDL_GAMEPAD_BUTTON_NORTH)
                            matrix_cast(&app.game, player, d->selected);
                        if (b == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ||
                            b == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                            matrix_select(&app.game, player,
                                          b == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_LEFT || b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                            matrix_cycle_service(&app.game, player,
                                                 b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT ? 1 : -1);
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                            interact_player(player);
                        continue;
                    }
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_BACK &&
                        matrix_near(&app.game, player) && !app.paused) {
                        matrix_toggle(&app.game, player);
                        continue;
                    }
                    if (p->journal && p->journal_page && app.game.story.version) {
                        int b = e.gbutton.button;
                        unsigned choice = b == SDL_GAMEPAD_BUTTON_DPAD_LEFT    ? 1
                                          : b == SDL_GAMEPAD_BUTTON_DPAD_UP    ? 2
                                          : b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT ? 3
                                                                               : 0;
                        if ((p->journal_page == 1 || p->journal_page == 6) && choice &&
                            choose_player(player, choice))
                            set_pause(false);
                        if (b == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                            p->journal_page = p->journal_page == 6   ? 1
                                              : p->journal_page == 3 ? 6
                                                                     : p->journal_page + 1;
                        if (b == SDL_GAMEPAD_BUTTON_WEST) {
                            if (p->journal_page == 6)
                                politics_read(&app.game, player);
                            else if (p->journal_page == 3)
                                game_story_event_read(&app.game, player);
                            else
                                game_story_read(&app.game, player);
                        }
                        if (b == SDL_GAMEPAD_BUTTON_NORTH && p->journal_page == 6)
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
                    if (p->map_open) {
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
                            p->journal = true;
                            p->journal_page = app.game.story.version ? 1 : 0;
                            set_pause(true);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_SOUTH || b == SDL_GAMEPAD_BUTTON_EAST) {
                            game_rig_toggle(&app.game, player,
                                            b == SDL_GAMEPAD_BUTTON_SOUTH ? MECH : WAGON);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_WEST) {
                            if (p->riding != MECH) {
                                p->weapon = SC_FLARE_CANNON;
                                p->cooldown = .15f;
                                app.game.dirty = true;
                            }
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_LEFT) {
                            game_psionic(&app.game, player, PSI_VEIL);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_UP) {
                            game_psionic(&app.game, player, PSI_FLIGHT);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) {
                            game_psionic(&app.game, player, PSI_LEAP);
                            continue;
                        }
                        if (b == SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
                            game_psionic(&app.game, player, PSI_NOVA);
                            continue;
                        }
                    }
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START) {
                        app.started = true;
                        set_pause(!app.paused);
                    }
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_BACK)
                        game_summon(&app.game, pad_player(i));
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                        game_weapon(&app.game, pad_player(i), 1);
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                        interact_player(pad_player(i));
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_STICK)
                        game_flashlight(&app.game, pad_player(i));
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT)
                        game_upgrade(&app.game, pad_player(i), 0);
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
                        game_upgrade(&app.game, pad_player(i), 1);
                    if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                        game_upgrade(&app.game, pad_player(i), 2);
                }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            if (e.wheel.y != 0) {
                if (app.game.matrix.divers[0].active)
                    matrix_select(&app.game, 0, e.wheel.y > 0 ? 1 : 0);
                else
                    game_weapon(&app.game, 0, e.wheel.y > 0 ? 1 : -1);
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
                    app.option_row = (app.option_row + (e.key.key == SDLK_UP ? 7 : 1)) % 8;
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
                if (p->journal && p->journal_page == 6) {
                    if (key == SDLK_L || key == SDLK_U) {
                        sc_web_action(key == SDLK_L ? 231 : 232);
                        break;
                    }
                }
                if (key == SDLK_F10) {
                    phase_journal(0);
                    break;
                }
                if (p->journal && p->journal_page == 5) {
                    if (key == SDLK_Q || key == SDLK_E) {
                        sc_web_action(key == SDLK_Q ? 221 : 222);
                        break;
                    }
                    if (key == SDLK_Z || key == SDLK_X) {
                        sc_web_action(key == SDLK_Z ? 223 : 224);
                        break;
                    }
                    if (key == SDLK_U) {
                        sc_web_action(225);
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
                    matrix_toggle(&app.game, 0);
                    break;
                }
                if (p->journal && p->journal_page == 4 && !d->choice) {
                    if (key >= SDLK_1 && key <= SDLK_6) {
                        matrix_equip(&app.game, 0, (unsigned)(key - SDLK_1));
                        break;
                    }
                    if (key == SDLK_Z || key == SDLK_X) {
                        matrix_select(&app.game, 0, key == SDLK_X);
                        break;
                    }
                    if (key == SDLK_O) {
                        matrix_upgrade(&app.game, 0);
                        break;
                    }
                }
                if (d->active) {
                    if (d->choice && key >= SDLK_1 && key <= SDLK_3) {
                        if (matrix_choose(&app.game, 0, (unsigned)(key - SDLK_1 + 1)))
                            set_pause(false);
                        break;
                    }
                    if (key == SDLK_C || key == SDLK_R) {
                        matrix_toggle(&app.game, 0);
                        break;
                    }
                    if (key == SDLK_F) {
                        matrix_service(&app.game, 0);
                        break;
                    }
                    if (key == SDLK_Q || key == SDLK_E) {
                        matrix_cycle_service(&app.game, 0, key == SDLK_E ? 1 : -1);
                        break;
                    }
                    if (key == SDLK_Z || key == SDLK_X) {
                        matrix_select(&app.game, 0, key == SDLK_X);
                        break;
                    }
                    if (key == SDLK_V) {
                        matrix_cast(&app.game, 0, d->selected);
                        break;
                    }
                    if (key == SDLK_O) {
                        matrix_upgrade(&app.game, 0);
                        break;
                    }
                    if (key == SDLK_1 || key == SDLK_2) {
                        matrix_cast(&app.game, 0, (unsigned)(key - SDLK_1));
                        break;
                    }
                    if (key == SDLK_ESCAPE && d->map) {
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
                if ((app.game.players[0].map_open || app.game.matrix.divers[0].map)) {
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
                sc_web_action(5);
                break;
            case SDLK_L:
                sc_web_action(12);
                break;
            case SDLK_M:
                game_map_toggle(&app.game, 0);
                break;
            case SDLK_V:
                game_psionic(&app.game, 0, PSI_VEIL);
                break;
            case SDLK_B:
                game_psionic(&app.game, 0, PSI_FLIGHT);
                break;
            case SDLK_J:
                game_psionic(&app.game, 0, PSI_LEAP);
                break;
            case SDLK_H:
                game_psionic(&app.game, 0, PSI_NOVA);
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
                if ((app.game.players[0].map_open || app.game.matrix.divers[0].map)) {
                    game_map_toggle(&app.game, 0);
                    game_map_toggle(&app.game, 0);
                }
                break;
            case SDLK_T:
                sc_web_action(15);
                break;
            case SDLK_Y:
                sc_web_action(16);
                break;
            case SDLK_K:
                sc_web_action(17);
                break;
            case SDLK_O:
                sc_web_action(18);
                break;
            case SDLK_P:
                sc_web_action(19);
                break;
            case SDLK_R:
                game_summon(&app.game, 0);
                break;
            case SDLK_G:
                interact_player(0);
                break;
            case SDLK_Z:
                game_weapon(&app.game, 0, -1);
                break;
            case SDLK_X:
                game_weapon(&app.game, 0, 1);
                break;
            case SDLK_I:
                sc_web_action(11);
                break;
            case SDLK_F7:
                sc_web_action(86);
                break;
            case SDLK_F6:
                sc_web_action(app.game.players[0].journal && app.game.players[0].journal_page == 2
                                  ? 230
                                  : 85);
                break;
            case SDLK_F4:
                sc_web_action(app.game.players[0].journal_page ? 84 : 80);
                break;
            case SDLK_U:
                game_story_track(&app.game);
                if (app.game.campaign.version)
                    app.game.campaign.tracked = SC_GUARDIANS;
                break;
            case SDLK_F:
                game_flashlight(&app.game, 0);
                break;
            case SDLK_F3:
                app.debug = !app.debug;
                break;
            case SDLK_F5:
                save_game();
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
                sc_web_action(4);
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
                if (app.game.players[0].journal && app.game.players[0].journal_page) {
                    if (((app.game.players[0].journal_page == 1 &&
                          app.game.players[0].decision_open) ||
                         app.game.players[0].journal_page == 6) &&
                        e.key.key <= SDLK_3)
                        sc_web_action(90 + (int)(e.key.key - SDLK_1));
                } else
                    game_upgrade(&app.game, 0, (int)(e.key.key - SDLK_1));
                break;
            case SDLK_0:
                game_upgrade(&app.game, 0, 9);
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
static void read_actions(Actions actions[2], double dt) {
    memset(actions, 0, 2 * sizeof(*actions));
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
            game_look(&app.game.players[player],
                      axis(pad, SDL_GAMEPAD_AXIS_RIGHTX, player) * dt * 2.3 * v->stick,
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
    for (int i = 0; i < app.game.player_count; i++)
        if (app.game.players[i].map_open || app.game.matrix.divers[i].map)
            actions[i] = (Actions){0};
}
static void shutdown_app(void) {
    if (!app.no_save && !app.failed)
        save_game();
    input_devices_close(&app.devices);
    if (app.audio)
        SDL_DestroyAudioStream(app.audio);
    renderer_destroy(&app.renderer);
    game_destroy(&app.game);
    if (app.context)
        SDL_GL_DestroyContext(app.context);
    if (app.window)
        SDL_DestroyWindow(app.window);
    SDL_Quit();
}
static void frame(void *unused) {
    (void)unused;
    double now = (double)SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
    double dt = clampd(now - app.last, 0, .1);
    app.last = now;
    if (app.smoke_frames)
        dt = 1.0 / 60;
    app.fps = app.fps * .96 + (dt > 0 ? 1 / dt : 60) * .04;
    events();
    Vec3 stream_positions[2] = {position_absolute(app.game.players[0].pos),
                                position_absolute(app.game.players[1].pos)};
    world_refresh(&app.game.world, stream_positions, app.game.player_count);
    world_stream(&app.game.world, SC_STREAM_BUDGET);
    if (app.inspect_tentacles && !app.game.world.pending) {
        Player *p = &app.game.players[0];
        Cell focus = world_cell(position_absolute(p->pos));
        for (int i = 0; i < SC_MAX_CHUNKS && app.inspect_tentacles; i++) {
            const Chunk *c = &app.game.world.chunks[i];
            if (!c->active || !cell_equal(c->cell, focus))
                continue;
            for (int j = 0; j < c->dressing_count; j++) {
                const Dressing *d = &c->dressing[j];
                if (d->kind != GROWTH || d->normal.y < .9 || d->scale < .5)
                    continue;
                Vec3 root = vadd(world_center(c->cell), d->pos);
                double distance = clampd(d->scale * 3.5, 2.5, 6);
                Vec3 feet = vadd(root, v3(d->pos.x > 0 ? -distance * .5 : distance * .5,
                                          .02 - d->pos.y, d->pos.z > 0 ? -distance : distance));
                if (!world_spawn_supported(&app.game.world, feet, 1.8) ||
                    world_overlap(&app.game.world, feet, .35, 1.8))
                    continue;
                p->pos = position_from(feet);
                p->yaw = atan2(root.x - feet.x, root.z - feet.z);
                p->pitch = atan2(d->scale * .8 - 1.6, distance * 1.12);
                app.inspect_tentacles = false;
                if (app.game.player_count == 2) {
                    app.game.players[1].pos = p->pos;
                    app.game.players[1].yaw = p->yaw;
                    app.game.players[1].pitch = p->pitch;
                }
                break;
            }
        }
        app.inspect_tentacles = false;
    }
    if (app.inspect_matrix && matrix_near(&app.game, 0) && matrix_toggle(&app.game, 0))
        app.inspect_matrix = false;
    if (app.inspect_alien && !app.game.world.pending) {
        Room *room =
            world_find(&app.game.world, world_cell(position_absolute(app.game.players[0].pos)));
        if (room && room->ready) {
            for (int n = 0; n < app.game.world.count; n++)
                app.game.world.rooms[n].enemy_count = 0;
            room->enemy_count = 1;
            unsigned kind = 31 + (unsigned)app.inspect_alien;
            Vec3 at = vadd(world_center(room->cell), v3(0, .02, 12));
            room->enemies[0] = (Enemy){.kind = (uint8_t)kind,
                                       .behavior = (uint8_t)(ALIEN_LISTENER + kind - 32),
                                       .pos = position_from(at),
                                       .home = at,
                                       .health = sc_enemies[kind].health,
                                       .normal = {0, 1, 0},
                                       .heading = {0, 0, -1},
                                       .body_heading = {0, 0, -1},
                                       .alive = true};
            alien_fragments(&app.game.world, &room->enemies[0]);
            app.inspect_alien = 0;
        }
    }
    if (app.inspect_form && !app.game.world.pending) {
        Room *room = world_find(&app.game.world, (Cell){0, 0, 0});
        if (room) {
            room->enemy_count = 0;
            for (int kind = 0; kind < SC_LEGACY_ENEMIES; kind++)
                if ((kind < 25 || kind >= 27) &&
                    sc_enemy_bodies[kind].form == app.inspect_form - 1) {
                    int at = room->enemy_count++;
                    Enemy *e = &room->enemies[at];
                    *e = (Enemy){.pos = position_from(v3((at - 2) * 3.2, .02, 22)),
                                 .health = sc_enemies[kind].health,
                                 .kind = (uint8_t)kind,
                                 .id = (uint8_t)at,
                                 .normal = {0, 1, 0},
                                 .heading = {0, 0, -1},
                                 .alive = true};
                }
        }
        app.inspect_form = 0;
    }
    if (app.inspect_rig && world_ready(&app.game.world, (Cell){0, 0, 0})) {
        game_rig_toggle(&app.game, 0, (RigKind)app.inspect_rig);
        game_rig_interact(&app.game, 0);
        app.inspect_rig = 0;
    }
    if (app.inspect_shelter && world_ready(&app.game.world, (Cell){0, 0, 0})) {
        game_summon(&app.game, 0);
        game_interact(&app.game, 0);
        app.inspect_shelter = false;
    }
    if (app.demo_encounter) {
        Room *room = world_find(&app.game.world, (Cell){1, 0, 0});
        if (room && room->ready) {
            room->enemy_count = 3;
            const Vec3 positions[] = {{-8, .02, 5}, {8, .02, -8}, {0, .02, 12}};
            for (int i = 0; i < 3; i++) {
                Enemy *e = &room->enemies[i];
                *e = (Enemy){.pos = position_from(vadd(world_center(room->cell), positions[i])),
                             .health = 100,
                             .kind = (uint8_t)i,
                             .id = (uint8_t)i,
                             .normal = {0, 1, 0},
                             .heading = {0, 0, -1},
                             .alive = true};
            }
            room->enemies[0].pos = position_from(
                vadd(world_center(room->cell), v3(-(room->width * .5 - 1.1), .02, -20)));
            app.demo_encounter = false;
        }
    }
    if (!app.paused) {
        Actions actions[2];
        read_actions(actions, dt);
        app.accumulator += dt;
        while (app.accumulator >= 1.0 / 60) {
            game_step(&app.game, actions, 1.0 / 60);
            app.accumulator -= 1.0 / 60;
        }
        app.autosave += dt;
        if (app.autosave > 20) {
            save_game();
            app.autosave = 0;
        }
    }
    audio_frame();
#ifdef __EMSCRIPTEN__
    int status = browser_storage_status();
    if (status == 3)
        snprintf(app.save_status, sizeof(app.save_status), "SAVED IN THIS BROWSER");
    if (status == 4)
        snprintf(app.save_status, sizeof(app.save_status), "BROWSER SAVE FAILED / EXPORT A BACKUP");
    if (status == 0)
        snprintf(app.save_status, sizeof(app.save_status), "BROWSER STORAGE UNAVAILABLE");
#endif
    int w, h;
    SDL_GetWindowSizeInPixels(app.window, &w, &h);
    bool draw_pause = app.paused;
#ifdef __EMSCRIPTEN__
    draw_pause = app.paused && app.started;
#endif
    renderer_draw(&app.renderer, &app.game, w, h, draw_pause, app.touch, app.debug, app.fps,
                  app.save_status);
    if (app.options)
        renderer_options(&app.renderer, w, h, app.option_player, app.option_row);
    GLenum error = gl.GetError();
    if (error != GL_NO_ERROR || app.renderer.failed || app.game.world.failed) {
        SDL_Log("Frame failed: GL error 0x%x / renderer %d / mesh %d / world %d / vertices %u",
                error, app.renderer.failed, app.renderer.mesh.failed, app.game.world.failed,
                (unsigned)app.renderer.mesh.count);
        app.failed = true;
        app.running = false;
    }
    app.frame++;
    if (app.smoke_frames && app.frame >= app.smoke_frames) {
        if (app.capture && !renderer_capture(w, h, app.capture)) {
            SDL_Log("Capture failed");
            app.failed = true;
        }
        SDL_Log("SMOKE %s: %d frames, %d players, %d rooms, %u vertices",
                app.failed ? "FAIL" : "PASS", app.frame, app.game.player_count,
                app.game.world.count, (unsigned)app.renderer.world_vertices);
        app.running = false;
    }
    SDL_GL_SwapWindow(app.window);
#ifdef __EMSCRIPTEN__
    if (!app.running) {
        emscripten_cancel_main_loop();
        shutdown_app();
    }
#endif
}
int main(int argc, char **argv) {
    memset(&app, 0, sizeof(app));
    app.running = true;
    app.paused = true;
    input_devices_init(&app.devices);
    app.fps = 60;
    app.shot_sample = 1800;
    uint32_t seed = (uint32_t)time(NULL);
    bool fresh = false, split = false;
    const char *save_dir = NULL, *scene = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--seed") && i + 1 < argc) {
            char *end;
            errno = 0;
            unsigned long n = strtoul(argv[++i], &end, 0);
            if (errno || *end || n > UINT32_MAX) {
                fprintf(stderr, "Invalid seed\n");
                return 2;
            }
            seed = (uint32_t)n;
            fresh = true;
        } else if (!strcmp(argv[i], "--new"))
            fresh = true;
        else if (!strcmp(argv[i], "--split"))
            split = true;
        else if (!strcmp(argv[i], "--no-save"))
            app.no_save = true;
        else if (!strcmp(argv[i], "--touch"))
            app.touch = true;
        else if (!strcmp(argv[i], "--smoke") && i + 1 < argc) {
            app.smoke_frames = atoi(argv[++i]);
            app.no_save = true;
            app.paused = false;
            if (app.smoke_frames < 1)
                return 2;
        } else if (!strcmp(argv[i], "--capture") && i + 1 < argc)
            app.capture = argv[++i];
        else if (!strcmp(argv[i], "--save-dir") && i + 1 < argc)
            save_dir = argv[++i];
        else if (!strcmp(argv[i], "--scene") && i + 1 < argc) {
            scene = argv[++i];
            fresh = true;
        } else if (!strcmp(argv[i], "--help")) {
            puts("Starship Crawl\n  --seed NUMBER --new --no-save --save-dir DIRECTORY\n  --touch "
                 "--split --smoke FRAMES --capture FILE.ppm\n  --scene "
                 "arrival|hull|space|skiff|cabin|bay|hall|engine|crew|tunnel|atrium|distant|"
                 "encounter|shelter|mech|wagon|luxury|"
                 "bridge|lab|"
                 "cargo|"
                 "cryonics|botany|tentacles|relay|nest|local0..7|biome0..13\nSolo by default. F2 "
                 "explicitly "
                 "toggles local "
                 "split-screen.");
            return 0;
        } else {
            fprintf(stderr, "Unknown or incomplete argument: %s\n", argv[i]);
            return 2;
        }
    }
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("SDL: %s", SDL_GetError());
        return 1;
    }
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    save_dir = "/persist";
    app.touch = app.touch || browser_touch();
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    app.window =
        SDL_CreateWindow("Starship Crawl", 1280, 720,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!app.window) {
        SDL_Log("Window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    app.context = SDL_GL_CreateContext(app.window);
    if (!app.context) {
        SDL_Log("OpenGL: %s", SDL_GetError());
        SDL_DestroyWindow(app.window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_SetSwapInterval(1);
    if (!renderer_init(&app.renderer)) {
        app.failed = true;
        shutdown_app();
        return 1;
    }
    game_init(&app.game, seed);
    char *pref = NULL;
    if (!save_dir) {
        pref = SDL_GetPrefPath("Stbange", "StarshipCrawl");
        save_dir = pref;
    }
    if (save_dir && strlen(save_dir) + 20 < sizeof(app.save_path)) {
        SDL_CreateDirectory(save_dir);
        snprintf(app.save_path, sizeof(app.save_path), "%s/expedition.sav", save_dir);
        snprintf(app.preferences_path, sizeof(app.preferences_path), "%s/preferences.cfg",
                 save_dir);
        if (!scene && !app.no_save)
            preferences_load(&app.renderer.preferences, app.preferences_path);
        if (!fresh && !app.no_save)
            game_load(&app.game, app.save_path);
    }
    SDL_free(pref);
    if (scene) {
        bool court_scene = !strncmp(scene, "court", 5) || !strncmp(scene, "conduit", 7) ||
                           !strncmp(scene, "crown", 5);
        bool frontier_scene = !strncmp(scene, "front", 5) || !strncmp(scene, "deep", 4) ||
                              !strncmp(scene, "reveal", 6) || !strncmp(scene, "alien", 5);
        bool encounter_scene =
            !strcmp(scene, "nest") || !strcmp(scene, "relay") || !strncmp(scene, "local", 5);
        bool habitat_scene = !strncmp(scene, "biome", 5) || !strncmp(scene, "volume", 6) ||
                             !strncmp(scene, "shape", 5) || !strcmp(scene, "tentacles");
        bool traversal_scene =
            !strcmp(scene, "stairs") || !strcmp(scene, "ramp") || !strcmp(scene, "lift");
        bool exterior_scene = !strcmp(scene, "arrival") || !strcmp(scene, "skiff") ||
                              !strcmp(scene, "cabin") || !strcmp(scene, "space") ||
                              !strcmp(scene, "hull");
        /* Historical inspection scenes are reproducible generator-6 sandboxes.
           The guardian preview and ordinary new games use the phased ruleset. */
        if (strcmp(scene, "guardian") && !exterior_scene && !traversal_scene && !habitat_scene &&
            !encounter_scene && !frontier_scene && !court_scene) {
            game_new_ship(&app.game, seed);
            world_destroy(&app.game.world);
            world_init(&app.game.world, seed);
            app.game.world.generator = 6;
            app.game.campaign = (Campaign){0};
            for (int i = 0; i < SC_PLAYERS; i++)
                app.game.players[i].weapons = SC_START_WEAPONS;
        }
        politics_init(&app.game);
        app.game.message_time = 0;
        Cell c = {0, 0, 0};
        if (court_scene) {
            unsigned role = !strncmp(scene, "court", 5)     ? COURT_AUDIENCE
                            : !strncmp(scene, "conduit", 7) ? COURT_CONDUIT
                                                            : COURT_FINALE;
            unsigned nation = (unsigned)(scene[strlen(scene) - 1] - '0') % SC_COUNTRIES;
            c = politics_site(seed, nation, (CourtRole)role);
            app.game.campaign = (Campaign){0};
            app.game.world.open_layer = 0;
            app.no_save = true;
        }
        if (!strcmp(scene, "guardian")) {
            c = campaign_site(0);
            app.no_save = true;
        }
        if (!strcmp(scene, "hall"))
            c.z = 1;
        else if (!strcmp(scene, "engine"))
            c.x = 1;
        else if (!strcmp(scene, "encounter")) {
            c.x = 1;
            app.demo_encounter = true;
            app.no_save = true;
        } else if (!strcmp(scene, "crew"))
            c.x = -1;
        else if (!strcmp(scene, "tunnel"))
            c.y = -1;
        else if (!strcmp(scene, "atrium"))
            c.y = 1;
        else if (!strcmp(scene, "distant"))
            c = (Cell){3000000, 2000, 3000000};
        const char *sector_scenes[] = {"luxury", "bridge", "lab", "cargo", "cryonics", "botany"};
        const District sectors[] = {LUXURY, COMMAND, RESEARCH, LOGISTICS, CRYONICS, BOTANY};
        for (int sector = 0; sector < 6; sector++)
            if (!strcmp(scene, sector_scenes[sector])) {
                bool found = false;
                for (int y = 0; y < 9 && !found; y++)
                    for (int z = 0; z < 12 && !found; z++)
                        for (int x = -8; x < 8 && !found; x++) {
                            RoomSpec spec = world_describe_version(seed, app.game.world.generator,
                                                                   (Cell){x, y, z});
                            if (spec.district == sectors[sector] && spec.theme >= 6 &&
                                spec.span_x == 1) {
                                c = spec.cell;
                                found = true;
                            }
                        }
            }
        int inspect_layout = -1, inspect_incident = -1, inspect_landmark = -1, inspect_role = -1;
        if (!strcmp(scene, "origin"))
            inspect_role = HISTORY_ORIGIN;
        if (!strcmp(scene, "waystation"))
            inspect_role = HISTORY_SURVIVORS;
        if (!strcmp(scene, "controller"))
            inspect_role = HISTORY_CONTROL;
        if (!strncmp(scene, "layout", 6) && scene[6] >= '0' && scene[6] <= '2' && !scene[7])
            inspect_layout = scene[6] - '0';
        if (!strncmp(scene, "history", 7) && scene[7] >= '0' && scene[7] <= '5' && !scene[8])
            inspect_incident = scene[7] - '0';
        if (!strncmp(scene, "landmark", 8) && scene[8] >= '1' && scene[8] <= '4' && !scene[9])
            inspect_landmark = scene[8] - '0';
        bool interior_scene = inspect_layout >= 0 || inspect_incident >= 0 ||
                              inspect_landmark >= 0 || inspect_role >= 0;
        bool found = false;
        if (inspect_role == HISTORY_ORIGIN) {
            c = world_history(seed).origin;
            found = true;
        }
        for (int y = 0; interior_scene && y < 9 && !found; y++)
            for (int z = 0; z < 24 && !found; z++)
                for (int x = -12; x < 12 && !found; x++) {
                    RoomSpec spec =
                        world_describe_version(seed, app.game.world.generator, (Cell){x, y, z});
                    if (spec.type != BAY && spec.height >= 3 &&
                        (inspect_role < 0 || spec.history_role == inspect_role) &&
                        (inspect_layout < 0 ||
                         (spec.layout == inspect_layout && spec.span_x == 1)) &&
                        (inspect_incident < 0 ||
                         (spec.incident == inspect_incident && spec.span_x == 1)) &&
                        (inspect_landmark < 0 || spec.landmark == inspect_landmark)) {
                        c = spec.cell;
                        found = true;
                    }
                }
        app.game.players[0].pos =
            position_from(vadd(world_center(c), v3(0, .02, court_scene ? 0 : -25)));
        if (!strncmp(scene, "form", 4) && scene[4] >= '0' && scene[4] < '0' + SC_BODY_FORMS &&
            !scene[5]) {
            app.game.players[0].pos = position_from(v3(0, .02, 14));
            app.inspect_form = scene[4] - '0' + 1;
            app.no_save = true;
        }
        if (!strcmp(scene, "mech") || !strcmp(scene, "wagon")) {
            app.game.players[0].pos = position_from(v3(0, .02, 12));
            app.inspect_rig = !strcmp(scene, "mech") ? MECH : WAGON;
            app.no_save = true;
        }
        if (!strcmp(scene, "matrix")) {
            app.game.players[0].pos = position_from(matrix_terminal((Cell){0}));
            app.inspect_matrix = true;
            app.no_save = true;
        }
        if (!strcmp(scene, "shelter")) {
            app.game.players[0].pos = position_from(v3(0, .02, 12));
            app.inspect_shelter = true;
            app.no_save = true;
        }
        app.game.players[0].crouched =
            world_describe_version(seed, app.game.world.generator, c).type == CONDUIT;
        app.game.players[0].pitch = .03;
        if (interior_scene) {
            app.game.players[0].pos = position_from(vadd(world_center(c), v3(0, .02, 0)));
            app.game.players[0].yaw = inspect_role >= 0 ? -2.36 : .78;
            app.game.players[0].pitch = inspect_landmark >= 0 ? .26 : .015;
        }
        if (habitat_scene) {
            bool found = false;
            for (int y = -4; y <= 4 && !found; y++)
                for (int rz = 1; rz < 5 && !found; rz++)
                    for (int rx = -4; rx <= 4 && !found; rx++) {
                        RegionPlan plan;
                        world_plan_version(seed, SC_GENERATOR_VERSION, (Cell){rx, y, rz}, &plan);
                        for (int i = 0; i < plan.count && !found; i++) {
                            RoomSpec room = plan.rooms[i];
                            int desired = atoi(scene + (!strncmp(scene, "volume", 6) ? 6 : 5));
                            bool match =
                                !strcmp(scene, "tentacles")
                                    ? (room.incident == BLOOM || room.district == BOTANY) &&
                                          room.height >= 8 && !room.habitat.volume &&
                                          room.span_x == 1 && room.span_z == 1
                                : !strncmp(scene, "biome", 5)  ? room.habitat.atmosphere == desired
                                : !strncmp(scene, "volume", 6) ? room.habitat.volume == desired
                                                               : room.habitat.form == desired;
                            if (match && room.height >= 3) {
                                c = room.cell;
                                found = true;
                            }
                        }
                    }
            Player *p = &app.game.players[0];
            p->pos = position_from(vadd(world_center(c), v3(0, .02, 0)));
            p->checkpoint = position_from(world_lift(c));
            p->yaw = .4;
            p->pitch = .05;
            if (!strncmp(scene, "volume", 6)) {
                if (scene[6] == '2') {
                    p->pos = position_from(vadd(world_center(c), v3(9.2, .02, 24)));
                    p->yaw = SC_PI * .5;
                    p->pitch = -.8;
                } else {
                    p->pos = position_from(vadd(world_center(c), v3(0, .02, 18)));
                    p->yaw = .7;
                    p->pitch = .52;
                }
            }
            app.game.campaign = (Campaign){0};
            app.game.world.open_layer = 0;
            app.no_save = true;
            app.inspect_tentacles = !strcmp(scene, "tentacles");
        }
        if (encounter_scene) {
            app.game.world.generator = 11;
            bool found = false;
            RoomSpec selected_room = {0};
            bool local_scene = !strncmp(scene, "local", 5);
            for (int y = -3; y <= 3 && !found; y++)
                for (int z = 1; z <= 5 && !found; z++)
                    for (int x = -5; x <= 5 && !found; x++) {
                        RegionPlan plan;
                        world_plan_version(seed, app.game.world.generator, (Cell){x, y, z}, &plan);
                        for (int i = 0; i < plan.count && !found; i++) {
                            RoomSpec a = plan.rooms[i];
                            bool match = local_scene
                                             ? (a.habitat.pace == PACE_QUIET ||
                                                a.habitat.pace == PACE_TRACE) &&
                                                   a.habitat.tale == (unsigned)atoi(scene + 5)
                                             : a.habitat.pace == PACE_FRONTIER &&
                                                   (a.habitat.ecology == MECHANICAL) ==
                                                       !strcmp(scene, "relay");
                            if (match && a.height >= 5 && a.width >= 40 && !a.habitat.volume) {
                                selected_room = a;
                                c = a.cell;
                                found = true;
                            }
                        }
                    }
            Player *p = &app.game.players[0];
            p->pos = position_from(
                vadd(world_center(c),
                     local_scene ? v3(selected_room.hash & 1 ? 18 : -18, .02, 2) : v3(0, .02, 0)));
            p->checkpoint = p->pos;
            p->yaw = 0;
            p->pitch = -.03;
            app.game.campaign = (Campaign){0};
            app.game.world.open_layer = 0;
            app.no_save = true;
        }
        if (frontier_scene) {
            world_destroy(&app.game.world);
            world_init(&app.game.world, seed);
            app.game.world.generator = 12;
            bool found = false;
            RoomSpec chosen = {0};
            unsigned desired = (unsigned)atoi(scene + (!strncmp(scene, "reveal", 6) ? 6
                                                       : !strncmp(scene, "deep", 4) ? 4
                                                                                    : 5));
            for (int y = -4; y <= 4 && !found; y++)
                for (int z = 1; z <= 7 && !found; z++)
                    for (int x = -7; x <= 7 && !found; x++) {
                        RegionPlan plan;
                        world_plan_version(seed, 12, (Cell){x, y, z}, &plan);
                        for (int i = 0; i < plan.count && !found; i++) {
                            RoomSpec a = plan.rooms[i];
                            bool match = !strncmp(scene, "reveal", 6)
                                             ? a.habitat.reveal == desired && a.habitat.form
                                         : !strncmp(scene, "alien", 5)
                                             ? a.habitat.pace == PACE_QUIET && a.height >= 10
                                         : !strncmp(scene, "deep", 4)
                                             ? a.habitat.contract == desired + 1
                                             : a.habitat.pace == PACE_FRONTIER &&
                                                   a.habitat.challenge == desired;
                            if (match && a.height >= 10 && a.width >= 50 && !a.habitat.volume) {
                                chosen = a;
                                c = a.cell;
                                found = true;
                            }
                        }
                    }
            Player *p = &app.game.players[0];
            p->pos = position_from(vadd(world_center(c), v3(0, .02, 0)));
            p->checkpoint = p->pos;
            p->yaw = 0;
            p->pitch = -.02;
            if (!strncmp(scene, "reveal", 6)) {
                p->yaw = 0;
                p->pitch = atan2(fmax(10, chosen.height * .60) - 1.62, 26);
            }
            if (!strncmp(scene, "alien", 5) && desired < 10)
                app.inspect_alien = (int)desired + 1;
            app.game.campaign = (Campaign){0};
            app.game.world.open_layer = 0;
            app.no_save = true;
        }
        if (traversal_scene) {
            Player *p = &app.game.players[0];
            p->pos = position_from(!strcmp(scene, "lift")
                                       ? world_lift((Cell){0, 0, 0})
                                       : v3(!strcmp(scene, "stairs") ? 6 : -6, .02, 17));
            p->yaw = 0;
            p->pitch = .25;
            app.no_save = true;
        }
        if (exterior_scene) {
            Player *p = &app.game.players[0];
            p->pos = position_from(sc_arrival_spawn);
            p->pitch = -.04;
            p->crouched = false;
            if (!strcmp(scene, "skiff")) {
                p->pos = position_from(v3(18, 10, -68));
                p->yaw = atan2(-18, -30);
                p->pitch = -.15;
            } else if (!strcmp(scene, "cabin")) {
                p->pos = position_from(v3(0, 4, -97));
                p->yaw = SC_PI;
                p->pitch = -.08;
            } else if (!strcmp(scene, "space")) {
                p->yaw = SC_PI;
                p->pitch = .65;
            } else if (!strcmp(scene, "hull")) {
                p->yaw = .9;
                p->pitch = .75;
            }
            app.no_save = true;
        }
        politics_init(&app.game);
        Vec3 p = position_absolute(app.game.players[0].pos);
        world_refresh(&app.game.world, &p, 1);
    }
    if (split) {
        game_join(&app.game);
        if (scene && !strcmp(scene, "distant"))
            app.game.players[1].pos = position_from(v3(0, 4, -34));
    }
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    for (int i = 0; i < count; i++)
        open_pad(ids[i]);
    SDL_free(ids);
    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_AudioSpec spec = {.format = SDL_AUDIO_F32, .channels = 1, .freq = 22050};
        app.audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        if (app.audio)
            SDL_ResumeAudioStreamDevice(app.audio);
    }
    app.last = (double)SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
#ifdef __EMSCRIPTEN__
    browser_ready();
    emscripten_set_main_loop_arg(frame, NULL, 0, 1);
#else
    while (app.running)
        frame(NULL);
    shutdown_app();
#endif
    return app.failed ? 1 : 0;
}
