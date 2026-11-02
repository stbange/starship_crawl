#ifndef SC_WORLD_H
#define SC_WORLD_H
#include "math3d.h"
#include <stdbool.h>
#include <stddef.h>
#define SC_CELL 96.0
#define SC_DECK 32.0
#define SC_MAX_ROOMS 64
#define SC_MAX_ENEMIES 20
typedef struct {
    int32_t x, y, z;
} Cell;
typedef enum { BAY, NAVE, ENGINE, CREW, CONDUIT, GARDEN, ROOM_TYPES } RoomType;
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
    Vec3 pos;
    float health, cooldown, flash;
    uint8_t kind, id;
    bool alive;
} Enemy;
typedef struct {
    Cell cell;
    RoomType type;
    double width, depth, height;
    uint32_t hash;
    size_t record;
    int enemy_count;
    Enemy enemies[SC_MAX_ENEMIES];
} Room;
typedef struct {
    uint32_t seed;
    Room rooms[SC_MAX_ROOMS];
    int count;
    RoomRecord *records;
    size_t record_count, record_cap;
    Box *boxes;
    size_t box_count, box_cap;
    uint32_t revision;
    bool failed;
    Cell anchors[2];
    int anchor_count;
} World;
uint32_t world_hash(uint32_t seed, Cell cell, uint32_t salt);
bool cell_equal(Cell a, Cell b);
Cell world_cell(Vec3 p);
Vec3 world_center(Cell c);
unsigned world_difficulty(Cell c);
const char *world_room_name(RoomType type);
void world_init(World *w, uint32_t seed);
void world_destroy(World *w);
bool world_refresh(World *w, const Vec3 *positions, int players);
Room *world_find(World *w, Cell cell);
Vec3 world_lift(Cell cell);
bool world_overlap(const World *w, Vec3 feet, double radius, double height);
double world_raycast(const World *w, Vec3 origin, Vec3 direction, double limit);
void world_kill(World *w, Room *room, int enemy);
#endif
