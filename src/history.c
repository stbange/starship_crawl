#include "world.h"
#include <stdio.h>

ShipHistory world_history(uint32_t seed) {
    uint32_t h = world_hash(seed, (Cell){0}, UINT32_C(0x48495354));
    ShipHistory history = {.cause = (HistoryCause)(h % HISTORY_CAUSES),
                           .arc = (HistoryArc)((h >> 5) % 2),
                           .origin = {2 + (int32_t)((h >> 8) % 2), (int32_t)((h >> 10) % 3) - 1, 0},
                           .population = 32 + (h >> 16) % 193};
    return history;
}
unsigned world_history_theme(HistoryCause cause) {
    const unsigned themes[] = {26, 20, 14, 2};
    return themes[(unsigned)cause % HISTORY_CAUSES];
}
Incident world_history_incident(HistoryCause cause) {
    const Incident incidents[] = {FROST, BLOOM, QUARANTINE, FIRE};
    return incidents[(unsigned)cause % HISTORY_CAUSES];
}
const char *world_history_cause(HistoryCause cause) {
    const char *names[] = {"CRYONIC REGULATORS RUPTURED AND RELEASED THE SLEEPING INFESTATION",
                           "A XENOBIOLOGY CULTURE ESCAPED INTO THE POWER CONDUITS",
                           "A DEFENSE INTELLIGENCE PURGED ITS CREW AND OPENED CONTAINMENT",
                           "A REACTOR CASCADE BURNT THROUGH THE CONTAINMENT GRID"};
    return names[(unsigned)cause % HISTORY_CAUSES];
}
const char *world_history_controller(HistoryCause cause) {
    const char *names[] = {
        "THE RIME CHOIR / FAILED CRYONIC REGULATORS", "THE ROOT / A CULTURE FEEDING ON SHIP POWER",
        "ORISON / THE QUARANTINE INTELLIGENCE", "THE ASH CHOIR / FRACTURED REACTOR AUTOMATA"};
    return names[(unsigned)cause % HISTORY_CAUSES];
}
const char *world_history_agenda(uint32_t seed) {
    const char *goals[] = {"THE PILGRIMS WANT THEIR SLEEPERS TO FINISH THE VOYAGE",
                           "THE PASSENGERS WANT A SAFE ROUTE TO THE NEAREST PORT",
                           "THE PRISONERS WANT THEIR CELL BLOCKS OPENED AND A WAY HOME",
                           "THE CUSTODIANS WANT THEIR PEOPLE AND SEED LIBRARY TO SURVIVE"};
    return goals[world_profile(seed)];
}
const char *world_history_role(unsigned role) {
    const char *names[] = {"", "CATASTROPHE ORIGIN", "SURVIVOR WAYSTATION", "CONTROLLER NOTICE"};
    return names[role % 4];
}
void world_history_notice(uint32_t seed, unsigned role, char *text, size_t size) {
    ShipHistory h = world_history(seed);
    if (role == HISTORY_ORIGIN)
        snprintf(text, size, "DAMAGE RECORD: %s. ORIGIN [%d,%d,%d].", world_history_cause(h.cause),
                 h.origin.x, h.origin.y, h.origin.z);
    else if (role == HISTORY_SURVIVORS)
        snprintf(text, size, "REFUGE NETWORK: %u PEOPLE REPORTED. %s.", h.population,
                 world_history_agenda(seed));
    else if (role == HISTORY_CONTROL)
        snprintf(text, size,
                 "ACTIVE CONTROL SIGNATURE: %s. THE LOCAL LOCKOUT DATES TO THE COLLAPSE.",
                 world_history_controller(h.cause));
    else
        snprintf(text, size, "NO HISTORICAL NOTICE AT THIS SITE");
}
