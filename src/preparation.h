#ifndef SC_PREPARATION_H
#define SC_PREPARATION_H
#include "world.h"
#define SC_KEEPSAKES 6
#define PREP_MEALS_SCENE (SC_STORY_SCENES + SC_RECURRING)
#define PREP_PROJECTS_SCENE (PREP_MEALS_SCENE + 1)
#define PREP_RECORD_SCENE (PREP_MEALS_SCENE + 2)
#define HURT_ELECTRIC (1u << 29)
enum { PROJECT_REFIT = 1, PROJECT_FLARES = 2, PROJECT_RESEARCH = 4 };
typedef enum { MEAL_NONE, MEAL_INSULATED, MEAL_WARM, MEAL_CHARGED } MealKind;
typedef struct {
    MealKind meal;
    float remaining;
    bool flare_charge;
} PersonalPreparation;
typedef struct {
    Cell anchor;
    uint32_t identity, family, condition, facts;
} Keepsake;
typedef struct {
    uint32_t projects, observed, researched, keepsake_count;
    PersonalPreparation personal[SC_PLAYERS];
    Keepsake keepsakes[SC_KEEPSAKES];
} Home;
struct Game;
struct Player;
bool preparation_valid(const Home *h, const World *w, const struct Player *players);
unsigned preparation_near(const struct Game *g, int player);
Vec3 preparation_station(unsigned scene);
bool preparation_interact(struct Game *g, int player);
bool preparation_choose(struct Game *g, int player, unsigned choice);
bool preparation_ready(const struct Game *g, int player);
const char *preparation_option(const struct Game *g, int player, unsigned choice);
void preparation_entry(const struct Game *g, int player, unsigned scene, char *text, size_t size);
void preparation_step(struct Game *g, double dt);
void preparation_observe(struct Game *g, const Room *room);
void preparation_flare(struct Game *g, int player, unsigned slot);
float preparation_damage(const struct Game *g, int player, float damage, uint32_t flags);
bool preparation_next_record(struct Game *g, int player, bool first);
const char *preparation_meal_name(unsigned meal);
#endif
