#ifndef SC_SNAPSHOT_H
#define SC_SNAPSHOT_H
#include "game.h"
#define SC_SNAPSHOT_VERSION 1u
#define SC_SNAPSHOT_LIMIT (32u * 1024u * 1024u)
#define SC_SNAPSHOT_RECORDS 65536u
/* Caller frees *data. Checkpoints and snapshots have distinct wire contracts.
   Decode is transactional and invalidates prior local player/actor handles. */
bool game_snapshot_encode(const Game *game, unsigned char **data, size_t *size);
bool game_snapshot_decode(Game *game, const unsigned char *data, size_t size);
#endif
