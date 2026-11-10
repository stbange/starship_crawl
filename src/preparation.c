#include "game.h"
#include <stdio.h>
#include <string.h>

const char *preparation_meal_name(unsigned meal) {
    static const char *names[] = {"NONE", "INSULATED BROTH", "EMBER STEW", "CHARGED TEA"};
    return names[meal <= MEAL_CHARGED ? meal : 0];
}
Vec3 preparation_station(unsigned scene) {
    return scene == PREP_MEALS_SCENE ? v3(0, .02, -.5) : v3(6.5, .02, -2.8);
}
unsigned preparation_near(const Game *g, int player) {
    if (g->world.generator < 16 || player < 0 || player >= g->player_count ||
        !g->players[player].sheltered || g->players[player].health <= 0 ||
        g->matrix.divers[player].active)
        return 0;
    for (unsigned scene = PREP_MEALS_SCENE; scene <= PREP_PROJECTS_SCENE; scene++)
        if (vlen(vsub(g->players[player].shelter_pos, preparation_station(scene))) < 1.6)
            return scene;
    return 0;
}
static unsigned research(const Home *h) {
    for (unsigned c = 1; c <= 3; c++)
        if ((h->observed & (1u << c)) && !(h->researched & (1u << c)))
            return c;
    return 0;
}
bool preparation_interact(Game *g, int player) {
    unsigned scene = preparation_near(g, player);
    if (!scene)
        return false;
    g->views[player].journal = g->interactions[player].decision_open = true;
    g->views[player].journal_page = 1;
    g->views[player].story_reading = g->interactions[player].decision_scene = (uint8_t)scene;
    g->interactions[player].prep_quote = g->home.projects | (g->home.researched << 3);
    return true;
}
bool preparation_ready(const Game *g, int player) {
    unsigned scene = preparation_near(g, player);
    return scene && g->players[player].credits >= (scene == PREP_MEALS_SCENE ? 30u : 100u) &&
           (scene == PREP_MEALS_SCENE || research(&g->home));
}
const char *preparation_option(const Game *g, int player, unsigned choice) {
    unsigned scene = preparation_near(g, player);
    if (!scene || choice < 1 || choice > 3)
        return "";
    if (scene == PREP_MEALS_SCENE) {
        static const char *meals[] = {
            "INSULATED BROTH / 30 SALVAGE / HALF ELECTRICAL DAMAGE / 5 MIN",
            "EMBER STEW / 30 SALVAGE / HALF COLD DAMAGE AND SLOW / 5 MIN",
            "CHARGED TEA / 30 SALVAGE / +6 ENERGY PER SECOND / 5 MIN"};
        return meals[choice - 1];
    }
    if (choice == 1)
        return g->home.projects & PROJECT_REFIT
                   ? "REFIT / 25 SALVAGE / REFUND SPENT ATTRIBUTES AS POINTS / NO RECOVERY"
                   : "BUILD SHARED REFIT BENCH / 250 SALVAGE / SAFE ATTRIBUTE REDISTRIBUTION";
    if (choice == 2)
        return g->home.projects & PROJECT_FLARES
                   ? "PREPARE ONE FLARE / 75 SALVAGE / DOUBLE LIFE, +50% LIGHT / FLARE CANNON "
                     "REQUIRED"
                   : "BUILD SHARED FLARE CALIBRATOR / 200 SALVAGE / EARNED FLARE CANNON REQUIRED";
    unsigned c = research(&g->home);
    return c == 1   ? "RESEARCH OBSERVED POWER / 100 SALVAGE / LEARN WARNINGS AND SAFE WATER"
           : c == 2 ? "RESEARCH OBSERVED FREEZE / 100 SALVAGE / LEARN VENT TIMING AND COVER"
           : c == 3 ? "RESEARCH OBSERVED CONDUITS / 100 SALVAGE / LEARN GROWTH AND ISOLATION"
                    : "NO UNRESEARCHED OBSERVED CONDITION / EXPLORE BEFORE RESEARCHING";
}
void preparation_entry(const Game *g, int player, unsigned scene, char *text, size_t size) {
    if (scene == PREP_RECORD_SCENE) {
        size_t index = g->views[player].district_reading;
        if (index >= g->world.record_count) {
            snprintf(text, size, "NO DISTRICT RECORD SELECTED / L CYCLES OBSERVED OUTCOMES.");
            return;
        }
        const RoomRecord *record = &g->world.records[index];
        RoomSpec spec = world_describe_version(g->world.seed, g->world.generator, record->cell);
        unsigned facts = record->district_state;
        snprintf(text, size,
                 "%s [%d,%d,%d]. %s%s%s%s%s THE JOURNAL RETAINS THIS RECORD WHEN THE SIX DISPLAY "
                 "SLOTS ARE FULL.",
                 district_name(spec.journey.family), record->cell.x, record->cell.y, record->cell.z,
                 facts & DISTRICT_SHORTCUT ? "RETURN CONDUIT OPEN. " : "",
                 facts & DISTRICT_CONDUCTOR ? "CONDUCTOR SEVERED. " : "",
                 facts & DISTRICT_RESCUED ? "REFUGE REPAIRED. " : "",
                 facts & DISTRICT_SALVAGED ? "REFUGE SALVAGED. " : "",
                 facts & DISTRICT_LOST ? "REFUGE LOST. " : "");
        return;
    }
    const PersonalPreparation *p = &g->home.personal[player];
    if (scene == PREP_MEALS_SCENE)
        snprintf(text, size,
                 "OPTIONAL MEAL / ONE BENEFIT AT A TIME. A NEW MEAL REPLACES THE OLD ONE. ACTIVE: "
                 "%s, %.0F SECONDS. THE BED, MEDICINE AND BASIC STARGRAIN STEW REMAIN FREE. TIMERS "
                 "PAUSE INSIDE THE FOLD.",
                 preparation_meal_name(p->meal), p->remaining);
    else {
        const char *note = g->home.researched & 2 ? "POWER: 2S WARNING; LEFT WATER LIVE FOR 5S. "
                                                    "HIGH GROUND AND THE DRY CENTER ARE SAFE. "
                                                  : "";
        snprintf(text, size,
                 "SHARED FACILITIES / PERSONAL PREPARATION. %s%s%s%sREFITTING CONSERVES YOUR "
                 "POINTS; IT DOES NOT GRANT EQUIPMENT OR HEAL YOU.",
                 note,
                 g->home.researched & 4
                     ? "FREEZE: VENTS WARN BEFORE THE COLD SWEEP; USE THE RAISED SHELTERED ROUTE. "
                     : "",
                 g->home.researched & 8 ? "CONDUITS: ISOLATE THE LIVE FEED BEFORE CROSSING GROWTH. "
                                        : "",
                 p->flare_charge ? "ONE CALIBRATED FLARE READY. " : "");
    }
}
bool preparation_choose(Game *g, int player, unsigned choice) {
    unsigned scene = preparation_near(g, player);
    if (!scene || choice < 1 || choice > 3 || !g->interactions[player].decision_open ||
        g->interactions[player].decision_scene != scene)
        return false;
    Player *p = &g->players[player];
    Home *h = &g->home;
    PersonalPreparation *prep = &h->personal[player];
    unsigned cost = 0, project = 0, condition = 0;
    if (scene == PREP_PROJECTS_SCENE &&
        g->interactions[player].prep_quote != (h->projects | (h->researched << 3))) {
        g->interactions[player].prep_quote = h->projects | (h->researched << 3);
        game_message(g, "BENCH UPDATED BY YOUR PARTNER / REVIEW THE NEW CHOICES BEFORE CONFIRMING");
        return false;
    }
    if (scene == PREP_MEALS_SCENE) {
        if (prep->meal == (MealKind)choice && prep->remaining > 0)
            return false;
        cost = 30;
    } else if (choice == 1) {
        project = PROJECT_REFIT;
        cost = h->projects & project ? 25 : 250;
        if ((h->projects & project) &&
            (!player_investment(p) || p->points + player_investment(p) > 1000))
            return false;
    } else if (choice == 2) {
        if (!(p->weapons & (1u << SC_FLARE_CANNON)) || prep->flare_charge)
            return false;
        project = PROJECT_FLARES;
        cost = h->projects & project ? 75 : 200;
    } else {
        condition = research(h);
        if (!condition)
            return false;
        project = PROJECT_RESEARCH;
        cost = 100;
    }
    if (p->credits < cost)
        return false;
    if (scene == PREP_MEALS_SCENE) {
        prep->meal = (MealKind)choice;
        prep->remaining = 300;
    } else if (project == PROJECT_REFIT && (h->projects & project)) {
        p->points += player_investment(p);
        for (unsigned a = 0; a < SC_ATTRIBUTES; a++)
            *player_attribute(p, (int)a) = 0;
        p->health = fminf(p->health, player_max_health(p));
        p->shield = fminf(p->shield, player_max_shield(p));
        p->armor = fminf(p->armor, player_max_armor(p));
        p->energy = fminf(p->energy, player_max_energy(p));
        for (unsigned rig = 0; rig < 2; rig++)
            g->rigs[player][rig].armor =
                fminf(g->rigs[player][rig].armor, game_rig_max_armor(p, (RigKind)(rig + 1)));
        g->builds[player] = (BuildEffects){0};
    } else if (project == PROJECT_FLARES && (h->projects & project))
        prep->flare_charge = true;
    else if (project == PROJECT_RESEARCH)
        h->researched |= 1u << condition;
    h->projects |= project;
    p->credits -= cost;
    g->interactions[player].decision_open = g->views[player].journal = false;
    g->dirty = true;
    game_message(g, scene == PREP_MEALS_SCENE
                        ? "MEAL PREPARED / ONE ACTIVE BENEFIT / FIVE MINUTES OUTSIDE THE FOLD"
                        : "PREPARATION RECORDED / USE THE BENCH AGAIN FOR PERSONAL OPTIONS");
    return true;
}
void preparation_observe(Game *g, const Room *r) {
    if (g->world.generator < 16 || !r->journey.family)
        return;
    bool present = false;
    for (int p = 0; p < g->player_count; p++)
        present |= !g->players[p].sheltered && !g->matrix.divers[p].active &&
                   world_find(&g->world, world_cell(position_absolute(g->players[p].pos))) == r;
    if (!present)
        return;
    uint32_t old = g->home.observed;
    g->home.observed |= 1u << r->journey.condition;
    g->dirty |= old != g->home.observed;
    unsigned facts = world_district_state(&g->world, r->journey.anchor) & ~DISTRICT_SEEN;
    if (!facts)
        return;
    unsigned slot = 0;
    while (slot < g->home.keepsake_count &&
           !cell_equal(g->home.keepsakes[slot].anchor, r->journey.anchor))
        slot++;
    if (slot == SC_KEEPSAKES)
        return;
    Keepsake value = {r->journey.anchor, r->journey.identity, r->journey.family,
                      r->journey.condition, facts};
    if (slot < g->home.keepsake_count && !memcmp(&g->home.keepsakes[slot], &value, sizeof(value)))
        return;
    if (slot == g->home.keepsake_count)
        g->home.keepsake_count++;
    g->home.keepsakes[slot] = value;
    g->dirty = true;
}
void preparation_step(Game *g, double dt) {
    if (g->world.generator < 16)
        return;
    for (int i = 0; i < g->player_count; i++) {
        PersonalPreparation *p = &g->home.personal[i];
        if (!p->meal || g->players[i].sheltered)
            continue;
        p->remaining = fmaxf(0, p->remaining - (float)dt);
        if (!p->remaining)
            p->meal = MEAL_NONE;
        if (p->meal == MEAL_CHARGED)
            g->players[i].energy =
                fminf(player_max_energy(&g->players[i]), g->players[i].energy + (float)dt * 6);
    }
}
float preparation_damage(const Game *g, int player, float damage, uint32_t flags) {
    if (g->world.generator < 16 || player < 0 || player >= g->player_count)
        return damage;
    MealKind meal = g->home.personal[player].meal;
    return (meal == MEAL_INSULATED && (flags & HURT_ELECTRIC)) ||
                   (meal == MEAL_WARM && (flags & CHILL))
               ? damage * .5f
               : damage;
}
void preparation_flare(Game *g, int player, unsigned slot) {
    if (g->world.generator < 16 || player < 0 || player >= g->player_count ||
        slot >= SC_MAX_FLARES || !g->home.personal[player].flare_charge || !g->flares[slot].active)
        return;
    g->home.personal[player].flare_charge = false;
    g->flares[slot].life *= 2;
    g->flares[slot].power *= 1.5f;
}
bool preparation_next_record(Game *g, int player, bool first) {
    if (g->world.generator < 16 || player < 0 || player >= g->player_count)
        return false;
    for (size_t i = first ? 0 : g->views[player].district_reading + 1; i < g->world.record_count;
         i++)
        if (g->world.records[i].district_state & ~DISTRICT_SEEN) {
            g->views[player].district_reading = i;
            g->views[player].story_reading = PREP_RECORD_SCENE;
            return true;
        }
    return false;
}
bool preparation_valid(const Home *h, const World *w, const Player *players) {
    if (h->projects > 7 || (h->observed & ~14u) || (h->researched & ~h->observed) ||
        (h->researched && !(h->projects & PROJECT_RESEARCH)) || h->keepsake_count > SC_KEEPSAKES)
        return false;
    for (unsigned p = 0; p < SC_PLAYERS; p++) {
        const PersonalPreparation *s = &h->personal[p];
        if ((unsigned)s->meal > MEAL_CHARGED || !isfinite(s->remaining) || s->remaining < 0 ||
            s->remaining > 300 || (s->meal == MEAL_NONE) != (s->remaining == 0) ||
            (s->flare_charge &&
             (!(h->projects & PROJECT_FLARES) || !(players[p].weapons & (1u << SC_FLARE_CANNON)))))
            return false;
    }
    for (unsigned i = 0; i < h->keepsake_count; i++) {
        const Keepsake *k = &h->keepsakes[i];
        unsigned recorded = world_district_state(w, k->anchor);
        if (!w->record_index)
            for (size_t r = 0; r < w->record_count; r++)
                if (cell_equal(w->records[r].cell, k->anchor)) {
                    recorded = w->records[r].district_state;
                    break;
                }
        if (k->family < 1 || k->family > 4 || k->condition < 1 || k->condition > 3 || !k->facts ||
            (k->facts & ~SC_DISTRICT_STATE_MASK) || (k->facts & ~recorded))
            return false;
        for (unsigned j = 0; j < i; j++)
            if (cell_equal(k->anchor, h->keepsakes[j].anchor))
                return false;
    }
    return true;
}
