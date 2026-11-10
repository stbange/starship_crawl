#define SDL_MAIN_HANDLED
#include "app_internal.h"
#include <SDL3/SDL_main.h>

App app;

void shutdown_app(void) {
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
void frame(void *unused) {
    (void)unused;
#ifdef SC_WEBXR
    if (quest_graphics_lost)
        return;
#endif
    double now = (double)SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
    double dt = clampd(now - app.last, 0, .1);
    app.last = now;
    if (app.smoke_frames)
        dt = 1.0 / 60;
#ifdef SC_WEBXR
    if (quest_running)
        dt = quest_dt;
#endif
    app.fps = app.fps * .96 + (dt > 0 ? 1 / dt : 60) * .04;
#ifdef SC_WEBXR
    /* The Quest launch canvas is a paused preview. SDL sees the same physical
       controllers as WebXR on some browsers; never replay its queued inputs. */
    (void)events;
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
#else
    events();
#endif
    session_sync(&app.session);
    if (app.paused)
        session_advance(&app.session, 0, true);
    app_inspect_frame();
    for (int i = 0; i < SC_LOCAL_VIEWS; i++) {
        app.game.views[i].guide_mode = app.renderer.preferences.view[i].guidance;
        app.game.views[i].guide_device = app.touch ? GUIDE_TOUCH : GUIDE_KEYBOARD;
        for (int pad = 0; pad < 2; pad++)
            if (app.devices.pads[pad] && app.devices.owner[pad] == i)
                app.game.views[i].guide_device = GUIDE_PAD;
#ifdef SC_WEBXR
        if (i == 0)
            app.game.views[i].guide_device = GUIDE_QUEST;
#endif
    }
    if (!app.paused) {
        Actions actions[SC_LOCAL_VIEWS];
        read_actions(actions, dt);
        for (unsigned view = 0; view < app.session.views.count; view++) {
            PlayerId player = app.session.views.players[view];
            session_input(&app.session, player, &actions[view]);
        }
        session_advance(&app.session, dt, false);
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
#ifdef SC_WEBXR
    if (quest_running) {
        app.frame++;
        return;
    }
#endif
    int w, h;
    SDL_GetWindowSizeInPixels(app.window, &w, &h);
    bool draw_pause = app.paused;
#ifdef __EMSCRIPTEN__
    draw_pause = app.paused && app.started;
#endif
    if (app.inspect_static)
        draw_pause = false;
    renderer_draw(&app.renderer, &app.game, &app.session.views, w, h, draw_pause, app.touch,
                  app.debug, app.fps, app.save_status);
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
#ifdef SC_WEBXR
    save_dir = "/quest-persist";
    app.touch = true; /* The preview never competes for browser pointer lock. */
#endif
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
    session_init(&app.session, &app.game);
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
    if (scene)
        app_inspect_scene(scene, seed);
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
        SDL_AudioSpec spec = {.format = SDL_AUDIO_F32, .channels = 2, .freq = 22050};
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
