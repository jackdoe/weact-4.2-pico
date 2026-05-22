#include "epd.h"
#include "epd_config.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
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
static uint8_t prev_fb[EPD_BYTES];
static const uint8_t ZERO = 0x00;
static int dma_ch;
static bool asleep = false;

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

static void dma_xfer(const void *src, size_t n, bool incr) {
    dc_data();
    cs_lo();
    dma_channel_config cfg = dma_channel_get_default_config(dma_ch);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_dreq(&cfg, spi_get_dreq(EPD_SPI, true));
    channel_config_set_read_increment(&cfg, incr);
    channel_config_set_write_increment(&cfg, false);
    dma_channel_configure(dma_ch, &cfg, &spi_get_hw(EPD_SPI)->dr, src, n, true);
    dma_channel_wait_for_finish_blocking(dma_ch);
    while (spi_is_busy(EPD_SPI)) tight_loop_contents();
    cs_hi();
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

static void wake_if_needed(void) {
    if (asleep) panel_init();
}

static void trigger(uint8_t mode) {
    send_cmd(CMD_DISP_UPDATE_2);
    send_data1(mode);
    send_cmd(CMD_MASTER_ACTIVATE);
    wait_busy();
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

    dma_ch = dma_claim_unused_channel(true);

    memset(fb, 0xFF, EPD_BYTES);
    memset(prev_fb, 0xFF, EPD_BYTES);

    panel_init();
}

void epd_sleep(void) {
    send_cmd(CMD_DEEP_SLEEP);
    send_data1(0x01);
    asleep = true;
}

uint8_t *epd_fb(void) { return fb; }

void epd_clear(bool black) {
    memset(fb, black ? 0x00 : 0xFF, EPD_BYTES);
}

void epd_refresh_full(void) {
    wake_if_needed();

    set_cursor(0, 0);
    send_cmd(CMD_WRITE_BW);
    dma_xfer(fb, EPD_BYTES, true);

    set_cursor(0, 0);
    send_cmd(CMD_WRITE_RED);
    dma_xfer(&ZERO, EPD_BYTES, false);

    trigger(UPD_FULL);

    memcpy(prev_fb, fb, EPD_BYTES);
}

void epd_refresh_partial(void) {
    wake_if_needed();

    set_cursor(0, 0);
    send_cmd(CMD_WRITE_BW);
    dma_xfer(fb, EPD_BYTES, true);

    set_cursor(0, 0);
    send_cmd(CMD_WRITE_RED);
    dma_xfer(prev_fb, EPD_BYTES, true);

    trigger(UPD_PARTIAL);

    memcpy(prev_fb, fb, EPD_BYTES);
}
