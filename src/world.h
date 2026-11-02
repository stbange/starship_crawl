#ifndef SC_WORLD_H
#define SC_WORLD_H
#include "math3d.h"
#include <stdbool.h>
#include <stddef.h>
#define SC_GENERATOR_VERSION 2u
#define SC_CELL 96.0
#define SC_DECK 64.0
#define SC_REGION 4
#define SC_REGION_CELLS 16
#define SC_MAX_CHUNKS 192
#define SC_MAX_ROOMS SC_MAX_CHUNKS
#define SC_CHUNK_BOXES 256
#define SC_STREAM_BUDGET 2
#define SC_MAX_ENEMIES 20
typedef struct {
    int32_t x, y, z;
} Cell;
typedef struct {
    Cell sector;
    Vec3 local;
} Position;
typedef enum { BAY, NAVE, ENGINE, CREW, CONDUIT, GARDEN, ROOM_TYPES } RoomType;
typedef enum { HABITAT, MACHINERY, SANCTUM, DISTRICT_TYPES } District;
typedef enum { WEST, EAST, SOUTH, NORTH } Direction;
typedef struct {
    bool open, crouch;
    double width, clearance;
    Vec3 local_center;
} Connector;
typedef struct {
    Cell cell;
    RoomType type;
    District district;
    uint8_t span_x, span_z;
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
    uint32_t killed;
} RoomRecord;
typedef struct {
    Position pos;
    float health, cooldown, flash;
    uint8_t kind, id;
    bool alive;
} Enemy;
typedef struct {
    Cell cell;
    RoomType type;
    District district;
    uint8_t span_x, span_z;
    double width, depth, height;
    uint32_t hash;
    size_t record;
    bool ready;
    int enemy_count;
    Enemy enemies[SC_MAX_ENEMIES];
} Room;
/* Geometry is relative to its integer chunk coordinate, never a distant camera. */
typedef struct {
    Cell cell, owner;
    Box *boxes;
    size_t box_count;
    uint32_t serial;
    bool active;
} Chunk;
typedef struct {
    uint32_t seed, generator;
    Room rooms[SC_MAX_ROOMS];
    int count;
    Chunk chunks[SC_MAX_CHUNKS];
    int chunk_count;
    RoomRecord *records;
    size_t record_count, record_cap;
    size_t *record_index, index_cap;
    size_t box_count;
    uint32_t revision, generated;
    bool failed;
    Cell anchors[2], desired[SC_MAX_CHUNKS];
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
void world_init(World *w, uint32_t seed);
void world_destroy(World *w);
/* Request work only. Pump once per displayed frame, also while paused. */
bool world_refresh(World *w, const Vec3 *positions, int players);
unsigned world_stream(World *w, unsigned budget);
Room *world_find(World *w, Cell cell);
const Chunk *world_chunk(const World *w, Cell cell);
bool world_ready(const World *w, Cell cell);
Vec3 world_lift(Cell cell);
bool world_overlap(const World *w, Vec3 feet, double radius, double height);
double world_raycast(const World *w, Vec3 origin, Vec3 direction, double limit);
void world_kill(World *w, Room *room, int enemy);
bool world_index_records(World *w);
#endif
