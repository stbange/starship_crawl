/* Living limbs use a continuous six-sided skin. Their root stays fixed while
   a travelling wave and a hooked tip move in the surface's local frame. */
static Vec3 tentacle_spine(Vec3 root, Vec3 axis, Vec3 bend, double length, double phase,
                           double time, double t) {
    Vec3 side = vcross(axis, bend);
    double wave = time * .85 + phase;
    double rise = sin(t * 2.2) / 1.9;
    double curl = .28 * (1 - cos(t * 2.2));
    double sway = .10 * t * t * sin(wave - t * 3.5);
    curl += .065 * t * t * sin(wave * .73 + t * 2);
    return vadd(root,
                vmul(vadd(vmul(axis, rise), vadd(vmul(bend, curl), vmul(side, sway))), length));
}
static void tentacle_mesh(Renderer *r, Vec3 root, Vec3 axis, Vec3 bend, double length,
                          double radius, double phase, double time, Color skin, Color suckers,
                          Color tip, float glow, int segments, bool detail) {
    Vec3 previous[6], side = vcross(axis, bend);
    Color rim = rgb(suckers.r * 1.35f, suckers.g * 1.35f, suckers.b * 1.35f);
    for (int j = 0; j <= segments; j++) {
        double t = (double)j / segments;
        Vec3 center = tentacle_spine(root, axis, bend, length, phase, time, t);
        Vec3 tangent =
            vnorm(vsub(tentacle_spine(root, axis, bend, length, phase, time, t + .001), center));
        Vec3 across = vnorm(vsub(side, vmul(tangent, vdot(side, tangent))));
        Vec3 inner = vcross(across, tangent), ring[6];
        double rad = radius * (.06 + .94 * pow(1 - t, 1.15));
        for (int k = 0; k < 6; k++) {
            double a = k * SC_PI / 3;
            ring[k] =
                vsub(vadd(center, vadd(vmul(inner, cos(a) * rad), vmul(across, sin(a) * rad))),
                     r->origin);
        }
        if (j)
            for (int k = 0; k < 6; k++) {
                Color color = k == 0 || k == 5 ? suckers : skin;
                quad(&r->mesh, previous[k], previous[(k + 1) % 6], ring[(k + 1) % 6], ring[k],
                     color, glow);
            }
        memcpy(previous, ring, sizeof(ring));
        /* Small open rings make actual sucker silhouettes, rather than painted dots. */
        if (detail && j > 0 && j < segments - 1 && j % 2 == 0)
            for (int row = -1; row <= 1; row += 2) {
                Vec3 normal = vnorm(vadd(inner, vmul(across, row * .42)));
                Vec3 at = vsub(vadd(center, vmul(normal, rad * 1.02)), r->origin);
                Vec3 lateral = vnorm(vcross(normal, tangent));
                double size = rad * .40;
                for (int k = 0; k < 6; k++) {
                    double a = k * SC_PI / 3, b = (k + 1) * SC_PI / 3;
                    Vec3 u = vadd(vmul(tangent, cos(a)), vmul(lateral, sin(a))),
                         v = vadd(vmul(tangent, cos(b)), vmul(lateral, sin(b)));
                    quad(&r->mesh, vadd(at, vmul(u, size)), vadd(at, vmul(v, size)),
                         vadd(at, vadd(vmul(v, size * .45), vmul(normal, size * .35))),
                         vadd(at, vadd(vmul(u, size * .45), vmul(normal, size * .35))), rim, glow);
                }
            }
        if (j == segments)
            octahedron(r, center, radius * .13, tip, .35f + .12f * (float)sin(time + phase));
    }
}

static void infestation_tentacles(Renderer *r, Game *g, int viewer) {
    typedef struct {
        const Dressing *dressing;
        Vec3 origin;
        double distance;
    } Root;
    Root roots[SC_TENTACLE_ROOTS];
    unsigned count = 0;
    const Player *p = &g->players[viewer];
    Vec3 eye = player_eye(p),
         forward = v3(sin(p->yaw) * cos(p->pitch), sin(p->pitch), cos(p->yaw) * cos(p->pitch));
    Cell focus = world_cell(eye);
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *c = &g->world.chunks[i];
        if (!c->active || r->chunk_serial[i] != c->serial || !world_ready(&g->world, c->cell) ||
            !visible_deck(&g->world, focus.y, c->cell.y))
            continue;
        const Room *room = world_find(&g->world, c->cell);
        if (!room || room->type == BAY || room->height < 4)
            continue;
        Vec3 origin = world_center(c->cell);
        for (int j = 0; j < c->dressing_count; j++) {
            const Dressing *d = &c->dressing[j];
            if (d->kind != GROWTH && d->kind != NEST)
                continue;
            Vec3 delta = vsub(vadd(origin, d->pos), eye);
            double distance = vlen(delta);
            if (distance > 85 || vdot(delta, forward) < -7)
                continue;
            unsigned slot = count;
            if (count < SC_TENTACLE_ROOTS)
                count++;
            else if (distance >= roots[count - 1].distance)
                continue;
            else
                slot--;
            while (slot > 0 && distance < roots[slot - 1].distance) {
                roots[slot] = roots[slot - 1];
                slot--;
            }
            roots[slot] = (Root){d, origin, distance};
        }
    }
    for (unsigned i = 0; i < count; i++) {
        const Dressing *d = roots[i].dressing;
        Vec3 axis = d->normal;
        Vec3 right = vnorm(vcross(axis, fabs(axis.y) > .8 ? v3(0, 0, 1) : v3(0, 1, 0)));
        Vec3 side = vcross(axis, right), anchor = vadd(roots[i].origin, d->pos);
        double scale = clampd(d->scale, .5, 2.2), phase = (d->hash % 1000) * .031;
        unsigned arms = d->kind == NEST ? 3 : 2;
        Color skin = d->hash % 2 ? rgb(.23f, .075f, .105f) : rgb(.13f, .17f, .09f);
        Color underside = d->hash % 2 ? rgb(.39f, .19f, .24f) : rgb(.28f, .33f, .16f);
        Color glow = d->hash % 2 ? rgb(.49f, .23f, .54f) : rgb(.38f, .56f, .18f);
        for (unsigned arm = 0; arm < arms; arm++) {
            double angle = phase + arm * SC_PI * 2 / arms;
            Vec3 bend = vadd(vmul(right, cos(angle)), vmul(side, sin(angle)));
            /* Rise from the outer root mat, clear of its eggs and support column. */
            Vec3 root = vadd(anchor, vadd(vmul(bend, scale * 1.4), vmul(axis, .08)));
            tentacle_mesh(r, root, axis, bend, scale * (3.6 + arm * .25), scale * .24,
                          phase + arm * 2.1, g->time, skin, underside, glow, -1,
                          roots[i].distance < 28 ? 12 : 6, roots[i].distance < 20);
            r->tentacle_drawn[viewer]++;
        }
    }
}
