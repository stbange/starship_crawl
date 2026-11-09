#ifndef SC_APP_INTERNAL_H
#define SC_APP_INTERNAL_H
/* Private SDL/browser application interfaces. The core never includes this header. */
#include "render.h"
#include "session.h"
#include "exterior.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
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
    Session session;
    Renderer renderer;
    Finger fingers[10];
    bool running, paused, started, touch, debug, new_armed, no_save, failed, mouse_fire;
    bool demo_encounter, inspect_shelter, inspect_matrix, inspect_tentacles;
    int frame, smoke_frames, inspect_form, inspect_rig, inspect_alien;
    bool options;
    int option_player, option_row;
    double last, fps, autosave, audio_time;
    Uint64 look_after;
    bool discard_look;
    float ambience[DISTRICT_TYPES];
    uint32_t audio_shots, audio_noise;
    int shot_sample;
    char save_path[1024], preferences_path[1024], save_status[96];
    const char *capture;
} App;
extern App app;
bool app_command(int player, CommandKind kind, int argument, int auxiliary);
void app_look(int player, double yaw, double pitch);
void app_aim(int player, double yaw, double pitch);
void set_pause(bool paused);
uint32_t new_seed(void);
void save_game(void);
bool save_preferences(void);
void options_open(int player);
void options_adjust(int direction);
void interact_player(int player);
bool choose_player(int player, unsigned choice);
void phase_journal(int player);
void matrix_journal(int player);
void audio_frame(void);
void open_pad(SDL_JoystickID id);
void events(void);
void read_actions(Actions actions[SC_LOCAL_VIEWS], double dt);
void shutdown_app(void);
void frame(void *unused);
void app_inspect_frame(void);
void app_inspect_scene(const char *scene, uint32_t seed);
void sc_web_start(void);
void sc_web_capture(void);
void sc_web_look(double dx, double dy);
void sc_web_reset_input(void);
int sc_web_setting(int player, int setting, double value);
const char *sc_web_preferences(void);
void sc_web_save(void);
int sc_web_import(void);
void sc_web_action(int action);
uint32_t sc_web_fingerprint(uint32_t seed, int x, int y, int z);
const char *sc_web_state(void);
#ifdef __EMSCRIPTEN__
void browser_flush(void);
int browser_storage_status(void);
int browser_touch(void);
void browser_pointer(int enabled);
void browser_ready(void);
#endif
#ifdef SC_WEBXR
extern bool quest_running, quest_graphics_lost;
extern double quest_dt;
extern Actions quest_actions;
#endif
#endif
