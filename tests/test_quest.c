#include "quest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "Quest check failed at %d: %s\n", __LINE__, #x);                       \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static void identity(float *m) {
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1;
}
static bool near(Vec3 a, Vec3 b) {
    return vlen(vsub(a, b)) < .0001;
}
static Vec3 transform(const float *m, Vec3 p) {
    return v3(m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
              m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
              m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]);
}
static void tracking(void) {
    QuestTracking q;
    quest_tracking_init(&q);
    CHECK(!q.calibrated && !quest_lean_safe(&q));
    CHECK(!quest_calibrate(&q, 0));
    CHECK(!quest_pose_valid(NULL));
    float poses[SC_QUEST_POSES][16];
    for (int i = 0; i < SC_QUEST_POSES; i++) {
        identity(poses[i]);
        poses[i][13] = 1.7f;
    }
    poses[QUEST_LEFT][12] = -.3f;
    poses[QUEST_RIGHT][12] = .3f;
    CHECK(quest_tracking_update(&q, poses[0], 31));
    CHECK(!quest_calibrate(&q, NAN));
    CHECK(quest_calibrate(&q, 0));
    CHECK(quest_lean_safe(&q));
    Vec3 base = v3(10, 1.62, 30);
    CHECK(near(quest_position(&q, QUEST_HEAD, base), base));
    CHECK(near(quest_position(&q, QUEST_LEFT, base), vadd(base, v3(-.3, 0, 0))));
    CHECK(near(quest_position(&q, QUEST_RIGHT, base), vadd(base, v3(.3, 0, 0))));
    CHECK(near(quest_forward(&q, QUEST_HEAD), v3(0, 0, 1)));
    CHECK(near(quest_axis(&q, QUEST_HEAD, 0), v3(1, 0, 0)));
    CHECK(near(quest_position(&q, -1, base), base));
    CHECK(near(quest_axis(&q, 5, 0), v3(0, 0, 0)));
    CHECK(near(quest_axis(&q, 0, 3), v3(0, 0, 0)));
    CHECK(!quest_snap(&q, 1)); /* entry requires the stick to be released */
    CHECK(!quest_snap(&q, 0));
    CHECK(quest_snap(&q, 1) == 1);
    CHECK(!quest_snap(&q, 1));
    CHECK(near(quest_forward(&q, 0), facing(SC_PI / 4, 0)));
    CHECK(!quest_snap(&q, .1));
    CHECK(quest_snap(&q, -1) == -1);
    CHECK(!quest_snap(&q, NAN));
    CHECK(!quest_snap(&q, 0));

    float eye[16], view[16];
    identity(eye);
    eye[12] = .032f;
    eye[13] = -1.7f;
    CHECK(quest_view(&q, eye, base, v3(0, 0, 0), view));
    CHECK(near(transform(view, vadd(base, v3(0, 0, 2))), v3(.032, 0, -2)));
    eye[12] = -.032f;
    CHECK(quest_view(&q, eye, base, v3(0, 0, 0), view));
    CHECK(near(transform(view, vadd(base, v3(0, 0, 2))), v3(-.032, 0, -2)));
    /* Independent distant render origins must retain metre-scale eye spacing. */
    base = v3(288000010, 1.62, 30);
    CHECK(quest_view(&q, eye, base, v3(288000000, 0, 0), view));
    CHECK(near(transform(view, v3(10, 1.62, 32)), v3(-.032, 0, -2)));
    CHECK(!quest_view(&q, eye, v3(NAN, 0, 0), base, view));
    CHECK(!quest_view(&q, eye, base, base, NULL));
    CHECK(quest_calibrate(&q, SC_PI / 2));
    CHECK(near(quest_forward(&q, 0), v3(1, 0, 0)));
    CHECK(quest_view(&q, eye, base, v3(288000000, 0, 0), view));
    CHECK(near(transform(view, v3(12, 1.62, 30)), v3(-.032, 0, -2)));

    poses[0][12] = .25f;
    poses[0][13] = 1.1f;
    CHECK(quest_tracking_update(&q, poses[0], 31));
    CHECK(near(quest_head_delta(&q), v3(.25, -.6, 0)) && quest_lean_safe(&q));
    poses[0][12] = 2;
    CHECK(quest_tracking_update(&q, poses[0], 31) && !quest_lean_safe(&q));
    poses[0][12] = 0;
    poses[0][13] = 4;
    CHECK(quest_tracking_update(&q, poses[0], 31) && !quest_lean_safe(&q));
    CHECK(!quest_tracking_update(&q, poses[0], 3));
    CHECK(!quest_lean_safe(&q) && !quest_snap(&q, 1));
    CHECK(q.valid == 3 && quest_view(&q, eye, base, base, view));
    CHECK(!quest_tracking_update(&q, poses[0], 30));
    CHECK(!quest_view(&q, eye, base, base, view) && !quest_calibrate(&q, 0));
    CHECK(!quest_tracking_update(&q, NULL, 31));
    CHECK(q.valid == 0);
    CHECK(!quest_tracking_update(&q, poses[0], 63) && q.valid == 0);
    poses[0][13] = 1.7f;
    for (int i = 0; i < 16; i++) {
        float saved = poses[0][i];
        poses[0][i] = NAN;
        CHECK(!quest_tracking_update(&q, poses[0], 31));
        CHECK(!(q.valid & 1) && (q.valid & 30) == 30);
        poses[0][i] = saved;
    }
    float broken[16];
    identity(broken);
    broken[12] = 20000;
    CHECK(!quest_pose_valid(broken));
    identity(broken);
    broken[3] = 1;
    CHECK(!quest_pose_valid(broken));
    identity(broken);
    broken[7] = 1;
    CHECK(!quest_pose_valid(broken));
    identity(broken);
    broken[11] = 1;
    CHECK(!quest_pose_valid(broken));
    identity(broken);
    broken[15] = 0;
    CHECK(!quest_pose_valid(broken));
    identity(broken);
    broken[0] = .5;
    CHECK(!quest_pose_valid(broken));
    identity(broken);
    broken[4] = 1;
    broken[5] = 0;
    CHECK(!quest_pose_valid(broken));
    identity(broken);
    broken[8] = 1;
    broken[10] = 0;
    CHECK(!quest_pose_valid(broken));
    identity(broken);
    broken[9] = 1;
    broken[10] = 0;
    CHECK(!quest_pose_valid(broken));
    identity(broken);
    broken[0] = -1;
    CHECK(!quest_pose_valid(broken));
    /* Head-only tracking must draw recovery before either controller arrives. */
    quest_tracking_init(&q);
    CHECK(!quest_tracking_update(&q, poses[0], 1));
    CHECK(quest_calibrate(&q, 0) && quest_lean_safe(&q));
    CHECK(quest_view(&q, eye, base, base, view));
    CHECK(!quest_snap(&q, 0) && !quest_snap(&q, 1));
    poses[QUEST_RIGHT][0] = NAN;
    CHECK(!quest_tracking_update(&q, poses[0], 31) && q.valid == (31u & ~(1u << QUEST_RIGHT)));
    CHECK(quest_view(&q, eye, base, base, view));
    poses[QUEST_RIGHT][0] = 1;
    /* Looking vertically at entry uses the horizontal right axis for yaw. */
    identity(poses[0]);
    poses[0][5] = 0;
    poses[0][6] = 1;
    poses[0][9] = -1;
    poses[0][10] = 0;
    CHECK(quest_tracking_update(&q, poses[0], 31));
    CHECK(quest_calibrate(&q, 0) && quest_lean_safe(&q));
    quest_tracking_init(&q);
    CHECK(near(quest_position(&q, 0, base), base));
    float projection[16] = {0};
    projection[0] = projection[5] = 1;
    projection[10] = -1.001f;
    projection[11] = -1;
    projection[14] = -.16f;
    CHECK(quest_projection_valid(projection));
    CHECK(!quest_projection_valid(NULL));
    projection[5] = 0;
    CHECK(!quest_projection_valid(projection));
    projection[5] = NAN;
    CHECK(!quest_projection_valid(projection));
    projection[5] = 100001;
    CHECK(!quest_projection_valid(projection));
}

static Game *new_game(bool inside) {
    Game *g = malloc(sizeof(*g));
    CHECK(g);
    game_init(g, 713);
    if (inside)
        g->players[0].pos = position_from(v3(0, 0, 12));
    Vec3 p = position_absolute(g->players[0].pos);
    CHECK(world_refresh(&g->world, &p, 1));
    for (int n = 0; g->world.pending && n < 500; n++)
        world_stream(&g->world, SC_STREAM_BUDGET);
    CHECK(!g->world.pending);
    return g;
}
static void tick(Game *g, Actions a) {
    Actions actions[2] = {a, {0}};
    game_step(g, actions, 1.0 / 60);
}
static void destroy(Game *g) {
    game_destroy(g);
    free(g);
}
static void combat(void) {
    Game *g = new_game(false);
    Player *p = &g->players[0];
    p->weapon = 3;
    p->weapons |= 1u << 3;
    g->campaign.profiles[0].guns[0] = 3;
    Vec3 expected = vadd(player_eye(p), v3(.3, -.2, .4));
    float energy = p->energy;
    Actions fire = {
        .fire = true, .aimed = true, .aim_offset = {.3, -.2, .4}, .aim_direction = {1, 0, 0}};
    tick(g, fire);
    CHECK(g->shots == 1 && p->energy < energy && p->cooldown > 0 && p->yaw == 0);
    Projectile *shot = &g->projectiles[0];
    CHECK(shot->active && shot->velocity.x > 0);
    CHECK(fabs(shot->velocity.z) < shot->velocity.x * .03);
    CHECK(near(vsub(position_absolute(shot->pos), vmul(shot->velocity, 1.0 / 60)), expected));
    tick(g, fire);
    CHECK(g->shots == 1);
    p->cooldown = 0;
    p->energy = 0;
    tick(g, fire);
    CHECK(g->shots == 1);
    destroy(g);

    g = new_game(false);
    p = &g->players[0];
    for (unsigned i = 0; i < SC_WEAPONS; i++)
        if (sc_weapons[i].flags & W_BEAM) {
            p->weapons |= 1u << i;
            p->weapon = i;
            g->campaign.profiles[0].guns[0] = i;
            break;
        }
    expected = vadd(player_eye(p), fire.aim_offset);
    tick(g, fire);
    CHECK(g->shots == 1 && g->beams[0].life > 0);
    CHECK(near(position_absolute(g->beams[0].from), expected));
    Vec3 beam = position_delta(g->beams[0].to, g->beams[0].from);
    CHECK(beam.x > 1 && fabs(beam.y) < .001 && fabs(beam.z) < .001);
    destroy(g);

    g = new_game(true);
    p = &g->players[0];
    energy = p->energy;
    fire.aim_offset = v3(0, -2.4, 0);
    tick(g, fire);
    CHECK(g->shots == 0 && p->energy == energy);
    fire.aim_offset = v3(4, 0, 0);
    tick(g, fire);
    CHECK(g->shots == 0);
    fire.aim_offset = v3(NAN, 0, 0);
    tick(g, fire);
    CHECK(g->shots == 0);
    fire.aim_offset = v3(0, 0, 0);
    fire.aim_direction = v3(0, 0, 0);
    tick(g, fire);
    CHECK(g->shots == 0);
    fire.aim_direction = v3(INFINITY, 0, 0);
    tick(g, fire);
    CHECK(g->shots == 0);
    fire.aim_direction = v3(101, 0, 0);
    tick(g, fire);
    CHECK(g->shots == 0);
    fire.aim_direction = v3(1, 0, 0);
    tick(g, fire);
    CHECK(g->shots == 1);
    destroy(g);
    g = new_game(false);
    tick(g, (Actions){.fire = true});
    CHECK(g->shots == 1 && g->projectiles[0].velocity.z > 0);
    destroy(g);
}
static void matrix_combat(void) {
    Game *g = new_game(true);
    g->players[0].intro = INTRO_INTERFACE | INTRO_GRID;
    g->matrix.profiles[0].programs = 7;
    g->players[0].pos = position_from(matrix_terminal((Cell){0, 0, 0}));
    CHECK(matrix_toggle(g, 0));
    MatrixDiver *d = &g->matrix.divers[0];
    MatrixNetwork *w = &g->matrix.networks[d->network];
    Actions fire = {
        .fire = true, .aimed = true, .aim_offset = {.2, 0, 0}, .aim_direction = {1, 0, 0}};
    d->bandwidth = 100;
    d->cooldown = 0;
    matrix_player_step(g, 0, &fire, 0);
    CHECK(g->shots == 1 && d->bandwidth == 98 && d->cooldown > 0);
    CHECK(near(w->shots[0].pos, vadd(matrix_eye(g, 0), fire.aim_offset)));
    CHECK(near(w->shots[0].velocity, v3(38, 0, 0)));
    matrix_player_step(g, 0, &fire, 0);
    CHECK(g->shots == 1);
    d->cooldown = 0;
    d->bandwidth = 0;
    matrix_player_step(g, 0, &fire, 0);
    CHECK(g->shots == 1);
    d->bandwidth = 100;
    fire.aim_offset = v3(4, 0, 0);
    matrix_player_step(g, 0, &fire, 0);
    CHECK(g->shots == 1);
    fire.aim_offset = v3(NAN, 0, 0);
    matrix_player_step(g, 0, &fire, 0);
    CHECK(g->shots == 1);
    fire.aim_offset = v3(0, -2.5, 0);
    matrix_player_step(g, 0, &fire, 0);
    CHECK(g->shots == 1);
    fire.aim_offset = v3(0, 0, 0);
    fire.aim_direction = v3(0, 0, 0);
    matrix_player_step(g, 0, &fire, 0);
    CHECK(g->shots == 1);
    fire.aim_direction = v3(101, 0, 0);
    matrix_player_step(g, 0, &fire, 0);
    CHECK(g->shots == 1);
    fire.aim_direction = v3(INFINITY, 0, 0);
    matrix_player_step(g, 0, &fire, 0);
    CHECK(g->shots == 1);
    CHECK(d->bandwidth == 100);
    fire.aim_direction = v3(1, 0, 0);
    matrix_player_step(g, 0, &fire, 0);
    CHECK(g->shots == 2);
    destroy(g);
}
int main(void) {
    tracking();
    combat();
    matrix_combat();
    puts("Quest tracking, stereo transforms, input latches and controller combat: PASS");
    return 0;
}
