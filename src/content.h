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
#define SC_ENEMY_TYPES 32
#define SC_WEAPONS 20
#define SC_GENERATED_WEAPONS 16
#define SC_MECH_WEAPONS 3
#define SC_FLARE_CANNON 19
#define SC_NEW_WEAPONS (15u << 16)
#define SC_START_WEAPONS (7u | SC_NEW_WEAPONS)
#define SC_ATTRIBUTES 10
#define SC_BODY_FORMS 6
#define SC_BODY_VARIANTS 5
typedef enum { HUSK, GAZER, VOID_RAY, WORM, CONSTRUCT, ROBOT } BodyForm;
typedef enum { DEATH_NONE, DEATH_ACID, DEATH_BLAST, DEATH_MINE } DeathEffect;
extern const DeathEffect sc_death_effects[SC_ENEMY_TYPES];
typedef struct {
    uint8_t form, variant;
} EnemyBody;
extern const EnemyBody sc_enemy_bodies[SC_ENEMY_TYPES];
extern const char *sc_body_names[SC_BODY_FORMS];
extern const char *sc_body_variants[SC_BODY_FORMS][SC_BODY_VARIANTS];
extern const float sc_projectile_speeds[SC_WEAPONS];
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
enum {
    W_PROJECTILE = 1,
    W_BLAST = 2,
    W_BURN = 4,
    W_CHILL = 8,
    W_LEECH = 16,
    W_CHAIN = 32,
    W_BEAM = 64,
    W_FLARE = 128
};
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
extern const WeaponDef sc_mech_weapons[SC_MECH_WEAPONS];
extern const char *sc_attributes[SC_ATTRIBUTES];
extern const char *sc_logs[12];
#endif
