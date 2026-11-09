#include "render_internal.h"

void sc_render_onboarding_journal(Renderer *r, const Game *g, int player, float w, float h) {
    float scale = fminf((w - 24) / 760, (h - 24) / 470);
    float width = 760 * scale, left = (w - width) * .5f, top = (h - 470 * scale) * .5f;
    Color ink = rgb(.76f, .84f, .8f), gold = rgb(.9f, .72f, .4f);
    sc_render_rect(r, left, top, width, 470 * scale, rgb(.015f, .03f, .04f));
    sc_render_frame(r, left, top, width, 470 * scale, gold);
    sc_render_text_at(r, left + 18 * scale, top + 16 * scale, 1.5f * scale,
                      "FIELD GUIDE / YOUR EXPEDITION", gold);
    float y = top + 48 * scale;
    char line[180];
    snprintf(line, sizeof(line), "GUIDANCE: %s / CHANGE IN SETTINGS / BACKSPACE DISMISSES A HINT",
             onboarding_mode(g->views[player].guide_mode));
    y = sc_render_wrapped(r, left + 18 * scale, y, scale, width - 36 * scale, line, gold) +
        12 * scale;
    y = sc_render_wrapped(
            r, left + 18 * scale, y, scale, width - 36 * scale,
            "EXPLORE, FIGHT, RECOVER. THREE KILLS EARN A SHOTGUN. AN ELITE OR GUARDIAN HOLDS "
            "THE SHELTER.",
            ink) +
        12 * scale;
    y = sc_render_wrapped(
            r, left + 18 * scale, y, scale, width - 36 * scale,
            "AFTER THE FIRST GUARDIAN, CLEAR A LAYER-2 TERMINAL AND RECOVER ITS MATRIX "
            "INTERFACE. HACKING IS OPTIONAL.",
            ink) +
        12 * scale;
    y = sc_render_wrapped(
            r, left + 18 * scale, y, scale, width - 36 * scale,
            "PHASE TWO ADDS POWERS. PHASE THREE ADDS RIGS AND SPECIALIZATIONS. EVERY REQUIRED "
            "FIGHT HAS A GUN-ONLY SOLUTION.",
            ink) +
        12 * scale;
    const unsigned lessons[] = {LESSON_BOARD,   LESSON_COMMISSION, LESSON_EXPLORE,   LESSON_WEAPON,
                                LESSON_UPGRADE, LESSON_SHELTER,    LESSON_INTERFACE, LESSON_MATRIX,
                                LESSON_POWER,   LESSON_RIG};
    for (unsigned i = 0; i < sizeof(lessons) / sizeof(*lessons); i++) {
        onboarding_text(lessons[i], g->views[player].guide_device, line, sizeof(line));
        y = sc_render_wrapped(r, left + 18 * scale, y, .85f * scale, width - 36 * scale, line,
                              ink) +
            9 * scale;
    }
    sc_render_text_at(r, left + 18 * scale, top + 444 * scale, scale,
                      "ENTER / START CLOSE / MAP AND OBJECTIVE ALWAYS AVAILABLE", gold);
}
void sc_render_onboarding_hud(Renderer *r, const Game *g, int player, float w, float h, float s) {
    const Player *p = &g->players[player];
    if (!PLAYER_VIEW(g, p).lesson || PLAYER_VIEW(g, p).guide_mode == GUIDE_OFF ||
        PLAYER_VIEW(g, p).message_time > 0)
        return;
    char line[180];
    onboarding_hint_text(g, player, line, sizeof(line));
    float width = fminf(w - 24, 650 * s), left = (w - width) * .5f;
    float scale = fminf(s, width / 510);
    sc_render_rect(r, left - 5, h * .72f - 6, width + 10, 56 * scale, rgb(.015f, .03f, .04f));
    sc_render_wrapped(r, left, h * .72f, scale, width, line, rgb(.76f, .84f, .8f));
    sc_render_text_at(r, left, h * .72f + 38 * scale, .75f * scale,
                      PLAYER_VIEW(g, p).guide_device == GUIDE_KEYBOARD
                          ? "BACKSPACE DISMISS / SLASH FIELD GUIDE / F1 GUIDANCE"
                          : "SETTINGS / FIELD GUIDE / DISMISS HINT",
                      rgb(.4f, .55f, .56f));
}
