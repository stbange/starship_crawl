#include "render_internal.h"

static Vec3 anatomical(const BodyPose *p, Vec3 local) {
    return vadd(p->at, vmul(vadd(vmul(p->right, local.x),
                                 vadd(vmul(p->up, local.y), vmul(p->forward, local.z))),
                            p->size));
}
void sc_render_organ(Renderer *r, const BodyPose *p, Vec3 at, Vec3 dimensions, Color color,
                     float glow) {
    sc_render_blob(r, anatomical(p, at), vmul(p->right, dimensions.x * p->size),
                   vmul(p->up, dimensions.y * p->size), vmul(p->forward, dimensions.z * p->size),
                   color, glow);
}
void sc_render_bone(Renderer *r, const BodyPose *p, Vec3 a, Vec3 b, double thickness, Color color,
                    float glow) {
    sc_render_tube(r, anatomical(p, a), anatomical(p, b), thickness * p->size,
                   thickness * p->size * .45, color, glow);
}
void sc_render_panel(Renderer *r, const BodyPose *p, Vec3 at, Vec3 size, Color color, float glow) {
    sc_render_cube(&r->mesh, vsub(anatomical(p, at), r->origin),
                   vmul(p->right, 2 * size.x * p->size), vmul(p->up, 2 * size.y * p->size),
                   vmul(p->forward, 2 * size.z * p->size), color, glow);
}
static void body_form(Renderer *r, const Enemy *e, const BodyPose *p, double time, Color shell,
                      Color flesh, float glow) {
    if (sc_render_alien_form(r, e, p, time, shell, flesh, glow))
        return;
    EnemyBody body = sc_enemy_bodies[e->kind];
    int v = body.variant;
    double gait = sin((e->behavior && e->alive ? e->stride * 4 : time * 7) + e->id) * .13;
    if (body.form == HUSK) {
        sc_render_organ(r, p, v3(0, .15, 0), v3(.45, .65, .3), shell, glow);
        BodyPose head = *p;
        if (e->behavior == ALIEN_PILGRIM && e->alive) {
            head.forward = vnorm(e->heading);
            head.right = vnorm(vcross(head.up, head.forward));
            sc_render_organ(r, &head, v3(0, .32, .12), v3(.28, .25, .26), flesh, glow);
            for (int side = -1; side <= 1; side += 2)
                sc_render_organ(r, &head, v3(side * .1, 1, .29), v3(.045, .055, .04),
                                rgb(.7f, .5f, .28f), .8f);
        }
        sc_render_organ(r, &head, v3(0, .95, .04), v3(.28, .3, .26), flesh, glow);
        for (int side = -1; side <= 1; side += 2) {
            sc_render_bone(r, p, v3(side * .22, -.2, 0), v3(side * .26, -.6, gait * side), .14,
                           shell, glow);
            sc_render_bone(r, p, v3(side * .38, .5, 0), v3(side * .65, -.25, -gait * side), .11,
                           flesh, glow);
            if (v == 2)
                sc_render_organ(r, p, v3(side * .48, .55, 0), v3(.38, .28, .38), shell, .05f);
            if (v == 3)
                sc_render_bone(r, p, v3(side * .18, 1.12, 0), v3(side * .48, 1.65, -.1), .11, shell,
                               glow);
            if (v == 4)
                sc_render_bone(r, p, v3(side * .3, .2, -.1), v3(side * .9, .4, .35), .09, flesh,
                               glow);
        }
        if (v == 1)
            sc_render_organ(r, p, v3(.7, -.05, .3), v3(.16, .7, .15), rgb(.35f, .35f, .32f), .1f);
        if (v == 4)
            sc_render_organ(r, p, v3(0, .65, -.4), v3(.5, .9, .2), rgb(.25f, .19f, .12f), glow);
    } else if (body.form == GAZER) {
        BodyPose swing = *p, eye = *p;
        if (e->behavior == ALIEN_GAZER && e->alive) {
            double angle = sin(time * 1.3 + e->id) * .34;
            swing.up = vadd(vmul(p->up, cos(angle)), vmul(p->right, sin(angle)));
            swing.right = vadd(vmul(p->right, cos(angle)), vmul(p->up, -sin(angle)));
            eye.up = v3(0, 1, 0);
            eye.forward = vnorm(e->heading);
            eye.right = vnorm(vcross(eye.up, eye.forward));
        }
        sc_render_organ(r, &swing, v3(0, .25, 0), v3(.68, .68, .6), shell, glow);
        sc_render_organ(r, &eye, v3(0, .3, .55), v3(.36, .36, .1), rgb(.5f, .65f, .35f), .65f);
        sc_render_organ(r, &eye, v3(0, .3, .66), v3(e->windup > 0 ? .035 : .09, .24, .05),
                        rgb(.05f, .07f, .03f), .1f);
        int arms = v == 4 ? 8 : 3 + v;
        for (int i = 0; i < arms; i++) {
            double a = i * SC_PI * 2 / arms;
            Vec3 root = anatomical(&swing, v3(cos(a) * .4, 0, sin(a) * .4));
            Vec3 bend = vadd(vmul(swing.right, cos(a)), vmul(swing.forward, sin(a)));
            sc_render_tentacle_mesh(r, root, vmul(swing.up, -1), bend, (1.25 + v * .13) * p->size,
                                    .105 * p->size, i * 1.7 + e->id, time * 1.5, shell, flesh,
                                    rgb(.38f, .55f, .23f), glow, 8, true);
            if (v == 1)
                sc_render_bone(r, p, v3(cos(a) * .4, .6, sin(a) * .4),
                               v3(cos(a) * .8, 1.1, sin(a) * .8), .12, shell, glow);
            if (v == 3)
                sc_render_organ(r, p, v3(cos(a) * .65, .3, sin(a) * .65), v3(.25, .32, .25), flesh,
                                glow);
        }
        if (v == 2)
            for (int i = -1; i <= 1; i += 2)
                sc_render_bone(r, p, v3(i * .6, -.1, 0), v3(i * .6, 1.1, 0), .08, shell, .2f);
    } else if (body.form == VOID_RAY) {
        sc_render_organ(r, p, v3(0, .2, 0), v3(.35, .2, .85), shell, glow);
        int wings = v == 4 ? 3 : 1;
        for (int row = 0; row < wings; row++)
            for (int side = -1; side <= 1; side += 2) {
                double span = v == 0 ? 1.05 : v == 2 ? 1.9 : 1.5;
                Vec3 tip =
                    v3(side * span, .2 + sin(time * 3 + row) * .13, row * .42 - (v == 1 ? .35 : 0));
                sc_render_organ(r, p, v3(side * span * .5, .2, row * .4),
                                v3(span * .6, .08, v == 0 ? .8 : .65), shell, glow);
                sc_render_bone(r, p, v3(0, .25, row * .4), tip, .09, flesh, glow);
                if (v == 1 || v == 3)
                    for (int i = 0; i < 3; i++)
                        sc_render_bone(r, p, v3(side * (.6 + i * .3), .2, -.3),
                                       v3(side * (.8 + i * .4), .4, -.8), .07, shell, glow);
            }
        sc_render_bone(r, p, v3(0, .2, -.65), v3(sin(time * 3) * .2, .25, -1.7), .1, flesh, glow);
    } else if (body.form == WORM) {
        int segments = v == 0 ? 3 : v == 4 ? 8 : 6;
        for (int i = segments - 1; i >= 0; i--) {
            double t = (double)i / segments, radius = v == 2 ? .32 : .55;
            Vec3 at =
                v3(sin(time * 3 - i * .6) * .14, -.1 + sin(i * .6 + time * 2) * .07, -i * .35);
            sc_render_organ(r, p, at, v3(radius * (1 - t * .5), radius * (1 - t * .5), .35),
                            i % 2 ? shell : flesh, glow);
            if (v == 1)
                for (int side = -1; side <= 1; side += 2)
                    sc_render_bone(r, p, vadd(at, v3(side * .3, 0, 0)),
                                   vadd(at, v3(side * .85, -.5, gait * side)), .06, shell, glow);
            if (v == 4)
                sc_render_bone(r, p, vadd(at, v3(0, .3, 0)), vadd(at, v3(0, .9, 0)), .12, shell,
                               glow);
        }
        if (v == 3)
            for (int i = 0; i < 8; i++) {
                double a = i * SC_PI * .25;
                sc_render_bone(r, p, v3(cos(a) * .5, sin(a) * .5, .2),
                               v3(cos(a) * .3, sin(a) * .3, .65), .09, rgb(.55f, .48f, .32f), .05f);
            }
        if (v == 2)
            sc_render_bone(r, p, v3(0, 0, .25), v3(0, .05, .95), .04, rgb(.5f, .18f, .2f), .1f);
    } else if (body.form == ROBOT) {
        Color steel = rgb(.32f, .35f, .37f), dark = rgb(.07f, .09f, .10f);
        Color lamp = v == 2 ? rgb(.25f, .9f, .5f) : v == 4 ? rgb(1, .2f, .06f) : rgb(.2f, .7f, 1);
        sc_render_panel(r, p, v3(0, .05, 0), v3(v == 3 ? .85 : .55, v == 1 ? .65 : .24, .65), shell,
                        glow);
        sc_render_panel(r, p, v3(0, .2, .66), v3(.28, .07, .035), lamp, 1.2f);
        if (v == 0) {
            for (int side = -1; side <= 1; side += 2)
                for (int end = -1; end <= 1; end += 2) {
                    sc_render_bone(r, p, v3(0, 0, 0), v3(side * .95, -.05, end * .65), .09, steel,
                                   .05f);
                    sc_render_panel(r, p, v3(side * .95, -.1, end * .65), v3(.28, .08, .28), dark,
                                    .1f);
                    sc_render_bone(r, p, v3(side * .95 - cos(time * 20) * .3, -.2, end * .65),
                                   v3(side * .95 + cos(time * 20) * .3, -.2, end * .65), .04, lamp,
                                   1);
                }
        } else if (v == 1 || v == 2) {
            if (v == 2)
                sc_render_panel(r, p, v3(0, .49, .05), v3(.10, .30, .10), steel, glow);
            sc_render_panel(r, p, v3(0, .9, .08), v3(.28, .23, .3), steel, glow);
            sc_render_panel(r, p, v3(0, .93, .39), v3(.21, .045, .03), lamp, 1);
            for (int side = -1; side <= 1; side += 2) {
                sc_render_panel(r, p, v3(side * .35, -.6, gait * side), v3(.2, .4, .25), dark,
                                glow);
                sc_render_panel(r, p, v3(side * .78, .25, .1), v3(.23, .3, .25), steel, glow);
                sc_render_bone(r, p, v3(side * .78, .2, .1), v3(side * .95, -.25, .6), .1, steel,
                               glow);
                for (int finger = -1; finger <= 1; finger += 2)
                    sc_render_panel(r, p, v3(side * .95 + finger * .12, -.25, .8),
                                    v3(.06, .08, .25), v == 2 ? lamp : steel, v == 2 ? .5f : glow);
            }
            if (v == 2)
                sc_render_panel(r, p, v3(0, .3, -.7), v3(.25, .6, .18), rgb(.18f, .45f, .3f), .1f);
        } else if (v == 3) {
            for (int side = -1; side <= 1; side += 2) {
                sc_render_panel(r, p, v3(side * .9, -.38, 0), v3(.22, .25, .9), dark, glow);
                for (int wheel = -1; wheel <= 1; wheel++)
                    sc_render_organ(r, p, v3(side * 1.12, -.38, wheel * .55), v3(.08, .18, .18),
                                    steel, glow);
            }
            sc_render_panel(r, p, v3(0, .43, 0), v3(.45, .22, .4), steel, glow);
            sc_render_bone(r, p, v3(0, .43, .3), v3(0, .43, 1.7), .13, steel, glow);
        } else {
            for (int side = -1; side <= 1; side += 2)
                sc_render_panel(r, p, v3(side * .62, -.25, 0), v3(.16, .32, .4), dark, glow);
            sc_render_bone(r, p, v3(0, .2, 0), v3(0, .9, 0), .045, steel, glow);
            sc_render_organ(r, p, v3(0, .9, 0), v3(.12, .12, .12), lamp, 1.4f);
        }
    } else {
        sc_render_organ(r, p, v3(0, .2, 0), v3(.5, .75, .5), shell, .1f);
        if (v == 0)
            for (int i = 0; i < 3; i++) {
                double a = time + i * SC_PI * 2 / 3;
                sc_render_organ(r, p, v3(cos(a) * .75, .3, sin(a) * .75), v3(.18, .45, .18), shell,
                                .15f);
            }
        if (v == 1)
            sc_render_organ(r, p, v3(0, .8, 0), v3(.35, 1.1, .35), shell, .1f);
        if (v == 2)
            for (int i = 0; i < 12; i++) {
                double a = time + i * SC_PI / 6;
                sc_render_organ(r, p, v3(cos(a) * .95, sin(a) * .95, .1), v3(.12, .22, .15), shell,
                                .25f);
            }
        if (v == 3)
            for (int i = 0; i < 3; i++) {
                double a = i * SC_PI * 2 / 3;
                sc_render_bone(r, p, v3(0, .1, 0), v3(cos(a) * 1.2, -.6, sin(a) * 1.2), .12, shell,
                               .05f);
            }
        if (v == 4)
            for (int i = -1; i <= 1; i++) {
                sc_render_organ(r, p, v3(i * .6, .55, 0), v3(.25, i == 0 ? 1.1 : .8, .35), shell,
                                .1f);
                sc_render_organ(r, p, v3(i * .6, .65, .32), v3(.07, .3, .05), rgb(.25f, .6f, .8f),
                                .75f);
            }
    }
}
static void death_mesh(Renderer *r, const EnemyDeath *d) {
    const Enemy *e = &d->body;
    double t = d->age, fall = fmin(1, t / .85), fade = fmin(1, (SC_DEATH_LIFE - t) / .8);
    BodyForm form = sc_enemy_bodies[e->kind].form;
    Vec3 at = vadd(vmul(enemy_center(e), 1 - fall), vmul(d->landing, fall));
    Vec3 forward = vnorm(v3(e->heading.x, 0, e->heading.z));
    if (vlen(forward) < .1)
        forward = v3(0, 0, 1);
    Vec3 right = vcross(v3(0, 1, 0), forward), up = v3(0, 1, 0);
    if (form == HUSK || form == ROBOT) {
        double angle = fall * SC_PI * .48;
        up = vadd(v3(0, cos(angle), 0), vmul(right, sin(angle)));
        right = vnorm(vcross(up, forward));
        at.y += .15 * fall;
    } else {
        up = v3(0, 1 - .78 * fall, 0);
        if (form == VOID_RAY)
            right = vmul(right, 1 - .7 * fall);
        if (form == WORM)
            forward = vmul(forward, 1 - .65 * fall);
    }
    double scale = sc_enemies[e->kind].size * (.05 + .95 * fade);
    BodyPose pose = {at, right, up, forward, scale};
    Color shell = habitat_enemy_color(e);
    shell = rgb(shell.r * (float)fade, shell.g * (float)fade, shell.b * (float)fade);
    body_form(r, e, &pose, d->age * .1, shell, rgb(.12f * fade, .06f * fade, .08f * fade), -.3f);
    for (int part = 0; part < 6; part++) {
        double a = part * SC_PI / 3 + e->id, speed = .8 + (part % 3) * .5;
        Vec3 debris =
            vadd(enemy_center(e), v3(cos(a) * t * speed, t * 2 - t * t * 3, sin(a) * t * speed));
        debris.y = fmax(debris.y, d->landing.y + .05);
        double size = .08 * fade;
        if (form == ROBOT || form == CONSTRUCT)
            sc_render_block(r, debris, v3(size * 2, size, size),
                            form == ROBOT ? rgb(.4f, .35f, .25f) : shell, t < .45 ? 1.2f : .05f);
        else
            sc_render_octahedron(r, debris, size, shell, t < .15 ? .6f : -.2f);
    }
}
static void hazard_mesh(Renderer *r, const DeathHazard *h) {
    Vec3 at = position_absolute(h->pos);
    bool acid = h->kind == DEATH_ACID;
    Color color = acid ? rgb(.3f, .8f, .12f) : h->spent ? rgb(1, .65f, .2f) : rgb(1, .24f, .06f);
    double pulse = .6 + .4 * sin(h->age * (h->triggered ? 32 : 13));
    double radius = h->spent ? h->radius * (1 - h->life / .45) : h->radius;
    for (int i = 0; i < 18; i++) {
        double a = i * SC_PI / 9, b = (i + 1) * SC_PI / 9;
        Vec3 first = vadd(at, v3(cos(a) * radius, .06, sin(a) * radius));
        Vec3 next = vadd(at, v3(cos(b) * radius, .06, sin(b) * radius));
        sc_render_tube(r, first, next, acid ? .06 : .035, .035, color, (float)pulse);
    }
    if (acid) {
        for (int i = 0; i < 5; i++) {
            double a = i * 2.4;
            sc_render_blob(r, vadd(at, v3(cos(a) * .7, .05, sin(a) * .7)), v3(.7, 0, 0),
                           v3(0, .07, 0), v3(0, 0, .7), color, .35f);
        }
    } else if (h->kind == DEATH_MINE && !h->spent) {
        sc_render_block(r, vadd(at, v3(0, .15, 0)), v3(.48, .18, .48), rgb(.23f, .24f, .21f), .1f);
        sc_render_octahedron(r, vadd(at, v3(0, .4, 0)), .13 + pulse * .08, color, 1.8f);
    } else
        sc_render_octahedron(r, vadd(at, v3(0, .2, 0)), h->spent ? radius * .6 : .18 + pulse * .14,
                             color, 2);
}
static void creature(Renderer *r, const Enemy *e, double time, double distance) {
    Vec3 center = enemy_center(e), up = e->climbing ? e->normal : v3(0, 1, 0);
    Vec3 heading = e->behavior ? e->body_heading : e->heading;
    Vec3 forward = vsub(heading, vmul(up, vdot(heading, up)));
    if (vlen(forward) < .1)
        forward = fabs(up.z) < .9 ? v3(0, 0, 1) : v3(1, 0, 0);
    forward = vnorm(forward);
    Vec3 right = vnorm(vcross(up, forward));
    const EnemyDef *def = &sc_enemies[e->kind];
    double size = def->size;
    bool cloaked =
        (def->flags & CLOAK) && distance > 12 && e->flash <= 0 && sin(time * 1.7 + e->id) > .1;
    if (cloaked) {
        sc_render_octahedron(r, center, .06, rgb(.16f, .2f, .3f), .5f);
        return;
    }
    Color flesh = e->flash > 0 ? rgb(.62f, .63f, .49f) : rgb(.18f, .09f, .12f);
    Color shell = e->flash > 0 ? rgb(.55f, .48f, .36f) : habitat_enemy_color(e);
    float glow = e->flash > 0 ? .25f : -1;
    BodyPose pose = {center, right, up, forward, size};
    body_form(r, e, &pose, time, shell, flesh, glow);
    if (e->behavior == ALIEN_SEAM && e->climbing && distance < 50)
        for (int i = 0; i < 4; i++) {
            Vec3 contact = e->contacts[i];
            if (vlen(contact) < .01 || vlen(vsub(contact, center)) > 3)
                continue;
            Vec3 root = vadd(center, vmul(right, i & 1 ? .3 : -.3));
            Vec3 joint = vadd(vmul(vadd(root, contact), .5), vmul(up, .5));
            sc_render_tube(r, root, joint, .09, .045, shell, glow);
            sc_render_tube(r, joint, contact, .045, .018, flesh, glow);
        }
    if ((def->flags & ARMORED) && sc_enemy_bodies[e->kind].form != ROBOT)
        sc_render_blob(r, vadd(center, vmul(up, size * .35)), vmul(right, size * .86),
                       vmul(up, size * .4), vmul(forward, size * .95), rgb(.25f, .24f, .21f), glow);
    if ((def->flags & HOVER) && sc_enemy_bodies[e->kind].form != ROBOT)
        for (int side = -1; side <= 1; side += 2) {
            Vec3 wing =
                vadd(center, vadd(vmul(right, side * size * 2.5), vmul(up, sin(time * 8) * .3)));
            sc_render_tube(r, center, wing, .12, .025, def->color, -1);
            sc_render_tube(r, wing, vadd(center, vmul(forward, -size)), .06, .02, def->color, -1);
        }
    if (def->flags & WARD)
        for (int k = 0; k < 8; k++) {
            double a = k * SC_PI / 4 + time * .5;
            Vec3 p = vadd(center,
                          vadd(vmul(right, cos(a) * size * 1.25), vmul(up, sin(a) * size * 1.25)));
            sc_render_blob(r, vadd(p, vmul(forward, size)), vmul(right, .07), vmul(up, .13),
                           vmul(forward, .05), rgb(.2f, .55f, .8f), .7f);
        }
    if (def->flags & BROOD)
        for (int k = 0; k < 4; k++)
            sc_render_blob(r, vadd(center, vadd(vmul(up, size * .7), vmul(right, (k - 1.5) * .35))),
                           vmul(right, .25), vmul(up, .4), vmul(forward, .3), rgb(.30f, .21f, .12f),
                           -1);
    if (def->flags & HEALER)
        sc_render_octahedron(r, vadd(center, vmul(up, size * 1.2)), .18, rgb(.6f, .25f, .85f),
                             1.6f);
    if (e->burn > 0)
        sc_render_octahedron(r, vadd(center, vmul(up, size * .8)), .15, rgb(1, .3f, .08f), 2);
    Vec3 face = vadd(center, vadd(vmul(forward, size * .88), vmul(up, size * .18)));
    for (int side = -1; side <= 1; side += 2) {
        Vec3 eye = vadd(face, vmul(right, side * size * .19));
        sc_render_blob(r, eye, vmul(right, .045), vmul(up, .055), vmul(forward, .04),
                       rgb(.50f, .58f, .31f), .9f);
    }
    if (e->windup > 0) {
        double charge = clampd(1 - e->windup / 1.05, 0, 1), radius = .09 + .25 * charge;
        sc_render_octahedron(r, vadd(face, vmul(forward, .26)), radius, rgb(.32f, .82f, .36f),
                             2.4f);
    }
}
static void fold_door(Renderer *r, Vec3 feet, double yaw) {
    Vec3 right = v3(cos(yaw), 0, -sin(yaw)), forward = v3(sin(yaw), 0, cos(yaw));
    for (int side = -1; side <= 1; side += 2)
        sc_render_cube(
            &r->mesh, vsub(vadd(feet, vadd(vmul(right, side * 1.05), v3(0, 1.35, 0))), r->origin),
            vmul(right, .22), v3(0, 2.7, 0), vmul(forward, .45), rgb(.32f, .28f, .22f), .15f);
    sc_render_cube(&r->mesh, vsub(vadd(feet, v3(0, 2.65, 0)), r->origin), vmul(right, 2.3),
                   v3(0, .2, 0), vmul(forward, .45), rgb(.7f, .53f, .3f), .6f);
    sc_render_cube(&r->mesh, vsub(vadd(feet, v3(0, 1.3, 0)), r->origin), vmul(right, 1.85),
                   v3(0, 2.5, 0), vmul(forward, .08), rgb(.13f, .48f, .53f), 1.1f);
    for (int i = 0; i < 5; i++)
        sc_render_cube(&r->mesh, vsub(vadd(feet, v3(0, .35 + i * .44, 0)), r->origin),
                       vmul(right, 1.5), v3(0, .025, 0), vmul(forward, .1), rgb(.65f, .75f, .55f),
                       .9f);
}
void sc_render_fold_interior(Renderer *r, const Game *g, int viewer) {
    Color stone = rgb(.23f, .25f, .27f), wood = rgb(.4f, .27f, .17f);
    sc_render_block(r, v3(0, -.2, 0), v3(18.4, .4, 16.4), stone, 0);
    sc_render_block(r, v3(0, 4.7, 0), v3(18.4, .4, 16.4), rgb(.13f, .18f, .23f), 0);
    sc_render_block(r, v3(-9.15, 2.25, 0), v3(.3, 4.5, 16.4), stone, 0);
    sc_render_block(r, v3(9.15, 2.25, 0), v3(.3, 4.5, 16.4), stone, 0);
    sc_render_block(r, v3(0, 2.25, 8.15), v3(18.4, 4.5, .3), stone, 0);
    sc_render_block(r, v3(0, 2.25, -8.15), v3(18.4, 4.5, .3), stone, 0);
    sc_render_block(r, v3(0, .012, -.6), v3(4.5, .025, 12), rgb(.24f, .14f, .14f), .04f);
    for (int z = -6; z <= 6; z += 4) {
        sc_render_block(r, v3(-8.85, 2.2, z), v3(.22, 4.4, .25), wood, .04f);
        sc_render_block(r, v3(8.85, 2.2, z), v3(.22, 4.4, .25), wood, .04f);
        sc_render_block(r, v3(0, 4.38, z), v3(18, .22, .25), wood, .04f);
    }
    size_t count;
    const Box *boxes = shelter_furniture(&count);
    for (size_t i = 0; i < count; i++)
        sc_render_block(r, vmul(vadd(boxes[i].min, boxes[i].max), .5),
                        vsub(boxes[i].max, boxes[i].min), boxes[i].color, boxes[i].glow);
    sc_render_matrix_terminal_mesh(r, v3(3, 0, -6.3), g->time);
    /* Bed, quilt, pillows and reading lamp. */
    sc_render_block(r, v3(-6.3, .9, 5.55), v3(2.5, .22, 3), rgb(.64f, .64f, .52f), .03f);
    sc_render_block(r, v3(-6.3, 1.04, 5.2), v3(2.35, .1, 2.1), rgb(.22f, .38f, .4f), .03f);
    sc_render_block(r, v3(-6.3, 1.08, 6.55), v3(1.8, .24, .55), rgb(.75f, .7f, .54f), .05f);
    sc_render_block(r, v3(-7.9, 1.7, 6.7), v3(.6, .7, .6), rgb(.85f, .57f, .24f), .7f);
    /* Medical screen, cross and instruments. */
    sc_render_block(r, v3(6.5, 1.7, 6.3), v3(2, 1, .2), rgb(.12f, .45f, .48f), .55f);
    sc_render_block(r, v3(6.5, 1.7, 6.15), v3(.85, .18, .1), rgb(.65f, .9f, .65f), 1);
    sc_render_block(r, v3(6.5, 1.7, 6.14), v3(.18, .75, .1), rgb(.65f, .9f, .65f), 1);
    for (int i = 0; i < 4; i++)
        sc_render_block(r, v3(5.5 + i * .5, 1.4, 5.4), v3(.16, .3, .16), rgb(.4f, .6f, .65f), .1f);
    /* Kitchen: hob, pot, pantry tins, sink and extraction hood. */
    sc_render_block(r, v3(-6.9, 1.12, -2), v3(1.2, .04, 1), rgb(.13f, .16f, .17f), .05f);
    sc_render_block(r, v3(-6.9, 1.18, -2), v3(.7, .08, .7), rgb(.9f, .34f, .12f), .65f);
    sc_render_block(r, v3(-6.9, 1.4, -2), v3(.55, .4, .55), rgb(.45f, .42f, .32f), .04f);
    sc_render_block(r, v3(-5.2, 1.14, -2), v3(.8, .06, .8), rgb(.23f, .43f, .47f), .1f);
    sc_render_block(r, v3(-6.9, 2.6, -2), v3(1.7, .3, 1.5), rgb(.24f, .28f, .3f), 0);
    for (int i = 0; i < 5; i++)
        sc_render_block(r, v3(-7.6 + i * .58, 1.3, -1.5), v3(.25, .4, .25), rgb(.65f, .47f, .24f),
                        .06f);
    /* Dining mugs and a star window. */
    for (int i = 0; i < 3; i++)
        sc_render_block(r, v3(-.8 + i * .7, 1, 2), v3(.18, .25, .18), rgb(.65f, .53f, .32f), .05f);
    sc_render_block(r, v3(0, 2.5, 7.95), v3(5.5, 2.4, .1), rgb(.05f, .1f, .2f), .45f);
    for (int i = 0; i < 24; i++) {
        double x = -2.5 + (i * 37 % 101) * .05, y = 1.45 + (i * 19 % 43) * .05;
        sc_render_block(r, v3(x, y, 7.86), v3(.035, .035, .02), rgb(.75f, .8f, .7f), 1.5f);
    }
    fold_door(r, v3(0, 0, -7.95), 0);
    for (int i = 0; i < g->player_count; i++)
        if (i != viewer && g->players[i].sheltered) {
            Vec3 p = g->players[i].shelter_pos;
            sc_render_block(r, vadd(p, v3(0, .7, 0)), v3(.65, 1.35, .4), rgb(.3f, .6f, .55f), .08f);
            sc_render_octahedron(r, vadd(p, v3(0, 1.5, 0)), .35, rgb(.7f, .72f, .65f), .1f);
        }
}
static void rig_block(Renderer *r, const Rig *rig, Vec3 at, Vec3 size, Color color, float glow) {
    Vec3 right = v3(cos(rig->yaw), 0, -sin(rig->yaw)),
         forward = v3(sin(rig->yaw), 0, cos(rig->yaw));
    Vec3 center = vadd(position_absolute(rig->pos),
                       vadd(vmul(right, at.x), vadd(v3(0, at.y, 0), vmul(forward, at.z))));
    sc_render_cube(&r->mesh, vsub(center, r->origin), vmul(right, size.x), v3(0, size.y, 0),
                   vmul(forward, size.z), color, glow);
}
static void rig_mesh(Renderer *r, const Rig *rig, RigKind kind, double time) {
    Color metal = rig->armor > 0 ? rgb(.24f, .29f, .3f) : rgb(.1f, .11f, .12f),
          trim = rgb(.7f, .43f, .16f);
    if (kind == MECH) {
        for (int side = -1; side <= 1; side += 2) {
            rig_block(r, rig, v3(side * .43, .55, 0), v3(.48, 1.1, .65), metal, 0);
            rig_block(r, rig, v3(side * .43, .15, .18), v3(.58, .3, .95), metal, 0);
            rig_block(r, rig, v3(side * .87, 1.8, 0), v3(.38, .8, .75), metal, 0);
            rig_block(r, rig, v3(side * .87, 1.55, .72), v3(.2, .22, 1.1), metal, 0);
            rig_block(r, rig, v3(side * .87, 1.55, 1.3), v3(.22, .24, .08), trim, .7f);
        }
        rig_block(r, rig, v3(0, 1.65, 0), v3(1.35, 1.25, 1), metal, 0);
        rig_block(r, rig, v3(0, 2.48, .05), v3(.75, .55, .75), metal, 0);
        rig_block(r, rig, v3(0, 2.48, .44), v3(.6, .12, .04),
                  rig->armor > 0 ? rgb(.8f, .57f, .25f) : rgb(.2f, .1f, .05f), .8f);
        rig_block(r, rig, v3(0, 1.8, -.7), v3(.9, 1.5, .5), metal, 0);
        rig_block(r, rig, v3(0, 1.8, -.96), v3(.16, 1.1, .04), trim, .3f);
    } else {
        rig_block(r, rig, v3(0, .45, 0), v3(1.2, .45, 1.9), metal, 0);
        rig_block(r, rig, v3(0, .9, -.35), v3(.8, .8, .45), metal, 0);
        rig_block(r, rig, v3(0, .8, .65), v3(1.2, .55, .5), trim, 0);
        for (int side = -1; side <= 1; side += 2)
            for (int end = -1; end <= 1; end += 2)
                rig_block(r, rig, v3(side * .65, .3, end * .65), v3(.28, .6, .6),
                          rgb(.08f, .1f, .11f), 0);
        rig_block(r, rig, v3(0, 1.18, .72), v3(.7, .08, .1), rgb(.3f, .65f, .55f), .5f);
    }
    (void)time;
}
static void story_prop(Renderer *r, unsigned scene, Vec3 at, bool done, double time) {
    Color metal = rgb(.13f, .18f, .19f), glow = done ? rgb(.2f, .35f, .29f) : rgb(.3f, .8f, .72f);
    if (scene >= STORY_TRACKS && scene <= STORY_RATIONS) {
        for (int i = 0; i < 5; i++) {
            Vec3 p = vadd(at, v3((i % 2 ? .25 : -.25), .04, (i - 2) * .38));
            if (scene == STORY_CONDUIT)
                sc_render_tube(r, p, vadd(p, v3(.8, .12, .8)), .025, .025, glow, .4f);
            else if (scene == STORY_FROST || scene == STORY_RESEARCH)
                sc_render_octahedron(r, vadd(p, v3(0, .12, 0)), .12, rgb(.35f, .5f, .65f), .3f);
            else
                sc_render_block(r, p, v3(.15, scene == STORY_RATIONS ? .17 : .03, .25),
                                rgb(.45f, .3f, .15f), .2f);
        }
        return;
    }
    sc_render_block(r, vadd(at, v3(0, .5, 0)), v3(.95, 1, .7), metal, 0);
    if (scene == STORY_REACTOR || scene == STORY_DRIVE || scene == STORY_POWER ||
        scene == STORY_RELAY) {
        for (int i = -1; i <= 1; i++)
            sc_render_tube(r, vadd(at, v3(i * .42, .8, 0)), vadd(at, v3(i * .42, 2.1, 0)), .16, .11,
                           i ? metal : glow, i ? .1f : 1);
    } else if (scene == STORY_SURVIVORS) {
        for (int i = -1; i <= 1; i += 2) {
            sc_render_block(r, vadd(at, v3(i * .65, .6, .5)), v3(.6, 1.2, 1.5), metal, 0);
            sc_render_block(r, vadd(at, v3(i * .65, 1.24, .5)), v3(.4, .08, 1.2), glow, .6f);
        }
    } else if (scene == STORY_ARCHIVE || scene == STORY_MEMORY || scene == STORY_MIRROR) {
        for (int i = -1; i <= 1; i += 2)
            sc_render_octahedron(r, vadd(at, v3(i * .4, 1.55, 0)), .4, glow, .8f);
    } else {
        sc_render_block(r, vadd(at, v3(0, 1.3, 0)), v3(1.2, .5, .8), glow, .6f);
        if (scene == STORY_REFUGE || scene == STORY_EVAC || scene == STORY_EXTRACTION)
            sc_render_tube(r, vadd(at, v3(.55, 1.5, 0)), vadd(at, v3(.55, 2.5, 0)), .04, .02, glow,
                           .5f);
    }
    if (scene == STORY_COUNCIL)
        for (int side = -1; side <= 1; side += 2) {
            Color faction = side < 0 ? rgb(.25f, .65f, .6f) : rgb(.65f, .43f, .18f);
            Vec3 person = vadd(at, v3(side * 1.8, 0, 0));
            sc_render_block(r, vadd(person, v3(0, .95, 0)), v3(.42, .85, .3), faction, .25f);
            sc_render_octahedron(r, vadd(person, v3(0, 1.65, 0)), .23, faction, .4f);
            for (int leg = -1; leg <= 1; leg += 2)
                sc_render_tube(r, vadd(person, v3(leg * .12, .1, 0)),
                               vadd(person, v3(leg * .12, .55, 0)), .06, .08, faction, .2f);
        }
    if (!done)
        sc_render_octahedron(r, vadd(at, v3(0, 2.8 + sin(time) * .1, 0)), .2, rgb(.85f, .65f, .25f),
                             1.5f);
}
void sc_render_actors(Renderer *r, Game *g, int viewer) {
    for (int i = 0; i < g->world.count; i++) {
        const Room *room = &g->world.rooms[i];
        Vec3 at = matrix_terminal(room->cell);
        if (room->ready && vlen(vsub(at, player_eye(&g->players[viewer]))) < 110)
            sc_render_matrix_terminal_mesh(r, at, g->time);
    }
    for (int i = 0; i < 16; i++)
        if (g->matrix.links[i].life > 0)
            sc_render_tube(r, position_absolute(g->matrix.links[i].from),
                           position_absolute(g->matrix.links[i].to), .045, .025, rgb(.25f, 1, .7f),
                           1.4f);

    const Enemy *visible[128];
    double distances[128];
    int count = 0;
    Vec3 eye = player_eye(&g->players[viewer]);
    Cell cell = world_cell(eye);
    for (int j = 0; j < g->world.count; j++)
        for (int i = 0; i < g->world.rooms[j].enemy_count; i++) {
            Enemy *e = &g->world.rooms[j].enemies[i];
            double distance = vlen(vsub(enemy_center(e), eye));
            if (!e->alive || distance > 130 ||
                !sc_render_visible_deck(&g->world, cell.y, world_cell(enemy_center(e)).y))
                continue;
            /* Reserve the bounded draw budget for the closest threats. */
            int slot = count;
            if (count < 128)
                count++;
            else if (distance >= distances[count - 1])
                continue;
            else
                slot = count - 1;
            while (slot > 0 && distance < distances[slot - 1]) {
                visible[slot] = visible[slot - 1];
                distances[slot] = distances[slot - 1];
                slot--;
            }
            visible[slot] = e;
            distances[slot] = distance;
        }
    for (int i = 0; i < count; i++)
        creature(r, visible[i], g->time, distances[i]);
    for (int i = 0; i < count; i++)
        if (visible[i]->allegiance == 2)
            sc_render_octahedron(r, vadd(enemy_center(visible[i]), v3(0, 1.2, 0)), .17,
                                 rgb(.2f, 1, .6f), 1.1f);
    for (int i = 0; i < SC_MAX_DEATHS; i++) {
        const EnemyDeath *d = &g->deaths[i];
        if (d->active && vlen(vsub(enemy_center(&d->body), eye)) < 130 &&
            sc_render_visible_deck(&g->world, cell.y, world_cell(enemy_center(&d->body)).y))
            death_mesh(r, d);
    }
    for (int i = 0; i < SC_MAX_DEATH_HAZARDS; i++) {
        const DeathHazard *h = &g->death_hazards[i];
        if (h->active && vlen(position_delta(h->pos, g->players[viewer].pos)) < 130 &&
            sc_render_visible_deck(&g->world, cell.y, h->pos.sector.y))
            hazard_mesh(r, h);
    }
    for (int i = 0; i < SC_MAX_PROJECTILES; i++) {
        const Projectile *p = &g->projectiles[i];
        if (!p->active)
            continue;
        Vec3 pos = position_absolute(p->pos);
        if (vlen(vsub(pos, r->origin)) > 140)
            continue;
        Color color = p->color;
        sc_render_octahedron(r, pos, p->owner < 0 ? .20 : .09, color, 2);
        sc_render_tube(r, pos, vsub(pos, vmul(vnorm(p->velocity), p->owner < 0 ? 1.1 : 1.8)), .055,
                       .02, color, 1.3f);
    }
    for (int j = 0; j < g->world.count; j++) {
        const Room *room = &g->world.rooms[j];
        for (int k = 0; k < room->pickup_count; k++) {
            const Pickup *loot = &room->pickups[k];
            Vec3 p = position_absolute(loot->pos);
            if (!loot->active || !sc_render_visible_deck(&g->world, cell.y, world_cell(p).y) ||
                vlen(vsub(p, eye)) > 100)
                continue;
            p.y += .15 * sin(g->time * 2 + k);
            Color colors[] = {{.8f, .2f, .18f},  {.2f, .5f, 1},     {.75f, .65f, .35f},
                              {.85f, .5f, .15f}, {.65f, .3f, .85f}, {.2f, .9f, .6f},
                              {.7f, .6f, .3f}};
            Color col = colors[loot->kind];
            if (loot->kind == PICK_MEDKIT) {
                sc_render_block(r, p, v3(.55, .2, .2), col, .7f);
                sc_render_block(r, p, v3(.2, .55, .2), col, .7f);
            } else if (loot->kind == PICK_WEAPON) {
                sc_render_block(r, p, v3(.65, .25, .2), col, .7f);
                sc_render_block(r, vadd(p, v3(.25, .05, 0)), v3(.6, .1, .1), col, .4f);
            } else
                sc_render_octahedron(r, p, loot->kind == PICK_LOG ? .25 : .35, col, .7f);
        }
    }
    if (g->shelter.deployed && world_cell(position_absolute(g->shelter.door)).y == cell.y &&
        vlen(vsub(position_absolute(g->shelter.door), eye)) < 100)
        fold_door(r, position_absolute(g->shelter.door), g->shelter.yaw);
    for (int i = 0; i < g->player_count; i++) {
        const Player *p = &g->players[i];
        if (p->sheltered || p->nova_flash <= 0)
            continue;
        Vec3 at = position_absolute(p->pos);
        double radius = 7 * (1 - p->nova_flash / .35);
        for (int j = 0; j < 24; j++) {
            double a = j * SC_PI / 12, b = (j + 1) * SC_PI / 12;
            sc_render_tube(r, vadd(at, v3(cos(a) * radius, .2, sin(a) * radius)),
                           vadd(at, v3(cos(b) * radius, .2, sin(b) * radius)), .055, .055,
                           rgb(.6f, .3f, 1), 1.5f);
        }
    }
    for (int i = 0; i < g->player_count; i++)
        if (g->players[i].lift_active) {
            Vec3 at = position_absolute(g->players[i].pos);
            sc_render_block(r, vadd(at, v3(0, -.14, 0)), v3(4.6, .24, 4.6), rgb(.09f, .21f, .25f),
                            .3f);
            for (int side = -1; side <= 1; side += 2)
                sc_render_block(r, vadd(at, v3(side * 2.25, .04, 0)), v3(.08, .08, 4.5),
                                rgb(.1f, .7f, .9f), 2);
        }
    for (int owner = 0; owner < SC_PLAYERS; owner++)
        for (int k = 0; k < 2; k++) {
            const Rig *rig = &g->rigs[owner][k];
            if (!rig->deployed || (owner == viewer && g->players[viewer].riding == (unsigned)k + 1))
                continue;
            Vec3 at = position_absolute(rig->pos);
            if (world_cell(at).y == cell.y && vlen(vsub(at, eye)) < 130)
                rig_mesh(r, rig, (RigKind)(k + 1), g->time);
        }
    for (int i = 0; i < SC_PLAYERS; i++) {
        const Beam *beam = &g->beams[i];
        if (beam->life <= 0 || g->players[i].sheltered)
            continue;
        Vec3 from = position_absolute(beam->from), to = position_absolute(beam->to);
        if (vlen(vsub(from, eye)) < 150) {
            sc_render_tube(r, from, to, .045, .035, beam->color, 2);
            sc_render_tube(r, from, to, .014, .014, rgb(.9f, .95f, 1), 3);
            sc_render_octahedron(r, to, .1, beam->color, 2);
        }
    }
    for (int i = 0; i < SC_MAX_FLARES; i++) {
        const Flare *f = &g->flares[i];
        if (!f->active)
            continue;
        Vec3 at = position_absolute(f->pos);
        if (world_cell(at).y != cell.y || vlen(vsub(at, eye)) > 100)
            continue;
        sc_render_tube(r, at, vadd(at, vmul(f->normal, .3)), .055, .045, rgb(.6f, .24f, .12f), .3f);
        sc_render_octahedron(r, vadd(at, vmul(f->normal, .25)), .11, rgb(1, .65f, .25f), 2);
    }
    int wards = 0;
    for (int i = 0; g->story.version >= 3 && i < g->world.count && wards < 4; i++) {
        const Room *room = &g->world.rooms[i];
        uint32_t systems = world_systems(&g->world, room->cell);
        Vec3 at = game_terminal(room->cell);
        if (!(systems & SYSTEM_BEACON) || room->cell.y != cell.y || vlen(vsub(at, eye)) > 100)
            continue;
        wards++;
        Color color = systems & SYSTEM_POWER ? rgb(.2f, .8f, .65f) : rgb(.65f, .35f, .12f);
        for (int n = 0; n < 24; n++) {
            double a = n * SC_PI / 12, b = (n + 1) * SC_PI / 12;
            sc_render_tube(r, vadd(at, v3(sin(a) * 18, .06, cos(a) * 18)),
                           vadd(at, v3(sin(b) * 18, .06, cos(b) * 18)), .035, .035, color, 1);
        }
    }
    Cell target;
    if (g->story.version) {
        for (int i = 0; i < SC_STORY_SCENES; i++) {
            Cell c = g->story.sites[i];
            if ((i >= STORY_ORIGIN_RECORD && i <= STORY_CONTROL_RECORD) ||
                !(g->story.assigned & (1u << i)) || c.y != cell.y || !world_ready(&g->world, c))
                continue;
            if (i == STORY_BRIEFING && (g->story.assigned & (1u << STORY_EXTRACTION)))
                continue;
            if (g->story.version >= 3 &&
                (i == STORY_EXTRACTION || i == STORY_CONTINUE || i == STORY_RETURN)) {
                unsigned bay = game_story_available(g, STORY_RETURN)          ? STORY_RETURN
                               : (g->story.assigned & (1u << STORY_CONTINUE)) ? STORY_CONTINUE
                                                                              : STORY_EXTRACTION;
                if ((unsigned)i != bay)
                    continue;
            }
            Vec3 at = game_terminal(c);
            if (vlen(vsub(at, eye)) < 170) {
                unsigned shape = (unsigned)i;
                if (i == STORY_RESOLUTION) {
                    const unsigned shapes[] = {STORY_MEMORY, STORY_SURVIVORS, STORY_MEMORY,
                                               STORY_SOURCE, STORY_DRIVE};
                    shape = shapes[g->story.arc % STORY_ARCS];
                }
                story_prop(r, shape, at, !game_story_available(g, (unsigned)i), g->time);
            }
        }
    } else if (game_objective(g, &target) && target.y == cell.y && world_ready(&g->world, target)) {
        Vec3 p = game_terminal(target);
        if (vlen(vsub(p, eye)) < 170) {
            sc_render_block(r, vadd(p, v3(0, .6, 0)), v3(.9, 1.2, .7), rgb(.18f, .23f, .24f), 0);
            sc_render_block(r, vadd(p, v3(0, 1.4, 0)), v3(1.2, .6, .8), rgb(.25f, .7f, .65f), .8f);
            sc_render_octahedron(r, vadd(p, v3(0, 2.4 + sin(g->time) * .1, 0)), .23,
                                 rgb(.8f, .65f, .3f), 1.5f);
        }
    }
    for (int i = 0; i < g->player_count; i++)
        if (i != viewer && !g->players[i].sheltered && !g->players[i].cloaked &&
            !g->players[i].riding) {
            Player *p = &g->players[i];
            if (vlen(vsub(position_absolute(p->pos), r->origin)) > 430)
                continue;
            Color color = p->tint % 3 == 0
                              ? rgb(.22f, .65f, .65f)
                              : (p->tint % 3 == 1 ? rgb(.75f, .48f, .24f) : rgb(.62f, .39f, .73f));
            sc_render_block(r, vadd(position_absolute(p->pos), v3(0, .7, 0)), v3(.65, 1.35, .4),
                            color, .1f);
            sc_render_octahedron(r, vadd(position_absolute(p->pos), v3(0, 1.5, 0)), .35,
                                 rgb(.7f, .72f, .65f), .2f);
        }
}
void sc_render_weapon(Renderer *r, const Player *p) {
    Vec3 f = facing(p->yaw, p->pitch), right = v3(cos(p->yaw), 0, -sin(p->yaw)),
         up = vcross(f, right);
    Vec3 center = vsub(
        vadd(player_eye(p), vadd(vmul(right, .35), vadd(vmul(up, -.3), vmul(f, .65 - p->muzzle)))),
        r->origin);
    const WeaponDef *gun = game_gun(p);
    if (p->riding == MECH) {
        for (int side = -1; side <= 1; side += 2) {
            Vec3 arm = vsub(
                vadd(player_eye(p), vadd(vmul(right, side * .7), vadd(vmul(up, -.55), vmul(f, 1)))),
                r->origin);
            sc_render_cube(&r->mesh, arm, vmul(right, .5), vmul(up, .4), vmul(f, 1.2),
                           rgb(.24f, .29f, .3f), 0);
            sc_render_cube(&r->mesh, vadd(arm, vmul(f, .65)), vmul(right, .23), vmul(up, .23),
                           vmul(f, .2), gun->color, p->muzzle > 0 ? 2 : .4f);
        }
        return;
    }
    if (p->riding == WAGON)
        sc_render_cube(&r->mesh,
                       vsub(vadd(player_eye(p), vadd(vmul(up, -.7), vmul(f, .8))), r->origin),
                       vmul(right, 1.3), vmul(up, .35), vmul(f, .8), rgb(.35f, .25f, .14f), 0);
    Color tint = p->tint % 3 == 0
                     ? rgb(.18f, .39f, .4f)
                     : (p->tint % 3 == 1 ? rgb(.55f, .32f, .15f) : rgb(.41f, .25f, .53f));
    sc_render_cube(&r->mesh, center, vmul(right, .19), vmul(up, .20),
                   vmul(f, .42 + gun->damage * .002), rgb(.23f, .27f, .28f), 0);
    sc_render_cube(&r->mesh, vadd(center, vmul(up, -.1)), vmul(right, .23), vmul(up, .15),
                   vmul(f, .28), tint, .3f);
    sc_render_cube(&r->mesh, vadd(center, vmul(up, .12)), vmul(right, .05), vmul(up, .05),
                   vmul(f, .4), gun->color, .5f);
    sc_render_cube(&r->mesh, vadd(center, vmul(f, .32)), vmul(right, .12), vmul(up, .12),
                   vmul(f, .18), rgb(.12f, .18f, .19f), 0);
    if (gun->pellets > 1)
        for (int side = -1; side <= 1; side += 2)
            sc_render_cube(&r->mesh, vadd(center, vadd(vmul(right, side * .12), vmul(f, .27))),
                           vmul(right, .1), vmul(up, .1), vmul(f, .4), tint, 0);
    if (gun->flags & W_PROJECTILE)
        sc_render_cube(&r->mesh, vadd(center, vmul(f, .2)), vmul(right, .35), vmul(up, .3),
                       vmul(f, .3), tint, .1f);
    if (p->muzzle > 0) {
        Vec3 flash = vadd(center, vmul(f, .48));
        sc_render_cube(&r->mesh, flash, vmul(right, .18), vmul(up, .18), vmul(f, .18),
                       rgb(1, .8f, .3f), 3);
    }
}
