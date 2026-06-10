#include <string.h>

#include "gfx.h"
#include "epd.h"

static inline int abs_i(int v) { return v < 0 ? -v : v; }

static inline void rot_to_phys(int lx, int ly, int *px, int *py) {
    switch (epd_rotation()) {
        case 0: *px = lx;             *py = ly;             break;
        case 1: *px = EPD_W - 1 - ly; *py = lx;             break;
        case 2: *px = EPD_W - 1 - lx; *py = EPD_H - 1 - ly; break;
        default: *px = ly;            *py = EPD_H - 1 - lx; break;
    }
}

static inline void phys_pixel(int px, int py, bool black) {
    if ((unsigned)px >= EPD_W || (unsigned)py >= EPD_H) return;
    uint8_t *p = epd_fb() + py * EPD_PITCH + (px >> 3);
    uint8_t mask = 0x80 >> (px & 7);
    if (black) *p &= (uint8_t)~mask;
    else       *p |= mask;
    epd_mark_dirty(px, py, px, py);
}

static void phys_hline(int x, int y, int w, bool black) {
    if ((unsigned)y >= EPD_H || w <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > EPD_W) w = EPD_W - x;
    if (w <= 0) return;

    uint8_t *row = epd_fb() + y * EPD_PITCH;
    int x1 = x + w - 1;
    int b0 = x >> 3, b1 = x1 >> 3;
    uint8_t lm = (uint8_t)(0xFF >> (x & 7));
    uint8_t rm = (uint8_t)(0xFF << (7 - (x1 & 7)));

    epd_mark_dirty(x, y, x1, y);

    if (b0 == b1) {
        uint8_t m = lm & rm;
        if (black) row[b0] &= (uint8_t)~m;
        else       row[b0] |= m;
        return;
    }
    if (black) {
        row[b0] &= (uint8_t)~lm;
        if (b1 - b0 > 1) memset(row + b0 + 1, 0x00, b1 - b0 - 1);
        row[b1] &= (uint8_t)~rm;
    } else {
        row[b0] |= lm;
        if (b1 - b0 > 1) memset(row + b0 + 1, 0xFF, b1 - b0 - 1);
        row[b1] |= rm;
    }
}

static void phys_fill_rect(int x, int y, int w, int h, bool black) {
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > EPD_W) w = EPD_W - x;
    if (y + h > EPD_H) h = EPD_H - y;
    if (w <= 0 || h <= 0) return;
    for (int i = 0; i < h; i++) phys_hline(x, y + i, w, black);
}

void gfx_pixel(int x, int y, bool black) {
    int px, py;
    rot_to_phys(x, y, &px, &py);
    phys_pixel(px, py, black);
}

void gfx_fill_rect(int x, int y, int w, int h, bool black) {
    if (w <= 0 || h <= 0) return;
    int ax, ay, bx, by;
    rot_to_phys(x, y, &ax, &ay);
    rot_to_phys(x + w - 1, y + h - 1, &bx, &by);
    int px = ax < bx ? ax : bx;
    int py = ay < by ? ay : by;
    int pw = abs_i(bx - ax) + 1;
    int ph = abs_i(by - ay) + 1;
    phys_fill_rect(px, py, pw, ph, black);
}

void gfx_hline(int x, int y, int w, bool black) {
    gfx_fill_rect(x, y, w, 1, black);
}

void gfx_vline(int x, int y, int h, bool black) {
    gfx_fill_rect(x, y, 1, h, black);
}

void gfx_line(int x0, int y0, int x1, int y1, bool black) {
    if (y0 == y1) { gfx_hline(x0 < x1 ? x0 : x1, y0, abs_i(x1 - x0) + 1, black); return; }
    if (x0 == x1) { gfx_vline(x0, y0 < y1 ? y0 : y1, abs_i(y1 - y0) + 1, black); return; }
    int dx = abs_i(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs_i(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        gfx_pixel(x0, y0, black);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_rect(int x, int y, int w, int h, bool black) {
    gfx_hline(x, y, w, black);
    gfx_hline(x, y + h - 1, w, black);
    gfx_vline(x, y, h, black);
    gfx_vline(x + w - 1, y, h, black);
}

void gfx_circle(int cx, int cy, int r, bool black) {
    int x = 0, y = r, d = 3 - 2 * r;
    while (y >= x) {
        gfx_pixel(cx + x, cy + y, black);
        gfx_pixel(cx - x, cy + y, black);
        gfx_pixel(cx + x, cy - y, black);
        gfx_pixel(cx - x, cy - y, black);
        gfx_pixel(cx + y, cy + x, black);
        gfx_pixel(cx - y, cy + x, black);
        gfx_pixel(cx + y, cy - x, black);
        gfx_pixel(cx - y, cy - x, black);
        x++;
        if (d > 0) { y--; d += 4 * (x - y) + 10; }
        else d += 4 * x + 6;
    }
}

void gfx_fill_circle(int cx, int cy, int r, bool black) {
    int x = 0, y = r, d = 3 - 2 * r;
    while (y >= x) {
        gfx_hline(cx - x, cy + y, 2 * x + 1, black);
        gfx_hline(cx - x, cy - y, 2 * x + 1, black);
        gfx_hline(cx - y, cy + x, 2 * y + 1, black);
        gfx_hline(cx - y, cy - x, 2 * y + 1, black);
        x++;
        if (d > 0) { y--; d += 4 * (x - y) + 10; }
        else d += 4 * x + 6;
    }
}

void gfx_char_f(int x, int y, char c, bool invert, const gfx_font_t *font) {
    int w = epd_width(), h = epd_height();
    if (x >= w || y >= h || x + font->w <= 0 || y + font->h <= 0) return;
    uint8_t ch = (uint8_t)c;
    if (ch < font->first || ch > font->last) ch = '?';
    int stride = (font->w + 7) / 8;
    const uint8_t *glyph = &font->bitmap[(ch - font->first) * font->h * stride];

    for (int row = 0; row < font->h; row++) {
        const uint8_t *rowp = &glyph[row * stride];
        for (int col = 0; col < font->w; col++) {
            bool on = (rowp[col >> 3] >> (7 - (col & 7))) & 1;
            gfx_pixel(x + col, y + row, on ^ invert);
        }
    }
}

void gfx_text_f(int x, int y, const char *s, bool invert, const gfx_font_t *font) {
    int cx = x, cy = y;
    while (*s) {
        if (*s == '\n') { cx = x; cy += font->h; s++; continue; }
        gfx_char_f(cx, cy, *s, invert, font);
        cx += font->w;
        s++;
    }
}

void gfx_char(int x, int y, char c, bool invert) {
    gfx_char_f(x, y, c, invert, &gfx_font_8x16);
}

void gfx_text(int x, int y, const char *s, bool invert) {
    gfx_text_f(x, y, s, invert, &gfx_font_8x16);
}
