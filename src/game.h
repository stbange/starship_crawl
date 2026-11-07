#ifndef SC_GAME_H
#define SC_GAME_H
#include "world.h"
#include "story.h"
#include "matrix.h"
#include "campaign.h"
#include "encounters.h"
#include "frontier.h"
#include "politics.h"
#define SC_PLAYERS 2
#define SC_MAX_PROJECTILES 192
#define SC_MAP_SIDE 9
#define SC_PSIONICS 4
#define SC_MAX_FLARES 32
#define SC_MAX_DEATHS 64
#define SC_MAX_DEATH_HAZARDS 32
#define SC_DEATH_LIFE 2.8f
typedef enum { ON_FOOT, MECH, WAGON } RigKind;
typedef enum { PSI_VEIL, PSI_FLIGHT, PSI_LEAP, PSI_NOVA } Psionic;
typedef enum {
    NAV_NONE,
    NAV_BOARD,
    NAV_DOOR,
    NAV_LIFT,
    NAV_GOAL,
    NAV_LANDING,
    NAV_BLOCKED
} NavKind;
typedef struct {
    Cell from, goal, next;
    Vec3 point;
    NavKind kind;
    unsigned searched;
} Navigation;
typedef struct Player {
    Position pos, checkpoint;
    Position lift_from, lift_to;
    float lift_progress;
    bool lift_active;  /* Transient: saves snapshot the departure pad. */
    bool lift_latched; /* Release at the landing before requesting another ride. */
    float arrival_time, navigation_time;
    unsigned shots_fired;  /* Transient, per-player field-drill observation. */
    Navigation navigation; /* Derived, per-view, never serialized. */
    Vec3 velocity;
    double yaw, pitch;
    float health, cooldown, hurt, muzzle, lift_cooldown;
    uint32_t xp, level, kills, points, power, vitality, haste, tint;
    uint32_t mods[7], weapons, weapon, credits, logs, victories, pickups;
    float shield, armor, energy, slow;
    uint8_t reading;
    uint8_t journal_page, story_reading, decision_scene, event_reading;
    uint32_t career[SC_OUTCOMES];
    bool decision_open;
    bool grounded, crouched, flashlight, journal;
    bool sheltered;
    Vec3 shelter_pos;
    double return_yaw, return_pitch;
    float resting, nourished;
    bool cloaked, map_open;
    float flight, leap, nova, nova_tick, nova_flash;
    uint8_t selected_power;
    Cell map_center;
    uint32_t riding, rig_weapon, flare_lifetime, flare_strength;
    float drive_speed;
    float noise, relic_cooldown;
} Player;
typedef struct Shelter {
    Position door;
    double yaw;
    bool deployed;
} Shelter;
typedef enum {
    SHELTER_NONE,
    SHELTER_DOOR,
    SHELTER_BED,
    SHELTER_MED,
    SHELTER_KITCHEN,
    SHELTER_TERMINAL
} ShelterUse;
typedef struct Actions {
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
    uint8_t guardian;
} Projectile;
typedef struct {
    Position pos;
    float armor;
    bool deployed;
    double yaw;
} Rig;
typedef struct {
    Position from, to;
    Color color;
    float life;
} Beam;
typedef struct {
    Position pos;
    Vec3 normal;
    float life, radius, power;
    uint32_t owner;
    bool active;
} Flare;
typedef struct {
    Enemy body;
    Vec3 landing;
    float age;
    bool active;
} EnemyDeath;
typedef struct {
    Position pos;
    DeathEffect kind;
    float life, age, timer, radius, damage;
    int owner;
    bool active, triggered, spent;
} DeathHazard;
typedef struct Game {
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
    Story story;
    Shelter shelter;
    Rig rigs[SC_PLAYERS][2];
    Beam beams[SC_PLAYERS];
    Flare flares[SC_MAX_FLARES];
    EnemyDeath deaths[SC_MAX_DEATHS];
    DeathHazard death_hazards[SC_MAX_DEATH_HAZARDS];
    MatrixState matrix;
    Campaign campaign;
    RewardDrill drills[SC_PLAYERS];
    FrontierProgress frontier;
    Politics politics;
    Director director;
    bool dirty;
} Game;
typedef struct {
    Cell origin, player, objective, shelter;
    RoomSpec rooms[SC_MAP_SIDE * SC_MAP_SIDE];
    uint8_t doors[SC_MAP_SIDE * SC_MAP_SIDE];
    uint32_t systems[SC_MAP_SIDE * SC_MAP_SIDE];
    bool valid[SC_MAP_SIDE * SC_MAP_SIDE], has_objective, has_shelter;
    int side, plans;
} MapView;
void game_lift_cancel(Game *g, int player);
void game_map_toggle(Game *g, int player);
void game_map_pan(Game *g, int player, int x, int y, int z);
bool game_map_build(const Game *g, int player, Cell center, int radius, MapView *view);
bool game_navigation_to(const Game *g, int player, Cell goal, Navigation *nav);
void game_navigation_step(Game *g, double dt);
void game_psionic(Game *g, int player, Psionic power);
void game_psionics_step(Game *g, Player *p, double dt);
void game_nova(Game *g, int player);
void game_damage_enemy(Game *g, Room *room, int index, int owner, float damage, uint32_t flags);
void game_enemy_death(Game *g, const Enemy *enemy, int owner);
void game_deaths_step(Game *g, double dt);
void game_deaths_reset(Game *g);
extern const char *sc_psionic_names[SC_PSIONICS];
extern const float sc_psionic_costs[SC_PSIONICS];
void game_rig_toggle(Game *g, int player, RigKind kind);
bool game_rig_interact(Game *g, int player);
RigKind game_rig_near(const Game *g, int player);
bool game_rig_blocks(const Game *g, int player, Vec3 feet, double radius, double height);
float game_rig_max_armor(const Player *p, RigKind kind);
double game_body_radius(const Player *p);
double game_body_height(const Player *p);
const WeaponDef *game_gun(const Player *p);
float game_shot_speed(const Player *p);
void game_equipment_reset(Game *g);
void game_equipment_step(Game *g, double dt);
void game_rig_repair(Game *g, int player);
void game_rig_damage(Game *g, int player, float *damage);
bool game_flare_upgrade(Game *g, int player, bool strength);
void game_attach_flare(Game *g, int owner, Vec3 point, Vec3 direction);
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
void game_story_step(Game *g);
void game_story_interact(Game *g, int player);
bool game_story_choose(Game *g, int player, unsigned choice);
void game_story_track(Game *g);
void game_story_read(Game *g, int player);
bool game_story_available(const Game *g, unsigned scene);
unsigned game_story_near(const Game *g, int player);
void game_story_entry(const Game *g, unsigned scene, char *text, size_t size);
void game_story_line(const Game *g, int line, char *text, size_t size);
const char *game_story_choice(const Game *g, unsigned choice);
const char *game_story_choice_for(const Game *g, int player, unsigned choice);
bool game_story_choice_ready(const Game *g, int player);
unsigned game_story_target(const Game *g);
unsigned game_story_operation(const Game *g);
bool game_story_prepared(const Game *g);
const char *game_story_title(const Game *g, unsigned scene);
void game_story_dossier(const Game *g, int line, char *text, size_t size);
/* Shared scene operations keep reward/clearance rules consistent across arcs. */
void story_reserve(Game *g, unsigned scene, Cell from);
void story_complete(Game *g, int player, unsigned scene);
bool story_clear(Game *g, unsigned scene);
bool expedition_available(const Story *s, unsigned scene);
void expedition_observe(Game *g, RoomSpec room);
void expedition_record(Game *g, unsigned scene, unsigned choice);
void expedition_interact(Game *g, int player, unsigned scene);
bool expedition_choose(Game *g, int player, unsigned scene, unsigned choice);
void expedition_complete(Game *g, int player, unsigned scene);
void expedition_conclude(Game *g, unsigned choice);
const char *expedition_option(const Game *g, unsigned scene, unsigned choice);
const char *expedition_title(const Game *g, unsigned scene);
void expedition_entry(const Game *g, unsigned scene, char *text, size_t size);
bool expedition_valid(const Story *s, uint32_t seed, uint32_t generator);
void game_story_debrief(const Game *g, int player, int line, char *text, size_t size);
void game_story_event_read(Game *g, int player);
bool game_story_safe(const Game *g, Vec3 feet);
bool game_story_inert(const Game *g, const Enemy *e);
void game_story_recovery(Game *g, Player *p, double dt);
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
