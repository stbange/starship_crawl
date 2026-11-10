#include "game.h"
#include <stdio.h>
#include <string.h>

static uint32_t person_identity(uint32_t seed, unsigned index) {
    uint32_t name = (world_hash(seed, (Cell){0}, 0x4e414d45u) + index * 3) % 8;
    return (world_hash(seed, (Cell){(int)index, 0, 0}, 0x50455253u) & ~15u) | 8u | name;
}
const char *chronicle_name(const RecurringPerson *p) {
    static const char *names[] = {"ESH-OF-GLASS",       "NINE QUIET HANDS", "VEI THE MENDER",
                                  "ORU WITHOUT SHADOW", "SABLE THIRD",      "ILIS-OF-RAIN",
                                  "THREN THE PATIENT",  "MOTH-IN-IRON"};
    return names[p->identity % 8];
}
const char *chronicle_role(unsigned role) {
    static const char *roles[] = {"UNKNOWN",  "ENGINEER", "ARCHIVIST", "CARETAKER",
                                  "DESERTER", "PILGRIM",  "SCAVENGER"};
    return roles[role <= 6 ? role : 0];
}
Vec3 chronicle_position(const RecurringPerson *p) {
    return vadd(district_control(p->site, JOURNEY_REFUGE), v3((p->role & 1) ? -2.5 : 2.5, 0, 0));
}
void chronicle_init(Game *g) {
    memset(&g->chronicle, 0, sizeof(g->chronicle));
    if (g->world.generator < 16)
        return;
    Chronicle *c = &g->chronicle;
    c->version = c->next_id = 1;
    c->count = 2;
    for (unsigned i = 0; i < c->count; i++) {
        RecurringPerson *p = &c->people[i];
        DistrictContract d = district_contract(g->world.seed, g->world.generator, campaign_site(i));
        p->identity = person_identity(g->world.seed, i);
        p->role = i + 1;
        p->home = p->site = p->previous = p->destination = d.refuge;
    }
}
bool chronicle_valid(const Chronicle *c, uint32_t seed, unsigned generator) {
    if (generator < 16)
        return true;
    if (c->version != 1 || c->count < 2 || c->count > SC_RECURRING || !c->next_id)
        return false;
    for (unsigned i = 0; i < c->count; i++) {
        const RecurringPerson *p = &c->people[i];
        if (p->identity != person_identity(seed, i) || p->role < 1 || p->role > 6 ||
            p->knowledge > 15 || p->fate < PERSON_WAITING || p->fate > PERSON_LEFT ||
            p->phase > 4 || !isfinite(p->travel) || p->travel < 0 || p->travel > 30 ||
            p->home.z < 0 || p->site.z < 0 || p->previous.z < 0 || p->destination.z < 0)
            return false;
        for (unsigned j = 0; j < i; j++)
            if (c->people[j].role == p->role)
                return false;
    }
    for (unsigned i = 0; i < SC_CONTINUING_CAMPAIGNS; i++) {
        const ContinuingCampaign *a = &c->campaigns[i];
        if (!a->identity)
            continue;
        if (a->identity >= c->next_id || a->country < 1 || a->country > 3 || a->pattern > 1 ||
            a->stage < CHAIN_SUPPLY || a->stage > CHAIN_RETREATED || !a->force ||
            !isfinite(a->clock) || a->clock < 0 || a->clock > 30 || a->source.z < 0 ||
            a->staging.z < 0 || a->goal.z < 0)
            return false;
        for (unsigned j = 0; j < i; j++)
            if (c->campaigns[j].identity == a->identity)
                return false;
    }
    return true;
}
int chronicle_near(const Game *g, int player) {
    if (g->world.generator < 16 || player < 0 || player >= g->player_count ||
        g->players[player].sheltered || g->players[player].health <= 0 ||
        g->matrix.divers[player].active)
        return -1;
    Vec3 at = position_absolute(g->players[player].pos);
    for (unsigned i = 0; i < g->chronicle.count; i++) {
        const RecurringPerson *p = &g->chronicle.people[i];
        if (p->fate != PERSON_TRAVELLING && world_ready(&g->world, p->site) &&
            vlen(vsub(at, chronicle_position(p))) < 2.4)
            return (int)i;
    }
    return -1;
}
bool chronicle_interact(Game *g, int player) {
    int i = chronicle_near(g, player);
    if (i < 0)
        return false;
    RecurringPerson *p = &g->chronicle.people[i];
    if (!(p->knowledge & PERSON_SEEN))
        p->meetings++;
    p->knowledge |= PERSON_SEEN;
    g->views[player].journal = g->interactions[player].decision_open = true;
    g->views[player].journal_page = 1;
    g->views[player].story_reading = g->interactions[player].decision_scene = SC_STORY_SCENES + i;
    g->dirty = true;
    return true;
}
bool chronicle_ready(const Game *g, int player) {
    int i = chronicle_near(g, player);
    return i >= 0 &&
           (g->chronicle.people[i].fate >= PERSON_LOST ||
            (g->chronicle.people[i].knowledge & (PERSON_HELPED | PERSON_STRIPPED)) ||
            district_powered(&g->world,
                             &(DistrictMark){.family = 1, .anchor = g->chronicle.people[i].site}) ||
            g->players[player].credits >= 60);
}
const char *chronicle_option(const Game *g, int player, unsigned choice) {
    int i = chronicle_near(g, player);
    if (i < 0 || choice < 1 || choice > 3)
        return "";
    const RecurringPerson *p = &g->chronicle.people[i];
    if (p->fate >= PERSON_LOST || p->knowledge & (PERSON_HELPED | PERSON_STRIPPED))
        return choice == 1 ? "READ THE RETAINED RECORD / MAIN ROUTE REMAINS OPEN"
                           : "LEAVE THIS RECORD UNCHANGED";
    StoryMotivation motive = story_motivation_version(g->world.seed, g->story.version);
    if (choice == 1)
        return motive == STORY_RESCUE
                   ? "MARK A RESCUE ROUTE / NO COST / LEAVE POWER IN PLACE"
                   : "COPY THE TECHNICAL RECORD / NO COST / LEAVE POWER IN PLACE";
    if (choice == 2)
        return "STRIP 90 SALVAGE / REFUGE LOSES POWER / INHABITANT MUST LEAVE";
    return chronicle_ready(g, player) &&
                   district_powered(&g->world, &(DistrictMark){.family = 1, .anchor = p->site})
               ? "RESTORE THE REFUGE / EXISTING POWER MAKES THIS FREE"
               : "RESTORE THE REFUGE / COST 60 SALVAGE / MACHINES WILL WAKE";
}
void chronicle_entry(const Game *g, unsigned index, char *text, size_t size) {
    if (index >= g->chronicle.count || !(g->chronicle.people[index].knowledge & PERSON_SEEN)) {
        snprintf(text, size, "NO MEETING RECORDED / THE REFUGE KEEPS AN EMERGENCY ROUTE RECORD.");
        return;
    }
    const RecurringPerson *p = &g->chronicle.people[index];
    const char *fact =
        p->fate == PERSON_LOST   ? "THE REFUGE FELL. THEIR RECORDER SURVIVED."
        : p->fate == PERSON_LEFT ? "YOU REMOVED THE REFUGE POWER. THEIR EVACUATION NOTE REMAINS."
        : p->fate == PERSON_TRAVELLING ? "THEY TOOK THE MARKED SERVICE PASSAGES TO THE NEXT REFUGE."
        : p->knowledge & PERSON_HELPED
            ? "THEY REMEMBER YOUR REPAIR OF THE FIRST REFUGE."
            : "THE REFUGE NEEDS POWER. STRIPPING IT FOR SALVAGE FORCES THEM OUT.";
    snprintf(
        text, size, "%s / %s. %s %s", chronicle_name(p), chronicle_role(p->role), fact,
        p->role == 1
            ? "THE PUMP WAKES ROBOTS; USE HIGH GROUND. A BASIC ROUTE NEEDS NO HACKING."
            : "THE ARCHIVE LEAVES ROUTE MARKS; ITS MAIN RECORD DOES NOT DEPEND ON THIS MEETING.");
}
bool chronicle_choose(Game *g, int player, unsigned choice) {
    int i = chronicle_near(g, player);
    if (i < 0 || choice < 1 || choice > 3 || !g->interactions[player].decision_open ||
        g->interactions[player].decision_scene != SC_STORY_SCENES + i)
        return false;
    RecurringPerson *p = &g->chronicle.people[i];
    bool already = (p->knowledge & (PERSON_HELPED | PERSON_STRIPPED)) || p->fate >= PERSON_LOST;
    if (!(p->knowledge & (PERSON_HELPED | PERSON_STRIPPED)) && p->fate < PERSON_LOST) {
        RoomSpec spec = world_describe_version(g->world.seed, g->world.generator, p->site);
        uint32_t fact = world_district_state(&g->world, spec.journey.anchor);
        /* Reserve the outcome record before changing power or charging salvage. */
        if (choice != 1 && !world_set_district_state(&g->world, spec.journey.anchor, fact))
            return false;
        if (choice == 3) {
            if (!chronicle_ready(g, player))
                return false;
            bool free_power = district_powered(&g->world, &spec.journey);
            if (!world_set_systems(&g->world, p->site,
                                   (world_systems(&g->world, p->site) | SYSTEM_POWER) &
                                       ~SYSTEM_BLACKOUT))
                return false;
            if (!world_set_district_state(&g->world, spec.journey.anchor, fact | DISTRICT_RESCUED))
                return false;
            if (!free_power)
                g->players[player].credits -= 60;
            p->knowledge |= PERSON_HELPED;
            factions_systems(g, p->site, world_systems(&g->world, p->site), player);
        } else if (choice == 2) {
            if (!world_set_systems(&g->world, p->site,
                                   (world_systems(&g->world, p->site) | SYSTEM_BLACKOUT) &
                                       ~SYSTEM_POWER) ||
                !world_set_district_state(&g->world, spec.journey.anchor, fact | DISTRICT_SALVAGED))
                return false;
            p->knowledge |= PERSON_STRIPPED;
            p->fate = PERSON_LEFT;
            factions_systems(g, p->site, world_systems(&g->world, p->site), player);
            g->players[player].credits = (uint32_t)fmin(1000000, g->players[player].credits + 90);
        }
    }
    for (int n = 0; n < g->player_count; n++)
        if (g->interactions[n].decision_scene == SC_STORY_SCENES + i)
            g->interactions[n].decision_open = g->views[n].journal = false;
    g->dirty = true;
    game_message(g, already       ? "REFUGE DECISION RETAINED / THE RECORDER KEEPS THE ROUTE"
                    : choice == 3 ? "REFUGE RESTORED / AMBER WARNING BEFORE MACHINES WAKE"
                    : choice == 2 ? "SALVAGE RECOVERED / REFUGE DARK / EVACUATION NOTE RETAINED"
                                  : "ROUTE RECORD COPIED / MAIN EXPEDITION REMAINS AVAILABLE");
    return true;
}
void chronicle_step(Game *g, double dt) {
    if (g->world.generator < 16)
        return;
    bool outside = false;
    for (int i = 0; i < g->player_count; i++)
        outside |= !g->players[i].sheltered;
    if (!outside)
        return;
    unsigned phase = campaign_phase(&g->campaign);
    for (unsigned i = 0; i < g->chronicle.count; i++) {
        RecurringPerson *p = &g->chronicle.people[i];
        if (p->fate >= PERSON_LOST)
            continue;
        const FactionRegion *r = factions_at(&g->factions, p->site);
        if (r && (r->flags & FS_CAPTURED) && r->casualties >= 3 &&
            !(p->knowledge & PERSON_HELPED) &&
            !factions_observed(g, vadd(chronicle_position(p), v3(0, 1, 0)), 12)) {
            RoomSpec spec = world_describe_version(g->world.seed, g->world.generator, p->site);
            if (!world_set_district_state(&g->world, spec.journey.anchor,
                                          world_district_state(&g->world, spec.journey.anchor) |
                                              DISTRICT_LOST))
                continue;
            p->fate = PERSON_LOST;
            g->dirty = true;
            continue;
        }
        if (p->fate != PERSON_TRAVELLING && (p->knowledge & PERSON_HELPED) && phase > p->phase &&
            phase < 4 && !factions_observed(g, vadd(chronicle_position(p), v3(0, 1, 0)), 12)) {
            DistrictContract d =
                district_contract(g->world.seed, g->world.generator, campaign_site(phase * 2));
            Navigation nav;
            if (d.family && !cell_equal(d.refuge, p->site) &&
                game_navigation_from(g, position_from(chronicle_position(p)), false, d.refuge,
                                     &nav) &&
                nav.kind != NAV_BLOCKED) {
                p->previous = p->site;
                p->destination = d.refuge;
                p->fate = PERSON_TRAVELLING;
                p->phase = phase;
                p->travel = 30;
                p->knowledge |= PERSON_TRAVEL_KNOWN;
                g->dirty = true;
            }
        }
        if (p->fate != PERSON_TRAVELLING)
            continue;
        p->travel = fmaxf(0, p->travel - (float)dt);
        if (p->travel > 0)
            continue;
        Navigation nav;
        if (!game_navigation_from(g, position_from(world_center(p->site)), false, p->destination,
                                  &nav) ||
            nav.kind == NAV_BLOCKED ||
            factions_observed(g, vadd(world_center(nav.next), v3(0, 1, 0)), 32))
            continue;
        p->site = nav.next;
        p->travel = 30;
        if (cell_equal(p->site, p->destination) || nav.kind == NAV_GOAL) {
            p->site = p->destination;
            p->fate = PERSON_SETTLED;
            p->meetings++;
        }
        g->dirty = true;
    }
}
