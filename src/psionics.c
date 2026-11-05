#include "game.h"
const char *sc_psionic_names[SC_PSIONICS] = {"VEIL", "FLIGHT", "LONG JUMP", "NOVA AURA"};
const float sc_psionic_costs[SC_PSIONICS] = {25, 20, 15, 35};
void game_psionic(Game *g, int player, Psionic power) {
    if (player < 0 || player >= g->player_count || power < 0 || power >= SC_PSIONICS)
        return;
    if (!campaign_power(g, player, (unsigned)power)) {
        game_message(g, "POWER LOCKED OR STORED / DEFEAT GUARDIANS / F10 LOADOUT");
        return;
    }
    Player *p = &g->players[player];
    p->selected_power = (uint8_t)power;
    if (p->sheltered || p->riding || p->health <= 0) {
        game_message(g, "PSIONICS REQUIRE BEING ON FOOT OUTSIDE THE SANCTUARY");
        return;
    }
    bool active = power == PSI_VEIL     ? p->cloaked
                  : power == PSI_FLIGHT ? p->flight > 0
                  : power == PSI_LEAP   ? p->leap > 0
                                        : p->nova > 0;
    if (!active && p->energy < sc_psionic_costs[power]) {
        game_message(g, "NOT ENOUGH ENERGY / REACTOR RECHARGING");
        return;
    }
    if (!active)
        p->energy -= sc_psionic_costs[power];
    if (power == PSI_VEIL) {
        p->cloaked = !active;
        p->nova = p->nova_tick = 0;
    }
    if (power == PSI_FLIGHT) {
        p->flight = active ? 0 : 18;
        p->velocity = v3(0, 0, 0);
    }
    if (power == PSI_LEAP)
        p->leap = active ? 0 : 45;
    if (power == PSI_NOVA) {
        p->nova = active ? 0 : 8;
        p->nova_tick = 0;
        if (!active)
            p->cloaked = false;
    }
    g->dirty = true;
    const char *messages[] = {"VEIL / HIDDEN UNTIL FIRING / V TO REVEAL",
                              "FLIGHT / SPACE RISE / CTRL DESCEND / B TO LAND",
                              "LONG JUMPS CHARGED / SPACE TO LEAP / J TO CANCEL",
                              "NOVA AURA / NEARBY ENEMIES TAKE PULSING DAMAGE"};
    game_message(g, active ? "PSIONIC POWER RELEASED" : messages[power]);
}
void game_psionics_step(Game *g, Player *p, double dt) {
    p->flight = fmaxf(0, p->flight - (float)dt);
    p->leap = fmaxf(0, p->leap - (float)dt);
    p->nova = fmaxf(0, p->nova - (float)dt);
    p->nova_flash = fmaxf(0, p->nova_flash - (float)dt);
    p->nova_tick = fmaxf(0, p->nova_tick - (float)dt);
    if (p->nova <= 0) {
        p->nova_tick = 0;
        return;
    }
    if (p->sheltered || p->riding)
        return;
    if (p->nova_tick <= 0) {
        game_nova(g, (int)(p - g->players));
        p->nova_tick = .5f;
        p->nova_flash = .35f;
    }
}
