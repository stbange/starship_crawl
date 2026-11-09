#ifndef SC_MATRIX_H
#define SC_MATRIX_H
#include "world.h"
#define SC_MATRIX_VERSION 1u
#define SC_MATRIX_NODES 16
#define SC_MATRIX_GUARDS 24
#define SC_MATRIX_SHOTS 64
#define SC_MATRIX_BOXES 160
#define SC_MATRIX_CONTROLS 32
#define SC_MATRIX_RECORDS 65536u
#define SC_MATRIX_PROGRAMS 6
#define SC_MATRIX_SERVICES 6
enum { MX_ROUTED = 1, MX_VAULT = 2 };
typedef enum { MX_VEIL, MX_ECHO, MX_INTERRUPT, MX_BRIDGE, MX_FORK, MX_SIPHON } MatrixProgram;
typedef enum { MX_POWER, MX_SECURITY, MX_BEACON, MX_ARCHIVE, MX_ROBOT, MX_TRAFFIC } MatrixService;
typedef struct {
    Cell key;
    uint32_t done[SC_MATRIX_SERVICES], keys, killed, loot, flags, seen, outcome, alert;
} MatrixRecord;
typedef struct {
    uint32_t programs, slots[2], upgrades[SC_MATRIX_PROGRAMS], data, dives, knowledge;
} MatrixProfile;
typedef struct {
    Cell key, terminal, blocked;
    Vec3 pos, velocity, echo;
    double body_yaw, body_pitch;
    float integrity, bandwidth, trace, cooldown, hurt, veil, siphon, grace, counter, lockout,
        echo_life;
    uint32_t network, selected, service, unbanked;
    bool active, choice, grounded;
} MatrixDiver;
typedef struct {
    Cell room;
    Vec3 at;
    uint32_t links, service;
    uint8_t district, architecture;
    bool valid, cache;
} MatrixNode;
typedef struct {
    Box box;
    uint8_t tag;
} MatrixBox;
typedef struct {
    Vec3 pos, aim;
    float health, cooldown, windup, stun;
    uint8_t role;
    bool active;
} MatrixGuard;
typedef struct {
    Vec3 pos, velocity;
    float life, damage;
    int owner;
    uint8_t role;
    bool active;
} MatrixShot;
typedef struct {
    Cell key;
    MatrixNode nodes[SC_MATRIX_NODES];
    MatrixBox boxes[SC_MATRIX_BOXES];
    MatrixGuard guards[SC_MATRIX_GUARDS];
    MatrixShot shots[SC_MATRIX_SHOTS];
    uint32_t mask, entry, credential, core, switch_b, vault, gap_a, gap_b, count, box_count;
    uint32_t theme, motif;
    size_t record;
    float switches[2], bridge, gate;
    bool active, rare;
} MatrixNetwork;
typedef struct {
    Cell room;
    float life;
    uint32_t kind, owner;
} MatrixControl;
typedef struct {
    Position from, to;
    float life;
} MatrixLink;
typedef struct {
    uint32_t version;
    MatrixProfile profiles[SC_PLAYERS];
    MatrixDiver divers[SC_PLAYERS];
    MatrixNetwork networks[SC_PLAYERS];
    MatrixControl controls[SC_MATRIX_CONTROLS];
    MatrixLink links[16];
    MatrixRecord *records;
    size_t count, capacity, *index, index_capacity;
} MatrixState;
extern const char *sc_matrix_programs[SC_MATRIX_PROGRAMS];
extern const char *sc_matrix_services[SC_MATRIX_SERVICES];
extern const float sc_matrix_costs[SC_MATRIX_PROGRAMS];
struct Game;
struct Player;
struct Shelter;
struct Actions;
void matrix_init(MatrixState *m);
void matrix_destroy(MatrixState *m);
bool matrix_index(MatrixState *m);
MatrixRecord *matrix_record(MatrixState *m, Cell key, bool create);
const MatrixRecord *matrix_find(const MatrixState *m, Cell key);
Cell matrix_region(Cell room);
void matrix_generate(uint32_t seed, uint32_t generator, Cell key, MatrixNetwork *w);
bool matrix_box_active(const MatrixNetwork *w, const MatrixRecord *r, unsigned box);
double matrix_raycast(const MatrixNetwork *w, const MatrixRecord *r, Vec3 from, Vec3 dir,
                      double limit);
bool matrix_overlap(const MatrixNetwork *w, const MatrixRecord *r, Vec3 feet, double height);
int matrix_node(const MatrixNetwork *w, Vec3 pos, double distance);
bool matrix_guarded(const MatrixNetwork *w, Vec3 pos);
bool matrix_rehydrate(MatrixState *m, uint32_t seed, uint32_t generator);
bool matrix_valid(const MatrixState *m, uint32_t seed, uint32_t generator,
                  const struct Player *players, const struct Shelter *shelter);
Vec3 matrix_terminal(Cell owner);
Vec3 matrix_eye(const struct Game *g, int player);
bool matrix_near(const struct Game *g, int player);
bool matrix_protected(const struct Game *g, int player);
bool matrix_toggle(struct Game *g, int player);
void matrix_disconnect(struct Game *g, int player, bool failed);
void matrix_player_step(struct Game *g, int player, const struct Actions *a, double dt);
void matrix_step(struct Game *g, double dt);
void matrix_use(struct Game *g, int player);
bool matrix_service(struct Game *g, int player);
void matrix_cycle_service(struct Game *g, int player, int delta);
bool matrix_choose(struct Game *g, int player, unsigned choice);
bool matrix_cast(struct Game *g, int player, unsigned slot);
bool matrix_equip(struct Game *g, int player, unsigned program);
bool matrix_upgrade(struct Game *g, int player);
bool matrix_operation_done(const struct Game *g, Cell owner, unsigned service);
void matrix_select(struct Game *g, int player, unsigned slot);
bool matrix_control_enemy(struct Game *g, Room *room, Enemy *e, double dt);
void matrix_body_hit(struct Game *g, int player);
void matrix_reset_ship(struct Game *g);
bool matrix_goal(const struct Game *g, int player, Vec3 *at);
void matrix_hint(const struct Game *g, int player, char *text, size_t size);
const char *matrix_controller_name(unsigned motif);
const char *matrix_controller(const MatrixNetwork *w);
/* -1: known preparation blocked/failed; 0: ordinary service; 1: shared completion. */
int matrix_story_operation(struct Game *g, int player, Cell owner, unsigned service);
void matrix_story_evidence(struct Game *g, Cell owner);
void matrix_story_event(struct Game *g, Cell owner, unsigned service, unsigned choice);
#endif
