#include "save_internal.h"
void sc_save_rhythm(Stream *s, Rhythm *r, bool read) {
    r->next_id = sc_save_number(s, r->next_id, read);
    r->completed = sc_save_number(s, r->completed, read);
    r->cancelled = sc_save_number(s, r->cancelled, read);
    r->cursor = sc_save_number(s, r->cursor, read);
    r->quiet = (float)sc_save_real(s, r->quiet, read);
    for (unsigned i = 0; i < SC_SEQUENCES; i++) {
        HiddenSequence *q = &r->sequences[i];
        q->identity = sc_save_number(s, q->identity, read);
        q->player = sc_save_number(s, q->player, read);
        q->member = sc_save_number(s, q->member, read);
        q->force = sc_save_number(s, q->force, read);
        q->site = sc_save_matrix_cell(s, q->site, read);
        q->kind = (SequenceKind)sc_save_number(s, (uint32_t)q->kind, read);
        q->beat = (SequenceBeat)sc_save_number(s, (uint32_t)q->beat, read);
        q->clock = (float)sc_save_real(s, q->clock, read);
        q->away = (float)sc_save_real(s, q->away, read);
        q->reserved = sc_save_matrix_bool(s, q->reserved, read);
        q->witnessed = sc_save_matrix_bool(s, q->witnessed, read);
    }
}
