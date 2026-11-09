#define SDL_MAIN_HANDLED
#include "platform_input.h"
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__, #x, SDL_GetError());           \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

static void settings(void) {
    const char *path = "platform-preferences-test.cfg";
    Preferences p, loaded;
    preferences_default(&p);
    CHECK(preferences_height(&p, 0, false) == 360);
    CHECK(preferences_height(&p, 0, true) == 240);
    CHECK(!preferences_set(&p, -1, PREF_MOUSE, 1));
    CHECK(!preferences_set(&p, 2, PREF_MOUSE, 1));
    CHECK(!preferences_set(&p, 0, PREF_COUNT, 1));
    CHECK(!preferences_set(&p, 0, PREF_MOUSE, NAN));
    CHECK(!preferences_set(&p, 0, PREF_MOUSE, INFINITY));
    CHECK(preferences_set(&p, 0, PREF_MOUSE, 100));
    CHECK(preferences_get(&p, 0, PREF_MOUSE) == 3);
    CHECK(preferences_set(&p, 0, PREF_TOUCH, -100));
    CHECK(preferences_get(&p, 0, PREF_TOUCH) == .25);
    CHECK(preferences_set(&p, 0, PREF_STICK, 1.75));
    CHECK(preferences_set(&p, 0, PREF_DEADZONE, .24));
    CHECK(preferences_set(&p, 0, PREF_INVERT, 1));
    CHECK(preferences_set(&p, 0, PREF_QUALITY, 1));
    CHECK(preferences_set(&p, 1, PREF_QUALITY, 4));
    CHECK(preferences_set(&p, 0, PREF_GUIDANCE, 2));
    CHECK(preferences_set(&p, 1, PREF_GUIDANCE, 1));
    CHECK(preferences_get(&p, 0, PREF_GUIDANCE) == 2);
    CHECK(preferences_height(&p, 0, false) == 180);
    CHECK(preferences_height(&p, 1, true) == 480);
    CHECK(preferences_save(&p, path));
    CHECK(preferences_save(&p, path)); /* Atomic replacement, including Windows. */
    CHECK(preferences_load(&loaded, path));
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < PREF_COUNT; j++)
            CHECK(fabs(preferences_get(&p, i, j) - preferences_get(&loaded, i, j)) < .00001);
    CHECK(!preferences_save(&p, ""));
    CHECK(!preferences_save(&p, "no-such-settings-directory/settings.cfg"));
    CHECK(!preferences_load(&loaded, NULL));
    CHECK(loaded.view[0].mouse == 1);
    FILE *legacy = fopen(path, "w");
    CHECK(legacy && fputs("SC_PREFS 1\n1.25 1 1 .16 0 2\n1 1 1 .2 1 3\n", legacy) >= 0);
    CHECK(!fclose(legacy) && preferences_load(&loaded, path));
    CHECK(loaded.view[0].guidance == 0 && loaded.view[0].mouse == 1.25f && loaded.view[1].inverted);
    const char *invalid[] = {"SC_PREFS 99\n", "SC_PREFS 1\n1 1 nan .16 0 0\n1 1 1 .16 0 0\n",
                             "SC_PREFS 1\n1 1 1 .16 0 0\n",
                             "SC_PREFS 1\n1 1 1 .16 0 0\n1 1 1 .16 0 0\njunk"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); i++) {
        FILE *f = fopen(path, "w");
        CHECK(f && fputs(invalid[i], f) >= 0);
        CHECK(fclose(f) == 0);
        CHECK(!preferences_load(&loaded, path));
        CHECK(loaded.view[0].mouse == 1 && loaded.view[1].quality == 0);
    }
    CHECK(remove(path) == 0);
    CHECK(!preferences_load(&loaded, path));
    CHECK(input_axis(0, .16f) == 0 && input_axis(4000, .16f) == 0);
    CHECK(input_axis(32767, .16f) == 1 && input_axis(-32768, .16f) == -1);
    CHECK(input_axis(20000, NAN) > 0 && input_axis(-20000, 2) < 0);
}
static SDL_JoystickID attach(const char *name) {
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1;
    desc.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
    desc.name = name;
    SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
    CHECK(id && SDL_IsGamepad(id));
    return id;
}
static void controllers(void) {
    InputDevices d;
    input_devices_init(&d);
    SDL_JoystickID a = attach("Starship virtual A"), b = attach("Starship virtual B"),
                   c = attach("Starship virtual C");
    CHECK(input_devices_add(&d, a, 1));
    CHECK(input_devices_add(&d, a, 1));
    CHECK(input_devices_add(&d, b, 1));
    CHECK(!input_devices_add(&d, c, 1));
    input_devices_players(&d, 2);
    CHECK(d.owner[0] == 0 && d.owner[1] == 1);
    SDL_Joystick *joystick = SDL_GetGamepadJoystick(d.pads[0]);
    CHECK(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_LEFTX, 24576));
    CHECK(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true));
    SDL_UpdateJoysticks();
    SDL_PumpEvents();
    CHECK(input_axis(SDL_GetGamepadAxis(d.pads[0], SDL_GAMEPAD_AXIS_LEFTX), .16f) > .65f);
    CHECK(SDL_GetGamepadButton(d.pads[0], SDL_GAMEPAD_BUTTON_SOUTH));
    CHECK(input_devices_remove(&d, b));
    input_devices_players(&d, 2);
    CHECK(d.owner[0] == 0); /* A must not silently become player two. */
    CHECK(!input_devices_remove(&d, b));
    CHECK(input_devices_add(&d, b, 2));
    CHECK(d.owner[1] == 1);
    CHECK(input_devices_remove(&d, a));
    input_devices_players(&d, 2);
    CHECK(d.owner[1] == 1);
    CHECK(input_devices_add(&d, a, 2));
    CHECK(d.owner[0] == 0);
    input_devices_players(&d, 1);
    CHECK(d.owner[0] == 0 && d.owner[1] == 0);
    CHECK(input_devices_remove(&d, a));
    input_devices_players(&d, 2);
    CHECK(d.owner[1] == 1); /* Explicit co-op join: keyboard P1, one pad P2. */
    CHECK(input_devices_add(&d, a, 2));
    CHECK(d.owner[0] == 0 && d.owner[1] == 1);
    input_devices_close(&d);
    CHECK(!d.pads[0] && !d.pads[1]);
    CHECK(!input_devices_add(&d, 0, 1));
    CHECK(SDL_DetachVirtualJoystick(a));
    CHECK(SDL_DetachVirtualJoystick(b));
    CHECK(SDL_DetachVirtualJoystick(c));
}
int main(void) {
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    CHECK(SDL_Init(SDL_INIT_GAMEPAD));
    settings();
    controllers();
    SDL_Quit();
    puts("PASS: preferences and actual SDL virtual-controller input/ownership");
    return 0;
}
