#include "render_internal.h"

static void assemblies(Renderer *r, const Game *g, const Room *room, unsigned id, bool done) {
    if (g->campaign.version < 2 || id == 8)
        return;
    const GuardianAttempt *a = &g->campaign.attempts[id];
    Vec3 center = world_center(room->cell);
    Color cold = rgb(.2f, .8f, 1), hot = rgb(1, .35f, .12f), quiet = rgb(.08f, .2f, .16f);
    Vec3 canopy = vadd(center, v3(0, done ? 9 : a->stage == GUARDIAN_OVERRUN ? 8 : 6, 22));
    sc_render_block(r, canopy, v3(9, 2, 7), rgb(.13f, .16f, .18f), .1f);
    for (int rib = -3; rib <= 3; rib++)
        sc_render_block(r, vadd(canopy, v3(rib, -.9, -3.5)), v3(.1, .15, .1),
                        a->opening > 0 || done ? cold : hot, .8f);
    for (int side = -1; side <= 1; side += 2)
        sc_render_tube(r, vadd(center, v3(side * 5, 11, 22)), vadd(canopy, v3(side * 4, .8, 0)),
                       .16, .16, quiet, .1f);
    for (unsigned i = 0; i < 2; i++) {
        bool broken = done || (a->broken & (1u << i));
        Vec3 at = enemy_center(&room->enemies[i + 1]);
        Color signal = broken ? quiet : i ? hot : cold;
        sc_render_block(r, vadd(at, v3(0, -.6, 0)), v3(2.2, .4, 2.2), quiet, .1f);
        if (guardian_pattern(id)) {
            sc_render_octahedron(r, at, broken ? .4 : 1.1, signal, broken ? .1f : 1.5f);
            if (!broken)
                sc_render_tube(r, at, enemy_center(&room->enemies[0]), .12, .04, signal, .7f);
        } else {
            sc_render_block(r, at, v3(1.4, broken ? .35 : 1.6, 1.4), signal, broken ? .1f : 1.2f);
            sc_render_tube(r, canopy, vadd(at, v3(0, 1, 0)), .11, .11, signal, .3f);
        }
        Vec3 console = guardian_console(id, i);
        uint32_t flag = i ? SYSTEM_SILENCED : SYSTEM_POWER;
        bool prepared = (world_systems(&g->world, room->cell) & flag) != 0;
        sc_render_block(r, vadd(console, v3(0, .65, 0)), v3(1, 1.3, .6), quiet, .1f);
        sc_render_octahedron(r, vadd(console, v3(0, 1.6, 0)), .22,
                             prepared ? rgb(.3f, 1, .5f) : signal, 1);
        if (a->active && room->enemies[0].windup > 0 && !broken) {
            if (!i && guardian_pattern(id) == 2)
                for (unsigned spoke = 0; spoke < 4; spoke++) {
                    double angle = spoke * SC_PI * .5 + (a->attack % 2) * SC_PI * .25;
                    Vec3 start = v3(at.x, center.y + .12, at.z);
                    sc_render_tube(r, start, vadd(start, v3(sin(angle) * 18, 0, cos(angle) * 18)),
                                   .08, .06, cold, 1.8f);
                }
            Vec3 ground = a->aim;
            ground.y = center.y + .1;
            sc_render_tube(r, at, ground, .025, .025, signal, 1.5f);
            if (!i)
                for (unsigned k = 0; k < 16; k++) {
                    double angle = k * SC_PI / 8;
                    sc_render_block(r, vadd(at, v3(sin(angle) * 3, -.9, cos(angle) * 3)),
                                    v3(.25, .08, .25), signal, 1.8f);
                }
        }
    }
    /* The working floor never moves. Only the suspended gantry retracts, so
       walkers, both co-op participants and either rig keep their support. */
    Vec3 breach = vadd(center, v3(0, 0, 39));
    for (int side = -1; side <= 1; side += 2)
        sc_render_block(r, vadd(breach, v3(side * 4, 3, 0)), v3(.5, 6, .5), done ? cold : quiet,
                        done ? 1 : .1f);
    sc_render_block(r, vadd(breach, v3(0, done ? 7 : 5, 0)), v3(8, .5, .5), done ? cold : hot, 1);
}
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
        bool done = (g->campaign.defeated & (1u << id)) != 0;
        sc_render_block(r, vadd(at, v3(0, .5, 0)), v3(1.5, 1, 1), rgb(.14f, .12f, .08f), 0);
        if (!g->campaign.attempts[id].active)
            sc_render_octahedron(r, vadd(at, v3(0, 1.8 + sin(g->time * 2) * .12, 0)), .5,
                                 done ? mint : gold, 1.4f);
        Room *room = world_find((World *)&g->world, cell);
        if (!room || !room->ready)
            continue;
        assemblies(r, g, room, id, done);
        if (done)
            continue;
        const Enemy *e = &room->enemies[0];
        const GuardianAttempt *a = &g->campaign.attempts[id];
        Vec3 core = vadd(position_absolute(e->pos), v3(0, 2.4, 0));
        Color signal = a->opening > 0 ? mint : gold;
        sc_render_octahedron(r, core, a->opening > 0 ? .55 : .3, signal, 1.4f);
        if (a->active && e->windup > 0) {
            Vec3 mark = a->aim;
            mark.y = cell.y * SC_DECK + .1;
            sc_render_tube(r, core, mark, .025, .025, gold, 1.2f);
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
        Vec3 at = world_center(chunk->cell);
        if (vlen(vsub(world_center(chunk->cell), viewer)) > 200)
            continue;
        for (int step = -1; step <= 1; step += 2) {
            Cell next = chunk->cell;
            next.y += step;
            if (campaign_layer(next) <= g->world.open_layer)
                continue;
            Vec3 lift = world_lift(chunk->cell);
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
