#pragma once

#include <stdint.h>
#include <stdbool.h>

#define EPD_W      400
#define EPD_H      300
#define EPD_PITCH  (EPD_W / 8)
#define EPD_BYTES  (EPD_PITCH * EPD_H)

void epd_init(void);
void epd_sleep(void);

uint8_t *epd_fb(void);

void epd_clear(bool black);

void epd_refresh_full(void);
void epd_refresh_partial(void);

void epd_refresh_full_async(void);
void epd_refresh_partial_async(void);
bool epd_busy(void);
void epd_wait(void);

void epd_set_rotation(int rot);
int  epd_rotation(void);
int  epd_width(void);
int  epd_height(void);
