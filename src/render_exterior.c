#include "render_internal.h"

/* The sky is camera-centered and unlit. Masking the endless hull analytically
   prevents far-clip gaps from revealing stars through the ship. */
void sc_render_starfield(Renderer *r, const Player *player) {
    Vec3 eye = player_eye(player), local_eye = vsub(eye, r->origin);
    Vec3 forward = facing(player->yaw, player->pitch);
    Vec3 right = v3(cos(player->yaw), 0, -sin(player->yaw));
    Vec3 up = vcross(forward, right);
    gl.Disable(GL_DEPTH_TEST);
    gl.Uniform1i(r->ui, 1);
    gl.Uniform3f(r->offset, 0, 0, 0);
    for (int i = 0; i < SC_SPACE_STARS; i++) {
        const SpaceStar *star = &r->stars[i];
        if (vdot(star->direction, forward) <= 0 || !exterior_sky_visible(eye, star->direction))
            continue;
        Vec3 at = vadd(local_eye, vmul(star->direction, 300));
        Vec3 x = vmul(right, star->radius), y = vmul(up, star->radius);
        sc_render_quad(&r->mesh, vsub(vsub(at, x), y), vadd(vsub(at, y), x), vadd(vadd(at, x), y),
                       vadd(vsub(at, x), y), star->color, 0);
    }
    sc_render_flush(r);
    gl.Uniform1i(r->ui, 0);
    gl.Enable(GL_DEPTH_TEST);
}
static void hull_lamp(Renderer *r, double x, double y, Vec3 eye, Color color) {
    double distance = vlen(vsub(v3(x, y, -4.25), eye));
    /* The tiny optical halo remains roughly a pixel at distance; nearby fittings
       and their lit panel provide physical scale without lighting the whole hull. */
    double halo = fmax(1, distance * .0018);
    sc_render_block(r, v3(x, y + 3.5, -4.12), v3(5, 12, .06),
                    rgb(color.r * .10f, color.g * .10f, color.b * .10f), .8f);
    sc_render_block(r, v3(x, y, -4.22), v3(halo, fmax(2.4, halo * 1.4), .06), color, 2.3f);
    /* A camera-facing optical glint prevents a distant fitting from collapsing
       to a subpixel sliver when viewed almost parallel to the hull. */
    if (distance > 140) {
        Vec3 center = v3(x, y, -4.3), normal = vnorm(vsub(eye, center));
        Vec3 right = vmul(vnorm(vcross(v3(0, 1, 0), normal)), halo * .65);
        Vec3 up = vcross(normal, right);
        center = vsub(vadd(center, vmul(normal, halo)), r->origin);
        sc_render_quad(&r->mesh, vsub(vsub(center, right), up), vadd(vsub(center, up), right),
                       vadd(vadd(center, right), up), vadd(vsub(center, right), up), color, 2.3f);
    }
}
void sc_render_hull_view(Renderer *r, const Game *g, Vec3 eye) {
    if (eye.z > 420)
        return;
    Color hull = rgb(.09f, .115f, .14f), trim = rgb(.18f, .22f, .25f);
    double left = r->origin.x - 6500, right = r->origin.x + 6500;
    double bottom = r->origin.y - 6500, top = r->origin.y + 6500;
    double a[4][4] = {{left, fmin(right, -42), bottom, top},
                      {fmax(left, 42), right, bottom, top},
                      {fmax(left, -42), fmin(right, 42), fmax(bottom, 28), top},
                      {fmax(left, -42), fmin(right, 42), bottom, fmin(top, 0)}};
    for (int i = 0; i < 4; i++)
        if (a[i][1] > a[i][0] && a[i][3] > a[i][2])
            sc_render_block(r, v3((a[i][0] + a[i][1]) * .5, (a[i][2] + a[i][3]) * .5, -2),
                            v3(a[i][1] - a[i][0], a[i][3] - a[i][2], 4), hull, 0);
    if (eye.z < 0) {
        int32_t column = (int32_t)floor(eye.x / 128), row = (int32_t)floor(eye.y / 96);
        /* A fixed world grid makes the perspective converge far along the wall.
           All loops are bounded, independent of the ship or exploration distance. */
        for (int k = -24; k <= 24; k++) {
            double x = (column + k) * 128.0 + 64, y = (row + k) * 96.0 + 32;
            sc_render_block(r, v3(x, r->origin.y, -4.08), v3(3, 12000, .16), trim, .025f);
            sc_render_block(r, v3(r->origin.x, y, -4.07), v3(12000, 1.4, .14), trim, .01f);
        }
        for (int j = -24; j <= 24; j++)
            for (int i = -24; i <= 24; i++) {
                Cell cell = {column + i, row + j, 0};
                uint32_t h = world_hash(g->world.seed, cell, 0x85);
                bool approach_row = (cell.x == -1 || cell.x == 0) && cell.y % 2 == 0;
                bool cross_row = cell.y == 0 && cell.x % 3 == 0;
                if (h % 7 && !approach_row && !cross_row)
                    continue;
                double x = cell.x * 128.0 + 64, y = cell.y * 96.0 + 48;
                Color color = h & 8 ? rgb(.75f, .49f, .24f) : rgb(.23f, .47f, .56f);
                hull_lamp(r, x, y, eye, color);
                /* Tall, dark buttress panels around a few working fittings. */
                sc_render_block(r, v3(x - 5, y + 21, -4.12), v3(1.3, 40, .1), trim, .02f);
                sc_render_block(r, v3(x + 5, y + 21, -4.12), v3(1.3, 40, .1), trim, .02f);
            }
    }
    if (fabs(eye.x) < 800 && fabs(eye.y) < 800) {
        sc_render_block(r, v3(0, 28.7, -4.1), v3(88, 1.4, .18), trim, .08f);
        sc_render_block(r, v3(-42.8, 14, -4.1), v3(1.6, 28, .18), trim, .04f);
        sc_render_block(r, v3(42.8, 14, -4.1), v3(1.6, 28, .18), trim, .04f);
        sc_render_block(r, v3(0, 28.1, -4.22), v3(80, .20, .06), rgb(.24f, .68f, .61f), 1.5f);
        for (int side = -1; side <= 1; side += 2)
            for (int j = 0; j < 3; j++)
                hull_lamp(r, side * 46, 4 + j * 10, eye, rgb(.82f, .56f, .28f));
    }
}
void sc_render_boarding_skiff(Renderer *r, const Game *g, Vec3 eye) {
    if (g->world.generator < SC_SKIFF_GENERATOR || vlen(vsub(eye, v3(0, 6, -98))) > 800)
        return;
    for (size_t i = 0; i < sc_skiff_box_count; i++) {
        const Box *b = &sc_skiff_boxes[i];
        sc_render_block(r, vmul(vadd(b->min, b->max), .5), vsub(b->max, b->min), b->color, b->glow);
    }
}
