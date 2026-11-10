#ifndef SC_RHYTHM_H
#define SC_RHYTHM_H
#include "world.h"
#define SC_SEQUENCES 2
typedef enum {
    SEQUENCE_NONE,
    SEQUENCE_HUNTER,
    SEQUENCE_SIEGE,
    SEQUENCE_MACHINE,
    SEQUENCE_DISCOVERY
} SequenceKind;
typedef enum {
    BEAT_NONE,
    BEAT_TRACE,
    BEAT_APPROACH,
    BEAT_PRESSURE,
    BEAT_FIGHT,
    BEAT_QUIET
} SequenceBeat;
typedef struct {
    uint32_t identity, player, member, force;
    Cell site;
    SequenceKind kind;
    SequenceBeat beat;
    float clock, away;
    bool reserved, witnessed;
} HiddenSequence;
typedef struct {
    HiddenSequence sequences[SC_SEQUENCES];
    uint32_t next_id, completed, cancelled, cursor;
    float quiet;
} Rhythm;
struct Game;
unsigned rhythm_reserved(const Rhythm *r);
bool rhythm_valid(const Rhythm *r, unsigned budget);
bool rhythm_start(struct Game *g, SequenceKind kind, int player, Cell site, unsigned member,
                  uint32_t force);
void rhythm_step(struct Game *g, double dt);
void rhythm_select(struct Game *g, int player);
bool rhythm_blocks(const struct Game *g, Cell site, bool pressure);
Vec3 rhythm_trace(Cell site, unsigned kind);
float rhythm_intensity(const struct Game *g, int player);
#endif
