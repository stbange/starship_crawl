#ifndef SC_RENDER_H
#define SC_RENDER_H
#include "game.h"
#include "gl_api.h"
typedef struct {
    float x, y, z, nx, ny, nz, r, g, b, e;
} Vertex;
typedef struct {
    Vertex *vertices;
    size_t count, cap;
    bool failed;
} Mesh;
typedef struct {
    GLuint program, vao, vbo, world_vbo, framebuffer, texture, depth;
    GLint mvp, eye, grid, ui;
    int width, height;
    uint32_t world_revision;
    size_t world_vertices;
    Vec3 origin;
    Mesh mesh;
    bool failed;
} Renderer;
bool renderer_init(Renderer *r);
void renderer_destroy(Renderer *r);
void renderer_draw(Renderer *r, Game *g, int width, int height, bool paused, bool touch, bool debug,
                   double fps, const char *save_status);
bool renderer_capture(int width, int height, const char *path);
#endif
