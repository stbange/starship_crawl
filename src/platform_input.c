#include "platform_input.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void preferences_default(Preferences *p) {
    for (int i = 0; i < 2; i++)
        p->view[i] = (ViewPreferences){1, 1, 1, .16f, false, 0};
}
bool preferences_set(Preferences *p, int player, int setting, double value) {
    if (player < 0 || player > 1 || setting < 0 || setting >= PREF_COUNT || !isfinite(value))
        return false;
    ViewPreferences *v = &p->view[player];
    float bounded = (float)fmax(.25, fmin(value, 3));
    switch (setting) {
    case PREF_MOUSE:
        v->mouse = bounded;
        break;
    case PREF_TOUCH:
        v->touch = bounded;
        break;
    case PREF_STICK:
        v->stick = bounded;
        break;
    case PREF_DEADZONE:
        v->deadzone = (float)fmax(.05, fmin(value, .4));
        break;
    case PREF_INVERT:
        v->inverted = value >= .5;
        break;
    case PREF_QUALITY:
        v->quality = (unsigned)fmax(0, fmin(round(value), 4));
        break;
    }
    return true;
}
double preferences_get(const Preferences *p, int player, int setting) {
    if (player < 0 || player > 1 || setting < 0 || setting >= PREF_COUNT)
        return 0;
    const ViewPreferences *v = &p->view[player];
    switch (setting) {
    case PREF_MOUSE:
        return v->mouse;
    case PREF_TOUCH:
        return v->touch;
    case PREF_STICK:
        return v->stick;
    case PREF_DEADZONE:
        return v->deadzone;
    case PREF_INVERT:
        return v->inverted;
    default:
        return v->quality;
    }
}
bool preferences_load(Preferences *p, const char *path) {
    Preferences next;
    preferences_default(p);
    preferences_default(&next);
    if (!path || !*path)
        return false;
    FILE *f = fopen(path, "r");
    if (!f)
        return false;
    unsigned version = 0;
    bool ok = fscanf(f, "SC_PREFS %u", &version) == 1 && version == 1;
    for (int i = 0; ok && i < 2; i++) {
        double values[PREF_COUNT];
        for (int j = 0; ok && j < PREF_COUNT; j++)
            ok = fscanf(f, "%lf", &values[j]) == 1 && preferences_set(&next, i, j, values[j]);
    }
    char extra;
    ok = ok && fscanf(f, " %c", &extra) == EOF && !ferror(f);
    if (fclose(f))
        ok = false;
    if (ok)
        *p = next;
    return ok;
}
bool preferences_save(const Preferences *p, const char *path) {
    if (!path || !*path)
        return false;
    size_t n = strlen(path) + 5;
    char *temporary = malloc(n);
    if (!temporary)
        return false;
    snprintf(temporary, n, "%s.tmp", path);
    FILE *f = fopen(temporary, "w");
    bool ok = f != NULL;
    if (f) {
        ok = fprintf(f, "SC_PREFS 1\n") > 0;
        for (int i = 0; i < 2; i++) {
            const ViewPreferences *v = &p->view[i];
            if (fprintf(f, "%.6g %.6g %.6g %.6g %u %u\n", v->mouse, v->touch, v->stick, v->deadzone,
                        (unsigned)v->inverted, v->quality) < 0)
                ok = false;
        }
        if (fflush(f))
            ok = false;
        if (fclose(f))
            ok = false;
        if (ok)
            ok = SDL_RenamePath(temporary, path);
    }
    if (!ok)
        remove(temporary);
    free(temporary);
    return ok;
}
int preferences_height(const Preferences *p, int player, bool touch) {
    const int heights[] = {0, 180, 240, 360, 480};
    unsigned quality = player >= 0 && player < 2 ? p->view[player].quality : 0;
    return quality > 0 && quality < 5 ? heights[quality] : (touch ? 240 : 360);
}
const char *preferences_quality(unsigned quality) {
    static const char *names[] = {"AUTO", "LOW / 180P", "BALANCED / 240P", "HIGH / 360P",
                                  "ULTRA / 480P"};
    return names[quality < 5 ? quality : 0];
}
void input_devices_init(InputDevices *d) {
    memset(d, 0, sizeof(*d));
    d->owner[0] = d->owner[1] = -1;
    d->players = 1;
}
void input_devices_players(InputDevices *d, int players) {
    players = players == 2 ? 2 : 1;
    if (d->players == players)
        return;
    d->players = players;
    for (int i = 0; i < 2; i++)
        d->owner[i] = players == 1 ? 0 : (d->pads[0] && d->pads[1] ? i : 1);
}
bool input_devices_add(InputDevices *d, SDL_JoystickID id, int players) {
    input_devices_players(d, players);
    for (int i = 0; i < 2; i++)
        if (d->pads[i] && SDL_GetGamepadID(d->pads[i]) == id)
            return true;
    for (int i = 0; i < 2; i++)
        if (!d->pads[i]) {
            SDL_Gamepad *pad = SDL_OpenGamepad(id);
            if (!pad)
                return false;
            d->pads[i] = pad;
            d->owner[i] = d->players == 1    ? 0
                          : d->pads[1 - i]   ? 1 - d->owner[1 - i]
                          : d->owner[i] >= 0 ? d->owner[i]
                                             : 1;
            return true;
        }
    return false;
}
bool input_devices_remove(InputDevices *d, SDL_JoystickID id) {
    for (int i = 0; i < 2; i++)
        if (d->pads[i] && SDL_GetGamepadID(d->pads[i]) == id) {
            SDL_CloseGamepad(d->pads[i]);
            d->pads[i] = NULL;
            return true;
        }
    return false;
}
void input_devices_close(InputDevices *d) {
    for (int i = 0; i < 2; i++)
        if (d->pads[i])
            SDL_CloseGamepad(d->pads[i]);
    input_devices_init(d);
}
float input_axis(Sint16 raw, float deadzone) {
    float v = fmaxf(-1, raw / 32767.0f);
    deadzone = isfinite(deadzone) ? fmaxf(.05f, fminf(deadzone, .4f)) : .16f;
    return fabsf(v) <= deadzone ? 0 : copysignf((fabsf(v) - deadzone) / (1 - deadzone), v);
}
