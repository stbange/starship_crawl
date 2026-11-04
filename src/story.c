#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIT(n) (UINT32_C(1) << (n))
#define EVIDENCE (BIT(STORY_REFUGE) | BIT(STORY_ARCHIVE) | BIT(STORY_LEDGER))
#define CLUES UINT32_C(62)
#define TARGETS (BIT(STORY_SURVIVORS) | BIT(STORY_MEMORY) | BIT(STORY_DRIVE))

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
    {"RETURN TO THE EXTRACTION BEACON", BAY, SANCTUM}};
static unsigned history(uint32_t seed, unsigned salt, unsigned count) {
    return world_hash(seed, (Cell){0, 0, 0}, UINT32_C(0x53540000) + salt) % count;
}
StoryMotivation story_motivation(uint32_t seed) {
    return (StoryMotivation)history(seed, 1, 3);
}
const char *story_ship(uint32_t seed) {
    static const char *names[] = {"SAINT VESPER", "MOURNING GLASS", "THE NINTH MERCY",
                                  "CROWN OF ASH", "PALE ORCHARD",   "VESSEL OF HOURS",
                                  "BLACK LITANY", "LAST DAWN"};
    return names[history(seed, 2, 8)];
}
const char *story_commission(uint32_t seed) {
    static const char *names[] = {"RESCUE / BRING THE LOST HOME",
                                  "KNOWLEDGE / RECOVER A LOST STAR CHART",
                                  "SALVAGE / RECOVER AN ALIEN DRIVE"};
    return names[story_motivation(seed)];
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
static unsigned target(uint32_t seed) {
    return STORY_SURVIVORS + story_motivation(seed);
}
static unsigned optional(uint32_t seed) {
    const unsigned scenes[] = {STORY_EVAC, STORY_MIRROR, STORY_REACTOR};
    return scenes[story_motivation(seed)];
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
    s->version = 1;
    s->assigned = BIT(STORY_BRIEFING);
}
static bool available(const Story *s, unsigned scene) {
    return scene < SC_STORY_SCENES && (s->assigned & BIT(scene)) && !(s->done & BIT(scene)) &&
           !(CLUES & BIT(scene)) && (!s->choice || scene == STORY_EXTRACTION);
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
static void reserve(Game *g, unsigned scene, Cell from) {
    Story *s = &g->story;
    if (s->assigned & BIT(scene))
        return;
    Cell best = {0};
    double score = 1e9;
    /* Bounded introductory catchment; all cells have lifts and connected doors.
       Logical-room owners avoid putting two operations in one multi-chunk hall. */
    for (int y = -1; y <= 1; y++)
        for (int z = 0; z < 4; z++)
            for (int x = 0; x < 4; x++) {
                Cell c = {x, y, z};
                RoomSpec room = world_describe_version(g->world.seed, g->world.generator, c);
                if (!site_room(room) || !cell_equal(c, room.cell))
                    continue;
                bool used = false;
                for (int i = STORY_REFUGE; i < STORY_EXTRACTION; i++)
                    used |= (s->assigned & BIT(i)) && cell_equal(s->sites[i], c);
                if (used)
                    continue;
                const StoryTemplate *def = &sc_story_templates[scene];
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
            game_message(g, sc_story_templates[at].title);
            return;
        }
    }
}
void game_story_step(Game *g) {
    Story *s = &g->story;
    if (!s->version || s->choice)
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
        unsigned found = clue(spec);
        if (s->seen & BIT(found))
            continue;
        s->sites[found] = room->cell;
        s->assigned |= BIT(found);
        s->seen |= BIT(found);
        reserve(g, STORY_REFUGE + (story_motivation(g->world.seed) + 1) % 3, room->cell);
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
static bool clear(Game *g, unsigned scene) {
    if (scene == STORY_BRIEFING || scene == STORY_EXTRACTION)
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
static void complete(Game *g, int player, unsigned scene) {
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
        p->weapons |= BIT(scene == STORY_BRIEFING ? 1 : 4 + story_motivation(g->world.seed));
        if (scene == STORY_EXTRACTION) {
            const unsigned pay[] = {0, 250, 180, 400};
            p->credits += pay[s->choice];
            p->victories++;
            p->weapons |= BIT(15);
        }
    }
    g->dirty = true;
}
void game_story_interact(Game *g, int player) {
    unsigned scene = game_story_near(g, player);
    if (scene == SC_STORY_SCENES) {
        game_message(g, "FOLLOW A KNOWN LEAD / I JOURNAL / U CHANGE MAP TARGET");
        return;
    }
    if (!clear(g, scene))
        return;
    Story *s = &g->story;
    if (TARGETS & BIT(scene)) {
        Player *p = &g->players[player];
        p->decision_open = p->journal = true;
        p->journal_page = 1;
        p->story_reading = (uint8_t)scene;
        s->seen |= BIT(scene);
        g->dirty = true;
        game_message(g, "REVIEW THE CONSEQUENCES / 1 2 3 CHOOSE / ENTER LEAVES UNDECIDED");
        return;
    }
    complete(g, player, scene);
    if (scene == STORY_BRIEFING) {
        unsigned lead = STORY_REFUGE + story_motivation(g->world.seed);
        reserve(g, lead, (Cell){0, 0, 0});
        s->tracked = lead;
    } else if (EVIDENCE & BIT(scene)) {
        unsigned goal = target(g->world.seed);
        reserve(g, goal, s->sites[scene]);
        reserve(g, optional(g->world.seed), s->sites[scene]);
        s->tracked = goal;
    } else if (scene != STORY_EXTRACTION)
        s->tracked = target(g->world.seed);
    char message[128];
    game_story_entry(g, scene, message, sizeof(message));
    game_message(g, message);
}
const char *game_story_choice(const Game *g, unsigned choice) {
    static const char *options[3][3] = {
        {"EVACUATE THE AWAKE / SLEEPERS STAY BEHIND", "FORTIFY THE REFUGE / ALL STAY ABOARD",
         "EVACUATE EVERYONE / ALL SURVIVORS GO HOME"},
        {"TAKE THE ARCHIVE / REFUGE LOSES ITS NAVIGATOR",
         "TRANSMIT A SURVEY / KEEP THE ARCHIVE ABOARD",
         "COPY THE FULL ARCHIVE / KEEP THE NAVIGATOR ABOARD"},
        {"TAKE THE DRIVE / REFUGE LOSES POWER", "LEAVE THE DRIVE / PRESERVE THE REFUGE",
         "TAKE THE DRIVE SAFELY / REFUGE KEEPS ITS POWER"}};
    return choice >= 1 && choice <= 3 ? options[story_motivation(g->world.seed)][choice - 1] : "";
}
bool game_story_choose(Game *g, int player, unsigned choice) {
    unsigned scene = game_story_near(g, player);
    if (choice < 1 || choice > 3 || scene != target(g->world.seed) ||
        !g->players[player].decision_open)
        return false;
    if (choice == 3 && !(g->story.done & BIT(optional(g->world.seed)))) {
        game_message(g, "OPTIONAL OPERATION REQUIRED / U TRACK IT / ENTER TO RETURN");
        return false;
    }
    if (!clear(g, scene))
        return false;
    complete(g, player, scene);
    g->story.choice = choice;
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
        "EXTRACTION COMPLETE. YOUR CHOICE IS RECORDED; EXPLORE ON OR BEGIN A NEW EXPEDITION."};
    RoomSpec room = world_describe_version(g->world.seed, g->world.generator, s->sites[scene]);
    snprintf(text, size, "%s: %s", sc_themes[room.theme].name, details[scene]);
}
void game_story_line(const Game *g, int line, char *text, size_t size) {
    const Story *s = &g->story;
    if (line == 0)
        snprintf(text, size, "%s / %s", story_ship(g->world.seed), survivors(g->world.seed));
    else if (line == 1)
        snprintf(text, size, "%s", story_commission(g->world.seed));
    else if (line == 2)
        snprintf(text, size, "SITE PAY 75 SALVAGE + 2 POINTS / EXTRACTION BONUS 180-400");
    else if (line == 3)
        snprintf(text, size, "%s",
                 s->done & EVIDENCE ? cause(g->world.seed)
                                    : "CAUSE OF COLLAPSE UNKNOWN / INVESTIGATE A LEAD");
    else if (line == 4) {
        const char *facts[] = {"THE VAULT HOLDS AWAKE SURVIVORS AND IMMOBILE SLEEPERS",
                               "THE REFUGE USES THE REQUESTED ARCHIVE AS ITS NAVIGATOR",
                               "THE REQUESTED DRIVE POWERS THE REFUGE'S LIFE SUPPORT"};
        snprintf(text, size, "%s",
                 s->done & EVIDENCE ? facts[story_motivation(g->world.seed)]
                                    : "THE TARGET'S CONDITION IS UNCONFIRMED");
    } else if (line == 5)
        snprintf(text, size, "%s",
                 s->choice ? "DECISION COMMITTED / OPTIONAL LEADS ARE CLOSED"
                 : s->done & BIT(optional(g->world.seed))
                     ? "OPTIONAL OPERATION COMPLETE / THIRD OUTCOME AVAILABLE"
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
bool story_valid(const Story *s, uint32_t seed, uint32_t generator, uint32_t mission) {
    if (s->version == 0) {
        Story empty = {0};
        return memcmp(s, &empty, sizeof(*s)) == 0 && mission <= 6;
    }
    unsigned goal = target(seed), op = optional(seed), lead = STORY_REFUGE + story_motivation(seed);
    unsigned alternate = STORY_REFUGE + (story_motivation(seed) + 1) % 3;
    uint32_t allowed =
        BIT(0) | CLUES | BIT(lead) | BIT(alternate) | BIT(op) | BIT(goal) | BIT(STORY_EXTRACTION);
    if (s->version != 1 || generator != SC_GENERATOR_VERSION || s->assigned & ~allowed ||
        !(s->assigned & BIT(0)) || s->seen & ~s->assigned || s->done & ~s->seen ||
        s->done & CLUES || s->choice > 3 || s->tracked >= SC_STORY_SCENES ||
        mission != progress(s) || (s->choice != 0) != ((s->done & BIT(goal)) != 0) ||
        (s->choice == 3 && !(s->done & BIT(op))))
        return false;
    bool started = (s->done & BIT(0)) != 0, evidence = (s->done & EVIDENCE) != 0;
    if ((!started && (s->assigned != BIT(0))) || (started && !(s->assigned & BIT(lead))) ||
        ((s->assigned & BIT(alternate)) != 0) != ((s->seen & CLUES) != 0) ||
        ((s->assigned & BIT(goal)) != 0) != evidence ||
        ((s->assigned & BIT(op)) != 0) != evidence ||
        ((s->assigned & BIT(STORY_EXTRACTION)) != 0) != (s->choice != 0) ||
        (!(s->done & BIT(STORY_EXTRACTION)) && !available(s, s->tracked)) ||
        ((s->done & BIT(STORY_EXTRACTION)) && s->tracked != STORY_EXTRACTION))
        return false;
    for (int i = 0; i < SC_STORY_SCENES; i++) {
        Cell c = s->sites[i];
        if (!(s->assigned & BIT(i)) || i == STORY_BRIEFING || i == STORY_EXTRACTION) {
            if (!cell_equal(c, (Cell){0, 0, 0}))
                return false;
            continue;
        }
        if (c.x < -10000000 || c.x > 10000000 || c.y < -4000000 || c.y > 4000000 || c.z < 0 ||
            c.z > 10000000)
            return false;
        RoomSpec room = world_describe_version(seed, generator, c);
        if (!site_room(room) || !cell_equal(c, room.cell))
            return false;
        if (CLUES & BIT(i)) {
            if (!(s->seen & BIT(i)) || clue(room) != (unsigned)i)
                return false;
        } else {
            if (c.x < 0 || c.x > 3 || c.y < -1 || c.y > 1 || c.z > 3)
                return false;
            for (int j = STORY_REFUGE; j < i; j++)
                if ((s->assigned & BIT(j)) && cell_equal(c, s->sites[j]))
                    return false;
        }
    }
    return true;
}
