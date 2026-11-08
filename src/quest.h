#ifndef SC_QUEST_H
#define SC_QUEST_H
#include "game.h"

#define SC_QUEST_POSES 5
enum { QUEST_HEAD, QUEST_LEFT, QUEST_RIGHT, QUEST_AIM_RIGHT, QUEST_AIM_LEFT };
enum {
    QUEST_FIRE = 1u,
    QUEST_USE = 2u,
    QUEST_A = 4u,
    QUEST_B = 8u,
    QUEST_X = 16u,
    QUEST_Y = 32u,
    QUEST_LAMP = 64u,
    QUEST_WRIST = 128u,
    QUEST_SPRINT = 256u,
    QUEST_CENTER = 512u
};
typedef struct {
    float poses[SC_QUEST_POSES][16];
    Vec3 reference;
    double yaw;
    unsigned valid;
    bool calibrated, turn_latched;
} QuestTracking;

void quest_tracking_init(QuestTracking *q);
bool quest_pose_valid(const float *m);
bool quest_tracking_update(QuestTracking *q, const float *poses, unsigned valid);
bool quest_calibrate(QuestTracking *q, double world_yaw);
Vec3 quest_vector(const QuestTracking *q, Vec3 tracking);
Vec3 quest_position(const QuestTracking *q, int pose, Vec3 base);
Vec3 quest_axis(const QuestTracking *q, int pose, int axis);
Vec3 quest_forward(const QuestTracking *q, int pose);
Vec3 quest_head_delta(const QuestTracking *q);
bool quest_lean_safe(const QuestTracking *q);
int quest_snap(QuestTracking *q, double axis);
bool quest_view(const QuestTracking *q, const float *tracking_view, Vec3 base, Vec3 render_origin,
                float *view);
bool quest_projection_valid(const float *projection);

/* A presentation packet, never saved with a character or expedition. */
typedef struct {
    QuestTracking tracking;
    Vec3 base, head, menu_center, menu_right, menu_up;
    bool active, menu, wrist, unsafe, vignette, recovery, controllers;
    float fade, movement;
    char title[64], lines[14][128], items[12][80], footer[128];
    int line_count, item_count, selected;
} QuestView;
#endif
