#include "snapshot_internal.h"
#include <float.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(float) == 4 && sizeof(double) == 8 && FLT_RADIX == 2 && sizeof(int) == 4 &&
                   FLT_MANT_DIG == 24 && DBL_MANT_DIG == 53,
               "Snapshots require IEEE binary32/binary64 and 32-bit int");
typedef struct {
    unsigned char *data;
    size_t size, cursor;
    uint32_t hash;
    bool read, ok;
} SnapshotStream;
static uint64_t word(SnapshotStream *s, uint64_t value, unsigned width) {
    if (!s->ok || width > s->size - s->cursor) {
        s->ok = false;
        return 0;
    }
    uint64_t result = 0;
    for (unsigned i = 0; i < width; i++) {
        unsigned char byte = s->read ? s->data[s->cursor] : (unsigned char)(value >> (i * 8));
        if (!s->read && s->data)
            s->data[s->cursor] = byte;
        result |= (uint64_t)byte << (i * 8);
        s->cursor++;
        s->hash = (s->hash ^ byte) * UINT32_C(16777619);
    }
    return result;
}
static void object(SnapshotStream *s, void *base, const SnapshotSchema *schema) {
    for (size_t f = 0; s->ok && f < schema->count; f++) {
        const SnapshotField *field = &schema->fields[f];
        for (size_t i = 0; s->ok && i < field->count; i++) {
            unsigned char *at = (unsigned char *)base + field->offset + i * field->stride;
            uint64_t bits = 0;
            double value = 0;
            unsigned width = 4;
            switch (field->kind) {
            case SW_STRUCT:
                object(s, at, field->child);
                continue;
            case SW_U8:
                bits = *(uint8_t *)at;
                width = 1;
                break;
            case SW_U16:
                bits = *(uint16_t *)at;
                width = 2;
                break;
            case SW_U32:
                bits = *(uint32_t *)at;
                break;
            case SW_U64:
                bits = *(uint64_t *)at;
                width = 8;
                break;
            case SW_SIZE:
                bits = *(size_t *)at == SIZE_MAX ? UINT64_MAX : *(size_t *)at;
                width = 8;
                break;
            case SW_BOOL:
                bits = *(bool *)at;
                width = 1;
                break;
            case SW_I32: {
                int32_t n;
                memcpy(&n, at, 4);
                bits = (uint32_t)n;
                break;
            }
            case SW_FLOAT: {
                uint32_t n;
                memcpy(&n, at, 4);
                bits = n;
                break;
            }
            case SW_DOUBLE:
                memcpy(&bits, at, 8);
                width = 8;
                break;
            }
            bits = word(s, bits, width);
            value = (double)bits;
            switch (field->kind) {
            case SW_U8:
                if (s->read)
                    *(uint8_t *)at = (uint8_t)bits;
                break;
            case SW_U16:
                if (s->read)
                    *(uint16_t *)at = (uint16_t)bits;
                break;
            case SW_U32:
                if (s->read)
                    *(uint32_t *)at = (uint32_t)bits;
                break;
            case SW_U64:
                if (s->read)
                    *(uint64_t *)at = bits;
                break;
            case SW_SIZE:
                if (bits != UINT64_MAX && bits > SIZE_MAX)
                    s->ok = false;
                if (s->read)
                    *(size_t *)at = bits == UINT64_MAX ? SIZE_MAX : (size_t)bits;
                break;
            case SW_BOOL:
                if (s->read)
                    *(bool *)at = bits != 0;
                break;
            case SW_I32: {
                int32_t n = (int32_t)(uint32_t)bits;
                value = n;
                if (s->read)
                    memcpy(at, &n, 4);
                break;
            }
            case SW_FLOAT: {
                uint32_t n = (uint32_t)bits;
                float number;
                memcpy(&number, &n, 4);
                value = number;
                if (s->read)
                    memcpy(at, &number, 4);
                break;
            }
            case SW_DOUBLE:
                memcpy(&value, &bits, 8);
                if (s->read)
                    memcpy(at, &value, 8);
                break;
            case SW_STRUCT:
                break;
            }
            if (!isfinite(value) || ((field->minimum != 0 || field->maximum != 0) &&
                                     (value < field->minimum || value > field->maximum)))
                s->ok = false;
        }
    }
}
static void *array(SnapshotStream *s, void *data, size_t count, size_t stride,
                   const SnapshotSchema *schema) {
    if (!s->ok || !count)
        return data;
    if (s->read) {
        data = calloc(count, stride);
        if (!data) {
            s->ok = false;
            return NULL;
        }
    }
    for (size_t i = 0; s->ok && i < count; i++)
        object(s, (unsigned char *)data + i * stride, schema);
    return data;
}
static void payload(SnapshotStream *s, Game *g) {
    uint32_t magic = (uint32_t)word(s, UINT32_C(0x53434c56), 4);
    uint32_t version = (uint32_t)word(s, SC_SNAPSHOT_VERSION, 4);
    uint32_t party = (uint32_t)word(s, SC_PLAYERS, 4);
    uint32_t schema = (uint32_t)word(s, sc_snapshot_schema_id, 4);
    if (schema != sc_snapshot_schema_id || magic != UINT32_C(0x53434c56) ||
        version != SC_SNAPSHOT_VERSION || party != SC_PLAYERS) {
        s->ok = false;
        return;
    }
    object(s, g, &sc_snapshot_game);
    void *records = array(s, g->world.records, g->world.record_count, sizeof(RoomRecord),
                          &sc_snapshot_room_record);
    if (s->read)
        g->world.records = records;
    records = array(s, g->matrix.records, g->matrix.count, sizeof(MatrixRecord),
                    &sc_snapshot_matrix_record);
    if (s->read)
        g->matrix.records = records;
    records = array(s, g->factions.regions, g->factions.count, sizeof(FactionRegion),
                    &sc_snapshot_faction_region);
    if (s->read)
        g->factions.regions = records;
    for (int i = 0; s->ok && i < SC_MAX_CHUNKS; i++) {
        void *boxes = array(s, g->world.chunks[i].boxes, g->world.chunks[i].box_count, sizeof(Box),
                            &sc_snapshot_box);
        if (s->read)
            g->world.chunks[i].boxes = boxes;
    }
    uint32_t hash = s->hash;
    if ((uint32_t)word(s, hash, 4) != hash)
        s->ok = false;
}
bool game_snapshot_encode(const Game *g, unsigned char **data, size_t *size) {
    *data = NULL;
    *size = 0;
    if (!sc_snapshot_valid(g))
        return false;
    SnapshotStream measure = {NULL, SC_SNAPSHOT_LIMIT, 0, UINT32_C(2166136261), false, true};
    payload(&measure, (Game *)g);
    if (!measure.ok)
        return false;
    unsigned char *bytes = malloc(measure.cursor);
    if (!bytes)
        return false;
    SnapshotStream output = {bytes, measure.cursor, 0, UINT32_C(2166136261), false, true};
    payload(&output, (Game *)g);
    *data = bytes;
    *size = output.cursor;
    return true;
}
bool game_snapshot_decode(Game *g, const unsigned char *data, size_t size) {
    if (!data || size < 20 || size > SC_SNAPSHOT_LIMIT)
        return false;
    Game *replacement = calloc(1, sizeof(*replacement));
    if (!replacement)
        return false;
    SnapshotStream input = {(unsigned char *)data, size, 0, UINT32_C(2166136261), true, true};
    payload(&input, replacement);
    bool ok = input.ok && input.cursor == size && sc_snapshot_valid(replacement);
    if (ok) {
        replacement->world.record_cap = replacement->world.record_count;
        replacement->matrix.capacity = replacement->matrix.count;
        replacement->factions.capacity = replacement->factions.count;
        ok = world_index_records(&replacement->world) && matrix_index(&replacement->matrix);
    }
    if (ok) {
        memcpy(replacement->views, g->views, sizeof(g->views));
        game_views_reset(replacement);
        replacement->epoch = g->epoch;
        game_identity_reset(replacement);
        for (int p = 0; p < SC_PLAYERS; p++) {
            if (replacement->builds[p].focus_target.force ||
                replacement->builds[p].focus_target.residency)
                replacement->builds[p].focus_target.epoch = replacement->epoch;
            if (replacement->builds[p].mark_target.force ||
                replacement->builds[p].mark_target.residency)
                replacement->builds[p].mark_target.epoch = replacement->epoch;
        }
        for (int i = 0; i < SC_MAX_PROJECTILES; i++)
            for (int j = 0; j < replacement->projectiles[i].hit_count; j++)
                replacement->projectiles[i].hit_actors[j].epoch = replacement->epoch;
        replacement->dirty = true;
        game_destroy(g);
        *g = *replacement;
    } else
        game_destroy(replacement);
    free(replacement);
    return ok;
}
