#include "campaign.h"
#include <stdlib.h>

unsigned campaign_layer(Cell c) {
    /* Whole regions, with symmetric negative/positive hull-side catchments. */
    double x = c.x < 0 ? (-(double)c.x - 1) / 4 : c.x / 4.0;
    double y = fabs((double)c.y) / 2, z = fmax(0, c.z) / 4.0;
    return 1 + (unsigned)fmax(x, fmax(y, z));
}
double campaign_gate_ray(const World *w, Vec3 origin, Vec3 direction, double limit) {
    if (!w->open_layer)
        return limit;
    double n = w->open_layer;
    double low[] = {(-4 * n - .5) * SC_CELL, (-2 * n + 1) * SC_DECK, 0};
    double high[] = {(4 * n - .5) * SC_CELL, 2 * n * SC_DECK, 4 * n * SC_CELL};
    double at[] = {origin.x, origin.y, origin.z}, dir[] = {direction.x, direction.y, direction.z};
    for (int axis = 0; axis < 3; axis++) {
        if (fabs(dir[axis]) < 1e-9)
            continue;
        /* Both faces block: streamed enemies beyond a sealed layer must not
           fire inward through its boundary. The loading-bay approach stays open. */
        for (int side = 0; side < 2; side++) {
            if (axis == 2 && side == 0)
                continue;
            double hit = ((side ? high[axis] : low[axis]) - at[axis]) / dir[axis];
            if (hit >= 0)
                limit = fmin(limit, hit);
        }
    }
    return limit;
}
ShipTraits campaign_traits(uint32_t seed, Cell c) {
    uint32_t h = world_hash(seed, (Cell){0}, 0x41584953u);
    double a = (h & 1) ? c.y * 2.0 : c.x, b = (h & 1) ? c.x : c.y * 2.0;
    int heat = (int)clampd(a / 3, -3, 3), order = (int)clampd(b / 3, -3, 3);
    return (ShipTraits){heat * ((h & 2) ? -1 : 1), order * ((h & 4) ? -1 : 1),
                        (unsigned)clampd(c.z / 4, 0, 3)};
}
Cell campaign_site(unsigned id) {
    if (id == 8)
        return (Cell){0, 1, 0};
    unsigned phase = (id % 8) / 2;
    return (Cell){id & 1 ? -(int)(phase * 4) - 3 : (int)(phase * 4) + 1, 0, 1};
}
int campaign_guardian(Cell owner) {
    for (unsigned i = 0; i < SC_GUARDIANS; i++)
        if (cell_equal(owner, campaign_site(i)))
            return (int)i;
    return -1;
}
bool campaign_arena_region(Cell region) {
    return region.y == 0 && region.z == 0 && region.x >= -4 && region.x <= 3;
}
unsigned campaign_family(uint32_t seed, unsigned id) {
    /* Neighboring approaches always have different families. */
    return (world_hash(seed, (Cell){(int32_t)(id / 2), 0, 0}, 0x424f5353u) % 5 + id % 2) % 5;
}
const char *campaign_guardian_name(uint32_t seed, unsigned id) {
    static const char *names[] = {"FOUNDRY MARSHAL", "CONVOY DEVOURER", "CHOIR OF GLASS",
                                  "CRYOGENIC CUSTODIAN", "FIREWALL SAINT"};
    return id == 8 ? "THRESHOLD HUNTER" : names[campaign_family(seed, id)];
}
const char *campaign_phase_name(unsigned phase) {
    static const char *names[] = {"FOOTHOLD", "COMMITMENT", "TRANSFORMATION", "RECKONING",
                                  "UNBOUND EXPLORATION"};
    return names[phase < SC_PHASES ? phase : SC_PHASES];
}
const char *campaign_module_name(unsigned module) {
    static const char *names[] = {"NO SPECIALIZATION", "MOBILE STORM", "SIEGE ENGINEER",
                                  "SILENT HUNTER", "SYSTEMS SABOTEUR"};
    return names[module <= SC_SPECIALIZATIONS ? module : 0];
}
void campaign_plan(uint32_t seed, RegionPlan *plan) {
    for (int i = 0; i < plan->count; i++) {
        RoomSpec *r = &plan->rooms[i];
        if (r->type == BAY)
            continue;
        ShipTraits t = campaign_traits(seed, r->cell);
        if (!r->landmark)
            r->layout = t.order > 0 ? SUITES : t.order < 0 ? WORKBAYS : r->layout;
        if (r->span_x == 1 && r->type != CONDUIT && r->type != CREW)
            r->height = fmin(56, r->height + fmin(4, campaign_layer(r->cell) - 1) * 4);
        if (campaign_guardian(r->cell) >= 0) {
            r->layout = ARCADES;
            r->landmark = NO_LANDMARK;
            if (r->height < 8)
                r->height = 12;
        }
    }
}
float campaign_guardian_health(unsigned id, unsigned participants) {
    unsigned phase = id == 8 ? 0 : id / 2;
    return (id == 8 ? 340.f : 2200.f + phase * 1800.f) * (participants == 3 ? 1.45f : 1);
}
void campaign_populate(World *w, Room *r) {
    for (int i = 0; i < r->pickup_count; i++)
        if (r->pickups[i].kind == PICK_WEAPON)
            r->pickups[i].kind = PICK_SALVAGE;
    int guardian = campaign_guardian(r->cell);
    if (guardian >= 0) {
        static const uint8_t bodies[] = {28, 21, 12, 29, 30};
        r->enemy_count = 1;
        Enemy *e = &r->enemies[0];
        e->kind = bodies[campaign_family(w->seed, (unsigned)guardian)];
        e->health = campaign_guardian_health((unsigned)guardian, 1);
        e->pos = position_from(vadd(world_center(r->cell), v3(0, .02, 22)));
        return;
    }
    ShipTraits t = campaign_traits(w->seed, r->cell);
    unsigned layer = campaign_layer(r->cell);
    for (int i = 0; i < r->enemy_count; i++) {
        Enemy *e = &r->enemies[i];
        if (e->kind == 26)
            continue;
        if (w->generator < 10 && r->height >= 3 && i % 3 == 0) {
            static const uint8_t machines[] = {27, 28, 29, 30};
            static const uint8_t swarm[] = {3, 11, 8, 23};
            if (t.order > 0)
                e->kind = machines[(unsigned)i % (layer > 4 ? 4 : layer)];
            else if (t.order < 0)
                e->kind = swarm[(unsigned)i % (layer > 4 ? 4 : layer)];
            else if (t.heat < 0)
                e->kind = 14;
            else if (t.heat > 0)
                e->kind = 13;
        }
        /* Horde enemies stay quick to kill even far beyond the final phase. */
        e->health = sc_enemies[e->kind].health + 10.f * (float)fmin(layer, 8);
    }
}
