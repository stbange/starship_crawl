#include "save_internal.h"
static void bytes(Stream *s, void *data, size_t n, bool read) {
    if (!s->ok)
        return;
    size_t done = read ? fread(data, 1, n, s->file) : fwrite(data, 1, n, s->file);
    if (done != n) {
        s->ok = false;
        return;
    }
    const unsigned char *b = data;
    for (size_t i = 0; i < n; i++) {
        s->hash ^= b[i];
        s->hash *= UINT32_C(16777619);
    }
}
uint32_t sc_save_number(Stream *s, uint32_t n, bool read) {
    unsigned char b[4];
    for (int i = 0; i < 4; i++)
        b[i] = (unsigned char)(n >> (i * 8));
    bytes(s, b, 4, read);
    n = 0;
    for (int i = 0; i < 4; i++)
        n |= (uint32_t)b[i] << (i * 8);
    return n;
}
double sc_save_real(Stream *s, double d, bool read) {
    uint64_t bits = 0;
    memcpy(&bits, &d, 8);
    uint32_t lo = sc_save_number(s, (uint32_t)bits, read),
             hi = sc_save_number(s, (uint32_t)(bits >> 32), read);
    bits = (uint64_t)lo | ((uint64_t)hi << 32);
    memcpy(&d, &bits, 8);
    return d;
}
Vec3 sc_save_vector(Stream *s, Vec3 v, bool read) {
    v.x = sc_save_real(s, v.x, read);
    v.y = sc_save_real(s, v.y, read);
    v.z = sc_save_real(s, v.z, read);
    return v;
}
bool sc_save_valid_vector(Vec3 v) {
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z) && fabs(v.x) < 9e8 && fabs(v.y) < 3e8 &&
           fabs(v.z) < 9e8;
}
Position sc_save_location(Stream *s, Position p, bool read, bool legacy) {
    if (legacy) {
        Vec3 v = sc_save_vector(s, position_absolute(p), read);
        if (!sc_save_valid_vector(v)) {
            s->ok = false;
            return (Position){0};
        }
        return position_from(v);
    }
    p.sector.x = (int32_t)sc_save_number(s, (uint32_t)p.sector.x, read);
    p.sector.y = (int32_t)sc_save_number(s, (uint32_t)p.sector.y, read);
    p.sector.z = (int32_t)sc_save_number(s, (uint32_t)p.sector.z, read);
    p.local = sc_save_vector(s, p.local, read);
    if (!sc_save_valid_vector(p.local) || p.local.x < -48 || p.local.x >= 48 || p.local.y < 0 ||
        p.local.y >= SC_DECK || p.local.z < -48 || p.local.z >= 48 ||
        !sc_save_valid_vector(position_absolute(p)))
        s->ok = false;
    return p;
}
Cell sc_save_matrix_cell(Stream *s, Cell c, bool read) {
    c.x = (int32_t)sc_save_number(s, (uint32_t)c.x, read);
    c.y = (int32_t)sc_save_number(s, (uint32_t)c.y, read);
    c.z = (int32_t)sc_save_number(s, (uint32_t)c.z, read);
    return c;
}
bool sc_save_matrix_bool(Stream *s, bool value, bool read) {
    uint32_t n = sc_save_number(s, value, read);
    if (n > 1)
        s->ok = false;
    return n != 0;
}
