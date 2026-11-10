#include "render_internal.h"
void sc_render_rhythm(Renderer *r, const Game *g, int player) {
    if (g->world.generator < 16 || g->players[player].sheltered || g->matrix.divers[player].active)
        return;
    for (int i = 0; i < g->world.count; i++) {
        const Room *room = &g->world.rooms[i];
        if (!room->ready ||
            vlen(vsub(world_room_center(room), player_eye(&g->players[player]))) > 160)
            continue;
        unsigned facts = world_rhythm_state(&g->world, room->cell);
        for (unsigned s = 0; s < SC_SEQUENCES; s++)
            if (g->rhythm.sequences[s].beat && cell_equal(g->rhythm.sequences[s].site, room->cell))
                facts |= 1u << (g->rhythm.sequences[s].kind - 1);
        for (unsigned kind = 1; kind <= 4; kind++) {
            if (!(facts & (1u << (kind - 1))))
                continue;
            Vec3 at = rhythm_trace(room->cell, kind);
            bool quiet = (facts & (16u << (kind - 1))) != 0;
            Color color = kind == SEQUENCE_HUNTER    ? rgb(.35f, .1f, .35f)
                          : kind == SEQUENCE_SIEGE   ? rgb(.6f, .27f, .08f)
                          : kind == SEQUENCE_MACHINE ? rgb(.25f, .45f, .55f)
                                                     : rgb(.15f, .5f, .35f);
            float glow = quiet ? .08f : .35f + .15f * (float)sin(g->time * 2);
            if (kind == SEQUENCE_HUNTER)
                for (int scratch = 0; scratch < 4; scratch++)
                    sc_render_tube(r, vadd(at, v3(scratch * .13, 0, -1)),
                                   vadd(at, v3(.3 + scratch * .15, 0, 1)), .025, .015, color, glow);
            else if (kind == SEQUENCE_SIEGE)
                for (int shell = 0; shell < 3; shell++)
                    sc_render_tube(r, vadd(at, v3(shell * .3, .1, 0)),
                                   vadd(at, v3(shell * .3, .1, .45)), .1, .07, color, glow);
            else {
                sc_render_block(r, vadd(at, v3(0, .25, 0)), v3(.7, .5, .5), rgb(.08f, .12f, .13f),
                                .05f);
                sc_render_octahedron(r, vadd(at, v3(0, .65, 0)), .12, color, glow);
                sc_render_world_label(r, vadd(at, v3(0, 1.2, 0)),
                                      kind == SEQUENCE_MACHINE ? "MAINTENANCE FEED"
                                                               : "FIELD SUPPLIES",
                                      color, player_eye(&g->players[player]), .009);
            }
        }
    }
}
