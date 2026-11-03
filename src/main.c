#define SDL_MAIN_HANDLED
#include "render.h"
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
    SDL_Gamepad *pads[2];
    Game game;
    Renderer renderer;
    Finger fingers[10];
    bool running, paused, started, touch, debug, new_armed, no_save, failed, mouse_fire;
    bool demo_encounter, inspect_shelter;
    int frame, smoke_frames, inspect_form;
    double last, accumulator, fps, autosave, audio_time;
    Uint64 look_after;
    bool discard_look;
    float ambience[DISTRICT_TYPES];
    uint32_t audio_shots, audio_noise;
    int shot_sample;
    char save_path[1024], save_status[96];
    const char *capture;
} App;
static App app;

static void set_pause(bool paused) {
    app.paused = paused;
    app.mouse_fire = false;
    memset(app.fingers, 0, sizeof(app.fingers));
    app.accumulator = 0;
    if (app.window && !app.touch)
        SDL_SetWindowRelativeMouseMode(app.window, !paused);
    if (!paused) {
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
    for (int i = 0; i < SC_PLAYERS; i++)
        app.game.players[i].journal = false;
    app.new_armed = false;
    set_pause(false);
}
EMSCRIPTEN_KEEPALIVE void sc_web_capture(void) {
    /* Chromium can deliver its centering warp well after the capture promise. */
    SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION);
    app.discard_look = true;
}
EMSCRIPTEN_KEEPALIVE void sc_web_look(double dx, double dy) {
    if (app.paused || app.touch || app.game.players[0].map_open || !isfinite(dx) || !isfinite(dy) ||
        SDL_GetTicksNS() < app.look_after || (dx == 0 && dy == 0))
        return;
    if (app.discard_look) {
        app.discard_look = false;
        return;
    }
    game_look(&app.game.players[0], dx * .0023, -dy * .0023);
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
EMSCRIPTEN_KEEPALIVE void sc_web_action(int action) {
    if (action >= 60 && action < 60 + SC_PSIONICS)
        game_psionic(&app.game, 0, (Psionic)(action - 60));
    else if (action >= 30 && action < 36) {
        const int pan[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}};
        game_map_pan(&app.game, 0, pan[action - 30][0], pan[action - 30][1], pan[action - 30][2]);
    } else if (action == 14)
        game_map_toggle(&app.game, 0);
    else if (action >= 100 && action < 112) {
        Player *p = &app.game.players[0];
        if (p->logs & (1u << (action - 100))) {
            p->reading = (uint8_t)(action - 100);
            p->journal = true;
            set_pause(true);
        }
    } else if (action == 12) {
        Player *p = &app.game.players[0];
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
        if (p->weapons & (1u << gun)) {
            p->weapon = gun;
            p->cooldown = .15f;
            app.game.dirty = true;
        }
    } else if (action == 13)
        game_summon(&app.game, 0);
    else if (action == 8)
        game_interact(&app.game, 0);
    else if (action == 9)
        game_weapon(&app.game, 0, 1);
    else if (action == 10)
        game_weapon(&app.game, 0, -1);
    else if (action == 11) {
        app.game.players[0].journal = !app.game.players[0].journal;
        set_pause(app.game.players[0].journal);
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
    static char result[3072];
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
    Room *room = world_find(&app.game.world, world_cell(feet));
    snprintf(
        result, sizeof(result),
        "{\"seed\":%u,\"players\":%d,\"paused\":%s,\"frames\":%d,\"rooms\":%d,"
        "\"x\":%.5f,\"y\":%.5f,\"z\":%.5f,\"rank\":%u,\"kills\":%u,\"shots\":%u,\"failed\":%s,"
        "\"generator\":%u,\"chunks\":%d,\"pending\":%d,\"generated\":%u,\"work\":%u,\"uploads\":%u,"
        "\"span\":%d,\"ready\":%s,\"fingerprint\":%u,\"flashlight\":%s,\"projectiles\":%u,\"enemy_"
        "shots\":%u,\"projectile_hits\":%u,\"climbers\":%u,\"mission\":%u,\"weapon\":%u,"
        "\"weapons\":%u,\"shield\":%.2f,\"armor\":%.2f,\"energy\":%.2f,\"credits\":%u,\"pickups\":%"
        "u,\"contracts\":%u,\"theme\":%u,\"logs\":%u,\"yaw\":%.4f,\"pitch\":%.4f,\"time\":%.3f,"
        "\"sheltered\":%s,\"shelter_deployed\":%s,\"sx\":%.3f,\"sz\":%.3f,\"resting\":%.2f,"
        "\"nourished\":%.2f,\"health\":%.2f,\"facility\":%d,"
        "\"map\":%s,\"map_x\":%d,\"map_y\":%d,\"map_z\":%d,\"veil\":%s,\"flight\":%.2f,\"leap\":%."
        "2f,\"nova\":%.2f,\"power\":%u,\"bodies\":%u,\"key_w\":%u}",
        app.game.world.seed, app.game.player_count, app.paused ? "true" : "false", app.frame,
        app.game.world.count, feet.x, feet.y, feet.z, p->level, p->kills, app.game.shots,
        app.failed ? "true" : "false", app.game.world.generator, app.game.world.chunk_count,
        app.game.world.pending, app.game.world.generated, app.game.world.last_work,
        app.renderer.last_uploads, room ? room->span_x * room->span_z : 0,
        room && room->ready ? "true" : "false",
        world_fingerprint(app.game.world.seed, world_region(world_cell(feet))),
        p->flashlight ? "true" : "false", projectiles, app.game.enemy_shots,
        app.game.projectile_hits, climbers, app.game.mission, p->weapon, p->weapons, p->shield,
        p->armor, p->energy, p->credits, p->pickups, app.game.contracts, room ? room->theme : 0,
        p->logs, p->yaw, p->pitch, app.game.time, p->sheltered ? "true" : "false",
        app.game.shelter.deployed ? "true" : "false", p->shelter_pos.x, p->shelter_pos.z,
        p->resting, p->nourished, p->health, (int)game_shelter_near(&app.game, 0),
        p->map_open ? "true" : "false", p->map_center.x, p->map_center.y, p->map_center.z,
        p->cloaked ? "true" : "false", p->flight, p->leap, p->nova, p->selected_power, bodies,
        (unsigned)SDL_GetKeyboardState(NULL)[SDL_SCANCODE_W]);
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
    for (int i = 0; i < 2; i++)
        if (app.pads[i] && SDL_GetGamepadID(app.pads[i]) == id)
            return;
    for (int i = 0; i < 2; i++)
        if (!app.pads[i]) {
            app.pads[i] = SDL_OpenGamepad(id);
            return;
        }
}
static int pad_player(int slot) {
    return app.game.player_count == 2 ? (app.pads[1] ? slot : 1) : 0;
}
static float axis(SDL_Gamepad *pad, SDL_GamepadAxis axis_id) {
    float v = SDL_GetGamepadAxis(pad, axis_id) / 32767.0f;
    return fabsf(v) < .16f ? 0 : (v > 0 ? (v - .16f) / .84f : (v + .16f) / .84f);
}
static int finger_role(float x, float y) {
    const float xs[] = {.90f, .77f, .64f, .90f, .77f, .64f, .90f, .77f, .64f, .51f, .51f, .51f},
                ys[] = {.72f, .85f, .85f, .48f, .48f, .48f, .27f, .27f, .27f, .27f, .48f, .1f};
    int w, h;
    SDL_GetWindowSize(app.window, &w, &h);
    float radius = fminf((float)w, (float)h) * .075f;
    if (app.game.players[0].map_open)
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
                if (app.fingers[i].role == 7)
                    game_flashlight(&app.game, 0);
                if (app.fingers[i].role == 8)
                    game_interact(&app.game, 0);
                if (app.fingers[i].role == 9)
                    game_weapon(&app.game, 0, 1);
                if (app.fingers[i].role == 10)
                    game_summon(&app.game, 0);
                if (app.fingers[i].role == 11)
                    game_psionic(&app.game, 0, (Psionic)app.game.players[0].selected_power);
                if (app.fingers[i].role == 12)
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
                    if (f->role == 1 && !app.paused && !app.game.players[0].map_open)
                        game_look(&app.game.players[0], (e->x - f->x) * 3.5, -(e->y - f->y) * 2.8);
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
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            set_pause(true);
            break;
        case SDL_EVENT_MOUSE_MOTION:
#ifndef __EMSCRIPTEN__
            if (!app.paused && e.motion.which != SDL_TOUCH_MOUSEID &&
                e.motion.timestamp >= app.look_after) {
                if (app.discard_look) {
                    app.discard_look = false;
                    break;
                }
                game_look(&app.game.players[0], e.motion.xrel * .0023, -e.motion.yrel * .0023);
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
            for (int i = 0; i < 2; i++)
                if (app.pads[i] && SDL_GetGamepadID(app.pads[i]) == e.gdevice.which) {
                    SDL_CloseGamepad(app.pads[i]);
                    app.pads[i] = NULL;
                    set_pause(true);
                }
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            for (int i = 0; i < 2; i++)
                if (app.pads[i] && SDL_GetGamepadID(app.pads[i]) == e.gbutton.which) {
                    int player = pad_player(i);
                    Player *p = &app.game.players[player];
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
                    if (SDL_GetGamepadAxis(app.pads[i], SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 16000) {
                        int b = e.gbutton.button;
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
                        game_interact(&app.game, pad_player(i));
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
            if (e.wheel.y != 0)
                game_weapon(&app.game, 0, e.wheel.y > 0 ? 1 : -1);
            break;
        case SDL_EVENT_KEY_DOWN:
            if (e.key.repeat)
                break;
            switch (e.key.key) {
            case SDLK_ESCAPE:
                if (app.game.players[0].map_open) {
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
                if (app.game.players[0].map_open) {
                    game_map_toggle(&app.game, 0);
                    game_map_toggle(&app.game, 0);
                }
                break;
            case SDLK_R:
                game_summon(&app.game, 0);
                break;
            case SDLK_G:
                game_interact(&app.game, 0);
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
                game_message(&app.game, "ENTER CONFIRMS A NEW SHIP / ESC CANCELS");
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
        if (app.pads[i]) {
            if (app.game.player_count == 1 && i == 1 && app.pads[0])
                continue;
            SDL_Gamepad *pad = app.pads[i];
            int player = pad_player(i);
            a = &actions[player];
            a->strafe += axis(pad, SDL_GAMEPAD_AXIS_LEFTX);
            a->forward -= axis(pad, SDL_GAMEPAD_AXIS_LEFTY);
            game_look(&app.game.players[player], axis(pad, SDL_GAMEPAD_AXIS_RIGHTX) * dt * 2.3,
                      -axis(pad, SDL_GAMEPAD_AXIS_RIGHTY) * dt * 1.8);
            a->fire |= SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 8000;
            a->jump |= SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH);
            a->crouch |= SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST);
            a->lift_up |= SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_WEST);
            a->lift_down |= SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_NORTH);
            a->sprint |= SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
        }
    for (int i = 0; i < app.game.player_count; i++)
        if (app.game.players[i].map_open)
            actions[i] = (Actions){0};
}
static void shutdown_app(void) {
    if (!app.no_save && !app.failed)
        save_game();
    for (int i = 0; i < 2; i++)
        if (app.pads[i])
            SDL_CloseGamepad(app.pads[i]);
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
    if (app.inspect_form && !app.game.world.pending) {
        Room *room = world_find(&app.game.world, (Cell){0, 0, 0});
        if (room) {
            room->enemy_count = 0;
            for (int kind = 0; kind < 25; kind++)
                if (sc_enemy_bodies[kind].form == app.inspect_form - 1) {
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
                vadd(world_center(room->cell), v3(-(room->width * .5 - 13.6), .02, -20)));
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
    GLenum error = gl.GetError();
    if (error != GL_NO_ERROR || app.renderer.failed || app.game.world.failed) {
        SDL_Log("Frame failed: GL error 0x%x", error);
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
                 "bay|hall|engine|crew|tunnel|atrium|distant|encounter|shelter|luxury|bridge|lab|"
                 "cargo|"
                 "cryonics|botany\nSolo by default. F2 "
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
        if (!fresh && !app.no_save)
            game_load(&app.game, app.save_path);
    }
    SDL_free(pref);
    if (scene) {
        app.game.message_time = 0;
        Cell c = {0, 0, 0};
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
                            RoomSpec spec = world_describe(seed, (Cell){x, y, z});
                            if (spec.district == sectors[sector] && spec.theme >= 6 &&
                                spec.span_x == 1) {
                                c = spec.cell;
                                found = true;
                            }
                        }
            }
        app.game.players[0].pos = position_from(vadd(world_center(c), v3(0, .02, -25)));
        if (!strncmp(scene, "form", 4) && scene[4] >= '0' && scene[4] <= '4' && !scene[5]) {
            app.game.players[0].pos = position_from(v3(0, .02, 14));
            app.inspect_form = scene[4] - '0' + 1;
            app.no_save = true;
        }
        if (!strcmp(scene, "shelter")) {
            app.game.players[0].pos = position_from(v3(0, .02, 12));
            app.inspect_shelter = true;
            app.no_save = true;
        }
        app.game.players[0].crouched = world_describe(seed, c).type == CONDUIT;
        app.game.players[0].pitch = .03;
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
