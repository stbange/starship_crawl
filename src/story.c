#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIT(n) (UINT32_C(1) << (n))
#define EVIDENCE (BIT(STORY_REFUGE) | BIT(STORY_ARCHIVE) | BIT(STORY_LEDGER))
#define CLUES UINT32_C(62)
#define HISTORY_RECORDS                                                                            \
    (BIT(STORY_ORIGIN_RECORD) | BIT(STORY_SURVIVOR_RECORD) | BIT(STORY_CONTROL_RECORD))
#define PASSIVE (CLUES | HISTORY_RECORDS)
#define TARGETS (BIT(STORY_SURVIVORS) | BIT(STORY_MEMORY) | BIT(STORY_DRIVE) | BIT(STORY_SOURCE))

const StoryTemplate sc_story_templates[SC_STORY_SCENES] = {
    {"RECEIVE THE COMMISSION", BAY, SANCTUM},
    {"RECENT BOOTPRINTS", CREW, HABITAT},
    {"SPLICED POWER CABLES", ENGINE, MACHINERY},
    {"THAWED IDENTIFICATION TAGS", CREW, CRYONICS},
    {"SCRUBBED SPECIMEN LABELS", ENGINE, RESEARCH},
    {"A WARM RATION SHRINE", GARDEN, LUXURY},
    {"SECURE THE REFUGE SIGNAL", CREW, HABITAT},
    {"RECOVER THE WITNESS ARCHIVE", NAVE, RESEARCH},
    {"READ THE CONTAINMENT LEDGER", ENGINE, LOGISTICS},
    {"REPAIR THE BACKUP REACTOR", ENGINE, MACHINERY},
    {"SECURE THE EVACUATION RELAY", GARDEN, COMMAND},
    {"BUILD A MEMORY MIRROR", NAVE, RESEARCH},
    {"REACH THE SURVIVOR VAULT", CREW, CRYONICS},
    {"REACH THE LIVING ARCHIVE", NAVE, RESEARCH},
    {"REACH THE ALIEN DRIVE", ENGINE, MACHINERY},
    {"RETURN TO THE EXTRACTION BEACON", BAY, SANCTUM},
    {"CONTAIN THE CATASTROPHE SOURCE", ENGINE, RESEARCH},
    {"RESTORE THE ISOLATION GRID", ENGINE, COMMAND},
    {"THE COLLAPSE ORIGIN", ENGINE, MACHINERY},
    {"A SURVIVOR NOTICE", CREW, HABITAT},
    {"THE CONTROLLER'S SIGNATURE", NAVE, COMMAND},
    {"RESTORE THE REFUGE MICROGRID", ENGINE, MACHINERY},
    {"SECURE THE ROUTE BEACON", GARDEN, HABITAT},
    {"RECOVER THE CONTROLLER CIPHER", NAVE, RESEARCH},
    {"MEET THE SURVIVOR COUNCIL", CREW, HABITAT},
    {"DISABLE THE LOCAL CONTROLLER", NAVE, COMMAND},
    {"ACCEPT ANOTHER COMMISSION AT THE BAY", BAY, SANCTUM},
    {"INVESTIGATE THE FOLLOW-ON SIGNAL", CREW, HABITAT},
    {"RESTORE THE FOLLOW-ON RELAY", ENGINE, MACHINERY},
    {"RESOLVE THE FOLLOW-ON TARGET", NAVE, RESEARCH},
    {"RETURN TO THE BAY / CHAPTER EXTRACTION", BAY, SANCTUM}};
static unsigned history(uint32_t seed, unsigned salt, unsigned count) {
    return world_hash(seed, (Cell){0, 0, 0}, UINT32_C(0x53540000) + salt) % count;
}
StoryMotivation story_motivation_version(uint32_t seed, unsigned version) {
    return (StoryMotivation)history(seed, 1, version < 2 ? 3 : 4);
}
StoryMotivation story_motivation(uint32_t seed) {
    return story_motivation_version(seed, 3);
}
static StoryMotivation motivation(const Game *g) {
    return story_motivation_version(g->world.seed, g->story.version);
}
static bool direct(const Game *g) {
    return g->story.version >= 2 && world_history(g->world.seed).arc == DIRECT_COMMISSION;
}
const char *story_ship(uint32_t seed) {
    static const char *names[] = {"SAINT VESPER", "MOURNING GLASS", "THE NINTH MERCY",
                                  "CROWN OF ASH", "PALE ORCHARD",   "VESSEL OF HOURS",
                                  "BLACK LITANY", "LAST DAWN"};
    return names[history(seed, 2, 8)];
}
const char *story_commission_version(uint32_t seed, unsigned version) {
    static const char *names[] = {"RESCUE / BRING THE LOST HOME",
                                  "KNOWLEDGE / RECOVER A LOST STAR CHART",
                                  "SALVAGE / RECOVER AN ALIEN DRIVE",
                                  "CONTAINMENT / KEEP THE INFESTATION FROM REACHING HOME"};
    return names[story_motivation_version(seed, version)];
}
const char *story_commission(uint32_t seed) {
    return story_commission_version(seed, 3);
}
static const char *survivors(uint32_t seed) {
    static const char *groups[] = {"PILGRIMS", "PASSENGERS", "PRISONERS", "SEED CUSTODIANS"};
    return groups[world_profile(seed)];
}
static const char *cause(uint32_t seed) {
    static const char *causes[] = {"A REACTOR CASCADE BROKE CONTAINMENT",
                                   "AN INFECTED CARGO BREACHED QUARANTINE",
                                   "A DEFENSE PURGE STRUCK THE LIFE SUPPORT GRID"};
    return causes[history(seed, 3, 3)];
}
static unsigned target(uint32_t seed, unsigned version) {
    const unsigned scenes[] = {STORY_SURVIVORS, STORY_MEMORY, STORY_DRIVE, STORY_SOURCE};
    return scenes[story_motivation_version(seed, version)];
}
static unsigned optional(uint32_t seed, unsigned version) {
    const unsigned scenes[] = {STORY_EVAC, STORY_MIRROR, STORY_REACTOR, STORY_ISOLATOR};
    return scenes[story_motivation_version(seed, version)];
}
unsigned game_story_target(const Game *g) {
    return g->story.version >= 3 && g->story.chapter ? STORY_RESOLUTION
                                                     : target(g->world.seed, g->story.version);
}
unsigned game_story_operation(const Game *g) {
    return g->story.version >= 3 && g->story.chapter ? STORY_RELAY
                                                     : optional(g->world.seed, g->story.version);
}
bool game_story_prepared(const Game *g) {
    if (g->story.version >= 3 && g->story.chapter)
        return (g->story.done & BIT(STORY_RELAY)) != 0;
    return (g->story.done & BIT(game_story_operation(g))) != 0 ||
           (g->story.version >= 3 && g->story.council == 3);
}
const char *game_story_title(const Game *g, unsigned scene) {
    if (g->story.version >= 3 && scene >= STORY_POWER)
        return expedition_title(g, scene);
    if (direct(g)) {
        if (scene == STORY_REACTOR)
            return "RESTART THE ALIEN FABRICATOR";
        if (scene == STORY_EVAC)
            return "REACH THE REMOTE SLEEPER RELAY";
        if (scene == STORY_MIRROR)
            return "REBUILD THE DAMAGED CATALOGUE";
    }
    return scene < SC_STORY_SCENES ? sc_story_templates[scene].title : "UNKNOWN RECORD";
}
static unsigned progress(const Story *s) {
    if (s->done & BIT(STORY_EXTRACTION))
        return 6;
    unsigned count = 0;
    for (int i = 0; i < SC_STORY_SCENES; i++)
        count += (s->done >> i) & 1;
    return count < 5 ? count : 5;
}
void story_init(Story *s) {
    memset(s, 0, sizeof(*s));
    s->version = 3;
    s->support[0] = s->support[1] = 50;
    s->assigned = BIT(STORY_BRIEFING);
}
static bool available(const Story *s, unsigned scene) {
    if (s->version >= 3 && scene >= STORY_POWER)
        return expedition_available(s, scene);
    return scene < SC_STORY_SCENES && (s->assigned & BIT(scene)) && !(s->done & BIT(scene)) &&
           !(PASSIVE & BIT(scene)) && (!s->choice || scene == STORY_EXTRACTION);
}
bool game_story_available(const Game *g, unsigned scene) {
    return g->story.version && available(&g->story, scene);
}
/* Narrative props occupy service positions on the protected circulation cross.
   Architecture/sector preferences select context; every tall room can house a
   refuge transmitter, archive cabinet or repair console without changing walls. */
static bool site_room(RoomSpec room) {
    return room.type != BAY && room.height >= 3;
}
static unsigned clue(RoomSpec room) {
    if (room.incident == FROST || room.district == CRYONICS)
        return STORY_FROST;
    if (room.district == RESEARCH)
        return STORY_RESEARCH;
    if (room.type == ENGINE || room.incident == BLACKOUT)
        return STORY_CONDUIT;
    return room.type == CREW ? STORY_TRACKS : STORY_RATIONS;
}
void story_reserve(Game *g, unsigned scene, Cell from) {
    Story *s = &g->story;
    if (s->assigned & BIT(scene))
        return;
    ShipHistory past = world_history(g->world.seed);
    if (scene == STORY_SOURCE) {
        s->sites[scene] = past.origin;
        s->assigned |= BIT(scene);
        return;
    }
    if (scene == STORY_RESOLUTION && s->arc == HARROW_ARC) {
        s->sites[scene] = (Cell){2, 1, 2};
        s->assigned |= BIT(scene);
        return;
    }
    Cell best = {0};
    double score = 1e9;
    /* Bounded introductory catchment; all cells have lifts and connected doors.
       Logical-room owners avoid putting two operations in one multi-chunk hall. */
    int zbase = scene >= STORY_CONTACT ? 4 + (int)(s->chapter - 1) * 4 : 0;
    for (int y = -1; y <= 1; y++)
        for (int z = zbase; z < zbase + (scene >= STORY_CONTACT ? 8 : 4); z++)
            for (int x = 0; x < (scene >= STORY_CONTACT ? 8 : 4); x++) {
                Cell c = {x, y, z};
                RoomSpec room = world_describe_version(g->world.seed, g->world.generator, c);
                if (!site_room(room) || !cell_equal(c, room.cell) ||
                    (scene == STORY_OVERRIDE &&
                     cell_equal(c, world_describe_version(g->world.seed, g->world.generator,
                                                          (Cell){2, 1, 2})
                                       .cell)))
                    continue;
                bool used = motivation(g) == STORY_CONTAINMENT && cell_equal(c, past.origin);
                for (int i = scene >= STORY_CONTACT ? STORY_CONTACT : STORY_REFUGE;
                     i < (scene >= STORY_CONTACT ? SC_STORY_SCENES : STORY_CONTINUE); i++)
                    used |= i != STORY_EXTRACTION && !(PASSIVE & BIT(i)) &&
                            (s->assigned & BIT(i)) &&
                            cell_equal(scene >= STORY_CONTACT
                                           ? world_describe_version(g->world.seed,
                                                                    g->world.generator, s->sites[i])
                                                 .cell
                                           : s->sites[i],
                                       c);
                if (used)
                    continue;
                StoryTemplate desired = sc_story_templates[scene];
                if (scene == STORY_RESOLUTION) {
                    const RoomType types[] = {NAVE, CREW, NAVE, ENGINE, ENGINE};
                    const District sectors[] = {COMMAND, CRYONICS, RESEARCH, RESEARCH, MACHINERY};
                    desired.architecture = types[s->arc % STORY_ARCS];
                    desired.sector = sectors[s->arc % STORY_ARCS];
                }
                const StoryTemplate *def = &desired;
                double value = (room.type == def->architecture ? 0 : 70) +
                               (room.district == def->sector ? 0 : 25) +
                               7 * (abs(c.x - from.x) + abs(c.z - from.z) + 2 * abs(c.y - from.y)) +
                               world_hash(g->world.seed, c, 0x7100u + scene) % 61;
                if (value < score) {
                    score = value;
                    best = c;
                }
            }
    s->sites[scene] = best;
    s->assigned |= BIT(scene);
}
void game_story_track(Game *g) {
    if (!g->story.version)
        return;
    for (unsigned n = 1; n <= SC_STORY_SCENES; n++) {
        unsigned at = (g->story.tracked + n) % SC_STORY_SCENES;
        if (available(&g->story, at)) {
            g->story.tracked = at;
            g->dirty = true;
            game_message(g, game_story_title(g, at));
            return;
        }
    }
}
void game_story_step(Game *g) {
    Story *s = &g->story;
    if (!s->version || (s->choice && s->version == 1))
        return;
    for (int p = 0; p < g->player_count; p++) {
        Player *player = &g->players[p];
        Vec3 feet = position_absolute(player->pos);
        if (player->sheltered || feet.z < 0)
            continue;
        Room *room = world_find(&g->world, world_cell(feet));
        if (!room || !room->ready)
            continue;
        for (int i = 0; i < SC_STORY_SCENES; i++)
            if ((s->assigned & BIT(i)) && !(s->seen & BIT(i)) &&
                vlen(vsub(feet, game_terminal(s->sites[i]))) < 10) {
                s->seen |= BIT(i);
                g->dirty = true;
            }
        RoomSpec spec = world_describe_version(g->world.seed, g->world.generator, room->cell);
        if (!(s->done & BIT(STORY_BRIEFING)) || !site_room(spec))
            continue;
        if (s->version >= 2 && spec.history_role) {
            unsigned record = STORY_ORIGIN_RECORD + spec.history_role - 1;
            if (!(s->seen & BIT(record))) {
                s->sites[record] = spec.cell;
                s->assigned |= BIT(record);
                s->seen |= BIT(record);
                if (s->version >= 3)
                    expedition_record(g, record, 0);
                g->dirty = true;
                game_message(g, "HISTORY NOTICE RECORDED / F6 SHIP DOSSIER");
            }
        }
        if (s->version >= 3)
            expedition_observe(g, spec);
        if (s->choice)
            continue;
        unsigned found = clue(spec);
        if (s->seen & BIT(found))
            continue;
        s->sites[found] = room->cell;
        s->assigned |= BIT(found);
        s->seen |= BIT(found);
        if (s->version >= 3)
            expedition_record(g, found, 0);
        if (s->version >= 3)
            expedition_observe(g, spec);
        story_reserve(g, STORY_REFUGE + (motivation(g) + 1) % 3, room->cell);
        g->dirty = true;
        char observation[128];
        snprintf(observation, sizeof(observation), "%s / NEW EVIDENCE IN I JOURNAL / U TRACK LEADS",
                 sc_story_templates[found].title);
        game_message(g, observation);
    }
}
unsigned game_story_near(const Game *g, int player) {
    if (!g->story.version || player < 0 || player >= g->player_count ||
        g->players[player].sheltered)
        return SC_STORY_SCENES;
    Vec3 feet = position_absolute(g->players[player].pos);
    for (unsigned i = 0; i < SC_STORY_SCENES; i++)
        if (available(&g->story, i) && world_ready(&g->world, g->story.sites[i]) &&
            vlen(vsub(feet, game_terminal(g->story.sites[i]))) <= 4)
            return i;
    return SC_STORY_SCENES;
}
bool story_clear(Game *g, unsigned scene) {
    if (scene == STORY_BRIEFING || scene == STORY_EXTRACTION || scene == STORY_CONTINUE ||
        scene == STORY_RETURN)
        return true;
    Vec3 site = game_terminal(g->story.sites[scene]);
    for (int r = 0; r < g->world.count; r++)
        for (int i = 0; i < g->world.rooms[r].enemy_count; i++) {
            Enemy *e = &g->world.rooms[r].enemies[i];
            Vec3 eye = vadd(site, v3(0, 1.5, 0));
            Vec3 delta = vsub(enemy_center(e), eye);
            double distance = vlen(delta);
            /* A creature behind a sealed partition cannot contest this console. */
            if (e->alive && distance < 12 &&
                world_raycast(&g->world, eye, vnorm(delta), distance) >= distance - .1) {
                game_message(g, "SITE CONTESTED / CLEAR NEARBY INFESTATION");
                return false;
            }
        }
    return true;
}
void story_complete(Game *g, int player, unsigned scene) {
    Story *s = &g->story;
    s->done |= BIT(scene);
    s->seen |= BIT(scene);
    g->mission = progress(s);
    for (int i = 0; i < g->player_count; i++) {
        Player *p = &g->players[i];
        p->points = (uint32_t)fmin(1000, p->points + 2);
        p->credits += 75;
        p->health = player_max_health(p);
        p->shield = player_max_shield(p);
        p->energy = player_max_energy(p);
        p->checkpoint = g->players[player].pos;
        p->weapons |= BIT(scene == STORY_BRIEFING ? 1 : 4 + motivation(g));
        if (scene == STORY_EXTRACTION) {
            const unsigned pay[] = {0, 250, 180, 400};
            p->credits += pay[s->choice];
            p->victories++;
            p->weapons |= BIT(15);
        }
    }
    g->dirty = true;
    if (s->version >= 3 && scene < STORY_POWER && !(TARGETS & BIT(scene)))
        expedition_complete(g, player, scene);
}
void game_story_interact(Game *g, int player) {
    unsigned scene = game_story_near(g, player);
    if (scene == SC_STORY_SCENES) {
        game_message(g, "FOLLOW A KNOWN LEAD / I JOURNAL / U CHANGE MAP TARGET");
        return;
    }
    if (g->story.version >= 3 && scene >= STORY_POWER) {
        expedition_interact(g, player, scene);
        return;
    }
    if (!story_clear(g, scene))
        return;
    Story *s = &g->story;
    if (TARGETS & BIT(scene)) {
        Player *p = &g->players[player];
        p->decision_open = p->journal = true;
        p->journal_page = 1;
        p->story_reading = p->decision_scene = (uint8_t)scene;
        s->seen |= BIT(scene);
        g->dirty = true;
        game_message(g, "REVIEW THE CONSEQUENCES / 1 2 3 CHOOSE / ENTER LEAVES UNDECIDED");
        return;
    }
    story_complete(g, player, scene);
    if (scene == STORY_BRIEFING) {
        unsigned lead = STORY_REFUGE + motivation(g) % 3;
        story_reserve(g, lead, (Cell){0, 0, 0});
        s->tracked = lead;
    } else if (EVIDENCE & BIT(scene)) {
        unsigned goal = game_story_target(g);
        story_reserve(g, goal, s->sites[scene]);
        story_reserve(g, game_story_operation(g), s->sites[scene]);
        s->tracked = goal;
    } else if (scene != STORY_EXTRACTION)
        s->tracked = game_story_target(g);
    char message[128];
    game_story_entry(g, scene, message, sizeof(message));
    game_message(g, message);
}
static const char *primary_choice(const Game *g, unsigned choice) {
    static const char *options[4][3] = {
        {"EVACUATE THE AWAKE / SLEEPERS STAY BEHIND", "FORTIFY THE REFUGE / ALL STAY ABOARD",
         "EVACUATE EVERYONE / ALL SURVIVORS GO HOME"},
        {"TAKE THE ARCHIVE / REFUGE LOSES ITS NAVIGATOR",
         "TRANSMIT A SURVEY / KEEP THE ARCHIVE ABOARD",
         "COPY THE FULL ARCHIVE / KEEP THE NAVIGATOR ABOARD"},
        {"TAKE THE DRIVE / REFUGE LOSES POWER", "LEAVE THE DRIVE / PRESERVE THE REFUGE",
         "TAKE THE DRIVE SAFELY / REFUGE KEEPS ITS POWER"},
        {"PURGE THE SOURCE / DESTROY ITS RESEARCH", "SEAL THE SOURCE / LEAVE IT ABOARD",
         "ISOLATE THE SOURCE / PRESERVE THE RESEARCH"}};
    static const char *simple[3][3] = {
        {"EVACUATE THE REFUGE / EVERYONE HERE GOES HOME", "ESTABLISH A REFUGE / CREW STAYS ABOARD",
         "EVACUATE BOTH GROUPS / RESCUE THE REMOTE SLEEPERS TOO"},
        {"COPY THE CHART / COMPLETE THE COMMISSION", "TRANSMIT THE CHART / LEAVE THE ORIGINAL",
         "COPY THE FULL CATALOGUE / RECOVER THE LOST ROUTES TOO"},
        {"TAKE THE SPARE DRIVE / COMPLETE THE COMMISSION",
         "TRANSMIT ITS BLUEPRINTS / LEAVE THE DRIVE",
         "RECOVER DRIVE AND FABRICATOR / BRING HOME MORE TECHNOLOGY"}};
    if (direct(g) && motivation(g) < STORY_CONTAINMENT && choice >= 1 && choice <= 3)
        return simple[motivation(g)][choice - 1];
    return choice >= 1 && choice <= 3 ? options[motivation(g)][choice - 1] : "";
}
const char *game_story_choice_for(const Game *g, int player, unsigned choice) {
    if (g->story.version >= 3 && player >= 0 && player < g->player_count &&
        g->players[player].decision_open && g->players[player].decision_scene >= STORY_POWER)
        return expedition_option(g, g->players[player].decision_scene, choice);
    return primary_choice(g, choice);
}
const char *game_story_choice(const Game *g, unsigned choice) {
    return game_story_choice_for(g, 0, choice);
}
bool game_story_choice_ready(const Game *g, int player) {
    if (g->story.version >= 3 && player >= 0 && player < g->player_count &&
        g->players[player].decision_open && g->players[player].decision_scene == STORY_COUNCIL)
        return (g->story.done & BIT(STORY_POWER)) && (g->story.done & BIT(STORY_CIPHER));
    return game_story_prepared(g);
}
bool game_story_choose(Game *g, int player, unsigned choice) {
    unsigned scene = game_story_near(g, player);
    if (g->story.version >= 3 && scene >= STORY_POWER && scene < SC_STORY_SCENES)
        return expedition_choose(g, player, scene, choice);
    if (choice < 1 || choice > 3 || scene != game_story_target(g) ||
        !g->players[player].decision_open)
        return false;
    if (choice == 3 && !game_story_prepared(g)) {
        game_message(g, "OPTIONAL OPERATION REQUIRED / U TRACK IT / ENTER TO RETURN");
        return false;
    }
    if (!story_clear(g, scene))
        return false;
    story_complete(g, player, scene);
    if (scene == STORY_SOURCE && choice == 1) {
        Room *source = world_find(&g->world, g->story.sites[scene]);
        if (source)
            for (int i = 0; i < source->enemy_count; i++)
                world_kill(&g->world, source, i);
    }
    g->story.choice = choice;
    if (g->story.version >= 3)
        expedition_conclude(g, choice);
    g->story.assigned |= BIT(STORY_EXTRACTION);
    g->story.tracked = STORY_EXTRACTION;
    for (int p = 0; p < g->player_count; p++)
        g->players[p].decision_open = g->players[p].journal = false;
    game_message(g, "DECISION RECORDED / RETURN TO THE BAY TO EXTRACT");
    return true;
}
void game_story_read(Game *g, int player) {
    if (player < 0 || player >= g->player_count)
        return;
    Player *p = &g->players[player];
    for (int n = 1; n <= SC_STORY_SCENES; n++) {
        unsigned at = (p->story_reading + n) % SC_STORY_SCENES;
        if (g->story.seen & BIT(at)) {
            p->story_reading = (uint8_t)at;
            return;
        }
    }
}
void game_story_entry(const Game *g, unsigned scene, char *text, size_t size) {
    const Story *s = &g->story;
    if (scene >= SC_STORY_SCENES || !(s->seen & BIT(scene))) {
        snprintf(text, size, "NO OBSERVATION YET / EXPLORE THE SHIP");
        return;
    }
    if (s->version >= 3 && scene >= STORY_POWER) {
        expedition_entry(g, scene, text, size);
        return;
    }
    if (scene >= STORY_ORIGIN_RECORD && scene <= STORY_CONTROL_RECORD) {
        world_history_notice(g->world.seed, scene - STORY_ORIGIN_RECORD + 1, text, size);
        return;
    }
    static const char *details[] = {
        "COMMISSION ACCEPTED. INVESTIGATE THE SIGNAL; SECURE SITES FOR 75 SALVAGE AND 2 POINTS.",
        "FRESH BOOTPRINTS CROSS OLD DUST. SOMEONE STILL TRAVELS BETWEEN THE REFUGES.",
        "NEW SPLICES BRIDGE DEAD CABLES. SURVIVORS ARE DIVERTING THE LAST POWER.",
        "WARM IDENTIFICATION TAGS LIE ON FROSTED DECKING. A RECENT THAW LEFT PEOPLE AWAKE.",
        "SPECIMEN LABELS WERE SCRUBBED BY HAND. THE CARGO RECORD MAY EXPLAIN CONTAINMENT.",
        "A RATION TIN IS STILL WARM. THIS SHRINE IS A WAYPOINT FOR LIVING PEOPLE.",
        "REFUGE SIGNAL SECURED. A SURVIVOR IDENTIFIED THE TARGET AND A SECOND WAY TO COMPLETE THE "
        "JOB.",
        "WITNESS ARCHIVE RECOVERED. THE LAST DUTY OFFICER RECORDED THE TARGET'S DEPENDENCIES.",
        "CONTAINMENT LEDGER READ. ITS SERVICE ROUTE IDENTIFIES THE TARGET AND A BACKUP SYSTEM.",
        "BACKUP REACTOR ONLINE. THE REFUGE CAN KEEP ITS POWER WHEN THE ALIEN DRIVE IS REMOVED.",
        "EVACUATION RELAY SECURED. LIFTER DRONES CAN NOW CARRY THE SLEEPERS TO THE BAY.",
        "MEMORY MIRROR READY. A COMPLETE COPY WILL LEAVE THE REFUGE'S NAVIGATOR INTACT.",
        "THE VAULT HAS AWAKE SURVIVORS AND IMMOBILE SLEEPERS. THE EVACUATION RELAY CAN CARRY BOTH.",
        "THE STAR CHART IS A LIVING ARCHIVE. THE REFUGE ALSO RELIES ON IT FOR SAFE NAVIGATION.",
        "THE ALIEN DRIVE IS INTACT. ITS OUTPUT IS KEEPING THE REFUGE'S LIFE SUPPORT ALIVE.",
        "EXTRACTION COMPLETE. YOUR CHOICE IS RECORDED; EXPLORE ON OR BEGIN A NEW EXPEDITION.",
        "THE COLLAPSE SOURCE IS STILL ACTIVE. PURGE IT, SEAL IT ABOARD, OR RESTORE ISOLATION TO "
        "SAVE THE RESEARCH.",
        "ISOLATION GRID RESTORED. THE SOURCE CAN NOW BE CONTAINED WITHOUT DESTROYING ITS "
        "RESEARCH."};
    const char *detail = details[scene];
    if (direct(g)) {
        switch (scene) {
        case STORY_ARCHIVE:
            detail = "THE WITNESS IDENTIFIES THE TARGET AND A SECOND RECOVERY OPPORTUNITY.";
            break;
        case STORY_REACTOR:
            detail =
                "FABRICATOR RESTORED. ITS ALIEN DESIGN CAN BE RECOVERED ALONGSIDE THE SPARE DRIVE.";
            break;
        case STORY_EVAC:
            detail = "REMOTE RELAY RESTORED. SLEEPERS FROM ANOTHER REFUGE CAN JOIN THE EVACUATION.";
            break;
        case STORY_MIRROR:
            detail = "CATALOGUE REBUILT. THE CHART CAN NOW INCLUDE PREVIOUSLY LOST ROUTES.";
            break;
        case STORY_SURVIVORS:
            detail = "THE REFUGE IS READY TO EVACUATE. THE RELAY CAN ALSO RESCUE A REMOTE GROUP OF "
                     "SLEEPERS.";
            break;
        case STORY_MEMORY:
            detail = "THE CHART IS AN INDEPENDENT SURVEY. ITS DAMAGED CATALOGUE COULD REVEAL "
                     "ADDITIONAL ROUTES.";
            break;
        case STORY_DRIVE:
            detail = "THIS IS A SPARE DRIVE. TAKING IT LEAVES SHIP POWER INTACT. A NEARBY "
                     "FABRICATOR OFFERS MORE TECHNOLOGY.";
            break;
        default:
            break;
        }
    }
    RoomSpec room = world_describe_version(g->world.seed, g->world.generator, s->sites[scene]);
    snprintf(text, size, "%s: %s", sc_themes[room.theme].name, detail);
}
void game_story_line(const Game *g, int line, char *text, size_t size) {
    const Story *s = &g->story;
    if (s->version >= 3 && s->chapter) {
        if (line == 1)
            snprintf(text, size, "CHAPTER %u / %s", s->chapter,
                     expedition_title(g, STORY_RESOLUTION));
        else if (line == 6)
            snprintf(text, size, "%s",
                     s->chapter_choice ? expedition_option(g, STORY_RESOLUTION, s->chapter_choice)
                                       : "NO CHAPTER DECISION MADE");
        else if (line == 7)
            snprintf(text, size, "G AT THE BAY FOR EXTRACTION / F7 DEBRIEF AND EVENT HISTORY");
        else if (line == 4)
            snprintf(text, size, "%u SURVIVORS STILL ABOARD / REFUGE SUPPORT %u / KEEPERS %u",
                     s->remaining, s->support[0], s->support[1]);
        else if (line == 5)
            snprintf(text, size, "%s",
                     s->done & BIT(STORY_RELAY) ? "RELAY READY / THIRD OUTCOME AVAILABLE"
                                                : "OPTIONAL RELAY CAN UNLOCK A THIRD OUTCOME");
        else if (line == 0)
            snprintf(text, size, "%s / THE EXPEDITION CONTINUES", story_ship(g->world.seed));
        else
            snprintf(text, size, "SITE PAY 75 SALVAGE + 2 POINTS / CHAPTER BONUS 180-400");
        return;
    }
    if (line == 0)
        snprintf(text, size, "%s / %s", story_ship(g->world.seed), survivors(g->world.seed));
    else if (line == 1)
        snprintf(text, size, "%s", story_commission_version(g->world.seed, s->version));
    else if (line == 2)
        snprintf(text, size, "SITE PAY 75 SALVAGE + 2 POINTS / EXTRACTION BONUS 180-400");
    else if (line == 3)
        snprintf(text, size, "%s",
                 (s->done & EVIDENCE) || (s->version >= 2 && (s->seen & BIT(STORY_ORIGIN_RECORD)))
                     ? (s->version >= 2 ? world_history_cause(world_history(g->world.seed).cause)
                                        : cause(g->world.seed))
                     : "CAUSE OF COLLAPSE UNKNOWN / INVESTIGATE A LEAD");
    else if (line == 4) {
        const char *facts[] = {"THE VAULT HOLDS AWAKE SURVIVORS AND IMMOBILE SLEEPERS",
                               "THE REFUGE USES THE REQUESTED ARCHIVE AS ITS NAVIGATOR",
                               "THE REQUESTED DRIVE POWERS THE REFUGE'S LIFE SUPPORT",
                               "THE ACTIVE SOURCE THREATENS THE SHIP'S NEXT INHABITED PORT"};
        const char *simple[] = {"THE REFUGE IS READY TO LEAVE / A SECOND GROUP COULD ALSO BE SAVED",
                                "THE CHART IS INDEPENDENT / ITS CATALOGUE HAS LOST ROUTES",
                                "THE SPARE DRIVE CAN BE REMOVED WITHOUT DISRUPTING SHIP POWER",
                                "THE ACTIVE SOURCE THREATENS THE SHIP'S NEXT INHABITED PORT"};
        snprintf(text, size, "%s",
                 s->done & EVIDENCE ? (direct(g) ? simple : facts)[motivation(g)]
                                    : "THE TARGET'S CONDITION IS UNCONFIRMED");
    } else if (line == 5)
        snprintf(text, size, "%s",
                 s->choice                ? "DECISION COMMITTED / OPTIONAL LEADS ARE CLOSED"
                 : game_story_prepared(g) ? "OPTIONAL OPERATION COMPLETE / THIRD OUTCOME AVAILABLE"
                                          : "OPTIONAL PREPARATION CAN UNLOCK ANOTHER OUTCOME");
    else if (line == 6)
        snprintf(text, size, "%s",
                 s->choice ? game_story_choice(g, s->choice) : "NO FINAL DECISION MADE");
    else
        snprintf(text, size, "%s",
                 s->done & BIT(STORY_EXTRACTION)
                     ? "VICTORY RECORDED / CONTINUE EXPLORING OR N FOR A NEW SHIP"
                     : "EXTRACT AT THE BAY AFTER RESOLVING THE TARGET");
}
void game_story_dossier(const Game *g, int line, char *text, size_t size) {
    if (g->story.version < 2) {
        snprintf(text, size, "ARCHIVAL EXPEDITION / SEE THE ORIGINAL FIELD RECORDS IN I JOURNAL");
        return;
    }
    ShipHistory past = world_history(g->world.seed);
    const Story *s = &g->story;
    bool evidence = (s->done & EVIDENCE) != 0;
    bool origin = evidence || (s->seen & BIT(STORY_ORIGIN_RECORD));
    bool group = evidence || (s->seen & BIT(STORY_SURVIVOR_RECORD));
    bool control = evidence || (s->seen & BIT(STORY_CONTROL_RECORD));
    if (line == 0)
        snprintf(text, size, "%s / ORIGINAL PURPOSE: %s", story_ship(g->world.seed),
                 world_profile_name(world_profile(g->world.seed)));
    else if (line == 1)
        snprintf(text, size, "LAST MANIFEST: %u %s / CURRENT SURVIVOR COUNT UNCONFIRMED",
                 past.population, survivors(g->world.seed));
    else if (line == 2)
        snprintf(text, size, "%s",
                 origin ? world_history_cause(past.cause)
                        : "COLLAPSE: UNKNOWN / INVESTIGATE WITNESSES OR DAMAGE NOTICES");
    else if (line == 3) {
        if (origin)
            snprintf(text, size, "CATASTROPHE ORIGIN [%d,%d,%d] / %s", past.origin.x, past.origin.y,
                     past.origin.z, sc_themes[world_history_theme(past.cause)].name);
        else
            snprintf(text, size, "ORIGIN: UNCONFIRMED / FOLLOW THE CONNECTED DAMAGE TRAIL");
    } else if (line == 4)
        snprintf(text, size, "%s",
                 control ? world_history_controller(past.cause)
                         : "CURRENT CONTROLLER: UNKNOWN / LOOK FOR LOCKOUT NOTICES");
    else if (line == 5)
        snprintf(text, size, "%s",
                 group ? world_history_agenda(g->world.seed)
                       : "SURVIVOR AGENDA: UNKNOWN / LOOK FOR WAYSTATION NOTICES");
    else if (line == 6)
        game_story_line(g, 4, text, size);
    else
        game_story_line(g, 6, text, size);
}
bool story_valid(const Story *s, uint32_t seed, uint32_t generator, uint32_t mission) {
    if (s->version == 0) {
        Story empty = {0};
        return memcmp(s, &empty, sizeof(*s)) == 0 && mission <= 6;
    }
    if (!expedition_valid(s, seed))
        return false;
    unsigned goal = target(seed, s->version), op = optional(seed, s->version);
    unsigned m = story_motivation_version(seed, s->version);
    unsigned lead = STORY_REFUGE + m % 3, alternate = STORY_REFUGE + (m + 1) % 3;
    uint32_t allowed = BIT(0) | CLUES | (s->version >= 2 ? HISTORY_RECORDS : 0) | BIT(lead) |
                       BIT(alternate) | BIT(op) | BIT(goal) | BIT(STORY_EXTRACTION) |
                       (s->version >= 3 ? UINT32_C(0x7fe00000) : 0);
    if ((s->version < 1 || s->version > 3) || generator != (s->version == 1 ? 4u : 5u) ||
        s->assigned & ~allowed || !(s->assigned & BIT(0)) || s->seen & ~s->assigned ||
        s->done & ~s->seen || s->done & PASSIVE || s->choice > 3 || s->tracked >= SC_STORY_SCENES ||
        mission != progress(s) || (s->choice != 0) != ((s->done & BIT(goal)) != 0) ||
        (s->choice == 3 && !(s->done & BIT(op)) && s->council != 3))
        return false;
    bool started = (s->done & BIT(0)) != 0, evidence = (s->done & EVIDENCE) != 0;
    if ((!started && (s->assigned != BIT(0))) || (started && !(s->assigned & BIT(lead))) ||
        ((s->assigned & BIT(alternate)) != 0) != ((s->seen & CLUES) != 0) ||
        ((s->assigned & BIT(goal)) != 0) != evidence ||
        ((s->assigned & BIT(op)) != 0) != evidence ||
        ((s->assigned & BIT(STORY_EXTRACTION)) != 0) != (s->choice != 0) ||
        (!(s->done & BIT(STORY_EXTRACTION)) && !available(s, s->tracked)) ||
        ((s->done & BIT(STORY_EXTRACTION)) && s->version < 3 && s->tracked != STORY_EXTRACTION) ||
        (s->version >= 3 && !available(s, s->tracked) &&
         !(s->chapter == 1000 && (s->done & BIT(STORY_RETURN)))))
        return false;
    for (int i = 0; i < SC_STORY_SCENES; i++) {
        Cell c = s->sites[i];
        if (!(s->assigned & BIT(i)) || i == STORY_BRIEFING || i == STORY_EXTRACTION ||
            i == STORY_CONTINUE || i == STORY_RETURN) {
            if (!cell_equal(c, (Cell){0, 0, 0}))
                return false;
            continue;
        }
        if (c.x < -10000000 || c.x > 10000000 || c.y < -4000000 || c.y > 4000000 || c.z < 0 ||
            c.z > 10000000)
            return false;
        RoomSpec room = world_describe_version(seed, generator, c);
        if (!site_room(room) ||
            (!cell_equal(c, room.cell) &&
             !(i == STORY_RESOLUTION && s->arc == HARROW_ARC && cell_equal(c, (Cell){2, 1, 2}))))
            return false;
        if (CLUES & BIT(i)) {
            if (!(s->seen & BIT(i)) || clue(room) != (unsigned)i)
                return false;
        } else if (HISTORY_RECORDS & BIT(i)) {
            if (!(s->seen & BIT(i)) || room.history_role != (unsigned)i - STORY_ORIGIN_RECORD + 1)
                return false;
        } else {
            if (i == STORY_SOURCE && !cell_equal(c, world_history(seed).origin))
                return false;
            if (i < STORY_CONTACT && (c.x < 0 || c.x > 3 || c.y < -1 || c.y > 1 || c.z > 3))
                return false;
            for (int j = i >= STORY_CONTACT ? STORY_CONTACT : STORY_REFUGE; j < i; j++)
                if (j != STORY_EXTRACTION && !(PASSIVE & BIT(j)) && (s->assigned & BIT(j)) &&
                    cell_equal(c, s->sites[j]))
                    return false;
        }
    }
    return true;
}
