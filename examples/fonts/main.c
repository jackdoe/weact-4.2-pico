#include "pico/stdlib.h"

#include "epd.h"
#include "gfx.h"

static void page_departure(void) {
    epd_clear(false);
    gfx_rect(0, 0, EPD_W, EPD_H, true);
    gfx_rect(3, 3, EPD_W - 6, EPD_H - 6, true);

    gfx_text_f(12, 10, "DEPARTURE MONO", false, &gfx_font_8x16);
    gfx_line(8, 32, EPD_W - 8, 32, true);

    gfx_text_f(12, 40,  "10x21", false, &gfx_font_8x16);
    gfx_text_f(12, 56,  "the quick brown fox",       false, &gfx_font_dep_10x21);

    gfx_text_f(12, 92,  "13x26", false, &gfx_font_8x16);
    gfx_text_f(12, 108, "the quick brown",           false, &gfx_font_dep_13x26);

    gfx_text_f(12, 150, "15x31", false, &gfx_font_8x16);
    gfx_text_f(12, 166, "lo fi tech",                false, &gfx_font_dep_15x31);

    gfx_text_f(12, 218, "0123456789 !@#$%^&*()",     false, &gfx_font_dep_13x26);
    epd_refresh_full();
}

static void page_3270(void) {
    epd_clear(false);
    gfx_rect(0, 0, EPD_W, EPD_H, true);
    gfx_rect(3, 3, EPD_W - 6, EPD_H - 6, true);

    gfx_text_f(12, 10, "3270 NERD MONO", false, &gfx_font_8x16);
    gfx_line(8, 32, EPD_W - 8, 32, true);

    gfx_text_f(12, 40,  "10x21", false, &gfx_font_8x16);
    gfx_text_f(12, 56,  "the quick brown fox",            false, &gfx_font_3270_10x21);

    gfx_text_f(12, 84,  "12x23", false, &gfx_font_8x16);
    gfx_text_f(12, 100, "mainframe vibes",                false, &gfx_font_3270_12x23);

    gfx_text_f(12, 138, "14x27", false, &gfx_font_8x16);
    gfx_text_f(12, 154, "terminal era",                   false, &gfx_font_3270_14x27);

    gfx_text_f(12, 200, "0123456789",                     false, &gfx_font_3270_14x27);
    gfx_text_f(12, 232, "!@#$%^&*()",                     false, &gfx_font_3270_14x27);
    epd_refresh_full();
}

int main(void) {
    epd_init();
    while (1) {
        page_departure();
        sleep_ms(8000);
        page_3270();
        sleep_ms(8000);
    }
}
