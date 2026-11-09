#ifndef SC_COMMANDS_H
#define SC_COMMANDS_H
#include "identity.h"
typedef enum {
    CMD_LOOK,
    CMD_AIM,
    CMD_UPGRADE,
    CMD_WEAPON_CYCLE,
    CMD_WEAPON_SELECT,
    CMD_RIG_WEAPON,
    CMD_FLASHLIGHT,
    CMD_INTERACT,
    CMD_PSIONIC,
    CMD_RIG_TOGGLE,
    CMD_FLARE_UPGRADE,
    CMD_SHELTER,
    CMD_MATRIX_TOGGLE,
    CMD_MATRIX_SERVICE,
    CMD_MATRIX_CYCLE,
    CMD_MATRIX_CAST,
    CMD_MATRIX_SELECT,
    CMD_MATRIX_EQUIP,
    CMD_MATRIX_UPGRADE,
    CMD_STORY_CHOOSE,
    CMD_MATRIX_CHOOSE,
    CMD_POLITICS_CHOOSE,
    CMD_CAMPAIGN_CHOOSE,
    CMD_CAMPAIGN_REFIT,
    CMD_TINT,
    CMD_DISMISS_HINT,
    CMD_CLOSE_INTERACTION,
    CMD_COUNT
} CommandKind;
typedef struct {
    CommandKind kind;
    PlayerId player;
    int32_t argument, auxiliary;
    double x, y;
} GameCommand;
typedef struct {
    const char *name;
    int minimum, maximum;
    const char *bindings[4];
} CommandDefinition;
extern const CommandDefinition sc_commands[CMD_COUNT];
const char *game_command_binding(CommandKind kind, unsigned device);
bool game_command(struct Game *g, const GameCommand *command);
#endif
