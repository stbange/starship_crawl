#include "render_internal.h"

/* Procedural pixel atlas: portable C assets, one draw call per camera. */
static const char *sprite_vertex_source =
    "layout(location=0) in vec3 aPosition;layout(location=1) in vec2 aUV;"
    "layout(location=2) in vec4 aColor;layout(location=3) in float aGlow;"
    "uniform mat4 uMVP;out vec2 vUV;out vec4 vColor;out vec3 vPosition;out float vGlow;"
    "void "
    "main(){vUV=aUV;vColor=aColor;vPosition=aPosition;vGlow=aGlow;gl_Position=uMVP*vec4(aPosition,"
    "1.0);}";
static const char *sprite_fragment_source =
    "in vec2 vUV;in vec4 vColor;in vec3 vPosition;in float vGlow;uniform sampler2D uAtlas;"
    "uniform vec3 uEye;uniform vec4 uTorch;uniform vec4 uFog;uniform float uTime;out vec4 result;"
#ifdef SC_WEBXR
    "uniform vec3 uTorchOrigin;"
#endif
    "void main(){vec4 texel=texture(uAtlas,vUV);float alpha=texel.a*vColor.a;"
    "if(alpha<.015)discard;vec3 delta=vPosition-uEye;float d=length(delta);"
#ifdef SC_WEBXR
    "vec3 lampDelta=vPosition-uTorchOrigin;float ld=length(lampDelta);"
    "float "
    "beam=uTorch.w*smoothstep(.79,.95,dot(lampDelta/max(ld,.001),uTorch.xyz))/(1.0+.005*ld*ld);"
#else
    "float beam=uTorch.w*smoothstep(.79,.95,dot(delta/max(d,.001),uTorch.xyz))/(1.0+.005*d*d);"
#endif
    "vec3 c=texel.rgb*vColor.rgb*(.15+abs(vGlow)+beam*.95);"
    "if(vGlow<0.0){float "
    "wave=sin(floor(vUV.x*64.0)*.7+floor(vUV.y*64.0)*.45-uTime*2.0);c*=.65+.35*step(.35,"
    "wave);}"
    "c=mix(c,uFog.rgb,1.0-exp(-d*uFog.a));result=vec4(c,alpha);}";
bool sc_render_atmosphere_init(Renderer *r) {
    GLuint vs = sc_render_shader(GL_VERTEX_SHADER, sprite_vertex_source),
           fs = sc_render_shader(GL_FRAGMENT_SHADER, sprite_fragment_source);
    if (!vs || !fs) {
        if (vs)
            gl.DeleteShader(vs);
        if (fs)
            gl.DeleteShader(fs);
        return false;
    }
    r->sprite_program = gl.CreateProgram();
    gl.AttachShader(r->sprite_program, vs);
    gl.AttachShader(r->sprite_program, fs);
    gl.LinkProgram(r->sprite_program);
    gl.DeleteShader(vs);
    gl.DeleteShader(fs);
    GLint ok = 0;
    gl.GetProgramiv(r->sprite_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        gl.GetProgramInfoLog(r->sprite_program, sizeof(log), NULL, log);
        SDL_Log("Sprites: %s", log);
        return false;
    }
    unsigned char atlas[64 * 64 * 4] = {0};
    for (unsigned tile = 0; tile < SPR_SPRITES; tile++)
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                double px = (x - 7.5) / 7.5, py = (y - 7.5) / 7.5, rad = px * px + py * py;
                uint32_t hash = world_hash(tile + 17, (Cell){x, y, 0}, 991);
                double alpha = 0, gray = .72 + (hash % 20) * .014;
                switch (tile) {
                case SPR_STEAM:
                case SPR_MIST:
                    alpha = fmax(0, 1 - rad) * (.55 + (hash % 40) * .01);
                    break;
                case SPR_RAIN:
                    alpha = abs(x - 8) <= 1 ? (.8 - fabs(py) * .4) : 0;
                    break;
                case SPR_WATER:
                    alpha = (.28 + .2 * (sin(rad * 28 + tile) > .2)) *
                            (fabs(px) < .95 && fabs(py) < .95);
                    break;
                case SPR_MOTE:
                    alpha = rad < .08 ? 1 : rad < .3 ? .25 : 0;
                    break;
                case SPR_FUNGUS:
                    alpha =
                        ((py < .1 && py > -.8 && px * px * 1.2 + (py + .2) * (py + .2) * 3 < 1) ||
                         (abs(x - 8) < 2 && y >= 8))
                            ? 1
                            : 0;
                    gray = (y < 8 ? .8 : .42) + (hash % 9) * .02;
                    break;
                case SPR_CLOTH:
                    alpha = (x > 1 && x < 14 && y < 15 - (x % 4) && y > 0) ? 1 : 0;
                    gray = .4 + (x % 3) * .12;
                    break;
                case SPR_BONES:
                    alpha = ((abs(x - y) < 2 && x > 2 && x < 13) ||
                             (abs(x + y - 15) < 2 && x > 2 && x < 13) || rad < .12)
                                ? 1
                                : 0;
                    break;
                case SPR_PAPERS:
                    alpha =
                        ((x > 2 && x < 7 && y > 1 && y < 8) || (x > 8 && x < 14 && y > 6 && y < 14))
                            ? 1
                            : 0;
                    gray = (y % 3 == 0) ? .4 : .85;
                    break;
                case SPR_GLYPH:
                    alpha = ((rad > .32 && rad < .55) || (abs(x - 8) < 1 && y > 2 && y < 14) ||
                             (abs(y - 8) < 1 && x > 2 && x < 14))
                                ? 1
                                : 0;
                    break;
                case SPR_SPARK:
                    alpha = (x == 8 || y == 8) && rad < .7 ? 1 : rad < .08 ? 1 : 0;
                    break;
                case SPR_ASH:
                    alpha = fabs(px) < .5 && fabs(py) < .45 && hash % 4 ? .8 : 0;
                    gray = .35 + (hash % 20) * .015;
                    break;
                case SPR_CRYSTAL:
                    alpha = fabs(px) * 2.5 + fabs(py) < .95 ? .85 : 0;
                    gray = x < 8 ? .55 : 1;
                    break;
                case SPR_BUBBLE:
                    alpha = rad > .38 && rad < .65 ? .8 : 0;
                    break;
                }
                size_t i = ((tile / 4 * 16 + (unsigned)y) * 64 + tile % 4 * 16 + (unsigned)x) * 4;
                atlas[i] = atlas[i + 1] = atlas[i + 2] = (unsigned char)(fmin(1, gray) * 255);
                atlas[i + 3] = (unsigned char)(fmin(1, alpha) * 255);
            }
    gl.GenTextures(1, &r->sprite_atlas);
    gl.BindTexture(GL_TEXTURE_2D, r->sprite_atlas);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.GenVertexArrays(1, &r->sprite_vao);
    gl.GenBuffers(1, &r->sprite_vbo);
    gl.BindVertexArray(r->sprite_vao);
    gl.BindBuffer(GL_ARRAY_BUFFER, r->sprite_vbo);
    int sizes[] = {3, 2, 4, 1}, offsets[] = {0, 3, 5, 9};
    for (int i = 0; i < 4; i++) {
        gl.EnableVertexAttribArray((GLuint)i);
        gl.VertexAttribPointer((GLuint)i, sizes[i], GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
                               (void *)(uintptr_t)(offsets[i] * sizeof(float)));
    }
    gl.BindVertexArray(r->vao);
    return true;
}
static int sprite_order(const void *a, const void *b) {
    const SpriteDraw *x = a, *y = b;
    if (x->distance != y->distance)
        return x->distance < y->distance ? 1 : -1;
    return x->order < y->order ? -1 : x->order > y->order;
}
void sc_render_atmosphere_draw(Renderer *r, const Game *g, int player, const float *matrix,
                               Vec3 eye, AtmosphereDef atmosphere) {
    const Player *p = &g->players[player];
    Vec3 right = v3(cos(p->yaw), 0, -sin(p->yaw)), up = v3(0, 1, 0);
    Vec3 forward = v3(sin(p->yaw) * cos(p->pitch), sin(p->pitch), cos(p->yaw) * cos(p->pitch));
    unsigned count = 0;
    /* Sorted tile traversal near-to-far makes budget pressure prefer nearby detail. */
    Cell focus = world_cell(position_absolute(p->pos));
    for (int ring = 0; ring <= 2; ring++)
        for (int c = 0; c < SC_MAX_CHUNKS; c++) {
            const Chunk *chunk = &g->world.chunks[c];
            int dx = abs(chunk->cell.x - focus.x), dz = abs(chunk->cell.z - focus.z),
                dy = abs(chunk->cell.y - focus.y);
            int distance = dx > dz ? dx : dz;
            distance = distance > dy ? distance : dy;
            if (!chunk->active || dy > 1 || distance != ring ||
                !world_ready(&g->world, chunk->cell))
                continue;
            Vec3 origin = vsub(world_center(chunk->cell), r->origin);
            const Room *source = world_find((World *)&g->world, chunk->cell);
            bool calmed = (world_local_state(&g->world, source) & LOCAL_RESOLVED) != 0;
            for (int s = 0; s < chunk->sprite_count && count < SC_VIEW_SPRITES; s++) {
                const EnvSprite *a = &chunk->sprites[s];
                Vec3 anchor = vadd(origin, a->pos), delta = vsub(anchor, eye);
                double d = vlen(delta);
                if (d > 165 || (d > 20 && vdot(delta, forward) < -10))
                    continue;
                unsigned copies = a->kind == SPR_RAIN                          ? 28
                                  : a->kind == SPR_STEAM                       ? 10
                                  : a->kind == SPR_MOTE                        ? 12
                                  : a->kind == SPR_MIST                        ? 3
                                  : a->kind == SPR_SPARK                       ? 5
                                  : a->kind >= SPR_ASH && vlen(a->normal) < .5 ? 18
                                                                               : 1;
                for (unsigned i = 0; i < copies && count < SC_VIEW_SPRITES; i++) {
                    uint32_t h = world_hash(a->hash, (Cell){(int32_t)i, 0, 0}, 17);
                    double phase = (h % 10000) / 10000.0, t = g->time;
                    SpriteDraw draw = {.center = anchor,
                                       .right = vmul(right, a->width),
                                       .up = vmul(up, a->height),
                                       .color = sc_atmospheres[a->atmosphere].tint,
                                       .alpha = .7f,
                                       .glow = .12f,
                                       .tile = a->kind,
                                       .order = count};
                    if (a->normal.y > .5) {
                        draw.right = v3(a->width, 0, 0);
                        draw.up = v3(0, 0, a->height);
                    } else if (vlen(a->normal) > .5) {
                        draw.right = vmul(vcross(up, a->normal), a->width);
                        draw.up = vmul(up, a->height);
                    }
                    if (a->kind == SPR_STEAM) {
                        double life = fmod(t * .21 + phase, 1.0);
                        draw.center = vadd(anchor, v3(sin(phase * 20 + t) * life, life * a->height,
                                                      cos(phase * 33 + t * .7) * life));
                        draw.right = vmul(right, a->width * (.4 + life));
                        draw.up = vmul(up, a->width * (.4 + life));
                        draw.alpha = (float)(sin(life * SC_PI) * .24);
                        draw.glow = .25f;
                        if (a->atmosphere == PRESSURE_AIR)
                            draw.alpha *= (float)(.2 + .8 * fmax(0, sin(t * 1.3 + phase)));
                    } else if (a->kind >= SPR_ASH && vlen(a->normal) < .5) {
                        double life = fmod(t * (a->kind == SPR_BUBBLE ? .6 : .11) + phase, 1);
                        draw.center =
                            vadd(anchor, v3((((h >> 8) % 1000) / 999.0 - .5) * a->width +
                                                sin(t * .3 + phase * 20),
                                            (a->kind == SPR_BUBBLE ? life : -life) * a->height,
                                            (((h >> 18) % 1000) / 999.0 - .5) * 4));
                        double size = a->kind == SPR_CRYSTAL ? .35 : .18;
                        draw.right = vmul(right, size);
                        draw.up = vmul(up, size * (a->kind == SPR_CRYSTAL ? 2 : 1));
                        draw.glow = a->kind == SPR_CRYSTAL ? .4f : .1f;
                        draw.alpha = (float)(sin(life * SC_PI) * .6);
                    } else if (a->kind == SPR_RAIN) {
                        double x = ((h >> 8) % 1000) / 999.0 - .5,
                               z = ((h >> 18) % 1000) / 999.0 - .5;
                        double fall =
                            fmod(t * (a->atmosphere == EMBER_AIR ? .14 : .7) + phase, 1.0);
                        draw.center = vadd(anchor, v3(x * a->width, -fall * a->height, z * 4));
                        draw.right = vmul(right, .07);
                        draw.up = vmul(up, a->atmosphere == EMBER_AIR ? .16 : .9);
                        draw.alpha = .55f;
                        draw.glow = .65f;
                        if (a->atmosphere == EMBER_AIR)
                            draw.tile = SPR_SPARK;
                        if (a->atmosphere == FLOODED && a->hash % 3 == 0)
                            draw.color = rgb(.65f, .24f, .13f);
                    } else if (a->kind == SPR_MOTE || a->kind == SPR_SPARK) {
                        draw.center = vadd(anchor, v3(sin(phase * 23 + t * .17) * a->width * .5,
                                                      sin(phase * 37 + t * .25) * a->height * .5,
                                                      cos(phase * 11 + t * .13) * a->width * .35));
                        draw.right = vmul(right, .16);
                        draw.up = vmul(up, .16);
                        draw.alpha = .7f;
                        draw.glow = .9f;
                    } else if (a->kind == SPR_MIST) {
                        draw.center = vadd(anchor, v3(sin(t * .09 + phase * 20) * 9, phase * .5,
                                                      cos(phase * 30) * 8));
                        draw.alpha = .07f;
                        draw.glow = .15f;
                    } else if (a->kind == SPR_WATER) {
                        draw.alpha = a->atmosphere == FROZEN_RUNOFF ? .7f : .42f;
                        draw.glow = a->atmosphere == FROZEN_RUNOFF ? .12f : -.29f;
                        if (a->atmosphere != FROZEN_RUNOFF)
                            draw.center.y += .006 * sin(t + phase * 10);
                    } else if (a->kind == SPR_GLYPH)
                        draw.glow = .25f;
                    else if (a->kind == SPR_BONES || a->kind == SPR_PAPERS)
                        draw.color = rgb(.64f, .57f, .42f);
                    if (calmed && vlen(a->normal) < .5)
                        draw.alpha *= .18f;
                    Vec3 from_eye = vsub(draw.center, eye);
                    draw.distance = vdot(from_eye, from_eye);
                    if (draw.distance < .12)
                        continue;
                    r->sprite_draws[count++] = draw;
                }
            }
        }
    if (g->factions.version)
        for (int i = 0; i < g->world.count; i++) {
            const Room *room = &g->world.rooms[i];
            const FactionRegion *region = factions_at(&g->factions, room->cell);
            if (!room->ready || !region ||
                (!cell_equal(region->site, room->cell) && !room->faction_edit))
                continue;
            bool growth = (region->flags & FS_GROWTH) != 0;
            bool damaged = (region->flags & (FS_DARK | FS_RAIDED)) != 0;
            if (!growth && !damaged && room->faction_edit != EDIT_WEATHER)
                continue;
            Vec3 origin = vsub(world_center(room->cell), r->origin);
            if (vlen(vsub(origin, eye)) > 150)
                continue;
            for (unsigned n = 0; n < 16 && count < SC_VIEW_SPRITES; n++) {
                double phase = n * .43, rise = fmod(g->time * .35 + phase, 3.0);
                Vec3 at = vadd(origin, v3(7 + sin(phase * 5) * 5, growth ? 1 + rise : rise,
                                          7 + cos(phase * 3) * 4));
                SpriteDraw draw = {.center = at,
                                   .right = vmul(right, growth ? .25 : 2),
                                   .up = vmul(up, growth ? .25 : 2),
                                   .color = growth ? rgb(.25f, .5f, .32f) : rgb(.45f, .5f, .55f),
                                   .alpha = growth ? .5f : .12f,
                                   .glow = growth ? .7f : .1f,
                                   .tile = growth ? SPR_MOTE : SPR_STEAM,
                                   .order = count};
                if (n < 4 && damaged) {
                    draw.center = vadd(origin, v3(8 + (n % 2) * 1.3, .04, 3 + (n / 2) * 1.5));
                    draw.right = v3(1.5, 0, 0);
                    draw.up = v3(0, 0, 1.5);
                    draw.tile = SPR_BONES;
                    draw.alpha = .8f;
                    draw.color = rgb(.4f, .35f, .26f);
                }
                Vec3 delta = vsub(draw.center, eye);
                draw.distance = vdot(delta, delta);
                r->sprite_draws[count++] = draw;
            }
        }
    qsort(r->sprite_draws, count, sizeof(SpriteDraw), sprite_order);
    unsigned n = 0;
    for (unsigned i = 0; i < count; i++) {
        const SpriteDraw *s = &r->sprite_draws[i];
        static const int corners[] = {0, 1, 2, 0, 2, 3};
        for (int v = 0; v < 6; v++) {
            int corner = corners[v];
            double x = corner == 1 || corner == 2 ? .5 : -.5, y = corner >= 2 ? .5 : -.5;
            Vec3 at = vadd(s->center, vadd(vmul(s->right, x), vmul(s->up, y)));
            float u = (float)((s->tile % 4 * 16 + .5 + (x + .5) * 15) / 64.0),
                  uv = (float)((s->tile / 4 * 16 + .5 + (y + .5) * 15) / 64.0);
            r->sprite_vertices[n++] =
                (SpriteVertex){(float)at.x, (float)at.y, (float)at.z, u,        uv,
                               s->color.r,  s->color.g,  s->color.b,  s->alpha, s->glow};
        }
    }
    r->sprite_drawn[r->active_view] = count;
    gl.UseProgram(r->sprite_program);
    gl.BindVertexArray(r->sprite_vao);
    gl.BindBuffer(GL_ARRAY_BUFFER, r->sprite_vbo);
    gl.BufferData(GL_ARRAY_BUFFER, n * sizeof(SpriteVertex), r->sprite_vertices, GL_DYNAMIC_DRAW);
    gl.UniformMatrix4fv(gl.GetUniformLocation(r->sprite_program, "uMVP"), 1, GL_FALSE, matrix);
    gl.Uniform1f(gl.GetUniformLocation(r->sprite_program, "uTime"), (float)g->time);
    gl.Uniform3f(gl.GetUniformLocation(r->sprite_program, "uEye"), (float)eye.x, (float)eye.y,
                 (float)eye.z);
    GLfloat torch[4] = {(float)forward.x, (float)forward.y, (float)forward.z,
                        p->flashlight ? 1.f : 0.f};
    GLfloat fog[4] = {atmosphere.fog.r, atmosphere.fog.g, atmosphere.fog.b, atmosphere.density};
#ifdef SC_WEBXR
    Vec3 lamp = r->quest_active ? r->quest_lamp_origin : eye;
    if (r->quest_active) {
        torch[0] = (float)r->quest_lamp_direction.x;
        torch[1] = (float)r->quest_lamp_direction.y;
        torch[2] = (float)r->quest_lamp_direction.z;
    }
    gl.Uniform3f(gl.GetUniformLocation(r->sprite_program, "uTorchOrigin"), (float)lamp.x,
                 (float)lamp.y, (float)lamp.z);
#endif
    gl.Uniform4fv(gl.GetUniformLocation(r->sprite_program, "uTorch"), 1, torch);
    gl.Uniform4fv(gl.GetUniformLocation(r->sprite_program, "uFog"), 1, fog);
    gl.BindTexture(GL_TEXTURE_2D, r->sprite_atlas);
    gl.Uniform1i(gl.GetUniformLocation(r->sprite_program, "uAtlas"), 0);
    gl.Enable(GL_BLEND);
    gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl.DepthMask(GL_FALSE);
    gl.DrawArrays(GL_TRIANGLES, 0, (GLsizei)n);
    gl.DepthMask(GL_TRUE);
    gl.Disable(GL_BLEND);
    gl.BindVertexArray(r->vao);
    gl.UseProgram(r->program);
}
