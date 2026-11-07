#ifndef SC_CAMPAIGN_H
#define SC_CAMPAIGN_H
#include "world.h"
#define SC_PHASES 4
#define SC_GUARDIANS 9
#define SC_SPECIALIZATIONS 4
#define SC_EMPTY_POWER 4u
typedef enum { MOD_NONE, MOD_STORM, MOD_SIEGE, MOD_HUNTER, MOD_SABOTEUR } Specialization;
typedef enum { REWARD_GUN, REWARD_POWER, REWARD_RIG, REWARD_MODULE } RewardKind;
typedef struct {
    unsigned kind, item;
} CampaignReward;
typedef struct {
    int heat, order;
    unsigned alien;
} ShipTraits;
typedef struct {
    uint32_t powers, rigs, modules, claimed;
    uint32_t guns[2], power_slots[2], rig, module;
} CampaignProfile;
typedef struct {
    uint32_t participants;
    bool active;
    float timer, opening;
    unsigned attack;
    Vec3 aim;
} GuardianAttempt;
typedef struct {
    uint32_t version, defeated, eligible[SC_GUARDIANS], tracked;
    CampaignProfile profiles[2];
    GuardianAttempt attempts[SC_GUARDIANS];
    /* Journal selection is presentation state and is reconstructed after loading. */
    unsigned slot[2], choice[2];
} Campaign;
struct Game;
struct Player;
unsigned campaign_layer(Cell cell);
ShipTraits campaign_traits(uint32_t seed, Cell cell);
Cell campaign_site(unsigned guardian);
int campaign_guardian(Cell owner);
bool campaign_arena_region(Cell region);
unsigned campaign_family(uint32_t seed, unsigned guardian);
const char *campaign_guardian_name(uint32_t seed, unsigned guardian);
const char *campaign_phase_name(unsigned phase);
const char *campaign_module_name(unsigned module);
void campaign_plan(uint32_t seed, RegionPlan *plan);
void campaign_populate(World *world, Room *room);
float campaign_guardian_health(unsigned guardian, unsigned participants);
unsigned campaign_phase(const Campaign *campaign);
void campaign_init(struct Game *game);
void campaign_sync(struct Game *game);
bool campaign_weapon(const struct Game *game, int player, unsigned weapon);
bool campaign_power(const struct Game *game, int player, unsigned power);
bool campaign_rig(const struct Game *game, int player, unsigned rig);
bool campaign_safe_refit(const struct Game *game, int player);
bool campaign_refit(struct Game *game, int player, unsigned slot, int delta);
void campaign_track(struct Game *game);
bool campaign_objective(const struct Game *game, Cell *cell);
bool campaign_engaged(const struct Game *game, int player);
double campaign_gate_ray(const World *world, Vec3 origin, Vec3 direction, double limit);
bool campaign_interact(struct Game *game, int player);
void campaign_journal(struct Game *game, int player);
CampaignReward campaign_reward(uint32_t seed, unsigned guardian, unsigned choice);
void campaign_reward_name(const struct Game *game, int player, unsigned guardian, unsigned choice,
                          char *text, size_t size);
bool campaign_choose(struct Game *game, int player, unsigned choice);
bool campaign_owns(const struct Game *game, int player, CampaignReward reward);
void campaign_grant(struct Game *game, int player, CampaignReward reward);
void campaign_defeat(struct Game *game, unsigned guardian);
void campaign_step(struct Game *game, double dt);
bool campaign_enemy_step(struct Game *game, Room *room, int index, double dt);
float campaign_damage(struct Game *game, const Room *room, int index, int owner, float damage);
float campaign_attack_bonus(const struct Game *game, int player);
void campaign_resume(struct Game *game);
bool campaign_valid(const Campaign *campaign, const struct Player *players, const World *world);
#endif
