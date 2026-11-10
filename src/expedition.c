#include "game.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIT(n) (UINT32_C(1) << (n))
#define EVIDENCE (BIT(STORY_REFUGE) | BIT(STORY_ARCHIVE) | BIT(STORY_LEDGER))
#define PASSIVE                                                                                    \
    (UINT32_C(62) | BIT(STORY_ORIGIN_RECORD) | BIT(STORY_SURVIVOR_RECORD) |                        \
     BIT(STORY_CONTROL_RECORD))
static const char *arcs[STORY_ARCS] = {"HARROW / ORISON'S HEART", "THE STRANDED REFUGE",
                                       "THE LOST CATALOGUE", "THE QUARANTINE WAKE",
                                       "THE DEAD FABRICATOR"};
/* Prerequisites and effects accompany the architecture/sector tags in the scene catalog. */
static const struct {
    uint32_t needs, effect;
} rules[] = {{BIT(STORY_BRIEFING), SYSTEM_POWER},
             {BIT(STORY_BRIEFING), SYSTEM_BEACON},
             {BIT(STORY_BRIEFING), 0},
             {BIT(STORY_BRIEFING), 0},
             {BIT(STORY_CIPHER), SYSTEM_SILENCED}};
static bool has(const Story *s, unsigned scene) {
    return (s->done & BIT(scene)) != 0;
}
static void support(Story *s, int refuge, int keepers) {
    s->support[0] = (uint32_t)clampd((double)s->support[0] + refuge, 0, 100);
    s->support[1] = (uint32_t)clampd((double)s->support[1] + keepers, 0, 100);
}
void expedition_record(Game *g, unsigned scene, unsigned choice) {
    Story *s = &g->story;
    if (s->version < 3 || scene >= SC_STORY_SCENES)
        return;
    if (s->event_count == SC_STORY_EVENTS) {
        memmove(s->events, s->events + 1, (SC_STORY_EVENTS - 1) * sizeof(*s->events));
        s->event_count--;
    }
    s->events[s->event_count++] = (StoryEvent){.chapter = s->chapter,
                                               .scene = scene,
                                               .choice = choice,
                                               .arc = s->arc,
                                               .site = s->sites[scene]};
    if (scene == STORY_SURVIVORS || scene == STORY_MEMORY || scene == STORY_DRIVE ||
        scene == STORY_SOURCE || scene == STORY_RESOLUTION || scene == STORY_EXTRACTION ||
        scene == STORY_RETURN)
        memcpy(s->events[s->event_count - 1].outcome, s->last, 4 * sizeof(uint32_t));
    g->dirty = true;
}
bool expedition_available(const Story *s, unsigned scene) {
    if (scene >= SC_STORY_SCENES || scene < STORY_POWER || !(s->assigned & BIT(scene)) ||
        has(s, scene))
        return false;
    if (scene <= STORY_OVERRIDE)
        return !s->choice &&
               (s->done & rules[scene - STORY_POWER].needs) == rules[scene - STORY_POWER].needs;
    if (scene == STORY_CONTINUE)
        return has(s, STORY_EXTRACTION) && s->chapter < 1000 &&
               (!s->chapter || has(s, STORY_RETURN));
    return s->chapter && (scene == STORY_RETURN ? s->chapter_choice != 0 : s->chapter_choice == 0);
}
void expedition_observe(Game *g, RoomSpec room) {
    Story *s = &g->story;
    if (s->version < 3 || s->choice || s->chapter || !has(s, STORY_BRIEFING))
        return;
    bool witness = (s->done & EVIDENCE) != 0;
    bool known[] = {witness || (s->seen & (BIT(STORY_CONDUIT) | BIT(STORY_FROST))),
                    witness || (s->seen & UINT32_C(62)),
                    witness || (s->seen & (BIT(STORY_RESEARCH) | BIT(STORY_CONTROL_RECORD))),
                    witness || (s->seen & BIT(STORY_SURVIVOR_RECORD)), has(s, STORY_CIPHER)};
    for (unsigned i = 0; i < 5; i++)
        if (known[i] && !(s->assigned & BIT(STORY_POWER + i))) {
            story_reserve(g, STORY_POWER + i, room.cell);
            if (STORY_POWER + i == STORY_COUNCIL) {
                uint32_t flags = (has(s, STORY_POWER) || has(s, STORY_REACTOR) ? SYSTEM_POWER : 0) |
                                 (has(s, STORY_BEACON) ? SYSTEM_BEACON : 0);
                if (flags)
                    world_set_systems(&g->world, s->sites[STORY_COUNCIL], flags);
            }
            g->dirty = true;
        }
}
static bool system_change(Game *g, Cell c, uint32_t add, uint32_t remove) {
    uint32_t state = (world_systems(&g->world, c) & ~remove) | add;
    return world_set_systems(&g->world, c, state);
}
static bool power(Game *g, Cell c) {
    return system_change(g, c, SYSTEM_POWER, SYSTEM_BLACKOUT);
}
static bool camp_power(Game *g) {
    return !(g->story.assigned & BIT(STORY_COUNCIL)) || power(g, g->story.sites[STORY_COUNCIL]);
}
bool game_story_safe(const Game *g, Vec3 feet) {
    if (feet.z < 0)
        return false;
    Cell c = world_cell(feet);
    uint32_t flags = world_systems(&g->world, c);
    if ((flags & (SYSTEM_POWER | SYSTEM_BEACON)) != (SYSTEM_POWER | SYSTEM_BEACON))
        return false;
    Cell owner = world_describe_version(g->world.seed, g->world.generator, c).cell;
    return vlen(vsub(feet, game_terminal(owner))) <= 18;
}
bool game_story_inert(const Game *g, const Enemy *e) {
    return (sc_enemies[e->kind].flags & TURRET) &&
           (world_systems(&g->world, world_cell(position_absolute(e->pos))) & SYSTEM_SILENCED);
}
void game_story_recovery(Game *g, Player *p, double dt) {
    if (p->sheltered || p->hurt > 0 || p->health <= 0 ||
        !(world_systems(&g->world, world_cell(position_absolute(p->pos))) & SYSTEM_POWER))
        return;
    p->health = fminf(player_max_health(p), p->health + 5 * (float)dt);
    p->shield = fminf(player_max_shield(p), p->shield + 8 * (float)dt);
    p->energy = fminf(player_max_energy(p), p->energy + 10 * (float)dt);
}
const char *expedition_title(const Game *g, unsigned scene) {
    const Story *s = &g->story;
    if (scene == STORY_RESOLUTION)
        return arcs[s->arc % STORY_ARCS];
    if (scene == STORY_CONTACT) {
        const char *names[] = {"TRACE ORISON'S HARROW SIGNAL", "LOCATE THE STRANDED SURVIVORS",
                               "TRACE THE MISSING CHART INDEX", "LOCATE THE SECONDARY BREACH",
                               "LOCATE THE ALIEN FABRICATOR"};
        return names[s->arc % STORY_ARCS];
    }
    if (scene == STORY_RELAY) {
        const char *names[] = {"RESTORE THE HARROW PARTITION RELAY",
                               "RESTORE THE REFUGE EVACUATION RELAY",
                               "RESTORE THE ARCHIVE COPY RELAY", "RESTORE THE QUARANTINE ISOLATOR",
                               "RESTORE THE FABRICATOR COOLANT LOOP"};
        return names[s->arc % STORY_ARCS];
    }
    return scene < SC_STORY_SCENES ? sc_story_templates[scene].title : "UNKNOWN COMMISSION";
}
const char *expedition_option(const Game *g, unsigned scene, unsigned choice) {
    if (choice < 1 || choice > 3)
        return "";
    if (scene == STORY_COUNCIL) {
        const char *options[] = {"PRIORITIZE EVACUATION / REFUGE GAINS A BEACON",
                                 "PRIORITIZE PRESERVATION / KEEPERS RESTORE CAMP POWER",
                                 "BROKER AN AGREEMENT / BOTH GROUPS PREPARE YOUR TARGET"};
        return options[choice - 1];
    }
    const char *options[STORY_ARCS][3] = {
        {"SEVER ORISON / FREE THE CAPTIVE CREW", "NEGOTIATE WITH ORISON / PRESERVE ITS CHARTS",
         "PARTITION ORISON / FREE THE CREW AND KEEP THE CHARTS"},
        {"EVACUATE THE AWAKE / LEAVE THE SLEEPERS", "FORTIFY THEIR REFUGE / PEOPLE STAY ABOARD",
         "EVACUATE ALL REMAINING SURVIVORS"},
        {"RECOVER THE CHART INDEX", "TRANSMIT A SURVEY / PRESERVE THE ORIGINAL",
         "RECONSTRUCT THE COMPLETE CATALOGUE"},
        {"PURGE THE BREACH / DESTROY ITS SAMPLES", "SEAL THE BREACH ABOARD",
         "ISOLATE THE BREACH / PRESERVE THE SAMPLES"},
        {"RECOVER THE ALIEN FABRICATOR CORE", "TRANSMIT ITS BLUEPRINTS / LEAVE IT ABOARD",
         "RECOVER THE CORE AND ITS PRODUCTION LIBRARY"}};
    return options[g->story.arc % STORY_ARCS][choice - 1];
}
static void debrief(Game *g, int player) {
    Player *p = &g->players[player];
    PLAYER_VIEW(g, p).journal = true;
    PLAYER_VIEW(g, p).journal_page = 3;
    PLAYER_VIEW(g, p).event_reading = 0;
}
static void settle(Game *g, int player, bool follow) {
    Story *s = &g->story;
    for (unsigned k = 0; k < SC_OUTCOMES; k++)
        s->totals[k] += s->last[k];
    for (int p = 0; p < g->player_count; p++) {
        for (unsigned k = 0; k < SC_OUTCOMES; k++)
            g->players[p].career[k] =
                (uint32_t)fmin(1000000000, (double)g->players[p].career[k] + s->last[k]);
        if (s->support[0] >= 70)
            g->players[p].credits += 100;
        if (!g->campaign.version && s->support[1] >= 70)
            g->players[p].weapons |= BIT(16);
        if (follow) {
            const unsigned pay[] = {0, 250, 180, 400};
            g->players[p].credits += pay[s->chapter_choice];
            g->players[p].victories++;
        }
    }
    s->assigned |= BIT(STORY_CONTINUE);
    s->done &= ~BIT(STORY_CONTINUE);
    s->tracked = s->chapter == 1000 ? STORY_RETURN : STORY_CONTINUE;
    expedition_record(g, follow ? STORY_RETURN : STORY_EXTRACTION, 0);
    debrief(g, player);
}
void expedition_complete(Game *g, int player, unsigned scene) {
    Story *s = &g->story;
    if (scene == STORY_EXTRACTION) {
        settle(g, player, false);
        return;
    }
    if (scene == STORY_BRIEFING)
        s->remaining = world_history(g->world.seed).population;
    if (scene == STORY_REACTOR) {
        power(g, s->sites[scene]);
        camp_power(g);
    }
    if (scene == STORY_EVAC) {
        power(g, s->sites[scene]);
        system_change(g, s->sites[scene], SYSTEM_BEACON, 0);
    }
    if (scene == STORY_MIRROR)
        support(s, 0, 5);
    expedition_record(g, scene, 0);
    if (s->done & EVIDENCE) {
        RoomSpec room = world_describe_version(g->world.seed, g->world.generator, s->sites[scene]);
        expedition_observe(g, room);
    }
}
void expedition_conclude(Game *g, unsigned choice) {
    Story *s = &g->story;
    unsigned m = story_motivation(g->world.seed);
    bool direct = world_history(g->world.seed).arc == DIRECT_COMMISSION;
    memset(s->last, 0, sizeof(s->last));
    s->last[4] = 1;
    if (m == STORY_RESCUE) {
        s->last[0] = choice == 3   ? s->remaining
                     : choice == 1 ? (s->remaining * (direct ? 3 : 2) + 3) / 4
                                   : 0;
    } else if (m == STORY_KNOWLEDGE)
        s->last[1] = choice == 3 ? 3 : 1;
    else if (m == STORY_SALVAGE)
        s->last[2] = choice == 3 ? 3 : choice == 1 ? 2 : 1;
    else {
        s->last[3] = 1;
        s->last[1] = choice == 3;
    }
    s->remaining -= s->last[0];
    bool harm = !direct && choice == 1 && (m == STORY_KNOWLEDGE || m == STORY_SALVAGE);
    support(s,
            harm          ? -25
            : choice == 3 ? 15
            : choice == 1 ? 10
                          : 0,
            m == STORY_CONTAINMENT && choice == 1 ? -15
            : harm                                ? -10
                                                  : 10);
    if (harm && m == STORY_SALVAGE && (s->assigned & BIT(STORY_COUNCIL)))
        system_change(g, s->sites[STORY_COUNCIL], SYSTEM_BLACKOUT, SYSTEM_POWER);
    expedition_record(g, game_story_target(g), choice);
}
static unsigned next_arc(const Game *g) {
    const Story *s = &g->story;
    if (s->remaining && (world_systems(&g->world, s->sites[STORY_COUNCIL]) & SYSTEM_BLACKOUT))
        return RELIEF_ARC;
    uint32_t hash = world_hash(g->world.seed,
                               (Cell){(int32_t)s->chapter, (int32_t)s->council,
                                      (int32_t)(s->choice + 7 * s->chapter_choice + 31 * s->arc)},
                               0x415243u);
    unsigned kind =
        (hash + s->support[0] + s->totals[1] + 3 * s->totals[2] + s->totals[4]) % STORY_ARCS;
    if (kind == RELIEF_ARC && !s->remaining)
        kind = CHART_ARC;
    return kind;
}
static bool guardian(Game *g, unsigned scene) {
    if (scene != STORY_RESOLUTION || g->story.arc != HARROW_ARC)
        return true;
    Room *room = world_find(&g->world, g->story.sites[scene]);
    if (room)
        for (int e = 0; e < room->enemy_count; e++)
            if (room->enemies[e].alive && room->enemies[e].kind == 26) {
                game_message(g, "ORISON'S WARDEN STILL LIVES / DEFEAT THE HEART GUARDIAN");
                return false;
            }
    return true;
}
static void interact(Game *g, int player, unsigned scene, bool remote) {
    Story *s = &g->story;
    if (!remote && (!story_clear(g, scene) || !guardian(g, scene)))
        return;
    if (scene == STORY_COUNCIL || scene == STORY_RESOLUTION) {
        Player *p = &g->players[player];
        PLAYER_INTERACTION(g, p).decision_open = PLAYER_VIEW(g, p).journal = true;
        PLAYER_VIEW(g, p).journal_page = 1;
        PLAYER_INTERACTION(g, p).decision_scene = PLAYER_VIEW(g, p).story_reading = (uint8_t)scene;
        s->seen |= BIT(scene);
        g->dirty = true;
        game_message(g, scene == STORY_COUNCIL
                            ? "REFUGE: GET US HOME / KEEPERS: SAVE THE SHIP / 1 2 3 DECIDE"
                            : "REVIEW THIS CHAPTER'S CONSEQUENCES / 1 2 3 DECIDE");
        return;
    }
    if (scene == STORY_CONTINUE) {
        s->chapter++;
        s->arc = next_arc(g);
        s->chapter_choice = 0;
        memset(s->last, 0, sizeof(s->last));
        uint32_t mask =
            BIT(STORY_CONTACT) | BIT(STORY_RELAY) | BIT(STORY_RESOLUTION) | BIT(STORY_RETURN);
        s->assigned &= ~mask;
        s->seen &= ~mask;
        s->done &= ~mask;
        for (unsigned i = STORY_CONTACT; i < SC_STORY_SCENES; i++)
            s->sites[i] = (Cell){0};
        s->done |= BIT(STORY_CONTINUE);
        s->seen |= BIT(STORY_CONTINUE);
        story_reserve(g, STORY_CONTACT, (Cell){0, 0, 4 + (int32_t)(s->chapter - 1) * 4});
        s->tracked = STORY_CONTACT;
        for (int p = 0; p < g->player_count; p++)
            g->views[p].journal = g->interactions[p].decision_open = false;
        expedition_record(g, scene, 0);
        game_message(g, arcs[s->arc]);
        return;
    }
    if (scene >= STORY_POWER && scene <= STORY_OVERRIDE) {
        uint32_t effect = rules[scene - STORY_POWER].effect;
        if (effect && !system_change(g, s->sites[scene], effect,
                                     effect == SYSTEM_POWER ? SYSTEM_BLACKOUT : 0))
            return;
        if (scene == STORY_POWER &&
            (!camp_power(g) || (has(s, STORY_BEACON) && !power(g, s->sites[STORY_BEACON]))))
            return;
        if (scene == STORY_BEACON && (s->assigned & BIT(STORY_COUNCIL)))
            if (!system_change(g, s->sites[STORY_COUNCIL], SYSTEM_BEACON, 0))
                return;
        if (scene == STORY_BEACON && has(s, STORY_POWER))
            power(g, s->sites[scene]);
    }
    if (scene == STORY_RELAY) {
        if (!power(g, s->sites[scene]) || !system_change(g, s->sites[scene], SYSTEM_BEACON, 0))
            return;
        if (s->arc == RELIEF_ARC && !camp_power(g))
            return;
    }
    story_complete(g, player, scene);
    if (scene == STORY_RETURN) {
        settle(g, player, true);
        return;
    }
    expedition_record(g, scene, 0);
    if (scene == STORY_CONTACT) {
        story_reserve(g, STORY_RESOLUTION, s->sites[scene]);
        story_reserve(g, STORY_RELAY, s->sites[scene]);
        s->tracked = STORY_RESOLUTION;
    } else if (scene == STORY_RELAY)
        s->tracked = STORY_RESOLUTION;
    else {
        support(s, scene == STORY_CIPHER ? 0 : 5, scene == STORY_CIPHER ? 10 : 0);
        RoomSpec room = world_describe_version(g->world.seed, g->world.generator, s->sites[scene]);
        expedition_observe(g, room);
        s->tracked = (s->assigned & BIT(game_story_target(g)))
                         ? game_story_target(g)
                         : STORY_REFUGE + story_motivation(g->world.seed) % 3;
    }
    char text[128];
    expedition_entry(g, scene, text, sizeof(text));
    game_message(g, text);
}
void expedition_interact(Game *g, int player, unsigned scene) {
    interact(g, player, scene, false);
}
int matrix_story_operation(Game *g, int player, Cell owner, unsigned service) {
    const unsigned scenes[][4] = {
        {STORY_REACTOR, STORY_POWER, STORY_RELAY, SC_STORY_SCENES},
        {STORY_ISOLATOR, STORY_OVERRIDE, SC_STORY_SCENES, SC_STORY_SCENES},
        {STORY_EVAC, STORY_BEACON, SC_STORY_SCENES, SC_STORY_SCENES},
        {STORY_MIRROR, STORY_CIPHER, SC_STORY_SCENES, SC_STORY_SCENES}};
    if (!g->story.version || service >= 4)
        return false;
    for (unsigned i = 0; i < 4; i++) {
        unsigned scene = scenes[service][i];
        if (scene >= SC_STORY_SCENES || !(g->story.assigned & BIT(scene)) ||
            !cell_equal(
                world_describe_version(g->world.seed, g->world.generator, g->story.sites[scene])
                    .cell,
                owner))
            continue;
        if (game_story_available(g, scene)) {
            if (scene >= STORY_POWER)
                interact(g, player, scene, true);
            else {
                story_complete(g, player, scene);
                g->story.tracked = game_story_target(g);
            }
        }
        return (g->story.done & BIT(scene)) ? 1 : -1;
    }
    return false;
}
void matrix_story_event(Game *g, Cell owner, unsigned service, unsigned choice) {
    Story *s = &g->story;
    if (s->version < 3 || !(s->done & BIT(STORY_BRIEFING)) || service > 6)
        return;
    unsigned scene = 64 + service;
    for (unsigned i = 0; i < s->event_count; i++)
        if (s->events[i].scene == scene && cell_equal(s->events[i].site, owner))
            return;
    if (s->event_count == SC_STORY_EVENTS) {
        memmove(s->events, s->events + 1, (SC_STORY_EVENTS - 1) * sizeof(*s->events));
        s->event_count--;
    }
    s->events[s->event_count++] = (StoryEvent){
        .chapter = s->chapter, .scene = scene, .choice = choice, .arc = s->arc, .site = owner};
    if (service == 6)
        s->events[s->event_count - 1].outcome[1] = choice == 3 ? 3 : choice == 1 ? 1 : 0;
    g->dirty = true;
}
bool expedition_choose(Game *g, int player, unsigned scene, unsigned choice) {
    Story *s = &g->story;
    if (player < 0 || player >= g->player_count ||
        (scene != STORY_COUNCIL && scene != STORY_RESOLUTION) || choice < 1 || choice > 3 ||
        !g->interactions[player].decision_open || g->interactions[player].decision_scene != scene)
        return false;
    if (choice == 3 && (scene == STORY_COUNCIL ? !(has(s, STORY_POWER) && has(s, STORY_CIPHER))
                                               : !has(s, STORY_RELAY))) {
        game_message(g, scene == STORY_COUNCIL
                            ? "AGREEMENT NEEDS MICROGRID POWER AND THE CONTROLLER CIPHER"
                            : "THIRD OUTCOME NEEDS THIS CHAPTER'S OPTIONAL RELAY");
        return false;
    }
    if (!story_clear(g, scene) || !guardian(g, scene))
        return false;
    if (scene == STORY_COUNCIL) {
        if (choice == 1 && !system_change(g, s->sites[scene], SYSTEM_BEACON, 0))
            return false;
        if (choice >= 2 && !power(g, s->sites[scene]))
            return false;
        if (choice == 3 && !system_change(g, s->sites[scene], SYSTEM_BEACON, 0))
            return false;
        s->council = choice;
        s->tracked = (s->assigned & BIT(game_story_target(g)))
                         ? game_story_target(g)
                         : STORY_REFUGE + story_motivation(g->world.seed) % 3;
        support(s,
                choice == 1   ? 20
                : choice == 2 ? -10
                              : 15,
                choice == 2   ? 20
                : choice == 1 ? -10
                              : 15);
    } else {
        memset(s->last, 0, sizeof(s->last));
        s->last[4] = 1;
        s->chapter_choice = choice;
        if (s->arc == RELIEF_ARC || s->arc == HARROW_ARC) {
            s->last[0] = choice == 3 ? s->remaining : choice == 1 ? (s->remaining + 1) / 2 : 0;
            s->remaining -= s->last[0];
            if (s->arc == HARROW_ARC) {
                s->last[1] = choice == 3 ? 2 : choice == 2 ? 1 : 0;
                s->last[3] = choice != 2;
            }
        } else if (s->arc == CHART_ARC)
            s->last[1] = choice == 3 ? 3 : 1;
        else if (s->arc == FABRICATOR_ARC)
            s->last[2] = choice == 3 ? 3 : choice == 1 ? 2 : 1;
        else {
            s->last[3] = 1;
            s->last[1] = choice == 3;
        }
        if (s->arc == HARROW_ARC && choice != 2)
            system_change(g, s->sites[scene], SYSTEM_SILENCED, 0);
        if (s->arc == QUARANTINE_ARC && choice == 1) {
            Room *room = world_find(&g->world, s->sites[scene]);
            if (room)
                for (int i = 0; i < room->enemy_count; i++) {
                    bool alive = room->enemies[i].alive;
                    world_kill(&g->world, room, i);
                    if (alive && !room->enemies[i].alive)
                        game_enemy_death(g, &room->enemies[i], player);
                }
        }
        support(s, choice == 3 ? 12 : choice == 1 ? 5 : 0, choice == 1 ? -5 : 10);
        s->assigned |= BIT(STORY_RETURN);
        s->tracked = STORY_RETURN;
    }
    story_complete(g, player, scene);
    expedition_record(g, scene, choice);
    for (int p = 0; p < g->player_count; p++)
        g->interactions[p].decision_open = g->views[p].journal = false;
    if (scene == STORY_COUNCIL)
        game_message(g,
                     choice == 3
                         ? "REFUGE AND KEEPERS AGREE / THEIR ASSISTANCE UNLOCKS YOUR THIRD OUTCOME"
                         : "COUNCIL DECISION RECORDED / FACTION SUPPORT AND SHIP SYSTEMS CHANGED");
    else
        game_message(g, "CHAPTER DECISION RECORDED / RETURN TO THE BAY / F7 DEBRIEF");
    return true;
}
void expedition_entry(const Game *g, unsigned scene, char *text, size_t size) {
    const Story *s = &g->story;
    const char *entries[] = {
        "REFUGE: THE MICROGRID IS LIVE. POWERED ROOMS IMPROVE RECOVERY AND LIGHTING.",
        "REFUGE: BEACON SECURED. POWER ITS ROOM TO ACTIVATE THE SAFE WARD.",
        "KEEPERS: THE CIPHER IDENTIFIES A CONTROLLER CONSOLE. IT CAN ALSO BROKER OUR AGREEMENT.",
        "REFUGE: BRING US HOME. KEEPERS: PRESERVE OUR SHIP. POWER AND EVIDENCE CAN SATISFY BOTH.",
        "KEEPERS: CONTROLLER OFFLINE. LOCAL TURRETS WILL NO LONGER TARGET YOU.",
        "THE BAY TRANSMITTER OFFERS ANOTHER COMMISSION BASED ON YOUR ACTIONS ABOARD.",
        "A WITNESS IDENTIFIES THE CHAPTER TARGET AND ITS OPTIONAL PREPARATION RELAY.",
        ("THE RELAY IS READY. ITS POWERED BEACON IS SAFE AND THE THIRD CHAPTER OUTCOME IS "
         "AVAILABLE."),
        "THE CHAPTER TARGET IS READY FOR YOUR DECISION. ITS GUARDIAN, IF ANY, MUST BE DEFEATED.",
        "CHAPTER EXTRACTION RECORDED. THE SHIP RETAINS YOUR CHOICES AND CAN OFFER ANOTHER JOB."};
    if (scene == STORY_CONTACT || scene == STORY_RESOLUTION)
        snprintf(text, size, "%s: %s", arcs[s->arc % STORY_ARCS], entries[scene - STORY_POWER]);
    else
        snprintf(text, size, "%s",
                 scene >= STORY_POWER && scene < SC_STORY_SCENES ? entries[scene - STORY_POWER]
                                                                 : "NO CHAPTER RECORD");
}
void game_story_event_read(Game *g, int player) {
    if (player < 0 || player >= g->player_count || !g->story.event_count)
        return;
    g->views[player].event_reading =
        (uint8_t)((g->views[player].event_reading + 1) % g->story.event_count);
}
void game_story_debrief(const Game *g, int player, int line, char *text, size_t size) {
    const Story *s = &g->story;
    if (s->version < 3) {
        snprintf(text, size, "ARCHIVAL EXPEDITION / YOUR ORIGINAL JOURNAL RETAINS ITS CONCLUSION");
        return;
    }
    bool settled = s->chapter ? has(s, STORY_RETURN) : has(s, STORY_EXTRACTION);
    if (line == 0)
        snprintf(text, size, "%s / CHAPTER %u / %s", story_ship(g->world.seed), s->chapter,
                 settled ? "EXTRACTED" : "IN PROGRESS");
    else if (line == 1)
        snprintf(text, size, "%s",
                 s->chapter_choice ? expedition_option(g, STORY_RESOLUTION, s->chapter_choice)
                 : s->chapter      ? "CHAPTER IN PROGRESS / PRIOR RESULTS ARE IN THE EVENT HISTORY"
                 : s->choice       ? game_story_choice(g, s->choice)
                                   : "NO FINAL DECISION RECORDED");
    else if (line == 2)
        snprintf(text, size,
                 "THIS OUTCOME / %u RESCUED / %u KNOWLEDGE / %u TECHNOLOGY / %u THREATS CONTAINED",
                 s->last[0], s->last[1], s->last[2], s->last[3]);
    else if (line == 3)
        snprintf(text, size,
                 "SHIP TOTAL / %u RESCUED / %u STILL ABOARD / %u KNOWLEDGE / %u TECHNOLOGY / %u "
                 "CONTAINED",
                 s->totals[0], s->remaining, s->totals[1], s->totals[2], s->totals[3]);
    else if (line == 4)
        snprintf(text, size, "REFUGE SUPPORT %u/100 / %s", s->support[0],
                 world_history_agenda(g->world.seed));
    else if (line == 5)
        snprintf(text, size, "KEEPERS SUPPORT %u/100 / PRESERVE THE SHIP AND ITS LIVING SYSTEMS",
                 s->support[1]);
    else if (line == 6) {
        uint32_t flags = world_systems(&g->world, s->sites[STORY_COUNCIL]);
        if (g->world.generator >= 16) {
            unsigned repaired = 0, left = 0, lost = 0;
            for (unsigned i = 0; i < g->chronicle.count; i++) {
                const RecurringPerson *person = &g->chronicle.people[i];
                repaired += (person->knowledge & PERSON_HELPED) != 0;
                left += person->fate == PERSON_LEFT;
                lost += person->fate == PERSON_LOST && (person->knowledge & PERSON_SEEN);
            }
            snprintf(text, size, "REFUGES / %u RESTORED / %u EVACUATED / %u KNOWN LOST / MAIN %s",
                     repaired, left, lost, flags & SYSTEM_BLACKOUT ? "OFFLINE" : "POWERED");
            return;
        }
        snprintf(text, size, "REFUGE SYSTEMS / %s / %s",
                 flags & SYSTEM_BLACKOUT ? "LIFE SUPPORT OFFLINE"
                 : flags & SYSTEM_POWER  ? "POWER RESTORED"
                                         : "EMERGENCY POWER",
                 (flags & (SYSTEM_POWER | SYSTEM_BEACON)) == (SYSTEM_POWER | SYSTEM_BEACON)
                     ? "BEACON WARD ACTIVE"
                     : "WARD NOT POWERED");
    } else if (line == 7) {
        const Player *p = &g->players[player >= 0 && player < g->player_count ? player : 0];
        snprintf(text, size,
                 "CAREER / %u ARCS / %u RESCUED / %u KNOWLEDGE / %u TECHNOLOGY / %u CONTAINED",
                 p->career[4], p->career[0], p->career[1], p->career[2], p->career[3]);
    } else if (line == 8 && s->event_count) {
        unsigned n = player >= 0 && player < g->player_count ? g->views[player].event_reading : 0;
        const StoryEvent *e = &s->events[s->event_count - 1 - n % s->event_count];
        snprintf(
            text, size,
            "EVENT %u / CHAPTER %u / %s / %s [%d,%d,%d] / CHOICE %u / RESCUED %u / KNOWLEDGE "
            "%u / TECH %u / CONTAINED %u",
            s->event_count - n % s->event_count, e->chapter,
            e->chapter ? arcs[e->arc] : "COMMISSION",
            e->scene >= 64
                ? (e->scene == 70
                       ? matrix_controller_name(
                             world_hash(g->world.seed, matrix_region(e->site), 0x4d415458u) % 4)
                       : sc_matrix_services[e->scene - 64])
                : sc_story_templates[e->scene].title,
            e->site.x, e->site.y, e->site.z, e->choice, e->outcome[0], e->outcome[1], e->outcome[2],
            e->outcome[3]);
    } else
        snprintf(text, size, "%s",
                 settled && s->chapter < 1000
                     ? "G AT THE BAY: ACCEPT ANOTHER COMMISSION / N THEN ENTER: NEW SHIP"
                     : "FOLLOW THE GOLD MAP LEAD / EXTRACT TO RECORD CAREER REWARDS");
}

bool expedition_valid(const Story *s, uint32_t seed, uint32_t generator) {
    if (s->version < 3) {
        Story empty = {0};
        return memcmp(s->support, empty.support, sizeof(*s) - offsetof(Story, support)) == 0;
    }
    if (s->support[0] > 100 || s->support[1] > 100 || s->council > 3 || s->chapter > 1000 ||
        s->arc >= STORY_ARCS || s->chapter_choice > 3 || s->event_count > SC_STORY_EVENTS)
        return false;
    bool started = has(s, STORY_BRIEFING), extracted = has(s, STORY_EXTRACTION);
    if (!started && (s->remaining || s->event_count || s->support[0] != 50 || s->support[1] != 50))
        return false;
    if ((s->council != 0) != has(s, STORY_COUNCIL) ||
        (s->council == 3 && !(has(s, STORY_POWER) && has(s, STORY_CIPHER))))
        return false;
    for (unsigned i = STORY_POWER; i <= STORY_OVERRIDE; i++)
        if (has(s, i) && (s->done & rules[i - STORY_POWER].needs) != rules[i - STORY_POWER].needs)
            return false;
    if ((s->assigned & BIT(STORY_OVERRIDE)) && !has(s, STORY_CIPHER))
        return false;
    if (((s->assigned & BIT(STORY_CONTINUE)) != 0) != extracted || (s->chapter && !extracted))
        return false;
    if (!s->chapter) {
        if (s->arc || s->chapter_choice ||
            (s->assigned &
             (BIT(STORY_CONTACT) | BIT(STORY_RELAY) | BIT(STORY_RESOLUTION) | BIT(STORY_RETURN))))
            return false;
    } else {
        if (!(s->assigned & BIT(STORY_CONTACT)) ||
            ((s->assigned & BIT(STORY_RELAY)) != 0) != has(s, STORY_CONTACT) ||
            ((s->assigned & BIT(STORY_RESOLUTION)) != 0) != has(s, STORY_CONTACT) ||
            ((s->assigned & BIT(STORY_RETURN)) != 0) != (s->chapter_choice != 0) ||
            (s->chapter_choice != 0) != has(s, STORY_RESOLUTION) ||
            (s->chapter_choice == 3 && !has(s, STORY_RELAY)) ||
            has(s, STORY_CONTINUE) == has(s, STORY_RETURN))
            return false;
        if (s->arc == HARROW_ARC && (s->assigned & BIT(STORY_RESOLUTION)) &&
            !cell_equal(s->sites[STORY_RESOLUTION], (Cell){2, 1, 2}))
            return false;
        for (unsigned i = STORY_CONTACT; i <= STORY_RESOLUTION; i++)
            if ((s->assigned & BIT(i)) && !(i == STORY_RESOLUTION && s->arc == HARROW_ARC)) {
                Cell c = s->sites[i];
                int base = 4 + (int)(s->chapter - 1) * 4;
                if (c.x < 0 || c.x > 7 || c.y < -1 || c.y > 1 || c.z < base || c.z >= base + 8)
                    return false;
            }
    }
    unsigned arcs_done = extracted ? s->chapter + (!s->chapter || has(s, STORY_RETURN) ? 1 : 0) : 0;
    if (s->totals[4] != arcs_done)
        return false;
    bool decided = s->chapter ? s->chapter_choice != 0 : s->choice != 0;
    bool pending = s->chapter ? !has(s, STORY_RETURN) : !extracted;
    unsigned people = world_history(seed).population;
    if (s->remaining > people || s->totals[0] > people || s->last[0] > people ||
        (started && s->remaining + s->totals[0] + (pending ? s->last[0] : 0) != people))
        return false;
    for (unsigned k = 0; k < SC_OUTCOMES; k++)
        if (s->last[k] > (k == 0   ? people
                          : k == 4 ? 1
                                   : 3) ||
            s->totals[k] > (k == 0 ? people : 3003) || (!decided && s->last[k]))
            return false;
    if (s->last[4] != (decided ? 1u : 0u))
        return false;
    for (unsigned i = 0; i < SC_STORY_EVENTS; i++) {
        const StoryEvent *e = &s->events[i];
        if (i >= s->event_count) {
            StoryEvent empty = {0};
            if (memcmp(e, &empty, sizeof(*e)))
                return false;
            continue;
        }
        if ((e->scene >= SC_STORY_SCENES && (e->scene < 64 || e->scene > 70)) || e->choice > 3 ||
            e->chapter > s->chapter || e->arc >= STORY_ARCS ||
            (i && e->chapter < s->events[i - 1].chapter))
            return false;
        Cell c = e->site;
        if (c.x < -10000000 || c.x > 10000000 || c.y < -4000000 || c.y > 4000000 || c.z < 0 ||
            c.z > 10000000)
            return false;
        if (e->scene == STORY_BRIEFING || e->scene == STORY_EXTRACTION ||
            e->scene == STORY_CONTINUE || e->scene == STORY_RETURN) {
            if (!cell_equal(c, (Cell){0}))
                return false;
        } else {
            RoomSpec r = world_describe_version(seed, generator, c);
            if ((e->scene < 64 && (r.type == BAY || r.height < 3)) ||
                (!cell_equal(c, r.cell) && !(e->scene == STORY_RESOLUTION && e->arc == HARROW_ARC &&
                                             cell_equal(c, (Cell){2, 1, 2}))))
                return false;
        }
        bool decision = e->scene == 70 || e->scene == STORY_COUNCIL ||
                        e->scene == STORY_RESOLUTION || e->scene == STORY_SURVIVORS ||
                        e->scene == STORY_MEMORY || e->scene == STORY_DRIVE ||
                        e->scene == STORY_SOURCE;
        if ((e->choice != 0) != decision)
            return false;
        for (unsigned k = 0; k < 4; k++)
            if (e->outcome[k] > (k == 0 ? people : 3))
                return false;
        for (unsigned j = 0; j < i; j++)
            if (s->events[j].chapter == e->chapter && s->events[j].scene == e->scene &&
                (e->scene < 64 || cell_equal(s->events[j].site, e->site)))
                return false;
    }
    return true;
}
