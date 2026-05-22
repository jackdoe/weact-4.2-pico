#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "epd.h"
#include "gfx.h"

#define BAR_X 40
#define BAR_Y 200
#define BAR_W 320
#define BAR_H 24

static void splash(void) {
    epd_clear(false);

    gfx_rect(0, 0, EPD_W, EPD_H, true);
    gfx_rect(4, 4, EPD_W - 8, EPD_H - 8, true);

    gfx_text(16, 20,  "WeAct 4.2\" E-Paper", false);
    gfx_text(16, 44,  "SSD1683  400 x 300", false);
    gfx_text(16, 68,  "RP2040   SPI + DMA", false);

    gfx_fill_circle(EPD_W / 2, EPD_H / 2 + 10, 36, true);
    gfx_circle     (EPD_W / 2, EPD_H / 2 + 10, 56, true);
    gfx_circle     (EPD_W / 2, EPD_H / 2 + 10, 76, true);

    gfx_line(20, EPD_H - 36, EPD_W - 20, EPD_H - 36, true);
    gfx_text(16, EPD_H - 26, "full refresh", false);

    uint64_t t0 = time_us_64();
    epd_refresh_full();
    uint64_t dt = time_us_64() - t0;

    char buf[32];
    snprintf(buf, sizeof(buf), "%llu ms", dt / 1000);
    gfx_fill_rect(EPD_W - 120, EPD_H - 26, 100, GFX_FONT_H, false);
    gfx_text     (EPD_W - 120, EPD_H - 26, buf, false);
}

static void partial_demo(int steps) {
    epd_clear(false);

    gfx_text(16, 16,  "PARTIAL UPDATES", false);
    gfx_line(16, 36, EPD_W - 16, 36, true);
    gfx_text(16, 48,  "tick:", false);
    gfx_text(16, 80,  "rate:", false);
    gfx_text(16, 112, "elapsed:", false);

    gfx_rect(BAR_X - 2, BAR_Y - 2, BAR_W + 4, BAR_H + 4, true);

    epd_refresh_full();

    uint64_t t0 = time_us_64();
    for (int i = 1; i <= steps; i++) {
        char buf[40];
        uint64_t now = time_us_64();
        uint64_t dt_us = now - t0;
        int rate_x10 = (int)((uint64_t)i * 10000000ULL / (dt_us ? dt_us : 1));

        snprintf(buf, sizeof(buf), "%4d / %4d ", i, steps);
        gfx_fill_rect(96, 48, 200, GFX_FONT_H, false);
        gfx_text     (96, 48, buf, false);

        snprintf(buf, sizeof(buf), "%d.%d Hz ", rate_x10 / 10, rate_x10 % 10);
        gfx_fill_rect(96, 80, 200, GFX_FONT_H, false);
        gfx_text     (96, 80, buf, false);

        snprintf(buf, sizeof(buf), "%llu ms ", dt_us / 1000);
        gfx_fill_rect(112, 112, 200, GFX_FONT_H, false);
        gfx_text     (112, 112, buf, false);

        int fill = BAR_W * i / steps;
        gfx_fill_rect(BAR_X, BAR_Y, fill,         BAR_H, true);
        gfx_fill_rect(BAR_X + fill, BAR_Y, BAR_W - fill, BAR_H, false);

        epd_refresh_partial();
    }
}

static void closer(void) {
    epd_clear(false);
    gfx_text(16, 16,  "DEEP SLEEP", false);
    gfx_line(16, 36, EPD_W - 16, 36, true);
    gfx_text(16, 56,  "panel power -> 3 uA", false);
    gfx_text(16, 80,  "wakes on next refresh", false);
    epd_refresh_full();
    epd_sleep();
}

int main(void) {
    epd_init();

    while (true) {
        splash();
        sleep_ms(3000);
        partial_demo(60);
        sleep_ms(2000);
        closer();
        sleep_ms(10000);
    }
}
