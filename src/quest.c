#include "quest.h"
#include <string.h>

void quest_tracking_init(QuestTracking *q) {
    memset(q, 0, sizeof(*q));
}
bool quest_pose_valid(const float *m) {
    if (!m)
        return false;
    for (int i = 0; i < 16; i++)
        if (!isfinite(m[i]) || fabs(m[i]) > 10000)
            return false;
    if (fabs(m[3]) > .001 || fabs(m[7]) > .001 || fabs(m[11]) > .001 || fabs(m[15] - 1) > .001)
        return false;
    Vec3 axes[3];
    for (int i = 0; i < 3; i++) {
        axes[i] = v3(m[i * 4], m[i * 4 + 1], m[i * 4 + 2]);
        if (fabs(vdot(axes[i], axes[i]) - 1) > .02)
            return false;
    }
    return fabs(vdot(axes[0], axes[1])) < .02 && fabs(vdot(axes[0], axes[2])) < .02 &&
           fabs(vdot(axes[1], axes[2])) < .02 && vdot(vcross(axes[0], axes[1]), axes[2]) > .98;
}
bool quest_tracking_update(QuestTracking *q, const float *poses, unsigned valid) {
    if (!poses || valid != (1u << SC_QUEST_POSES) - 1) {
        q->valid = 0;
        q->turn_latched = true;
        return false;
    }
    for (int i = 0; i < SC_QUEST_POSES; i++)
        if (!quest_pose_valid(poses + i * 16)) {
            q->valid = 0;
            q->turn_latched = true;
            return false;
        }
    memcpy(q->poses, poses, sizeof(q->poses));
    q->valid = valid;
    return true;
}
static Vec3 pose_position(const float *m) {
    return v3(m[12], m[13], m[14]);
}
bool quest_calibrate(QuestTracking *q, double world_yaw) {
    if (!q->valid || !isfinite(world_yaw))
        return false;
    const float *head = q->poses[QUEST_HEAD];
    /* The existing game camera uses +Z forward and +X screen-right. Reflect
       tracking Z before applying its yaw; a 180-degree rotation mirrors eyes. */
    Vec3 forward = v3(-head[8], -head[9], head[10]);
    if (hypot(forward.x, forward.z) < .1)
        return false;
    q->yaw = remainder(world_yaw - atan2(forward.x, forward.z), 2 * SC_PI);
    q->reference = pose_position(head);
    q->calibrated = true;
    q->turn_latched = true;
    return true;
}
Vec3 quest_vector(const QuestTracking *q, Vec3 v) {
    double c = cos(q->yaw), s = sin(q->yaw);
    return v3(c * v.x - s * v.z, v.y, -s * v.x - c * v.z);
}
Vec3 quest_position(const QuestTracking *q, int pose, Vec3 base) {
    if (pose < 0 || pose >= SC_QUEST_POSES || !q->calibrated)
        return base;
    return vadd(base, quest_vector(q, vsub(pose_position(q->poses[pose]), q->reference)));
}
Vec3 quest_axis(const QuestTracking *q, int pose, int axis) {
    if (pose < 0 || pose >= SC_QUEST_POSES || axis < 0 || axis > 2)
        return v3(0, 0, 0);
    const float *m = q->poses[pose] + axis * 4;
    return quest_vector(q, v3(m[0], m[1], m[2]));
}
Vec3 quest_forward(const QuestTracking *q, int pose) {
    return vmul(quest_axis(q, pose, 2), -1);
}
Vec3 quest_head_delta(const QuestTracking *q) {
    return vsub(pose_position(q->poses[QUEST_HEAD]), q->reference);
}
bool quest_lean_safe(const QuestTracking *q) {
    if (!q->valid || !q->calibrated)
        return false;
    Vec3 d = quest_head_delta(q);
    return hypot(d.x, d.z) <= 1.5 && fabs(d.y) <= 1.8;
}
int quest_snap(QuestTracking *q, double axis) {
    if (!isfinite(axis) || !q->valid || !q->calibrated) {
        q->turn_latched = true;
        return 0;
    }
    if (fabs(axis) < .25)
        q->turn_latched = false;
    if (fabs(axis) < .65 || q->turn_latched)
        return 0;
    int direction = axis > 0 ? 1 : -1;
    q->yaw = remainder(q->yaw + direction * SC_PI / 4, 2 * SC_PI);
    q->turn_latched = true;
    return direction;
}
bool quest_view(const QuestTracking *q, const float *tracking_view, Vec3 base, Vec3 render_origin,
                float *view) {
    if (!q->valid || !q->calibrated || !view || !quest_pose_valid(tracking_view))
        return false;
    Vec3 delta = vsub(render_origin, base);
    if (!isfinite(delta.x) || !isfinite(delta.y) || !isfinite(delta.z))
        return false;
    /* This orthogonal transform is its own inverse: F * inverse(yaw). */
    double c = cos(q->yaw), s = sin(q->yaw);
    float map[16] = {(float)c, 0, (float)-s, 0, 0, 1, 0, 0, (float)-s, 0, (float)-c, 0, 0, 0, 0, 1};
    Vec3 at = vadd(q->reference, quest_vector(q, delta));
    map[12] = (float)at.x;
    map[13] = (float)at.y;
    map[14] = (float)at.z;
    float result[16] = {0};
    for (int column = 0; column < 4; column++)
        for (int row = 0; row < 4; row++)
            for (int k = 0; k < 4; k++)
                result[column * 4 + row] += tracking_view[k * 4 + row] * map[column * 4 + k];
    memcpy(view, result, sizeof(result));
    return true;
}
bool quest_projection_valid(const float *m) {
    if (!m)
        return false;
    for (int i = 0; i < 16; i++)
        if (!isfinite(m[i]) || fabs(m[i]) > 100000)
            return false;
    return m[0] > 0 && m[5] > 0 && m[10] < 0 && fabs(m[11] + 1) < .001 && fabs(m[15]) < .001;
}
