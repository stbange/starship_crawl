#include "save_internal.h"
void sc_save_chronicle(Stream *s, Chronicle *c, bool read) {
#define N(v) (v) = sc_save_number(s, (uint32_t)(v), read)
#define C(v) (v) = sc_save_matrix_cell(s, (v), read)
    N(c->version);
    N(c->count);
    N(c->next_id);
    for (unsigned i = 0; i < SC_RECURRING; i++) {
        RecurringPerson *p = &c->people[i];
        N(p->identity);
        N(p->role);
        N(p->knowledge);
        N(p->meetings);
        N(p->phase);
        N(p->fate);
        C(p->home);
        C(p->site);
        C(p->previous);
        C(p->destination);
        p->travel = (float)sc_save_real(s, p->travel, read);
    }
    for (unsigned i = 0; i < SC_CONTINUING_CAMPAIGNS; i++) {
        ContinuingCampaign *a = &c->campaigns[i];
        N(a->identity);
        N(a->country);
        N(a->pattern);
        N(a->force);
        N(a->stage);
        C(a->source);
        C(a->staging);
        C(a->goal);
        a->clock = (float)sc_save_real(s, a->clock, read);
    }
#undef N
#undef C
}
