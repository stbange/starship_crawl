#ifndef SC_POLITICS_H
#define SC_POLITICS_H
#include "world.h"
#define SC_COUNTRIES 3
#define SC_DIRECTOR_LOG 8
#define SC_POLITICS_SAVE_BYTES ((2 + SC_COUNTRIES * 5 + 9 + 3 + 2 * 4 + SC_DIRECTOR_LOG * 2) * 4u)
typedef enum { COURT_NONE, COURT_AUDIENCE, COURT_CONDUIT, COURT_FINALE } CourtRole;
typedef enum { DIVIDED_CROWN, MOLTING_COURT, ECHO_PARLIAMENT } Government;
typedef enum { REL_PEACE, REL_WAR, REL_TRIBUTE, REL_SYMBIOSIS } Relation;
typedef struct {
    uint32_t met, pledge, stage, claimed, offended;
} CountryState;
typedef struct {
    uint32_t version, tracked;
    CountryState countries[SC_COUNTRIES];
    /* Per-view reading/decision state is transient. */
    unsigned reading[2], audience[2];
} Politics;
typedef enum {
    EVENT_NONE,
    EVENT_SUPPLIES,
    EVENT_COURIER,
    EVENT_REVEAL,
    EVENT_CONFLICT,
    EVENTS
} DirectorEvent;
typedef enum {
    REASON_RECOVERY,
    REASON_DIPLOMACY,
    REASON_VARIETY,
    REASON_CHALLENGE,
    DIRECTOR_REASONS
} DirectorReason;
typedef struct {
    uint32_t version, sequence, cooldown, budget, recharge, quiet, kind, remaining, claimed;
    Cell site, last_room[2];
    uint32_t seen[2], history[SC_DIRECTOR_LOG], reasons[SC_DIRECTOR_LOG];
} Director;
struct Game;
Government politics_government(uint32_t seed, unsigned country);
const char *politics_name(uint32_t seed, unsigned country);
const char *politics_government_name(Government government);
const char *politics_relation_name(Relation relation);
Color politics_color(uint32_t seed, unsigned country);
unsigned politics_country(uint32_t seed, Cell cell);
void politics_plan(uint32_t seed, RegionPlan *plan);
Cell politics_region(uint32_t seed, unsigned country, unsigned role);
Cell politics_site(uint32_t seed, unsigned country, CourtRole role);
void politics_populate(World *world, Room *room);
void politics_init(struct Game *game);
unsigned politics_owner(const struct Game *game, Cell cell);
Relation politics_relation(const struct Game *game, unsigned a, unsigned b);
int politics_reputation(const struct Game *game, unsigned country, int player);
bool politics_threat(const struct Game *game, const Enemy *enemy);
bool politics_valid(const Politics *politics, unsigned generator);
bool politics_world_valid(const Politics *politics, const Director *director, const World *world);
bool politics_interact(struct Game *game, int player);
bool politics_choose(struct Game *game, int player, unsigned choice);
void politics_journal(struct Game *game, int player);
void politics_read(struct Game *game, int player);
void politics_track(struct Game *game, int player);
bool politics_objective(const struct Game *game, Cell *cell);
const char *politics_goal(const struct Game *game);
void politics_line(const struct Game *game, int player, int line, char *text, size_t size);
bool politics_enemy_step(struct Game *game, Room *room, Enemy *enemy, double dt);
void politics_attack(struct Game *game, const Enemy *enemy, int player);
void politics_step(struct Game *game, double dt);
void director_init(Director *director, unsigned generator);
bool director_valid(const Director *director, unsigned generator);
void director_step(struct Game *game, double dt);
bool director_interact(struct Game *game, int player);
const char *director_name(unsigned kind);
const char *director_reason(unsigned reason);
Vec3 director_point(const Director *director);
#endif
