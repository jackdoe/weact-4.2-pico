#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "epd.h"
#include "gfx.h"

static void draw_arrow(int cx, int cy, int len) {
    gfx_line(cx, cy - len, cx, cy + len, true);
    gfx_line(cx, cy - len, cx - 14, cy - len + 14, true);
    gfx_line(cx, cy - len, cx + 14, cy - len + 14, true);
    gfx_line(cx - 14, cy - len + 14, cx + 14, cy - len + 14, true);
}

static void draw_demo(int r) {
    int w = epd_width();
    int h = epd_height();

    epd_clear(false);

    gfx_rect(0, 0, w, h, true);
    gfx_rect(3, 3, w - 6, h - 6, true);

    char title[24];
    snprintf(title, sizeof(title), "ROTATION  %d", r * 90);
    int tx = (w - (int)strlen(title) * GFX_FONT_W) / 2;
    gfx_text(tx, 14, title, false);
    gfx_line(tx - 6, 34, tx + (int)strlen(title) * GFX_FONT_W + 6, 34, true);

    gfx_text(14,           14,                       "TL", false);
    gfx_text(w - 14 - 16,  14,                       "TR", false);
    gfx_text(14,           h - 14 - GFX_FONT_H,      "BL", false);
    gfx_text(w - 14 - 16,  h - 14 - GFX_FONT_H,      "BR", false);

    int cx = w / 2;
    int cy = h / 2;
    draw_arrow(cx, cy - 6, 50);
    gfx_text(cx - GFX_FONT_W, cy + 56, "UP", false);

    char dim[24];
    snprintf(dim, sizeof(dim), "%d x %d", w, h);
    int dx = (w - (int)strlen(dim) * GFX_FONT_W) / 2;
    gfx_text(dx, h - 34, dim, false);
}

int main(void) {
    epd_init();

    while (1) {
        for (int r = 0; r < 4; r++) {
            epd_set_rotation(r);
            draw_demo(r);
            epd_refresh_full();
            sleep_ms(3000);
        }
    }
}
