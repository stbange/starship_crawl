#ifndef SC_CHRONICLE_H
#define SC_CHRONICLE_H
#include "world.h"
#define SC_RECURRING 3
#define SC_CONTINUING_CAMPAIGNS 6
enum { PERSON_SEEN = 1, PERSON_HELPED = 2, PERSON_STRIPPED = 4, PERSON_TRAVEL_KNOWN = 8 };
typedef enum {
    PERSON_WAITING,
    PERSON_TRAVELLING,
    PERSON_SETTLED,
    PERSON_LOST,
    PERSON_LEFT
} PersonFate;
typedef struct {
    uint32_t identity, role, knowledge, meetings, phase;
    Cell home, site, previous, destination;
    PersonFate fate;
    float travel;
} RecurringPerson;
typedef enum {
    CHAIN_NONE,
    CHAIN_SUPPLY,
    CHAIN_SIEGE,
    CHAIN_FACTORY,
    CHAIN_REINFORCE,
    CHAIN_OCCUPIED,
    CHAIN_RETREATED
} ContinuingStage;
typedef struct {
    uint32_t identity, country, pattern, force;
    Cell source, staging, goal;
    ContinuingStage stage;
    float clock;
} ContinuingCampaign;
typedef struct {
    uint32_t version, count, next_id;
    RecurringPerson people[SC_RECURRING];
    ContinuingCampaign campaigns[SC_CONTINUING_CAMPAIGNS];
} Chronicle;
struct Game;
struct FactionForce;
void chronicle_init(struct Game *g);
bool chronicle_valid(const Chronicle *c, uint32_t seed, unsigned generator);
void chronicle_step(struct Game *g, double dt);
bool chronicle_order(struct Game *g, unsigned country, unsigned order, Cell source, Cell goal);
bool chronicle_arrival(struct Game *g, uint32_t force);
void chronicle_campaigns(struct Game *g, double dt);
const char *chronicle_name(const RecurringPerson *p);
const char *chronicle_role(unsigned role);
Vec3 chronicle_position(const RecurringPerson *p);
int chronicle_near(const struct Game *g, int player);
bool chronicle_interact(struct Game *g, int player);
bool chronicle_choose(struct Game *g, int player, unsigned choice);
bool chronicle_ready(const struct Game *g, int player);
const char *chronicle_option(const struct Game *g, int player, unsigned choice);
void chronicle_entry(const struct Game *g, unsigned index, char *text, size_t size);
#endif
