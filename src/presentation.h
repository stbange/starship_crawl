#ifndef SC_PRESENTATION_H
#define SC_PRESENTATION_H
#include "identity.h"
#define SC_PRESENTATION_EVENTS 32
/* A zero recipient is a party broadcast; other events are addressed to a character. */
typedef struct {
    uint64_t sequence;
    PlayerId recipient;
    char text[128];
} PresentationEvent;
typedef struct {
    uint64_t sequence;
    PresentationEvent events[SC_PRESENTATION_EVENTS];
} Presentation;
struct Game;
void game_player_message(struct Game *game, PlayerId player, const char *text);
bool game_event_next(const struct Game *game, PlayerId player, uint64_t *cursor,
                     PresentationEvent *event);
void game_views_reset(struct Game *game);
#endif
