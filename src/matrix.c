#include "game.h"
#include <stdio.h>
#include <string.h>

static bool player_ok(const Game *g, int p) {
    return g && p >= 0 && p < SC_PLAYERS;
}
static uint32_t add(uint32_t n, uint32_t amount) {
    return (uint32_t)fmin(1000000000.0, (double)n + amount);
}
static float max_integrity(const Player *p) {
    return 100 + 2 * p->mods[4];
}
static float max_bandwidth(const Player *p) {
    return 100 + 4 * p->mods[5];
}
/* Aiming and swept hits use the same torso point, separate from the camera. */
static Vec3 avatar_center(const MatrixDiver *d) {
    return vadd(d->pos, v3(0, .8, 0));
}
Vec3 matrix_terminal(Cell c) {
    return vadd(world_center(c), v3(3, .02, 0));
}
Vec3 matrix_eye(const Game *g, int p) {
    return vadd(g->matrix.divers[p].pos, v3(0, 1.4, 0));
}
int matrix_node(const MatrixNetwork *w, Vec3 pos, double distance) {
    int found = -1;
    for (int i = 0; i < 16; i++)
        if (w->nodes[i].valid) {
            double d = vlen(vsub(pos, w->nodes[i].at));
            if (d < distance) {
                distance = d;
                found = i;
            }
        }
    return found;
}
bool matrix_guarded(const MatrixNetwork *w, Vec3 pos) {
    for (int i = 0; i < SC_MATRIX_GUARDS; i++)
        if (w->guards[i].active && w->guards[i].stun <= 0 && vlen(vsub(w->guards[i].pos, pos)) < 6)
            return true;
    return false;
}
bool matrix_near(const Game *g, int p) {
    if (!player_ok(g, p))
        return false;
    const Player *body = &g->players[p];
    if (body->sheltered)
        return vlen(vsub(body->shelter_pos, shelter_station(SHELTER_TERMINAL))) < 2.3;
    Vec3 feet = position_absolute(body->pos);
    if (feet.z < 0 || !world_ready(&g->world, world_cell(feet)))
        return false;
    Cell owner = world_describe_version(g->world.seed, g->world.generator, world_cell(feet)).cell;
    Vec3 to = vsub(vadd(matrix_terminal(owner), v3(0, .8, 0)), player_eye(body));
    return vlen(vsub(feet, matrix_terminal(owner))) < 3.4 &&
           world_raycast(&g->world, player_eye(body), vnorm(to), vlen(to)) >= vlen(to) - .1;
}
bool matrix_protected(const Game *g, int p) {
    if (!player_ok(g, p) || !g->matrix.divers[p].active)
        return false;
    const Player *body = &g->players[p];
    return body->sheltered || game_story_safe(g, position_absolute(body->pos)) ||
           world_describe_version(g->world.seed, g->world.generator,
                                  world_cell(position_absolute(body->pos)))
                   .type == BAY;
}
static int network_for(Game *g, Cell key) {
    MatrixRecord *r = matrix_record(&g->matrix, key, true);
    if (!r)
        return -1;
    for (int i = 0; i < 2; i++)
        if (g->matrix.networks[i].active && cell_equal(g->matrix.networks[i].key, key))
            return i;
    for (int i = 0; i < 2; i++) {
        bool used = false;
        for (int p = 0; p < 2; p++)
            used |= g->matrix.divers[p].active && g->matrix.divers[p].network == (unsigned)i;
        if (used)
            continue;
        MatrixNetwork *w = &g->matrix.networks[i];
        matrix_generate(g->world.seed, g->world.generator, key, w);
        w->record = (size_t)(r - g->matrix.records);
        for (unsigned j = 0; j < SC_MATRIX_GUARDS; j++) {
            unsigned extra = r->alert == 0 ? 0 : r->alert == 1 ? 2 : r->alert == 2 ? 5 : 8;
            w->guards[j].active =
                (j < 16 ? (w->mask & (1u << j)) && j != w->entry : j < 16 + extra) &&
                !(r->killed & (1u << j));
        }
        return i;
    }
    return -1;
}
bool matrix_toggle(Game *g, int p) {
    if (!player_ok(g, p) || p >= g->player_count)
        return false;
    MatrixDiver *d = &g->matrix.divers[p];
    Player *body = &g->players[p];
    if (d->active) {
        matrix_disconnect(g, p, false);
        return true;
    }
    if (body->lift_active || body->riding || body->health <= 0 || !matrix_near(g, p)) {
        game_message(g, "C / FIND A MATRIX TERMINAL / DISMOUNT BEFORE LINKING");
        return false;
    }
    Cell terminal = world_describe_version(g->world.seed, g->world.generator,
                                           world_cell(position_absolute(body->pos)))
                        .cell;
    if (d->lockout > 0 && cell_equal(terminal, d->blocked)) {
        game_message(g, "UPLINK RECOVERING / TERMINAL LOCKED BRIEFLY");
        return false;
    }
    Cell key = matrix_region(terminal);
    int index = network_for(g, key);
    if (index < 0) {
        game_message(g, "UPLINK UNAVAILABLE / NETWORK RECORD BUDGET");
        return false;
    }
    MatrixNetwork *w = &g->matrix.networks[index];
    unsigned node = (unsigned)((terminal.z - key.z) * 4 + terminal.x - key.x);
    *d = (MatrixDiver){.key = key,
                       .terminal = terminal,
                       .network = (unsigned)index,
                       .active = true,
                       .pos = vadd(w->nodes[node].at, v3(0, .02, -3)),
                       .body_yaw = body->yaw,
                       .body_pitch = body->pitch,
                       .integrity = max_integrity(body),
                       .bandwidth = max_bandwidth(body),
                       .grace = 1.5f,
                       .service = w->nodes[node].service};
    g->matrix.records[w->record].seen |= 1u << node;
    body->velocity = v3(0, 0, 0);
    body->flight = body->leap = body->nova = body->nova_tick = body->nova_flash = 0;
    body->resting = body->muzzle = 0;
    body->journal = body->decision_open = body->map_open = false;
    body->yaw = body->pitch = 0;
    g->dirty = true;
    game_message(g, "MATRIX LINK / F SERVICES / G DATA / 1 2 PROGRAMS / C DISCONNECT");
    return true;
}
void matrix_disconnect(Game *g, int p, bool failed) {
    if (!player_ok(g, p))
        return;
    MatrixDiver *d = &g->matrix.divers[p];
    if (!d->active)
        return;
    MatrixProfile *profile = &g->matrix.profiles[p];
    Player *body = &g->players[p];
    if (!failed) {
        profile->data = add(profile->data, d->unbanked);
        profile->dives = add(profile->dives, 1);
    }
    bool locked = failed || d->trace >= 100;
    body->yaw = d->body_yaw;
    body->pitch = d->body_pitch;
    body->velocity = v3(0, 0, 0);
    body->journal = false;
    d->active = d->choice = d->map = false;
    d->integrity = fmaxf(0, d->integrity);
    d->counter = 0;
    d->unbanked = 0;
    d->echo_life = d->veil = d->siphon = 0;
    d->blocked = d->terminal;
    d->lockout = locked ? 8 : 0;
    g->dirty = true;
    game_message(g, failed ? "LINK SEVERED / CONFIRMED WORK SAFE / UNBANKED DATA LOST"
                           : "DISCONNECTED / DATA BANKED / RETURNED TO YOUR BODY");
}
void matrix_body_hit(Game *g, int p) {
    if (player_ok(g, p) && g->matrix.divers[p].active && !matrix_protected(g, p)) {
        matrix_disconnect(g, p, false);
        game_message(g, "BODY UNDER ATTACK / EMERGENCY DISCONNECT");
    }
}
void matrix_reset_ship(Game *g) {
    for (int p = 0; p < 2; p++)
        matrix_disconnect(g, p, false);
    MatrixProfile profiles[2];
    memcpy(profiles, g->matrix.profiles, sizeof(profiles));
    matrix_destroy(&g->matrix);
    matrix_init(&g->matrix);
    memcpy(g->matrix.profiles, profiles, sizeof(profiles));
}
static void move_avatar(MatrixNetwork *w, const MatrixRecord *r, MatrixDiver *d, Vec3 delta) {
    double amounts[] = {delta.x, delta.y, delta.z};
    for (int axis = 0; axis < 3; axis++) {
        int steps = (int)ceil(fabs(amounts[axis]) / .15);
        if (!steps)
            continue;
        double step = amounts[axis] / steps;
        for (int i = 0; i < steps; i++) {
            Vec3 next = d->pos;
            if (axis == 0)
                next.x += step;
            else if (axis == 1)
                next.y += step;
            else
                next.z += step;
            if (matrix_overlap(w, r, next, 1.5)) {
                if (axis == 1) {
                    if (step < 0)
                        d->grounded = true;
                    d->velocity.y = 0;
                }
                break;
            }
            d->pos = next;
        }
    }
}
static void pulse(Game *g, int p) {
    MatrixDiver *d = &g->matrix.divers[p];
    MatrixNetwork *w = &g->matrix.networks[d->network];
    if (d->bandwidth < 2 || d->cooldown > 0)
        return;
    for (int i = 0; i < SC_MATRIX_SHOTS; i++)
        if (!w->shots[i].active) {
            w->shots[i] =
                (MatrixShot){.pos = matrix_eye(g, p),
                             .velocity = vmul(facing(g->players[p].yaw, g->players[p].pitch), 38),
                             .life = 1.6f,
                             .damage = 20 + g->players[p].power * 2,
                             .owner = p,
                             .active = true};
            d->bandwidth -= 2;
            d->trace = fminf(100, d->trace + .8f);
            d->cooldown = .28f;
            d->veil = 0;
            g->shots++;
            return;
        }
}
void matrix_player_step(Game *g, int p, const Actions *a, double dt) {
    MatrixDiver *d = &g->matrix.divers[p];
    if (!d->active || d->map || d->choice || g->players[p].journal)
        return;
    MatrixNetwork *w = &g->matrix.networks[d->network];
    MatrixRecord *r = &g->matrix.records[w->record];
    Vec3 f = facing(g->players[p].yaw, 0), right = v3(f.z, 0, -f.x);
    Vec3 wish = vadd(vmul(f, a->forward), vmul(right, a->strafe));
    if (vlen(wish) > 1)
        wish = vnorm(wish);
    d->velocity.x = wish.x * (a->sprint ? 12 : 10);
    d->velocity.z = wish.z * (a->sprint ? 12 : 10);
    if (a->jump && d->grounded)
        d->velocity.y = 6.2;
    d->velocity.y = fmax(-18, d->velocity.y - 14 * dt);
    d->grounded = false;
    move_avatar(w, r, d, vmul(d->velocity, dt));
    if (d->pos.y < -10 || fabs(d->pos.x) > 160 || fabs(d->pos.z) > 160) {
        d->pos = vadd(w->nodes[w->entry].at, v3(0, .02, -3));
        d->velocity = v3(0, 0, 0);
        d->integrity -= 10;
        d->grace = 1.2f;
        game_message(g, "PACKET LOST / AVATAR RECOVERED AT THE ENTRY NODE");
        if (d->integrity <= 0) {
            matrix_disconnect(g, p, true);
            return;
        }
    }
    int node = matrix_node(w, d->pos, 7);
    if (node >= 0 && d->pos.y < 2)
        r->seen |= 1u << node;
    if (a->fire)
        pulse(g, p);
}
static double sphere(Vec3 from, Vec3 dir, Vec3 center, double radius, double limit) {
    Vec3 delta = vsub(from, center);
    double b = vdot(delta, dir), c = vdot(delta, delta) - radius * radius, disc = b * b - c;
    if (disc < 0)
        return limit;
    double t = -b - sqrt(disc);
    if (t < 0 && c < 0)
        t = 0;
    return t >= 0 && t < limit ? t : limit;
}
static void hurt_avatar(Game *g, int p, float damage, unsigned role) {
    MatrixDiver *d = &g->matrix.divers[p];
    if (!d->active || d->grace > 0)
        return;
    d->integrity -= damage;
    d->hurt = 3;
    if (role == 2)
        d->bandwidth = fmaxf(0, d->bandwidth - 8);
    if (d->integrity <= 0)
        matrix_disconnect(g, p, true);
}
static void shots(Game *g, MatrixNetwork *w, double dt) {
    MatrixRecord *r = &g->matrix.records[w->record];
    for (int i = 0; i < SC_MATRIX_SHOTS; i++) {
        MatrixShot *s = &w->shots[i];
        if (!s->active)
            continue;
        double length = vlen(s->velocity) * fmin(dt, s->life);
        Vec3 dir = vnorm(s->velocity);
        double hit = matrix_raycast(w, r, s->pos, dir, length), distance = hit;
        int target = -1;
        bool echo = false;
        if (s->owner >= 0) {
            for (int k = 0; k < SC_MATRIX_GUARDS; k++)
                if (w->guards[k].active) {
                    double t = sphere(s->pos, dir, w->guards[k].pos, .7, distance);
                    if (t < distance) {
                        distance = t;
                        target = k;
                    }
                }
        } else
            for (int p = 0; p < g->player_count; p++) {
                MatrixDiver *d = &g->matrix.divers[p];
                if (!d->active || !cell_equal(d->key, w->key))
                    continue;
                double t = sphere(s->pos, dir, avatar_center(d), .48, distance);
                if (t < distance) {
                    distance = t;
                    target = p;
                    echo = false;
                }
                if (d->echo_life > 0) {
                    t = sphere(s->pos, dir, d->echo, .6, distance);
                    if (t < distance) {
                        distance = t;
                        target = p;
                        echo = true;
                    }
                }
            }
        s->pos = vadd(s->pos, vmul(dir, distance));
        s->life -= (float)dt;
        if (target >= 0) {
            if (s->owner < 0) {
                if (echo)
                    g->matrix.divers[target].echo_life = 0;
                else
                    hurt_avatar(g, target, s->damage, s->role);
            } else {
                MatrixGuard *guard = &w->guards[target];
                guard->health -= s->damage;
                g->hits++;
                if (guard->health <= 0) {
                    guard->active = false;
                    if (!(r->killed & (1u << target))) {
                        r->killed |= 1u << target;
                        MatrixDiver *d = &g->matrix.divers[s->owner];
                        if (d->active && cell_equal(d->key, w->key)) {
                            d->unbanked++;
                            if (d->siphon > 0) {
                                d->bandwidth =
                                    fminf(max_bandwidth(&g->players[s->owner]), d->bandwidth + 12);
                                d->trace = fmaxf(0, d->trace - 5);
                            }
                        } else
                            g->matrix.profiles[s->owner].data =
                                add(g->matrix.profiles[s->owner].data, 1);
                        g->dirty = true;
                    }
                }
            }
        }
        if (target >= 0 || hit < length || s->life <= 0)
            s->active = false;
    }
}
static void guards(Game *g, MatrixNetwork *w, double dt) {
    MatrixRecord *r = &g->matrix.records[w->record];
    unsigned extras = r->alert == 0 ? 0 : r->alert == 1 ? 2 : r->alert == 2 ? 5 : 8;
    for (unsigned i = 16; i < 16 + extras; i++)
        if (!(r->killed & (1u << i)))
            w->guards[i].active = true;
    for (int i = 0; i < SC_MATRIX_GUARDS; i++) {
        MatrixGuard *e = &w->guards[i];
        if (!e->active)
            continue;
        e->cooldown = fmaxf(0, e->cooldown - (float)dt);
        e->stun = fmaxf(0, e->stun - (float)dt);
        if (e->stun > 0)
            continue;
        int target = -1;
        Vec3 at = {0};
        double distance = 32;
        for (int p = 0; p < g->player_count; p++) {
            MatrixDiver *d = &g->matrix.divers[p];
            if (!d->active || !cell_equal(d->key, w->key))
                continue;
            double echo = d->echo_life > 0 ? vlen(vsub(d->echo, e->pos)) : 1000;
            if (echo < distance) {
                distance = echo;
                at = d->echo;
                target = -2;
            }
            double range = vlen(vsub(matrix_eye(g, p), e->pos));
            if (d->veil <= 0 && d->grace <= 0 && range < distance) {
                distance = range;
                at = avatar_center(d);
                target = p;
            }
        }
        if (target == -1) {
            e->windup = 0;
            continue;
        }
        Vec3 to = vsub(at, e->pos), direction = vnorm(v3(to.x, 0, to.z));
        if (distance > (e->role == 1 ? 1.5 : 9)) {
            Vec3 next = vadd(e->pos, vmul(direction, dt * (e->role == 1 ? 4 : 2.4)));
            if (!matrix_overlap(w, r, next, .7))
                e->pos = next;
        }
        if (e->role == 1 && distance < 1.8 && e->cooldown <= 0) {
            if (target >= 0)
                hurt_avatar(g, target, 12, 1);
            else
                for (int p = 0; p < 2; p++)
                    if (g->matrix.divers[p].echo_life > 0 &&
                        vlen(vsub(g->matrix.divers[p].echo, at)) < .1)
                        g->matrix.divers[p].echo_life = 0;
            e->cooldown = 1;
            continue;
        }
        bool sight = matrix_raycast(w, r, e->pos, vnorm(to), vlen(to)) >= vlen(to) - .1;
        if (!sight) {
            e->windup = 0;
            continue;
        }
        if (e->windup > 0) {
            e->windup = fmaxf(0, e->windup - (float)dt);
            if (e->windup == 0)
                for (int shot = 0; shot < (e->role == 3 ? 3 : 1); shot++)
                    for (int j = 0; j < SC_MATRIX_SHOTS; j++)
                        if (!w->shots[j].active) {
                            Vec3 dir = vnorm(vadd(vsub(e->aim, e->pos),
                                                  v3(e->role == 3 ? (shot - 1) * .35 : 0, 0, 0)));
                            w->shots[j] = (MatrixShot){.pos = e->pos,
                                                       .velocity = vmul(dir, 14),
                                                       .life = 3,
                                                       .damage = 10,
                                                       .owner = -1,
                                                       .role = e->role,
                                                       .active = true};
                            break;
                        }
        } else if (e->cooldown <= 0 && e->role != 1) {
            e->aim = at;
            e->windup = .65f;
            e->cooldown = 2;
        }
    }
}
void matrix_step(Game *g, double dt) {
    if (!isfinite(dt) || dt <= 0 || dt > .05)
        return;
    bool resident[2] = {false, false};
    for (int p = 0; p < 2; p++) {
        MatrixDiver *d = &g->matrix.divers[p];
        d->lockout = fmaxf(0, d->lockout - (float)dt);
        if (!d->active || p >= g->player_count)
            continue;
        resident[d->network] = true;
        d->cooldown = fmaxf(0, d->cooldown - (float)dt);
        d->hurt = fmaxf(0, d->hurt - (float)dt);
        d->veil = fmaxf(0, d->veil - (float)dt);
        d->siphon = fmaxf(0, d->siphon - (float)dt);
        d->grace = fmaxf(0, d->grace - (float)dt);
        d->echo_life = fmaxf(0, d->echo_life - (float)dt);
        d->bandwidth = fminf(max_bandwidth(&g->players[p]),
                             d->bandwidth + (float)dt * (5 + .2f * g->players[p].mods[5]));
        if (d->hurt == 0)
            d->integrity = fminf(max_integrity(&g->players[p]),
                                 d->integrity + (float)dt * (2 + .1f * g->players[p].mods[4]));
        MatrixNetwork *w = &g->matrix.networks[d->network];
        MatrixRecord *r = &g->matrix.records[w->record];
        unsigned alert = d->trace >= 75 ? 3 : d->trace >= 50 ? 2 : d->trace >= 25 ? 1 : 0;
        if (alert > r->alert) {
            r->alert = alert;
            game_message(g, alert == 3   ? "COUNTERINTRUSION / TRACE 100 FORCES DISCONNECT"
                            : alert == 2 ? "NETWORK LOCKDOWN / MORE SECURITY PATROLS"
                                         : "TRACE DETECTED / SECURITY PATROLS ACTIVATED");
            g->dirty = true;
        }
        bool calm = true;
        for (int j = 0; j < SC_MATRIX_GUARDS; j++)
            if (w->guards[j].active && vlen(vsub(w->guards[j].pos, d->pos)) < 20)
                calm = false;
        if (calm && d->cooldown <= 0)
            d->trace = fmaxf(0, d->trace - (float)dt * 2);
        if (d->trace >= 100 && d->counter <= 0) {
            d->counter = 5;
            game_message(g, "COUNTERINTRUSION / C TO DISCONNECT / FIVE SECONDS");
        }
        if (d->trace < 90)
            d->counter = 0;
        if (d->counter > 0) {
            d->counter -= (float)dt;
            if (d->counter <= 0)
                matrix_disconnect(g, p, true);
        }
    }
    for (int i = 0; i < 2; i++)
        if (resident[i]) {
            MatrixNetwork *w = &g->matrix.networks[i];
            w->bridge = fmaxf(0, w->bridge - (float)dt);
            w->gate = fmaxf(0, w->gate - (float)dt);
            for (int j = 0; j < 2; j++)
                w->switches[j] = fmaxf(0, w->switches[j] - (float)dt);
            guards(g, w, dt);
            shots(g, w, dt);
        }
    for (int i = 0; i < SC_MATRIX_CONTROLS; i++)
        g->matrix.controls[i].life = fmaxf(0, g->matrix.controls[i].life - (float)dt);
    for (int i = 0; i < 16; i++)
        g->matrix.links[i].life = fmaxf(0, g->matrix.links[i].life - (float)dt);
}
