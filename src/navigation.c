#include "game.h"
#include <string.h>
static Cell bounded(Cell c) {
    c.x = (int32_t)clampd(c.x, -9000000, 9000000);
    c.y = (int32_t)clampd(c.y, -4000000, 4000000);
    c.z = (int32_t)clampd(c.z, 0, 9000000);
    return c;
}
void game_map_toggle(Game *g, int player) {
    if (player < 0 || player >= g->player_count)
        return;
    if (g->matrix.divers[player].active) {
        g->matrix.divers[player].map = !g->matrix.divers[player].map;
        return;
    }
    Player *p = &g->players[player];
    p->map_open = !p->map_open;
    p->map_center = bounded(world_cell(position_absolute(p->pos)));
}
void game_map_pan(Game *g, int player, int x, int y, int z) {
    if (player < 0 || player >= g->player_count || !g->players[player].map_open)
        return;
    Cell *c = &g->players[player].map_center;
    c->x += (x > 0) - (x < 0);
    c->y += (y > 0) - (y < 0);
    c->z += (z > 0) - (z < 0);
    *c = bounded(*c);
}
bool game_map_build(const Game *g, int player, Cell center, int radius, MapView *view) {
    memset(view, 0, sizeof(*view));
    if (player < 0 || player >= g->player_count)
        return false;
    center = bounded(center);
    radius = (int)clampd(radius, 1, 4);
    view->side = radius * 2 + 1;
    view->origin = (Cell){center.x - radius, center.y, center.z - radius};
    view->player = world_cell(position_absolute(g->players[player].pos));
    view->has_objective = game_objective(g, &view->objective);
    view->has_shelter = g->shelter.deployed;
    view->shelter = world_cell(position_absolute(g->shelter.door));
    RegionPlan plans[9];
    for (int z = 0; z < view->side; z++)
        for (int x = 0; x < view->side; x++) {
            Cell c = {view->origin.x + x, center.y, view->origin.z + z};
            if (c.z < 0)
                continue;
            Cell region = world_region(c);
            int found = -1;
            for (int n = 0; n < view->plans; n++)
                if (cell_equal(world_region(plans[n].origin), region)) {
                    found = n;
                    break;
                }
            if (found < 0) {
                found = view->plans++;
                world_plan_version(g->world.seed, g->world.generator, region, &plans[found]);
            }
            int at = z * view->side + x;
            Cell origin = plans[found].origin;
            int local = (c.z - origin.z) * 4 + c.x - origin.x;
            view->valid[at] = true;
            view->rooms[at] = plans[found].rooms[plans[found].owner[local]];
            view->doors[at] = plans[found].doors[local];
            view->systems[at] = world_systems(&g->world, c);
        }
    return true;
}
