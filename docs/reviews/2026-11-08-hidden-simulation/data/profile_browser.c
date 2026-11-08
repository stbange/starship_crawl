/* Review-only linker wrappers. The ordinary packaged game's objects/core are
   unchanged. This object is never linked into the published game. */
#include "game.h"
#include <emscripten.h>
#include <stdlib.h>
static double times[6][32768];
static unsigned counts[6];
static void record(unsigned part, double elapsed) {
    if (counts[part] < 32768)
        times[part][counts[part]++] = elapsed * 1000;
}
static int compare(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
void __real_factions_step(Game *, double);
void __real_factions_forces_step(Game *, double);
void __real_hidden_director_step(Game *, double);
void __real_factions_tactics(Game *, double);
void __real_game_step(Game *, const Actions *, double);
void __wrap_factions_step(Game *g, double dt) {
    unsigned tick = g->factions.tick;
    double start = emscripten_get_now();
    __real_factions_step(g, dt);
    double elapsed = emscripten_get_now() - start;
    record(0, elapsed);
    if (g->factions.tick != tick)
        record(3, elapsed);
}
void __wrap_factions_forces_step(Game *g, double dt) {
    double start = emscripten_get_now();
    __real_factions_forces_step(g, dt);
    record(1, emscripten_get_now() - start);
}
void __wrap_hidden_director_step(Game *g, double dt) {
    double start = emscripten_get_now();
    __real_hidden_director_step(g, dt);
    record(2, emscripten_get_now() - start);
}
void __wrap_factions_tactics(Game *g, double dt) {
    double start = emscripten_get_now();
    __real_factions_tactics(g, dt);
    record(4, emscripten_get_now() - start);
}
void __wrap_game_step(Game *g, const Actions *actions, double dt) {
    double start = emscripten_get_now();
    __real_game_step(g, actions, dt);
    record(5, emscripten_get_now() - start);
    if (!EM_ASM_INT({ return !!window.scReviewRequest; }))
        return;
    EM_ASM({ window.scReviewRequest = false; window.scReviewProfile = []; });
    for (unsigned p = 0; p < 6; p++) {
        unsigned n = counts[p];
        if (!n)
            continue;
        qsort(times[p], n, sizeof(double), compare);
        EM_ASM({window.scReviewProfile.push({part:$0, samples:$1, p50_us:$2,
                 p95_us:$3, p99_us:$4, max_us:$5});},
               p, n, times[p][n / 2], times[p][n * 95 / 100], times[p][n * 99 / 100], times[p][n - 1]);
        counts[p] = 0;
    }
    EM_ASM({window.scReviewMemory = ({wasm_bytes:HEAPU8.length, game_bytes:$0,
            faction_bytes:$1, region_bytes:$2, regions:$3, region_capacity:$4});},
           sizeof(Game), sizeof(Factions), sizeof(FactionRegion), g->factions.count,
           g->factions.capacity);
}
