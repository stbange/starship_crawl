#include "game.h"
#include <stdio.h>
#include <stdlib.h>

/* Read-only validation and description of an unmodified gameplay save. */
int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    Game *g = calloc(1, sizeof(*g));
    if (!g)
        return 2;
    game_init(g, 1);
    if (!game_load(g, argv[1])) {
        fprintf(stderr, "Save refused: %s\n", argv[1]);
        game_destroy(g);
        free(g);
        return 3;
    }
    while (g->world.pending && !g->world.failed)
        world_stream(&g->world, SC_STREAM_BUDGET);
    Player *p = &g->players[0];
    Vec3 feet = position_absolute(p->pos);
    Room *r = world_find(&g->world, world_cell(feet));
    ShipTraits t = campaign_traits(g->world.seed, world_cell(feet));
    printf("{\"seed\":%u,\"generator\":%u,\"rank\":%u,\"kills\":%u,\"phase\":%u,"
           "\"position\":[%.5f,%.5f,%.5f],\"yaw\":%.5f,\"pitch\":%.5f,\"enemy_spawn_supported\":%s,"
           "\"sheltered\":%s,\"ready\":%s,\"failed\":%s,\"story_done\":%u,"
           "\"heat\":%d,\"order\":%d,\"alien\":%u,\"theme\":%u,\"height\":%.1f,"
           "\"form\":%u,\"volume\":%u,\"weather\":%u,\"country\":%u,\"court\":%u,"
           "\"max_health\":%.2f,\"health\":%.2f,\"max_shield\":%.2f,\"shield\":%.2f,"
           "\"max_armor\":%.2f,\"armor\":%.2f,\"energy\":%.2f,\"max_energy\":%.2f,"
           "\"credits\":%u,\"points\":%u,\"pickups\":%u,\"owned_weapons\":%u,\"guns\":[%u,%u],"
           "\"owned_powers\":%u,\"owned_rigs\":%u,\"attributes\":[",
           g->world.seed, g->world.generator, p->level, p->kills, campaign_phase(&g->campaign),
           feet.x, feet.y, feet.z, p->yaw, p->pitch,
           world_spawn_supported(&g->world, feet, p->crouched ? 1 : 1.78) ? "true" : "false",
           p->sheltered ? "true" : "false", r && r->ready ? "true" : "false",
           g->world.failed ? "true" : "false", g->story.done, t.heat, t.order, t.alien,
           r ? r->theme : 0, r ? r->height : 0, r ? r->habitat.form : 0, r ? r->habitat.volume : 0,
           r ? r->habitat.atmosphere : 0, politics_owner(g, world_cell(feet)),
           r ? r->habitat.court : 0, player_max_health(p), p->health, player_max_shield(p),
           p->shield, player_max_armor(p), p->armor, p->energy, player_max_energy(p), p->credits,
           p->points, p->pickups, p->weapons, g->campaign.profiles[0].guns[0],
           g->campaign.profiles[0].guns[1], g->campaign.profiles[0].powers,
           g->campaign.profiles[0].rigs);
    for (int i = 0; i < SC_ATTRIBUTES; i++)
        printf("%s%u", i ? "," : "", *player_attribute(p, i));
    Vec3 focus = r ? frontier_focus(r) : feet;
    printf("],\"reveal\":%u,\"focus\":[%.4f,%.4f,%.4f],\"player_clear\":%s,\"countries\":[",
           r ? r->habitat.reveal : 0, focus.x, focus.y, focus.z,
           !world_overlap(&g->world, feet, .34, p->crouched ? 1 : 1.78) ? "true" : "false");
    for (int i = 0; i < SC_COUNTRIES; i++) {
        CountryState *c = &g->politics.countries[i];
        printf("%s{\"government\":%u,\"stage\":%u,\"pledge\":%u,\"offended\":%u}", i ? "," : "",
               politics_government(g->world.seed, i), c->stage, c->pledge, c->offended);
    }
    printf("]}\n");
    bool ok = !g->world.failed;
    game_destroy(g);
    free(g);
    return ok ? 0 : 1;
}
