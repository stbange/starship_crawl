#ifndef SC_RENDER_H
#define SC_RENDER_H
#include "game.h"
#include "gl_api.h"
#include "platform_input.h"
#define SC_VIEW_LIGHTS 12
#define SC_SPACE_STARS 1800
#define SC_VIEW_SPRITES 1536
#define SC_TENTACLE_ROOTS 32
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
    Vec3 center, right, up;
    Color color;
    float alpha, glow;
    double distance;
    unsigned tile, order;
} SpriteDraw;
typedef struct {
    float x, y, z, u, v, r, g, b, a, glow;
} SpriteVertex;
typedef struct {
    GLuint program, vao, vbo;
    GLuint framebuffer[SC_PLAYERS], texture[SC_PLAYERS], depth[SC_PLAYERS];
    GLint mvp, eye, grid, ui, offset;
    GLint torch, time, lamp_count, lamp_pos, lamp_color, fog_density, fog_color;
    GLuint sprite_program, sprite_vao, sprite_vbo, sprite_atlas;
    SpriteDraw sprite_draws[SC_VIEW_SPRITES];
    SpriteVertex sprite_vertices[SC_VIEW_SPRITES * 6];
    unsigned sprite_drawn[SC_PLAYERS];
    unsigned tentacle_drawn[SC_PLAYERS];
    int width[SC_PLAYERS], height[SC_PLAYERS];
    unsigned target_resizes[SC_PLAYERS];
    uint32_t world_revision;
    size_t world_vertices;
    GLuint chunk_vbo[SC_MAX_CHUNKS];
    uint32_t chunk_serial[SC_MAX_CHUNKS];
    size_t chunk_vertices[SC_MAX_CHUNKS];
    unsigned last_uploads;
    Vec3 origin;
    SpaceStar stars[SC_SPACE_STARS];
    Mesh mesh;
    Preferences preferences;
    int view_height[SC_PLAYERS];
    bool failed;
} Renderer;
bool renderer_init(Renderer *r);
void renderer_destroy(Renderer *r);
void renderer_draw(Renderer *r, Game *g, int width, int height, bool paused, bool touch, bool debug,
                   double fps, const char *save_status);
bool renderer_capture(int width, int height, const char *path);
void renderer_options(Renderer *r, int width, int height, int player, int selected);
#endif
