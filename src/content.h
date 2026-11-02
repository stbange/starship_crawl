#ifndef SC_CONTENT_H
#define SC_CONTENT_H
#include "math3d.h"
#include <stdint.h>
typedef enum { BAY, NAVE, ENGINE, CREW, CONDUIT, GARDEN, ROOM_TYPES } RoomType;
typedef enum {
    HABITAT,
    MACHINERY,
    SANCTUM,
    LUXURY,
    COMMAND,
    RESEARCH,
    LOGISTICS,
    CRYONICS,
    BOTANY,
    DISTRICT_TYPES
} District;
#define SC_THEMES 28
#define SC_ENEMY_TYPES 27
#define SC_WEAPONS 16
#define SC_ATTRIBUTES 10
typedef struct {
    const char *name;
    RoomType architecture;
    District sector;
} RoomTheme;
enum {
    CLIMB = 1,
    LEAP = 2,
    CHARGE = 4,
    ARMORED = 8,
    REGENERATE = 16,
    HEALER = 32,
    BROOD = 64,
    STRAFE = 128,
    CLOAK = 256,
    BURST = 512,
    DETONATE = 1024,
    CHILL = 2048,
    DRAIN = 4096,
    HOVER = 8192,
    TURRET = 16384,
    WARD = 32768
};
typedef struct {
    const char *name;
    float health, speed, size, damage, interval;
    uint32_t flags;
    uint8_t role, tier;
    Color color;
} EnemyDef;
enum { W_PROJECTILE = 1, W_BLAST = 2, W_BURN = 4, W_CHILL = 8, W_LEECH = 16, W_CHAIN = 32 };
typedef struct {
    const char *name;
    float damage, interval, range, spread, blast, cost;
    int pellets, pierce;
    uint32_t flags;
    Color color;
} WeaponDef;
extern const RoomTheme sc_themes[SC_THEMES];
extern const EnemyDef sc_enemies[SC_ENEMY_TYPES];
extern const WeaponDef sc_weapons[SC_WEAPONS];
extern const char *sc_attributes[SC_ATTRIBUTES];
extern const char *sc_logs[12];
#endif
