#ifndef SC_FRONTIER_H
#define SC_FRONTIER_H
#include "interior.h"
typedef enum {
    CH_LINK,
    CH_AERIE,
    CH_COLONY,
    CH_SIEGE,
    CH_CIRCUIT,
    CH_CIPHER,
    CHALLENGES
} Challenge;
typedef enum { RELIC_HEAT, RELIC_COLD, RELIC_ORDER, RELIC_CHAOS, RELIC_ALIEN, RELICS } Relic;
typedef struct {
    uint32_t ranks[SC_PLAYERS][RELICS];
} FrontierProgress;
struct Game;
struct Player;
void frontier_plan(uint32_t seed, RoomSpec *room);
void frontier_build(Builder *builder, const Room *room);
void frontier_populate(World *world, Room *room);
const char *frontier_name(unsigned kind);
const char *frontier_hint(unsigned kind);
const char *frontier_relic(unsigned relic);
const char *frontier_effect(unsigned relic);
const char *frontier_risk(unsigned contract);
const char *frontier_reveal(unsigned reveal);
Vec3 frontier_focus(const Room *room);
Vec3 frontier_pylon(const Room *room, unsigned index);
Vec3 frontier_hazard(const Room *room);
bool frontier_hazard_live(const Room *room);
void frontier_step(struct Game *game, Room *room, double dt);
bool frontier_ward(const struct Game *game, const Room *room, int player);
float frontier_damage(struct Game *game, Room *room, int index, int player, float damage);
void frontier_claim(struct Game *game, int player, const Room *room);
void frontier_offer(const struct Game *game, int player, const Room *room, char *text, size_t size);
void frontier_shot(const struct Game *game, int player, WeaponDef *shot);
void frontier_player_step(struct Game *game, int player, double dt);
bool frontier_valid(const FrontierProgress *progress, unsigned generator);
bool alien_special_step(struct Game *game, Room *room, int index, int target, bool attack,
                        double dt);
float alien_damage(Enemy *enemy, float damage);
void alien_fragments(const World *world, Enemy *enemy);
bool alien_observed(const struct Game *game, const Enemy *enemy);
const char *alien_hint(unsigned behavior);
void alien_surface_step(struct Game *game, Room *room, Enemy *enemy, Vec3 direction, double dt);
#endif
