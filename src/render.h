#ifndef SC_RENDER_H
#define SC_RENDER_H
#include "game.h"
#include "gl_api.h"
#define SC_VIEW_LIGHTS 12
#define SC_SPACE_STARS 1800
typedef struct {
    Vec3 direction;
    Color color;
    double radius;
} SpaceStar;
typedef struct {
    float x, y, z, nx, ny, nz, r, g, b, e;
} Vertex;
typedef struct {
    Vertex *vertices;
    size_t count, cap;
    bool failed;
} Mesh;
typedef struct {
    GLuint program, vao, vbo, framebuffer, texture, depth;
    GLint mvp, eye, grid, ui, offset;
    GLint torch, time, lamp_count, lamp_pos, lamp_color, fog_density;
    int width, height;
    uint32_t world_revision;
    size_t world_vertices;
    GLuint chunk_vbo[SC_MAX_CHUNKS];
    uint32_t chunk_serial[SC_MAX_CHUNKS];
    size_t chunk_vertices[SC_MAX_CHUNKS];
    unsigned last_uploads;
    Vec3 origin;
    SpaceStar stars[SC_SPACE_STARS];
    Mesh mesh;
    bool failed;
} Renderer;
bool renderer_init(Renderer *r);
void renderer_destroy(Renderer *r);
void renderer_draw(Renderer *r, Game *g, int width, int height, bool paused, bool touch, bool debug,
                   double fps, const char *save_status);
bool renderer_capture(int width, int height, const char *path);
#endif
