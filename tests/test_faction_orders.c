#include "faction_fixture.h"
static void autonomy(void) {
    unsigned launched = 0, changes = 0;
    for (unsigned seed = 1; seed <= 9; seed++) {
        Game *a = fixture(seed), *b = fixture(seed);
        Cell start = ordinary(a, (Cell){1, 1, 2});
        load_area(a, start);
        load_area(b, start);
        for (unsigned tick = 0; tick < 300; tick++) {
            factions_step(a, 2);
            factions_step(b, 2);
            CHECK(a->factions.random == b->factions.random &&
                  a->factions.launched == b->factions.launched);
            CHECK(!a->factions.director.sequence && factions_valid(&a->factions, 14));
        }
        launched += a->factions.launched;
        for (unsigned i = 0; i < SC_FACTION_EVENTS; i++)
            changes += a->factions.events[i].kind == FE_RELATION;
        CHECK(a->factions.tick == 300 && a->factions.decisions > 0);
        for (unsigned c = 0; c < 3; c++)
            for (unsigned r = 0; r < 3; r++)
                CHECK(a->factions.nations[c].supplies[r] < 1000);
        finish(a);
        finish(b);
    }
    CHECK(launched > 15 && changes > 0);
}
static void hostility(void) {
    Game *g = fixture(713);
    Cell site = ordinary(g, (Cell){2, 0, 4});
    load_area(g, site);
    empty_actors(g);
    factions_record(g, site)->owner = 1;
    CHECK(factions_launch(g, 1, ORDER_PATROL, site, site));
    FactionForce *a = factions_force(g, 1);
    Enemy *e = &a->members[0];
    factions_attack(g, e, 0, 1);
    CHECK(!factions_hostile(g, e, 0));
    factions_attack(g, e, 0, 1);
    CHECK(g->factions.incidents[0][0].strikes == 1);
    factions_incidents(g, 1);
    factions_attack(g, e, 0, 1);
    CHECK(factions_hostile(g, e, 0));
    CHECK(!factions_hostile(g, e, 1));
    factions_incidents(g, 5);
    CHECK(g->factions.incidents[0][0].reports == 1);
    CHECK(world_set_systems(&g->world, site, SYSTEM_SILENCED));
    factions_systems(g, site, SYSTEM_SILENCED, 0);
    CHECK(a->wait >= 6 && g->factions.nations[0].next_order == g->factions.tick + 30);
    unsigned events = g->factions.event_count;
    factions_systems(g, site, SYSTEM_SILENCED, 0);
    CHECK(g->factions.event_count == events);
    factions_attack(g, e, 0, 10000);
    factions_incidents(g, 5);
    CHECK(g->factions.incidents[0][0].reports == 1);
    CHECK(world_set_systems(&g->world, site, 0));
    for (int i = 0; i < 2; i++) {
        factions_attack(g, e, 0, 10000);
        factions_incidents(g, 5);
    }
    CHECK(g->factions.incidents[0][0].reports == 3);
    Enemy distant = *e;
    distant.pos = position_from(vadd(world_center((Cell){40, 0, 40}), v3(0, 1, 0)));
    CHECK(factions_hostile(g, &distant, 0));
    for (int i = 0; i < 4; i++)
        factions_incidents(g, 60);
    CHECK(!factions_hostile(g, &distant, 0));
    factions_attack(g, e, -1, 2);
    factions_attack(g, e, 2, 2);
    factions_attack(g, e, 0, 0);
    e->alive = false;
    factions_attack(g, e, 0, 10);
    factions_systems(g, site, 99, 0);
    factions_systems(g, site, SYSTEM_POWER, 0);
    CHECK((factions_at(&g->factions, site)->flags & FS_REPAIRED) &&
          !(factions_at(&g->factions, site)->flags & FS_DARK));
    finish(g);
}
static Cell deployment(unsigned government, bool shortage, unsigned serial) {
    Game *g = fixture(713);
    g->world.generator = 15; /* Original immediate orders retain their resource policy. */
    unsigned country = 1;
    while (politics_government(g->world.seed, country - 1) != government)
        country++;
    Cell start = ordinary(g, (Cell){4, 0, 4});
    load_area(g, start);
    Cell region = world_region(start);
    Cell cells[3] = {start,
                     {(region.x - 1) * 4, region.y, region.z * 4},
                     {(region.x + 1) * 4, region.y, region.z * 4}};
    if (government == DIVIDED_CROWN) {
        cells[1] = (Cell){region.x * 4, region.y - 1, region.z * 4};
        cells[2] = (Cell){region.x * 4, region.y + 1, region.z * 4};
    }
    for (unsigned i = 0; i < 3; i++) {
        Cell site = ordinary(g, cells[i]);
        CHECK(cell_equal(world_region(site), world_region(cells[i])));
        FactionRegion *r = factions_record(g, cells[i]);
        r->site = site;
        r->owner = i ? country % 3 + 1 : country;
        r->kind = i == 1 && government != DIVIDED_CROWN ? SITE_BIOMASS : SITE_POWER;
        cells[i] = site;
    }
    for (unsigned c = 0; c < 3; c++) {
        g->factions.nations[c].next_order = c + 1 == country ? 0 : UINT32_MAX;
        g->factions.nations[c].next_relation = UINT32_MAX;
        g->factions.nations[c].cycle = 1;
        for (unsigned b = 0; b < 3; b++)
            g->factions.relations[c][b] = c == b ? REL_PEACE : REL_WAR;
    }
    if (shortage)
        g->factions.nations[country - 1].supplies[0] = 4;
    g->factions.tick = 1;
    g->factions.next_id = serial;
    factions_orders(g);
    FactionForce *a = factions_force(g, serial);
    CHECK(a && a->country == country && a->order == ORDER_RAID);
    Cell selected = a->goal;
    CHECK(cell_equal(selected, cells[1]) || cell_equal(selected, cells[2]));
    if (government == MOLTING_COURT && !shortage)
        CHECK(cell_equal(selected, cells[1]));
    else if (government != DIVIDED_CROWN)
        CHECK(cell_equal(selected, cells[2]));
    else
        CHECK(selected.y == start.y + (serial & 1 ? 1 : -1));
    CHECK(factions_valid(&g->factions, 14));
    finish(g);
    return selected;
}
static void doctrines_and_treaties(void) {
    deployment(MOLTING_COURT, false, 1);
    deployment(ECHO_PARLIAMENT, false, 1);
    deployment(MOLTING_COURT, true, 1);
    deployment(DIVIDED_CROWN, false, 1);
    deployment(DIVIDED_CROWN, false, 2);
    Game *g = fixture(713);
    for (unsigned c = 0; c < 3; c++) {
        g->factions.nations[c].next_order = UINT32_MAX;
        g->factions.nations[c].next_relation = UINT32_MAX;
        for (unsigned b = 0; b < 3; b++)
            g->factions.relations[c][b] = REL_PEACE;
    }
    g->factions.relations[0][1] = g->factions.relations[1][0] = REL_TRIBUTE;
    unsigned before[3];
    for (unsigned r = 0; r < 3; r++)
        before[r] = g->factions.nations[0].supplies[r] + g->factions.nations[1].supplies[r];
    g->factions.tick = 30;
    factions_orders(g);
    CHECK(g->factions.nations[0].supplies[2] == 197 && g->factions.nations[1].supplies[2] == 203);
    for (unsigned r = 0; r < 3; r++)
        CHECK(g->factions.nations[0].supplies[r] + g->factions.nations[1].supplies[r] == before[r]);
    g->factions.relations[0][1] = g->factions.relations[1][0] = REL_SYMBIOSIS;
    g->factions.nations[1].supplies[0] = 250;
    g->factions.tick = 60;
    factions_orders(g);
    CHECK(g->factions.nations[0].supplies[0] == 203 && g->factions.nations[1].supplies[0] == 247);
    g->factions.nations[0].next_relation = 0;
    g->factions.nations[0].losses = 6;
    factions_orders(g);
    CHECK(g->factions.nations[0].ruler == 1);
    uint32_t relation[3][3];
    memcpy(relation, g->factions.relations, sizeof(relation));
    CHECK(g->factions.nations[0].next_relation >= g->factions.tick + 120);
    for (unsigned i = 0; i < 20; i++) {
        g->factions.tick++;
        factions_orders(g);
        CHECK(g->factions.nations[0].ruler == 1 &&
              !memcmp(relation, g->factions.relations, sizeof(relation)));
    }
    CHECK(game_save(g, "succession.sav") && game_load(g, "succession.sav"));
    CHECK(g->factions.nations[0].ruler == 1 &&
          !memcmp(relation, g->factions.relations, sizeof(relation)));
    remove("succession.sav");
    finish(g);
}
int main(void) {
    autonomy();
    hostility();
    doctrines_and_treaties();
    puts(
        "PASS autonomous deployments, deterministic wars, graded incidents and relay interference");
}
