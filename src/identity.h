#ifndef SC_IDENTITY_H
#define SC_IDENTITY_H
#include <stdint.h>
#include <stdbool.h>
/* Local screens and historical checkpoint profiles are distinct contracts. */
#define SC_LOCAL_VIEWS 2
#define SC_LEGACY_PLAYERS 2
#ifndef SC_PARTY_CAPACITY
#define SC_PARTY_CAPACITY 2
#endif
#define SC_PLAYERS SC_PARTY_CAPACITY
#define SC_PLAYER_MASK ((1u << SC_PLAYERS) - 1u)
_Static_assert(SC_PLAYERS >= 2 && SC_PLAYERS <= 4, "Supported party capacity is two through four");
typedef struct {
    uint32_t epoch, slot, generation;
} PlayerId;
typedef struct {
    uint32_t value;
} ConnectionId;
typedef struct {
    PlayerId players[SC_LOCAL_VIEWS];
    unsigned count;
} ViewBindings;
struct Game;
PlayerId game_player_id(const struct Game *g, int slot);
int game_player_slot(const struct Game *g, PlayerId id);
bool game_bind_view(const struct Game *g, ViewBindings *views, unsigned view, PlayerId player);
int game_view_slot(const struct Game *g, const ViewBindings *views, unsigned view);
bool game_party_resize(struct Game *g, unsigned count);
void game_identity_reset(struct Game *g);
#endif
