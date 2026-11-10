#include "render_internal.h"
void sc_render_preparation_scene(Renderer *r, const Game *g, int player) {
    if (g->world.generator < 16)
        return;
    Vec3 eye = player_eye(&g->players[player]);
    Color mint = rgb(.3f, .75f, .6f), gold = rgb(.8f, .6f, .3f);
    sc_render_world_label(r, v3(0, 1.4, 1), "OPTIONAL MEALS / 30 SALVAGE", mint, eye, .012);
    for (int cup = 0; cup < 3; cup++) {
        Color color = cup == 0 ? rgb(.3f, .7f, .9f) : cup == 1 ? gold : mint;
        sc_render_octahedron(r, v3(-.8 + cup * .8, 1.1, 1.7), .17, color, .4f);
    }
    sc_render_block(r, v3(7.7, 1.25, -4.5), v3(1.3, .12, .6), gold, .3f);
    sc_render_world_label(r, v3(7, 2, -4.4), "EXPEDITION PROJECTS", gold, eye, .014);
    for (unsigned i = 0; i < SC_KEEPSAKES; i++) {
        Vec3 at = v3(-8.5, 2.65, -6 + i * 2);
        sc_render_block(r, vadd(at, v3(0, -.3, 0)), v3(.7, .1, .8), rgb(.3f, .2f, .12f), .1f);
        if (i >= g->home.keepsake_count)
            continue;
        const Keepsake *k = &g->home.keepsakes[i];
        Color color = k->facts & DISTRICT_RESCUED    ? mint
                      : k->facts & DISTRICT_SALVAGED ? gold
                                                     : rgb(.4f, .6f, .9f);
        sc_render_octahedron(r, at, .22, color, .35f);
        sc_render_tube(r, vadd(at, v3(0, -.1, -.3)), vadd(at, v3(0, .2, .3)), .04, .025, color,
                       .5f);
    }
    sc_render_world_label(r, v3(-6, 3.5, 7.9), "KEEPSAKES / I THEN L FOR RECORDS", mint, eye, .014);
}
void sc_render_preparation_hud(Renderer *r, const Game *g, int player, float w, float h,
                               float scale) {
    if (g->world.generator < 16)
        return;
    const PersonalPreparation *p = &g->home.personal[player];
    char text[160];
    if (p->meal) {
        snprintf(text, sizeof(text), "%s / %.0FS%s", preparation_meal_name(p->meal), p->remaining,
                 g->players[player].sheltered ? " / TIMER PAUSED" : "");
        sc_render_centered(r, w * .5f, h - 84 * scale, .8f * scale, text, rgb(.5f, .8f, .7f));
    }
    unsigned scene = preparation_near(g, player);
    if (scene)
        sc_render_centered(r, w * .5f, h * .68f, scale,
                           scene == PREP_MEALS_SCENE
                               ? "G / OPTIONAL MEALS / BASIC KITCHEN RECOVERY IS FREE"
                               : "G / REFIT, FLARE PREPARATION AND CONDITION RESEARCH",
                           rgb(.85f, .7f, .4f));
}
