#include "game.h"
/* Stable semantic commands. Numeric browser actions are translated by the client adapter. */
const CommandDefinition sc_commands[CMD_COUNT] = {
    [CMD_CAMPAIGN_BRANCH] = {"Equip previewed branch",
                             0,
                             SC_BUILD_BRANCHES,
                             {"G IN BUILD", "A IN BUILD", "MENU / EQUIP PREVIEW",
                              "Y / EQUIP PREVIEW"}},
    [CMD_LOOK] = {"Look", 0, 0, {"MOUSE", "RIGHT STICK", "LOOK PAD", "HEAD"}},
    [CMD_AIM] = {"Tracked look", 0, 0, {"MOUSE", "RIGHT STICK", "LOOK PAD", "HEAD"}},
    [CMD_UPGRADE] = {"Upgrade attribute",
                     0,
                     9,
                     {"F4 / CHARACTER", "DPAD LEFT / UP / RIGHT", "MENU / CHARACTER",
                      "Y / CHARACTER UPGRADES"}},
    [CMD_WEAPON_CYCLE] = {"Cycle weapon", -1, 1, {"Z / X", "RIGHT SHOULDER", "WEAPON", "X"}},
    [CMD_WEAPON_SELECT] = {"Select weapon",
                           0,
                           SC_WEAPONS - 1,
                           {"CHARACTER", "BUILD", "MENU / WEAPONS", "Y / WEAPONS"}},
    [CMD_RIG_WEAPON] = {"Select mech weapon",
                        0,
                        SC_MECH_WEAPONS - 1,
                        {"Z / X", "RIGHT SHOULDER", "WEAPON", "X"}},
    [CMD_FLASHLIGHT] = {"Flashlight", 0, 0, {"F", "RIGHT STICK CLICK", "LAMP", "LEFT GRIP"}},
    [CMD_INTERACT] = {"Interact", 0, 0, {"G", "DPAD DOWN", "USE", "RIGHT GRIP"}},
    [CMD_PSIONIC] = {"Psionic",
                     0,
                     SC_PSIONICS - 1,
                     {"V / B / J / H", "LEFT TRIGGER + DPAD", "CAST",
                      "Y / ABILITIES AND EQUIPMENT"}},
    [CMD_RIG_TOGGLE] = {"Deploy rig",
                        MECH,
                        WAGON,
                        {"T / Y", "LEFT TRIGGER + A / B", "MENU / EQUIPMENT",
                         "Y / ABILITIES AND EQUIPMENT"}},
    [CMD_FLARE_UPGRADE] = {"Upgrade flare",
                           0,
                           1,
                           {"O / P", "MENU", "MENU / EQUIPMENT", "Y / ABILITIES AND EQUIPMENT"}},
    [CMD_SHELTER] = {"Fold shelter",
                     0,
                     0,
                     {"R", "BACK / SELECT", "MENU / SHELTER", "MENU / EQUIPMENT"}},
    [CMD_MATRIX_TOGGLE] = {"Matrix link", 0, 0, {"C", "BACK / SELECT", "USE", "RIGHT GRIP"}},
    [CMD_MATRIX_SERVICE] = {"Run service", 0, 0, {"F", "X", "MENU / RUN SERVICE", "RIGHT GRIP"}},
    [CMD_MATRIX_CYCLE] = {"Choose service",
                          -1,
                          1,
                          {"Q / E", "DPAD", "MENU / SERVICE", "Y / MATRIX"}},
    [CMD_MATRIX_CAST] = {"Run program", 0, 1, {"1 / 2", "RIGHT TRIGGER", "FIRE", "RIGHT TRIGGER"}},
    [CMD_MATRIX_SELECT] = {"Select program slot", 0, 1, {"Z / X", "SHOULDER", "WEAPON", "X"}},
    [CMD_MATRIX_EQUIP] = {"Equip program",
                          0,
                          SC_MATRIX_PROGRAMS - 1,
                          {"F8", "LEFT TRIGGER + RIGHT STICK", "MENU / PROGRAMS", "Y / PROGRAMS"}},
    [CMD_MATRIX_UPGRADE] = {"Upgrade interface",
                            0,
                            0,
                            {"R", "DPAD UP", "MENU / UPGRADE", "Y / PROGRAMS"}},
    [CMD_STORY_CHOOSE] = {"Story choice",
                          1,
                          3,
                          {"1 / 2 / 3", "DPAD", "MENU / JOURNAL", "Y / JOURNAL"}},
    [CMD_MATRIX_CHOOSE] = {"Controller choice",
                           1,
                           3,
                           {"1 / 2 / 3", "DPAD", "MENU / JOURNAL", "Y / MATRIX"}},
    [CMD_POLITICS_CHOOSE] = {"Court choice",
                             1,
                             3,
                             {"1 / 2 / 3", "DPAD", "MENU / JOURNAL", "Y / COURTS"}},
    [CMD_CAMPAIGN_CHOOSE] = {"Claim reward",
                             1,
                             3,
                             {"F10", "LEFT TRIGGER + START", "MENU / BUILD", "Y / BUILD"}},
    [CMD_CAMPAIGN_REFIT] =
        {"Refit loadout", 0, 5, {"F10", "LEFT TRIGGER + START", "MENU / BUILD", "Y / BUILD"}},
    [CMD_TINT] = {"Armor color", 0, 0, {"PAUSE / C", "MENU", "MENU / COLOR", "Y / SETTINGS"}},
    [CMD_CLOSE_INTERACTION] = {"Close interaction",
                               0,
                               0,
                               {"ENTER / ESCAPE", "START", "RESUME", "B"}},
    [CMD_DISMISS_HINT] = {"Dismiss hint",
                          0,
                          0,
                          {"BACKSPACE", "SETTINGS", "MENU / DISMISS HINT", "Y / DISMISS HINT"}}};
const char *game_command_binding(CommandKind kind, unsigned device) {
    if ((unsigned)kind >= CMD_COUNT)
        return "";
    return sc_commands[kind].bindings[device < 4 ? device : 0];
}
bool game_command(Game *g, const GameCommand *c) {
    if (!c)
        return false;
    int i = game_player_slot(g, c->player);
    if (i < 0 || (unsigned)c->kind >= CMD_COUNT || !isfinite(c->x) || !isfinite(c->y))
        return false;
    const CommandDefinition *def = &sc_commands[c->kind];
    if (c->argument < def->minimum || c->argument > def->maximum ||
        (c->kind == CMD_CAMPAIGN_REFIT ? (c->auxiliary != -1 && c->auxiliary != 1)
                                       : c->auxiliary != 0))
        return false;
    if (c->kind != CMD_LOOK && c->kind != CMD_AIM && (c->x != 0 || c->y != 0))
        return false;
    Player *p = &g->players[i];
    switch (c->kind) {
    case CMD_LOOK:
        game_look(p, c->x, c->y);
        break;
    case CMD_AIM:
        if (fabs(c->x) > SC_PI || fabs(c->y) > 1.48)
            return false;
        p->yaw = c->x;
        p->pitch = c->y;
        break;
    case CMD_UPGRADE:
        return game_upgrade(g, i, c->argument);
    case CMD_WEAPON_CYCLE:
        if (!c->argument)
            return false;
        game_weapon(g, i, c->argument);
        break;
    case CMD_WEAPON_SELECT:
        if (p->riding == MECH || !(p->weapons & (1u << c->argument)) ||
            !campaign_weapon(g, i, c->argument))
            return false;
        p->weapon = c->argument;
        p->cooldown = .15f;
        g->dirty = true;
        break;
    case CMD_RIG_WEAPON:
        if (!campaign_rig(g, i, MECH))
            return false;
        p->rig_weapon = c->argument;
        p->cooldown = .15f;
        g->dirty = true;
        break;
    case CMD_FLASHLIGHT:
        game_flashlight(g, i);
        break;
    case CMD_INTERACT:
        game_interact(g, i);
        break;
    case CMD_PSIONIC:
        if (!campaign_power(g, i, (unsigned)c->argument))
            return false;
        game_psionic(g, i, (Psionic)c->argument);
        break;
    case CMD_RIG_TOGGLE:
        if (!campaign_rig(g, i, (unsigned)c->argument))
            return false;
        game_rig_toggle(g, i, (RigKind)c->argument);
        break;
    case CMD_FLARE_UPGRADE:
        return game_flare_upgrade(g, i, c->argument != 0);
    case CMD_SHELTER:
        if (!(onboarding_capabilities(g, i) & OB_SHELTER))
            return false;
        game_summon(g, i);
        break;
    case CMD_MATRIX_TOGGLE:
        return matrix_toggle(g, i);
    case CMD_MATRIX_SERVICE:
        return matrix_service(g, i);
    case CMD_MATRIX_CYCLE:
        if (!c->argument)
            return false;
        matrix_cycle_service(g, i, c->argument);
        break;
    case CMD_MATRIX_CAST:
        return matrix_cast(g, i, c->argument);
    case CMD_MATRIX_SELECT:
        matrix_select(g, i, c->argument);
        break;
    case CMD_MATRIX_EQUIP:
        return matrix_equip(g, i, c->argument);
    case CMD_MATRIX_UPGRADE:
        return matrix_upgrade(g, i);
    case CMD_STORY_CHOOSE:
        return game_story_choose(g, i, c->argument);
    case CMD_MATRIX_CHOOSE:
        return matrix_choose(g, i, c->argument);
    case CMD_POLITICS_CHOOSE:
        return politics_choose(g, i, c->argument);
    case CMD_CAMPAIGN_CHOOSE:
        return campaign_choose(g, i, c->argument);
    case CMD_CAMPAIGN_REFIT:
        return campaign_refit(g, i, c->argument, c->auxiliary);
    case CMD_TINT:
        p->tint = (p->tint + 1) % 3;
        g->dirty = true;
        break;
    case CMD_DISMISS_HINT:
        onboarding_dismiss(g, i);
        break;
    case CMD_CLOSE_INTERACTION:
        g->views[i].branch_preview = 0;
        g->interactions[i].decision_open = false;
        g->politics.audience[i] = 0;
        g->matrix.divers[i].choice = false;
        break;
    case CMD_CAMPAIGN_BRANCH:
        return game_build_equip(g, i, (unsigned)c->argument);
    case CMD_COUNT:
        return false;
    }
    return true;
}
