/* Compile this ordinary-control expedition against both libraries. Comparing
   the serialized result catches accidental changes to shared defaults/rules. */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    Game *g = malloc(sizeof(*g));
    if (!g)
        return 1;
    game_init(g, 713);
    for (int i = 0; i < 1200; i++) {
        Vec3 position = position_absolute(g->players[0].pos);
        if (!world_refresh(&g->world, &position, 1))
            return 1;
        world_stream(&g->world, SC_STREAM_BUDGET);
        Actions a[2] = {{.forward = i < 720 ? 1 : 0,
                         .strafe = i >= 720 && i < 900 ? .3f : 0,
                         .fire = i % 20 < 2,
                         .sprint = i < 500,
                         .crouch = i > 1000},
                        {0}};
        game_step(g, a, 1.0 / 60);
    }
    bool ok = game_save(g, argv[1]);
    printf("Parity expedition: %.3f seconds, %u shots, seed %u\n", g->time, g->shots,
           g->world.seed);
    game_destroy(g);
    free(g);
    return ok ? 0 : 1;
}
