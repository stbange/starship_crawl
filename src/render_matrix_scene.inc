/* Included after the renderer's primitive builders. Coordinates are matrix-local. */
static Color matrix_color(unsigned theme) {
    const Color colors[] = {{.22f, .85f, .65f}, {1, .48f, .15f},   {.62f, .38f, .95f},
                            {.95f, .72f, .34f}, {.3f, .65f, 1},    {.9f, .3f, .65f},
                            {.3f, .8f, .85f},   {.4f, .65f, .95f}, {.38f, .9f, .3f}};
    return colors[theme % DISTRICT_TYPES];
}
static void matrix_terminal_mesh(Renderer *r, Vec3 at, double time) {
    Color cyan = rgb(.2f, .8f, .88f);
    block(r, vadd(at, v3(0, .55, 0)), v3(.7, 1.1, .65), rgb(.08f, .15f, .18f), .1f);
    block(r, vadd(at, v3(0, 1.3, 0)), v3(1.15, .65, .15), cyan, .8f);
    for (int i = 0; i < 4; i++)
        block(r, vadd(at, v3(-.35 + i * .23, 1.35, .09)), v3(.07, .2 + .08 * sin(time + i), .04),
              rgb(.65f, .95f, .7f), 1);
    octahedron(r, vadd(at, v3(0, 2.2 + sin(time * 2) * .1, 0)), .2, cyan, 1.5f);
}
static void matrix_scene(Renderer *r, const Game *g, int player) {
    const MatrixDiver *d = &g->matrix.divers[player];
    const MatrixNetwork *w = &g->matrix.networks[d->network];
    const MatrixRecord *record = &g->matrix.records[w->record];
    Color color = matrix_color(w->theme), gold = rgb(1, .72f, .25f), red = rgb(.95f, .16f, .4f);
    for (unsigned i = 0; i < w->box_count; i++)
        if (matrix_box_active(w, record, i)) {
            const MatrixBox *b = &w->boxes[i];
            Vec3 center = vmul(vadd(b->box.min, b->box.max), .5),
                 size = vsub(b->box.max, b->box.min);
            block(r, center, size, b->tag == 2 ? red : b->box.color, b->box.glow);
            if (b->tag == 2)
                for (int y = 0; y < 8; y++)
                    block(r, vadd(center, v3(0, -1.7 + y * .48, 0)),
                          v3(size.x + .04, .025, size.z + .04), gold, 1.2f);
            else if (size.y < 1) {
                for (int side = -1; side <= 1; side += 2) {
                    block(r, vadd(center, v3(side * (size.x * .5 - .06), size.y * .5 + .02, 0)),
                          v3(.045, .04, size.z), color, .8f);
                    block(r, vadd(center, v3(0, size.y * .5 + .02, side * (size.z * .5 - .06))),
                          v3(size.x, .04, .045), color, .8f);
                }
            }
        }
    for (unsigned i = 0; i < 16; i++)
        if (w->nodes[i].valid) {
            const MatrixNode *n = &w->nodes[i];
            Vec3 at = n->at;
            bool known = (record->seen & (1u << i)) != 0;
            matrix_terminal_mesh(r, at, g->time);
            for (int line = -3; line <= 3; line++) {
                block(r, vadd(at, v3(line, .025, 0)), v3(.025, .025, 7), color, .22f);
                block(r, vadd(at, v3(0, .025, line)), v3(7, .025, .025), color, .22f);
            }
            /* Each sector has a different abstract building language. */
            for (int side = -1; side <= 1; side += 2) {
                Vec3 tower = vadd(at, v3(side * 7, 0, 7));
                if (w->theme == SANCTUM) {
                    tube(r, vadd(tower, v3(0, -12, 0)), vadd(tower, v3(0, 42, 0)), .8, .12, color,
                         .16f);
                    for (int h = 0; h < 5; h++)
                        tube(r, vadd(tower, v3(0, 5 + h * 6, 0)), vadd(at, v3(0, 12 + h * 6, 8)),
                             .08, .02, color, .4f);
                } else if (w->theme == BOTANY) {
                    for (int h = 0; h < 5; h++) {
                        Vec3 tip = vadd(tower, v3(sin(h * 3 + i) * 2, 3 + h * 2, cos(h + i) * 2));
                        tube(r, vadd(tower, v3(0, h * 1.5, 0)), tip, .14, .02, color, .7f);
                        octahedron(r, tip, .35, color, .8f);
                    }
                } else if (w->theme == LUXURY || w->theme == HABITAT) {
                    for (int h = 0; h < 4; h++) {
                        block(r, vadd(tower, v3(0, 2 + h * 3, 0)), v3(3, .1, 2), color, .6f);
                        octahedron(r, vadd(tower, v3(0, 3 + h * 3, 0)), .45, gold, .5f);
                    }
                } else if (w->theme == MACHINERY || w->theme == LOGISTICS) {
                    for (int h = 0; h < 6; h++) {
                        block(r, vadd(tower, v3(0, 1 + h * 2, 0)), v3(2.5, 1.6, 2.5),
                              rgb(.08f, .13f, .18f), .1f);
                        tube(r, vadd(tower, v3(-1.4, 1 + h * 2, 0)),
                             vadd(tower, v3(1.4, 1 + h * 2, 0)), .06, .06, color, .8f);
                    }
                } else {
                    for (int h = 0; h < 5; h++)
                        octahedron(r, vadd(tower, v3(0, 2 + h * 3, 0)), 1.3 - h * .18, color, .3f);
                    tube(r, vadd(tower, v3(0, -12, 0)), vadd(tower, v3(0, 18, 0)), .04, .04, color,
                         1);
                }
            }
            if (i == w->credential) {
                Color key = record->keys ? rgb(.12f, .3f, .25f) : gold;
                octahedron(r, vadd(at, v3(3, 1 + sin(g->time * 2) * .15, 0)), .4, key, 1);
                block(r, vadd(at, v3(3, .12, 0)), v3(1, .24, 1), key, .4f);
            }
            if (i == w->entry || i == w->switch_b) {
                unsigned sw = i == w->entry ? 0 : 1;
                Color signal = (record->flags & MX_ROUTED) || w->switches[sw] > 0 ? color : gold;
                block(r, vadd(at, v3(-3, .45, 0)), v3(.65, .9, .65), signal, .9f);
                for (int h = 0; h < 3; h++)
                    octahedron(r, vadd(at, v3(-3, 1.4 + h * .35, 0)), .15, signal, 1);
            }
            if (n->cache || (w->rare && i == w->vault)) {
                Color loot = record->loot & (1u << i)   ? rgb(.08f, .2f, .23f)
                             : w->rare && i == w->vault ? gold
                                                        : color;
                octahedron(r, vadd(at, v3(-3, .9, -3)), w->rare && i == w->vault ? .65 : .35, loot,
                           known ? .9f : .2f);
            }
            if (i == w->core) {
                for (int h = 0; h < 5; h++) {
                    double a = g->time * .4 + h * SC_PI * 2 / 5;
                    Vec3 orb = vadd(at, v3(sin(a) * 1.5, 3.8 + cos(a * 2) * .3, cos(a) * 1.5));
                    octahedron(r, orb, .4, record->outcome ? color : gold, 1);
                    tube(r, vadd(at, v3(0, 3.8, 0)), orb, .02, .02, color, 1);
                }
                octahedron(r, vadd(at, v3(0, 3.8, 0)), 1, color, .7f);
            }
        }
    for (int i = 0; i < SC_MATRIX_GUARDS; i++) {
        const MatrixGuard *e = &w->guards[i];
        if (!e->active)
            continue;
        Vec3 at = e->pos;
        Color shell = e->stun > 0 ? rgb(.22f, .65f, .95f) : red;
        if (e->role == 0) {
            octahedron(r, at, .65, shell, .6f);
            for (int j = -1; j <= 1; j += 2)
                tube(r, vadd(at, v3(j * .5, 0, 0)), vadd(at, v3(j * 1.2, .4, 0)), .09, .01, color,
                     .8f);
        } else if (e->role == 1) {
            for (int j = 0; j < 5; j++)
                octahedron(r, vadd(at, v3(sin(g->time * 7 + j) * .25, 0, -j * .3)), .42 - j * .05,
                           shell, .55f);
        } else if (e->role == 2) {
            block(r, at, v3(.9, .9, .9), shell, .5f);
            octahedron(r, vadd(at, v3(0, .8, 0)), .3, gold, .8f);
        } else {
            octahedron(r, at, .85, gold, .8f);
            for (int side = -1; side <= 1; side += 2)
                tube(r, at, vadd(at, v3(side * 1.7, .3, 0)), .2, .01, red, 1);
        }
        if (e->windup > 0) {
            octahedron(r, vadd(at, v3(0, .9, 0)), .15 + (.65 - e->windup) * .3, gold, 1.5f);
            tube(r, at, e->aim, .012, .012, red, .6f);
        }
    }
    for (int i = 0; i < SC_MATRIX_SHOTS; i++) {
        const MatrixShot *s = &w->shots[i];
        if (!s->active)
            continue;
        Color light = s->owner < 0 ? gold : rgb(.45f, .95f, 1);
        octahedron(r, s->pos, .12, light, 1.5f);
        tube(r, s->pos, vsub(s->pos, vmul(vnorm(s->velocity), .7)), .05, .01, light, 1.4f);
    }
    for (int i = 0; i < g->player_count; i++) {
        const MatrixDiver *other = &g->matrix.divers[i];
        if (!other->active || !cell_equal(other->key, d->key))
            continue;
        if (other->echo_life > 0)
            octahedron(r, other->echo, .5, color, 1);
        if (i == player)
            continue;
        octahedron(r, vadd(other->pos, v3(0, 1, 0)), .6, rgb(.5f, 1, .6f), 1);
    }
}
