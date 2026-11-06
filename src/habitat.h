#ifndef SC_HABITAT_H
#define SC_HABITAT_H
#include "interior.h"
typedef struct {
    Color tint, fog;
    float density;
    const char *name;
} AtmosphereDef;
extern const AtmosphereDef sc_atmospheres[ATMOSPHERES];
extern const char *sc_habitat_forms[9];
extern const char *sc_ecologies[ECOLOGIES];
bool habitat_owns(Cell owner, int span_x, int span_z, uint16_t mask, Cell cell);
void habitat_reserve(uint32_t seed, Cell region, int *x, int *z, RoomSpec *macro);
void habitat_plan(uint32_t seed, RoomSpec *room);
void habitat_build(Builder *builder, const Room *room);
void habitat_floor(Builder *builder, const Room *room);
unsigned habitat_enemy(const Room *room, unsigned difficulty, uint32_t hash);
bool habitat_spawn(const World *world, const Room *room, Vec3 position);
void habitat_account(const RoomSpec *room, char *text, size_t size);
Color habitat_enemy_color(const Enemy *enemy);
#endif
