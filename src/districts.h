#ifndef SC_DISTRICTS_H
#define SC_DISTRICTS_H
#include "interior.h"
enum {
    DISTRICT_SEEN = 1,
    DISTRICT_SHORTCUT = 2,
    DISTRICT_CONDUCTOR = 4,
    DISTRICT_RESCUED = 8,
    DISTRICT_SALVAGED = 16,
    DISTRICT_LOST = 32
};
#define SC_DISTRICT_STATE_MASK 63u
enum { DISTRICT_SOURCE_CONTROL = 16, DISTRICT_RETURN_CONTROL = 32 };
struct Game;
void district_plan(uint32_t seed, RegionPlan *plan);
const char *district_name(unsigned family);
const char *district_condition_name(unsigned condition);
DistrictContract district_contract(uint32_t seed, uint32_t generator, Cell cell);
void district_build(Builder *builder, const Room *room);
bool district_powered(const World *world, const DistrictMark *mark);
bool district_inert(const struct Game *game, const Room *room, const Enemy *enemy);
bool district_interact(struct Game *game, int player);
void district_step(struct Game *game, double dt);
void district_shot(struct Game *game, int player, Vec3 impact);
void district_victory(struct Game *game, Cell owner);
Vec3 district_control(Cell owner, unsigned role);
bool district_wet(const Room *room, Vec3 at);
#endif
