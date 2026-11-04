#include "world.h"
#include <string.h>
#include <stdlib.h>

/* Fixed-width unsigned arithmetic and category salts: no mutable RNG state. */
uint32_t world_hash(uint32_t seed, Cell c, uint32_t salt) {
    uint32_t h = seed ^ salt;
    h ^= (uint32_t)c.x * UINT32_C(0x9e3779b9);
    h ^= (uint32_t)c.y * UINT32_C(0x85ebca6b);
    h ^= (uint32_t)c.z * UINT32_C(0xc2b2ae35);
    h ^= h >> 16;
    h *= UINT32_C(0x7feb352d);
    h ^= h >> 15;
    h *= UINT32_C(0x846ca68b);
    return h ^ (h >> 16);
}
bool cell_equal(Cell a, Cell b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
static int32_t floor_div(int32_t n, int32_t d) {
    return n / d - (n % d < 0);
}
Cell world_region(Cell c) {
    return (Cell){floor_div(c.x, 4), c.y, floor_div(c.z, 4)};
}
Cell world_cell(Vec3 p) {
    return (Cell){(int32_t)clampd(floor((p.x + SC_CELL / 2) / SC_CELL), -10000000, 10000000),
                  (int32_t)clampd(floor((p.y + .001) / SC_DECK), -10000000, 10000000),
                  (int32_t)clampd(floor(p.z / SC_CELL), 0, 10000000)};
}
Vec3 world_center(Cell c) {
    return v3(c.x * SC_CELL, c.y * SC_DECK, c.z * SC_CELL + SC_CELL / 2);
}
Position position_from(Vec3 absolute) {
    Cell sector = {(int32_t)floor((absolute.x + 48) / SC_CELL),
                   (int32_t)floor(absolute.y / SC_DECK), (int32_t)floor(absolute.z / SC_CELL)};
    return (Position){sector, vsub(absolute, world_center(sector))};
}
Vec3 position_absolute(Position p) {
    return vadd(world_center(p.sector), p.local);
}
Vec3 position_delta(Position a, Position b) {
    return vadd(v3(((double)a.sector.x - b.sector.x) * SC_CELL,
                   ((double)a.sector.y - b.sector.y) * SC_DECK,
                   ((double)a.sector.z - b.sector.z) * SC_CELL),
                vsub(a.local, b.local));
}
void position_move(Position *p, Vec3 delta) {
    p->local = vadd(p->local, delta);
    int32_t x = (int32_t)floor((p->local.x + 48) / SC_CELL),
            y = (int32_t)floor(p->local.y / SC_DECK),
            z = (int32_t)floor((p->local.z + 48) / SC_CELL);
    p->sector.x += x;
    p->sector.y += y;
    p->sector.z += z;
    p->local = vsub(p->local, v3(x * SC_CELL, y * SC_DECK, z * SC_CELL));
}
Vec3 world_room_center(const Room *r) {
    return vadd(world_center(r->cell),
                v3((r->span_x - 1) * SC_CELL / 2, 0, (r->span_z - 1) * SC_CELL / 2));
}
Vec3 world_lift(Cell c) {
    return vadd(world_center(c), v3(0, .02, -10));
}
unsigned world_difficulty(Cell c) {
    double d = sqrt((double)c.x * c.x + (double)c.y * c.y * 4 / 9 + (double)c.z * c.z);
    return !c.x && !c.y && !c.z ? 0 : (unsigned)fmax(1, fmin(d * 1.5, 10000));
}
const char *world_room_name(RoomType t) {
    static const char *names[] = {"THE THRESHOLD",   "CATHEDRAL NAVE", "REACTOR CHOIR",
                                  "CREW SEPULCHRES", "THE UNDERCROFT", "GILDED ATRIUM"};
    return t >= BAY && t < ROOM_TYPES ? names[t] : "UNKNOWN";
}
const char *world_district_name(District d) {
    static const char *names[] = {"HABITATION",       "MACHINERY", "SANCTUM",
                                  "LUXURY RESORT",    "COMMAND",   "RESEARCH",
                                  "CARGO AND SUPPLY", "CRYONICS",  "BOTANICAL"};
    return d >= HABITAT && d < DISTRICT_TYPES ? names[d] : "UNSURVEYED";
}
Cell world_neighbor(Cell c, Direction d) {
    if (d == WEST)
        c.x--;
    if (d == EAST)
        c.x++;
    if (d == SOUTH)
        c.z--;
    if (d == NORTH)
        c.z++;
    return c;
}
static bool bay_edge(Cell a, Cell b) {
    return (!a.x && !a.y && !a.z) || (!b.x && !b.y && !b.z);
}
static bool boundary(uint32_t seed, uint32_t generator, Cell c, Direction d) {
    Cell n = world_neighbor(c, d);
    if (n.z < 0)
        return false;
    if (bay_edge(c, n))
        return true;
    /* Both sides hash the lower region and axis of their shared face. */
    Cell r = world_region(d == WEST || d == SOUTH ? n : c);
    uint32_t h = world_hash(seed ^ generator, r, d <= EAST ? 801 : 802);
    int lane = d <= EAST ? c.z % 4 : c.x - floor_div(c.x, 4) * 4;
    int first = (int)(h % 4), second = (first + 1 + (int)((h >> 8) % 3)) % 4;
    return lane == first || lane == second;
}
static int root(int *parent, int i) {
    while (parent[i] != i)
        i = parent[i];
    return i;
}
ShipProfile world_profile(uint32_t seed) {
    return (ShipProfile)(world_hash(seed, (Cell){0, 0, 0}, 41001) % SHIP_PROFILES);
}
const char *world_profile_name(ShipProfile profile) {
    static const char *names[] = {"PILGRIM ARK", "CROWN LINER", "SIEGE CARRIER", "SEED VAULT"};
    return names[(unsigned)profile % SHIP_PROFILES];
}
const char *world_layout_name(unsigned layout) {
    static const char *names[] = {"ENCLOSED SUITES", "STAGGERED WORK BAYS", "OPEN ARCADES"};
    return names[layout % INTERIOR_LAYOUTS];
}
const char *world_incident_name(unsigned incident) {
    static const char *names[] = {"DORMANT",    "BLACKOUT",   "CRYO LEAK",
                                  "QUARANTINE", "BURN SCARS", "BLOOM"};
    return names[incident % INCIDENT_TYPES];
}
const char *world_landmark_name(unsigned landmark) {
    static const char *names[] = {"", "THE LAST BANQUET", "SURVIVOR CONSERVATORY",
                                  "THE HANGING HEART", "RELIQUARY OF VOICES"};
    return names[landmark % LANDMARK_TYPES];
}
static void history(RegionPlan *p, uint32_t hash, int preferred, unsigned incident) {
    /* Distances are over logical rooms, so a hall is one connected incident.
       The safe bay is excluded from propagation as well as decoration. */
    int distance[16], queue[16], head = 0, tail = 1;
    for (int i = 0; i < p->count; i++)
        distance[i] = 99;
    int source = preferred >= 0 ? preferred : (int)(hash % (unsigned)p->count);
    if (p->rooms[source].type == BAY)
        source = (source + 1) % p->count;
    distance[source] = 0;
    queue[0] = source;
    while (head < tail) {
        int a = queue[head++];
        for (int i = 0; i < 16; i++) {
            if (p->owner[i] != a)
                continue;
            for (int d = 0; d < 4; d++) {
                int x = i % 4 + (d == WEST ? -1 : d == EAST ? 1 : 0);
                int z = i / 4 + (d == SOUTH ? -1 : d == NORTH ? 1 : 0);
                if (x < 0 || x >= 4 || z < 0 || z >= 4 || !(p->doors[i] & (1 << d)))
                    continue;
                int b = p->owner[z * 4 + x];
                if (distance[b] == 99 && p->rooms[b].type != BAY) {
                    distance[b] = distance[a] + 1;
                    queue[tail++] = b;
                }
            }
        }
    }
    for (int i = 0; i < p->count; i++)
        if (distance[i] < 4) {
            p->rooms[i].incident = (uint8_t)incident;
            p->rooms[i].intensity = (uint8_t)(distance[i] < 2 ? 3 - distance[i] : 1);
        }
}
void world_plan_version(uint32_t seed, uint32_t generator, Cell region, RegionPlan *p) {
    /* Version 6 expands encounters; its architecture and historical sites stay on version 5. */
    if (generator == 6)
        generator = 5;
    memset(p, 0, sizeof(*p));
    p->origin = (Cell){region.x * 4, region.y, region.z * 4};
    uint32_t h = world_hash(seed ^ generator, region, 101);
    ShipHistory past = world_history(seed);
    int layer = ((region.y % DISTRICT_TYPES) + DISTRICT_TYPES) % DISTRICT_TYPES;
    p->district =
        (District)((layer + (h % 4 == 0 ? (h >> 14) % DISTRICT_TYPES : 0)) % DISTRICT_TYPES);
    if (generator >= 4) {
        static const District decks[SHIP_PROFILES][DISTRICT_TYPES] = {
            {SANCTUM, HABITAT, CRYONICS, COMMAND, MACHINERY, RESEARCH, LUXURY, LOGISTICS, BOTANY},
            {LUXURY, HABITAT, BOTANY, COMMAND, RESEARCH, SANCTUM, MACHINERY, LOGISTICS, CRYONICS},
            {LOGISTICS, MACHINERY, COMMAND, HABITAT, RESEARCH, CRYONICS, SANCTUM, BOTANY, LUXURY},
            {BOTANY, RESEARCH, CRYONICS, HABITAT, MACHINERY, SANCTUM, LUXURY, COMMAND, LOGISTICS}};
        p->district = decks[world_profile(seed)][p->district];
    }
    if (generator >= 5 && (h >> 11) % 4 == 0)
        p->district = sc_themes[world_history_theme(past.cause)].sector;
    if (!region.x && !region.y && !region.z)
        p->district = SANCTUM;
    int hx = (int)((h >> 5) % 3), hz = (int)((h >> 9) % 3);
    if (!region.x && !region.z) {
        hx = 1;
        hz = 1;
    }
    if (!region.x && !region.y && !region.z) {
        hx = 0;
        hz = 1;
    }
    /* Keep the introductory crew room outside a large footprint too. */
    if (region.x == -1 && !region.y && !region.z) {
        hx = 0;
        hz = 1;
    }
    for (int z = 0; z < 4; z++)
        for (int x = 0; x < 4; x++) {
            if (x >= hx && x < hx + 2 && z >= hz && z < hz + 2 && (x != hx || z != hz))
                continue;
            int id = p->count++;
            RoomSpec *r = &p->rooms[id];
            r->cell = (Cell){p->origin.x + x, region.y, p->origin.z + z};
            r->hash = world_hash(seed ^ generator, r->cell, 123);
            r->district = p->district;
            r->span_x = r->span_z = (uint8_t)(x == hx && z == hz ? 2 : 1);
            unsigned choices[SC_THEMES], count = 0;
            for (unsigned t = 1; t < SC_THEMES; t++)
                if (sc_themes[t].sector == p->district)
                    choices[count++] = t;
            r->theme = (uint8_t)(count ? choices[r->hash % count] : 1);
            if (generator >= 5 && ((r->hash >> 18) % 5 == 0 || cell_equal(r->cell, past.origin)))
                r->theme = (uint8_t)world_history_theme(past.cause);
            if (generator >= 5 && cell_equal(r->cell, past.origin)) {
                r->history_role = HISTORY_ORIGIN;
                r->district = sc_themes[r->theme].sector;
            }
            r->type = sc_themes[r->theme].architecture;
            if (r->span_x == 2 && r->type == CREW)
                r->type = GARDEN;
            if (!r->cell.x && !r->cell.y && !r->cell.z)
                r->type = BAY;
            if (!r->cell.x && !r->cell.y && r->cell.z == 1)
                r->type = NAVE;
            if (r->cell.x == -1 && !r->cell.y && !r->cell.z)
                r->type = CREW;
            if (r->cell.x == 1 && !r->cell.y && !r->cell.z)
                r->type = ENGINE;
            if (!r->cell.x && r->cell.y == 1 && !r->cell.z)
                r->type = GARDEN;
            if (!r->cell.x && r->cell.y == -1 && !r->cell.z)
                r->type = CONDUIT;
            /* The arrival routes retain their familiar architectural examples. */
            if ((!r->cell.x && !r->cell.z) || (!r->cell.y && !r->cell.z && abs(r->cell.x) == 1) ||
                (!r->cell.x && !r->cell.y && r->cell.z == 1))
                r->theme = (uint8_t)r->type;
            static const double sizes[][3] = {{84, 92, 28}, {82, 84, 52},  {64, 72, 36},
                                              {76, 80, 8},  {7, 68, 1.55}, {78, 80, 36}};
            r->width = sizes[r->type][0];
            r->depth = sizes[r->type][1];
            r->height = sizes[r->type][2];
            if (generator >= 4) {
                r->layout = (uint8_t)(((r->hash >> 22) + world_profile(seed)) % INTERIOR_LAYOUTS);
                if (r->span_x == 2 && (h >> 18) % 4 == 0 && (region.x || region.y || region.z))
                    r->landmark = (uint8_t)(p->district == LUXURY      ? BANQUET
                                            : p->district == BOTANY    ? CONSERVATORY
                                            : p->district == MACHINERY ? SUSPENDED_ENGINE
                                            : p->district == SANCTUM   ? RELIQUARY
                                                                       : 1 + (r->hash >> 24) % 4);
            }
            if (r->landmark)
                r->layout = ARCADES;
            if (r->span_x == 2) {
                r->width = 180;
                r->depth = 180;
                r->height = 58;
            } else if (r->type != BAY && r->type != CONDUIT) {
                r->width -= (r->hash >> 12) % 4 * 2;
                r->depth -= (r->hash >> 16) % 4 * 2;
            }
            for (int dz = 0; dz < r->span_z; dz++)
                for (int dx = 0; dx < r->span_x; dx++)
                    p->owner[(z + dz) * 4 + x + dx] = (uint8_t)id;
        }
    struct Edge {
        int a, b;
        Direction dir;
        uint32_t key;
    } edges[24];
    int count = 0, parent[16];
    for (int i = 0; i < p->count; i++)
        parent[i] = i;
    for (int z = 0; z < 4; z++)
        for (int x = 0; x < 4; x++) {
            int i = z * 4 + x;
            Cell c = {p->origin.x + x, region.y, p->origin.z + z};
            for (int d = 0; d < 4; d++) {
                bool edge = (d == WEST && !x) || (d == EAST && x == 3) || (d == SOUTH && !z) ||
                            (d == NORTH && z == 3);
                if (edge && boundary(seed, generator, c, (Direction)d))
                    p->doors[i] |= (uint8_t)(1 << d);
            }
            if (x < 3)
                edges[count++] =
                    (struct Edge){i, i + 1, EAST, world_hash(seed ^ generator, c, 301)};
            if (z < 3)
                edges[count++] =
                    (struct Edge){i, i + 4, NORTH, world_hash(seed ^ generator, c, 302)};
        }
    /* Stable Kruskal over footprints provides a reachable backbone plus loops. */
    for (int i = 1; i < count; i++) {
        struct Edge e = edges[i];
        int j = i;
        while (j && edges[j - 1].key > e.key) {
            edges[j] = edges[j - 1];
            j--;
        }
        edges[j] = e;
    }
    for (int i = 0; i < count; i++) {
        struct Edge e = edges[i];
        int a = p->owner[e.a], b = p->owner[e.b];
        int ra = root(parent, a), rb = root(parent, b);
        Cell ca = {p->origin.x + e.a % 4, region.y, p->origin.z + e.a / 4};
        if (a == b || ra != rb || e.key % 4 == 0 || bay_edge(ca, world_neighbor(ca, e.dir))) {
            p->doors[e.a] |= (uint8_t)(1 << e.dir);
            p->doors[e.b] |= (uint8_t)(1 << (e.dir ^ 1));
            parent[ra] = rb;
        }
    }
    if (generator >= 4) {
        uint32_t damage = world_hash(seed, region, 42001);
        int source = -1;
        unsigned incident = 1 + (damage >> 8) % (INCIDENT_TYPES - 1);
        if (generator >= 5) {
            unsigned origin_theme = world_history_theme(past.cause);
            for (int i = 0; i < p->count; i++)
                if (p->rooms[i].history_role == HISTORY_ORIGIN) {
                    source = i;
                    break;
                } else if (p->rooms[i].theme == origin_theme)
                    source = i;
            incident = (unsigned)world_history_incident(past.cause);
            if (!cell_equal(region, world_region(past.origin)) && damage % 4 == 0)
                incident = past.cause == XENO_OUTBREAK ? QUARANTINE : BLACKOUT;
        }
        history(p, damage, source, incident);
        if (generator >= 5)
            for (int i = 0; i < p->count; i++) {
                RoomSpec *r = &p->rooms[i];
                if (r->history_role || r->type == BAY || r->height < 3)
                    continue;
                if ((r->type == CREW || r->type == GARDEN) && r->hash % 5 == 0)
                    r->history_role = HISTORY_SURVIVORS;
                else if ((r->type == ENGINE || r->type == NAVE) && r->hash % 5 == 0)
                    r->history_role = HISTORY_CONTROL;
            }
    }
}
RoomSpec world_describe_version(uint32_t seed, uint32_t generator, Cell c) {
    RegionPlan p;
    world_plan_version(seed, generator, world_region(c), &p);
    return p.rooms[p.owner[(c.z - p.origin.z) * 4 + c.x - p.origin.x]];
}
bool world_door_version(uint32_t seed, uint32_t generator, Cell c, Direction d) {
    if (c.z < 0 || (d == SOUTH && !c.z))
        return false;
    RegionPlan p;
    world_plan_version(seed, generator, world_region(c), &p);
    return (p.doors[(c.z - p.origin.z) * 4 + c.x - p.origin.x] & (1 << d)) != 0;
}
Connector world_connector_version(uint32_t seed, uint32_t generator, Cell cell,
                                  Direction direction) {
    Connector c = {0};
    c.open = world_door_version(seed, generator, cell, direction);
    c.local_center = v3(direction == WEST   ? -48
                        : direction == EAST ? 48
                                            : 0,
                        0,
                        direction == SOUTH   ? -48
                        : direction == NORTH ? 48
                                             : 0);
    if (c.open) {
        RoomSpec a = world_describe_version(seed, generator, cell);
        RoomSpec b = world_describe_version(seed, generator, world_neighbor(cell, direction));
        c.width = 7;
        c.clearance = fmin(5, fmin(a.height, b.height));
        c.crouch = c.clearance < 1.78;
    }
    return c;
}
uint32_t world_fingerprint_version(uint32_t seed, uint32_t generator, Cell region) {
    RegionPlan p;
    world_plan_version(seed, generator, region, &p);
    uint32_t h = UINT32_C(2166136261);
    for (int i = 0; i < 16; i++) {
        const RoomSpec *r = &p.rooms[p.owner[i]];
        uint32_t values[] = {(uint32_t)r->cell.x,
                             (uint32_t)r->cell.y,
                             (uint32_t)r->cell.z,
                             r->type,
                             r->district,
                             r->theme,
                             r->span_x,
                             r->span_z,
                             (uint32_t)(r->width * 100),
                             (uint32_t)(r->depth * 100),
                             (uint32_t)(r->height * 100),
                             p.doors[i],
                             r->hash,
                             r->layout,
                             r->incident,
                             r->intensity,
                             r->landmark,
                             r->history_role};
        for (size_t j = 0; j < (generator == 3   ? 13
                                : generator == 4 ? 17
                                                 : sizeof(values) / sizeof(values[0]));
             j++)
            for (int byte = 0; byte < 4; byte++) {
                h ^= (values[j] >> (byte * 8)) & 255;
                h *= UINT32_C(16777619);
            }
    }
    return h;
}

void world_plan(uint32_t seed, Cell region, RegionPlan *p) {
    world_plan_version(seed, SC_GENERATOR_VERSION, region, p);
}
RoomSpec world_describe(uint32_t seed, Cell cell) {
    return world_describe_version(seed, SC_GENERATOR_VERSION, cell);
}
bool world_door(uint32_t seed, Cell cell, Direction direction) {
    return world_door_version(seed, SC_GENERATOR_VERSION, cell, direction);
}
Connector world_connector(uint32_t seed, Cell cell, Direction direction) {
    return world_connector_version(seed, SC_GENERATOR_VERSION, cell, direction);
}
uint32_t world_fingerprint(uint32_t seed, Cell region) {
    return world_fingerprint_version(seed, SC_GENERATOR_VERSION, region);
}
