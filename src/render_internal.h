#ifndef SC_RENDER_INTERNAL_H
#define SC_RENDER_INTERNAL_H
#define SC_GPU_UPLOAD_BUDGET 2u
/* Private rendering modules; gameplay cannot depend on these helpers. */
#include "render.h"
#include "exterior.h"
#include "habitat.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
extern const unsigned char sc_render_glyphs[][7];
typedef struct {
    Vec3 at, right, up, forward;
    double size;
} BodyPose;
void sc_render_actors(Renderer *r, Game *g, int viewer);
bool sc_render_alien_form(Renderer *r, const Enemy *e, const BodyPose *pose, double time,
                          Color shell, Color flesh, float glow);
void sc_render_atmosphere_draw(Renderer *r, const Game *g, int player, const float *matrix,
                               Vec3 eye, AtmosphereDef atmosphere);
bool sc_render_atmosphere_init(Renderer *r);
void sc_render_blob(Renderer *r, Vec3 center, Vec3 x, Vec3 up, Vec3 z, Color color, float glow);
void sc_render_block(Renderer *r, Vec3 c, Vec3 size, Color col, float glow);
void sc_render_boarding_skiff(Renderer *r, const Game *g, Vec3 eye);
void sc_render_bone(Renderer *r, const BodyPose *p, Vec3 a, Vec3 b, double thickness, Color color,
                    float glow);
void sc_render_camera(float *matrix, Vec3 eye, double yaw, double pitch, double aspect,
                      double clip_far);
void sc_render_campaign_hud(Renderer *r, const Game *g, int player, float w, float h, float s);
void sc_render_campaign_journal_draw(Renderer *r, const Game *g, int player, float w, float h);
void sc_render_campaign_scene(Renderer *r, const Game *g, int player);
void sc_render_district_scene(Renderer *r, const Game *g, int player);
void sc_render_district_hud(Renderer *r, const Game *g, int player, float w, float h, float s);
void sc_render_centered(Renderer *r, float x, float y, float s, const char *text, Color c);
void sc_render_cube(Mesh *m, Vec3 center, Vec3 sx, Vec3 sy, Vec3 sz, Color col, float glow);
void sc_render_deck_map(Renderer *r, Game *g, int player, float x, float y, float size, bool large);
void sc_render_draw_chunks(Renderer *r, Game *g);
void sc_render_encounter_hud(Renderer *r, const Game *g, int player, float w, float h, float s,
                             bool touch);
void sc_render_encounter_scene(Renderer *r, const Game *g, int player);
void sc_render_flush(Renderer *r);
Vec3 sc_render_focal_anchor(const Room *room);
Color sc_render_focal_color(const Room *room);
void sc_render_fold_interior(Renderer *r, const Game *g, int viewer);
void sc_render_frame(Renderer *r, float x, float y, float w, float h, Color c);
int sc_render_glyph(char c);
void sc_render_hud(Renderer *r, Game *g, int player, int width, int height, bool paused, bool touch,
                   bool debug, double fps, const char *save_status);
void sc_render_hull_view(Renderer *r, const Game *g, Vec3 eye);
void sc_render_infestation_tentacles(Renderer *r, Game *g, int viewer);
void sc_render_layout(GLuint buffer);
void sc_render_lights(Renderer *r, const Game *g, const Player *viewer);
Color sc_render_matrix_color(unsigned theme);
void sc_render_matrix_hud(Renderer *r, const Game *g, int player, float w, float h, bool paused,
                          bool touch, const char *save_status);
void sc_render_matrix_journal_draw(Renderer *r, const Game *g, int player, float width,
                                   float height);
void sc_render_matrix_scene(Renderer *r, const Game *g, int player);
void sc_render_matrix_schematic(Renderer *r, const Game *g, int player, float x, float y,
                                float size);
void sc_render_matrix_terminal_mesh(Renderer *r, Vec3 at, double time);
void sc_render_multiply(float *out, const float *a, const float *b);
void sc_render_navigation_hud(Renderer *r, const Game *g, int player, float w, float h, float s,
                              bool touch);
void sc_render_navigation_scene(Renderer *r, const Game *g, int player);
void sc_render_octahedron(Renderer *r, Vec3 center, double radius, Color col, float glow);
void sc_render_onboarding_hud(Renderer *r, const Game *g, int player, float w, float h, float s);
void sc_render_onboarding_journal(Renderer *r, const Game *g, int player, float w, float h);
void sc_render_organ(Renderer *r, const BodyPose *p, Vec3 at, Vec3 dimensions, Color color,
                     float glow);
void sc_render_ortho(float *m, float w, float h);
void sc_render_panel(Renderer *r, const BodyPose *p, Vec3 at, Vec3 size, Color color, float glow);
void sc_render_politics_hud(Renderer *r, const Game *g, int player, float w, float h, float scale);
void sc_render_politics_journal_draw(Renderer *r, const Game *g, int player, float w, float h);
void sc_render_politics_scene(Renderer *r, const Game *g, int player);
void sc_render_quad(Mesh *m, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Color color, float glow);
void sc_render_rect(Renderer *r, float x, float y, float w, float h, Color c);
GLuint sc_render_shader(GLenum type, const char *source);
void sc_render_ship_sign(Renderer *r, Vec3 at, const char *label, Color color, Vec3 eye);
void sc_render_starfield(Renderer *r, const Player *player);
void sc_render_tentacle_mesh(Renderer *r, Vec3 root, Vec3 axis, Vec3 bend, double length,
                             double radius, double phase, double time, Color skin, Color suckers,
                             Color tip, float glow, int segments, bool detail);
void sc_render_text_at(Renderer *r, float x, float y, float scale, const char *text, Color color);
void sc_render_triangle(Mesh *m, Vec3 a, Vec3 b, Vec3 c, Color color, float glow);
void sc_render_tube(Renderer *r, Vec3 a, Vec3 b, double ra, double rb, Color color, float glow);
void sc_render_upload_world(Renderer *r, Game *g);
bool sc_render_visible_deck(const World *w, int viewer, int target);
void sc_render_weapon(Renderer *r, const Player *p);
float sc_render_wrapped(Renderer *r, float x, float y, float scale, float width, const char *text,
                        Color c);
#endif
