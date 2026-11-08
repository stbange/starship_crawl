#ifndef SC_FACTIONS_H
#define SC_FACTIONS_H
#include "world.h"

#define SC_FACTION_FORCES 32
#define SC_FORCE_MEMBERS 3
#define SC_FACTION_EVENTS 128
#define SC_FACTION_REGIONS 1000000u
#define SC_FACTIONS_SAVE_BYTES 20064u
#define SC_FACTION_REGION_BYTES 72u
typedef enum { SITE_NONE, SITE_CROWN, SITE_POWER, SITE_BIOMASS, SITE_FABRICATOR } FactionSite;
typedef enum {
    ORDER_NONE,
    ORDER_PATROL,
    ORDER_RAID,
    ORDER_ESCORT,
    ORDER_REINFORCE,
    ORDER_OCCUPY,
    ORDER_RETREAT,
    FORCE_ORDERS
} ForceOrder;
enum {
    FS_SEEN = 1,
    FS_CAPTURED = 2,
    FS_RAIDED = 4,
    FS_REPAIRED = 8,
    FS_DARK = 16,
    FS_GROWTH = 32,
    FS_STASH = 64,
    FS_SPENT = 128
};
typedef struct {
    Cell region, site;
    uint32_t owner, kind, flags, epoch, casualties, systems;
    Cell edit_site;
    uint32_t seen, edit, edit_claims;
} FactionRegion;
typedef struct {
    uint32_t supplies[3], losses, ruler, cycle, next_order, next_relation;
} FactionNation;
typedef struct {
    Cell site;
    uint32_t strikes, reports, witness, member;
    float local_time, grace, transmit;
} FactionIncident;
typedef struct {
    uint32_t id, country, order, count, cargo, generation;
    Cell home, goal, next;
    float age, wait, route_time, stuck;
    Vec3 waypoint;
    Enemy members[SC_FORCE_MEMBERS];
    uint32_t participation[2];
    uint32_t claimed, arrivals, siege, present;
    bool physical;
    /* Disposable navigation cache; persistent positions and lift progress are above. */
    Vec3 route_point[SC_FORCE_MEMBERS];
    Cell route_from[SC_FORCE_MEMBERS], route_next[SC_FORCE_MEMBERS];
    float nav_time[SC_FORCE_MEMBERS];
    uint8_t nav_kind[SC_FORCE_MEMBERS];
} FactionForce;
typedef enum {
    FE_REGION,
    FE_LAUNCH,
    FE_ARRIVE,
    FE_LOSS,
    FE_CAPTURE,
    FE_RETREAT,
    FE_RELATION,
    FE_SUCCESSION,
    FE_SYSTEM,
    FE_DIRECTOR,
    FE_REJECT,
    FE_EXPERIENCE,
    FE_EVENTS
} FactionEventKind;
typedef struct {
    uint32_t tick, kind, country, force, detail;
    Cell site;
} FactionEvent;
typedef enum { PACE_BUILDUP, PACE_PRESSURE, PACE_RECOVERY, PACE_DISCOVERY } DirectionPace;
typedef enum {
    EDIT_NONE,
    EDIT_SCOUT,
    EDIT_STASH,
    EDIT_NEST,
    EDIT_ROUTE,
    EDIT_DELAY,
    EDIT_WEATHER,
    EDIT_CANCEL,
    EDIT_WITHDRAW,
    EDIT_SUBSTITUTE,
    DIRECTOR_EDITS
} DirectionEdit;
enum { EXPERIENCE_COMBAT = 2, EXPERIENCE_AVOIDED = 4, EXPERIENCE_CONTENT = 8 };
typedef struct {
    uint32_t kind, phase, player, force, cost, sequence;
    Cell site;
    float remaining;
} DirectionProposal;
typedef struct {
    uint32_t pace[2], history[16], sequence, budget, accepted, rejected;
    Cell last_room[2];
    float pressure[2], quiet[2], recovery[2], cooldown, recharge;
    float defenses[2];
    DirectionProposal proposal;
} HiddenDirector;
typedef struct {
    uint32_t version, tick, random, next_id, event_count;
    float remainder;
    FactionNation nations[3];
    uint32_t relations[3][3];
    FactionIncident incidents[2][3];
    FactionRegion *regions;
    size_t count, capacity;
    FactionForce forces[SC_FACTION_FORCES];
    FactionEvent events[SC_FACTION_EVENTS];
    HiddenDirector director;
    uint32_t launched, arrived, casualties, captures, retreats, decisions;
} Factions;
struct Game;
void factions_init(struct Game *g);
void factions_destroy(Factions *f);
void factions_plan(uint32_t seed, RegionPlan *plan);
void factions_populate(World *w, Room *room);
const FactionRegion *factions_at(const Factions *f, Cell cell);
FactionRegion *factions_record(struct Game *g, Cell cell);
unsigned factions_owner(const struct Game *g, Cell cell);
unsigned factions_relation(const struct Game *g, unsigned a, unsigned b);
bool factions_hostile(const struct Game *g, const Enemy *e, int player);
bool factions_safe(const struct Game *g, Cell cell);
bool factions_required(const struct Game *g, Cell cell);
bool factions_active(const struct Game *g);
bool factions_observed(const struct Game *g, Vec3 point, double margin);
void factions_log(struct Game *g, unsigned kind, unsigned country, unsigned force, Cell site,
                  unsigned detail);
void factions_step(struct Game *g, double dt);
bool factions_valid(const Factions *f, unsigned generator);
bool factions_world_valid(const Factions *f, const World *world, unsigned layer);
FactionForce *factions_force(struct Game *g, uint32_t id);
unsigned factions_strength(const FactionForce *force);
bool factions_launch(struct Game *g, unsigned country, ForceOrder order, Cell source, Cell goal);
void factions_forces_sync(struct Game *g, bool detach);
void factions_forces_step(struct Game *g, double dt);
void factions_forces_tick(struct Game *g);
void factions_damage(struct Game *g, Room *room, int index, float damage, int player);
void factions_attack(struct Game *g, const Enemy *e, int player, float damage);
void factions_tactics(struct Game *g, double dt);
bool factions_feral(struct Game *g, Room *room, Enemy *enemy, double dt);
void factions_orders(struct Game *g);
void factions_incidents(struct Game *g, double dt);
void factions_systems(struct Game *g, Cell site, uint32_t systems, int player);
void hidden_director_step(struct Game *g, double dt);
unsigned factions_interest(struct Game *g, FactionRegion **sites, unsigned capacity);
bool hidden_director_propose(struct Game *g, unsigned kind, int player, Cell site, uint32_t force);
void factions_environment(struct Game *g);
#endif
