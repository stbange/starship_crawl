#ifndef SC_PLATFORM_INPUT_H
#define SC_PLATFORM_INPUT_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "identity.h"

enum {
    PREF_MOUSE,
    PREF_TOUCH,
    PREF_STICK,
    PREF_DEADZONE,
    PREF_INVERT,
    PREF_QUALITY,
    PREF_GUIDANCE,
    PREF_COUNT
};
typedef struct {
    float mouse, touch, stick, deadzone;
    bool inverted;
    unsigned quality, guidance;
} ViewPreferences;
typedef struct {
    ViewPreferences view[SC_LOCAL_VIEWS];
} Preferences;
typedef struct {
    SDL_Gamepad *pads[SC_LOCAL_VIEWS];
    int owner[SC_LOCAL_VIEWS], players;
} InputDevices;

void preferences_default(Preferences *p);
bool preferences_set(Preferences *p, int player, int setting, double value);
double preferences_get(const Preferences *p, int player, int setting);
bool preferences_load(Preferences *p, const char *path);
bool preferences_save(const Preferences *p, const char *path);
int preferences_height(const Preferences *p, int player, bool touch);
const char *preferences_quality(unsigned quality);
void input_devices_init(InputDevices *d);
void input_devices_players(InputDevices *d, int players);
bool input_devices_add(InputDevices *d, SDL_JoystickID id, int players);
bool input_devices_remove(InputDevices *d, SDL_JoystickID id);
void input_devices_close(InputDevices *d);
float input_axis(Sint16 raw, float deadzone);
#endif
