#ifndef SC_STORY_H
#define SC_STORY_H
#include "world.h"
#define SC_STORY_SCENES 16
typedef enum { STORY_RESCUE, STORY_KNOWLEDGE, STORY_SALVAGE } StoryMotivation;
typedef enum {
    STORY_BRIEFING,
    STORY_TRACKS,
    STORY_CONDUIT,
    STORY_FROST,
    STORY_RESEARCH,
    STORY_RATIONS,
    STORY_REFUGE,
    STORY_ARCHIVE,
    STORY_LEDGER,
    STORY_REACTOR,
    STORY_EVAC,
    STORY_MIRROR,
    STORY_SURVIVORS,
    STORY_MEMORY,
    STORY_DRIVE,
    STORY_EXTRACTION
} StoryScene;
typedef struct {
    uint32_t version, assigned, seen, done, tracked, choice;
    Cell sites[SC_STORY_SCENES];
} Story;
typedef struct {
    const char *title;
    RoomType architecture;
    District sector;
} StoryTemplate;
extern const StoryTemplate sc_story_templates[SC_STORY_SCENES];
void story_init(Story *s);
StoryMotivation story_motivation(uint32_t seed);
const char *story_ship(uint32_t seed);
const char *story_commission(uint32_t seed);
bool story_valid(const Story *s, uint32_t seed, uint32_t generator, uint32_t mission);
#endif
