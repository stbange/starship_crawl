#include "render_internal.h"

void sc_render_campaign_scene(Renderer *r, const Game *g, int player) {
    if (!g->campaign.version)
        return;
    Vec3 viewer = position_absolute(g->players[player].pos);
    Color gold = rgb(.9f, .55f, .12f), mint = rgb(.2f, .85f, .7f);
    for (unsigned id = 0; id < SC_GUARDIANS; id++) {
        Cell cell = campaign_site(id);
        Vec3 at = game_terminal(cell);
        if (vlen(vsub(at, viewer)) > 240 || fabs(at.y - viewer.y) > 60)
            continue;
        at = vsub(at, r->origin);
        bool done = (g->campaign.defeated & (1u << id)) != 0;
        sc_render_block(r, vadd(at, v3(0, .5, 0)), v3(1.5, 1, 1), rgb(.14f, .12f, .08f), 0);
        sc_render_octahedron(r, vadd(at, v3(0, 1.8 + sin(g->time * 2) * .12, 0)), .5,
                             done ? mint : gold, 1.4f);
        Room *room = world_find((World *)&g->world, cell);
        if (!room || !room->ready || done)
            continue;
        const Enemy *e = &room->enemies[0];
        const GuardianAttempt *a = &g->campaign.attempts[id];
        Vec3 core = vsub(vadd(position_absolute(e->pos), v3(0, 2.4, 0)), r->origin);
        Color signal = a->opening > 0 ? mint : gold;
        sc_render_octahedron(r, core, a->opening > 0 ? .55 : .3, signal, 1.4f);
        if (a->active && e->windup > 0) {
            sc_render_tube(r, core, vsub(a->aim, r->origin), .025, .025, gold, 1.2f);
            Vec3 mark = vsub(a->aim, r->origin);
            mark.y = cell.y * SC_DECK + .1 - r->origin.y;
            for (int i = 0; i < 16; i++) {
                double t = i * SC_PI / 8;
                sc_render_block(r, vadd(mark, v3(sin(t) * 2, 0, cos(t) * 2)), v3(.2, .08, .2), gold,
                                2);
            }
        }
    }
    if (!g->world.open_layer)
        return;
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *chunk = &g->world.chunks[i];
        if (!chunk->active || campaign_layer(chunk->cell) > g->world.open_layer)
            continue;
        Vec3 at = vsub(world_center(chunk->cell), r->origin);
        if (vlen(vsub(world_center(chunk->cell), viewer)) > 200)
            continue;
        for (int step = -1; step <= 1; step += 2) {
            Cell next = chunk->cell;
            next.y += step;
            if (campaign_layer(next) <= g->world.open_layer)
                continue;
            Vec3 lift = vsub(world_lift(chunk->cell), r->origin);
            sc_render_octahedron(r, vadd(lift, v3(step * .9, 1.6, 0)), .28, gold, 1.4f);
            sc_render_block(r, vadd(lift, v3(step * .9, 1.6, -.3)), v3(.6, .08, .08), gold, 1.5f);
        }
        for (int d = 0; d < 4; d++) {
            Cell next = world_neighbor(chunk->cell, (Direction)d);
            if (next.z < 0 || campaign_layer(next) <= g->world.open_layer ||
                !world_door_version(g->world.seed, g->world.generator, chunk->cell, (Direction)d))
                continue;
            Vec3 offset = v3(d == WEST   ? -48
                             : d == EAST ? 48
                                         : 0,
                             SC_DECK * .5,
                             d == SOUTH   ? -48
                             : d == NORTH ? 48
                                          : 0);
            sc_render_block(r, vadd(at, offset), d < 2 ? v3(.35, SC_DECK, 7) : v3(7, SC_DECK, .35),
                            rgb(.13f, .07f, .16f), .25f);
            for (int bar = -2; bar <= 2; bar++) {
                Vec3 shift = d < 2 ? v3(0, 0, bar * 1.3) : v3(bar * 1.3, 0, 0);
                sc_render_block(r, vadd(vadd(at, offset), shift),
                                d < 2 ? v3(.4, SC_DECK, .08) : v3(.08, SC_DECK, .4), gold, 1.1f);
            }
        }
    }
}
