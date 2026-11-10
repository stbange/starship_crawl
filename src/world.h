#ifndef SC_WORLD_H
#define SC_WORLD_H
#include "content.h"
#include "identity.h"
#include <stdbool.h>
#include <stddef.h>
#define SC_GENERATOR_VERSION 16u
#define SC_OLDEST_GENERATOR 3u
#define SC_CELL 96.0
#define SC_DECK 64.0
#define SC_REGION 4
#define SC_REGION_CELLS 16
#define SC_MAX_CHUNKS (96 * SC_PLAYERS)
#define SC_MAX_ROOMS SC_MAX_CHUNKS
#define SC_LEGACY_BOXES 256
#define SC_CHUNK_BOXES 512
#define SC_CHUNK_FLIGHTS 16
#define SC_STREAM_BUDGET 2
#define SC_MAX_ENEMIES 20
#define SC_CHUNK_LIGHTS 6
#define SC_CHUNK_DRESSING 32
#define SC_CHUNK_SPRITES 32
typedef struct {
    int32_t x, y, z;
} Cell;
typedef struct {
    Cell sector;
    Vec3 local;
} Position;
typedef enum { WEST, EAST, SOUTH, NORTH } Direction;
typedef enum { PILGRIM_ARK, CROWN_LINER, SIEGE_CARRIER, SEED_VAULT, SHIP_PROFILES } ShipProfile;
typedef enum { SUITES, WORKBAYS, ARCADES, INTERIOR_LAYOUTS } InteriorLayout;
typedef enum { QUIET, BLACKOUT, FROST, QUARANTINE, FIRE, BLOOM, INCIDENT_TYPES } Incident;
typedef enum {
    CRYO_RUPTURE,
    XENO_OUTBREAK,
    MACHINE_PURGE,
    REACTOR_CASCADE,
    HISTORY_CAUSES
} HistoryCause;
typedef enum { DIRECT_COMMISSION, DEPENDENT_TARGET } HistoryArc;
typedef enum { HISTORY_NONE, HISTORY_ORIGIN, HISTORY_SURVIVORS, HISTORY_CONTROL } HistoryRole;
typedef struct {
    HistoryCause cause;
    HistoryArc arc;
    Cell origin;
    unsigned population;
} ShipHistory;
typedef enum {
    NO_LANDMARK,
    BANQUET,
    CONSERVATORY,
    SUSPENDED_ENGINE,
    RELIQUARY,
    LANDMARK_TYPES
} Landmark;
typedef struct {
    bool open, crouch;
    double width, clearance;
    Vec3 local_center;
} Connector;
typedef enum {
    STILL_AIR,
    STEAM_LEAK,
    FLOODED,
    SPORE_AIR,
    RIME_AIR,
    EMBER_AIR,
    VOID_GLOW,
    BOILING_AIR,
    PRESSURE_AIR,
    ASH_AIR,
    HEAT_HAZE,
    ICE_SHARDS,
    FROZEN_RUNOFF,
    COLD_CLEAR,
    ATMOSPHERES
} AtmosphereKind;
typedef enum {
    EVACUATION,
    LAST_FEAST,
    TRIAGE,
    CONTAINMENT,
    REPAIR,
    PILGRIMAGE,
    MUTINY,
    CULTIVATION,
    LOCAL_TALES
} LocalTale;
typedef enum { FERAL, MECHANICAL, BROOD_NEST, CRYOPHILE, PHANTOM, SCAVENGER, ECOLOGIES } Ecology;
typedef enum { PACE_SWARM, PACE_QUIET, PACE_TRACE, PACE_FRONTIER } RoomPace;
typedef struct {
    uint16_t mask;        /* Four-bit rows relative to the owner; zero means a legacy rectangle. */
    uint8_t form, volume; /* 0 ordinary, 1 lower vault, 2 upper gallery. */
    uint8_t atmosphere, tale, ecology;
    uint8_t pace;                        /* Generator 11: density and optional encounter grammar. */
    uint8_t challenge, reveal, contract; /* Generator 12; zero retains older behavior. */
    uint8_t country, court; /* Generator 13; political ownership never changes geometry. */
} Habitat;
typedef struct {
    Cell cell;
    RoomType type;
    District district;
    uint8_t theme, layout, incident, intensity, landmark, history_role;
    uint8_t span_x, span_z;
    Habitat habitat;
    double width, depth, height;
    uint32_t hash;
} RoomSpec;
typedef struct {
    Cell origin;
    District district;
    RoomSpec rooms[SC_REGION_CELLS];
    uint8_t owner[SC_REGION_CELLS], doors[SC_REGION_CELLS];
    int count;
} RegionPlan;
typedef struct {
    Vec3 min, max;
    Color color;
    float glow;
    bool solid;
} Box;
typedef struct {
    Cell cell;
    uint32_t killed, collected, systems, local;
} RoomRecord;
#define SC_LOCAL_STATE_MASK                                                                        \
    (3u | ((SC_PLAYER_MASK & 3u) << 2) | ((SC_PLAYER_MASK & 3u) << 4) |                            \
     ((SC_PLAYER_MASK & 12u) << 4) | ((SC_PLAYER_MASK & 12u) << 6))
static inline uint32_t world_eligible_bit(unsigned player) {
    return 1u << (player < 2 ? player + 2 : player + 4);
}
static inline uint32_t world_claim_bit(unsigned player) {
    return 1u << (player < 2 ? player + 4 : player + 6);
}
static inline uint32_t world_participants(uint32_t local) {
    return ((local >> 2) & 3u) | ((local >> 4) & 12u);
}
enum { SYSTEM_POWER = 1, SYSTEM_BEACON = 2, SYSTEM_SILENCED = 4, SYSTEM_BLACKOUT = 8 };
enum {
    LOCAL_RESOLVED = 1,
    FRONTIER_CLEARED = 2,
    FRONTIER_ELIGIBLE = 4,
    FRONTIER_CLAIMED = 16
}; /* Legacy eligible/claimed bits retain their positions; expanded slots use higher bits. */
typedef enum {
    ALIEN_NONE,
    ALIEN_PILGRIM,
    ALIEN_GAZER,
    ALIEN_SEAM,
    ALIEN_CONDUCTOR,
    ALIEN_LISTENER,
    ALIEN_WATCHED,
    ALIEN_REVERSER,
    ALIEN_COLONY,
    ALIEN_MOLTER,
    ALIEN_MACHINE,
    ALIEN_AMBUSH,
    ALIEN_REFLECTION,
    ALIEN_BRITTLE,
    ALIEN_PRESSURE,
    ALIEN_BEHAVIORS
} AlienBehavior;
typedef enum {
    MOTION_DORMANT,
    MOTION_ALERT,
    MOTION_APPROACH,
    MOTION_WARN,
    MOTION_COMMIT,
    MOTION_RECOVER
} AlienMotion;
typedef struct {
    Position pos;
    float health, cooldown, flash;
    float windup, climb_time, vertical_speed, special, slow, burn;
    Vec3 aim, normal, heading;
    uint8_t kind, id, last_player, allegiance, ally_owner, ecology;
    uint8_t country;
    uint32_t force; /* Persistent mobile identity; zero keeps the static room kill mask. */
    uint8_t member;
    bool alive, climbing;
    uint8_t behavior, motion;
    float motion_time, stride, travel_speed, link_strain;
    Vec3 body_heading, contacts[4];
    bool attack_done;
    bool molted, reversed, observed;
    float warning_sound, unfold;
    Vec3 home, fragments[3];
} Enemy;
typedef struct {
    Vec3 pos;
    Color color;
    float radius, power;
    uint32_t phase;
} PointLight;
typedef enum { GROWTH, NEST, CABLE, SCRAP } DressingKind;
typedef struct {
    Vec3 pos, normal;
    uint32_t hash;
    DressingKind kind;
    float scale;
} Dressing;
typedef enum {
    SPR_STEAM,
    SPR_RAIN,
    SPR_WATER,
    SPR_MIST,
    SPR_MOTE,
    SPR_FUNGUS,
    SPR_CLOTH,
    SPR_BONES,
    SPR_PAPERS,
    SPR_GLYPH,
    SPR_SPARK,
    SPR_ASH,
    SPR_CRYSTAL,
    SPR_BUBBLE,
    SPR_SPRITES
} SpriteKind;
typedef struct {
    Vec3 pos, normal;
    float width, height;
    uint32_t hash;
    uint8_t kind, atmosphere;
} EnvSprite;
typedef struct {
    Position pos;
    uint8_t kind, item, id;
    bool active;
} Pickup;
enum { PICK_MEDKIT, PICK_SHIELD, PICK_ARMOR, PICK_WEAPON, PICK_LOG, PICK_ENERGY, PICK_SALVAGE };
typedef struct {
    Cell cell;
    RoomType type;
    District district;
    uint8_t theme, layout, incident, intensity, landmark, history_role;
    uint8_t span_x, span_z;
    Habitat habitat;
    double width, depth, height;
    uint32_t hash;
    uint64_t residency;
    int pickup_count;
    Pickup pickups[5];
    size_t record;
    bool ready;
    int enemy_count;
    Enemy enemies[SC_MAX_ENEMIES];
    bool frontier_active;
    uint8_t recipe; /* 0 keeps older encounters; 1..6 are authored role combinations. */
    uint8_t frontier_attacker;
    float frontier_timer, frontier_disrupt;
    float hazard_time, circuit_time, alarm_time;
    uint8_t circuit, hazard_stage;
    uint8_t faction_edit; /* Applied once per streamed room; durable decision lives in Factions. */
} Room;
/* Geometry is relative to its integer chunk coordinate, never a distant camera. */
/* A thin sloped slab, optionally quantized into 32 physical stair treads. */
typedef struct {
    Vec3 min, max;
    Color color;
    bool reverse, stairs;
} Flight;
typedef struct {
    Cell cell, owner;
    Box *boxes;
    size_t box_count;
    Flight flights[SC_CHUNK_FLIGHTS];
    int flight_count;
    PointLight lights[SC_CHUNK_LIGHTS];
    Dressing dressing[SC_CHUNK_DRESSING];
    int light_count, dressing_count;
    EnvSprite sprites[SC_CHUNK_SPRITES];
    int sprite_count;
    uint32_t serial;
    bool active;
} Chunk;
typedef struct {
    uint32_t seed, generator;
    unsigned open_layer; /* Zero disables phase barriers for legacy/inspection worlds. */
    Room rooms[SC_MAX_ROOMS];
    int count;
    Chunk chunks[SC_MAX_CHUNKS];
    int chunk_count;
    RoomRecord *records;
    size_t record_count, record_cap;
    size_t *record_index, index_cap;
    size_t box_count;
    uint32_t revision, generated;
    uint64_t next_residency;
    bool failed;
    Cell anchors[SC_PLAYERS], desired[SC_MAX_CHUNKS];
    int anchor_count, desired_count, pending, next_player;
    unsigned last_work;
} World;
uint32_t world_hash(uint32_t seed, Cell cell, uint32_t salt);
bool cell_equal(Cell a, Cell b);
Cell world_cell(Vec3 p);
Cell world_region(Cell cell);
Vec3 world_center(Cell c);
Position position_from(Vec3 absolute);
Vec3 position_absolute(Position p);
Vec3 position_delta(Position a, Position b);
void position_move(Position *p, Vec3 delta);
Vec3 world_room_center(const Room *r);
unsigned world_difficulty(Cell c);
const char *world_room_name(RoomType type);
const char *world_district_name(District district);
void world_plan(uint32_t seed, Cell region, RegionPlan *plan);
RoomSpec world_describe(uint32_t seed, Cell cell);
bool world_door(uint32_t seed, Cell cell, Direction direction);
Connector world_connector(uint32_t seed, Cell cell, Direction direction);
Cell world_neighbor(Cell cell, Direction direction);
uint32_t world_fingerprint(uint32_t seed, Cell region);
/* Saved expeditions select their own generator; default wrappers create new ships. */
void world_plan_version(uint32_t seed, uint32_t generator, Cell region, RegionPlan *plan);
RoomSpec world_describe_version(uint32_t seed, uint32_t generator, Cell cell);
bool world_door_version(uint32_t seed, uint32_t generator, Cell cell, Direction direction);
Connector world_connector_version(uint32_t seed, uint32_t generator, Cell cell,
                                  Direction direction);
uint32_t world_fingerprint_version(uint32_t seed, uint32_t generator, Cell region);
ShipProfile world_profile(uint32_t seed);
ShipHistory world_history(uint32_t seed);
unsigned world_history_theme(HistoryCause cause);
Incident world_history_incident(HistoryCause cause);
const char *world_history_cause(HistoryCause cause);
const char *world_history_controller(HistoryCause cause);
const char *world_history_agenda(uint32_t seed);
const char *world_history_role(unsigned role);
void world_history_notice(uint32_t seed, unsigned role, char *text, size_t size);
const char *world_profile_name(ShipProfile profile);
const char *world_layout_name(unsigned layout);
const char *world_incident_name(unsigned incident);
const char *world_landmark_name(unsigned landmark);
void world_init(World *w, uint32_t seed);
void world_destroy(World *w);
/* Request work only. Session ticks own simulation pumping; GPU uploads are independent. */
bool world_refresh(World *w, const Vec3 *positions, int players);
unsigned world_stream(World *w, unsigned budget);
Room *world_find(World *w, Cell cell);
const Chunk *world_chunk(const World *w, Cell cell);
bool world_ready(const World *w, Cell cell);
Vec3 world_lift(Cell cell);
bool world_service_column(uint32_t seed, Cell cell);
double flight_top(const Flight *flight, double z);
bool flight_overlap(const Flight *flight, Vec3 feet, double radius, double height);
double flight_ray(const Flight *flight, Vec3 origin, Vec3 direction, double limit);
/* Shared physical movement. Stepping is enabled only for grounded walkers. */
void world_move(const World *world, Position *position, Vec3 delta, double radius, double height,
                bool walking, bool *grounded);
bool world_overlap(const World *w, Vec3 feet, double radius, double height);
bool world_spawn_clear(const World *w, Vec3 feet, double radius, double height);
bool world_spawn_supported(const World *w, Vec3 feet, double height);
double world_raycast(const World *w, Vec3 origin, Vec3 direction, double limit);
void world_kill(World *w, Room *room, int enemy);
uint32_t world_systems(const World *w, Cell cell);
bool world_set_systems(World *w, Cell cell, uint32_t systems);
uint32_t world_local_state(const World *w, const Room *room);
bool world_mark_local(World *w, Room *room, uint32_t bits);
void world_collect(World *w, Room *room, int pickup);
bool world_index_records(World *w);
float world_lamp_power(uint32_t phase, double time);
#endif
