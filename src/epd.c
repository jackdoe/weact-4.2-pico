#include "epd.h"
#include "epd_config.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include <string.h>

#define CMD_DRIVER_OUT       0x01
#define CMD_SOFT_START       0x0C
#define CMD_DEEP_SLEEP       0x10
#define CMD_DATA_ENTRY       0x11
#define CMD_SW_RESET         0x12
#define CMD_TEMP_SENSOR      0x18
#define CMD_MASTER_ACTIVATE  0x20
#define CMD_DISP_UPDATE_2    0x22
#define CMD_WRITE_BW         0x24
#define CMD_WRITE_RED        0x26
#define CMD_ACVCOM           0x2B
#define CMD_DUMMY_LINE       0x3A
#define CMD_GATE_WIDTH       0x3B
#define CMD_BORDER           0x3C
#define CMD_RAM_X_WINDOW     0x44
#define CMD_RAM_Y_WINDOW     0x45
#define CMD_RAM_X_COUNTER    0x4E
#define CMD_RAM_Y_COUNTER    0x4F
#define CMD_ANALOG_BLOCK     0x74
#define CMD_DIGITAL_BLOCK    0x7E

#define UPD_FULL             0xF7
#define UPD_PARTIAL          0xFF

static uint8_t fb[EPD_BYTES];
static bool asleep = false;
static int rot = 0;
static int dirty_x0 = EPD_W;
static int dirty_y0 = EPD_H;
static int dirty_x1 = -1;
static int dirty_y1 = -1;

static inline void cs_lo(void)   { gpio_put(EPD_PIN_CS, 0); }
static inline void cs_hi(void)   { gpio_put(EPD_PIN_CS, 1); }
static inline void dc_cmd(void)  { gpio_put(EPD_PIN_DC, 0); }
static inline void dc_data(void) { gpio_put(EPD_PIN_DC, 1); }

static void wait_busy(void) {
    while (gpio_get(EPD_PIN_BUSY)) sleep_ms(1);
}

static void send_cmd(uint8_t c) {
    dc_cmd();
    cs_lo();
    spi_write_blocking(EPD_SPI, &c, 1);
    cs_hi();
}

static void send_data(const uint8_t *d, size_t n) {
    if (!n) return;
    dc_data();
    cs_lo();
    spi_write_blocking(EPD_SPI, d, n);
    cs_hi();
}

static void send_data1(uint8_t v) {
    send_data(&v, 1);
}

static void hw_reset(void) {
    gpio_put(EPD_PIN_RST, 1); sleep_ms(10);
    gpio_put(EPD_PIN_RST, 0); sleep_ms(10);
    gpio_put(EPD_PIN_RST, 1); sleep_ms(10);
    wait_busy();
}

static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    send_cmd(CMD_RAM_X_WINDOW);
    uint8_t xx[2] = { (uint8_t)(x0 / 8), (uint8_t)(x1 / 8) };
    send_data(xx, 2);
    send_cmd(CMD_RAM_Y_WINDOW);
    uint8_t yy[4] = { (uint8_t)(y0 & 0xFF), (uint8_t)((y0 >> 8) & 0xFF),
                      (uint8_t)(y1 & 0xFF), (uint8_t)((y1 >> 8) & 0xFF) };
    send_data(yy, 4);
}

static void set_cursor(uint16_t x, uint16_t y) {
    send_cmd(CMD_RAM_X_COUNTER);
    send_data1((uint8_t)(x / 8));
    send_cmd(CMD_RAM_Y_COUNTER);
    uint8_t yy[2] = { (uint8_t)(y & 0xFF), (uint8_t)((y >> 8) & 0xFF) };
    send_data(yy, 2);
}

static void panel_init(void) {
    hw_reset();

    send_cmd(CMD_SW_RESET);
    wait_busy();

    send_cmd(CMD_ANALOG_BLOCK);  send_data1(0x54);
    send_cmd(CMD_DIGITAL_BLOCK); send_data1(0x3B);

    send_cmd(CMD_SOFT_START);    send_data((const uint8_t[]){0x8E, 0x8C, 0x85, 0x3F}, 4);
    send_cmd(CMD_ACVCOM);        send_data((const uint8_t[]){0x04, 0x63}, 2);

    send_cmd(CMD_DRIVER_OUT);    send_data((const uint8_t[]){0x2B, 0x01, 0x00}, 3);

    send_cmd(CMD_DUMMY_LINE);    send_data1(0x2C);
    send_cmd(CMD_GATE_WIDTH);    send_data1(0x0A);

    send_cmd(CMD_TEMP_SENSOR);   send_data1(0x80);

    send_cmd(CMD_BORDER);        send_data1(0x01);

    send_cmd(CMD_DATA_ENTRY);    send_data1(0x03);

    set_window(0, 0, EPD_W - 1, EPD_H - 1);
    set_cursor(0, 0);

    wait_busy();
    asleep = false;
}

static void send_fb(uint8_t reg, int xb0, int y0, int xb1, int y1) {
    set_window((uint16_t)(xb0 << 3), (uint16_t)y0, (uint16_t)((xb1 << 3) | 7), (uint16_t)y1);
    set_cursor((uint16_t)(xb0 << 3), (uint16_t)y0);
    send_cmd(reg);
    dc_data();
    cs_lo();
    for (int y = y0; y <= y1; y++)
        spi_write_blocking(EPD_SPI, fb + y * EPD_PITCH + xb0, (size_t)(xb1 - xb0 + 1));
    cs_hi();
}

static void ready(void) {
    if (asleep) panel_init();
    else wait_busy();
}

static void trigger(uint8_t mode) {
    dirty_x0 = EPD_W;
    dirty_y0 = EPD_H;
    dirty_x1 = -1;
    dirty_y1 = -1;
    send_cmd(CMD_DISP_UPDATE_2);
    send_data1(mode);
    send_cmd(CMD_MASTER_ACTIVATE);
}

void epd_init(void) {
    spi_init(EPD_SPI, EPD_SPI_HZ);
    spi_set_format(EPD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(EPD_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(EPD_PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(EPD_PIN_CS);   gpio_set_dir(EPD_PIN_CS, GPIO_OUT);   gpio_put(EPD_PIN_CS, 1);
    gpio_init(EPD_PIN_DC);   gpio_set_dir(EPD_PIN_DC, GPIO_OUT);   gpio_put(EPD_PIN_DC, 1);
    gpio_init(EPD_PIN_RST);  gpio_set_dir(EPD_PIN_RST, GPIO_OUT);  gpio_put(EPD_PIN_RST, 1);
    gpio_init(EPD_PIN_BUSY); gpio_set_dir(EPD_PIN_BUSY, GPIO_IN);

    memset(fb, 0xFF, EPD_BYTES);

    panel_init();
    send_fb(CMD_WRITE_BW, 0, 0, EPD_PITCH - 1, EPD_H - 1);
    send_fb(CMD_WRITE_RED, 0, 0, EPD_PITCH - 1, EPD_H - 1);
}

void epd_sleep(void) {
    if (asleep) return;
    wait_busy();
    send_cmd(CMD_DEEP_SLEEP);
    send_data1(0x01);
    asleep = true;
}

uint8_t *epd_fb(void) { return fb; }

void epd_clear(bool black) {
    memset(fb, black ? 0x00 : 0xFF, EPD_BYTES);
    epd_mark_dirty(0, 0, EPD_W - 1, EPD_H - 1);
}

void epd_mark_dirty(int x0, int y0, int x1, int y1) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > EPD_W - 1) x1 = EPD_W - 1;
    if (y1 > EPD_H - 1) y1 = EPD_H - 1;
    if (x0 > x1 || y0 > y1) return;
    if (x0 < dirty_x0) dirty_x0 = x0;
    if (y0 < dirty_y0) dirty_y0 = y0;
    if (x1 > dirty_x1) dirty_x1 = x1;
    if (y1 > dirty_y1) dirty_y1 = y1;
}

bool epd_busy(void) {
    return !asleep && gpio_get(EPD_PIN_BUSY);
}

void epd_wait(void) {
    if (!asleep) wait_busy();
}

void epd_refresh_full_async(void) {
    ready();
    send_fb(CMD_WRITE_BW, 0, 0, EPD_PITCH - 1, EPD_H - 1);
    send_fb(CMD_WRITE_RED, 0, 0, EPD_PITCH - 1, EPD_H - 1);
    trigger(UPD_FULL);
}

void epd_refresh_partial_async(void) {
    if (dirty_y1 < dirty_y0) return;
    ready();
    send_fb(CMD_WRITE_BW, dirty_x0 >> 3, dirty_y0, dirty_x1 >> 3, dirty_y1);
    trigger(UPD_PARTIAL);
}

void epd_refresh_full(void) {
    epd_refresh_full_async();
    wait_busy();
}

void epd_refresh_partial(void) {
    epd_refresh_partial_async();
    wait_busy();
}

void epd_set_rotation(int r) { rot = r & 3; }
int  epd_rotation(void) { return rot; }
int  epd_width(void)  { return (rot & 1) ? EPD_H : EPD_W; }
int  epd_height(void) { return (rot & 1) ? EPD_W : EPD_H; }
