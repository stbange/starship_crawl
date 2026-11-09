/* Read-only narrative evidence from an earned checkpoint. No simulation ticks. */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>

static void string(const char *s) {
    putchar('"');
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p == '"' || *p == '\\')
            putchar('\\');
        if (*p < 32)
            printf("\\u%04x", *p);
        else
            putchar(*p);
    }
    putchar('"');
}
static void field(const char *key, const char *value) {
    printf(",\"%s\":", key);
    string(value);
}
int main(int argc, char **argv) {
    if (argc < 2)
        return 2;
    for (int file = 1; file < argc; file++) {
        Game *g = calloc(1, sizeof(*g));
        if (!g)
            return 2;
        game_init(g, 713);
        if (!game_load(g, argv[file])) {
            fprintf(stderr, "Cannot load %s\n", argv[file]);
            game_destroy(g);
            free(g);
            return 1;
        }
        ShipHistory past = world_history(g->world.seed);
        printf("{\"seed\":%u,\"generator\":%u,\"seen\":%u,\"done\":%u,"
               "\"choice\":%u,\"extracted\":%s,\"chapter\":%u",
               g->world.seed, g->world.generator, g->story.seen, g->story.done, g->story.choice,
               (g->story.done & (1u << STORY_EXTRACTION)) ? "true" : "false", g->story.chapter);
        field("checkpoint", argv[file]);
        field("ship", story_ship(g->world.seed));
        field("commission", story_commission_version(g->world.seed, g->story.version));
        field("ship_type", world_profile_name(world_profile(g->world.seed)));
        field("history", world_history_cause(past.cause));
        field("controller", world_history_controller(past.cause));
        field("survivor_agenda", world_history_agenda(g->world.seed));
        printf(",\"debrief\":[");
        char text[1024];
        for (int line = 0; line < 8; line++) {
            if (line)
                putchar(',');
            game_story_debrief(g, 0, line, text, sizeof(text));
            string(text);
        }
        printf("],\"scenes\":[");
        for (unsigned i = 0; i < SC_STORY_SCENES; i++) {
            printf("%s{\"id\":%u,\"seen\":%s,\"done\":%s,\"site\":[%d,%d,%d]", i ? "," : "", i,
                   (g->story.seen & (1u << i)) ? "true" : "false",
                   (g->story.done & (1u << i)) ? "true" : "false", g->story.sites[i].x,
                   g->story.sites[i].y, g->story.sites[i].z);
            field("title", game_story_title(g, i));
            game_story_entry(g, i, text, sizeof(text));
            field("entry", text);
            putchar('}');
        }
        printf("],\"guardians\":[");
        for (unsigned i = 0; i < SC_GUARDIANS; i++) {
            printf("%s{\"id\":%u,\"defeated\":%s", i ? "," : "", i,
                   (g->campaign.defeated & (1u << i)) ? "true" : "false");
            field("name", campaign_guardian_name(g->world.seed, i));
            putchar('}');
        }
        printf("],\"themes\":[");
        for (unsigned i = 0; i < SC_THEMES; i++) {
            if (i)
                putchar(',');
            string(sc_themes[i].name);
        }
        printf("],\"weapons\":[");
        for (unsigned i = 0; i < SC_WEAPONS; i++) {
            printf("%s{\"id\":%u,\"owned\":%s", i ? "," : "", i,
                   (g->players[0].weapons & (1u << i)) ? "true" : "false");
            field("name", sc_weapons[i].name);
            putchar('}');
        }
        puts("]}");
        game_destroy(g);
        free(g);
    }
    return 0;
}
