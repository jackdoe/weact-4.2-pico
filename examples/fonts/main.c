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

static void page_display(void) {
    epd_clear(false);
    gfx_rect(0, 0, EPD_W, EPD_H, true);
    gfx_rect(3, 3, EPD_W - 6, EPD_H - 6, true);

    gfx_text_f(12, 10, "DISPLAY SIZES", false, &gfx_font_8x16);
    gfx_line(8, 32, EPD_W - 8, 32, true);

    gfx_text_f(12, 44,  "12:34:56", false, &gfx_font_dep_31x48);

    gfx_text_f(12, 108, "dep 20x32", false, &gfx_font_8x16);
    gfx_text_f(12, 126, "HEADLINE 42",  false, &gfx_font_dep_20x32);

    gfx_text_f(12, 174, "3270 18x27", false, &gfx_font_8x16);
    gfx_text_f(12, 192, "BIG IRON 3270", false, &gfx_font_3270_18x27);

    gfx_text_f(12, 236, "0123456789 #$%", false, &gfx_font_3270_18x27);
    epd_refresh_full();
}

static void page_dots(void) {
    epd_clear(false);
    gfx_rect(0, 0, EPD_W, EPD_H, true);
    gfx_rect(3, 3, EPD_W - 6, EPD_H - 6, true);

    gfx_text_f(12, 10, "CLAUDE DOTS  5x8 grid, drawn from memory", false, &gfx_font_8x16);
    gfx_line(8, 32, EPD_W - 8, 32, true);

    gfx_text_f(12, 42,  "DEPARTURES", false, &gfx_font_dots_30x41);

    gfx_text_f(12, 96,  "GATE B7  ON TIME", false, &gfx_font_dots_18x25);
    gfx_text_f(12, 124, "the quick brown fox!", false, &gfx_font_dots_18x25);
    gfx_text_f(12, 152, "0123456789 @#$%&*", false, &gfx_font_dots_18x25);

    gfx_text_f(12, 192, "23:59:42", false, &gfx_font_dots_30x41);
    gfx_text_f(12, 244, "{[(<~|~>)]} +-=/", false, &gfx_font_dots_18x25);
    epd_refresh_full();
}

int main(void) {
    epd_init();
    while (1) {
        page_departure();
        sleep_ms(8000);
        page_3270();
        sleep_ms(8000);
        page_display();
        sleep_ms(8000);
        page_dots();
        sleep_ms(8000);
    }
}
