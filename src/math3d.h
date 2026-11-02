#ifndef SC_MATH3D_H
#define SC_MATH3D_H
#include <math.h>
#include <stdint.h>
#define SC_PI 3.14159265358979323846
typedef struct {
    double x, y, z;
} Vec3;
typedef struct {
    float r, g, b;
} Color;
static inline Vec3 v3(double x, double y, double z) {
    Vec3 v = {x, y, z};
    return v;
}
static inline Vec3 vadd(Vec3 a, Vec3 b) {
    return v3(a.x + b.x, a.y + b.y, a.z + b.z);
}
static inline Vec3 vsub(Vec3 a, Vec3 b) {
    return v3(a.x - b.x, a.y - b.y, a.z - b.z);
}
static inline Vec3 vmul(Vec3 a, double s) {
    return v3(a.x * s, a.y * s, a.z * s);
}
static inline double vdot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
static inline double vlen(Vec3 a) {
    return sqrt(vdot(a, a));
}
static inline Vec3 vnorm(Vec3 a) {
    double n = vlen(a);
    return n > 1e-9 ? vmul(a, 1 / n) : v3(0, 0, 0);
}
static inline Vec3 vcross(Vec3 a, Vec3 b) {
    return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
static inline double clampd(double x, double a, double b) {
    return x < a ? a : (x > b ? b : x);
}
static inline Color rgb(float r, float g, float b) {
    Color c = {r, g, b};
    return c;
}
static inline Vec3 facing(double yaw, double pitch) {
    return v3(sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch));
}
#endif
