#include "politics.h"
#include "campaign.h"
#include "frontier.h"
#include <float.h>

Government politics_government(uint32_t seed, unsigned country) {
    return (Government)((world_hash(seed, (Cell){0}, 0x43524f57u) % 3 + country) % 3);
}
const char *politics_government_name(Government government) {
    static const char *names[] = {"DIVIDED CROWN", "MOLTING COURT", "PARLIAMENT OF ECHOES"};
    return names[(unsigned)government % 3];
}
const char *politics_name(uint32_t seed, unsigned country) {
    static const char *names[3][3] = {{"VESPER CROWN", "GLASS REGENCY", "THREEFOLD THRONE"},
                                      {"AMBER DYNASTY", "CHITIN COURT", "PALE SUCCESSION"},
                                      {"ECHO ASSEMBLY", "DEAD CONCORD", "ANCESTRAL SYNOD"}};
    return names[politics_government(seed, country)]
                [world_hash(seed, (Cell){(int)country, 0, 0}, 7101) % 3];
}
const char *politics_relation_name(Relation relation) {
    static const char *names[] = {"PEACE", "WAR", "TRIBUTE", "SHARED NERVOUS SYSTEM"};
    return names[(unsigned)relation % 4];
}
Color politics_color(uint32_t seed, unsigned country) {
    static const Color colors[] = {{.72f, .40f, .16f}, {.23f, .70f, .51f}, {.50f, .39f, .83f}};
    return colors[politics_government(seed, country)];
}
Cell politics_region(uint32_t seed, unsigned country, unsigned role) {
    Cell c = country == 0   ? (Cell){(int)role - 1, 1, 0}
             : country == 1 ? (Cell){-(int)role, -1, 0}
                            : (Cell){0, 0, (int)role};
    uint32_t h = world_hash(seed, (Cell){0}, 7102);
    if (h & 1)
        c.x = -c.x - 1;
    if (h & 2)
        c.y = -c.y;
    return c;
}
unsigned politics_country(uint32_t seed, Cell cell) {
    if (cell.z < 0 || (!cell.x && !cell.y && !cell.z))
        return 0;
    Cell region = world_region(cell);
    for (unsigned nation = 0; nation < SC_COUNTRIES; nation++)
        for (unsigned role = COURT_AUDIENCE; role <= COURT_FINALE; role++)
            if (cell_equal(region, politics_region(seed, nation, role)))
                return role == COURT_CONDUIT ? (nation + 1) % 3 + 1 : nation + 1;
    if (world_hash(seed, (Cell){region.x / 2, region.y / 3, region.z / 2}, 7103) % 5 == 0)
        return 0; /* Wilderness between inhabited infrastructure catchments. */
    double best = DBL_MAX;
    unsigned owner = 0;
    for (unsigned i = 0; i < SC_COUNTRIES; i++) {
        Cell c = politics_region(seed, i, COURT_AUDIENCE);
        double x = (double)region.x - c.x, y = ((double)region.y - c.y) * .7,
               z = (double)region.z - c.z;
        double score = x * x + y * y + z * z;
        if (score < best) {
            best = score;
            owner = i + 1;
        }
    }
    return owner;
}
void politics_plan(uint32_t seed, RegionPlan *plan) {
    for (int i = 0; i < plan->count; i++)
        plan->rooms[i].habitat.country = (uint8_t)politics_country(seed, plan->rooms[i].cell);
    Cell region = world_region(plan->origin);
    for (unsigned nation = 0; nation < SC_COUNTRIES; nation++)
        for (unsigned role = COURT_AUDIENCE; role <= COURT_FINALE; role++) {
            if (!cell_equal(region, politics_region(seed, nation, role)))
                continue;
            int best = -1;
            for (int i = 0; i < plan->count; i++) {
                RoomSpec *r = &plan->rooms[i];
                if (r->type != BAY && r->height >= 8 && !r->habitat.volume &&
                    campaign_guardian(r->cell) < 0 &&
                    (best < 0 || r->hash < plan->rooms[best].hash))
                    best = i;
            }
            /* These regions always contain a full-height room (reserved macro backbone). */
            if (best < 0)
                best = 0;
            RoomSpec *r = &plan->rooms[best];
            r->habitat.country = (uint8_t)(nation + 1);
            r->habitat.court = (uint8_t)role;
            r->habitat.pace = role == COURT_AUDIENCE ? PACE_QUIET : PACE_FRONTIER;
            r->habitat.contract = 0;
            static const uint8_t patterns[] = {CH_AERIE, CH_COLONY, CH_CIPHER};
            r->habitat.challenge =
                role == COURT_AUDIENCE ? 0 : patterns[politics_government(seed, nation)];
        }
}
Cell politics_site(uint32_t seed, unsigned country, CourtRole role) {
    RegionPlan plan;
    world_plan_version(seed, 13, politics_region(seed, country % 3, role), &plan);
    for (int i = 0; i < plan.count; i++)
        if (plan.rooms[i].habitat.court == role)
            return plan.rooms[i].cell;
    return plan.rooms[0].cell;
}
void politics_populate(World *w, Room *r) {
    if (r->habitat.court == COURT_FINALE && r->enemy_count)
        r->enemies[0].health += 300; /* Advertised fixed royal guard; no adaptive stats. */
    if (!r->habitat.country || r->habitat.pace == PACE_FRONTIER ||
        campaign_guardian(r->cell) >= 0 || r->hash % 3)
        return;
    for (int i = 0; i < r->enemy_count && i < 2; i++) {
        Enemy *e = &r->enemies[i];
        e->country = r->habitat.country;
        e->home = position_absolute(e->pos);
        e->kind = politics_government(w->seed, e->country - 1) == ECHO_PARLIAMENT ? 28 : 24;
        e->health = sc_enemies[e->kind].health + 10.f * (float)fmin(campaign_layer(r->cell), 8);
        e->behavior = ALIEN_NONE;
    }
}
