#include "render_internal.h"

static void dressing_mesh(Renderer *r, const Dressing *d) {
    Vec3 up = d->normal;
    Vec3 right = vnorm(vcross(up, fabs(up.y) > .8 ? v3(0, 0, 1) : v3(0, 1, 0))),
         forward = vcross(right, up);
    double scale = d->scale, phase = (d->hash % 1000) * .031;
    Color skin = rgb(.20f, .105f, .11f), ridge = rgb(.26f, .19f, .14f);
    if (d->kind == SCRAP) {
        Vec3 ax = vmul(vadd(vmul(right, cos(phase)), vmul(forward, sin(phase))), scale * 1.4);
        Vec3 az = vmul(vcross(vnorm(ax), up), scale * .7);
        sc_render_cube(&r->mesh, vsub(d->pos, r->origin), ax, vmul(up, .06), az,
                       rgb(.18f, .14f, .10f), 0);
        sc_render_tube(r, d->pos, vadd(d->pos, vmul(vnorm(ax), scale)), .05, .025, ridge, 0);
        return;
    }
    if (d->kind == CABLE) {
        Vec3 prev = vadd(d->pos, vmul(up, scale * 3.4));
        for (int j = 1; j <= 5; j++) {
            double t = j / 5.0;
            Vec3 next = vadd(d->pos, vadd(vmul(up, scale * 3.4 * (1 - t)),
                                          vmul(right, sin(t * 4 + phase) * scale * .32)));
            sc_render_tube(r, prev, next, .065, .055, rgb(.085f, .09f, .08f), 0);
            prev = next;
        }
        return;
    }
    int arms = d->kind == NEST ? 4 : 5;
    for (int arm = 0; arm < arms; arm++) {
        double angle = phase + arm * SC_PI * 2 / arms;
        Vec3 outward = vadd(vmul(right, cos(angle)), vmul(forward, sin(angle)));
        Vec3 prev = vadd(d->pos, vmul(up, .15));
        for (int j = 1; j <= 4; j++) {
            double t = j / 4.0;
            Vec3 next = vadd(d->pos, vadd(vmul(outward, t * scale * 4.3),
                                          vadd(vmul(right, sin(t * 5 + angle) * scale * .4),
                                               vmul(up, .07 + sin(t * SC_PI) * .2))));
            sc_render_tube(r, prev, next, (1 - t) * scale * .22 + .035, (1 - t) * scale * .17 + .02,
                           skin, -1);
            prev = next;
        }
    }
    int eggs = d->kind == NEST ? 7 : 2;
    for (int i = 0; i < eggs; i++) {
        double angle = i * 2.4 + phase, spread = d->kind == NEST ? scale : .3;
        Vec3 p = vadd(d->pos,
                      vadd(vmul(right, cos(angle) * spread * .8),
                           vadd(vmul(forward, sin(angle) * spread * .8), vmul(up, scale * .25))));
        sc_render_blob(r, p, vmul(right, scale * .42), vmul(up, scale * .65),
                       vmul(forward, scale * .36), i % 2 ? skin : ridge, -1);
        if (i == 2 || i == 5)
            sc_render_blob(r, vadd(p, vmul(up, scale * .6)), vmul(right, .05), vmul(up, .09),
                           vmul(forward, .06), rgb(.31f, .43f, .19f), .6f);
    }
}
void sc_render_upload_world(Renderer *r, Game *g) {
    World *w = &g->world;
    if (!r->world_revision)
        memset(r->chunk_serial, 0, sizeof(r->chunk_serial));
    r->last_uploads = 0;
    r->world_vertices = 0;
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *c = &w->chunks[i];
        if (!c->active) {
            if (r->chunk_vbo[i])
                gl.DeleteBuffers(1, &r->chunk_vbo[i]);
            r->chunk_vbo[i] = 0;
            r->chunk_serial[i] = 0;
            r->chunk_vertices[i] = 0;
            continue;
        }
        if (r->chunk_serial[i] != c->serial && r->last_uploads < SC_GPU_UPLOAD_BUDGET) {
            r->mesh.count = 0;
            r->origin = v3(0, 0, 0);
            for (size_t j = 0; j < c->box_count; j++) {
                const Box *b = &c->boxes[j];
                sc_render_block(r, vmul(vadd(b->min, b->max), .5), vsub(b->max, b->min), b->color,
                                b->glow);
            }
            for (int j = 0; j < c->flight_count; j++) {
                const Flight *f = &c->flights[j];
                int n = f->stairs ? 32 : 1;
                for (int k = 0; k < n; k++) {
                    double za = f->min.z + (f->max.z - f->min.z) * k / n;
                    double zb = f->min.z + (f->max.z - f->min.z) * (k + 1) / n;
                    double ya = flight_top(f, f->stairs ? (za + zb) * .5 : za);
                    double yb = flight_top(f, f->stairs ? (za + zb) * .5 : zb);
                    sc_render_cube(
                        &r->mesh,
                        v3((f->min.x + f->max.x) * .5, (ya + yb) * .5 - .175, (za + zb) * .5),
                        v3(f->max.x - f->min.x, 0, 0), v3(0, .35, 0), v3(0, yb - ya, zb - za),
                        f->color, 0);
                }
            }
            for (int j = 0; j < c->dressing_count; j++)
                dressing_mesh(r, &c->dressing[j]);
            if (!r->chunk_vbo[i])
                gl.GenBuffers(1, &r->chunk_vbo[i]);
            sc_render_layout(r->chunk_vbo[i]);
            gl.BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(r->mesh.count * sizeof(Vertex)),
                          r->mesh.vertices, GL_STATIC_DRAW);
            r->chunk_vertices[i] = r->mesh.count;
            r->chunk_serial[i] = c->serial;
            r->mesh.count = 0;
            r->last_uploads++;
        }
        if (r->chunk_serial[i] == c->serial)
            r->world_vertices += r->chunk_vertices[i];
    }
    r->world_revision = w->revision ? w->revision : 1;
}
bool sc_render_visible_deck(const World *w, int viewer, int target) {
    return abs(viewer - target) <= (w->generator >= 10 ? 1 : 0);
}
void sc_render_draw_chunks(Renderer *r, Game *g) {
    World *w = &g->world;
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *c = &w->chunks[i];
        if (!c->active || r->chunk_serial[i] != c->serial || !world_ready(w, c->cell))
            continue;
        if (!sc_render_visible_deck(w, world_cell(r->origin).y, c->cell.y))
            continue;
        Vec3 delta = vsub(world_center(c->cell), r->origin);
        if (fabs(delta.x) > 520 || fabs(delta.y) > 480 || fabs(delta.z) > 520)
            continue;
        gl.Uniform3f(r->offset, (float)delta.x, (float)delta.y, (float)delta.z);
        sc_render_layout(r->chunk_vbo[i]);
        gl.DrawArrays(GL_TRIANGLES, 0, (GLsizei)r->chunk_vertices[i]);
    }
    gl.Uniform3f(r->offset, 0, 0, 0);
    /* Visible bulkheads match the collision guard at unfinished passages. */
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *c = &w->chunks[i];
        if (!c->active || !world_ready(w, c->cell))
            continue;
        Vec3 center = world_center(c->cell);
        if (vlen(vsub(center, r->origin)) > 300)
            continue;
        for (int d = 0; d < 4; d++)
            if (!world_ready(w, world_neighbor(c->cell, (Direction)d)) &&
                world_door_version(w->seed, w->generator, c->cell, (Direction)d)) {
                Vec3 p = vadd(center, v3(d == WEST   ? -48
                                         : d == EAST ? 48
                                                     : 0,
                                         2.4,
                                         d == SOUTH   ? -48
                                         : d == NORTH ? 48
                                                      : 0));
                sc_render_block(r, p, d <= EAST ? v3(.12, 4.8, 7) : v3(7, 4.8, .12),
                                rgb(.08f, .18f, .22f), .1f);
                sc_render_block(r, vadd(p, v3(0, -1.2, 0)),
                                d <= EAST ? v3(.16, .12, 6) : v3(6, .12, .16), rgb(.12f, .6f, .7f),
                                1);
            }
    }
}

Vec3 sc_render_focal_anchor(const Room *room) {
    return vadd(world_room_center(room), v3(0, fmax(5, fmin(22, room->height * .6)), 5));
}
Color sc_render_focal_color(const Room *room) {
    return room->district % 3 == 0   ? rgb(.24f, .52f, .62f)
           : room->district % 3 == 1 ? rgb(.6f, .36f, .14f)
                                     : rgb(.36f, .52f, .31f);
}
void sc_render_lights(Renderer *r, const Game *g, const Player *viewer) {
    PointLight selected[SC_VIEW_LIGHTS];
    double scores[SC_VIEW_LIGHTS];
    int count = 0;
    int player = (int)(viewer - g->players);
    bool matrix = g->matrix.divers[player].active;
    if (matrix) {
        const MatrixDiver *d = &g->matrix.divers[player];
        const MatrixNetwork *w = &g->matrix.networks[d->network];
        for (int i = 0; i < 16; i++)
            if (w->nodes[i].valid && vlen(vsub(w->nodes[i].at, d->pos)) < 55 &&
                count < SC_VIEW_LIGHTS)
                selected[count++] = (PointLight){vadd(w->nodes[i].at, v3(0, 4, 0)),
                                                 sc_render_matrix_color(w->theme), 28, 2.2f, 0};
    }
    if (!matrix && viewer->sheltered) {
        const Vec3 lamps[] = {{-6, 3.8, 5}, {6, 3.8, 5}, {-6, 3.8, -3}, {3, 3.8, -4}, {0, 3.8, 1}};
        for (int i = 0; i < 5; i++)
            selected[count++] = (PointLight){
                lamps[i], i == 1 ? rgb(.45f, .75f, .8f) : rgb(1, .77f, .48f), 14, 2.4f, 0};
    }
    Vec3 eye = player_eye(viewer);
    for (int group = 0; !matrix && !viewer->sheltered && group < 8; group++) {
        int total = group == 0   ? SC_MAX_CHUNKS
                    : group == 1 ? SC_MAX_PROJECTILES
                    : group == 3 ? SC_MAX_FLARES
                    : group == 5 ? SC_MAX_DEATH_HAZARDS
                    : group == 6 ? SC_SKIFF_LIGHTS + 2
                    : group == 7 ? 1
                                 : g->player_count;
        for (int i = 0; i < total; i++) {
            const Chunk *chunk = group == 0 ? &g->world.chunks[i] : NULL;
            if (chunk && (!chunk->active ||
                          !sc_render_visible_deck(&g->world, world_cell(eye).y, chunk->cell.y)))
                continue;
            int lamps = chunk ? chunk->light_count : 1;
            for (int j = 0; j < lamps; j++) {
                PointLight light;
                if (chunk) {
                    light = chunk->lights[j];
                    light.pos = vadd(world_center(chunk->cell), light.pos);
                    uint32_t systems = world_systems(&g->world, chunk->cell);
                    const FactionRegion *region =
                        g->factions.version ? factions_at(&g->factions, chunk->cell) : NULL;
                    if (region && (region->flags & FS_DARK) && !(systems & SYSTEM_POWER))
                        light.power *= .35f;
                    if (region && (region->flags & FS_REPAIRED))
                        light.power = fmaxf(light.power, .6f);
                    if (systems & SYSTEM_BLACKOUT)
                        light.power *= .08f;
                    else if (systems & SYSTEM_POWER) {
                        light.power = fmaxf(light.power, 1.15f);
                        light.phase = 0;
                    }
                    light.power *= world_lamp_power(light.phase, g->time);
                } else if (group == 7) {
                    const Room *room = world_find((World *)&g->world, world_cell(eye));
                    if (!room || !room->ready || room->type == BAY || room->height < 9 || eye.z < 0)
                        continue;
                    light = (PointLight){sc_render_focal_anchor(room), sc_render_focal_color(room),
                                         28, .65f, 0};
                } else if (group == 1) {
                    const Projectile *p = &g->projectiles[i];
                    if (!p->active)
                        continue;
                    light = (PointLight){position_absolute(p->pos), p->color, 9, 1.5f, 0};
                } else if (group == 3) {
                    const Flare *f = &g->flares[i];
                    if (!f->active)
                        continue;
                    light = (PointLight){position_absolute(f->pos), rgb(1, .65f, .28f), f->radius,
                                         f->power, 0};
                } else if (group == 5) {
                    const DeathHazard *h = &g->death_hazards[i];
                    if (!h->active || world_cell(position_absolute(h->pos)).y != world_cell(eye).y)
                        continue;
                    light = (PointLight){position_absolute(h->pos),
                                         h->kind == DEATH_ACID ? rgb(.4f, .85f, .12f)
                                                               : rgb(1, .25f, .08f),
                                         h->spent ? 10 : 5, h->spent ? 2 : .55f, 0};
                } else if (group == 6) {
                    if (i < SC_SKIFF_LIGHTS) {
                        if (g->world.generator < SC_SKIFF_GENERATOR)
                            continue;
                        light = sc_skiff_lights[i];
                    } else
                        light = (PointLight){v3(i == SC_SKIFF_LIGHTS ? -46 : 46, 16, -4.7),
                                             rgb(.85f, .58f, .30f), 24, 1.5f, 0};
                } else if (group == 4) {
                    const Beam *b = &g->beams[i];
                    if (b->life <= 0 || g->players[i].sheltered)
                        continue;
                    light = (PointLight){position_absolute(b->to), b->color, 7, 2, 0};
                } else {
                    const Player *p = &g->players[i];
                    if (p->sheltered || p->muzzle <= 0)
                        continue;
                    light = (PointLight){vadd(player_eye(p), facing(p->yaw, p->pitch)),
                                         rgb(1, .5f, .15f), 12, 1.2f, 0};
                }
                double distance = vlen(vsub(light.pos, eye));
                if (distance > 65 || light.power < .01f)
                    continue;
                double score = distance / (light.radius + 1) - (group == 3 ? 3 : group ? 2 : 0);
                int slot = count;
                if (count < SC_VIEW_LIGHTS)
                    count++;
                else if (score >= scores[count - 1])
                    continue;
                else
                    slot = count - 1;
                while (slot > 0 && score < scores[slot - 1]) {
                    selected[slot] = selected[slot - 1];
                    scores[slot] = scores[slot - 1];
                    slot--;
                }
                selected[slot] = light;
                scores[slot] = score;
            }
        }
    }
    GLfloat positions[SC_VIEW_LIGHTS * 4] = {0}, colors[SC_VIEW_LIGHTS * 4] = {0};
    for (int i = 0; i < count; i++) {
        Vec3 p = vsub(selected[i].pos, r->origin);
        positions[i * 4] = (float)p.x;
        positions[i * 4 + 1] = (float)p.y;
        positions[i * 4 + 2] = (float)p.z;
        positions[i * 4 + 3] = selected[i].radius;
        colors[i * 4] = selected[i].color.r;
        colors[i * 4 + 1] = selected[i].color.g;
        colors[i * 4 + 2] = selected[i].color.b;
        colors[i * 4 + 3] = selected[i].power;
    }
    Vec3 dir = facing(viewer->yaw, viewer->pitch);
    GLfloat torch[] = {(float)dir.x, (float)dir.y, (float)dir.z,
                       !matrix && viewer->flashlight ? 1.0f : 0.0f};
    gl.Uniform4fv(r->torch, 1, torch);
#ifdef SC_WEBXR
    Vec3 torch_origin = vsub(player_eye(viewer), r->origin);
    gl.Uniform3f(r->torch_origin, (float)torch_origin.x, (float)torch_origin.y,
                 (float)torch_origin.z);
#endif
    gl.Uniform1f(r->time, (float)fmod(g->time, 4096));
    gl.Uniform1i(r->lamp_count, count);
    gl.Uniform4fv(r->lamp_pos, count, positions);
    gl.Uniform4fv(r->lamp_color, count, colors);
}
