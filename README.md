# weact-4.2-pico

A clean SSD1683 driver for the **WeAct 4.2" e-paper** (Seekink E042A87, 400 × 300, 1-bit B/W) on the **Raspberry Pi Pico** / RP2040.

Hardware SPI, DMA frame transfer, dual-RAM differential partial refresh, deep sleep, full font + 1bpp graphics — under 1000 lines of code total.

![Tetris running on WeAct 4.2" e-paper](screenshot.jpg)

## Layout

```
src/                 ← the drop-in library
  epd.h epd.c         driver core (init, refresh, sleep)
  epd_config.h        SPI peripheral + pin assignments
  gfx.h gfx.c         1bpp graphics on the framebuffer
  font8x16.c          96-glyph ASCII font

examples/
  demo/               splash + partial ticker + deep-sleep cycle
  tetris/             playable tetris driven by 8 GPIO keys
```

To use the driver in your own project, copy `src/` and edit `epd_config.h` for your pin map.

## Wiring

| Panel | RP2040    | Note               |
|-------|-----------|--------------------|
| VCC   | 3V3       |                    |
| GND   | GND       |                    |
| SCK   | GP18      | spi0 SCK           |
| MOSI  | GP19      | spi0 TX            |
| CS    | GP20      |                    |
| DC    | GP21      |                    |
| RST   | GP26      |                    |
| BUSY  | GP27      | input, panel drives HIGH while busy |

Edit `src/epd_config.h` to change.

## Build

```sh
export PICO_SDK_PATH=~/path/to/pico-sdk
mkdir build && cd build
cmake .. && make -j
```

Produces `examples/demo/demo.uf2` and `examples/tetris/tetris.uf2`. Hold BOOTSEL while plugging the Pico in, then drag a `.uf2` to `RPI-RP2`.

## API

The entire public surface — six functions, four macros — is in `epd.h` and `gfx.h`.

### Display (`epd.h`)

```c
#define EPD_W      400
#define EPD_H      300
#define EPD_PITCH  (EPD_W / 8)        // 50 bytes per row
#define EPD_BYTES  (EPD_PITCH * EPD_H) // 15000 bytes per frame
```

```c
void     epd_init(void);          // SPI + DMA + GPIO + panel init
void     epd_sleep(void);         // deep sleep (~3 µA); next refresh auto-wakes
uint8_t *epd_fb(void);            // direct framebuffer access (15000 bytes)
void     epd_clear(bool black);   // memset the framebuffer
void     epd_refresh_full(void);  // ~2 s, clears ghosting
void     epd_refresh_partial(void); // ~300 ms, differential against last frame
```

The framebuffer is one contiguous 15000-byte block, 1 bit per pixel, MSB = leftmost pixel, **1 = white, 0 = black** (matches the SSD1683 RAM polarity).

A typical update is just three calls:

```c
epd_clear(false);                 // white background
gfx_text(20, 20, "Hello!", false);
epd_refresh_full();
```

After `epd_sleep()`, any subsequent `epd_refresh_*` will hardware-reset and re-init the panel automatically — call it whenever you're done updating and want to drop to micro-amp idle current.

### Graphics (`gfx.h`)

All shapes write into the framebuffer; nothing reaches the panel until you call a refresh. `black` selects ink (true = black, false = white). For `gfx_char` / `gfx_text`, `invert = false` means black text on white background.

```c
#define GFX_FONT_W 8
#define GFX_FONT_H 16

void gfx_pixel      (int x, int y, bool black);
void gfx_hline      (int x, int y, int w, bool black);
void gfx_vline      (int x, int y, int h, bool black);
void gfx_line       (int x0, int y0, int x1, int y1, bool black);
void gfx_rect       (int x, int y, int w, int h, bool black);
void gfx_fill_rect  (int x, int y, int w, int h, bool black);
void gfx_circle     (int cx, int cy, int r, bool black);
void gfx_fill_circle(int cx, int cy, int r, bool black);
void gfx_char       (int x, int y, char c, bool invert);
void gfx_text       (int x, int y, const char *s, bool invert);
```

`gfx_hline` / `gfx_fill_rect` collapse to `memset` on byte-aligned middle bytes; `gfx_char` has a fast path when `x % 8 == 0` (one byte-write per glyph row). All coordinates are clipped to the panel.

## Refresh strategy

- **Full** (`0x22 = 0xF7`): loads the OTP mode-1 LUT, drives every pixel through the full waveform. ~2 s, no ghosting.
- **Partial** (`0x22 = 0xFF`): loads the OTP mode-2 LUT and runs the differential update — the driver writes the new frame to BW RAM (`0x24`) **and** the previously-displayed frame to RED RAM (`0x26`). Unchanged pixels hit the LUT's "stay" entries and don't move at all; only the actually-different pixels get driven. ~300 ms, no fading on unchanged regions.

Both transfers use one DMA channel streaming straight into the SPI TX FIFO — the 15 KB frame is on the bus in ~12 ms at 20 MHz; the rest of the refresh time is the panel waveform, waited on the `BUSY` pin.

For long-running partial-update sessions (e.g. the Tetris example), schedule an occasional `epd_refresh_full()` — every 30–50 partials is a reasonable rhythm — to clear accumulated waveform drift.

## Credits

Driver written by **Claude Opus 4.7**, tested on real hardware by **[jackdoe](https://github.com/jackdoe)**.

## License

Public domain / do whatever you want.
