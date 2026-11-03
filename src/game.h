#ifndef SC_GAME_H
#define SC_GAME_H
#include "world.h"
#define SC_PLAYERS 2
#define SC_MAX_PROJECTILES 192
#define SC_MAP_SIDE 9
#define SC_PSIONICS 4
typedef enum { PSI_VEIL, PSI_FLIGHT, PSI_LEAP, PSI_NOVA } Psionic;
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
    bool sheltered;
    Vec3 shelter_pos;
    double return_yaw, return_pitch;
    float resting, nourished;
    bool cloaked, map_open;
    float flight, leap, nova, nova_tick, nova_flash;
    uint8_t selected_power;
    Cell map_center;
} Player;
typedef struct {
    Position door;
    double yaw;
    bool deployed;
} Shelter;
typedef enum { SHELTER_NONE, SHELTER_DOOR, SHELTER_BED, SHELTER_MED, SHELTER_KITCHEN } ShelterUse;
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
    int pierce, hit_count;
    Cell hit_rooms[4];
    uint8_t hit_ids[4];
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
    Shelter shelter;
    bool dirty;
} Game;
typedef struct {
    Cell origin, player, objective, shelter;
    RoomSpec rooms[SC_MAP_SIDE * SC_MAP_SIDE];
    uint8_t doors[SC_MAP_SIDE * SC_MAP_SIDE];
    bool valid[SC_MAP_SIDE * SC_MAP_SIDE], has_objective, has_shelter;
    int side, plans;
} MapView;
void game_map_toggle(Game *g, int player);
void game_map_pan(Game *g, int player, int x, int y, int z);
bool game_map_build(const Game *g, int player, Cell center, int radius, MapView *view);
void game_psionic(Game *g, int player, Psionic power);
void game_psionics_step(Game *g, Player *p, double dt);
void game_nova(Game *g, int player);
extern const char *sc_psionic_names[SC_PSIONICS];
extern const float sc_psionic_costs[SC_PSIONICS];
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
void game_summon(Game *g, int player);
ShelterUse game_shelter_near(const Game *g, int player);
bool game_shelter_interact(Game *g, int player);
void game_shelter_step(Game *g, Player *p, const Actions *a, double dt);
const Box *shelter_furniture(size_t *count);
Vec3 shelter_station(ShelterUse use);
bool shelter_clear(Vec3 feet);
bool game_save(const Game *g, const char *path);
bool game_load(Game *g, const char *path);
#endif
