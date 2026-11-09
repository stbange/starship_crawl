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
        g->views[player].matrix_map = !g->views[player].matrix_map;
        return;
    }
    Player *p = &g->players[player];
    PLAYER_VIEW(g, p).map_open = !PLAYER_VIEW(g, p).map_open;
    PLAYER_VIEW(g, p).map_center = bounded(world_cell(position_absolute(p->pos)));
}
void game_map_pan(Game *g, int player, int x, int y, int z) {
    if (player < 0 || player >= g->player_count || !g->views[player].map_open)
        return;
    Cell *c = &g->views[player].map_center;
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

/* A rolling sixteen-cell square: sixteen pure plans, at most 256 visited cells.
   No world allocation, streaming, discovery, or unbounded far-goal search. */
static bool allowed(const Game *g, Cell c) {
    return c.z >= 0 && (!g->world.open_layer || campaign_layer(c) <= g->world.open_layer);
}
bool game_navigation_to(const Game *g, int player, Cell goal, Navigation *nav) {
    memset(nav, 0, sizeof(*nav));
    if (player < 0 || player >= g->player_count)
        return false;
    const Player *p = &g->players[player];
    if (p->sheltered || g->matrix.divers[player].active)
        return false;
    return game_navigation_from(g, p->pos, p->lift_active, goal, nav);
}
bool game_navigation_from(const Game *g, Position position, bool in_lift, Cell goal,
                          Navigation *nav) {
    memset(nav, 0, sizeof(*nav));
    Vec3 feet = position_absolute(position);
    Cell from = bounded(world_cell(feet));
    nav->from = nav->next = from;
    nav->goal = bounded(goal);
    nav->point = feet;
    if (feet.z < 0) {
        nav->kind = NAV_BOARD;
        nav->point = v3(0, 2, 8);
        return true;
    }
    Vec3 origin = world_center(from), local = vsub(feet, origin);
    bool service = g->world.generator >= 9 && !in_lift &&
                   world_service_column(g->world.seed, from) && fabs(local.x) > 3.7 &&
                   fabs(local.x) < 12 && local.z < -6;
    /* Deck-height landings connect across the back of the shaft. A diagonal
       to the pad would cut through its inner rail (and the next flight). */
    if (service && local.y < .1 && local.z < -28) {
        nav->kind = NAV_LANDING;
        nav->point = vadd(origin, v3(0, .02, -31));
        return true;
    }
    /* A shaft landing is not the same as the deck floor. Follow the physical
       switchbacks before offering a pad/door at an unreachable height. */
    if (service && local.y >= .1) {
        bool up = goal.y > from.y;
        int flight =
            (int)clampd(up ? floor((local.y + .05) / 8) : ceil((local.y - .35) / 8) - 1, 0, 7);
        double x = (local.x > 0 ? 1 : -1) * (flight % 2 ? 10 : 6);
        double start = flight % 2 ? -9 : -31, end = flight % 2 ? -31 : -9;
        bool align = fabs(local.x - x) > .6;
        nav->point = vadd(origin, v3(x, (flight + (up ? 1 : 0)) * 8,
                                     align ? (up ? start : end) : (up ? end : start)));
        nav->kind = NAV_LANDING;
        nav->next.y += up ? 1 : 0;
        return true;
    }
    if (from.y != nav->goal.y) {
        nav->next.y += nav->goal.y > from.y ? 1 : -1;
        nav->point = world_lift(from);
        nav->kind = allowed(g, nav->next) ? NAV_LIFT : NAV_BLOCKED;
        return true;
    }
    if (cell_equal(from, nav->goal)) {
        nav->kind = NAV_GOAL;
        nav->point = game_terminal(goal);
        if (g->politics.version && g->politics.tracked) {
            Cell political;
            if (politics_objective(g, &political) && cell_equal(goal, political))
                nav->point = vadd(world_center(goal), v3(0, .02, 2));
        }
        return true;
    }
    enum { SIDE = 16, N = SIDE * SIDE };
    uint8_t doors[N] = {0};
    int parent[N], queue[N], head = 0, tail = 0;
    Cell region = world_region(from);
    int ox = (region.x - 1) * 4, oz = (region.z - 1) * 4;
    for (int z = 0; z < 4; z++)
        for (int x = 0; x < 4; x++) {
            RegionPlan plan;
            world_plan_version(g->world.seed, g->world.generator,
                               (Cell){region.x - 1 + x, from.y, region.z - 1 + z}, &plan);
            for (int j = 0; j < 4; j++)
                for (int i = 0; i < 4; i++)
                    doors[(z * 4 + j) * SIDE + x * 4 + i] = plan.doors[j * 4 + i];
        }
    for (int i = 0; i < N; i++)
        parent[i] = -1;
    int start = (from.z - oz) * SIDE + from.x - ox, best = start;
    double score = fabs((double)nav->goal.x - from.x) + fabs((double)nav->goal.z - from.z);
    parent[start] = start;
    queue[tail++] = start;
    const int dx[] = {-1, 1, 0, 0}, dz[] = {0, 0, -1, 1};
    while (head < tail) {
        int at = queue[head++], x = at % SIDE, z = at / SIDE;
        double distance =
            fabs((double)nav->goal.x - (ox + x)) + fabs((double)nav->goal.z - (oz + z));
        if (distance < score) {
            score = distance;
            best = at;
        }
        if (distance == 0)
            break;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], nz = z + dz[d];
            if (nx < 0 || nx >= SIDE || nz < 0 || nz >= SIDE)
                continue;
            int next = nz * SIDE + nx;
            if (parent[next] >= 0 || !(doors[at] & (1u << d)) || !(doors[next] & (1u << (d ^ 1))) ||
                !allowed(g, (Cell){ox + nx, from.y, oz + nz}))
                continue;
            parent[next] = at;
            queue[tail++] = next;
        }
    }
    nav->searched = (unsigned)head;
    if (best == start) {
        nav->kind = NAV_BLOCKED;
        return true;
    }
    while (parent[best] != start)
        best = parent[best];
    nav->next = (Cell){ox + best % SIDE, from.y, oz + best / SIDE};
    nav->kind = NAV_DOOR;
    int x = nav->next.x - from.x, z = nav->next.z - from.z;
    /* Cross the real portal; align with the protected cross corridor first. */
    nav->point = vadd(origin, v3(x * 50, .02, z * 50));
    if ((x && fabs(local.z) > 3) || (z && fabs(local.x) > 3))
        nav->point = vadd(origin, v3(0, .02, 0));
    return true;
}
void game_navigation_step(Game *g, double dt) {
    Cell goal = {0};
    bool objective = game_objective(g, &goal);
    for (int i = 0; i < g->player_count; i++) {
        Player *p = &g->players[i];
        p->navigation_time -= (float)dt;
        if (!objective || p->sheltered || g->matrix.divers[i].active) {
            memset(&p->navigation, 0, sizeof(p->navigation));
            p->navigation_time = 0;
        } else if (p->navigation_time <= 0 || !cell_equal(goal, p->navigation.goal) ||
                   !cell_equal(world_cell(position_absolute(p->pos)), p->navigation.from)) {
            game_navigation_to(g, i, goal, &p->navigation);
            p->navigation_time = .35f;
        }
    }
}
