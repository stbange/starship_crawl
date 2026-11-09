#include "render_internal.h"

/* Original 5 x 7 bitmap glyphs. UI stays crisp independently of scene resolution. */
const unsigned char sc_render_glyphs[][7] = {
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

int sc_render_glyph(char c) {
    c = (char)toupper((unsigned char)c);
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= '0' && c <= '9')
        return 26 + c - '0';
    const char *extra = "-/+:.[]!><|";
    const char *p = strchr(extra, c);
    return p ? 36 + (int)(p - extra) : -1;
}
void sc_render_rect(Renderer *r, float x, float y, float w, float h, Color c) {
    sc_render_quad(&r->mesh, v3(x, y, 0), v3(x + w, y, 0), v3(x + w, y + h, 0), v3(x, y + h, 0), c,
                   0);
}
void sc_render_text_at(Renderer *r, float x, float y, float scale, const char *text, Color color) {
    for (; *text; text++, x += scale * 6) {
        int g = sc_render_glyph(*text);
        if (g < 0)
            continue;
        for (int row = 0; row < 7; row++)
            for (int col = 0; col < 5; col++)
                if (sc_render_glyphs[g][row] & (1 << (4 - col)))
                    sc_render_rect(r, x + col * scale, y + row * scale, scale, scale, color);
    }
}
void sc_render_centered(Renderer *r, float x, float y, float s, const char *text, Color c) {
    sc_render_text_at(r, x - (float)strlen(text) * s * 3, y, s, text, c);
}
float sc_render_wrapped(Renderer *r, float x, float y, float scale, float width, const char *text,
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
        sc_render_text_at(r, x, y, scale, line, c);
        text += n;
        while (*text == ' ')
            text++;
        y += 13 * scale;
    }
    return y;
}
void sc_render_frame(Renderer *r, float x, float y, float w, float h, Color c) {
    sc_render_rect(r, x, y, w, 1, c);
    sc_render_rect(r, x, y + h - 1, w, 1, c);
    sc_render_rect(r, x, y, 1, h, c);
    sc_render_rect(r, x + w - 1, y, 1, h, c);
}
