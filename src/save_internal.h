#ifndef SC_SAVE_INTERNAL_H
#define SC_SAVE_INTERNAL_H
/* Private checkpoint format 1-17 codecs. They never encode a live simulation. */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    FILE *file;
    uint32_t hash;
    bool ok;
} Stream;
uint32_t sc_save_number(Stream *s, uint32_t n, bool read);
double sc_save_real(Stream *s, double d, bool read);
Vec3 sc_save_vector(Stream *s, Vec3 v, bool read);
bool sc_save_valid_vector(Vec3 v);
Position sc_save_location(Stream *s, Position p, bool read, bool legacy);
Cell sc_save_matrix_cell(Stream *s, Cell c, bool read);
bool sc_save_matrix_bool(Stream *s, bool value, bool read);
void sc_save_profile(Stream *s, Player *p, bool read, unsigned version);
void sc_save_sanctuary(Stream *s, Shelter *shelter, Player *players, bool read);
void sc_save_powers(Stream *s, Player *players, bool read);
void sc_save_equipment(Stream *s, Player *players, Rig rigs[SC_LEGACY_PLAYERS][2], Flare *flares,
                       bool read);
void sc_save_narrative(Stream *s, Story *story, bool read, unsigned version);
void sc_save_aftermath(Stream *s, Story *story, Player *players, RoomRecord *records, size_t count,
                       bool read, unsigned version);
void sc_save_matrix_stream(Stream *s, MatrixState *m, bool read);
void sc_save_campaign_stream(Stream *s, Campaign *c, bool read);
void sc_save_politics_stream(Stream *s, Politics *p, Director *d, bool read);
void sc_save_factions_stream(Stream *s, Factions *f, bool read);
void sc_save_chronicle(Stream *s, Chronicle *c, bool read);
typedef struct {
    World *world;
    MatrixState *matrix;
    Factions *factions;
    const Player *players;
    Rig (*rigs)[2];
    const Flare *flares;
    Shelter shelter;
    Story story;
    Chronicle chronicle;
    Campaign campaign;
    FrontierProgress frontier;
    Politics politics;
    Director director;
    uint32_t mission, kills, loot, contracts, version;
} SavedExpedition;
void sc_save_restore(Game *g, const SavedExpedition *checkpoint);
#endif
