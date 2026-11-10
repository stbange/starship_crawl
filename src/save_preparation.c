#include "save_internal.h"
void sc_save_preparation(Stream *s, Home *h, bool read) {
    h->projects = sc_save_number(s, h->projects, read);
    h->observed = sc_save_number(s, h->observed, read);
    h->researched = sc_save_number(s, h->researched, read);
    h->keepsake_count = sc_save_number(s, h->keepsake_count, read);
    for (unsigned i = 0; i < SC_LEGACY_PLAYERS; i++) {
        PersonalPreparation *p = &h->personal[i];
        p->meal = (MealKind)sc_save_number(s, (uint32_t)p->meal, read);
        p->remaining = (float)sc_save_real(s, p->remaining, read);
        p->flare_charge = sc_save_matrix_bool(s, p->flare_charge, read);
    }
    for (unsigned i = 0; i < SC_KEEPSAKES; i++) {
        Keepsake *k = &h->keepsakes[i];
        k->anchor = sc_save_matrix_cell(s, k->anchor, read);
        k->identity = sc_save_number(s, k->identity, read);
        k->family = sc_save_number(s, k->family, read);
        k->condition = sc_save_number(s, k->condition, read);
        k->facts = sc_save_number(s, k->facts, read);
    }
}
