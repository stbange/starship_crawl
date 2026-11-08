/* Read-only save inspection; never advances or edits the expedition. */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    for (int file = 1; file < argc; file++) {
        Game *g = calloc(1, sizeof(*g));
        if (!g)
            return 2;
        game_init(g, 713);
        if (!game_load(g, argv[file])) {
            fprintf(stderr, "Cannot load %s\n", argv[file]);
            game_destroy(g);
            free(g);
            return 1;
        }
        Vec3 p = position_absolute(g->players[0].pos);
        Cell cell = world_cell(p);
        const FactionRegion *r = factions_at(&g->factions, cell);
        printf("{\"path\":\"%s\",\"at\":[%.3f,%.3f,%.3f],\"cell\":[%d,%d,%d],"
               "\"game_bytes\":%zu,\"faction_bytes\":%zu,\"region_bytes\":%zu,"
               "\"regions\":%zu,\"room_records\":%zu,\"record_bytes\":%zu,"
               "\"tick\":%u,\"valid\":%s,\"supplies\":[",
               argv[file], p.x, p.y, p.z, cell.x, cell.y, cell.z, sizeof(Game),
               sizeof(Factions), sizeof(FactionRegion), g->factions.count,
               g->world.record_count, sizeof(RoomRecord), g->factions.tick,
               factions_valid(&g->factions, 14) ? "true" : "false");
        for (unsigned c = 0; c < 3; c++)
            printf("%s[%u,%u,%u]", c ? "," : "", g->factions.nations[c].supplies[0],
                   g->factions.nations[c].supplies[1], g->factions.nations[c].supplies[2]);
        printf("],\"site\":{\"owner\":%u,\"flags\":%u,\"casualties\":%u,\"systems\":%u},"
               "\"pressure\":[%.4f,%.4f],\"pacing\":[%u,%u],\"forces\":[",
               r ? r->owner : 0, r ? r->flags : 0, r ? r->casualties : 0, r ? r->systems : 0,
               g->factions.director.pressure[0], g->factions.director.pressure[1],
               g->factions.director.pace[0], g->factions.director.pace[1]);
        unsigned printed = 0;
        for (unsigned i = 0; i < SC_FACTION_FORCES; i++) {
            const FactionForce *a = &g->factions.forces[i];
            if (a->id)
                printf("%s{\"id\":%u,\"country\":%u,\"order\":%u,\"alive\":%u,\"cargo\":%u,"
                       "\"present\":%u,\"participation\":[%u,%u],\"claimed\":%u}",
                       printed++ ? "," : "", a->id, a->country, a->order, factions_strength(a),
                       a->cargo, a->present, a->participation[0], a->participation[1], a->claimed);
        }
        puts("]}");
        game_destroy(g);
        free(g);
    }
}
