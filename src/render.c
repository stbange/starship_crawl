#include "render.h"
#include "exterior.h"
#include "habitat.h"
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
    "uniform vec3 uEye;uniform vec3 uGrid;uniform int uUI;uniform float uTime;uniform float "
    "uFogDensity;uniform vec3 uFogColor;\n"
    "uniform vec4 uTorch;uniform int uLampCount;uniform vec4 uLampPos[12];uniform vec4 "
    "uLampColor[12];out vec4 result;\n"
    "float lamp(float phase){if(mod(phase,4.0)<0.5)return 0.0;float "
    "t=mod(uTime+phase*.37,7.0+phase*.13);if(t<.3)return .06;if(t<.55)return .4;if(t<1.1)return "
    ".02;return .75+.25*sin(t*2.0+phase);}\n"
    "void main(){if(uUI==1){result=vec4(vColor,1.0);return;}\n"
    " vec3 n=normalize(vNormal);vec3 p=vPosition+uGrid;vec3 fromEye=vPosition-uEye;\n"
    " float distance=length(fromEye);vec3 view=-fromEye/max(distance,.001);\n"
    " vec3 illumination=vec3(.045,.055,.062)*(0.45+0.55*max(n.y,0.0));\n"
    " for(int i=0;i<12;i++){if(i>=uLampCount)break;vec3 delta=uLampPos[i].xyz-vPosition;float "
    "d2=dot(delta,delta);float radius=uLampPos[i].w;\n"
    "  if(d2<radius*radius){float d=sqrt(d2);float falloff=pow(1.0-d/radius,2.0);float "
    "diffuse=max(dot(n,delta/max(d,.001)),0.0);\n"
    "   illumination+=uLampColor[i].rgb*uLampColor[i].w*falloff*(.12+diffuse);}}\n"
    " float cone=smoothstep(.79,.95,dot(fromEye/max(distance,.001),uTorch.xyz));\n"
    " float beam=uTorch.w*cone/(1.0+.0025*distance*distance)*max(0.0,1.0-distance/70.0);\n"
    " illumination+=vec3(1.5,1.35,1.05)*beam*(.15+.95*max(dot(n,view),0.0));\n"
    " vec2 tile=abs(n.y)>.5?p.xz:(abs(n.x)>.5?p.zy:p.xy);vec2 panel=fract(tile/3.0);\n"
    " float seam=step(panel.x,.018)+step(panel.y,.018);\n"
    " float grain=fract(sin(dot(floor(p*9.0),vec3(12.9898,78.233,39.425)))*43758.5453);\n"
    " float stain=smoothstep(.1,.8,sin(p.x*.21+sin(p.z*.17)*2.0)*sin(p.z*.29+p.y*.41));\n"
    " vec3 surface=vColor*(.65+grain*.26)*(1.0-stain*.38);\n"
    " if(vGlow>=0.0&&vGlow<.1){surface*=1.0-min(seam,1.0)*.48;surface=mix(surface,vec3(.10,.046,."
    "023),stain*.28);}\n"
    " if(vGlow<0.0)surface*=.75+.25*sin(p.x*14.0+p.z*9.0+sin(p.y*7.0));\n"
    " vec3 color=surface*illumination;\n"
    " float emissive=vGlow>=4.0?lamp(floor(vGlow-4.0))*.8:max(vGlow,0.0)*.55;\n"
    " color+=vColor*emissive;\n"
    " if(vGlow<0.0)color+=vec3(.13,.16,.12)*pow(max(dot(n,view),0.0),18.0)*beam*.16;\n"
    " float fog=1.0-exp(-distance*uFogDensity);color=mix(color,uFogColor,fog);\n"
    " color=pow(max(color,vec3(0.0)),vec3(.75));\n"
    " float d=mod(gl_FragCoord.x,2.0)*.25+mod(gl_FragCoord.y,2.0)*.5;\n"
    " color=floor(color*42.0+d)/42.0;result=vec4(color,1.0);}\n";

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
#include "render_atmosphere.inc"

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
    preferences_default(&r->preferences);
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
    r->torch = gl.GetUniformLocation(r->program, "uTorch");
    r->time = gl.GetUniformLocation(r->program, "uTime");
    r->fog_density = gl.GetUniformLocation(r->program, "uFogDensity");
    r->fog_color = gl.GetUniformLocation(r->program, "uFogColor");
    r->lamp_count = gl.GetUniformLocation(r->program, "uLampCount");
    r->lamp_pos = gl.GetUniformLocation(r->program, "uLampPos[0]");
    r->lamp_color = gl.GetUniformLocation(r->program, "uLampColor[0]");
    gl.GenVertexArrays(1, &r->vao);
    gl.GenBuffers(1, &r->vbo);
    gl.GenFramebuffers(SC_PLAYERS, r->framebuffer);
    gl.GenTextures(SC_PLAYERS, r->texture);
    gl.GenRenderbuffers(SC_PLAYERS, r->depth);
    gl.BindVertexArray(r->vao);
    layout(r->vbo);
    gl.DepthFunc(GL_LEQUAL);
    /* A fixed celestial sphere has no near-field parallax and is shared by both
       cameras. Cache directions once instead of rebuilding trigonometry per frame. */
    for (int i = 0; i < SC_SPACE_STARS; i++) {
        uint32_t h = world_hash(0x51a7, (Cell){i, 0, 0}, 17);
        double y = 1 - 2 * (i + .5) / SC_SPACE_STARS;
        double radial = sqrt(1 - y * y), angle = i * 2.399963229728653;
        float brightness = .20f + (h % 100) * .0055f;
        r->stars[i] = (SpaceStar){v3(radial * cos(angle), y, radial * sin(angle)),
                                  h % 7 ? rgb(brightness * .83f, brightness * .91f, brightness)
                                        : rgb(brightness, brightness * .87f, brightness * .70f),
                                  .28 + ((h >> 8) % 100) * .0045};
    }
    if (!atmosphere_init(r)) {
        renderer_destroy(r);
        return false;
    }
    SDL_Log("Renderer: %s / %s", gl.GetString(GL_RENDERER), gl.GetString(GL_VERSION));
    return true;
}
void renderer_destroy(Renderer *r) {
    if (r->sprite_program)
        gl.DeleteProgram(r->sprite_program);
    if (r->sprite_vao)
        gl.DeleteVertexArrays(1, &r->sprite_vao);
    if (r->sprite_vbo)
        gl.DeleteBuffers(1, &r->sprite_vbo);
    if (r->sprite_atlas)
        gl.DeleteTextures(1, &r->sprite_atlas);
    if (r->program)
        gl.DeleteProgram(r->program);
    if (r->vao)
        gl.DeleteVertexArrays(1, &r->vao);
    if (r->vbo)
        gl.DeleteBuffers(1, &r->vbo);
    for (int i = 0; i < SC_PLAYERS; i++) {
        if (r->framebuffer[i])
            gl.DeleteFramebuffers(1, &r->framebuffer[i]);
        if (r->texture[i])
            gl.DeleteTextures(1, &r->texture[i]);
        if (r->depth[i])
            gl.DeleteRenderbuffers(1, &r->depth[i]);
    }
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
        triangle(&r->mesh, top, edge[(i + 1) % 4], edge[i], col, glow);
        triangle(&r->mesh, bottom, edge[i], edge[(i + 1) % 4], col, glow);
    }
}
/* Low-sided tapered tubes make roots, cables and articulated limbs readable
   at the game's pixel resolution without importing a model framework. */
static void tube(Renderer *r, Vec3 a, Vec3 b, double ra, double rb, Color color, float glow) {
    Vec3 dir = vnorm(vsub(b, a));
    if (vlen(dir) < .5)
        return;
    Vec3 ref = fabs(dir.y) > .9 ? v3(1, 0, 0) : v3(0, 1, 0);
    Vec3 x = vnorm(vcross(dir, ref)), z = vcross(dir, x), p[8];
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 4; i++) {
            double angle = SC_PI * .5 * i, rad = j ? rb : ra;
            p[j * 4 + i] =
                vsub(vadd(j ? b : a, vadd(vmul(x, cos(angle) * rad), vmul(z, sin(angle) * rad))),
                     r->origin);
        }
    for (int i = 0; i < 4; i++)
        quad(&r->mesh, p[i], p[(i + 1) % 4], p[4 + (i + 1) % 4], p[4 + i], color, glow);
}
static void blob(Renderer *r, Vec3 center, Vec3 x, Vec3 up, Vec3 z, Color color, float glow) {
    Vec3 top = vsub(vadd(center, up), r->origin), bottom = vsub(vsub(center, up), r->origin),
         edge[4];
    for (int i = 0; i < 4; i++)
        edge[i] =
            vsub(vadd(center, vadd(vmul(x, cos(i * SC_PI * .5)), vmul(z, sin(i * SC_PI * .5)))),
                 r->origin);
    for (int i = 0; i < 4; i++) {
        triangle(&r->mesh, top, edge[(i + 1) % 4], edge[i], color, glow);
        triangle(&r->mesh, bottom, edge[i], edge[(i + 1) % 4], color, glow);
    }
}
static void dressing_mesh(Renderer *r, const Dressing *d) {
    Vec3 up = d->normal;
    Vec3 right = vnorm(vcross(up, fabs(up.y) > .8 ? v3(0, 0, 1) : v3(0, 1, 0))),
         forward = vcross(right, up);
    double scale = d->scale, phase = (d->hash % 1000) * .031;
    Color skin = rgb(.20f, .105f, .11f), ridge = rgb(.26f, .19f, .14f);
    if (d->kind == SCRAP) {
        Vec3 ax = vmul(vadd(vmul(right, cos(phase)), vmul(forward, sin(phase))), scale * 1.4);
        Vec3 az = vmul(vcross(vnorm(ax), up), scale * .7);
        cube(&r->mesh, vsub(d->pos, r->origin), ax, vmul(up, .06), az, rgb(.18f, .14f, .10f), 0);
        tube(r, d->pos, vadd(d->pos, vmul(vnorm(ax), scale)), .05, .025, ridge, 0);
        return;
    }
    if (d->kind == CABLE) {
        Vec3 prev = vadd(d->pos, vmul(up, scale * 3.4));
        for (int j = 1; j <= 5; j++) {
            double t = j / 5.0;
            Vec3 next = vadd(d->pos, vadd(vmul(up, scale * 3.4 * (1 - t)),
                                          vmul(right, sin(t * 4 + phase) * scale * .32)));
            tube(r, prev, next, .065, .055, rgb(.085f, .09f, .08f), 0);
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
            tube(r, prev, next, (1 - t) * scale * .22 + .035, (1 - t) * scale * .17 + .02, skin,
                 -1);
            prev = next;
        }
    }
    int eggs = d->kind == NEST ? 7 : 2;
    for (int i = 0; i < eggs; i++) {
        double angle = i * 2.4 + phase, spread = d->kind == NEST ? scale : .3;
        Vec3 p = vadd(d->pos,
                      vadd(vmul(right, cos(angle) * spread * .8),
                           vadd(vmul(forward, sin(angle) * spread * .8), vmul(up, scale * .25))));
        blob(r, p, vmul(right, scale * .42), vmul(up, scale * .65), vmul(forward, scale * .36),
             i % 2 ? skin : ridge, -1);
        if (i == 2 || i == 5)
            blob(r, vadd(p, vmul(up, scale * .6)), vmul(right, .05), vmul(up, .09),
                 vmul(forward, .06), rgb(.31f, .43f, .19f), .6f);
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
static void camera(float *matrix, Vec3 eye, double yaw, double pitch, double aspect,
                   double clip_far) {
    float projection[16] = {0}, view[16] = {0};
    double f = 1 / tan(75 * SC_PI / 360), clip_near = .08;
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
static void resize_target(Renderer *r, int player, int w, int h) {
    if (r->width[player] == w && r->height[player] == h)
        return;
    r->width[player] = w;
    r->height[player] = h;
    r->target_resizes[player]++;
    gl.BindTexture(GL_TEXTURE_2D, r->texture[player]);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.BindRenderbuffer(GL_RENDERBUFFER, r->depth[player]);
    gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
    gl.BindFramebuffer(GL_FRAMEBUFFER, r->framebuffer[player]);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->texture[player],
                            0);
    gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                               r->depth[player]);
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
            for (int j = 0; j < c->flight_count; j++) {
                const Flight *f = &c->flights[j];
                int n = f->stairs ? 32 : 1;
                for (int k = 0; k < n; k++) {
                    double za = f->min.z + (f->max.z - f->min.z) * k / n;
                    double zb = f->min.z + (f->max.z - f->min.z) * (k + 1) / n;
                    double ya = flight_top(f, f->stairs ? (za + zb) * .5 : za);
                    double yb = flight_top(f, f->stairs ? (za + zb) * .5 : zb);
                    cube(&r->mesh,
                         v3((f->min.x + f->max.x) * .5, (ya + yb) * .5 - .175, (za + zb) * .5),
                         v3(f->max.x - f->min.x, 0, 0), v3(0, .35, 0), v3(0, yb - ya, zb - za),
                         f->color, 0);
                }
            }
            for (int j = 0; j < c->dressing_count; j++)
                dressing_mesh(r, &c->dressing[j]);
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
#include "render_exterior.inc"
static bool visible_deck(const World *w, int viewer, int target) {
    return abs(viewer - target) <= (w->generator >= 10 ? 1 : 0);
}
static void draw_chunks(Renderer *r, Game *g) {
    World *w = &g->world;
    for (int i = 0; i < SC_MAX_CHUNKS; i++) {
        const Chunk *c = &w->chunks[i];
        if (!c->active || r->chunk_serial[i] != c->serial || !world_ready(w, c->cell))
            continue;
        if (!visible_deck(w, world_cell(r->origin).y, c->cell.y))
            continue;
        Vec3 delta = vsub(world_center(c->cell), r->origin);
        if (fabs(delta.x) > 520 || fabs(delta.y) > 480 || fabs(delta.z) > 520)
            continue;
        gl.Uniform3f(r->offset, (float)delta.x, (float)delta.y, (float)delta.z);
        layout(r->chunk_vbo[i]);
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
                block(r, p, d <= EAST ? v3(.12, 4.8, 7) : v3(7, 4.8, .12), rgb(.08f, .18f, .22f),
                      .1f);
                block(r, vadd(p, v3(0, -1.2, 0)), d <= EAST ? v3(.16, .12, 6) : v3(6, .12, .16),
                      rgb(.12f, .6f, .7f), 1);
            }
    }
}
typedef struct {
    Vec3 at, right, up, forward;
    double size;
} BodyPose;
static Vec3 anatomical(const BodyPose *p, Vec3 local) {
    return vadd(p->at, vmul(vadd(vmul(p->right, local.x),
                                 vadd(vmul(p->up, local.y), vmul(p->forward, local.z))),
                            p->size));
}
static void organ(Renderer *r, const BodyPose *p, Vec3 at, Vec3 dimensions, Color color,
                  float glow) {
    blob(r, anatomical(p, at), vmul(p->right, dimensions.x * p->size),
         vmul(p->up, dimensions.y * p->size), vmul(p->forward, dimensions.z * p->size), color,
         glow);
}
static void bone(Renderer *r, const BodyPose *p, Vec3 a, Vec3 b, double thickness, Color color,
                 float glow) {
    tube(r, anatomical(p, a), anatomical(p, b), thickness * p->size, thickness * p->size * .45,
         color, glow);
}
static void panel(Renderer *r, const BodyPose *p, Vec3 at, Vec3 size, Color color, float glow) {
    cube(&r->mesh, vsub(anatomical(p, at), r->origin), vmul(p->right, 2 * size.x * p->size),
         vmul(p->up, 2 * size.y * p->size), vmul(p->forward, 2 * size.z * p->size), color, glow);
}
static void body_form(Renderer *r, const Enemy *e, const BodyPose *p, double time, Color shell,
                      Color flesh, float glow) {
    EnemyBody body = sc_enemy_bodies[e->kind];
    int v = body.variant;
    double gait = sin(time * 7 + e->id) * .13;
    if (body.form == HUSK) {
        organ(r, p, v3(0, .15, 0), v3(.45, .65, .3), shell, glow);
        organ(r, p, v3(0, .95, .04), v3(.28, .3, .26), flesh, glow);
        for (int side = -1; side <= 1; side += 2) {
            bone(r, p, v3(side * .22, -.2, 0), v3(side * .26, -.6, gait * side), .14, shell, glow);
            bone(r, p, v3(side * .38, .5, 0), v3(side * .65, -.25, -gait * side), .11, flesh, glow);
            if (v == 2)
                organ(r, p, v3(side * .48, .55, 0), v3(.38, .28, .38), shell, .05f);
            if (v == 3)
                bone(r, p, v3(side * .18, 1.12, 0), v3(side * .48, 1.65, -.1), .11, shell, glow);
            if (v == 4)
                bone(r, p, v3(side * .3, .2, -.1), v3(side * .9, .4, .35), .09, flesh, glow);
        }
        if (v == 1)
            organ(r, p, v3(.7, -.05, .3), v3(.16, .7, .15), rgb(.35f, .35f, .32f), .1f);
        if (v == 4)
            organ(r, p, v3(0, .65, -.4), v3(.5, .9, .2), rgb(.25f, .19f, .12f), glow);
    } else if (body.form == GAZER) {
        organ(r, p, v3(0, .25, 0), v3(.68, .68, .6), shell, glow);
        organ(r, p, v3(0, .3, .55), v3(.36, .36, .1), rgb(.5f, .65f, .35f), .65f);
        organ(r, p, v3(0, .3, .66), v3(.09, .24, .05), rgb(.05f, .07f, .03f), .1f);
        int arms = v == 4 ? 8 : 3 + v;
        for (int i = 0; i < arms; i++) {
            double a = i * SC_PI * 2 / arms, phase = time * 2 + i;
            Vec3 last = v3(cos(a) * .4, 0, sin(a) * .4);
            for (int j = 1; j <= 3; j++) {
                Vec3 next = v3(cos(a) * (.4 + j * .16) + sin(phase + j) * .12, -j * .26,
                               sin(a) * (.4 + j * .16));
                bone(r, p, last, next, .07, flesh, glow);
                last = next;
            }
            if (v == 1)
                bone(r, p, v3(cos(a) * .4, .6, sin(a) * .4), v3(cos(a) * .8, 1.1, sin(a) * .8), .12,
                     shell, glow);
            if (v == 3)
                organ(r, p, v3(cos(a) * .65, .3, sin(a) * .65), v3(.25, .32, .25), flesh, glow);
        }
        if (v == 2)
            for (int i = -1; i <= 1; i += 2)
                bone(r, p, v3(i * .6, -.1, 0), v3(i * .6, 1.1, 0), .08, shell, .2f);
    } else if (body.form == VOID_RAY) {
        organ(r, p, v3(0, .2, 0), v3(.35, .2, .85), shell, glow);
        int wings = v == 4 ? 3 : 1;
        for (int row = 0; row < wings; row++)
            for (int side = -1; side <= 1; side += 2) {
                double span = v == 0 ? 1.05 : v == 2 ? 1.9 : 1.5;
                Vec3 tip =
                    v3(side * span, .2 + sin(time * 3 + row) * .13, row * .42 - (v == 1 ? .35 : 0));
                organ(r, p, v3(side * span * .5, .2, row * .4),
                      v3(span * .6, .08, v == 0 ? .8 : .65), shell, glow);
                bone(r, p, v3(0, .25, row * .4), tip, .09, flesh, glow);
                if (v == 1 || v == 3)
                    for (int i = 0; i < 3; i++)
                        bone(r, p, v3(side * (.6 + i * .3), .2, -.3),
                             v3(side * (.8 + i * .4), .4, -.8), .07, shell, glow);
            }
        bone(r, p, v3(0, .2, -.65), v3(sin(time * 3) * .2, .25, -1.7), .1, flesh, glow);
    } else if (body.form == WORM) {
        int segments = v == 0 ? 3 : v == 4 ? 8 : 6;
        for (int i = segments - 1; i >= 0; i--) {
            double t = (double)i / segments, radius = v == 2 ? .32 : .55;
            Vec3 at =
                v3(sin(time * 3 - i * .6) * .14, -.1 + sin(i * .6 + time * 2) * .07, -i * .35);
            organ(r, p, at, v3(radius * (1 - t * .5), radius * (1 - t * .5), .35),
                  i % 2 ? shell : flesh, glow);
            if (v == 1)
                for (int side = -1; side <= 1; side += 2)
                    bone(r, p, vadd(at, v3(side * .3, 0, 0)),
                         vadd(at, v3(side * .85, -.5, gait * side)), .06, shell, glow);
            if (v == 4)
                bone(r, p, vadd(at, v3(0, .3, 0)), vadd(at, v3(0, .9, 0)), .12, shell, glow);
        }
        if (v == 3)
            for (int i = 0; i < 8; i++) {
                double a = i * SC_PI * .25;
                bone(r, p, v3(cos(a) * .5, sin(a) * .5, .2), v3(cos(a) * .3, sin(a) * .3, .65), .09,
                     rgb(.55f, .48f, .32f), .05f);
            }
        if (v == 2)
            bone(r, p, v3(0, 0, .25), v3(0, .05, .95), .04, rgb(.5f, .18f, .2f), .1f);
    } else if (body.form == ROBOT) {
        Color steel = rgb(.32f, .35f, .37f), dark = rgb(.07f, .09f, .10f);
        Color lamp = v == 2 ? rgb(.25f, .9f, .5f) : v == 4 ? rgb(1, .2f, .06f) : rgb(.2f, .7f, 1);
        panel(r, p, v3(0, .05, 0), v3(v == 3 ? .85 : .55, v == 1 ? .65 : .24, .65), shell, glow);
        panel(r, p, v3(0, .2, .66), v3(.28, .07, .035), lamp, 1.2f);
        if (v == 0) {
            for (int side = -1; side <= 1; side += 2)
                for (int end = -1; end <= 1; end += 2) {
                    bone(r, p, v3(0, 0, 0), v3(side * .95, -.05, end * .65), .09, steel, .05f);
                    panel(r, p, v3(side * .95, -.1, end * .65), v3(.28, .08, .28), dark, .1f);
                    bone(r, p, v3(side * .95 - cos(time * 20) * .3, -.2, end * .65),
                         v3(side * .95 + cos(time * 20) * .3, -.2, end * .65), .04, lamp, 1);
                }
        } else if (v == 1 || v == 2) {
            if (v == 2)
                panel(r, p, v3(0, .49, .05), v3(.10, .30, .10), steel, glow);
            panel(r, p, v3(0, .9, .08), v3(.28, .23, .3), steel, glow);
            panel(r, p, v3(0, .93, .39), v3(.21, .045, .03), lamp, 1);
            for (int side = -1; side <= 1; side += 2) {
                panel(r, p, v3(side * .35, -.6, gait * side), v3(.2, .4, .25), dark, glow);
                panel(r, p, v3(side * .78, .25, .1), v3(.23, .3, .25), steel, glow);
                bone(r, p, v3(side * .78, .2, .1), v3(side * .95, -.25, .6), .1, steel, glow);
                for (int finger = -1; finger <= 1; finger += 2)
                    panel(r, p, v3(side * .95 + finger * .12, -.25, .8), v3(.06, .08, .25),
                          v == 2 ? lamp : steel, v == 2 ? .5f : glow);
            }
            if (v == 2)
                panel(r, p, v3(0, .3, -.7), v3(.25, .6, .18), rgb(.18f, .45f, .3f), .1f);
        } else if (v == 3) {
            for (int side = -1; side <= 1; side += 2) {
                panel(r, p, v3(side * .9, -.38, 0), v3(.22, .25, .9), dark, glow);
                for (int wheel = -1; wheel <= 1; wheel++)
                    organ(r, p, v3(side * 1.12, -.38, wheel * .55), v3(.08, .18, .18), steel, glow);
            }
            panel(r, p, v3(0, .43, 0), v3(.45, .22, .4), steel, glow);
            bone(r, p, v3(0, .43, .3), v3(0, .43, 1.7), .13, steel, glow);
        } else {
            for (int side = -1; side <= 1; side += 2)
                panel(r, p, v3(side * .62, -.25, 0), v3(.16, .32, .4), dark, glow);
            bone(r, p, v3(0, .2, 0), v3(0, .9, 0), .045, steel, glow);
            organ(r, p, v3(0, .9, 0), v3(.12, .12, .12), lamp, 1.4f);
        }
    } else {
        organ(r, p, v3(0, .2, 0), v3(.5, .75, .5), shell, .1f);
        if (v == 0)
            for (int i = 0; i < 3; i++) {
                double a = time + i * SC_PI * 2 / 3;
                organ(r, p, v3(cos(a) * .75, .3, sin(a) * .75), v3(.18, .45, .18), shell, .15f);
            }
        if (v == 1)
            organ(r, p, v3(0, .8, 0), v3(.35, 1.1, .35), shell, .1f);
        if (v == 2)
            for (int i = 0; i < 12; i++) {
                double a = time + i * SC_PI / 6;
                organ(r, p, v3(cos(a) * .95, sin(a) * .95, .1), v3(.12, .22, .15), shell, .25f);
            }
        if (v == 3)
            for (int i = 0; i < 3; i++) {
                double a = i * SC_PI * 2 / 3;
                bone(r, p, v3(0, .1, 0), v3(cos(a) * 1.2, -.6, sin(a) * 1.2), .12, shell, .05f);
            }
        if (v == 4)
            for (int i = -1; i <= 1; i++) {
                organ(r, p, v3(i * .6, .55, 0), v3(.25, i == 0 ? 1.1 : .8, .35), shell, .1f);
                organ(r, p, v3(i * .6, .65, .32), v3(.07, .3, .05), rgb(.25f, .6f, .8f), .75f);
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
            block(r, debris, v3(size * 2, size, size), form == ROBOT ? rgb(.4f, .35f, .25f) : shell,
                  t < .45 ? 1.2f : .05f);
        else
            octahedron(r, debris, size, shell, t < .15 ? .6f : -.2f);
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
        tube(r, first, next, acid ? .06 : .035, .035, color, (float)pulse);
    }
    if (acid) {
        for (int i = 0; i < 5; i++) {
            double a = i * 2.4;
            blob(r, vadd(at, v3(cos(a) * .7, .05, sin(a) * .7)), v3(.7, 0, 0), v3(0, .07, 0),
                 v3(0, 0, .7), color, .35f);
        }
    } else if (h->kind == DEATH_MINE && !h->spent) {
        block(r, vadd(at, v3(0, .15, 0)), v3(.48, .18, .48), rgb(.23f, .24f, .21f), .1f);
        octahedron(r, vadd(at, v3(0, .4, 0)), .13 + pulse * .08, color, 1.8f);
    } else
        octahedron(r, vadd(at, v3(0, .2, 0)), h->spent ? radius * .6 : .18 + pulse * .14, color, 2);
}
static void creature(Renderer *r, const Enemy *e, double time, double distance) {
    Vec3 center = enemy_center(e), up = e->climbing ? e->normal : v3(0, 1, 0);
    Vec3 forward = vsub(e->heading, vmul(up, vdot(e->heading, up)));
    if (vlen(forward) < .1)
        forward = fabs(up.z) < .9 ? v3(0, 0, 1) : v3(1, 0, 0);
    forward = vnorm(forward);
    Vec3 right = vnorm(vcross(up, forward));
    const EnemyDef *def = &sc_enemies[e->kind];
    double size = def->size;
    bool cloaked =
        (def->flags & CLOAK) && distance > 12 && e->flash <= 0 && sin(time * 1.7 + e->id) > .1;
    if (cloaked) {
        octahedron(r, center, .06, rgb(.16f, .2f, .3f), .5f);
        return;
    }
    Color flesh = e->flash > 0 ? rgb(.62f, .63f, .49f) : rgb(.18f, .09f, .12f);
    Color shell = e->flash > 0 ? rgb(.55f, .48f, .36f) : habitat_enemy_color(e);
    float glow = e->flash > 0 ? .25f : -1;
    BodyPose pose = {center, right, up, forward, size};
    body_form(r, e, &pose, time, shell, flesh, glow);
    if ((def->flags & ARMORED) && sc_enemy_bodies[e->kind].form != ROBOT)
        blob(r, vadd(center, vmul(up, size * .35)), vmul(right, size * .86), vmul(up, size * .4),
             vmul(forward, size * .95), rgb(.25f, .24f, .21f), glow);
    if ((def->flags & HOVER) && sc_enemy_bodies[e->kind].form != ROBOT)
        for (int side = -1; side <= 1; side += 2) {
            Vec3 wing =
                vadd(center, vadd(vmul(right, side * size * 2.5), vmul(up, sin(time * 8) * .3)));
            tube(r, center, wing, .12, .025, def->color, -1);
            tube(r, wing, vadd(center, vmul(forward, -size)), .06, .02, def->color, -1);
        }
    if (def->flags & WARD)
        for (int k = 0; k < 8; k++) {
            double a = k * SC_PI / 4 + time * .5;
            Vec3 p = vadd(center,
                          vadd(vmul(right, cos(a) * size * 1.25), vmul(up, sin(a) * size * 1.25)));
            blob(r, vadd(p, vmul(forward, size)), vmul(right, .07), vmul(up, .13),
                 vmul(forward, .05), rgb(.2f, .55f, .8f), .7f);
        }
    if (def->flags & BROOD)
        for (int k = 0; k < 4; k++)
            blob(r, vadd(center, vadd(vmul(up, size * .7), vmul(right, (k - 1.5) * .35))),
                 vmul(right, .25), vmul(up, .4), vmul(forward, .3), rgb(.30f, .21f, .12f), -1);
    if (def->flags & HEALER)
        octahedron(r, vadd(center, vmul(up, size * 1.2)), .18, rgb(.6f, .25f, .85f), 1.6f);
    if (e->burn > 0)
        octahedron(r, vadd(center, vmul(up, size * .8)), .15, rgb(1, .3f, .08f), 2);
    Vec3 face = vadd(center, vadd(vmul(forward, size * .88), vmul(up, size * .18)));
    for (int side = -1; side <= 1; side += 2) {
        Vec3 eye = vadd(face, vmul(right, side * size * .19));
        blob(r, eye, vmul(right, .045), vmul(up, .055), vmul(forward, .04), rgb(.50f, .58f, .31f),
             .9f);
    }
    if (e->windup > 0) {
        double charge = 1 - e->windup / .8, radius = .09 + .25 * charge;
        octahedron(r, vadd(face, vmul(forward, .26)), radius, rgb(.32f, .82f, .36f), 2.4f);
    }
}
static void fold_door(Renderer *r, Vec3 feet, double yaw) {
    Vec3 right = v3(cos(yaw), 0, -sin(yaw)), forward = v3(sin(yaw), 0, cos(yaw));
    for (int side = -1; side <= 1; side += 2)
        cube(&r->mesh, vsub(vadd(feet, vadd(vmul(right, side * 1.05), v3(0, 1.35, 0))), r->origin),
             vmul(right, .22), v3(0, 2.7, 0), vmul(forward, .45), rgb(.32f, .28f, .22f), .15f);
    cube(&r->mesh, vsub(vadd(feet, v3(0, 2.65, 0)), r->origin), vmul(right, 2.3), v3(0, .2, 0),
         vmul(forward, .45), rgb(.7f, .53f, .3f), .6f);
    cube(&r->mesh, vsub(vadd(feet, v3(0, 1.3, 0)), r->origin), vmul(right, 1.85), v3(0, 2.5, 0),
         vmul(forward, .08), rgb(.13f, .48f, .53f), 1.1f);
    for (int i = 0; i < 5; i++)
        cube(&r->mesh, vsub(vadd(feet, v3(0, .35 + i * .44, 0)), r->origin), vmul(right, 1.5),
             v3(0, .025, 0), vmul(forward, .1), rgb(.65f, .75f, .55f), .9f);
}
#include "render_matrix_scene.inc"
#include "render_campaign_scene.inc"
static void fold_interior(Renderer *r, const Game *g, int viewer) {
    Color stone = rgb(.23f, .25f, .27f), wood = rgb(.4f, .27f, .17f);
    block(r, v3(0, -.2, 0), v3(18.4, .4, 16.4), stone, 0);
    block(r, v3(0, 4.7, 0), v3(18.4, .4, 16.4), rgb(.13f, .18f, .23f), 0);
    block(r, v3(-9.15, 2.25, 0), v3(.3, 4.5, 16.4), stone, 0);
    block(r, v3(9.15, 2.25, 0), v3(.3, 4.5, 16.4), stone, 0);
    block(r, v3(0, 2.25, 8.15), v3(18.4, 4.5, .3), stone, 0);
    block(r, v3(0, 2.25, -8.15), v3(18.4, 4.5, .3), stone, 0);
    block(r, v3(0, .012, -.6), v3(4.5, .025, 12), rgb(.24f, .14f, .14f), .04f);
    for (int z = -6; z <= 6; z += 4) {
        block(r, v3(-8.85, 2.2, z), v3(.22, 4.4, .25), wood, .04f);
        block(r, v3(8.85, 2.2, z), v3(.22, 4.4, .25), wood, .04f);
        block(r, v3(0, 4.38, z), v3(18, .22, .25), wood, .04f);
    }
    size_t count;
    const Box *boxes = shelter_furniture(&count);
    for (size_t i = 0; i < count; i++)
        block(r, vmul(vadd(boxes[i].min, boxes[i].max), .5), vsub(boxes[i].max, boxes[i].min),
              boxes[i].color, boxes[i].glow);
    matrix_terminal_mesh(r, v3(3, 0, -6.3), g->time);
    /* Bed, quilt, pillows and reading lamp. */
    block(r, v3(-6.3, .9, 5.55), v3(2.5, .22, 3), rgb(.64f, .64f, .52f), .03f);
    block(r, v3(-6.3, 1.04, 5.2), v3(2.35, .1, 2.1), rgb(.22f, .38f, .4f), .03f);
    block(r, v3(-6.3, 1.08, 6.55), v3(1.8, .24, .55), rgb(.75f, .7f, .54f), .05f);
    block(r, v3(-7.9, 1.7, 6.7), v3(.6, .7, .6), rgb(.85f, .57f, .24f), .7f);
    /* Medical screen, cross and instruments. */
    block(r, v3(6.5, 1.7, 6.3), v3(2, 1, .2), rgb(.12f, .45f, .48f), .55f);
    block(r, v3(6.5, 1.7, 6.15), v3(.85, .18, .1), rgb(.65f, .9f, .65f), 1);
    block(r, v3(6.5, 1.7, 6.14), v3(.18, .75, .1), rgb(.65f, .9f, .65f), 1);
    for (int i = 0; i < 4; i++)
        block(r, v3(5.5 + i * .5, 1.4, 5.4), v3(.16, .3, .16), rgb(.4f, .6f, .65f), .1f);
    /* Kitchen: hob, pot, pantry tins, sink and extraction hood. */
    block(r, v3(-6.9, 1.12, -2), v3(1.2, .04, 1), rgb(.13f, .16f, .17f), .05f);
    block(r, v3(-6.9, 1.18, -2), v3(.7, .08, .7), rgb(.9f, .34f, .12f), .65f);
    block(r, v3(-6.9, 1.4, -2), v3(.55, .4, .55), rgb(.45f, .42f, .32f), .04f);
    block(r, v3(-5.2, 1.14, -2), v3(.8, .06, .8), rgb(.23f, .43f, .47f), .1f);
    block(r, v3(-6.9, 2.6, -2), v3(1.7, .3, 1.5), rgb(.24f, .28f, .3f), 0);
    for (int i = 0; i < 5; i++)
        block(r, v3(-7.6 + i * .58, 1.3, -1.5), v3(.25, .4, .25), rgb(.65f, .47f, .24f), .06f);
    /* Dining mugs and a star window. */
    for (int i = 0; i < 3; i++)
        block(r, v3(-.8 + i * .7, 1, 2), v3(.18, .25, .18), rgb(.65f, .53f, .32f), .05f);
    block(r, v3(0, 2.5, 7.95), v3(5.5, 2.4, .1), rgb(.05f, .1f, .2f), .45f);
    for (int i = 0; i < 24; i++) {
        double x = -2.5 + (i * 37 % 101) * .05, y = 1.45 + (i * 19 % 43) * .05;
        block(r, v3(x, y, 7.86), v3(.035, .035, .02), rgb(.75f, .8f, .7f), 1.5f);
    }
    fold_door(r, v3(0, 0, -7.95), 0);
    for (int i = 0; i < g->player_count; i++)
        if (i != viewer && g->players[i].sheltered) {
            Vec3 p = g->players[i].shelter_pos;
            block(r, vadd(p, v3(0, .7, 0)), v3(.65, 1.35, .4), rgb(.3f, .6f, .55f), .08f);
            octahedron(r, vadd(p, v3(0, 1.5, 0)), .35, rgb(.7f, .72f, .65f), .1f);
        }
}
static void rig_block(Renderer *r, const Rig *rig, Vec3 at, Vec3 size, Color color, float glow) {
    Vec3 right = v3(cos(rig->yaw), 0, -sin(rig->yaw)),
         forward = v3(sin(rig->yaw), 0, cos(rig->yaw));
    Vec3 center = vadd(position_absolute(rig->pos),
                       vadd(vmul(right, at.x), vadd(v3(0, at.y, 0), vmul(forward, at.z))));
    cube(&r->mesh, vsub(center, r->origin), vmul(right, size.x), v3(0, size.y, 0),
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
static void lights(Renderer *r, const Game *g, const Player *viewer) {
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
                                                 matrix_color(w->theme), 28, 2.2f, 0};
    }
    if (!matrix && viewer->sheltered) {
        const Vec3 lamps[] = {{-6, 3.8, 5}, {6, 3.8, 5}, {-6, 3.8, -3}, {3, 3.8, -4}, {0, 3.8, 1}};
        for (int i = 0; i < 5; i++)
            selected[count++] = (PointLight){
                lamps[i], i == 1 ? rgb(.45f, .75f, .8f) : rgb(1, .77f, .48f), 14, 2.4f, 0};
    }
    Vec3 eye = player_eye(viewer);
    for (int group = 0; !matrix && !viewer->sheltered && group < 7; group++) {
        int total = group == 0   ? SC_MAX_CHUNKS
                    : group == 1 ? SC_MAX_PROJECTILES
                    : group == 3 ? SC_MAX_FLARES
                    : group == 5 ? SC_MAX_DEATH_HAZARDS
                    : group == 6 ? SC_SKIFF_LIGHTS + 2
                                 : g->player_count;
        for (int i = 0; i < total; i++) {
            const Chunk *chunk = group == 0 ? &g->world.chunks[i] : NULL;
            if (chunk &&
                (!chunk->active || !visible_deck(&g->world, world_cell(eye).y, chunk->cell.y)))
                continue;
            int lamps = chunk ? chunk->light_count : 1;
            for (int j = 0; j < lamps; j++) {
                PointLight light;
                if (chunk) {
                    light = chunk->lights[j];
                    light.pos = vadd(world_center(chunk->cell), light.pos);
                    uint32_t systems = world_systems(&g->world, chunk->cell);
                    if (systems & SYSTEM_BLACKOUT)
                        light.power *= .08f;
                    else if (systems & SYSTEM_POWER) {
                        light.power = fmaxf(light.power, 1.15f);
                        light.phase = 0;
                    }
                    light.power *= world_lamp_power(light.phase, g->time);
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
    gl.Uniform1f(r->time, (float)fmod(g->time, 4096));
    gl.Uniform1i(r->lamp_count, count);
    gl.Uniform4fv(r->lamp_pos, count, positions);
    gl.Uniform4fv(r->lamp_color, count, colors);
}
static void story_prop(Renderer *r, unsigned scene, Vec3 at, bool done, double time) {
    Color metal = rgb(.13f, .18f, .19f), glow = done ? rgb(.2f, .35f, .29f) : rgb(.3f, .8f, .72f);
    if (scene >= STORY_TRACKS && scene <= STORY_RATIONS) {
        for (int i = 0; i < 5; i++) {
            Vec3 p = vadd(at, v3((i % 2 ? .25 : -.25), .04, (i - 2) * .38));
            if (scene == STORY_CONDUIT)
                tube(r, p, vadd(p, v3(.8, .12, .8)), .025, .025, glow, .4f);
            else if (scene == STORY_FROST || scene == STORY_RESEARCH)
                octahedron(r, vadd(p, v3(0, .12, 0)), .12, rgb(.35f, .5f, .65f), .3f);
            else
                block(r, p, v3(.15, scene == STORY_RATIONS ? .17 : .03, .25), rgb(.45f, .3f, .15f),
                      .2f);
        }
        return;
    }
    block(r, vadd(at, v3(0, .5, 0)), v3(.95, 1, .7), metal, 0);
    if (scene == STORY_REACTOR || scene == STORY_DRIVE || scene == STORY_POWER ||
        scene == STORY_RELAY) {
        for (int i = -1; i <= 1; i++)
            tube(r, vadd(at, v3(i * .42, .8, 0)), vadd(at, v3(i * .42, 2.1, 0)), .16, .11,
                 i ? metal : glow, i ? .1f : 1);
    } else if (scene == STORY_SURVIVORS) {
        for (int i = -1; i <= 1; i += 2) {
            block(r, vadd(at, v3(i * .65, .6, .5)), v3(.6, 1.2, 1.5), metal, 0);
            block(r, vadd(at, v3(i * .65, 1.24, .5)), v3(.4, .08, 1.2), glow, .6f);
        }
    } else if (scene == STORY_ARCHIVE || scene == STORY_MEMORY || scene == STORY_MIRROR) {
        for (int i = -1; i <= 1; i += 2)
            octahedron(r, vadd(at, v3(i * .4, 1.55, 0)), .4, glow, .8f);
    } else {
        block(r, vadd(at, v3(0, 1.3, 0)), v3(1.2, .5, .8), glow, .6f);
        if (scene == STORY_REFUGE || scene == STORY_EVAC || scene == STORY_EXTRACTION)
            tube(r, vadd(at, v3(.55, 1.5, 0)), vadd(at, v3(.55, 2.5, 0)), .04, .02, glow, .5f);
    }
    if (scene == STORY_COUNCIL)
        for (int side = -1; side <= 1; side += 2) {
            Color faction = side < 0 ? rgb(.25f, .65f, .6f) : rgb(.65f, .43f, .18f);
            Vec3 person = vadd(at, v3(side * 1.8, 0, 0));
            block(r, vadd(person, v3(0, .95, 0)), v3(.42, .85, .3), faction, .25f);
            octahedron(r, vadd(person, v3(0, 1.65, 0)), .23, faction, .4f);
            for (int leg = -1; leg <= 1; leg += 2)
                tube(r, vadd(person, v3(leg * .12, .1, 0)), vadd(person, v3(leg * .12, .55, 0)),
                     .06, .08, faction, .2f);
        }
    if (!done)
        octahedron(r, vadd(at, v3(0, 2.8 + sin(time) * .1, 0)), .2, rgb(.85f, .65f, .25f), 1.5f);
}
static void actors(Renderer *r, Game *g, int viewer) {
    for (int i = 0; i < g->world.count; i++) {
        const Room *room = &g->world.rooms[i];
        Vec3 at = matrix_terminal(room->cell);
        if (room->ready && vlen(vsub(at, player_eye(&g->players[viewer]))) < 110)
            matrix_terminal_mesh(r, at, g->time);
    }
    for (int i = 0; i < 16; i++)
        if (g->matrix.links[i].life > 0)
            tube(r, position_absolute(g->matrix.links[i].from),
                 position_absolute(g->matrix.links[i].to), .045, .025, rgb(.25f, 1, .7f), 1.4f);

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
                !visible_deck(&g->world, cell.y, world_cell(enemy_center(e)).y))
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
            octahedron(r, vadd(enemy_center(visible[i]), v3(0, 1.2, 0)), .17, rgb(.2f, 1, .6f),
                       1.1f);
    for (int i = 0; i < SC_MAX_DEATHS; i++) {
        const EnemyDeath *d = &g->deaths[i];
        if (d->active && vlen(vsub(enemy_center(&d->body), eye)) < 130 &&
            visible_deck(&g->world, cell.y, world_cell(enemy_center(&d->body)).y))
            death_mesh(r, d);
    }
    for (int i = 0; i < SC_MAX_DEATH_HAZARDS; i++) {
        const DeathHazard *h = &g->death_hazards[i];
        if (h->active && vlen(position_delta(h->pos, g->players[viewer].pos)) < 130 &&
            visible_deck(&g->world, cell.y, h->pos.sector.y))
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
        octahedron(r, pos, p->owner < 0 ? .20 : .09, color, 2);
        tube(r, pos, vsub(pos, vmul(vnorm(p->velocity), p->owner < 0 ? 1.1 : 1.8)), .055, .02,
             color, 1.3f);
    }
    for (int j = 0; j < g->world.count; j++) {
        const Room *room = &g->world.rooms[j];
        for (int k = 0; k < room->pickup_count; k++) {
            const Pickup *loot = &room->pickups[k];
            Vec3 p = position_absolute(loot->pos);
            if (!loot->active || !visible_deck(&g->world, cell.y, world_cell(p).y) ||
                vlen(vsub(p, eye)) > 100)
                continue;
            p.y += .15 * sin(g->time * 2 + k);
            Color colors[] = {{.8f, .2f, .18f},  {.2f, .5f, 1},     {.75f, .65f, .35f},
                              {.85f, .5f, .15f}, {.65f, .3f, .85f}, {.2f, .9f, .6f},
                              {.7f, .6f, .3f}};
            Color col = colors[loot->kind];
            if (loot->kind == PICK_MEDKIT) {
                block(r, p, v3(.55, .2, .2), col, .7f);
                block(r, p, v3(.2, .55, .2), col, .7f);
            } else if (loot->kind == PICK_WEAPON) {
                block(r, p, v3(.65, .25, .2), col, .7f);
                block(r, vadd(p, v3(.25, .05, 0)), v3(.6, .1, .1), col, .4f);
            } else
                octahedron(r, p, loot->kind == PICK_LOG ? .25 : .35, col, .7f);
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
            tube(r, vadd(at, v3(cos(a) * radius, .2, sin(a) * radius)),
                 vadd(at, v3(cos(b) * radius, .2, sin(b) * radius)), .055, .055, rgb(.6f, .3f, 1),
                 1.5f);
        }
    }
    for (int i = 0; i < g->player_count; i++)
        if (g->players[i].lift_active) {
            Vec3 at = position_absolute(g->players[i].pos);
            block(r, vadd(at, v3(0, -.14, 0)), v3(4.6, .24, 4.6), rgb(.09f, .21f, .25f), .3f);
            for (int side = -1; side <= 1; side += 2)
                block(r, vadd(at, v3(side * 2.25, .04, 0)), v3(.08, .08, 4.5), rgb(.1f, .7f, .9f),
                      2);
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
            tube(r, from, to, .045, .035, beam->color, 2);
            tube(r, from, to, .014, .014, rgb(.9f, .95f, 1), 3);
            octahedron(r, to, .1, beam->color, 2);
        }
    }
    for (int i = 0; i < SC_MAX_FLARES; i++) {
        const Flare *f = &g->flares[i];
        if (!f->active)
            continue;
        Vec3 at = position_absolute(f->pos);
        if (world_cell(at).y != cell.y || vlen(vsub(at, eye)) > 100)
            continue;
        tube(r, at, vadd(at, vmul(f->normal, .3)), .055, .045, rgb(.6f, .24f, .12f), .3f);
        octahedron(r, vadd(at, vmul(f->normal, .25)), .11, rgb(1, .65f, .25f), 2);
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
            tube(r, vadd(at, v3(sin(a) * 18, .06, cos(a) * 18)),
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
            block(r, vadd(p, v3(0, .6, 0)), v3(.9, 1.2, .7), rgb(.18f, .23f, .24f), 0);
            block(r, vadd(p, v3(0, 1.4, 0)), v3(1.2, .6, .8), rgb(.25f, .7f, .65f), .8f);
            octahedron(r, vadd(p, v3(0, 2.4 + sin(g->time) * .1, 0)), .23, rgb(.8f, .65f, .3f),
                       1.5f);
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
    const WeaponDef *gun = game_gun(p);
    if (p->riding == MECH) {
        for (int side = -1; side <= 1; side += 2) {
            Vec3 arm = vsub(
                vadd(player_eye(p), vadd(vmul(right, side * .7), vadd(vmul(up, -.55), vmul(f, 1)))),
                r->origin);
            cube(&r->mesh, arm, vmul(right, .5), vmul(up, .4), vmul(f, 1.2), rgb(.24f, .29f, .3f),
                 0);
            cube(&r->mesh, vadd(arm, vmul(f, .65)), vmul(right, .23), vmul(up, .23), vmul(f, .2),
                 gun->color, p->muzzle > 0 ? 2 : .4f);
        }
        return;
    }
    if (p->riding == WAGON)
        cube(&r->mesh, vsub(vadd(player_eye(p), vadd(vmul(up, -.7), vmul(f, .8))), r->origin),
             vmul(right, 1.3), vmul(up, .35), vmul(f, .8), rgb(.35f, .25f, .14f), 0);
    Color tint = p->tint % 3 == 0
                     ? rgb(.18f, .39f, .4f)
                     : (p->tint % 3 == 1 ? rgb(.55f, .32f, .15f) : rgb(.41f, .25f, .53f));
    cube(&r->mesh, center, vmul(right, .19), vmul(up, .20), vmul(f, .42 + gun->damage * .002),
         rgb(.23f, .27f, .28f), 0);
    cube(&r->mesh, vadd(center, vmul(up, -.1)), vmul(right, .23), vmul(up, .15), vmul(f, .28), tint,
         .3f);
    cube(&r->mesh, vadd(center, vmul(up, .12)), vmul(right, .05), vmul(up, .05), vmul(f, .4),
         gun->color, .5f);
    cube(&r->mesh, vadd(center, vmul(f, .32)), vmul(right, .12), vmul(up, .12), vmul(f, .18),
         rgb(.12f, .18f, .19f), 0);
    if (gun->pellets > 1)
        for (int side = -1; side <= 1; side += 2)
            cube(&r->mesh, vadd(center, vadd(vmul(right, side * .12), vmul(f, .27))),
                 vmul(right, .1), vmul(up, .1), vmul(f, .4), tint, 0);
    if (gun->flags & W_PROJECTILE)
        cube(&r->mesh, vadd(center, vmul(f, .2)), vmul(right, .35), vmul(up, .3), vmul(f, .3), tint,
             .1f);
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
static float wrapped(Renderer *r, float x, float y, float scale, float width, const char *text,
                     Color c) {
    unsigned limit = (unsigned)fmaxf(1, fminf(159, width / (6 * scale)));
    while (*text) {
        unsigned n = (unsigned)strlen(text);
        if (n > limit) {
            n = limit;
            while (n > 1 && text[n] != ' ')
                n--;
            if (n == 1)
                n = limit;
        }
        char line[160];
        memcpy(line, text, n);
        line[n] = 0;
        text_at(r, x, y, scale, line, c);
        text += n;
        while (*text == ' ')
            text++;
        y += 13 * scale;
    }
    return y;
}
static void frame(Renderer *r, float x, float y, float w, float h, Color c) {
    rect(r, x, y, w, 1, c);
    rect(r, x, y + h - 1, w, 1, c);
    rect(r, x, y, 1, h, c);
    rect(r, x + w - 1, y, 1, h, c);
}

static void map_marker(Renderer *r, const MapView *map, Vec3 point, float x, float y, float tile,
                       Color color, bool edge) {
    float cx = x + (float)(point.x / SC_CELL - map->origin.x + .5) * tile;
    float cy = y + (float)(map->origin.z + map->side - point.z / SC_CELL) * tile;
    float margin = tile * .3f, side = map->side * tile;
    if (!edge && (cx < x || cx > x + side || cy < y || cy > y + side))
        return;
    cx = (float)clampd(cx, x + margin, x + side - margin);
    cy = (float)clampd(cy, y + margin, y + side - margin);
    rect(r, cx - 3, cy - 3, 6, 6, color);
    frame(r, cx - 5, cy - 5, 10, 10, color);
}
static void deck_map(Renderer *r, Game *g, int player, float x, float y, float size, bool large) {
    Player *p = &g->players[player];
    MapView map;
    Cell center = large ? p->map_center : world_cell(position_absolute(p->pos));
    game_map_build(g, player, center, large ? 4 : 2, &map);
    float tile = size / map.side;
    float text_scale = fmaxf(1, size / (large ? 420 : 112));
    Color panel = rgb(.018f, .035f, .043f), wall = rgb(.35f, .43f, .43f), gold = rgb(1, .7f, .25f),
          mint = rgb(.3f, 1, .75f);
    rect(r, x - 6, y - 21, size + 12, size + 46, panel);
    frame(r, x - 6, y - 21, size + 12, size + 46, wall);
    char label[120];
    snprintf(label, sizeof(label), large ? "DECK %+d / SCHEMATIC" : "DECK %+d / M", map.origin.y);
    text_at(r, x, y - 15, (large ? 1.1f : .8f) * text_scale, label, mint);
    const Color colors[DISTRICT_TYPES] = {
        {.12f, .2f, .2f},   {.23f, .17f, .12f}, {.2f, .16f, .27f},
        {.27f, .22f, .13f}, {.14f, .22f, .29f}, {.17f, .25f, .22f},
        {.22f, .2f, .16f},  {.16f, .22f, .3f},  {.16f, .25f, .13f}};
    for (int z = 0; z < map.side; z++)
        for (int xx = 0; xx < map.side; xx++) {
            int at = z * map.side + xx;
            if (!map.valid[at])
                continue;
            float left = x + xx * tile, top = y + (map.side - 1 - z) * tile;
            Cell mapped = {map.origin.x + xx, map.origin.y, map.origin.z + z};
            bool locked = g->world.open_layer && campaign_layer(mapped) > g->world.open_layer;
            rect(r, left, top, tile, tile,
                 locked ? rgb(.09f, .045f, .11f) : colors[map.rooms[at].district]);
            if (g->campaign.version && campaign_guardian(map.rooms[at].cell) >= 0) {
                int id = campaign_guardian(map.rooms[at].cell);
                frame(r, left + tile * .3f, top + tile * .3f, tile * .4f, tile * .4f,
                      g->campaign.defeated & (1u << id) ? mint : gold);
            }
            if (map.rooms[at].landmark) {
                /* Pink corner brackets distinguish discoveries from gold goals. */
                Color discovery = rgb(.86f, .42f, .63f);
                float gap = tile * .22f;
                frame(r, left + gap, top + gap, tile - 2 * gap, tile - 2 * gap, discovery);
                if (large)
                    text_at(r, left + tile * .42f, top + tile * .39f, .8f * text_scale, "!",
                            discovery);
            }
            unsigned doors = map.doors[at];
            const int dx[4] = {-1, 1, 0, 0}, dz[4] = {0, 0, -1, 1};
            if (map.systems[at] & SYSTEM_BEACON)
                frame(r, left + tile * .15f, top + tile * .15f, tile * .7f, tile * .7f,
                      map.systems[at] & SYSTEM_POWER ? mint : gold);
            for (int d = 0; d < 4; d++) {
                int nx = xx + dx[d], nz = z + dz[d];
                if (nx >= 0 && nx < map.side && nz >= 0 && nz < map.side &&
                    map.valid[nz * map.side + nx] &&
                    cell_equal(map.rooms[at].cell, map.rooms[nz * map.side + nx].cell))
                    continue;
                bool open = (doors & (1 << d)) != 0;
                int pieces = open ? 2 : 1;
                float length = open ? tile * .38f : tile;
                for (int j = 0; j < pieces; j++) {
                    float shift = j * tile * .62f;
                    if (d <= EAST)
                        rect(r, left + (d == EAST ? tile - 1.5f : 0), top + shift, 1.5f, length,
                             wall);
                    else
                        rect(r, left + shift, top + (d == SOUTH ? tile - 1.5f : 0), length, 1.5f,
                             wall);
                }
            }
        }
    if (map.has_shelter && map.shelter.y == map.origin.y)
        map_marker(r, &map, position_absolute(g->shelter.door), x, y, tile, rgb(.65f, .4f, 1),
                   false);
    if (map.has_objective) {
        map_marker(r, &map, game_terminal(map.objective), x, y, tile, gold, true);
        if (map.objective.y != map.player.y && map.player.y == map.origin.y)
            map_marker(r, &map, world_lift(map.player), x, y, tile, rgb(.2f, .7f, 1), false);
        if (large)
            snprintf(label, sizeof(label), "GOAL [%d,%d] DECK %+d / %+d FROM YOU", map.objective.x,
                     map.objective.z, map.objective.y, map.objective.y - map.player.y);
        else if (map.objective.y != map.player.y)
            snprintf(label, sizeof(label), "GOAL %s %dD",
                     map.objective.y > map.player.y ? "UP" : "DOWN",
                     abs(map.objective.y - map.player.y));
        else
            snprintf(label, sizeof(label), "GOAL ON DECK");
    } else
        snprintf(label, sizeof(label), "BEACON LIT");
    text_at(r, x, y + size + 6, .8f * text_scale, label, gold);
    for (int i = 0; i < g->player_count; i++) {
        const Player *other = &g->players[i];
        Cell c = world_cell(position_absolute(other->pos));
        if (c.y != map.origin.y || c.x < map.origin.x || c.x >= map.origin.x + map.side ||
            c.z < map.origin.z || c.z >= map.origin.z + map.side)
            continue;
        Vec3 feet = position_absolute(other->pos);
        float cx = x + (float)(feet.x / SC_CELL - map.origin.x + .5) * tile;
        float cy = y + (float)(map.origin.z + map.side - feet.z / SC_CELL) * tile;
        Color col = i == player ? mint : rgb(.4f, .65f, 1);
        double yaw = other->sheltered ? other->return_yaw : other->yaw;
        Vec3 a = v3(cx + sin(yaw) * 6, cy - cos(yaw) * 6, 0);
        Vec3 b = v3(cx + sin(yaw + 2.4) * 5, cy - cos(yaw + 2.4) * 5, 0);
        Vec3 c2 = v3(cx + sin(yaw - 2.4) * 5, cy - cos(yaw - 2.4) * 5, 0);
        triangle(&r->mesh, a, b, c2, col, 0);
    }
    if (large) {
        text_at(r, x, y + size + 32, .85f * text_scale,
                "ARROWS PAN / PAGE UP-DOWN DECK / HOME RECENTER / M CLOSE", mint);
        text_at(r, x, y + size + 49, .85f * text_scale,
                "GREEN YOU / BLUE LIFT / GOLD GOAL / VIOLET SHELTER / PINK DISCOVERY / TEAL WARD",
                wall);
    }
}
static void story_journal(Renderer *r, const Game *g, int player, float w, float h) {
    const Player *p = &g->players[player];
    float s = fminf((w - 24) / 800, (h - 24) / 510), x = (w - 800 * s) * .5f,
          y = (h - 510 * s) * .5f;
    Color gold = rgb(.88f, .7f, .4f), ink = rgb(.8f, .85f, .8f), mint = rgb(.3f, .8f, .7f),
          dim = rgb(.45f, .55f, .57f);
    rect(r, x, y, 800 * s, 510 * s, rgb(.02f, .04f, .05f));
    frame(r, x, y, 800 * s, 510 * s, gold);
    x += 18 * s;
    char text[320];
    text_at(r, x, y + 16 * s, 1.4f * s, "EXPEDITION / THE SHIP REMEMBERS", gold);
    for (int line = 0; line < 3; line++) {
        game_story_line(g, line, text, sizeof(text));
        text_at(r, x, y + (40 + line * 18) * s, s, text, line == 1 ? mint : ink);
    }
    text_at(r, x, y + 105 * s, .9f * s, "CURRENT GOAL / GOLD ON YOUR MAP / U CHANGES LEAD", gold);
    text_at(r, x, y + 123 * s, s, game_mission(g), ink);
    text_at(r, x, y + 151 * s, .9f * s, "CONFIRMED FACTS", gold);
    for (int line = 3; line < 6; line++) {
        game_story_line(g, line, text, sizeof(text));
        text_at(r, x, y + (169 + (line - 3) * 17) * s, .92f * s, text, ink);
    }
    snprintf(text, sizeof(text), "FIELD RECORD / L NEXT / %s",
             game_story_title(g, p->story_reading));
    text_at(r, x, y + 231 * s, .9f * s, text, gold);
    game_story_entry(g, p->story_reading, text, sizeof(text));
    wrapped(r, x, y + 250 * s, .95f * s, 760 * s, text, ink);
    text_at(r, x, y + 294 * s, .9f * s, "UNRESOLVED LEADS / U TRACKS THE NEXT KNOWN SITE", gold);
    int row = 0;
    for (unsigned n = 0; n < SC_STORY_SCENES; n++) {
        unsigned i = (g->story.tracked + n) % SC_STORY_SCENES;
        if (game_story_available(g, i) && row < 3) {
            Cell c = g->story.sites[i];
            snprintf(text, sizeof(text), "%c %s [%d,%d,%d]", i == g->story.tracked ? '>' : ' ',
                     game_story_title(g, i), c.x, c.y, c.z);
            text_at(r, x, y + (313 + row++ * 17) * s, .9f * s, text,
                    i == g->story.tracked ? mint : dim);
        }
    }
    if (p->decision_open) {
        text_at(r, x, y + 385 * s, s, "CHOOSE HERE / 1 2 3 / GAMEPAD LEFT UP RIGHT", gold);
        bool prepared = game_story_choice_ready(g, player);
        for (unsigned i = 1; i <= 3; i++) {
            snprintf(text, sizeof(text), "%u %s%s", i, game_story_choice_for(g, player, i),
                     i == 3 && !prepared ? " / LOCKED" : "");
            text_at(r, x, y + (408 + (i - 1) * 21) * s, .9f * s, text,
                    i == 3 && !prepared ? dim : mint);
        }
        text_at(r, x, y + 472 * s, .85f * s,
                prepared ? "ALL THREE OUTCOMES AVAILABLE / ENTER LEAVES UNDECIDED"
                         : "THIRD CHOICE NEEDS THE OPTIONAL OPERATION / ENTER LEAVES UNDECIDED",
                dim);
    } else {
        game_story_line(g, 6, text, sizeof(text));
        text_at(r, x, y + 402 * s, .95f * s, text, mint);
        game_story_line(g, 7, text, sizeof(text));
        text_at(r, x, y + 426 * s, .9f * s, text, ink);
        text_at(r, x, y + 455 * s, .9f * s,
                "G AT A CLEARED SITE TO INVESTIGATE OR REVIEW ITS CHOICES", dim);
    }
    text_at(r, x, y + 491 * s, .85f * s,
            "F6 DOSSIER / F7 DEBRIEF / F4 CHARACTER / ENTER CLOSE / PAD RB PAGES", gold);
}
static void history_journal(Renderer *r, const Game *g, int player, float w, float h) {
    float s = fminf((w - 24) / 800, (h - 24) / 510), x = (w - 800 * s) * .5f,
          y = (h - 510 * s) * .5f;
    Color gold = rgb(.88f, .7f, .4f), ink = rgb(.8f, .85f, .8f);
    rect(r, x, y, 800 * s, 510 * s, rgb(.02f, .04f, .05f));
    frame(r, x, y, 800 * s, 510 * s, gold);
    text_at(r, x + 18 * s, y + 16 * s, 1.4f * s, "SHIP DOSSIER / TRACES OF THE COLLAPSE", gold);
    char text[320];
    for (int line = 0; line < (g->story.version < 2 ? 1 : 8); line++) {
        game_story_dossier(g, line, text, sizeof(text));
        wrapped(r, x + 18 * s, y + (52 + line * 40) * s, .88f * s, 760 * s, text, ink);
    }
    if (g->world.generator >= 10 && !g->players[player].sheltered &&
        !g->matrix.divers[player].active && position_absolute(g->players[player].pos).z >= 0) {
        RoomSpec room =
            world_describe_version(g->world.seed, g->world.generator,
                                   world_cell(position_absolute(g->players[player].pos)));
        char account[600];
        habitat_account(&room, account, sizeof(account));
        text_at(r, x + 18 * s, y + 384 * s, .9f * s, "LOCAL TRACE / F6", gold);
        wrapped(r, x + 18 * s, y + 404 * s, .82f * s, 760 * s, account, ink);
    } else
        text_at(r, x + 18 * s, y + 455 * s, .9f * s,
                "ROOM NOTICES AND WITNESS RECORDS REVEAL THE SHIP'S HISTORY", gold);
    text_at(r, x + 18 * s, y + 486 * s, .85f * s,
            "I EXPEDITION / F4 CHARACTER / ENTER CLOSE / PAD RB EXPEDITION", gold);
}
static void debrief_journal(Renderer *r, const Game *g, int player, float w, float h) {
    float s = fminf((w - 24) / 820, (h - 24) / 570), x = (w - 820 * s) * .5f,
          y = (h - 570 * s) * .5f;
    Color gold = rgb(.88f, .7f, .4f), ink = rgb(.8f, .85f, .8f);
    rect(r, x, y, 820 * s, 570 * s, rgb(.02f, .04f, .05f));
    frame(r, x, y, 820 * s, 570 * s, gold);
    text_at(r, x + 18 * s, y + 16 * s, 1.35f * s, "EXPEDITION DEBRIEF / YOUR CHOICES ENDURE", gold);
    char text[400];
    for (int line = 0; line < (g->story.version < 3 ? 1 : 10); line++) {
        game_story_debrief(g, player, line, text, sizeof(text));
        wrapped(r, x + 18 * s, y + (48 + line * 45) * s, .95f * s, 780 * s, text, ink);
    }
    text_at(r, x + 18 * s, y + 535 * s, .85f * s,
            "L PREVIOUS EVENT / I EXPEDITION / F4 CHARACTER / ENTER CLOSE / PAD RB PAGES", gold);
}
#include "render_matrix_hud.inc"
#include "render_campaign_hud.inc"
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
    if (g->matrix.divers[player].active) {
        matrix_hud(r, g, player, w, h, paused, touch, save_status);
        flush(r);
        return;
    }
    float pad = 24 * s;
    char label[160];
    deck_map(r, g, player, pad, pad + 16 * s, 112 * s, false);
    Cell cell = world_cell(position_absolute(p->pos));
    Room *room = world_find(&g->world, cell);
    const char *name =
        position_absolute(p->pos).z < 0
            ? (exterior_cabin(&g->world, position_absolute(p->pos)) ? "BOARDING SKIFF"
                                                                    : "THE SILENT EXPANSE")
            : (room ? (room->landmark       ? world_landmark_name(room->landmark)
                       : room->habitat.form ? sc_habitat_forms[room->habitat.form]
                                            : sc_themes[room->theme].name)
                    : "BETWEEN DECKS");
    centered(r, w * .5f, pad, 1.6f * s,
             p->sheltered         ? "THE FOLD / SANCTUARY"
             : p->riding == MECH  ? "REAVER / MECH PILOT LINK"
             : p->riding == WAGON ? "FOLD WAGON / ROAD LINK"
                                  : name,
             gold);
    snprintf(label, sizeof(label), "DEPTH %03u  /  DECK %+d",
             position_absolute(p->pos).z < 0 ? 0 : world_difficulty(cell), cell.y);
    centered(r, w * .5f, pad + 18 * s, 1.1f * s,
             p->sheltered ? "SAFE / THE INFESTATION CANNOT ENTER" : label, dim);
    if (!p->sheltered && room && position_absolute(p->pos).z >= 0) {
        snprintf(label, sizeof(label), "%s / %s", world_district_name(room->district),
                 g->world.generator >= 4 ? world_incident_name(room->incident) : "UNSURVEYED");
        if (g->world.generator >= 10)
            snprintf(label, sizeof(label), "%s / %s / F6 TRACE",
                     world_district_name(room->district),
                     sc_atmospheres[room->habitat.atmosphere].name);
        if (room->history_role)
            snprintf(label, sizeof(label), "%s / F6 DOSSIER",
                     world_history_role(room->history_role));
        centered(r, w * .5f, pad + 33 * s, .9f * s, label, dim);
    }
    if (!p->sheltered && g->story.version >= 3 && game_story_safe(g, position_absolute(p->pos)))
        centered(r, w * .5f, pad + 48 * s, .9f * s, "BEACON WARD / SAFE RECOVERY", mint);
    for (int i = 0; !p->sheltered && i < SC_MAX_DEATH_HAZARDS; i++) {
        const DeathHazard *hazard = &g->death_hazards[i];
        if (hazard->active && !hazard->spent &&
            vlen(position_delta(hazard->pos, p->pos)) < hazard->radius + 2) {
            centered(r, w * .5f, h * .67f, .95f * s,
                     hazard->kind == DEATH_ACID ? "ACID RESIDUE / KEEP CLEAR"
                     : hazard->kind == DEATH_MINE && !hazard->triggered
                         ? "DROPPED MINE / KEEP CLEAR"
                         : "UNSTABLE CORE / MOVE AWAY",
                     rgb(1, .4f, .18f));
            break;
        }
    }
    if (g->world.pending)
        centered(r, w * .5f, h * .8f, 1 * s, "SURVEYING NEARBY PASSAGES", dim);
    snprintf(label, sizeof(label), "SHIP %08X", g->world.seed);
    if (w >= 600) {
        text_at(r, w - pad - 13 * 7 * s, pad, 1.1f * s, label, dim);
        const char *profile = g->world.generator >= 4
                                  ? world_profile_name(world_profile(g->world.seed))
                                  : "LEGACY EXPEDITION";
        text_at(r, w - pad - (float)strlen(profile) * 5.4f * s, pad + 18 * s, .9f * s, profile,
                dim);
    }
    if (g->player_count == 2) {
        snprintf(label, sizeof(label), "PLAYER %d", player + 1);
        text_at(r, w - pad - 90 * s, pad + 33 * s, 1.2f * s, label, mint);
    }
    if (!paused) {
        float cx = w * .5f, cy = h * .5f;
        rect(r, cx - 8 * s, cy, 5 * s, 1, ink);
        rect(r, cx + 4 * s, cy, 5 * s, 1, ink);
        rect(r, cx, cy - 8 * s, 1, 5 * s, ink);
        rect(r, cx, cy + 4 * s, 1, 5 * s, ink);
    }
    float by = h - 94 * s;
    rect(r, pad - 8 * s, by - 10 * s, 270 * s, 86 * s, panel);
    snprintf(label, sizeof(label), "VITALITY  %03d / %03d", (int)ceilf(p->health),
             (int)player_max_health(p));
    text_at(r, pad, by, 1.6f * s, label, ink);
    rect(r, pad, by + 18 * s, 230 * s, 6 * s, rgb(.12f, .20f, .21f));
    rect(r, pad, by + 18 * s, 230 * s * p->health / player_max_health(p), 6 * s,
         p->hurt > 0 ? rgb(.9f, .25f, .19f) : mint);
    snprintf(label, sizeof(label), "RANK %02u  /  %u ELIMINATED", p->level, p->kills);
    text_at(r, pad, by + 34 * s, 1.1f * s, label, gold);
    snprintf(label, sizeof(label), "SHIELD %03.0f  ARMOR %03.0f  SALVAGE %u", p->shield, p->armor,
             p->credits);
    text_at(r, pad, by + 50 * s, .9f * s, label, dim);
    rect(r, pad, by + 65 * s, 230 * s * p->shield / player_max_shield(p), 4 * s,
         rgb(.25f, .5f, .9f));
    Cell objective;
    if (g->story.version && !(g->story.done & 1u) && !p->sheltered)
        centered(r, w * .5f, 77 * s, .85f * s,
                 story_commission_version(g->world.seed, g->story.version), mint);
    if (!p->sheltered && game_objective(g, &objective)) {
        snprintf(label, sizeof(label), "%s  /  [%d,%d,%d]", game_mission(g), objective.x,
                 objective.y, objective.z);
        centered(r, w * .5f, 96 * s, .85f * s, label, gold);
        if (!g->story.version &&
            vlen(vsub(position_absolute(p->pos), game_terminal(objective))) < 4)
            centered(r, w * .5f, h * .58f, 1.2f * s, "G / USE MISSION TERMINAL", gold);
    }
    unsigned story_site = game_story_near(g, player);
    if (story_site < SC_STORY_SCENES) {
        snprintf(label, sizeof(label), "G / %s", game_story_title(g, story_site));
        centered(r, w * .5f, h * .58f, s, label, gold);
    }
    const Enemy *aimed = NULL;
    int aimed_guardian = -1;
    double nearest = 55;
    Vec3 eye = player_eye(p), aim = facing(p->yaw, p->pitch);
    for (int j = 0; !p->sheltered && j < g->world.count; j++)
        for (int i = 0; i < g->world.rooms[j].enemy_count; i++) {
            const Enemy *e = &g->world.rooms[j].enemies[i];
            Vec3 delta = vsub(enemy_center(e), eye);
            double distance = vlen(delta);
            if (e->alive && distance < nearest && vdot(vnorm(delta), aim) > .985 &&
                world_raycast(&g->world, eye, vnorm(delta), distance) >= distance - .1) {
                aimed = e;
                aimed_guardian =
                    g->campaign.version && i == 0 ? campaign_guardian(g->world.rooms[j].cell) : -1;
                nearest = distance;
            }
        }
    if (aimed && !paused) {
        EnemyBody body = sc_enemy_bodies[aimed->kind];
        if (aimed_guardian >= 0)
            snprintf(label, sizeof(label), "%s / %.0f",
                     campaign_guardian_name(g->world.seed, (unsigned)aimed_guardian),
                     aimed->health);
        else
            snprintf(label, sizeof(label), "%s / %s %s / %.0f", sc_enemies[aimed->kind].name,
                     sc_body_names[body.form], sc_body_variants[body.form][body.variant],
                     aimed->health);
        centered(r, w * .5f, h * .43f, s, label, mint);
    }
    float exp = (float)p->xp / (float)player_next_level(p);
    rect(r, 0, h - 3, w * exp, 3, gold);
    if (!touch) {
        text_at(r, w - 260 * s, by, 1.3f * s,
                p->sheltered ? "WEAPONS HOLSTERED" : game_gun(p)->name, ink);
        snprintf(label, sizeof(label), "DMG %.0f  ENERGY %.0f  CORE %u",
                 game_gun(p)->damage * (1 + .35f * p->power), p->energy, p->points);
        text_at(r, w - 260 * s, by + 19 * s, 1.1f * s, label, gold);
        text_at(r, w - 260 * s, by + 36 * s, 1 * s,
                p->flashlight ? "F LAMP ON / F5 SAVE / ESC PAUSE"
                              : "F LAMP OFF / F5 SAVE / ESC PAUSE",
                dim);
    }
    if (p->points && !p->riding) {
        centered(r, w * .5f, h - 115 * s, 1.4f * s, "ASCENSION AVAILABLE", gold);
        centered(r, w * .5f, h - 97 * s, 1.1f * s,
                 touch ? "OPEN MENU TO CHOOSE AN UPGRADE" : "I JOURNAL / 1 THROUGH 0 UPGRADE", ink);
    }
    Vec3 lift = world_lift(cell), d = vsub(position_absolute(p->pos), lift);
    d.y = 0;
    if (g->world.generator >= 9 && !p->sheltered && !p->lift_active &&
        position_absolute(p->pos).z >= 0 && world_service_column(g->world.seed, cell) &&
        fabs(position_absolute(p->pos).x - world_center(cell).x) < 16 &&
        position_absolute(p->pos).z - world_center(cell).z < -5)
        centered(r, w * .5f, h * .59f, 1.15f * s, "SERVICE SHAFT / AMBER STAIRS / BLUE RAMPS",
                 gold);
    if (p->lift_active) {
        snprintf(label, sizeof(label), "GRAV TRANSIT / DECK %d / %d%%", p->lift_to.sector.y,
                 (int)(100 * p->lift_progress));
        centered(r, w * .5f, h * .65f, 1.7f * s, label, mint);
    } else if (!p->sheltered && position_absolute(p->pos).z >= 0 && vlen(d) < 3.3)
        centered(r, w * .5f, h * .65f, 1.7f * s,
                 touch ? "GRAV LIFT / UP OR DOWN" : "GRAV LIFT  /  E UP  /  Q DOWN", mint);
    if (p->riding) {
        const Rig *rig = &g->rigs[player][p->riding - 1];
        Color amber = rgb(.95f, .62f, .22f);
        if (p->riding == MECH) {
            frame(r, 8 * s, 8 * s, w - 16 * s, h - 16 * s, amber);
            rect(r, 8 * s, h * .3f, 4 * s, h * .32f, amber);
            rect(r, w - 12 * s, h * .3f, 4 * s, h * .32f, amber);
        }
        rect(r, w * .5f - 180 * s, h - 172 * s, 360 * s, 49 * s, panel);
        snprintf(label, sizeof(label), "%s PLATING %.0f / %.0f",
                 p->riding == MECH ? "MECH" : "WAGON", rig->armor,
                 game_rig_max_armor(p, (RigKind)p->riding));
        centered(r, w * .5f, h - 165 * s, 1.1f * s, label, amber);
        rect(r, w * .5f - 160 * s, h - 149 * s, 320 * s, 5 * s, dim);
        rect(r, w * .5f - 160 * s, h - 149 * s,
             320 * s * rig->armor / game_rig_max_armor(p, (RigKind)p->riding), 5 * s, amber);
        snprintf(label, sizeof(label),
                 touch ? "%.1f M/S / GUN / USE TO EXIT" : "%.1f M/S / Z-X WEAPONS / G DISMOUNT",
                 p->drive_speed);
        centered(r, w * .5f, h - 136 * s, .9f * s, label, ink);
    } else if (game_rig_near(g, player) != ON_FOOT)
        centered(r, w * .5f, h * .63f, 1.1f * s,
                 game_rig_near(g, player) == MECH ? "G / BOARD MECH" : "G / RIDE WAGON", mint);
    ShelterUse use = game_shelter_near(g, player);
    if (use != SHELTER_NONE) {
        const char *prompts[] = {"",
                                 p->sheltered ? "G / USE TO RETURN TO SHIP"
                                              : "G / USE TO ENTER FOLD SHELTER",
                                 "G / USE TO REST IN BED",
                                 "G / USE MED STATION / HEAL AND REPAIR",
                                 "G / USE KITCHEN / COOK STARGRAIN STEW",
                                 "C OR G / LINK SHELTER MATRIX WORKSTATION"};
        centered(r, w * .5f, h * .58f, 1.1f * s, prompts[use], mint);
    }
    if (matrix_near(g, player))
        centered(r, w * .5f, h * .53f, 1.1f * s, "C / LINK TO SHIP MATRIX", mint);
    if (p->sheltered)
        centered(r, w * .5f, pad + 38 * s, .8f * s,
                 "BED AHEAD LEFT / MED STATION AHEAD RIGHT / KITCHEN ON LEFT", gold);
    if (p->resting > 0 || p->nourished > 0) {
        snprintf(label, sizeof(label),
                 p->resting > 0 ? "RESTING %.0fS / MOVE TO WAKE"
                                : "WELL FED / RECOVERY BOOST %.0fS",
                 p->resting > 0 ? p->resting : p->nourished);
        centered(r, w * .5f, h - 137 * s, s, label, mint);
    }
    if (!touch && !p->sheltered)
        text_at(r, w - 260 * s, by + 53 * s, .9f * s, "T MECH / Y WAGON / K FLARE", dim);
    snprintf(label, sizeof(label), "V VEIL%s / B FLY %.0f / J LEAP %.0f / H NOVA %.0f",
             p->cloaked ? " ON" : "", p->flight, p->leap, p->nova);
    if (g->campaign.version) {
        char powers[4][25];
        const char *names[] = {"VEIL", "FLY", "LEAP", "NOVA"};
        float life[] = {p->cloaked ? 1 : 0, p->flight, p->leap, p->nova};
        for (unsigned i = 0; i < 4; i++)
            if (campaign_power(g, player, i))
                snprintf(powers[i], sizeof(powers[i]), "%s %.0f", names[i], life[i]);
            else
                snprintf(powers[i], sizeof(powers[i]), "%s LOCKED", names[i]);
        snprintf(label, sizeof(label), "V %s / B %s / J %s / H %s", powers[0], powers[1], powers[2],
                 powers[3]);
    }
    if (!touch && !p->riding)
        centered(r, w * .5f, h - 158 * s, .85f * s, label, p->cloaked ? rgb(.65f, .45f, 1) : mint);
    if (touch) {
        if (p->riding)
            snprintf(label, sizeof(label), "%s / ENERGY %.0f", game_gun(p)->name, p->energy);
        else if (!campaign_power(g, player, p->selected_power))
            snprintf(label, sizeof(label), "PSI LOCKED / EARN AND EQUIP IN BUILD MENU");
        else
            snprintf(label, sizeof(label), "PSI: %s / %.0f ENERGY",
                     sc_psionic_names[p->selected_power], sc_psionic_costs[p->selected_power]);
        centered(r, w * .5f, 56 * s, .8f * s, label, mint);
    }
    if (!touch && !p->sheltered && p->weapon == SC_FLARE_CANNON && p->riding != MECH) {
        snprintf(label, sizeof(label), "O BURN %us / P STRENGTH %u / EACH MAX 5",
                 30 + 15 * p->flare_lifetime, p->flare_strength);
        centered(r, w * .5f, h - 181 * s, .85f * s, label, gold);
    }
    if (g->message_time > 0 && !paused)
        centered(r, w * .5f, g->campaign.version ? fmaxf(174 * s, h * .33f) : h * .19f, 1.2f * s,
                 g->message, gold);
    if (!p->sheltered && position_absolute(p->pos).z < 0 && !paused)
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
        const char *names[] = {"FIRE", "JUMP", "CROUCH", "UP",   "DOWN",  "LAMP",
                               "USE",  "GUN",  "FOLD",   "CAST", "POWER", "MAP"};
        const float xs[] = {.90f, .77f, .64f, .90f, .77f, .64f, .90f, .77f, .64f, .51f, .51f, .51f},
                    ys[] = {.72f, .85f, .85f, .48f, .48f, .48f, .27f, .27f, .27f, .27f, .48f, .1f};
        for (int i = 0; i < 12; i++) {
            float x = w * xs[i], y = h * ys[i];
            rect(r, x - size * .5f, y - size * .5f, size, size, panel);
            frame(r, x - size * .5f, y - size * .5f, size, size, dim);
            centered(r, x, y - 4 * s, s, names[i], mint);
        }
    }
    if (debug) {
        snprintf(label, sizeof(label), "%.0f FPS / %d CHUNKS / %d QUEUED / %u BUILT / %u UPLOADS",
                 fps, g->world.chunk_count, g->world.pending, g->world.last_work, r->last_uploads);
        text_at(r, pad, 203 * s, .8f * s, label, mint);
        RegionPlan plan;
        world_plan_version(g->world.seed, g->world.generator, world_region(cell), &plan);
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
        centered(r, w * .5f, my + 173 * s, 1.1f * s,
                 "E / Q LIFTS   G USE   I JOURNAL   Z / X WEAPONS", dim);
        centered(r, w * .5f, my + 192 * s, 1.1f * s, "F1 SETTINGS / PAD BACK   F2 CO-OP   F5 SAVE",
                 dim);
        centered(r, w * .5f, my + 223 * s, 1.1f * s, "N + ENTER / NEW SHIP AND NEW CHARACTERS",
                 gold);
        if (g->message_time > 0)
            centered(r, w * .5f, my + 242 * s, .9f * s, g->message, mint);
    }
    if (!paused)
        campaign_hud(r, g, player, w, h, s);
    if (p->journal && p->journal_page == 5)
        campaign_journal_draw(r, g, player, w, h);
    else if (p->journal && p->journal_page == 4)
        matrix_journal_draw(r, g, player, w, h);
    else if (p->journal && p->journal_page == 3)
        debrief_journal(r, g, player, w, h);
    else if (p->journal && p->journal_page == 2)
        history_journal(r, g, player, w, h);
    else if (p->journal && g->story.version && p->journal_page)
        story_journal(r, g, player, w, h);
    else if (p->journal) {
        float pw = fminf(w - 24, 740 * s), ph = fminf(h - 32, 450 * s), px = (w - pw) * .5f,
              py = (h - ph) * .5f;
        rect(r, px, py, pw, ph, panel);
        frame(r, px, py, pw, ph, gold);
        text_at(r, px + 18 * s, py + 16 * s, 1.4f * s, "RECLAIMER / CHARACTER AND MISSION JOURNAL",
                gold);
        snprintf(label, sizeof(label), "RANK %u  POINTS %u  SALVAGE %u  BEACONS %u", p->level,
                 p->points, p->credits, p->victories);
        text_at(r, px + 18 * s, py + 40 * s, s, label, ink);
        for (int i = 0; i < SC_ATTRIBUTES; i++) {
            unsigned level = *player_attribute(p, i);
            snprintf(label, sizeof(label), "%d %s %u / COST %u", (i + 1) % 10, sc_attributes[i],
                     level, 60 + level * 25);
            text_at(r, px + 18 * s + (i / 5) * pw * .5f, py + (64 + (i % 5) * 20) * s, s, label,
                    mint);
        }
        text_at(r, px + 18 * s, py + 160 * s, .8f * s,
                "UPGRADE WITH 1 POINT OR SALVAGE / F4 EXPEDITION PAGE", dim);
        text_at(r, px + 18 * s, py + 175 * s, s, game_mission(g), gold);
        if (game_objective(g, &objective)) {
            Cell here = world_cell(position_absolute(p->pos));
            snprintf(label, sizeof(label), "TARGET [%d,%d,%d]  DELTA [%d,%d,%d] / G AT TERMINAL",
                     objective.x, objective.y, objective.z, objective.x - here.x,
                     objective.y - here.y, objective.z - here.z);
            text_at(r, px + 18 * s, py + 195 * s, .85f * s, label, ink);
        }
        snprintf(label, sizeof(label),
                 "CAREER %u ARCS / %u RESCUED / %u KNOWLEDGE / %u TECH / %u CONTAINED",
                 p->career[4], p->career[0], p->career[1], p->career[2], p->career[3]);
        text_at(r, px + 18 * s, py + 345 * s, .75f * s, label, mint);
        snprintf(label, sizeof(label), "CONTRACTS / PURGE %u/12  SALVAGE %u/8  SYSTEMS %u/3",
                 g->mission_kills, g->mission_loot, g->mission);
        text_at(r, px + 18 * s, py + 218 * s, .85f * s, label, mint);
        text_at(r, px + 18 * s, py + 240 * s, s,
                "Z / X SWITCH WEAPONS / EACH USES RECHARGING ENERGY", dim);
        snprintf(label, sizeof(label), "EQUIPPED: %s / ARMORY:", sc_weapons[p->weapon].name);
        text_at(r, px + 18 * s, py + 262 * s, s, label, gold);
        int row = 0;
        for (int i = 0; i < SC_WEAPONS; i++)
            if (p->weapons & (1u << i)) {
                if (row < 16)
                    text_at(r, px + 18 * s + (row / 4) * pw * .25f, py + (282 + (row % 4) * 17) * s,
                            .8f * s, sc_weapons[i].name, ink);
                row++;
            }
        unsigned logs = 0;
        for (int i = 0; i < 12; i++)
            logs += (p->logs >> i) & 1;
        snprintf(label, sizeof(label), "%d WEAPONS / %u RECORDS / L NEXT LOG / ENTER CLOSE", row,
                 logs);
        text_at(r, px + 18 * s, py + 365 * s, .9f * s, label, dim);
        int reading = p->reading;
        if (!(p->logs & (1u << reading)))
            for (int i = 0; i < 12; i++)
                if (p->logs & (1u << i)) {
                    reading = i;
                    break;
                }
        if (p->logs)
            text_at(r, px + 18 * s, py + 390 * s, .72f * s, sc_logs[reading], gold);
    }
    if (p->cloaked && !paused)
        frame(r, 3, 3, w - 6, h - 6, rgb(.35f, .2f, .6f));
    if (p->map_open) {
        float size = fminf(w - 90 * s, h - (touch ? 185 : 150) * s);
        rect(r, 12 * s, 12 * s, w - 24 * s, h - 24 * s, panel);
        centered(r, w * .5f, 25 * s, 1.2f * s, "SHIP CARTOGRAPH / NORTH UP", gold);
        deck_map(r, g, player, (w - size) * .5f, (touch ? 85 : 65) * s, size, true);
        if (touch) {
            float cx = w * .51f, cy = h * .1f;
            rect(r, cx - 30, cy - 10, 60, 20, panel);
            frame(r, cx - 30, cy - 10, 60, 20, mint);
            centered(r, cx, cy - 4, .9f, "CLOSE", mint);
        }
    }
    flush(r);
}
void renderer_options(Renderer *r, int width, int height, int player, int selected) {
    if (width < 80 || height < 80)
        return;
    float s = fminf((width - 20) / 650.0f, (height - 20) / 420.0f);
    float x = (width - 650 * s) * .5f, y = (height - 420 * s) * .5f, matrix[16];
    Color gold = rgb(.9f, .72f, .4f), ink = rgb(.76f, .84f, .8f), dim = rgb(.4f, .55f, .56f);
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl.Viewport(0, 0, width, height);
    ortho(matrix, (float)width, (float)height);
    gl.UniformMatrix4fv(r->mvp, 1, GL_FALSE, matrix);
    gl.Uniform1i(r->ui, 1);
    gl.Uniform3f(r->offset, 0, 0, 0);
    gl.Disable(GL_DEPTH_TEST);
    rect(r, x, y, 650 * s, 420 * s, rgb(.015f, .03f, .04f));
    frame(r, x, y, 650 * s, 420 * s, gold);
    text_at(r, x + 22 * s, y + 20 * s, 1.5f * s, "CONTROLS AND DISPLAY", gold);
    const char *labels[] = {
        "PLAYER",         "MOUSE SENSITIVITY",    "TOUCH SENSITIVITY", "STICK SENSITIVITY",
        "STICK DEADZONE", "INVERT VERTICAL LOOK", "WORLD RESOLUTION",  "RESET THIS PLAYER"};
    for (int row = 0; row < 8; row++) {
        float yy = y + (68 + row * 36) * s;
        if (row == selected)
            rect(r, x + 12 * s, yy - 8 * s, 626 * s, 30 * s, rgb(.06f, .12f, .13f));
        text_at(r, x + 24 * s, yy, s, labels[row], row == selected ? gold : ink);
        char value[64] = "LEFT / RIGHT TO RESET";
        if (row == 0)
            snprintf(value, sizeof(value), "%d", player + 1);
        else if (row < 5)
            snprintf(value, sizeof(value), "%.2f",
                     preferences_get(&r->preferences, player, row - 1));
        else if (row == 5)
            snprintf(value, sizeof(value), "%s",
                     r->preferences.view[player].inverted ? "ON" : "OFF");
        else if (row == 6)
            snprintf(value, sizeof(value), "%s",
                     preferences_quality(r->preferences.view[player].quality));
        text_at(r, x + 380 * s, yy, s, value, row == selected ? gold : ink);
    }
    text_at(r, x + 22 * s, y + 372 * s, .85f * s,
            "ARROWS / DPAD SELECT AND ADJUST / ENTER OR START RESUMES", dim);
    text_at(r, x + 22 * s, y + 394 * s, .85f * s,
            "F1 OPTIONS / DEVICE SETTINGS ARE SEPARATE FROM SHIP SAVES", dim);
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
    r->sprite_drawn[0] = r->sprite_drawn[1] = 0;
    for (int p = 0; p < count; p++) {
        int vw = count == 1 ? width : (vertical ? width / 2 : width),
            vh = count == 1 ? height : (vertical ? height : height / 2);
        int vx = count == 1 ? 0 : (vertical ? p * vw : 0),
            vy = count == 1 ? 0 : (vertical ? 0 : (1 - p) * vh);
        if (vw < 1 || vh < 1)
            continue;
        int rh = preferences_height(&r->preferences, p, touch), rw = (int)((double)rh * vw / vh);
        r->view_height[p] = rh;
        if (rw < 160)
            rw = 160;
        if (rw > 960)
            rw = 960;
        resize_target(r, p, rw, rh);
        gl.BindFramebuffer(GL_FRAMEBUFFER, r->framebuffer[p]);
        gl.Viewport(0, 0, rw, rh);
        Player *player = &g->players[p];
        bool matrix = g->matrix.divers[p].active;
        bool outside = !matrix && !player->sheltered && position_absolute(player->pos).z < 0;
        gl.ClearColor(outside ? .001f : .012f, outside ? .002f : .024f,
                      outside ? .004f : .039f, 1);
        gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl.Enable(GL_DEPTH_TEST);
        gl.Uniform1i(r->ui, 0);
        r->origin = matrix || player->sheltered
                        ? v3(0, 0, 0)
                        : world_center(world_cell(position_absolute(player->pos)));
        Vec3 eye = matrix ? matrix_eye(g, p) : vsub(player_eye(player), r->origin);
        float m[16];
        camera(m, eye, player->yaw, player->pitch, (double)vw / vh,
               outside ? fmax(6000, fabs(player_eye(player).z) + 6500) : 420);
        gl.UniformMatrix4fv(r->mvp, 1, GL_FALSE, m);
        gl.Uniform3f(r->eye, (float)eye.x, (float)eye.y, (float)eye.z);
        gl.Uniform3f(r->grid, (float)fmod(r->origin.x, 4096), (float)fmod(r->origin.y, 4096),
                     (float)fmod(r->origin.z, 4096));
        AtmosphereDef atmosphere = sc_atmospheres[STILL_AIR];
        if (g->world.generator >= 10 && !outside && !matrix && !player->sheltered) {
            RoomSpec room = world_describe_version(g->world.seed, g->world.generator,
                                                   world_cell(position_absolute(player->pos)));
            atmosphere = sc_atmospheres[room.habitat.atmosphere];
        }
        gl.Uniform1f(r->fog_density, outside ? 0
                                     : g->world.generator < 10 || matrix || player->sheltered
                                         ? .009f
                                         : atmosphere.density);
        gl.Uniform3f(r->fog_color, atmosphere.fog.r, atmosphere.fog.g, atmosphere.fog.b);
        lights(r, g, player);
        if (matrix)
            matrix_scene(r, g, p);
        else if (player->sheltered)
            fold_interior(r, g, p);
        else {
            starfield(r, player);
            draw_chunks(r, g);
            hull_view(r, g, player_eye(player));
            boarding_skiff(r, g, player_eye(player));
            actors(r, g, p);
            campaign_scene(r, g, p);
        }
        flush(r);
        r->sprite_drawn[p] = 0;
        if (!outside && !player->sheltered && !matrix && g->world.generator >= 10)
            atmosphere_draw(r, g, p, m, eye, atmosphere);
        if (!paused && !player->sheltered && !matrix) {
            gl.Clear(GL_DEPTH_BUFFER_BIT);
            weapon(r, player);
            flush(r);
        }
        gl.BindFramebuffer(GL_READ_FRAMEBUFFER, r->framebuffer[p]);
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
