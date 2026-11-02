#ifndef SC_GAME_H
#define SC_GAME_H
#include "world.h"
#define SC_PLAYERS 2
#define SC_MAX_PROJECTILES 96
typedef struct {
    Position pos, checkpoint;
    Vec3 velocity;
    double yaw, pitch;
    float health, cooldown, hurt, muzzle, lift_cooldown;
    uint32_t xp, level, kills, points, power, vitality, haste, tint;
    uint32_t mods[7], weapons, weapon, credits, logs, victories, pickups;
    float shield, armor, energy, slow;
    uint8_t reading;
    bool grounded, crouched, flashlight, journal;
} Player;
typedef struct {
    float forward, strafe, vertical;
    bool fire, jump, crouch, lift_up, lift_down, sprint;
} Actions;
typedef struct {
    Position pos;
    Vec3 velocity;
    float life, damage;
    bool active;
    int owner;
    uint32_t flags;
    float blast;
    Color color;
} Projectile;
typedef struct {
    World world;
    Player players[SC_PLAYERS];
    int player_count;
    double time;
    float message_time;
    char message[128];
    uint32_t shots, hits, respawns;
    Projectile projectiles[SC_MAX_PROJECTILES];
    uint32_t enemy_shots, projectile_hits;
    uint32_t mission, mission_kills, mission_loot, contracts;
    bool dirty;
} Game;
void game_init(Game *g, uint32_t seed);
void game_destroy(Game *g);
void game_step(Game *g, const Actions *actions, double dt);
void game_look(Player *p, double yaw, double pitch);
void game_join(Game *g);
void game_leave(Game *g);
void game_new_ship(Game *g, uint32_t seed);
bool game_upgrade(Game *g, int player, int upgrade);
float player_max_health(const Player *p);
unsigned player_next_level(const Player *p);
Vec3 player_eye(const Player *p);
void game_message(Game *g, const char *message);
void game_flashlight(Game *g, int player);
Vec3 enemy_center(const Enemy *e);
float player_max_shield(const Player *p);
float player_max_armor(const Player *p);
float player_max_energy(const Player *p);
uint32_t *player_attribute(Player *p, int attribute);
void game_progression_step(Game *g, double dt);
void game_weapon(Game *g, int player, int delta);
void game_interact(Game *g, int player);
void game_hurt(Game *g, int player, float damage, uint32_t flags);
bool game_objective(const Game *g, Cell *cell);
const char *game_mission(const Game *g);
Vec3 game_terminal(Cell cell);
bool game_save(const Game *g, const char *path);
bool game_load(Game *g, const char *path);
#endif
