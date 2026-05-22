#include <string.h>

#include "gfx.h"
#include "epd.h"

static inline int abs_i(int v) { return v < 0 ? -v : v; }

void gfx_pixel(int x, int y, bool black) {
    if ((unsigned)x >= EPD_W || (unsigned)y >= EPD_H) return;
    uint8_t *p = epd_fb() + y * EPD_PITCH + (x >> 3);
    uint8_t mask = 0x80 >> (x & 7);
    if (black) *p &= (uint8_t)~mask;
    else       *p |= mask;
}

void gfx_hline(int x, int y, int w, bool black) {
    if ((unsigned)y >= EPD_H || w <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > EPD_W) w = EPD_W - x;
    if (w <= 0) return;

    uint8_t *row = epd_fb() + y * EPD_PITCH;
    int x1 = x + w - 1;
    int b0 = x >> 3;
    int b1 = x1 >> 3;
    uint8_t lm = (uint8_t)(0xFF >> (x & 7));
    uint8_t rm = (uint8_t)(0xFF << (7 - (x1 & 7)));

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

void gfx_vline(int x, int y, int h, bool black) {
    if ((unsigned)x >= EPD_W || h <= 0) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > EPD_H) h = EPD_H - y;
    if (h <= 0) return;

    uint8_t mask = 0x80 >> (x & 7);
    uint8_t *p = epd_fb() + y * EPD_PITCH + (x >> 3);
    if (black) for (int i = 0; i < h; i++, p += EPD_PITCH) *p &= (uint8_t)~mask;
    else       for (int i = 0; i < h; i++, p += EPD_PITCH) *p |= mask;
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

void gfx_fill_rect(int x, int y, int w, int h, bool black) {
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > EPD_W) w = EPD_W - x;
    if (y + h > EPD_H) h = EPD_H - y;
    if (w <= 0 || h <= 0) return;
    for (int i = 0; i < h; i++) gfx_hline(x, y + i, w, black);
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

void gfx_char(int x, int y, char c, bool invert) {
    if (x >= EPD_W || y >= EPD_H || x + GFX_FONT_W <= 0 || y + GFX_FONT_H <= 0) return;
    uint8_t ch = (uint8_t)c;
    if (ch < 0x20 || ch > 0x7F) ch = '?';
    const uint8_t *glyph = &font8x16[(ch - 0x20) * 16];

    if ((x & 7) == 0 && x >= 0 && x + GFX_FONT_W <= EPD_W) {
        int r0 = y < 0 ? -y : 0;
        int r1 = (y + GFX_FONT_H > EPD_H) ? (EPD_H - y) : GFX_FONT_H;
        uint8_t *p = epd_fb() + (y + r0) * EPD_PITCH + (x >> 3);
        for (int row = r0; row < r1; row++) {
            uint8_t bits = glyph[row];
            *p = invert ? bits : (uint8_t)~bits;
            p += EPD_PITCH;
        }
        return;
    }

    for (int row = 0; row < GFX_FONT_H; row++) {
        int yy = y + row;
        if ((unsigned)yy >= EPD_H) continue;
        uint8_t bits = glyph[row];
        for (int col = 0; col < GFX_FONT_W; col++) {
            int xx = x + col;
            if ((unsigned)xx >= EPD_W) continue;
            bool on = (bits & (0x80 >> col)) != 0;
            bool black = on ^ invert;
            uint8_t *p = epd_fb() + yy * EPD_PITCH + (xx >> 3);
            uint8_t mask = 0x80 >> (xx & 7);
            if (black) *p &= (uint8_t)~mask;
            else       *p |= mask;
        }
    }
}

void gfx_text(int x, int y, const char *s, bool invert) {
    int cx = x, cy = y;
    while (*s) {
        if (*s == '\n') { cx = x; cy += GFX_FONT_H; s++; continue; }
        gfx_char(cx, cy, *s, invert);
        cx += GFX_FONT_W;
        s++;
    }
}
