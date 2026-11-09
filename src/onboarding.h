#ifndef SC_ONBOARDING_H
#define SC_ONBOARDING_H
#include "campaign.h"
enum { GUIDE_GUIDED, GUIDE_MINIMAL, GUIDE_OFF };
enum { GUIDE_KEYBOARD, GUIDE_PAD, GUIDE_TOUCH, GUIDE_QUEST };
enum {
    OB_SHELTER = 1,
    OB_MATRIX = 2,
    OB_UPGRADES = 4,
    OB_BUILD = 8,
    OB_POWERS = 16,
    OB_RIGS = 32,
    OB_FULL_STATS = 64,
    OB_GUNS = 128,
    OB_PROGRAMS = 256
};
enum { INTRO_INTERFACE = 1, INTRO_GRID = 2 };
typedef enum {
    LESSON_NONE,
    LESSON_BOARD,
    LESSON_COMMISSION,
    LESSON_EXPLORE,
    LESSON_RECOVER,
    LESSON_WEAPON,
    LESSON_UPGRADE,
    LESSON_SHELTER,
    LESSON_INTERFACE,
    LESSON_LINK,
    LESSON_MATRIX,
    LESSON_POWER,
    LESSON_RIG,
    LESSON_COUNT
} Lesson;
struct Actions;
bool onboarding_active(const struct Game *g);
unsigned onboarding_capabilities(const struct Game *g, int player);
bool onboarding_action(const struct Game *g, int player, int action);
bool onboarding_danger(const struct Game *g, int player);
bool onboarding_opening(Cell cell);
void onboarding_plan(RegionPlan *plan);
void onboarding_populate(World *world, Room *room);
CampaignReward onboarding_reward(const struct Game *g, CampaignReward reward, unsigned tier);
bool onboarding_interface_site(const struct Game *g, int player);
bool onboarding_interface_ready(const struct Game *g, int player);
bool onboarding_interact(struct Game *g, int player);
bool onboarding_matrix_intro(const struct Game *g, int player);
void onboarding_step(struct Game *g, const struct Actions *actions, double dt);
void onboarding_dismiss(struct Game *g, int player);
const char *onboarding_mode(unsigned mode);
void onboarding_text(unsigned lesson, unsigned device, char *text, size_t size);
void onboarding_hint_text(const struct Game *g, int player, char *text, size_t size);
bool onboarding_valid(uint32_t intro, uint32_t lessons, unsigned generator, unsigned defeated);
#endif
