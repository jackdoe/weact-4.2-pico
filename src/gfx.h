#pragma once

#include <stdint.h>
#include <stdbool.h>

#define GFX_FONT_W 8
#define GFX_FONT_H 16

extern const uint8_t font8x16[96 * 16];

void gfx_pixel(int x, int y, bool black);
void gfx_hline(int x, int y, int w, bool black);
void gfx_vline(int x, int y, int h, bool black);
void gfx_line(int x0, int y0, int x1, int y1, bool black);
void gfx_rect(int x, int y, int w, int h, bool black);
void gfx_fill_rect(int x, int y, int w, int h, bool black);
void gfx_circle(int cx, int cy, int r, bool black);
void gfx_fill_circle(int cx, int cy, int r, bool black);

void gfx_char(int x, int y, char c, bool invert);
void gfx_text(int x, int y, const char *s, bool invert);
