#include "render_internal.h"

static void map_marker(Renderer *r, const MapView *map, Vec3 point, float x, float y, float tile,
                       Color color, bool edge) {
    float cx = x + (float)(point.x / SC_CELL - map->origin.x + .5) * tile;
    float cy = y + (float)(map->origin.z + map->side - point.z / SC_CELL) * tile;
    float margin = tile * .3f, side = map->side * tile;
    if (!edge && (cx < x || cx > x + side || cy < y || cy > y + side))
        return;
    cx = (float)clampd(cx, x + margin, x + side - margin);
    cy = (float)clampd(cy, y + margin, y + side - margin);
    sc_render_rect(r, cx - 3, cy - 3, 6, 6, color);
    sc_render_frame(r, cx - 5, cy - 5, 10, 10, color);
}
void sc_render_deck_map(Renderer *r, Game *g, int player, float x, float y, float size,
                        bool large) {
    Player *p = &g->players[player];
    MapView map;
    Cell center = large ? PLAYER_VIEW(g, p).map_center : world_cell(position_absolute(p->pos));
    game_map_build(g, player, center, large ? 4 : 2, &map);
    float tile = size / map.side;
    float text_scale = fmaxf(1, size / (large ? 420 : 112));
    Color panel = rgb(.018f, .035f, .043f), wall = rgb(.35f, .43f, .43f), gold = rgb(1, .7f, .25f),
          mint = rgb(.3f, 1, .75f);
    sc_render_rect(r, x - 6, y - 21, size + 12, size + 46, panel);
    sc_render_frame(r, x - 6, y - 21, size + 12, size + 46, wall);
    char label[120];
    snprintf(label, sizeof(label), large ? "DECK %+d / SCHEMATIC" : "DECK %+d / M", map.origin.y);
    sc_render_text_at(r, x, y - 15, (large ? 1.1f : .8f) * text_scale, label, mint);
    const Color colors[DISTRICT_TYPES] = {
        {.12f, .2f, .2f},   {.23f, .17f, .12f}, {.2f, .16f, .27f},
        {.27f, .22f, .13f}, {.14f, .22f, .29f}, {.17f, .25f, .22f},
        {.22f, .2f, .16f},  {.16f, .22f, .3f},  {.16f, .25f, .13f}};
    for (int z = 0; z < map.side; z++)
        for (int xx = 0; xx < map.side; xx++) {
            int at = z * map.side + xx;
            if (!map.valid[at])
                continue;
            float left = x + xx * tile, top = y + (map.side - 1 - z) * tile;
            Cell mapped = {map.origin.x + xx, map.origin.y, map.origin.z + z};
            bool locked = g->world.open_layer && campaign_layer(mapped) > g->world.open_layer;
            sc_render_rect(r, left, top, tile, tile,
                           locked ? rgb(.09f, .045f, .11f) : colors[map.rooms[at].district]);
            if (g->campaign.version && campaign_guardian(map.rooms[at].cell) >= 0) {
                int id = campaign_guardian(map.rooms[at].cell);
                sc_render_frame(r, left + tile * .3f, top + tile * .3f, tile * .4f, tile * .4f,
                                g->campaign.defeated & (1u << id) ? mint : gold);
            }
            if (!locked && map.rooms[at].habitat.pace == PACE_FRONTIER &&
                cell_equal(mapped, map.rooms[at].cell)) {
                sc_render_frame(r, left + tile * .38f, top + tile * .38f, tile * .24f, tile * .24f,
                                rgb(.9f, .9f, .8f));
                if (large)
                    sc_render_text_at(r, left + tile * .42f, top + tile * .25f, .7f * text_scale,
                                      "+", mint);
            }
            if (map.rooms[at].landmark) {
                /* Pink corner brackets distinguish discoveries from gold goals. */
                Color discovery = rgb(.86f, .42f, .63f);
                float gap = tile * .22f;
                sc_render_frame(r, left + gap, top + gap, tile - 2 * gap, tile - 2 * gap,
                                discovery);
                if (large)
                    sc_render_text_at(r, left + tile * .42f, top + tile * .39f, .8f * text_scale,
                                      "!", discovery);
            }
            unsigned nation = g->politics.version ? politics_owner(g, map.rooms[at].cell) : 0;
            if (nation) {
                Color banner = politics_color(g->world.seed, nation - 1);
                sc_render_rect(r, left + tile * .25f, top + tile * .12f, tile * .5f, 2, banner);
                if (map.rooms[at].habitat.court)
                    sc_render_text_at(r, left + tile * .4f, top + tile * .4f, .8f * text_scale, "C",
                                      banner);
            }
            unsigned doors = map.doors[at];
            const int dx[4] = {-1, 1, 0, 0}, dz[4] = {0, 0, -1, 1};
            if (map.systems[at] & SYSTEM_BEACON)
                sc_render_frame(r, left + tile * .15f, top + tile * .15f, tile * .7f, tile * .7f,
                                map.systems[at] & SYSTEM_POWER ? mint : gold);
            for (int d = 0; d < 4; d++) {
                int nx = xx + dx[d], nz = z + dz[d];
                if (nx >= 0 && nx < map.side && nz >= 0 && nz < map.side &&
                    map.valid[nz * map.side + nx] &&
                    cell_equal(map.rooms[at].cell, map.rooms[nz * map.side + nx].cell))
                    continue;
                bool open = (doors & (1 << d)) != 0;
                int pieces = open ? 2 : 1;
                float length = open ? tile * .38f : tile;
                for (int j = 0; j < pieces; j++) {
                    float shift = j * tile * .62f;
                    if (d <= EAST)
                        sc_render_rect(r, left + (d == EAST ? tile - 1.5f : 0), top + shift, 1.5f,
                                       length, wall);
                    else
                        sc_render_rect(r, left + shift, top + (d == SOUTH ? tile - 1.5f : 0),
                                       length, 1.5f, wall);
                }
            }
        }
    if (g->director.kind && g->director.site.y == map.origin.y)
        map_marker(r, &map, director_point(&g->director), x, y, tile, rgb(.3f, .85f, .95f), false);
    if (map.has_shelter && map.shelter.y == map.origin.y)
        map_marker(r, &map, position_absolute(g->shelter.door), x, y, tile, rgb(.65f, .4f, 1),
                   false);
    if (map.has_objective) {
        map_marker(r, &map, game_terminal(map.objective), x, y, tile, gold, true);
        if (map.objective.y != map.player.y && map.player.y == map.origin.y)
            map_marker(r, &map, world_lift(map.player), x, y, tile, rgb(.2f, .7f, 1), false);
        if (large)
            snprintf(label, sizeof(label), "GOAL [%d,%d] DECK %+d / %+d FROM YOU", map.objective.x,
                     map.objective.z, map.objective.y, map.objective.y - map.player.y);
        else if (map.objective.y != map.player.y)
            snprintf(label, sizeof(label), "GOAL %s %dD",
                     map.objective.y > map.player.y ? "UP" : "DOWN",
                     abs(map.objective.y - map.player.y));
        else
            snprintf(label, sizeof(label), "GOAL ON DECK");
    } else
        snprintf(label, sizeof(label), "BEACON LIT");
    if ((p->navigation.kind == NAV_DOOR || p->navigation.kind == NAV_LIFT ||
         p->navigation.kind == NAV_LANDING) &&
        map.origin.y == map.player.y)
        map_marker(r, &map, p->navigation.point, x, y, tile, rgb(.25f, .8f, .95f), false);
    sc_render_text_at(r, x, y + size + 6, .8f * text_scale, label, gold);
    for (int i = 0; i < g->player_count; i++) {
        const Player *other = &g->players[i];
        Cell c = world_cell(position_absolute(other->pos));
        if (c.y != map.origin.y || c.x < map.origin.x || c.x >= map.origin.x + map.side ||
            c.z < map.origin.z || c.z >= map.origin.z + map.side)
            continue;
        Vec3 feet = position_absolute(other->pos);
        float cx = x + (float)(feet.x / SC_CELL - map.origin.x + .5) * tile;
        float cy = y + (float)(map.origin.z + map.side - feet.z / SC_CELL) * tile;
        Color col = i == player ? mint : rgb(.4f, .65f, 1);
        double yaw = other->sheltered ? other->return_yaw : other->yaw;
        Vec3 a = v3(cx + sin(yaw) * 6, cy - cos(yaw) * 6, 0);
        Vec3 b = v3(cx + sin(yaw + 2.4) * 5, cy - cos(yaw + 2.4) * 5, 0);
        Vec3 c2 = v3(cx + sin(yaw - 2.4) * 5, cy - cos(yaw - 2.4) * 5, 0);
        sc_render_triangle(&r->mesh, a, b, c2, col, 0);
    }
    if (large) {
        sc_render_text_at(r, x, y + size + 32, .85f * text_scale,
                          "ARROWS PAN / PAGE UP-DOWN DECK / HOME RECENTER / M CLOSE", mint);
        sc_render_text_at(
            r, x, y + size + 49, .85f * text_scale,
            "GREEN YOU / CYAN NEXT / GOLD GOAL / VIOLET SHELTER / PINK DISCOVERY / TEAL WARD",
            wall);
        if (g->world.generator >= 11)
            sc_render_text_at(r, x, y + size + 65, .85f * text_scale,
                              "WHITE BEACON / OPTIONAL CHALLENGE", wall);
    }
}
static void story_journal(Renderer *r, const Game *g, int player, float w, float h) {
    const Player *p = &g->players[player];
    float s = fminf((w - 24) / 800, (h - 24) / 510), x = (w - 800 * s) * .5f,
          y = (h - 510 * s) * .5f;
    Color gold = rgb(.88f, .7f, .4f), ink = rgb(.8f, .85f, .8f), mint = rgb(.3f, .8f, .7f),
          dim = rgb(.45f, .55f, .57f);
    sc_render_rect(r, x, y, 800 * s, 510 * s, rgb(.02f, .04f, .05f));
    sc_render_frame(r, x, y, 800 * s, 510 * s, gold);
    x += 18 * s;
    char text[320];
    sc_render_text_at(r, x, y + 16 * s, 1.4f * s, "EXPEDITION / THE SHIP REMEMBERS", gold);
    for (int line = 0; line < 3; line++) {
        game_story_line(g, line, text, sizeof(text));
        sc_render_text_at(r, x, y + (40 + line * 18) * s, s, text, line == 1 ? mint : ink);
    }
    sc_render_text_at(r, x, y + 105 * s, .9f * s,
                      "CURRENT GOAL / GOLD ON YOUR MAP / U CHANGES LEAD", gold);
    sc_render_text_at(r, x, y + 123 * s, s, game_mission(g), ink);
    sc_render_text_at(r, x, y + 151 * s, .9f * s, "CONFIRMED FACTS", gold);
    for (int line = 3; line < 6; line++) {
        game_story_line(g, line, text, sizeof(text));
        sc_render_text_at(r, x, y + (169 + (line - 3) * 17) * s, .92f * s, text, ink);
    }
    snprintf(text, sizeof(text), "FIELD RECORD / L NEXT / %s",
             game_story_title(g, PLAYER_VIEW(g, p).story_reading));
    sc_render_text_at(r, x, y + 231 * s, .9f * s, text, gold);
    game_story_entry(g, PLAYER_VIEW(g, p).story_reading, text, sizeof(text));
    sc_render_wrapped(r, x, y + 250 * s, .95f * s, 760 * s, text, ink);
    sc_render_text_at(r, x, y + 294 * s, .9f * s, "UNRESOLVED LEADS / U TRACKS THE NEXT KNOWN SITE",
                      gold);
    int row = 0;
    for (unsigned n = 0; n < SC_STORY_SCENES; n++) {
        unsigned i = (g->story.tracked + n) % SC_STORY_SCENES;
        if (game_story_available(g, i) && row < 3) {
            Cell c = g->story.sites[i];
            snprintf(text, sizeof(text), "%c %s [%d,%d,%d]", i == g->story.tracked ? '>' : ' ',
                     game_story_title(g, i), c.x, c.y, c.z);
            sc_render_text_at(r, x, y + (313 + row++ * 17) * s, .9f * s, text,
                              i == g->story.tracked ? mint : dim);
        }
    }
    if (PLAYER_INTERACTION(g, p).decision_open) {
        sc_render_text_at(r, x, y + 385 * s, s, "CHOOSE HERE / 1 2 3 / GAMEPAD LEFT UP RIGHT",
                          gold);
        bool prepared = game_story_choice_ready(g, player);
        for (unsigned i = 1; i <= 3; i++) {
            snprintf(text, sizeof(text), "%u %s%s", i, game_story_choice_for(g, player, i),
                     i == 3 && !prepared ? " / LOCKED" : "");
            sc_render_text_at(r, x, y + (408 + (i - 1) * 21) * s, .9f * s, text,
                              i == 3 && !prepared ? dim : mint);
        }
        sc_render_text_at(
            r, x, y + 472 * s, .85f * s,
            prepared ? "ALL THREE OUTCOMES AVAILABLE / ENTER LEAVES UNDECIDED"
                     : "THIRD CHOICE NEEDS THE OPTIONAL OPERATION / ENTER LEAVES UNDECIDED",
            dim);
    } else {
        game_story_line(g, 6, text, sizeof(text));
        sc_render_text_at(r, x, y + 402 * s, .95f * s, text, mint);
        game_story_line(g, 7, text, sizeof(text));
        sc_render_text_at(r, x, y + 426 * s, .9f * s, text, ink);
        sc_render_text_at(r, x, y + 455 * s, .9f * s,
                          "G AT A CLEARED SITE TO INVESTIGATE OR REVIEW ITS CHOICES", dim);
    }
    sc_render_text_at(r, x, y + 491 * s, .85f * s,
                      "F6 DOSSIER / F6 AGAIN COURTS / F7 DEBRIEF / ENTER CLOSE / PAD RB PAGES",
                      gold);
}
static void history_journal(Renderer *r, const Game *g, int player, float w, float h) {
    float s = fminf((w - 24) / 800, (h - 24) / 510), x = (w - 800 * s) * .5f,
          y = (h - 510 * s) * .5f;
    Color gold = rgb(.88f, .7f, .4f), ink = rgb(.8f, .85f, .8f);
    sc_render_rect(r, x, y, 800 * s, 510 * s, rgb(.02f, .04f, .05f));
    sc_render_frame(r, x, y, 800 * s, 510 * s, gold);
    sc_render_text_at(r, x + 18 * s, y + 16 * s, 1.4f * s, "SHIP DOSSIER / TRACES OF THE COLLAPSE",
                      gold);
    char text[320];
    for (int line = 0; line < (g->story.version < 2 ? 1 : 8); line++) {
        game_story_dossier(g, line, text, sizeof(text));
        sc_render_wrapped(r, x + 18 * s, y + (52 + line * 40) * s, .88f * s, 760 * s, text, ink);
    }
    if (g->world.generator >= 10 && !g->players[player].sheltered &&
        !g->matrix.divers[player].active && position_absolute(g->players[player].pos).z >= 0) {
        RoomSpec room =
            world_describe_version(g->world.seed, g->world.generator,
                                   world_cell(position_absolute(g->players[player].pos)));
        char account[600];
        habitat_account(&room, account, sizeof(account));
        sc_render_text_at(r, x + 18 * s, y + 384 * s, .9f * s, "LOCAL TRACE / F6", gold);
        sc_render_wrapped(r, x + 18 * s, y + 404 * s, .82f * s, 760 * s, account, ink);
    } else
        sc_render_text_at(r, x + 18 * s, y + 455 * s, .9f * s,
                          "ROOM NOTICES AND WITNESS RECORDS REVEAL THE SHIP'S HISTORY", gold);
    sc_render_text_at(r, x + 18 * s, y + 486 * s, .85f * s,
                      "I EXPEDITION / F4 CHARACTER / ENTER CLOSE / PAD RB EXPEDITION", gold);
}
static void debrief_journal(Renderer *r, const Game *g, int player, float w, float h) {
    float s = fminf((w - 24) / 820, (h - 24) / 570), x = (w - 820 * s) * .5f,
          y = (h - 570 * s) * .5f;
    Color gold = rgb(.88f, .7f, .4f), ink = rgb(.8f, .85f, .8f);
    sc_render_rect(r, x, y, 820 * s, 570 * s, rgb(.02f, .04f, .05f));
    sc_render_frame(r, x, y, 820 * s, 570 * s, gold);
    sc_render_text_at(r, x + 18 * s, y + 16 * s, 1.35f * s,
                      "EXPEDITION DEBRIEF / YOUR CHOICES ENDURE", gold);
    char text[400];
    for (int line = 0; line < (g->story.version < 3 ? 1 : 10); line++) {
        game_story_debrief(g, player, line, text, sizeof(text));
        sc_render_wrapped(r, x + 18 * s, y + (48 + line * 45) * s, .95f * s, 780 * s, text, ink);
    }
    sc_render_text_at(r, x + 18 * s, y + 535 * s, .85f * s,
                      "L PREVIOUS EVENT / I EXPEDITION / F4 CHARACTER / ENTER CLOSE / PAD RB PAGES",
                      gold);
}
void sc_render_hud(Renderer *r, Game *g, int player, int width, int height, bool paused, bool touch,
                   bool debug, double fps, const char *save_status) {
    Player *p = &g->players[player];
    float w = (float)width, h = (float)height, s = fmaxf(.85f, fminf(w / 960, h / 540));
    Color ink = rgb(.78f, .80f, .72f), dim = rgb(.36f, .48f, .49f), gold = rgb(.84f, .65f, .36f),
          mint = rgb(.27f, .79f, .69f), panel = rgb(.025f, .052f, .063f);
    float matrix[16];
    sc_render_ortho(matrix, w, h);
    gl.UniformMatrix4fv(r->mvp, 1, GL_FALSE, matrix);
    gl.Uniform1i(r->ui, 1);
    gl.Uniform3f(r->offset, 0, 0, 0);
    gl.Disable(GL_DEPTH_TEST);
    if (g->matrix.divers[player].active) {
        sc_render_matrix_hud(r, g, player, w, h, paused, touch, save_status);
        sc_render_flush(r);
        return;
    }
    float pad = 24 * s;
    char label[160];
    sc_render_deck_map(r, g, player, pad, pad + 16 * s, 112 * s, false);
    Cell cell = world_cell(position_absolute(p->pos));
    Room *room = world_find(&g->world, cell);
    const char *name =
        position_absolute(p->pos).z < 0
            ? (exterior_cabin(&g->world, position_absolute(p->pos)) ? "BOARDING SKIFF"
                                                                    : "THE SILENT EXPANSE")
            : (room ? (room->landmark       ? world_landmark_name(room->landmark)
                       : room->habitat.form ? sc_habitat_forms[room->habitat.form]
                                            : sc_themes[room->theme].name)
                    : "BETWEEN DECKS");
    const char *title = p->sheltered         ? "THE FOLD / SANCTUARY"
                        : p->riding == MECH  ? "REAVER / MECH PILOT LINK"
                        : p->riding == WAGON ? "FOLD WAGON / ROAD LINK"
                                             : name;
    float header_left = w < 700 || touch ? 148 * s : 0;
    float header_width = (touch && w >= 700 ? w * .51f - 36 * s : w - 20 * s) - header_left;
    float header_center = header_left + header_width * .5f;
    sc_render_centered(r, header_center, pad, fminf(1.6f * s, header_width / (6 * strlen(title))),
                       title, gold);
    snprintf(label, sizeof(label),
             g->campaign.version ? ((onboarding_capabilities(g, player) & OB_BUILD)
                                        ? "LAYER %u / DECK %+d / F10 BUILD"
                                        : "LAYER %u / DECK %+d")
                                 : "DEPTH %03u / DECK %+d",
             position_absolute(p->pos).z < 0 ? 0
             : g->campaign.version           ? campaign_layer(cell)
                                             : world_difficulty(cell),
             cell.y);
    const char *subtitle = p->sheltered ? "SAFE / THE INFESTATION CANNOT ENTER" : label;
    sc_render_centered(r, header_center, pad + 18 * s,
                       fminf(1.1f * s, header_width / (6 * strlen(subtitle))), subtitle, dim);
    if (debug && !p->sheltered && room && position_absolute(p->pos).z >= 0) {
        snprintf(label, sizeof(label), "%s / %s", world_district_name(room->district),
                 g->world.generator >= 4 ? world_incident_name(room->incident) : "UNSURVEYED");
        if (g->world.generator >= 10)
            snprintf(label, sizeof(label), "%s / %s / F6 TRACE",
                     world_district_name(room->district),
                     sc_atmospheres[room->habitat.atmosphere].name);
        if (room->history_role)
            snprintf(label, sizeof(label), "%s / F6 DOSSIER",
                     world_history_role(room->history_role));
        sc_render_centered(r, w * .5f, pad + 33 * s, .9f * s, label, dim);
    }
    if (!p->sheltered && g->story.version >= 3 && game_story_safe(g, position_absolute(p->pos)))
        sc_render_centered(r, w * .5f, pad + 48 * s, .9f * s, "BEACON WARD / SAFE RECOVERY", mint);
    for (int i = 0; !p->sheltered && i < SC_MAX_DEATH_HAZARDS; i++) {
        const DeathHazard *hazard = &g->death_hazards[i];
        if (hazard->active && !hazard->spent &&
            vlen(position_delta(hazard->pos, p->pos)) < hazard->radius + 2) {
            sc_render_centered(r, w * .5f, h * .67f, .95f * s,
                               hazard->kind == DEATH_ACID ? "ACID RESIDUE / KEEP CLEAR"
                               : hazard->kind == DEATH_MINE && !hazard->triggered
                                   ? "DROPPED MINE / KEEP CLEAR"
                                   : "UNSTABLE CORE / MOVE AWAY",
                               rgb(1, .4f, .18f));
            break;
        }
    }
    if (g->world.pending)
        sc_render_centered(r, w * .5f, h * .8f, 1 * s, "SURVEYING NEARBY PASSAGES", dim);
    snprintf(label, sizeof(label), "SHIP %08X", g->world.seed);
    if (debug && w >= 600) {
        sc_render_text_at(r, w - pad - 13 * 7 * s, pad, 1.1f * s, label, dim);
        const char *profile = g->world.generator >= 4
                                  ? world_profile_name(world_profile(g->world.seed))
                                  : "LEGACY EXPEDITION";
        sc_render_text_at(r, w - pad - (float)strlen(profile) * 5.4f * s, pad + 18 * s, .9f * s,
                          profile, dim);
    }
    if (g->player_count == 2) {
        snprintf(label, sizeof(label), "PLAYER %d", player + 1);
        sc_render_text_at(r, w - pad - 90 * s, pad + 33 * s, 1.2f * s, label, mint);
    }
    if (!paused) {
        float cx = w * .5f, cy = h * .5f;
        sc_render_rect(r, cx - 8 * s, cy, 5 * s, 1, ink);
        sc_render_rect(r, cx + 4 * s, cy, 5 * s, 1, ink);
        sc_render_rect(r, cx, cy - 8 * s, 1, 5 * s, ink);
        sc_render_rect(r, cx, cy + 4 * s, 1, 5 * s, ink);
    }
    float by = h - 94 * s;
    sc_render_rect(r, pad - 8 * s, by - 10 * s, 270 * s, 86 * s, panel);
    snprintf(label, sizeof(label), "VITALITY  %03d / %03d", (int)ceilf(p->health),
             (int)player_max_health(p));
    sc_render_text_at(r, pad, by, 1.6f * s, label, ink);
    sc_render_rect(r, pad, by + 18 * s, 230 * s, 6 * s, rgb(.12f, .20f, .21f));
    sc_render_rect(r, pad, by + 18 * s, 230 * s * p->health / player_max_health(p), 6 * s,
                   p->hurt > 0 ? rgb(.9f, .25f, .19f) : mint);
    snprintf(label, sizeof(label), "RANK %02u  /  %u ELIMINATED", p->level, p->kills);
    sc_render_text_at(r, pad, by + 34 * s, 1.1f * s, label, gold);
    snprintf(label, sizeof(label), "SHIELD %03.0f  ARMOR %03.0f  SALVAGE %u", p->shield, p->armor,
             p->credits);
    sc_render_text_at(r, pad, by + 50 * s, .9f * s, label, dim);
    sc_render_rect(r, pad, by + 65 * s, 230 * s * p->shield / player_max_shield(p), 4 * s,
                   rgb(.25f, .5f, .9f));
    Cell objective;
    if (debug && g->story.version && !(g->story.done & 1u) && !p->sheltered)
        sc_render_centered(r, w * .5f, 77 * s, .85f * s,
                           story_commission_version(g->world.seed, g->story.version), mint);
    if (!p->sheltered && game_objective(g, &objective)) {
        if (debug) {
            snprintf(label, sizeof(label), "%s  /  [%d,%d,%d]", game_mission(g), objective.x,
                     objective.y, objective.z);
            sc_render_centered(r, w * .5f, 96 * s, .85f * s, label, gold);
        }
        if (!g->story.version &&
            vlen(vsub(position_absolute(p->pos), game_terminal(objective))) < 4)
            sc_render_centered(r, w * .5f, h * .58f, 1.2f * s, "G / USE MISSION TERMINAL", gold);
    }
    unsigned story_site = game_story_near(g, player);
    if (story_site < SC_STORY_SCENES) {
        snprintf(label, sizeof(label), "G / %s", game_story_title(g, story_site));
        sc_render_centered(r, w * .5f, h * .58f, s, label, gold);
    }
    const Enemy *aimed = NULL;
    int aimed_guardian = -1;
    double nearest = 55;
    Vec3 eye = player_eye(p), aim = facing(p->yaw, p->pitch);
    for (int j = 0; !p->sheltered && j < g->world.count; j++)
        for (int i = 0; i < g->world.rooms[j].enemy_count; i++) {
            const Enemy *e = &g->world.rooms[j].enemies[i];
            Vec3 delta = vsub(enemy_center(e), eye);
            double distance = vlen(delta);
            if (e->alive && distance < nearest && vdot(vnorm(delta), aim) > .985 &&
                world_raycast(&g->world, eye, vnorm(delta), distance) >= distance - .1) {
                aimed = e;
                aimed_guardian =
                    g->campaign.version && i == 0 ? campaign_guardian(g->world.rooms[j].cell) : -1;
                nearest = distance;
            }
        }
    if (aimed && !paused) {
        EnemyBody body = sc_enemy_bodies[aimed->kind];
        if (aimed->country && g->politics.version)
            snprintf(label, sizeof(label), "%s / %s / %.0f",
                     politics_name(g->world.seed, aimed->country - 1),
                     politics_reputation(g, aimed->country, player) < 0 ? "HOSTILE PATROL"
                                                                        : "NEUTRAL PATROL",
                     aimed->health);
        else if (aimed_guardian >= 0)
            snprintf(label, sizeof(label), "%s / %.0f",
                     campaign_guardian_name(g->world.seed, (unsigned)aimed_guardian),
                     aimed->health);
        else if (aimed->behavior == ALIEN_CONDUCTOR)
            snprintf(label, sizeof(label), "SUPPORT / BREAK THE LINKS / %.0f", aimed->health);
        else if (aimed->behavior >= ALIEN_LISTENER)
            snprintf(label, sizeof(label), "%s / %.0f", sc_enemies[aimed->kind].name,
                     aimed->health);
        else
            snprintf(label, sizeof(label), "%s / %s %s / %.0f", sc_enemies[aimed->kind].name,
                     sc_body_names[body.form], sc_body_variants[body.form][body.variant],
                     aimed->health);
        sc_render_centered(r, w * .5f, h * .43f, fminf(s, (w - 24 * s) / (6 * strlen(label))),
                           label, mint);
        if (aimed->behavior) {
            const char *hint = alien_hint(aimed->behavior);
            sc_render_centered(r, w * .5f, h * .43f + 14 * s,
                               fminf(.75f * s, (w - 24 * s) / (6 * fmax(1, strlen(hint)))), hint,
                               mint);
        }
    }
    float exp = (float)p->xp / (float)player_next_level(p);
    sc_render_rect(r, 0, h - 3, w * exp, 3, gold);
    if (!touch && w < 620) {
        snprintf(label, sizeof(label), "%s / ENERGY %.0f", game_gun(p)->name, p->energy);
        sc_render_text_at(r, pad, by - 44 * s, .9f * s, label, ink);
    } else if (!touch) {
        sc_render_text_at(r, w - 260 * s, by, 1.3f * s,
                          p->sheltered ? "WEAPONS HOLSTERED" : game_gun(p)->name, ink);
        snprintf(label, sizeof(label), "DMG %.0f  ENERGY %.0f  CORE %u",
                 game_gun(p)->damage * (1 + .35f * p->power), p->energy, p->points);
        sc_render_text_at(r, w - 260 * s, by + 19 * s, 1.1f * s, label, gold);
        sc_render_text_at(r, w - 260 * s, by + 36 * s, 1 * s,
                          p->flashlight ? "F LAMP ON / F5 SAVE / ESC PAUSE"
                                        : "F LAMP OFF / F5 SAVE / ESC PAUSE",
                          dim);
    }
    if (p->points) {
        snprintf(label, sizeof(label), "+%u UPGRADES / %s", p->points, touch ? "MENU" : "I");
        sc_render_text_at(r, pad, by - 25 * s, .9f * s, label, gold);
    }
    if (!debug)
        sc_render_navigation_hud(r, g, player, w, h, s, touch);
    sc_render_encounter_hud(r, g, player, w, h, s, touch);
    Vec3 lift = world_lift(cell), d = vsub(position_absolute(p->pos), lift);
    d.y = 0;
    if (g->world.generator >= 9 && !p->sheltered && !p->lift_active &&
        story_site >= SC_STORY_SCENES && position_absolute(p->pos).z >= 0 &&
        world_service_column(g->world.seed, cell) &&
        fabs(position_absolute(p->pos).x - world_center(cell).x) < 16 &&
        position_absolute(p->pos).z - world_center(cell).z < -5)
        sc_render_centered(r, w * .5f, h * .59f, 1.15f * s,
                           "SERVICE SHAFT / AMBER STAIRS / BLUE RAMPS", gold);
    if (p->lift_active) {
        snprintf(label, sizeof(label), "GRAV TRANSIT / DECK %d / %d%%", p->lift_to.sector.y,
                 (int)(100 * p->lift_progress));
        sc_render_centered(r, w * .5f, h * .65f, 1.7f * s, label, mint);
    } else if (!p->sheltered && position_absolute(p->pos).z >= 0 && vlen(d) < 3.3 &&
               fabs(position_absolute(p->pos).y - lift.y) < 1) {
        sc_render_centered(r, w * .5f, h * .65f, fminf(1.3f * s, (w - 32) / 240),
                           p->lift_latched ? "RELEASE LIFT CONTROL TO TRAVEL AGAIN"
                           : touch         ? "GRAV LIFT / UP OR DOWN"
                                           : "GRAV LIFT  /  E UP  /  Q DOWN",
                           mint);
        for (int side = -1; side <= 1; side += 2) {
            Cell to = cell;
            to.y += side;
            RoomSpec destination = world_describe_version(g->world.seed, g->world.generator, to);
            bool sealed = g->world.open_layer && campaign_layer(to) > g->world.open_layer;
            snprintf(label, sizeof(label), "%s %+d / %s", side > 0 ? "UP" : "DOWN", to.y,
                     sealed ? "SEALED / F10" : sc_themes[destination.theme].name);
            sc_render_centered(r, w * .5f, h * .65f + (side > 0 ? 22 : 36) * s, .85f * s, label,
                               sealed ? gold : dim);
        }
    }
    if (p->riding) {
        const Rig *rig = &g->rigs[player][p->riding - 1];
        Color amber = rgb(.95f, .62f, .22f);
        if (p->riding == MECH) {
            sc_render_frame(r, 8 * s, 8 * s, w - 16 * s, h - 16 * s, amber);
            sc_render_rect(r, 8 * s, h * .3f, 4 * s, h * .32f, amber);
            sc_render_rect(r, w - 12 * s, h * .3f, 4 * s, h * .32f, amber);
        }
        sc_render_rect(r, w * .5f - 180 * s, h - 172 * s, 360 * s, 49 * s, panel);
        snprintf(label, sizeof(label), "%s PLATING %.0f / %.0f",
                 p->riding == MECH ? "MECH" : "WAGON", rig->armor,
                 game_rig_max_armor(p, (RigKind)p->riding));
        sc_render_centered(r, w * .5f, h - 165 * s, 1.1f * s, label, amber);
        sc_render_rect(r, w * .5f - 160 * s, h - 149 * s, 320 * s, 5 * s, dim);
        sc_render_rect(r, w * .5f - 160 * s, h - 149 * s,
                       320 * s * rig->armor / game_rig_max_armor(p, (RigKind)p->riding), 5 * s,
                       amber);
        snprintf(label, sizeof(label),
                 touch ? "%.1f M/S / GUN / USE TO EXIT" : "%.1f M/S / Z-X WEAPONS / G DISMOUNT",
                 p->drive_speed);
        sc_render_centered(r, w * .5f, h - 136 * s, .9f * s, label, ink);
    } else if (game_rig_near(g, player) != ON_FOOT)
        sc_render_centered(r, w * .5f, h * .63f, 1.1f * s,
                           game_rig_near(g, player) == MECH ? "G / BOARD MECH" : "G / RIDE WAGON",
                           mint);
    ShelterUse use = game_shelter_near(g, player);
    if (use != SHELTER_NONE) {
        const char *prompts[] = {"",
                                 p->sheltered ? "G / USE TO RETURN TO SHIP"
                                              : "G / USE TO ENTER FOLD SHELTER",
                                 "G / USE TO REST IN BED",
                                 "G / USE MED STATION / HEAL AND REPAIR",
                                 "G / USE KITCHEN / COOK STARGRAIN STEW",
                                 "C OR G / LINK SHELTER MATRIX WORKSTATION"};
        sc_render_centered(r, w * .5f, h * .58f, 1.1f * s, prompts[use], mint);
    }
    if ((onboarding_capabilities(g, player) & OB_MATRIX) && matrix_near(g, player))
        sc_render_centered(r, w * .5f, h * .53f, 1.1f * s, "C / LINK TO SHIP MATRIX", mint);
    if (onboarding_interface_site(g, player))
        sc_render_centered(r, w * .5f, h * .53f, s, "USE / RECOVER MATRIX INTERFACE", mint);
    if (p->sheltered && PLAYER_VIEW(g, p).guide_mode != GUIDE_OFF)
        sc_render_centered(r, w * .5f, pad + 38 * s, .8f * s,
                           "BED AHEAD LEFT / MED STATION AHEAD RIGHT / KITCHEN ON LEFT", gold);
    if (p->resting > 0 || p->nourished > 0) {
        snprintf(label, sizeof(label),
                 p->resting > 0 ? "RESTING %.0fS / MOVE TO WAKE"
                                : "WELL FED / RECOVERY BOOST %.0fS",
                 p->resting > 0 ? p->resting : p->nourished);
        sc_render_centered(r, w * .5f, h - 137 * s, s, label, mint);
    }
    if (!touch && w >= 620 && !p->sheltered &&
        (debug || campaign_rig(g, player, MECH) || campaign_rig(g, player, WAGON)))
        sc_render_text_at(r, w - 260 * s, by + 53 * s, .9f * s, "T MECH / Y WAGON / K FLARE", dim);
    snprintf(label, sizeof(label), "V VEIL%s / B FLY %.0f / J LEAP %.0f / H NOVA %.0f",
             p->cloaked ? " ON" : "", p->flight, p->leap, p->nova);
    if (g->campaign.version) {
        char powers[4][25];
        const char *names[] = {"VEIL", "FLY", "LEAP", "NOVA"};
        float life[] = {p->cloaked ? 1 : 0, p->flight, p->leap, p->nova};
        for (unsigned i = 0; i < 4; i++)
            if (campaign_power(g, player, i))
                snprintf(powers[i], sizeof(powers[i]), "%s %.0f", names[i], life[i]);
            else
                snprintf(powers[i], sizeof(powers[i]), "%s LOCKED", names[i]);
        snprintf(label, sizeof(label), "V %s / B %s / J %s / H %s", powers[0], powers[1], powers[2],
                 powers[3]);
    }
    if (!touch && !p->riding &&
        (debug || p->cloaked || p->flight > 0 || p->leap > 0 || p->nova > 0))
        sc_render_centered(r, w * .5f, h - 158 * s, .85f * s, label,
                           p->cloaked ? rgb(.65f, .45f, 1) : mint);
    if (touch) {
        if (p->riding)
            snprintf(label, sizeof(label), "%s / ENERGY %.0f", game_gun(p)->name, p->energy);
        else if (!campaign_power(g, player, p->selected_power))
            snprintf(label, sizeof(label), "%s / ENERGY %.0f", game_gun(p)->name, p->energy);
        else
            snprintf(label, sizeof(label), "PSI: %s / %.0f ENERGY",
                     sc_psionic_names[p->selected_power], sc_psionic_costs[p->selected_power]);
        sc_render_centered(r, header_center, 56 * s,
                           fminf(.8f * s, header_width / (6 * strlen(label))), label, mint);
    }
    if (!touch && !p->sheltered && p->weapon == SC_FLARE_CANNON && p->riding != MECH) {
        snprintf(label, sizeof(label), "O BURN %us / P STRENGTH %u / EACH MAX 5",
                 30 + 15 * p->flare_lifetime, p->flare_strength);
        sc_render_centered(r, w * .5f, h - 181 * s, .85f * s, label, gold);
    }
    if (PLAYER_VIEW(g, p).message_time > 0 && !paused)
        sc_render_centered(
            r, w * .5f, g->campaign.version ? fmaxf(174 * s, h * .33f) : h * .19f,
            fminf(1.2f * s,
                  (w - 24 * s) / (6 * fmaxf(1, (float)strlen(PLAYER_VIEW(g, p).message)))),
            PLAYER_VIEW(g, p).message, gold);
    if (!onboarding_active(g) && PLAYER_VIEW(g, p).guide_mode != GUIDE_OFF && !p->sheltered &&
        position_absolute(p->pos).z < 0 && !paused)
        sc_render_centered(r, w * .5f, h * .72f, 1.2f * s,
                           touch ? "DRAG LEFT TO DRIFT / RIGHT TO AIM"
                                 : "WASD TO DRIFT / SPACE TO RISE / CTRL TO DESCEND",
                           ink);
    if (save_status && *save_status)
        sc_render_text_at(r, pad, h - 13 * s, 1 * s, save_status, dim);
    if (p->hurt > 0) {
        float border = 5 * s;
        Color red = rgb(.55f, .12f, .1f);
        sc_render_rect(r, 0, 0, w, border, red);
        sc_render_rect(r, 0, h - border, w, border, red);
        sc_render_rect(r, 0, 0, border, h, red);
        sc_render_rect(r, w - border, 0, border, h, red);
    }
    if (touch && !paused) {
        float size = fminf(w, h) * .12f;
        sc_render_frame(r, w * .13f - size * .5f, h * .73f - size * .5f, size, size, dim);
        sc_render_centered(r, w * .13f, h * .73f - 3 * s, 1.1f * s, "MOVE", dim);
        const char *names[] = {"FIRE", "JUMP", "CROUCH", "UP",   "DOWN",  "LAMP",
                               "USE",  "GUN",  "FOLD",   "CAST", "POWER", "MAP"};
        const float xs[] = {.90f, .77f, .64f, .90f, .77f, .64f, .90f, .77f, .64f, .51f, .51f, .51f},
                    ys[] = {.72f, .85f, .85f, .48f, .48f, .48f, .27f, .27f, .27f, .27f, .48f, .1f};
        for (int i = 0; i < 12; i++) {
            unsigned caps = onboarding_capabilities(g, player);
            if ((i == 7 && !(caps & OB_GUNS)) || (i == 8 && !(caps & OB_SHELTER)) ||
                ((i == 9 || i == 10) && !(caps & OB_POWERS)))
                continue;
            float x = w * xs[i], y = h * ys[i];
            sc_render_rect(r, x - size * .5f, y - size * .5f, size, size, panel);
            sc_render_frame(r, x - size * .5f, y - size * .5f, size, size, dim);
            sc_render_centered(r, x, y - 4 * s, s, names[i], mint);
        }
    }
    if (debug) {
        snprintf(label, sizeof(label), "%.0f FPS / %d CHUNKS / %d QUEUED / %u BUILT / %u UPLOADS",
                 fps, g->world.chunk_count, g->world.pending, g->world.last_work, r->last_uploads);
        sc_render_text_at(r, pad, 203 * s, .8f * s, label, mint);
        RegionPlan plan;
        world_plan_version(g->world.seed, g->world.generator, world_region(cell), &plan);
        float size = 22 * s, mx = w - pad - 4 * size, my = 100 * s;
        for (int i = 0; i < 16; i++) {
            RoomSpec *spec = &plan.rooms[plan.owner[i]];
            Color colors[] = {{.4f, .5f, .5f}, {.4f, .3f, .5f}, {.5f, .3f, .15f},
                              {.2f, .4f, .5f}, {.2f, .4f, .2f}, {.5f, .4f, .25f}};
            float x = mx + (i % 4) * size, y = my + (i / 4) * size;
            sc_render_rect(r, x, y, size - 2 * s, size - 2 * s, colors[spec->type]);
            if (plan.doors[i] & (1 << EAST))
                sc_render_rect(r, x + size - 3 * s, y + size * .4f, 5 * s, size * .2f, mint);
            if (plan.doors[i] & (1 << NORTH))
                sc_render_rect(r, x + size * .4f, y + size - 3 * s, size * .2f, 5 * s, mint);
            if (cell.x == plan.origin.x + i % 4 && cell.z == plan.origin.z + i / 4)
                sc_render_frame(r, x, y, size - 2 * s, size - 2 * s, gold);
        }
        if (room) {
            snprintf(label, sizeof(label), "%.0f X %.0f X %.0f M", room->width, room->depth,
                     room->height);
            sc_render_text_at(r, mx, my + 4 * size + 5 * s, .85f * s, label, gold);
        }
    }
    if (paused) {
        float mw = fminf(530 * s, w - 40), mx = (w - mw) * .5f, my = h * .25f;
        sc_render_rect(r, mx, my, mw, 260 * s, panel);
        sc_render_frame(r, mx, my, mw, 260 * s, rgb(.24f, .33f, .32f));
        sc_render_centered(r, w * .5f, my + 23 * s, 1.3f * s, "A SHIP WITHOUT END", gold);
        sc_render_centered(r, w * .5f, my + 52 * s, 4 * s, "STARSHIP CRAWL", ink);
        sc_render_centered(r, w * .5f, my + 100 * s, 1.5f * s, "ENTER / CLICK TO EXPLORE", mint);
        sc_render_centered(r, w * .5f, my + 135 * s, 1.1f * s, "WASD MOVE   MOUSE AIM   CLICK FIRE",
                           ink);
        sc_render_centered(r, w * .5f, my + 154 * s, 1.1f * s,
                           "SPACE JUMP   CTRL CROUCH   SHIFT SPRINT", dim);
        sc_render_centered(r, w * .5f, my + 173 * s, 1.1f * s,
                           (onboarding_capabilities(g, player) & OB_GUNS)
                               ? "E / Q LIFTS   G USE   I JOURNAL   Z / X WEAPONS"
                               : "E / Q LIFTS   G USE   F FLASHLIGHT   M MAP",
                           dim);
        sc_render_centered(r, w * .5f, my + 192 * s, 1.1f * s,
                           "F1 SETTINGS / PAD BACK   F2 CO-OP   F5 SAVE", dim);
        sc_render_centered(r, w * .5f, my + 223 * s, 1.1f * s,
                           "N + ENTER / NEW SHIP AND NEW CHARACTERS", gold);
        if (PLAYER_VIEW(g, p).message_time > 0)
            sc_render_centered(r, w * .5f, my + 242 * s, .9f * s, PLAYER_VIEW(g, p).message, mint);
    }
    if (!paused)
        sc_render_campaign_hud(r, g, player, w, h, s);
    if (!paused)
        sc_render_politics_hud(r, g, player, w, h, s);
    if (!paused)
        sc_render_onboarding_hud(r, g, player, w, h, s);
    if (PLAYER_VIEW(g, p).journal && PLAYER_VIEW(g, p).journal_page == 7)
        sc_render_onboarding_journal(r, g, player, w, h);
    else if (PLAYER_VIEW(g, p).journal && PLAYER_VIEW(g, p).journal_page == 6)
        sc_render_politics_journal_draw(r, g, player, w, h);
    else if (PLAYER_VIEW(g, p).journal && PLAYER_VIEW(g, p).journal_page == 5)
        sc_render_campaign_journal_draw(r, g, player, w, h);
    else if (PLAYER_VIEW(g, p).journal && PLAYER_VIEW(g, p).journal_page == 4)
        sc_render_matrix_journal_draw(r, g, player, w, h);
    else if (PLAYER_VIEW(g, p).journal && PLAYER_VIEW(g, p).journal_page == 3)
        debrief_journal(r, g, player, w, h);
    else if (PLAYER_VIEW(g, p).journal && PLAYER_VIEW(g, p).journal_page == 2)
        history_journal(r, g, player, w, h);
    else if (PLAYER_VIEW(g, p).journal && g->story.version && PLAYER_VIEW(g, p).journal_page)
        story_journal(r, g, player, w, h);
    else if (PLAYER_VIEW(g, p).journal) {
        float pw = fminf(w - 24, 740 * s), ph = fminf(h - 32, 450 * s), px = (w - pw) * .5f,
              py = (h - ph) * .5f;
        sc_render_rect(r, px, py, pw, ph, panel);
        sc_render_frame(r, px, py, pw, ph, gold);
        sc_render_text_at(r, px + 18 * s, py + 16 * s, 1.4f * s,
                          "RECLAIMER / CHARACTER AND MISSION JOURNAL", gold);
        snprintf(label, sizeof(label), "RANK %u  POINTS %u  SALVAGE %u  BEACONS %u", p->level,
                 p->points, p->credits, p->victories);
        sc_render_text_at(r, px + 18 * s, py + 40 * s, s, label, ink);
        for (int i = 0; i < SC_ATTRIBUTES; i++) {
            if (!onboarding_action(g, player, 20 + i))
                continue;
            unsigned level = *player_attribute(p, i);
            snprintf(label, sizeof(label), "%d %s %u / COST %u", (i + 1) % 10, sc_attributes[i],
                     level, 60 + level * 25);
            sc_render_text_at(r, px + 18 * s + (i / 5) * pw * .5f, py + (64 + (i % 5) * 20) * s, s,
                              label, mint);
        }
        sc_render_text_at(r, px + 18 * s, py + 160 * s, .8f * s,
                          "UPGRADE WITH 1 POINT OR SALVAGE / F4 EXPEDITION PAGE", dim);
        sc_render_text_at(r, px + 18 * s, py + 175 * s, s, game_mission(g), gold);
        if (game_objective(g, &objective)) {
            Cell here = world_cell(position_absolute(p->pos));
            snprintf(label, sizeof(label), "TARGET [%d,%d,%d]  DELTA [%d,%d,%d] / G AT TERMINAL",
                     objective.x, objective.y, objective.z, objective.x - here.x,
                     objective.y - here.y, objective.z - here.z);
            sc_render_text_at(r, px + 18 * s, py + 195 * s, .85f * s, label, ink);
        }
        snprintf(label, sizeof(label),
                 "CAREER %u ARCS / %u RESCUED / %u KNOWLEDGE / %u TECH / %u CONTAINED",
                 p->career[4], p->career[0], p->career[1], p->career[2], p->career[3]);
        sc_render_text_at(r, px + 18 * s, py + 345 * s, .75f * s, label, mint);
        snprintf(label, sizeof(label), "CONTRACTS / PURGE %u/12  SALVAGE %u/8  SYSTEMS %u/3",
                 g->mission_kills, g->mission_loot, g->mission);
        sc_render_text_at(r, px + 18 * s, py + 218 * s, .85f * s, label, mint);
        sc_render_text_at(r, px + 18 * s, py + 240 * s, s,
                          "Z / X SWITCH WEAPONS / EACH USES RECHARGING ENERGY", dim);
        snprintf(label, sizeof(label), "EQUIPPED: %s / ARMORY:", sc_weapons[p->weapon].name);
        sc_render_text_at(r, px + 18 * s, py + 262 * s, s, label, gold);
        int row = 0;
        for (int i = 0; i < SC_WEAPONS; i++)
            if (p->weapons & (1u << i)) {
                if (row < 16)
                    sc_render_text_at(r, px + 18 * s + (row / 4) * pw * .25f,
                                      py + (282 + (row % 4) * 17) * s, .8f * s, sc_weapons[i].name,
                                      ink);
                row++;
            }
        unsigned logs = 0;
        for (int i = 0; i < 12; i++)
            logs += (p->logs >> i) & 1;
        snprintf(label, sizeof(label), "%d WEAPONS / %u RECORDS / L NEXT LOG / ENTER CLOSE", row,
                 logs);
        sc_render_text_at(r, px + 18 * s, py + 365 * s, .9f * s, label, dim);
        int reading = PLAYER_VIEW(g, p).reading;
        if (!(p->logs & (1u << reading)))
            for (int i = 0; i < 12; i++)
                if (p->logs & (1u << i)) {
                    reading = i;
                    break;
                }
        if (p->logs)
            sc_render_text_at(r, px + 18 * s, py + 390 * s, .72f * s, sc_logs[reading], gold);
    }
    if (p->cloaked && !paused)
        sc_render_frame(r, 3, 3, w - 6, h - 6, rgb(.35f, .2f, .6f));
    if (PLAYER_VIEW(g, p).map_open) {
        float size = fminf(w - 90 * s, h - (touch ? 185 : 150) * s);
        sc_render_rect(r, 12 * s, 12 * s, w - 24 * s, h - 24 * s, panel);
        sc_render_centered(r, w * .5f, 25 * s, 1.2f * s, "SHIP CARTOGRAPH / NORTH UP", gold);
        sc_render_deck_map(r, g, player, (w - size) * .5f, (touch ? 85 : 65) * s, size, true);
        if (touch) {
            float cx = w * .51f, cy = h * .1f;
            sc_render_rect(r, cx - 30, cy - 10, 60, 20, panel);
            sc_render_frame(r, cx - 30, cy - 10, 60, 20, mint);
            sc_render_centered(r, cx, cy - 4, .9f, "CLOSE", mint);
        }
    }
    sc_render_flush(r);
}
void renderer_options(Renderer *r, int width, int height, int player, int selected) {
    if (width < 80 || height < 80)
        return;
    float s = fminf((width - 20) / 650.0f, (height - 20) / 420.0f);
    float x = (width - 650 * s) * .5f, y = (height - 420 * s) * .5f, matrix[16];
    Color gold = rgb(.9f, .72f, .4f), ink = rgb(.76f, .84f, .8f), dim = rgb(.4f, .55f, .56f);
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl.Viewport(0, 0, width, height);
    sc_render_ortho(matrix, (float)width, (float)height);
    gl.UniformMatrix4fv(r->mvp, 1, GL_FALSE, matrix);
    gl.Uniform1i(r->ui, 1);
    gl.Uniform3f(r->offset, 0, 0, 0);
    gl.Disable(GL_DEPTH_TEST);
    sc_render_rect(r, x, y, 650 * s, 420 * s, rgb(.015f, .03f, .04f));
    sc_render_frame(r, x, y, 650 * s, 420 * s, gold);
    sc_render_text_at(r, x + 22 * s, y + 20 * s, 1.5f * s, "CONTROLS AND DISPLAY", gold);
    const char *labels[] = {
        "PLAYER",         "MOUSE SENSITIVITY",    "TOUCH SENSITIVITY", "STICK SENSITIVITY",
        "STICK DEADZONE", "INVERT VERTICAL LOOK", "WORLD RESOLUTION",  "GUIDANCE",
        "FIELD GUIDE",    "DISMISS HINT",         "RESET THIS PLAYER"};
    for (int row = 0; row < 11; row++) {
        float yy = y + (58 + row * 28) * s;
        if (row == selected)
            sc_render_rect(r, x + 12 * s, yy - 8 * s, 626 * s, 30 * s, rgb(.06f, .12f, .13f));
        sc_render_text_at(r, x + 24 * s, yy, s, labels[row], row == selected ? gold : ink);
        char value[64] = "LEFT / RIGHT TO RESET";
        if (row == 0)
            snprintf(value, sizeof(value), "%d", player + 1);
        else if (row < 5)
            snprintf(value, sizeof(value), "%.2f",
                     preferences_get(&r->preferences, player, row - 1));
        else if (row == 5)
            snprintf(value, sizeof(value), "%s",
                     r->preferences.view[player].inverted ? "ON" : "OFF");
        else if (row == 6)
            snprintf(value, sizeof(value), "%s",
                     preferences_quality(r->preferences.view[player].quality));
        else if (row == 7)
            snprintf(value, sizeof(value), "%s",
                     onboarding_mode(r->preferences.view[player].guidance));
        else if (row == 8 || row == 9)
            snprintf(value, sizeof(value), "LEFT / RIGHT TO SELECT");
        sc_render_text_at(r, x + 380 * s, yy, s, value, row == selected ? gold : ink);
    }
    sc_render_text_at(r, x + 22 * s, y + 372 * s, .85f * s,
                      "ARROWS / DPAD SELECT AND ADJUST / ENTER OR START RESUMES", dim);
    sc_render_text_at(r, x + 22 * s, y + 394 * s, .85f * s,
                      "F1 OPTIONS / DEVICE SETTINGS ARE SEPARATE FROM SHIP SAVES", dim);
    sc_render_flush(r);
}
