#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int            w;
    int            h;
    int            first;
    int            last;
    const uint8_t *bitmap;
} gfx_font_t;

extern const gfx_font_t gfx_font_8x16;
extern const gfx_font_t gfx_font_dep_10x21;
extern const gfx_font_t gfx_font_dep_13x26;
extern const gfx_font_t gfx_font_dep_15x31;
extern const gfx_font_t gfx_font_dep_20x32;
extern const gfx_font_t gfx_font_dep_31x48;
extern const gfx_font_t gfx_font_3270_10x21;
extern const gfx_font_t gfx_font_3270_12x23;
extern const gfx_font_t gfx_font_3270_14x27;
extern const gfx_font_t gfx_font_3270_18x27;
extern const gfx_font_t gfx_font_dots_18x25;
extern const gfx_font_t gfx_font_dots_30x41;

#define GFX_FONT_W 8
#define GFX_FONT_H 16

void gfx_pixel(int x, int y, bool black);
void gfx_hline(int x, int y, int w, bool black);
void gfx_vline(int x, int y, int h, bool black);
void gfx_line(int x0, int y0, int x1, int y1, bool black);
void gfx_rect(int x, int y, int w, int h, bool black);
void gfx_fill_rect(int x, int y, int w, int h, bool black);
void gfx_circle(int cx, int cy, int r, bool black);
void gfx_fill_circle(int cx, int cy, int r, bool black);

void gfx_char  (int x, int y, char c,         bool invert);
void gfx_text  (int x, int y, const char *s,  bool invert);
void gfx_char_f(int x, int y, char c,         bool invert, const gfx_font_t *font);
void gfx_text_f(int x, int y, const char *s,  bool invert, const gfx_font_t *font);
