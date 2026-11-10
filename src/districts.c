#include "game.h"
#include <stdio.h>
#include <stdlib.h>

const char *district_name(unsigned family) {
    static const char *names[] = {"UNSURVEYED", "DROWNED FOUNDRY", "FROZEN TERRACE",
                                  "COMMAND RELIQUARY", "TRANSIT GARDEN"};
    return names[family <= 4 ? family : 0];
}
const char *district_condition_name(unsigned condition) {
    static const char *names[] = {"UNKNOWN", "POWER WAKES MACHINES", "FREEZE / VENT CYCLE",
                                  "LIVING CONDUITS"};
    return names[condition <= 3 ? condition : 0];
}
static bool compatible(const RoomSpec *r) {
    return r->type != BAY && r->type != CONDUIT && r->height >= 5 && r->width >= 48 &&
           r->depth >= 48 && !r->habitat.form && !r->habitat.volume;
}
void district_plan(uint32_t seed, RegionPlan *plan) {
    int vista = -1;
    for (int i = 0; i < plan->count; i++) {
        RoomSpec *r = &plan->rooms[i];
        if (!compatible(r) || (onboarding_opening(r->cell) && campaign_guardian(r->cell) < 0))
            continue;
        if (vista < 0 || campaign_guardian(r->cell) >= 0 ||
            r->width * r->depth > plan->rooms[vista].width * plan->rooms[vista].depth)
            vista = i;
        if (campaign_guardian(r->cell) >= 0)
            break;
    }
    if (vista < 0)
        return; /* No compatible floor: retain the original routes and objectives. */
    RoomSpec *landmark = &plan->rooms[vista];
    uint32_t id = world_hash(seed, plan->origin, 0x44495354u);
    unsigned condition = district_condition(seed);
    unsigned family = 1; /* E7 families share the same connection contract. */
    ShipTraits traits = campaign_traits(seed, landmark->cell);
    if (traits.heat <= -2 || (id >> 4) % 4 == 1)
        family = 2;
    else if (traits.order >= 2)
        family = 3;
    else if (traits.order <= -2 || (id >> 4) % 4 == 3)
        family = 4;
    else if ((id >> 4) % 4 == 2)
        family = 3;
    if (!district_compatible(family, condition))
        family = condition == 1 ? 1 : condition == 2 ? 2 + (id % 3) : 4;
    if (campaign_guardian(landmark->cell) == 0 && condition == 1)
        family = 1;
    plan->journey = (DistrictContract){.anchor = landmark->cell,
                                       .preparation = landmark->cell,
                                       .refuge = landmark->cell,
                                       .identity = id,
                                       .family = (uint8_t)family,
                                       .condition = (uint8_t)condition,
                                       .reservations = 7};
    /* Roles are selected from the existing connected plan. No portal or owner
       table is rewritten, and historical/introductory rooms keep their content. */
    unsigned next_role = JOURNEY_PUMP;
    for (int i = 0; i < plan->count; i++) {
        RoomSpec *r = &plan->rooms[i];
        if (r->type == BAY || (onboarding_opening(r->cell) && i != vista))
            continue;
        r->journey = (DistrictMark){landmark->cell, id, (uint8_t)family, (uint8_t)condition, 0};
        if (i == vista || r->history_role || !compatible(r) || next_role > JOURNEY_REFUGE)
            continue;
        r->journey.role = (uint8_t)next_role;
        if (next_role == JOURNEY_PUMP)
            plan->journey.preparation = r->cell;
        else
            plan->journey.refuge = r->cell;
        r->habitat.challenge = r->habitat.reveal = 0;
        r->habitat.pace = PACE_QUIET;
        next_role <<= 1;
    }
    landmark->journey.role = JOURNEY_VISTA;
    if (cell_equal(plan->journey.preparation, landmark->cell))
        landmark->journey.role |= JOURNEY_PUMP;
    if (cell_equal(plan->journey.refuge, landmark->cell))
        landmark->journey.role |= JOURNEY_REFUGE;
    if (landmark->width < 80 || landmark->depth < 80)
        landmark->journey.role |= JOURNEY_COMPACT;
    landmark->height = fmax(landmark->height, 32);
    landmark->habitat.atmosphere = condition == 2   ? FROZEN_RUNOFF
                                   : condition == 3 ? VOID_GLOW
                                                    : FLOODED;
    landmark->habitat.ecology = condition == 2   ? CRYOPHILE
                                : condition == 3 ? BROOD_NEST
                                                 : MECHANICAL;
    /* A non-guardian vista still has an optional support encounter. */
    if (campaign_guardian(landmark->cell) < 0 && !landmark->history_role) {
        landmark->habitat.pace = PACE_FRONTIER;
        landmark->habitat.challenge = 0;
    }
}
DistrictContract district_contract(uint32_t seed, uint32_t generator, Cell cell) {
    RegionPlan plan;
    world_plan_version(seed, generator, world_region(cell), &plan);
    return plan.journey;
}
Vec3 district_control(Cell owner, unsigned role) {
    Vec3 offset = role == JOURNEY_PUMP     ? v3(-18, .02, -6)
                  : role == JOURNEY_REFUGE ? v3(18, .02, -6)
                                           : v3(-24, .02, -20);
    if (role == DISTRICT_SOURCE_CONTROL)
        offset = v3(-16, .02, 22);
    if (role == DISTRICT_RETURN_CONTROL)
        offset = v3(24, .02, 28);
    return vadd(world_center(owner), offset);
}
static void piece(Builder *b, Vec3 at, Vec3 size, Color color, float glow, bool solid) {
    if (b->chunk->box_count < SC_CHUNK_BOXES - 112)
        world_box(b, at, size, color, glow, solid);
}
static void sprite(Builder *b, const Room *r, SpriteKind kind, Vec3 at, float width, float height) {
    if (!cell_equal(b->chunk->cell, r->cell) || b->chunk->sprite_count >= SC_CHUNK_SPRITES)
        return;
    b->chunk->sprites[b->chunk->sprite_count++] = (EnvSprite){.pos = at,
                                                              .normal = {0, 0, 0},
                                                              .width = width,
                                                              .height = height,
                                                              .hash = r->hash ^ (uint32_t)kind,
                                                              .kind = (uint8_t)kind,
                                                              .atmosphere = r->habitat.atmosphere};
}
void district_build(Builder *b, const Room *r) {
    if (!r->journey.role)
        return;
    Vec3 c = world_center(r->cell);
    Color metal = rgb(.14f, .19f, .2f), copper = rgb(.45f, .25f, .1f), blue = rgb(.12f, .6f, .8f);
    if (r->journey.role & JOURNEY_VISTA) {
        bool compact = (r->journey.role & JOURNEY_COMPACT) != 0;
        /* The reactor hangs above tiny homes. The cross routes and lift reserve
           remain clear; the lower route is ordinary supported floor. */
        unsigned family = r->journey.family;
        if (family == 1)
            piece(b, vadd(c, compact ? v3(0, 14, 12) : v3(0, 21, 24)),
                  compact ? v3(14, 6, 10) : v3(24, 8, 18), metal, .06f, false);
        else if (family == 2) {
            for (int tier = 0; tier < 4; tier++) {
                piece(b, vadd(c, v3(0, 13 + tier * 3, 20)), v3(18 - tier * 3, .3, 10 - tier * 2),
                      rgb(.35f, .5f, .65f), .15f, false);
                for (int strand = -3; strand <= 3; strand++)
                    piece(b, vadd(c, v3(strand * 2, 11 + tier * 3, 20)), v3(.1, 3, .1), blue, .35f,
                          false);
            }
        } else if (family == 3) {
            for (int rib = -3; rib <= 3; rib++)
                piece(b, vadd(c, v3(rib * 3, 16 + abs(rib), 22)), v3(.4, 9, 12), copper, .25f,
                      false);
            piece(b, vadd(c, v3(0, 12, 22)), v3(13, 2, 9), rgb(.15f, .12f, .22f), .25f, false);
            sprite(b, r, SPR_GLYPH, v3(0, 15, 15), 10, 8);
        } else {
            for (int span = 0; span < 5; span++)
                piece(b, vadd(c, v3((span & 1) ? -4 : 4, 12 + span * 3, 20)), v3(23, .8, 3),
                      rgb(.12f, .2f, .13f), .03f, false);
            sprite(b, r, SPR_FUNGUS, v3(0, 14, 20), 13, 16);
        }
        for (int x = -1; x <= 1; x += 2) {
            piece(b, vadd(c, v3(x * 10, 28, 24)), v3(.5, 12, .5), copper, .1f, false);
            for (int z = 0; z < 3; z++) {
                double home_x = x * (compact ? 22 : 29), home_z = 6 + z * (compact ? 7 : 10);
                if (family <= 2)
                    piece(b, vadd(c, v3(home_x, 2, home_z)), v3(7, 4, 7),
                          family == 1 ? metal : rgb(.26f, .24f, .3f), 0, true);
                else if (family == 3) {
                    piece(b, vadd(c, v3(home_x, 7, home_z)), v3(2, 14, 2), copper, .06f, true);
                    for (int shelf = 0; shelf < 4; shelf++)
                        piece(b, vadd(c, v3(home_x, 1 + shelf * 1.8, home_z)), v3(5, .4, 5), metal,
                              .12f, false);
                } else {
                    piece(b, vadd(c, v3(home_x, 2, home_z)), v3(4, 4, 5), rgb(.12f, .18f, .1f),
                          .05f, true);
                    sprite(b, r, SPR_FUNGUS, v3(home_x, 4, home_z), 4, 7);
                    sprite(b, r, SPR_CLOTH, v3(home_x, 8, home_z), 2, 9);
                }
                for (int window = -1; window <= 1; window++)
                    piece(b, vadd(c, v3(home_x - x * 3.6, 2.5, home_z + window * 1.5)),
                          v3(.08, .5, .7), blue, .45f, false);
            }
        }
        for (int rib = -5; rib <= 5; rib++)
            piece(b, vadd(c, v3(rib * 2, 16.9, 14.9)), v3(.25, .3, .12), copper, .75f, false);
        if (!(r->journey.role & JOURNEY_COMPACT)) {
            piece(b, vadd(c, v3(-23, 2.8, 22)), v3(4, .4, 12), metal, 0, true);
            for (int stair = 0; stair < 12; stair++) {
                double top = (stair + 1) * .25;
                for (int end = 0; end < 2; end++)
                    piece(b, vadd(c, v3(-23, top * .5, end ? 39.5 - stair : 4.5 + stair)),
                          v3(4, top, 1), metal, 0, true);
            }
            for (int z = 0; z < 4; z++)
                piece(b, vadd(c, v3(-24.8, 3.05, 17 + z * 3)), v3(.15, .08, .5), blue, 1, false);
        }
        Vec3 source = district_control(r->cell, DISTRICT_SOURCE_CONTROL);
        piece(b, vadd(source, v3(0, .75, 0)), v3(1.2, 1.5, 1.2), copper, .6f, true);
        sprite(b, r,
               r->journey.condition == 2 ? SPR_CRYSTAL
               : family == 4             ? SPR_MOTE
                                         : SPR_STEAM,
               v3(10, 15, 24), 4, 9);
        sprite(b, r, SPR_STEAM, v3(-23, 3.5, 20), 2, 4);
        sprite(b, r, SPR_CLOTH, v3(-25, 2, 6), 2, 3);
    }
    for (unsigned role = JOURNEY_PUMP; role <= JOURNEY_REFUGE; role <<= 1)
        if (r->journey.role & role) {
            Vec3 at = district_control(r->cell, role);
            piece(b, vadd(at, v3(0, .6, 0)), v3(1.5, 1.2, 1), metal, 0, false);
            piece(b, vadd(at, v3(0, 1.4, 0)), v3(1.2, .25, .1), blue, .9f, false);
            if (!(r->journey.role & JOURNEY_VISTA))
                for (int side = -1; side <= 1; side += 2) {
                    Vec3 prop = vadd(c, v3(side * 12, role == JOURNEY_PUMP ? 1.5 : .5, 12));
                    piece(b, prop, role == JOURNEY_PUMP ? v3(4, 3, 5) : v3(3, 1, 5), metal, 0,
                          true);
                    piece(b, vadd(prop, v3(0, role == JOURNEY_PUMP ? 1.55 : .55, 0)),
                          v3(2.8, .12, 4.7), role == JOURNEY_PUMP ? copper : blue, .2f, false);
                }
        }
}
bool district_powered(const World *w, const DistrictMark *mark) {
    if (!mark->family)
        return false;
    Cell region = world_region(mark->anchor);
    /* Sixteen exact record-index probes; no growing history scan or planner. */
    for (int z = 0; z < 4; z++)
        for (int x = 0; x < 4; x++)
            if (world_record_systems(w, (Cell){region.x * 4 + x, region.y, region.z * 4 + z}) &
                SYSTEM_POWER)
                return true;
    return false;
}
bool district_inert(const Game *g, const Room *r, const Enemy *e) {
    if (g->world.generator < 16 || !r->journey.family || r->journey.condition != 1 ||
        campaign_guardian(r->cell) >= 0 || e->force || e->allegiance || r->frontier_active ||
        r->alarm_time > 0 || sc_enemy_bodies[e->kind].form != ROBOT)
        return false;
    return !r->district_powered || r->district_warning > 0;
}
bool district_wet(const Room *r, Vec3 at) {
    Vec3 local = vsub(at, world_center(r->cell));
    return (r->journey.role & JOURNEY_VISTA) && fabs(local.x) < 17 &&
           (!r->district_powered || fabs(local.x) > 4) && local.z > 9 &&
           local.z < fmin(37, r->depth * .5 - 4) && local.y >= -.1 && local.y < .4;
}
void district_victory(Game *g, Cell owner) {
    if (g->world.generator < 16)
        return;
    RoomSpec spec = world_describe_version(g->world.seed, g->world.generator, owner);
    if (!spec.journey.family)
        return;
    uint32_t state = world_district_state(&g->world, spec.journey.anchor);
    if (world_set_district_state(&g->world, spec.journey.anchor, state | DISTRICT_SHORTCUT))
        g->dirty = true;
}
bool district_interact(Game *g, int player) {
    if (g->world.generator < 16 || player < 0 || player >= g->player_count)
        return false;
    Player *p = &g->players[player];
    if (p->sheltered || p->health <= 0 || p->lift_active || g->matrix.divers[player].active)
        return false;
    Vec3 at = position_absolute(p->pos);
    Room *r = world_find(&g->world, world_cell(at));
    if (!r || !r->ready || !r->journey.family)
        return false;
    uint32_t state = world_district_state(&g->world, r->journey.anchor);
    if ((r->journey.role & JOURNEY_PUMP) &&
        vlen(vsub(at, district_control(r->cell, JOURNEY_PUMP))) < 2.5) {
        uint32_t old = world_systems(&g->world, r->cell);
        if (world_set_systems(&g->world, r->cell, (old | SYSTEM_POWER) & ~SYSTEM_BLACKOUT)) {
            factions_systems(g, r->cell, world_systems(&g->world, r->cell), player);
            g->dirty |= !(old & SYSTEM_POWER);
            if (campaign_guardian(r->journey.anchor) < 0)
                district_victory(g, r->cell);
            game_message(
                g, r->journey.condition == 2
                       ? "PUMP RESTORED / VENTS KEEP CYCLING / CENTER AND STAIRS REMAIN SAFE"
                   : r->journey.condition == 3
                       ? "PUMP RESTORED / AMBER WARNING: CONDUITS FEED GROWTH / CUT THE LIVING FEED"
                       : "PUMP POWER RESTORED / AMBER WARNING: DORMANT MACHINES WILL WAKE / DRY "
                         "STAIRS REMAIN");
        }
        return true;
    }
    if ((r->journey.role & JOURNEY_REFUGE) &&
        vlen(vsub(at, district_control(r->cell, JOURNEY_REFUGE))) < 2.5) {
        game_message(
            g, r->journey.condition == 2
                   ? "REFUGE / AMBER VENTS WARN FOR 3 SECONDS / CENTER AND RAISED ROUTES STAY CLEAR"
               : r->journey.condition == 3
                   ? "REFUGE / POWER FEEDS THE GROWTH / SEVER ITS FEED TO ISOLATE THE FLOOR"
                   : "WORKERS' REFUGE / POWER FEEDS BOTH SHELTER LIGHTS AND THE FOUNDRY MACHINES");
        return true;
    }
    if (!(r->journey.role & JOURNEY_VISTA) || (r->journey.role & JOURNEY_COMPACT))
        return false;
    for (unsigned side = 0; side < 2; side++) {
        Vec3 from = district_control(r->cell, side ? DISTRICT_RETURN_CONTROL : 0);
        if (vlen(vsub(at, from)) >= 2.5)
            continue;
        if (!(state & DISTRICT_SHORTCUT)) {
            game_message(g,
                         "RETURN CONDUIT SEALED / OVERCOME THE GUARDIAN OR RESTORE THE LOCAL PUMP");
            return true;
        }
        Vec3 to = district_control(r->cell, side ? 0 : DISTRICT_RETURN_CONTROL);
        if (!world_ready(&g->world, world_cell(to)) ||
            world_overlap(&g->world, to, game_body_radius(p), game_body_height(p)) ||
            !world_spawn_supported(&g->world, to, game_body_height(p)) ||
            game_rig_blocks(g, player, to, game_body_radius(p), game_body_height(p))) {
            game_message(g, "RETURN CONDUIT OBSTRUCTED / CLEAR THE OTHER LANDING");
            return true;
        }
        p->pos = position_from(to);
        p->velocity = v3(0, 0, 0);
        p->grounded = true;
        if (p->riding)
            g->rigs[player][p->riding - 1].pos = p->pos;
        g->dirty = true;
        game_message(g, "RETURN CONDUIT / THE DISTRICT REMEMBERS ITS OPEN ROUTE");
        return true;
    }
    return false;
}
void district_shot(Game *g, int player, Vec3 impact) {
    if (g->world.generator < 16 || player < 0 || player >= g->player_count ||
        g->players[player].sheltered)
        return;
    Room *r = world_find(&g->world, world_cell(impact));
    if (!r || !r->ready || !(r->journey.role & JOURNEY_VISTA) ||
        vlen(vsub(impact,
                  vadd(district_control(r->cell, DISTRICT_SOURCE_CONTROL), v3(0, .75, 0)))) > 1.6)
        return;
    uint32_t state = world_district_state(&g->world, r->journey.anchor);
    if ((state & DISTRICT_CONDUCTOR) ||
        (r->journey.condition != 2 && !district_powered(&g->world, &r->journey)))
        return;
    if (!world_set_district_state(&g->world, r->journey.anchor, state | DISTRICT_CONDUCTOR))
        return;
    r->district_arc = r->journey.condition == 1 ? 7 : 0;
    r->district_owner = (uint8_t)(player + 1);
    game_message(g, r->journey.condition == 1
                        ? "CONDUCTOR BROKEN / WATER FLASHING / TWO SECONDS TO REACH DRY GROUND"
                    : r->journey.condition == 2
                        ? "VENT FEED SEVERED / FROST SWEEP ISOLATED / CYCLE CONTINUES ELSEWHERE"
                        : "LIVING CONDUIT ISOLATED / GROWTH NO LONGER FEEDS THIS FLOOR");
    g->dirty = true;
}
void district_step(Game *g, double dt) {
    if (g->world.generator < 16)
        return;
    bool outside = false;
    for (int p = 0; p < g->player_count; p++)
        outside |= !g->players[p].sheltered;
    if (!outside)
        return;
    for (int n = 0; n < g->world.count; n++) {
        Room *r = &g->world.rooms[n];
        if (!r->ready || !r->journey.family)
            continue;
        bool powered = district_powered(&g->world, &r->journey);
        if (powered && !r->district_powered && r->journey.condition == 1)
            r->district_warning = 2;
        r->district_powered = powered;
        r->district_warning = fmaxf(0, r->district_warning - (float)dt);
        for (int p = 0; p < g->player_count; p++)
            if (!g->players[p].sheltered &&
                world_find(&g->world, world_cell(position_absolute(g->players[p].pos))) == r) {
                uint32_t state = world_district_state(&g->world, r->journey.anchor);
                if (!(state & DISTRICT_SEEN) &&
                    world_set_district_state(&g->world, r->journey.anchor, state | DISTRICT_SEEN)) {
                    g->dirty = true;
                    char notice[128];
                    snprintf(notice, sizeof(notice),
                             "%s / %s / CENTER CROSS AND RAISED ROUTE STAY SAFE",
                             district_name(r->journey.family),
                             district_condition_name(r->journey.condition));
                    game_message(g, notice);
                }
                preparation_observe(g, r);
            }
        float before = r->district_arc;
        r->district_arc = fmaxf(0, before - (float)dt);
        if (before > 5 || before <= 0 || !powered)
            continue;
        for (int p = 0; p < g->player_count; p++)
            if (!g->players[p].sheltered && !g->matrix.divers[p].active &&
                district_wet(r, position_absolute(g->players[p].pos)) &&
                position_absolute(g->players[p].pos).x < world_center(r->cell).x)
                game_hurt(g, p, (float)dt * 14, HURT_ELECTRIC);
        for (int e = 0; e < r->enemy_count; e++)
            if (r->enemies[e].alive && district_wet(r, position_absolute(r->enemies[e].pos)) &&
                r->district_owner &&
                position_absolute(r->enemies[e].pos).x < world_center(r->cell).x)
                game_damage_enemy(g, r, e, r->district_owner - 1, (float)dt * 22, 0);
    }
    condition_step(g, dt);
}
