#ifndef SC_ENCOUNTERS_H
#define SC_ENCOUNTERS_H
#include "campaign.h"
typedef struct {
    CampaignReward reward;
    Cell site;
    float time;
    unsigned shots;
    bool complete;
} RewardDrill;
void encounter_plan(uint32_t seed, RoomSpec *room);
void encounter_populate(World *world, Room *room);
const char *encounter_name(const Room *room);
Vec3 encounter_console(const Room *room);
Vec3 local_console(const Room *room);
bool local_available(const World *world, const Room *room);
const char *local_action(unsigned tale);
CampaignReward encounter_reward(const struct Game *game, int player, const Room *room);
void encounter_reward_name(const struct Game *game, int player, const Room *room, char *text,
                           size_t size);
bool game_encounter_interact(struct Game *game, int player);
void game_encounters_step(struct Game *game, double dt);
bool encounter_passive(const Room *room, int enemy);
bool encounter_attack(const Room *room, int enemy);
bool encounter_linked(const World *world, const Room *room, int enemy);
float encounter_damage(struct Game *game, Room *room, int enemy, float damage, uint32_t flags);
void game_reward_drill(struct Game *game, int player, CampaignReward reward, Cell site);
const char *reward_drill_hint(CampaignReward reward);
bool game_alien_step(struct Game *game, Room *room, int index, int target, bool attack, double dt);
void game_enemy_shot(struct Game *game, const Room *room, Enemy *enemy);
#endif
