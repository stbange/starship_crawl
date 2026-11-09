#ifndef SC_SNAPSHOT_INTERNAL_H
#define SC_SNAPSHOT_INTERNAL_H
#include "snapshot.h"
#include <stddef.h>
typedef enum {
    SW_U8,
    SW_U16,
    SW_U32,
    SW_U64,
    SW_I32,
    SW_SIZE,
    SW_BOOL,
    SW_FLOAT,
    SW_DOUBLE,
    SW_STRUCT
} SnapshotKind;
typedef struct SnapshotSchema SnapshotSchema;
typedef struct {
    size_t offset, count, stride;
    SnapshotKind kind;
    const SnapshotSchema *child;
    double minimum, maximum;
} SnapshotField;
struct SnapshotSchema {
    const SnapshotField *fields;
    size_t count;
};
extern const uint32_t sc_snapshot_schema_id;
extern const SnapshotSchema sc_snapshot_game, sc_snapshot_box, sc_snapshot_room_record,
    sc_snapshot_matrix_record, sc_snapshot_faction_region;
bool sc_snapshot_valid(const Game *game);
#endif
