#include "game.h"
#include <stdio.h>
#include <string.h>
void game_player_message(Game *g, PlayerId player, const char *text) {
    int slot = game_player_slot(g, player);
    if (player.epoch && slot < 0)
        return;
    snprintf(g->message, sizeof(g->message), "%s", text);
    g->message_time = 4;
    uint64_t sequence = ++g->presentation.sequence;
    PresentationEvent *event = &g->presentation.events[(sequence - 1) % SC_PRESENTATION_EVENTS];
    *event = (PresentationEvent){.sequence = sequence, .recipient = player};
    snprintf(event->text, sizeof(event->text), "%s", text);
    for (int i = 0; i < g->player_count; i++)
        if (!player.epoch || slot == i) {
            snprintf(g->views[i].message, sizeof(g->views[i].message), "%s", text);
            g->views[i].message_time = 4;
        }
}
bool game_event_next(const Game *g, PlayerId player, uint64_t *cursor, PresentationEvent *event) {
    if (game_player_slot(g, player) < 0)
        return false;
    uint64_t oldest = g->presentation.sequence > SC_PRESENTATION_EVENTS
                          ? g->presentation.sequence - SC_PRESENTATION_EVENTS
                          : 0;
    if (*cursor < oldest)
        *cursor = oldest;
    while (*cursor < g->presentation.sequence) {
        const PresentationEvent *next =
            &g->presentation.events[(*cursor)++ % SC_PRESENTATION_EVENTS];
        if (!next->recipient.epoch ||
            (next->recipient.epoch == player.epoch && next->recipient.slot == player.slot &&
             next->recipient.generation == player.generation)) {
            *event = *next;
            return true;
        }
    }
    return false;
}
void game_views_reset(Game *g) {
    for (int i = 0; i < SC_PLAYERS; i++) {
        unsigned mode = g->views[i].guide_mode, device = g->views[i].guide_device;
        g->views[i] = (PlayerView){.guide_mode = mode, .guide_device = device};
    }
    memset(&g->presentation, 0, sizeof(g->presentation));
}
