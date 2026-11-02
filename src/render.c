#include "render.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static const char *vertex_source = "layout(location=0) in vec3 aPosition;\n"
                                   "layout(location=1) in vec3 aNormal;\n"
                                   "layout(location=2) in vec3 aColor;\n"
                                   "layout(location=3) in float aGlow;\n"
                                   "uniform mat4 uMVP;uniform vec3 uOffset;out vec3 vPosition;out "
                                   "vec3 vNormal;out vec3 vColor;out float vGlow;\n"
                                   "void "
                                   "main(){vPosition=aPosition+uOffset;vNormal=aNormal;vColor="
                                   "aColor;vGlow=aGlow;gl_Position=uMVP*vec4("
                                   "vPosition,1.0);}\n";
static const char *fragment_source =
    "in vec3 vPosition;in vec3 vNormal;in vec3 vColor;in float vGlow;\n"
    "uniform vec3 uEye;uniform vec3 uGrid;uniform int uUI;out vec4 result;\n"
    "void main(){if(uUI==1){result=vec4(vColor,1.0);return;}\n"
    " vec3 n=normalize(vNormal);vec3 p=vPosition+uGrid;\n"
    " float diffuse=0.44+0.56*max(dot(n,normalize(vec3(-0.35,0.8,-0.5))),0.0);\n"
    " float nearLight=1.0/(1.0+0.0018*dot(vPosition-uEye,vPosition-uEye));\n"
    " vec2 tile=abs(n.y)>0.5?p.xz:(abs(n.x)>0.5?p.zy:p.xy);\n"
    " vec2 cell=fract(tile/4.0);float seam=step(cell.x,0.018)+step(cell.y,0.018);\n"
    " float grain=fract(sin(dot(floor(p*7.0),vec3(12.9898,78.233,39.425)))*43758.5453);\n"
    " vec3 color=vColor*(diffuse+nearLight*0.55)*(0.90+grain*0.12);\n"
    " if(vGlow<0.1)color*=1.0-min(seam,1.0)*0.34;\n"
    " color=mix(color,vColor*(1.0+vGlow*0.3),min(vGlow,1.0));\n"
    " float fog=1.0-exp(-length(vPosition-uEye)*0.0065);\n"
    " color=mix(color,vec3(0.018,0.035,0.051),fog);\n"
    " float d=mod(gl_FragCoord.x,2.0)*0.25+mod(gl_FragCoord.y,2.0)*0.5;\n"
    " color=floor(color*35.0+d)/35.0;result=vec4(color,1.0);}\n";

static GLuint shader(GLenum type, const char *source) {
#ifdef __EMSCRIPTEN__
    const char *header = "#version 300 es\nprecision highp float;\n";
#else
    const char *header = "#version 330 core\n";
#endif
    GLuint id = gl.CreateShader(type);
    const char *sources[] = {header, source};
    gl.ShaderSource(id, 2, sources, NULL);
    gl.CompileShader(id);
    GLint ok = 0;
    gl.GetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        gl.GetShaderInfoLog(id, sizeof(log), NULL, log);
        SDL_Log("Shader: %s", log);
        gl.DeleteShader(id);
        return 0;
    }
    return id;
}
static void layout(GLuint buffer) {
    gl.BindBuffer(GL_ARRAY_BUFFER, buffer);
    gl.EnableVertexAttribArray(0);
    gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    gl.EnableVertexAttribArray(1);
    gl.VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(3 * sizeof(float)));
    gl.EnableVertexAttribArray(2);
    gl.VertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(6 * sizeof(float)));
    gl.EnableVertexAttribArray(3);
    gl.VertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(9 * sizeof(float)));
}
bool renderer_init(Renderer *r) {
    memset(r, 0, sizeof(*r));
    if (!gl_load())
        return false;
    GLuint vs = shader(GL_VERTEX_SHADER, vertex_source),
           fs = shader(GL_FRAGMENT_SHADER, fragment_source);
    if (!vs || !fs) {
        if (vs)
            gl.DeleteShader(vs);
        if (fs)
            gl.DeleteShader(fs);
        return false;
    }
    r->program = gl.CreateProgram();
    gl.AttachShader(r->program, vs);
    gl.AttachShader(r->program, fs);
    gl.LinkProgram(r->program);
    gl.DeleteShader(vs);
    gl.DeleteShader(fs);
    GLint ok;
    gl.GetProgramiv(r->program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        gl.GetProgramInfoLog(r->program, sizeof(log), NULL, log);
        SDL_Log("Program: %s", log);
        renderer_destroy(r);
        return false;
    }
    r->mvp = gl.GetUniformLocation(r->program, "uMVP");
    r->eye = gl.GetUniformLocation(r->program, "uEye");
    r->grid = gl.GetUniformLocation(r->program, "uGrid");
    r->ui = gl.GetUniformLocation(r->program, "uUI");
    r->offset = gl.GetUniformLocation(r->program, "uOffset");
    gl.GenVertexArrays(1, &r->vao);
    gl.GenBuffers(1, &r->vbo);
    gl.GenFramebuffers(1, &r->framebuffer);
    gl.GenTextures(1, &r->texture);
    gl.GenRenderbuffers(1, &r->depth);
    gl.BindVertexArray(r->vao);
    layout(r->vbo);
    gl.DepthFunc(GL_LEQUAL);
    SDL_Log("Renderer: %s / %s", gl.GetString(GL_RENDERER), gl.GetString(GL_VERSION));
    return true;
}
void renderer_destroy(Renderer *r) {
    if (r->program)
        gl.DeleteProgram(r->program);
    if (r->vao)
        gl.DeleteVertexArrays(1, &r->vao);
    if (r->vbo)
        gl.DeleteBuffers(1, &r->vbo);
    if (r->framebuffer)
        gl.DeleteFramebuffers(1, &r->framebuffer);
    if (r->texture)
        gl.DeleteTextures(1, &r->texture);
    if (r->depth)
        gl.DeleteRenderbuffers(1, &r->depth);
    for (int i = 0; i < SC_MAX_CHUNKS; i++)
        if (r->chunk_vbo[i])
            gl.DeleteBuffers(1, &r->chunk_vbo[i]);
    free(r->mesh.vertices);
    memset(r, 0, sizeof(*r));
}
static void vertex(Mesh *m, Vec3 p, Vec3 n, Color c, float e) {
    if (m->count == m->cap) {
        size_t cap = m->cap ? m->cap * 2 : 16384;
        Vertex *v = realloc(m->vertices, cap * sizeof(*v));
        if (!v) {
            m->failed = true;
            return;
        }
        m->vertices = v;
        m->cap = cap;
    }
    m->vertices[m->count++] = (Vertex){(float)p.x, (float)p.y, (float)p.z, (float)n.x, (float)n.y,
                                       (float)n.z, c.r,        c.g,        c.b,        e};
}
static void triangle(Mesh *m, Vec3 a, Vec3 b, Vec3 c, Color color, float glow) {
    Vec3 n = vnorm(vcross(vsub(b, a), vsub(c, a)));
    vertex(m, a, n, color, glow);
    vertex(m, b, n, color, glow);
    vertex(m, c, n, color, glow);
}
static void quad(Mesh *m, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Color color, float glow) {
    triangle(m, a, b, c, color, glow);
    triangle(m, a, c, d, color, glow);
}
static void cube(Mesh *m, Vec3 center, Vec3 sx, Vec3 sy, Vec3 sz, Color col, float glow) {
    Vec3 p[8];
    for (int i = 0; i < 8; i++)
        p[i] = vadd(center, vadd(vmul(sx, i & 1 ? .5 : -.5),
                                 vadd(vmul(sy, i & 2 ? .5 : -.5), vmul(sz, i & 4 ? .5 : -.5))));
    static const int faces[6][4] = {{0, 4, 6, 2}, {1, 3, 7, 5}, {0, 1, 5, 4},
                                    {2, 6, 7, 3}, {0, 2, 3, 1}, {4, 5, 7, 6}};
    for (int i = 0; i < 6; i++)
        quad(m, p[faces[i][0]], p[faces[i][1]], p[faces[i][2]], p[faces[i][3]], col, glow);
}
static void block(Renderer *r, Vec3 c, Vec3 size, Color col, float glow) {
    cube(&r->mesh, vsub(c, r->origin), v3(size.x, 0, 0), v3(0, size.y, 0), v3(0, 0, size.z), col,
         glow);
}
static void octahedron(Renderer *r, Vec3 center, double radius, Color col, float glow) {
    center = vsub(center, r->origin);
    Vec3 top = vadd(center, v3(0, radius, 0)), bottom = vsub(center, v3(0, radius, 0));
    Vec3 edge[4];
    for (int i = 0; i < 4; i++)
        edge[i] = vadd(center, v3(cos(i * SC_PI * .5) * radius, 0, sin(i * SC_PI * .5) * radius));
    for (int i = 0; i < 4; i++) {
        triangle(&r->mesh, top, edge[i], edge[(i + 1) % 4], col, glow);
        triangle(&r->mesh, bottom, edge[(i + 1) % 4], edge[i], col, glow);
    }
}
static void multiply(float *out, const float *a, const float *b) {
    float result[16] = {0};
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            for (int k = 0; k < 4; k++)
                result[c * 4 + r] += a[k * 4 + r] * b[c * 4 + k];
    memcpy(out, result, sizeof(result));
}
static void camera(float *matrix, Vec3 eye, double yaw, double pitch, double aspect) {
    float projection[16] = {0}, view[16] = {0};
    double f = 1 / tan(75 * SC_PI / 360), clip_near = .08, clip_far = 420;
    projection[0] = (float)(f / aspect);
    projection[5] = (float)f;
    projection[10] = (float)((clip_far + clip_near) / (clip_near - clip_far));
    projection[11] = -1;
    projection[14] = (float)(2 * clip_far * clip_near / (clip_near - clip_far));
    Vec3 forward = facing(yaw, pitch), right = v3(cos(yaw), 0, -sin(yaw)),
         up = vcross(forward, right);
    view[0] = (float)right.x;
    view[4] = (float)right.y;
    view[8] = (float)right.z;
    view[12] = (float)-vdot(right, eye);
    view[1] = (float)up.x;
    view[5] = (float)up.y;
    view[9] = (float)up.z;
    view[13] = (float)-vdot(up, eye);
    view[2] = (float)-forward.x;
    view[6] = (float)-forward.y;
    view[10] = (float)-forward.z;
    view[14] = (float)vdot(forward, eye);
    view[15] = 1;
    multiply(matrix, projection, view);
}
static void ortho(float *m, float w, float h) {
    memset(m, 0, 16 * sizeof(float));
    m[0] = 2 / w;
    m[5] = -2 / h;
    m[10] = -1;
    m[12] = -1;
    m[13] = 1;
    m[15] = 1;
}
static void flush(Renderer *r) {
    layout(r->vbo);
    gl.BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(r->mesh.count * sizeof(Vertex)), r->mesh.vertices,
                  GL_STREAM_DRAW);
    gl.DrawArrays(GL_TRIANGLES, 0, (GLsizei)r->mesh.count);
    r->mesh.count = 0;
}
static void resize_target(Renderer *r, int w, int h) {
    if (r->width == w && r->height == h)
        return;
    r->width = w;
    r->height = h;
    gl.BindTexture(GL_TEXTURE_2D, r->texture);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.BindRenderbuffer(GL_RENDERBUFFER, r->depth);
    gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
    gl.BindFramebuffer(GL_FRAMEBUFFER, r->framebuffer);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->texture, 0);
    gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, r->depth);
    if (gl.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_Log("Incomplete render target");
        r->failed = true;
    }
}
static void upload_world(Renderer *r, Game *g) {
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
        if (r->chunk_serial[i] != c->serial && r->last_uploads < SC_STREAM_BUDGET) {
            r->mesh.count = 0;
            r->origin = v3(0, 0, 0);
            for (size_t j = 0; j < c->box_count; j++) {
                const Box *b = &c->boxes[j];
                block(r, vmul(vadd(b->min, b->max), .5), vsub(b->max, b->min), b->color, b->glow);
            }
            if (!r->chunk_vbo[i])
                gl.GenBuffers(1, &r->chunk_vbo[i]);
            layout(r->chunk_vbo[i]);
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
static void hull_view(Renderer *r) {
    if (r->origin.z > 420)
        return;
    Color hull = rgb(.10f, .14f, .18f), trim = rgb(.21f, .25f, .28f);
    double left = r->origin.x - 1400, right = r->origin.x + 1400;
    double bottom = r->origin.y - 1400, top = r->origin.y + 1400;
    double a[4][4] = {{left, fmin(right, -42), bottom, top},
                      {fmax(left, 42), right, bottom, top},
                      {fmax(left, -42), fmin(right, 42), fmax(bottom, 28), top},
                      {fmax(left, -42), fmin(right, 42), bottom, fmin(top, 0)}};
    for (int i = 0; i < 4; i++)
        if (a[i][1] > a[i][0] && a[i][3] > a[i][2])
            block(r, v3((a[i][0] + a[i][1]) * .5, (a[i][2] + a[i][3]) * .5, -2),
                  v3(a[i][1] - a[i][0], a[i][3] - a[i][2], 4), hull, 0);
    if (fabs(r->origin.x) < 800 && fabs(r->origin.y) < 800) {
        for (int s = -1; s <= 1; s += 2)
            for (int k = 0; k < 12; k++) {
                block(r, v3(s * (45 + k * 32), 65, -5), v3(4, 290, 10), trim, 0);
                if (k)
                    block(r, v3(s * (60 + k * 32), 40 + (k % 3) * 25, -4.2), v3(2, 35, 1),
                          rgb(.08f, .22f, .3f), .7f);
            }
        block(r, v3(0, 28, -4), v3(84, 3, 8), trim, 0);
        block(r, v3(0, 26.2, -8.1), v3(76, .25, .2), rgb(.24f, .78f, .73f), 2);
        for (int i = 0; i < 180; i++) {
            uint32_t h = world_hash(713, (Cell){i, 0, 0}, 0);
            block(r,
                  v3((double)(h % 2000) / 5 - 200, (double)((h >> 12) % 1000) / 4 - 100,
                     -110 - (i % 5) * 20),
                  v3(.15, .15, .15), rgb(.32f, .46f, .55f), 2);
        }
    }
}
static void draw_chunks(Renderer *r, Game *g) {
    World *w = &g->world;
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *c = &w->chunks[i];
        if (!c->active || r->chunk_serial[i] != c->serial || !world_ready(w, c->cell))
            continue;
        Vec3 delta = vsub(world_center(c->cell), r->origin);
        if (fabs(delta.x) > 520 || fabs(delta.y) > 480 || fabs(delta.z) > 520)
            continue;
        gl.Uniform3f(r->offset, (float)delta.x, (float)delta.y, (float)delta.z);
        layout(r->chunk_vbo[i]);
        gl.DrawArrays(GL_TRIANGLES, 0, (GLsizei)r->chunk_vertices[i]);
    }
    gl.Uniform3f(r->offset, 0, 0, 0);
    hull_view(r);
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
                world_door(w->seed, c->cell, (Direction)d)) {
                Vec3 p = vadd(center, v3(d == WEST   ? -48
                                         : d == EAST ? 48
                                                     : 0,
                                         2.4,
                                         d == SOUTH   ? -48
                                         : d == NORTH ? 48
                                                      : 0));
                block(r, p, d <= EAST ? v3(.12, 4.8, 7) : v3(7, 4.8, .12), rgb(.08f, .18f, .22f),
                      .1f);
                block(r, vadd(p, v3(0, -1.2, 0)), d <= EAST ? v3(.16, .12, 6) : v3(6, .12, .16),
                      rgb(.12f, .6f, .7f), 1);
            }
    }
}
static void actors(Renderer *r, Game *g, int viewer) {
    for (int j = 0; j < g->world.count; j++)
        for (int i = 0; i < g->world.rooms[j].enemy_count; i++) {
            Enemy *e = &g->world.rooms[j].enemies[i];
            if (!e->alive || vlen(vsub(position_absolute(e->pos), r->origin)) > 430)
                continue;
            Color colors[] = {{.88f, .16f, .21f}, {.65f, .27f, .92f}, {.96f, .53f, .13f}};
            Color col = e->flash > 0 ? rgb(1, .9f, .6f) : colors[e->kind];
            Vec3 pos = vadd(position_absolute(e->pos), v3(0, .78 + sin(g->time * 2 + i) * .08, 0));
            if (e->kind == 2)
                block(r, pos, v3(1.4, 1.4, 1.4), col, .2f);
            else
                octahedron(r, pos, e->kind == 0 ? .7 : .85, col, .4f);
            block(r, vadd(pos, v3(0, .1, -.71)), v3(.55, .12, .08), rgb(1, .8f, .4f), 2);
        }
    for (int i = 0; i < g->player_count; i++)
        if (i != viewer) {
            Player *p = &g->players[i];
            if (vlen(vsub(position_absolute(p->pos), r->origin)) > 430)
                continue;
            Color color = p->tint % 3 == 0
                              ? rgb(.22f, .65f, .65f)
                              : (p->tint % 3 == 1 ? rgb(.75f, .48f, .24f) : rgb(.62f, .39f, .73f));
            block(r, vadd(position_absolute(p->pos), v3(0, .7, 0)), v3(.65, 1.35, .4), color, .1f);
            octahedron(r, vadd(position_absolute(p->pos), v3(0, 1.5, 0)), .35, rgb(.7f, .72f, .65f),
                       .2f);
        }
}
static void weapon(Renderer *r, const Player *p) {
    Vec3 f = facing(p->yaw, p->pitch), right = v3(cos(p->yaw), 0, -sin(p->yaw)),
         up = vcross(f, right);
    Vec3 center = vsub(
        vadd(player_eye(p), vadd(vmul(right, .35), vadd(vmul(up, -.3), vmul(f, .65 - p->muzzle)))),
        r->origin);
    Color tint = p->tint % 3 == 0
                     ? rgb(.18f, .39f, .4f)
                     : (p->tint % 3 == 1 ? rgb(.55f, .32f, .15f) : rgb(.41f, .25f, .53f));
    cube(&r->mesh, center, vmul(right, .19), vmul(up, .20), vmul(f, .62), rgb(.23f, .27f, .28f), 0);
    cube(&r->mesh, vadd(center, vmul(up, -.1)), vmul(right, .23), vmul(up, .15), vmul(f, .28), tint,
         .3f);
    cube(&r->mesh, vadd(center, vmul(up, .12)), vmul(right, .05), vmul(up, .05), vmul(f, .4),
         rgb(.78f, .56f, .25f), .5f);
    cube(&r->mesh, vadd(center, vmul(f, .32)), vmul(right, .12), vmul(up, .12), vmul(f, .18),
         rgb(.12f, .18f, .19f), 0);
    if (p->muzzle > 0) {
        Vec3 flash = vadd(center, vmul(f, .48));
        cube(&r->mesh, flash, vmul(right, .18), vmul(up, .18), vmul(f, .18), rgb(1, .8f, .3f), 3);
    }
}

/* Original 5 x 7 bitmap glyphs. UI stays crisp independently of scene resolution. */
static const unsigned char glyphs[][7] = {
    {14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30}, {14, 17, 16, 16, 16, 17, 14},
    {30, 17, 17, 17, 17, 17, 30}, {31, 16, 16, 30, 16, 16, 31}, {31, 16, 16, 30, 16, 16, 16},
    {14, 17, 16, 23, 17, 17, 15}, {17, 17, 17, 31, 17, 17, 17}, {14, 4, 4, 4, 4, 4, 14},
    {7, 2, 2, 2, 18, 18, 12},     {17, 18, 20, 24, 20, 18, 17}, {16, 16, 16, 16, 16, 16, 31},
    {17, 27, 21, 21, 17, 17, 17}, {17, 25, 25, 21, 19, 19, 17}, {14, 17, 17, 17, 17, 17, 14},
    {30, 17, 17, 30, 16, 16, 16}, {14, 17, 17, 17, 21, 18, 13}, {30, 17, 17, 30, 20, 18, 17},
    {15, 16, 16, 14, 1, 1, 30},   {31, 4, 4, 4, 4, 4, 4},       {17, 17, 17, 17, 17, 17, 14},
    {17, 17, 17, 17, 17, 10, 4},  {17, 17, 17, 21, 21, 27, 17}, {17, 17, 10, 4, 10, 17, 17},
    {17, 17, 10, 4, 4, 4, 4},     {31, 1, 2, 4, 8, 16, 31},     {14, 17, 19, 21, 25, 17, 14},
    {4, 12, 4, 4, 4, 4, 14},      {14, 17, 1, 2, 4, 8, 31},     {30, 1, 1, 14, 1, 1, 30},
    {2, 6, 10, 18, 31, 2, 2},     {31, 16, 16, 30, 1, 1, 30},   {14, 16, 16, 30, 17, 17, 14},
    {31, 1, 2, 4, 8, 8, 8},       {14, 17, 17, 14, 17, 17, 14}, {14, 17, 17, 15, 1, 1, 14},
    {0, 0, 0, 31, 0, 0, 0},       {1, 2, 2, 4, 8, 8, 16},       {0, 4, 4, 31, 4, 4, 0},
    {0, 4, 4, 0, 4, 4, 0},        {0, 0, 0, 0, 0, 6, 6},        {14, 8, 8, 8, 8, 8, 14},
    {14, 2, 2, 2, 2, 2, 14},      {4, 4, 4, 4, 4, 0, 4},        {0, 8, 4, 2, 4, 8, 0},
    {0, 2, 4, 8, 4, 2, 0},        {4, 4, 4, 4, 4, 4, 4}};
static int glyph(char c) {
    c = (char)toupper((unsigned char)c);
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= '0' && c <= '9')
        return 26 + c - '0';
    const char *extra = "-/+:.[]!><|";
    const char *p = strchr(extra, c);
    return p ? 36 + (int)(p - extra) : -1;
}
static void rect(Renderer *r, float x, float y, float w, float h, Color c) {
    quad(&r->mesh, v3(x, y, 0), v3(x + w, y, 0), v3(x + w, y + h, 0), v3(x, y + h, 0), c, 0);
}
static void text_at(Renderer *r, float x, float y, float scale, const char *text, Color color) {
    for (; *text; text++, x += scale * 6) {
        int g = glyph(*text);
        if (g < 0)
            continue;
        for (int row = 0; row < 7; row++)
            for (int col = 0; col < 5; col++)
                if (glyphs[g][row] & (1 << (4 - col)))
                    rect(r, x + col * scale, y + row * scale, scale, scale, color);
    }
}
static void centered(Renderer *r, float x, float y, float s, const char *text, Color c) {
    text_at(r, x - (float)strlen(text) * s * 3, y, s, text, c);
}
static void frame(Renderer *r, float x, float y, float w, float h, Color c) {
    rect(r, x, y, w, 1, c);
    rect(r, x, y + h - 1, w, 1, c);
    rect(r, x, y, 1, h, c);
    rect(r, x + w - 1, y, 1, h, c);
}

static void hud(Renderer *r, Game *g, int player, int width, int height, bool paused, bool touch,
                bool debug, double fps, const char *save_status) {
    Player *p = &g->players[player];
    float w = (float)width, h = (float)height, s = fmaxf(.85f, fminf(w / 960, h / 540));
    Color ink = rgb(.78f, .80f, .72f), dim = rgb(.36f, .48f, .49f), gold = rgb(.84f, .65f, .36f),
          mint = rgb(.27f, .79f, .69f), panel = rgb(.025f, .052f, .063f);
    float matrix[16];
    ortho(matrix, w, h);
    gl.UniformMatrix4fv(r->mvp, 1, GL_FALSE, matrix);
    gl.Uniform1i(r->ui, 1);
    gl.Uniform3f(r->offset, 0, 0, 0);
    gl.Disable(GL_DEPTH_TEST);
    float pad = 24 * s;
    char label[160];
    if (w >= 600) {
        rect(r, pad, pad, 3 * s, 25 * s, gold);
        text_at(r, pad + 12 * s, pad, 2 * s, "STARSHIP", ink);
        text_at(r, pad + 12 * s, pad + 17 * s, 1.3f * s, "C R A W L   /   0.1", dim);
    }
    Cell cell = world_cell(position_absolute(p->pos));
    Room *room = world_find(&g->world, cell);
    const char *name = position_absolute(p->pos).z < 0
                           ? "THE SILENT EXPANSE"
                           : (room ? world_room_name(room->type) : "BETWEEN DECKS");
    centered(r, w * .5f, pad, 1.6f * s, name, gold);
    snprintf(label, sizeof(label), "DEPTH %03u  /  DECK %+d",
             position_absolute(p->pos).z < 0 ? 0 : world_difficulty(cell), cell.y);
    centered(r, w * .5f, pad + 18 * s, 1.1f * s, label, dim);
    if (room && position_absolute(p->pos).z >= 0)
        centered(r, w * .5f, pad + 33 * s, .9f * s, world_district_name(room->district), dim);
    if (g->world.pending)
        centered(r, w * .5f, h * .8f, 1 * s, "SURVEYING NEARBY PASSAGES", dim);
    snprintf(label, sizeof(label), "SHIP %08X", g->world.seed);
    if (w >= 600)
        text_at(r, w - pad - 13 * 7 * s, pad, 1.1f * s, label, dim);
    if (g->player_count == 2) {
        snprintf(label, sizeof(label), "PLAYER %d", player + 1);
        text_at(r, w - pad - 90 * s, pad + 18 * s, 1.2f * s, label, mint);
    }
    if (!paused) {
        float cx = w * .5f, cy = h * .5f;
        rect(r, cx - 8 * s, cy, 5 * s, 1, ink);
        rect(r, cx + 4 * s, cy, 5 * s, 1, ink);
        rect(r, cx, cy - 8 * s, 1, 5 * s, ink);
        rect(r, cx, cy + 4 * s, 1, 5 * s, ink);
    }
    float by = h - 74 * s;
    rect(r, pad - 8 * s, by - 10 * s, 255 * s, 62 * s, panel);
    snprintf(label, sizeof(label), "VITALITY  %03d / %03d", (int)ceilf(p->health),
             (int)player_max_health(p));
    text_at(r, pad, by, 1.6f * s, label, ink);
    rect(r, pad, by + 18 * s, 230 * s, 6 * s, rgb(.12f, .20f, .21f));
    rect(r, pad, by + 18 * s, 230 * s * p->health / player_max_health(p), 6 * s,
         p->hurt > 0 ? rgb(.9f, .25f, .19f) : mint);
    snprintf(label, sizeof(label), "RANK %02u  /  %u ELIMINATED", p->level, p->kills);
    text_at(r, pad, by + 34 * s, 1.1f * s, label, gold);
    float exp = (float)p->xp / (float)player_next_level(p);
    rect(r, 0, h - 3, w * exp, 3, gold);
    if (!touch) {
        text_at(r, w - 260 * s, by, 1.3f * s, "RELIQUARY / PULSE CARBINE", ink);
        snprintf(label, sizeof(label), "DAMAGE %u   CORE %u", 27 + 11 * p->power, p->points);
        text_at(r, w - 260 * s, by + 19 * s, 1.1f * s, label, gold);
        text_at(r, w - 260 * s, by + 36 * s, 1 * s, "ESC PAUSE  /  F5 SAVE  /  TAB HELP", dim);
    }
    if (p->points) {
        centered(r, w * .5f, h - 115 * s, 1.4f * s, "ASCENSION AVAILABLE", gold);
        centered(r, w * .5f, h - 97 * s, 1.1f * s,
                 touch ? "OPEN MENU TO CHOOSE AN UPGRADE" : "1 DAMAGE  /  2 VITALITY  /  3 HASTE",
                 ink);
    }
    Vec3 lift = world_lift(cell), d = vsub(position_absolute(p->pos), lift);
    d.y = 0;
    if (position_absolute(p->pos).z >= 0 && vlen(d) < 3.3)
        centered(r, w * .5f, h * .65f, 1.7f * s,
                 touch ? "GRAV LIFT / UP OR DOWN" : "GRAV LIFT  /  E UP  /  Q DOWN", mint);
    if (g->message_time > 0 && !paused)
        centered(r, w * .5f, h * .19f, 1.2f * s, g->message, gold);
    if (position_absolute(p->pos).z < 0 && !paused)
        centered(r, w * .5f, h * .72f, 1.2f * s,
                 touch ? "DRAG LEFT TO DRIFT / RIGHT TO AIM"
                       : "WASD TO DRIFT / SPACE TO RISE / CTRL TO DESCEND",
                 ink);
    if (save_status && *save_status)
        text_at(r, pad, h - 13 * s, 1 * s, save_status, dim);
    if (p->hurt > 0) {
        float border = 5 * s;
        Color red = rgb(.55f, .12f, .1f);
        rect(r, 0, 0, w, border, red);
        rect(r, 0, h - border, w, border, red);
        rect(r, 0, 0, border, h, red);
        rect(r, w - border, 0, border, h, red);
    }
    if (touch && !paused) {
        float size = fminf(w, h) * .12f;
        frame(r, w * .13f - size * .5f, h * .73f - size * .5f, size, size, dim);
        centered(r, w * .13f, h * .73f - 3 * s, 1.1f * s, "MOVE", dim);
        const char *names[] = {"FIRE", "JUMP", "CROUCH", "UP", "DOWN"};
        const float xs[] = {.90f, .77f, .64f, .90f, .77f}, ys[] = {.72f, .85f, .85f, .48f, .48f};
        for (int i = 0; i < 5; i++) {
            float x = w * xs[i], y = h * ys[i];
            rect(r, x - size * .5f, y - size * .5f, size, size, panel);
            frame(r, x - size * .5f, y - size * .5f, size, size, dim);
            centered(r, x, y - 4 * s, s, names[i], mint);
        }
    }
    if (debug) {
        snprintf(label, sizeof(label), "%.0f FPS / %d CHUNKS / %d QUEUED / %u BUILT / %u UPLOADS",
                 fps, g->world.chunk_count, g->world.pending, g->world.last_work, r->last_uploads);
        text_at(r, pad, 76 * s, s, label, mint);
        RegionPlan plan;
        world_plan(g->world.seed, world_region(cell), &plan);
        float size = 22 * s, mx = w - pad - 4 * size, my = 100 * s;
        for (int i = 0; i < 16; i++) {
            RoomSpec *spec = &plan.rooms[plan.owner[i]];
            Color colors[] = {{.4f, .5f, .5f}, {.4f, .3f, .5f}, {.5f, .3f, .15f},
                              {.2f, .4f, .5f}, {.2f, .4f, .2f}, {.5f, .4f, .25f}};
            float x = mx + (i % 4) * size, y = my + (i / 4) * size;
            rect(r, x, y, size - 2 * s, size - 2 * s, colors[spec->type]);
            if (plan.doors[i] & (1 << EAST))
                rect(r, x + size - 3 * s, y + size * .4f, 5 * s, size * .2f, mint);
            if (plan.doors[i] & (1 << NORTH))
                rect(r, x + size * .4f, y + size - 3 * s, size * .2f, 5 * s, mint);
            if (cell.x == plan.origin.x + i % 4 && cell.z == plan.origin.z + i / 4)
                frame(r, x, y, size - 2 * s, size - 2 * s, gold);
        }
        if (room) {
            snprintf(label, sizeof(label), "%.0f X %.0f X %.0f M", room->width, room->depth,
                     room->height);
            text_at(r, mx, my + 4 * size + 5 * s, .85f * s, label, gold);
        }
    }
    if (paused) {
        float mw = fminf(530 * s, w - 40), mx = (w - mw) * .5f, my = h * .25f;
        rect(r, mx, my, mw, 260 * s, panel);
        frame(r, mx, my, mw, 260 * s, rgb(.24f, .33f, .32f));
        centered(r, w * .5f, my + 23 * s, 1.3f * s, "A SHIP WITHOUT END", gold);
        centered(r, w * .5f, my + 52 * s, 4 * s, "STARSHIP CRAWL", ink);
        centered(r, w * .5f, my + 100 * s, 1.5f * s, "ENTER / CLICK TO EXPLORE", mint);
        centered(r, w * .5f, my + 135 * s, 1.1f * s, "WASD MOVE   MOUSE AIM   CLICK FIRE", ink);
        centered(r, w * .5f, my + 154 * s, 1.1f * s, "SPACE JUMP   CTRL CROUCH   SHIFT SPRINT",
                 dim);
        centered(r, w * .5f, my + 173 * s, 1.1f * s, "E / Q LIFTS   1 / 2 / 3 UPGRADES   C COLOR",
                 dim);
        centered(r, w * .5f, my + 192 * s, 1.1f * s, "F2 OPTIONAL CO-OP   F5 SAVE   F9 LOAD", dim);
        centered(r, w * .5f, my + 223 * s, 1.1f * s, "N + ENTER / NEW SHIP WITH THIS CHARACTER",
                 gold);
        if (g->message_time > 0)
            centered(r, w * .5f, my + 242 * s, .9f * s, g->message, mint);
    }
    flush(r);
}
void renderer_draw(Renderer *r, Game *g, int width, int height, bool paused, bool touch, bool debug,
                   double fps, const char *save_status) {
    if (width <= 0 || height <= 0)
        return;
    gl.UseProgram(r->program);
    gl.BindVertexArray(r->vao);
    upload_world(r, g);
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl.Viewport(0, 0, width, height);
    gl.ClearColor(.008f, .014f, .022f, 1);
    gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    bool vertical = width >= height * 1.5;
    int count = g->player_count;
    for (int p = 0; p < count; p++) {
        int vw = count == 1 ? width : (vertical ? width / 2 : width),
            vh = count == 1 ? height : (vertical ? height : height / 2);
        int vx = count == 1 ? 0 : (vertical ? p * vw : 0),
            vy = count == 1 ? 0 : (vertical ? 0 : (1 - p) * vh);
        int rh = touch ? 240 : 360, rw = (int)((double)rh * vw / vh);
        if (rw < 160)
            rw = 160;
        if (rw > 960)
            rw = 960;
        resize_target(r, rw, rh);
        gl.BindFramebuffer(GL_FRAMEBUFFER, r->framebuffer);
        gl.Viewport(0, 0, rw, rh);
        gl.ClearColor(.012f, .024f, .039f, 1);
        gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl.Enable(GL_DEPTH_TEST);
        gl.Uniform1i(r->ui, 0);
        Player *player = &g->players[p];
        r->origin = world_center(world_cell(position_absolute(player->pos)));
        Vec3 eye = vsub(player_eye(player), r->origin);
        float m[16];
        camera(m, eye, player->yaw, player->pitch, (double)vw / vh);
        gl.UniformMatrix4fv(r->mvp, 1, GL_FALSE, m);
        gl.Uniform3f(r->eye, (float)eye.x, (float)eye.y, (float)eye.z);
        gl.Uniform3f(r->grid, (float)fmod(r->origin.x, 4096), (float)fmod(r->origin.y, 4096),
                     (float)fmod(r->origin.z, 4096));
        draw_chunks(r, g);
        actors(r, g, p);
        flush(r);
        if (!paused) {
            gl.Clear(GL_DEPTH_BUFFER_BIT);
            weapon(r, player);
            flush(r);
        }
        gl.BindFramebuffer(GL_READ_FRAMEBUFFER, r->framebuffer);
        gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        gl.BlitFramebuffer(0, 0, rw, rh, vx, vy, vx + vw, vy + vh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.Viewport(vx, vy, vw, vh);
        hud(r, g, p, vw, vh, paused, touch && count == 1, debug, fps, save_status);
    }
    if (r->mesh.failed)
        r->failed = true;
}
bool renderer_capture(int width, int height, const char *path) {
    size_t n = (size_t)width * (size_t)height * 4;
    unsigned char *pixels = malloc(n);
    if (!pixels)
        return false;
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl.ReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    FILE *f = fopen(path, "wb");
    if (!f) {
        free(pixels);
        return false;
    }
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    bool ok = true;
    for (int y = height - 1; y >= 0; y--)
        for (int x = 0; x < width; x++)
            if (fwrite(pixels + ((size_t)y * width + x) * 4, 1, 3, f) != 3)
                ok = false;
    if (fclose(f) != 0)
        ok = false;
    free(pixels);
    return ok;
}
