#include "render_internal.h"

void sc_render_layout(GLuint buffer) {
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
void sc_render_triangle(Mesh *m, Vec3 a, Vec3 b, Vec3 c, Color color, float glow) {
    Vec3 n = vnorm(vcross(vsub(b, a), vsub(c, a)));
    vertex(m, a, n, color, glow);
    vertex(m, b, n, color, glow);
    vertex(m, c, n, color, glow);
}
void sc_render_quad(Mesh *m, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Color color, float glow) {
    sc_render_triangle(m, a, b, c, color, glow);
    sc_render_triangle(m, a, c, d, color, glow);
}
void sc_render_cube(Mesh *m, Vec3 center, Vec3 sx, Vec3 sy, Vec3 sz, Color col, float glow) {
    Vec3 p[8];
    for (int i = 0; i < 8; i++)
        p[i] = vadd(center, vadd(vmul(sx, i & 1 ? .5 : -.5),
                                 vadd(vmul(sy, i & 2 ? .5 : -.5), vmul(sz, i & 4 ? .5 : -.5))));
    static const int faces[6][4] = {{0, 4, 6, 2}, {1, 3, 7, 5}, {0, 1, 5, 4},
                                    {2, 6, 7, 3}, {0, 2, 3, 1}, {4, 5, 7, 6}};
    for (int i = 0; i < 6; i++)
        sc_render_quad(m, p[faces[i][0]], p[faces[i][1]], p[faces[i][2]], p[faces[i][3]], col,
                       glow);
}
void sc_render_block(Renderer *r, Vec3 c, Vec3 size, Color col, float glow) {
    sc_render_cube(&r->mesh, vsub(c, r->origin), v3(size.x, 0, 0), v3(0, size.y, 0),
                   v3(0, 0, size.z), col, glow);
}
void sc_render_octahedron(Renderer *r, Vec3 center, double radius, Color col, float glow) {
    center = vsub(center, r->origin);
    Vec3 top = vadd(center, v3(0, radius, 0)), bottom = vsub(center, v3(0, radius, 0));
    Vec3 edge[4];
    for (int i = 0; i < 4; i++)
        edge[i] = vadd(center, v3(cos(i * SC_PI * .5) * radius, 0, sin(i * SC_PI * .5) * radius));
    for (int i = 0; i < 4; i++) {
        sc_render_triangle(&r->mesh, top, edge[(i + 1) % 4], edge[i], col, glow);
        sc_render_triangle(&r->mesh, bottom, edge[i], edge[(i + 1) % 4], col, glow);
    }
}
/* Low-sided tapered tubes make roots, cables and articulated limbs readable
   at the game's pixel resolution without importing a model framework. */
void sc_render_tube(Renderer *r, Vec3 a, Vec3 b, double ra, double rb, Color color, float glow) {
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
        sc_render_quad(&r->mesh, p[i], p[(i + 1) % 4], p[4 + (i + 1) % 4], p[4 + i], color, glow);
}
void sc_render_blob(Renderer *r, Vec3 center, Vec3 x, Vec3 up, Vec3 z, Color color, float glow) {
    Vec3 top = vsub(vadd(center, up), r->origin), bottom = vsub(vsub(center, up), r->origin),
         edge[4];
    for (int i = 0; i < 4; i++)
        edge[i] =
            vsub(vadd(center, vadd(vmul(x, cos(i * SC_PI * .5)), vmul(z, sin(i * SC_PI * .5)))),
                 r->origin);
    for (int i = 0; i < 4; i++) {
        sc_render_triangle(&r->mesh, top, edge[(i + 1) % 4], edge[i], color, glow);
        sc_render_triangle(&r->mesh, bottom, edge[i], edge[(i + 1) % 4], color, glow);
    }
}
void sc_render_multiply(float *out, const float *a, const float *b) {
    float result[16] = {0};
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            for (int k = 0; k < 4; k++)
                result[c * 4 + r] += a[k * 4 + r] * b[c * 4 + k];
    memcpy(out, result, sizeof(result));
}
void sc_render_camera(float *matrix, Vec3 eye, double yaw, double pitch, double aspect,
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
    sc_render_multiply(matrix, projection, view);
}
void sc_render_ortho(float *m, float w, float h) {
    memset(m, 0, 16 * sizeof(float));
    m[0] = 2 / w;
    m[5] = -2 / h;
    m[10] = -1;
    m[12] = -1;
    m[13] = 1;
    m[15] = 1;
}
void sc_render_flush(Renderer *r) {
    sc_render_layout(r->vbo);
    gl.BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(r->mesh.count * sizeof(Vertex)), r->mesh.vertices,
                  GL_STREAM_DRAW);
    gl.DrawArrays(GL_TRIANGLES, 0, (GLsizei)r->mesh.count);
    r->mesh.count = 0;
}
